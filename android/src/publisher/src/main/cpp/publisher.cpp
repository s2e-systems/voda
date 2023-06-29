#include <jni.h>
#include <gst/gst.h>
#include <string>
#include <android/log.h>

#include <stdint.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/gstbin.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <pthread.h>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;

struct ThreadData {
    GstElement *pipeline;
    GMainLoop *main_loop;
    GMainContext *context;
};


static JNIEnv *get_jni_env_from_java_vm(JavaVM *java_vm) {
    JNIEnv *jni_env = nullptr;
    java_vm->GetEnv(reinterpret_cast<void **>(&jni_env), JNI_VERSION_1_4);
    return jni_env;
}

static void set_ui_message(const gchar *message, JNIEnv *jni_env, jobject app) {
    jstring jmessage = jni_env->NewStringUTF(message);
    const jmethodID set_message_method_id = jni_env->GetMethodID(jni_env->GetObjectClass(app),
                                                                 "setMessage",
                                                                 "(Ljava/lang/String;)V");
    jni_env->CallVoidMethod(app, set_message_method_id, jmessage);
    if (jni_env->ExceptionCheck()) {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer", "Failed to call Java method");
        jni_env->ExceptionClear();
    }
    jni_env->DeleteLocalRef(jmessage);
}

static JavaVM *JAVA_VM;

static void error_cb(GstBus *bus, GstMessage *message, jobject app) {
    GError *error;
    gchar *debug_info;
    gst_message_parse_error(message, &error, &debug_info);
    gchar *const message_string = g_strdup_printf("Error received from element %s: %s",
                                                  GST_OBJECT_NAME(message->src), error->message);
    g_clear_error(&error);
    g_free(debug_info);
    set_ui_message(message_string, get_jni_env_from_java_vm(JAVA_VM), app);
    g_free(message_string);
}

static void state_changed_cb(GstBus *bus, GstMessage *message, jobject app) {
    GstState old_state;
    GstState new_state;
    GstState pending_state;
    gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
    // Only show messages coming from the pipeline, not its children

    if (GST_IS_PIPELINE(GST_MESSAGE_SRC(message))) {
        JNIEnv *const jni_env = get_jni_env_from_java_vm(JAVA_VM);
        gchar *const message = g_strdup_printf("State changed to %s",
                                               gst_element_state_get_name(new_state));
        __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "%s", message);
        set_ui_message(message, jni_env, app);
        g_free(message);
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


/* Main method for the native code. This is executed on its own thread. */
static void *app_function(void *userdata) {
    ThreadData *data = (ThreadData *) userdata;

    JNIEnv *env;
    if (JAVA_VM->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "MyGStreamer", "Failed to attach current thread");
        return NULL;
    }
    g_main_context_push_thread_default(data->context);

    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
    g_main_loop_run(data->main_loop);

    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;
    g_main_context_pop_thread_default(data->context);
    g_main_context_unref(data->context);
    gst_element_set_state(data->pipeline, GST_STATE_NULL);
    gst_object_unref(data->pipeline);

    if (JAVA_VM->DetachCurrentThread() != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "MyGStreamer", "Failed to detach current thread");
        return NULL;
    }
    return NULL;
}


jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JAVA_VM = vm;
    return JNI_VERSION_1_4;
}

