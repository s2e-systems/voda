#include "Subscriber.h"

#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>


class VideoListener: public virtual dds::sub::DataReaderListener<S2E::Video>,
                     public virtual dds::sub::NoOpDataReaderListener<S2E::Video> {
public:
    VideoListener(GstAppSrc* const appSrc):
            m_appSrc(appSrc)
    {    }

private:
    virtual void on_data_available(dds::sub::DataReader<S2E::Video>& reader)
    {
        //Check if the GStreamer pipeline is running, by checking the state of the appsrc
        GstState appSrcState = GST_STATE_NULL;
        gst_element_get_state(GST_ELEMENT(m_appSrc), &appSrcState, nullptr /*pending*/, 10/*timeout nanosec*/);
        if (appSrcState != GST_STATE_PLAYING && appSrcState != GST_STATE_PAUSED && appSrcState != GST_STATE_READY)
        {
            return;
        }

        const auto samples = reader.take();

        // It may happen that multiple samples came in in one go, in that
        // case all the samples are pushed into the GStreamer pipeline.
        for (const auto& sample : samples)
        {
            if (sample.info().valid() == false)
            {
                continue;
            }

            std::cout << "received frame " << sample.data().frameNum() << std::endl;

            const auto& frame = sample.data().frame();
            const auto rawDataPtr = reinterpret_cast<const void*>(frame.data());
            const auto byteCount = static_cast<const gsize>(sample.data().frame().size());

            // Copy the arrived memory from the DDS into a GStreamer buffer
            GstMapInfo mapInfo;
            auto gstBuffer = gst_buffer_new_allocate(nullptr /* no allocator */, byteCount, nullptr /* no parameter */);
            gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_WRITE);
            std::memcpy(static_cast<void*>(mapInfo.data), rawDataPtr, byteCount);
            gst_buffer_unmap(gstBuffer, &mapInfo);

            // Push the buffer into the pipeline. Data freeing is now handled by the pipeline
            const auto ret = gst_app_src_push_buffer(m_appSrc, gstBuffer);
            if (ret != GST_FLOW_OK)
            {
                return;
            }
        }
    }

    GstAppSrc* const m_appSrc;
};

static void error_cb(GstBus *, GstMessage *gst_message, std::unique_ptr<MainActivityBinding>& main_activity_binding) {
    GError *error;
    gchar *debug_info;
    gst_message_parse_error(gst_message, &error, &debug_info);
    gchar *const g_message = g_strdup_printf("Error received from element %s: %s",
                                             GST_OBJECT_NAME(gst_message->src), error->message);
    g_clear_error(&error);
    g_free(debug_info);
    main_activity_binding->setUiMessage(g_message);
    g_free(g_message);
}

static void state_changed_cb(GstBus *, GstMessage *gst_message, std::unique_ptr<MainActivityBinding>& main_activity_binding) {
    GstState old_state;
    GstState new_state;
    GstState pending_state;
    gst_message_parse_state_changed(gst_message, &old_state, &new_state, &pending_state);
    // Only show messages coming from the pipeline, not its children
    if (GST_IS_PIPELINE(GST_MESSAGE_SRC(gst_message))) {
        gchar *const g_message = g_strdup_printf("State changed to %s",
                                                 gst_element_state_get_name(new_state));
        main_activity_binding->setUiMessage(g_message);
        g_free(g_message);
    }
}

static void thread_function(GstElement *pipeline, GMainLoop *main_loop, GMainContext *context) {
    g_main_context_push_thread_default(context);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

Subscriber::Subscriber(const dds::domain::DomainParticipant& domain_participant, std::unique_ptr<MainActivityBinding> main_activity_binding) :
        m_data_reader{dds::sub::Subscriber{domain_participant},
                      dds::topic::Topic<S2E::Video>{domain_participant, "VideoStream"},
                      [] {
                          dds::sub::qos::DataReaderQos qos;
                          qos << dds::core::policy::Ownership::Exclusive();
                          qos << dds::core::policy::Liveliness::Automatic();
                          qos << dds::core::policy::History{ dds::core::policy::HistoryKind::KEEP_LAST, 20 };
                          return qos;
                      }()
        },
        m_main_activity_binding{std::move(main_activity_binding)}
{
    GError *error = nullptr;
    m_pipeline = gst_parse_launch("appsrc name=app_src ! openh264dec ! videoconvert ! autovideosink",&error);
    if (error) {
        const std::string error_msg(std::string("Unable to build pipeline: ") + std::string(error->message));
        g_clear_error(&error);
        throw std::runtime_error(error_msg);
    }

    GstBus *bus = gst_element_get_bus(m_pipeline);
    g_signal_connect(G_OBJECT(bus), "message::error", G_CALLBACK(error_cb), &m_main_activity_binding);
    g_signal_connect(G_OBJECT(bus), "message::state-changed", G_CALLBACK(state_changed_cb), &m_main_activity_binding);

    auto src_caps = gst_caps_new_simple("video/x-h264",
                                        "stream-format", G_TYPE_STRING, "byte-stream",
                                        "alignment", G_TYPE_STRING, "au",
                                        "profile", G_TYPE_STRING, "constrained-baseline",
                                        nullptr
    );
    GstElement* app_src = gst_bin_get_by_name(GST_BIN(m_pipeline), "app_src");
    if (!app_src) {
        throw std::runtime_error("Could not retrieve app_src");
    }
    m_video_listener = new VideoListener(GST_APP_SRC_CAST(app_src));
    dds::core::status::StatusMask mask;
    mask << dds::core::status::StatusMask::data_available();
    m_data_reader.listener(m_video_listener, mask);

    g_object_set(app_src,
                 "caps", src_caps,
                 "is-live", true,
                 "format", GST_FORMAT_TIME,
                 nullptr
    );

    //    auto sink_caps = gst_caps_new_simple("video/x-h264",
    //                                         "stream-format", G_TYPE_STRING, "byte-stream",
    //                                        "profile", G_TYPE_STRING, "constrained-baseline",
    //                                         "level", G_TYPE_STRING, "2",
    //                                         nullptr
    //    );
    //    GstElement* caps_filter = gst_bin_get_by_name(GST_BIN(data->pipeline), "caps_filter");
    //    g_object_set(caps_filter, "caps", sink_caps, nullptr);

    // Set the pipeline to READY to receive a video_sink
    gst_element_set_state(m_pipeline, GST_STATE_READY);
    m_context = g_main_context_new();

    GSource *bus_source = gst_bus_create_watch(bus);
    // Instruct the bus to emit signals for each received message
    g_source_set_callback(bus_source, (GSourceFunc) gst_bus_async_signal_func, nullptr,
                          nullptr);
    g_source_attach(bus_source, m_context);
    g_source_unref(bus_source);
    gst_object_unref(bus);

    m_main_loop = g_main_loop_new(m_context, FALSE);
    m_thread = std::thread(thread_function, m_pipeline, m_main_loop, m_context);
}

Subscriber::~Subscriber() {
    g_main_loop_quit((GMainLoop *) m_main_loop);
    m_thread.join();
    delete m_video_listener;
}

GstElement *Subscriber::video_sink() {
    return gst_bin_get_by_interface(GST_BIN(m_pipeline), GST_TYPE_VIDEO_OVERLAY);
}

