#include "Publisher.h"

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/gstbin.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"
#include <string>
#include <thread>
#include "Publisher.h"


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

static GstFlowReturn pullSampleAndSendViaDDS(GstAppSink *appSink, gpointer userData) {
    auto dataWriter = reinterpret_cast<dds::pub::DataWriter<S2E::Video> *>(userData);
    if (dataWriter == nullptr || dataWriter->is_nil()) {
        __android_log_print(ANDROID_LOG_ERROR, "DDS", "DataWriter not valid");
        return GST_FLOW_ERROR;
    }

    const int userid = 0;
    static int frameNum = 0;
    frameNum++;

    // Pull a sample from the GStreamer pipeline
    auto sample = gst_app_sink_pull_sample(appSink);
    if (sample != nullptr) {
        auto sampleBuffer = gst_sample_get_buffer(sample);
        if (sampleBuffer != nullptr) {
            GstMapInfo mapInfo;
            gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

            const auto byteCount = int(mapInfo.size);
            const auto rawData = static_cast<uint8_t *>(mapInfo.data);
            const dds::core::ByteSeq frame{rawData, rawData + byteCount};
            *dataWriter << S2E::Video{userid, frameNum, frame};
            gst_buffer_unmap(sampleBuffer, &mapInfo);
        }
        gst_sample_unref(sample);
    }

    return GST_FLOW_OK;
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

Publisher::Publisher(const dds::domain::DomainParticipant& domain_participant, std::unique_ptr<MainActivityBinding> main_activity_binding) :
        m_data_writer{dds::pub::Publisher{domain_participant},
                      dds::topic::Topic<S2E::Video>{domain_participant, "VideoStream"},
                      [] {
                          dds::pub::qos::DataWriterQos dataWriterQos;
                          dataWriterQos
                                  << dds::core::policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
                          dataWriterQos << dds::core::policy::Ownership::Exclusive();
                          dataWriterQos << dds::core::policy::Liveliness::ManualByTopic(
                                  dds::core::Duration::from_millisecs(5000));
                          dataWriterQos << dds::core::policy::OwnershipStrength(1000);
                          return dataWriterQos;
                      }(),
                      nullptr,
                      dds::core::status::StatusMask::none()
        },
        m_main_activity_binding{std::move(main_activity_binding)}
{
    GError *error = nullptr;
    m_pipeline = gst_parse_launch(
            "ahcsrc device=1 ! video/x-raw,format=NV21 ! videoconvert ! "
            "tee name=t ! queue leaky=2 ! autovideosink  "
            "t. ! queue leaky=2 ! videoconvert ! openh264enc ! appsink name=app_sink",
            &error);
    if (error) {
        const std::string error_msg(std::string("Unable to build pipeline: ") + std::string(error->message));
        g_clear_error(&error);
        throw std::runtime_error(error_msg);
    }

    GstBus *bus = gst_element_get_bus(m_pipeline);
    g_signal_connect(G_OBJECT(bus), "message::error", G_CALLBACK(error_cb), &m_main_activity_binding);
    g_signal_connect(G_OBJECT(bus), "message::state-changed", G_CALLBACK(state_changed_cb), &m_main_activity_binding);

    GstElement *app_sink = gst_bin_get_by_name(GST_BIN(m_pipeline), "app_sink");
    if (!app_sink) {
        throw std::runtime_error("Could not retrieve app_sink");
    }

    g_object_set(app_sink,
                 "emit-signals", true,
                 "max-buffers", 1,
                 "drop", false,
                 "sync", false,
                 nullptr
    );
    g_signal_connect(app_sink, "new-sample", G_CALLBACK(pullSampleAndSendViaDDS),
                     reinterpret_cast<gpointer>(&m_data_writer));

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
    m_thread = new std::thread(thread_function, m_pipeline, m_main_loop, m_context);
}

Publisher::~Publisher() {
    g_main_loop_quit((GMainLoop *) m_main_loop);
    m_thread->join();
}

GstElement *Publisher::video_sink() {
    return gst_bin_get_by_interface(GST_BIN(m_pipeline), GST_TYPE_VIDEO_OVERLAY);
}