class Publisher {
    dds::pub::DataWriter<S2E::Video> m_data_writer;
    jobject m_java_global_ref;
    JNIEnv *m_env;
    ThreadData *m_data;
    pthread_t m_gst_app_thread;

public:
    Publisher(JNIEnv *env, const jobject java_obj, dds::domain::DomainParticipant domain_participant) :
            m_env{env},
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
            }
        {
        m_java_global_ref = env->NewGlobalRef(java_obj);
        GError *error = nullptr;

        m_data = new ThreadData;

        m_data->pipeline = gst_parse_launch(
                "ahcsrc device=1 ! video/x-raw,format=NV21 ! videoconvert ! tee name=t ! queue leaky=2 ! autovideosink  t. ! queue leaky=2 ! videoconvert ! openh264enc ! appsink name=app_sink",
                &error);
        if (error) {
            __android_log_print(ANDROID_LOG_ERROR, "MyGStreamer", "gst_parse_launch failed");
            gchar *message = g_strdup_printf("Unable to build pipeline: %s", error->message);
            g_clear_error(&error);
            set_ui_message(message, env, m_java_global_ref);
            g_free(message);
        }

        GstBus *bus = gst_element_get_bus(m_data->pipeline);
        g_signal_connect(G_OBJECT(bus), "message::error", (GCallback) error_cb, m_java_global_ref);
        g_signal_connect(G_OBJECT(bus), "message::state-changed", (GCallback) state_changed_cb,
                         m_java_global_ref);

        GstElement *app_sink = gst_bin_get_by_name(GST_BIN(m_data->pipeline), "app_sink");
        if (!app_sink) {
            __android_log_print(ANDROID_LOG_ERROR, "MyGStreamer", "Could not retrieve app_sink");
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
        gst_element_set_state(m_data->pipeline, GST_STATE_READY);
        m_data->context = g_main_context_new();

        GSource *bus_source = gst_bus_create_watch(bus);
        // Instruct the bus to emit signals for each received message
        g_source_set_callback(bus_source, (GSourceFunc) gst_bus_async_signal_func, nullptr,
                              nullptr);
        g_source_attach(bus_source, m_data->context);
        g_source_unref(bus_source);
        gst_object_unref(bus);

        m_data->main_loop = g_main_loop_new(m_data->context, FALSE);
        pthread_create(&m_gst_app_thread, nullptr, &app_function, (void *) m_data);
    }

    virtual ~Publisher() {
        g_main_loop_quit((GMainLoop *) m_data->main_loop);
        pthread_join(m_gst_app_thread, nullptr);
        delete m_data;

        m_env->DeleteGlobalRef(m_java_global_ref);
    };

    GstElement *video_sink() {
        return gst_bin_get_by_interface(GST_BIN(m_data->pipeline), GST_TYPE_VIDEO_OVERLAY);
    }
};


Publisher *NATIVE_PUBLISHER = nullptr;

extern "C"
JNIEXPORT jlong JNICALL
Java_mainactivity_MainActivity_nativePublisherInit(JNIEnv *env, jobject thiz) {

    setenv("CYCLONEDDS_URI", R"(<CycloneDDS><Domain><General><Interfaces>
        <NetworkInterface name="eth1" presence_required="false" />
        <NetworkInterface name="wlan0" presence_required="false" />
        </Interfaces></General></Domain></CycloneDDS>)", 1);

    try {
        NATIVE_PUBLISHER = new Publisher(env, thiz,
                                         dds::domain::DomainParticipant{domain::default_id()});
    } catch (const dds::core::Exception &e) {
        __android_log_print(ANDROID_LOG_ERROR, "DDS", "DDS initializaion failed with: %s",
                            e.what());
    }
    return reinterpret_cast<jlong>(NATIVE_PUBLISHER->video_sink());
}

extern "C"
JNIEXPORT void JNICALL
Java_mainactivity_MainActivity_nativePublisherFinalize__(JNIEnv *, jobject) {
    delete NATIVE_PUBLISHER;
}

extern "C"
JNIEXPORT void JNICALL
Java_mainactivity_SurfaceHolderCallback_nativeSurfaceInit(JNIEnv *env, jobject,
                                                          jobject surface, jlong video_sink) {
    auto native_window = reinterpret_cast<guintptr>(ANativeWindow_fromSurface(env, surface));
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), native_window);
}

extern "C"
JNIEXPORT void JNICALL
Java_mainactivity_SurfaceHolderCallback_nativeSurfaceFinalize(JNIEnv *env, jobject,
                                                              jobject surface) {
    ANativeWindow_release(ANativeWindow_fromSurface(env, surface));
}