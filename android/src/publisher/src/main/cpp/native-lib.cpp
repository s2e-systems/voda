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


GST_DEBUG_CATEGORY_STATIC(debug_category);
#define GST_CAT_DEFAULT debug_category


typedef struct _CustomData
{
    JavaVM* java_vm;
    pthread_t gst_app_thread;
    jobject app;
    GstElement* pipeline;
    GMainLoop* main_loop;
    GstElement* video_sink;
    ANativeWindow* native_window;
} CustomData;


/*
 * Private methods
 */

static JNIEnv* get_jni_env_from_java_vm(JavaVM* java_vm) {
    JNIEnv* jni_env = nullptr;
    java_vm->GetEnv(reinterpret_cast<void**>(&jni_env), JNI_VERSION_1_4);
    return jni_env;
}

static void set_ui_message(const gchar *message, JNIEnv *jni_env, jobject app) {
    jstring jmessage = jni_env->NewStringUTF(message);
    const jmethodID set_message_method_id = jni_env->GetMethodID(jni_env->GetObjectClass(app),
                                                           "setMessage", "(Ljava/lang/String;)V");
    jni_env->CallVoidMethod(app, set_message_method_id, jmessage);
    if (jni_env->ExceptionCheck()) {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer", "Failed to call Java method");
        jni_env->ExceptionClear();
    }
    jni_env->DeleteLocalRef(jmessage);
}

struct ErrorCbData {
    JNIEnv* jni_env;
    jobject object;
};

static void error_cb(GstBus *bus, GstMessage *message, CustomData *data) {
    GError *error;
    gchar *debug_info;
    gst_message_parse_error(message, &error, &debug_info);
    gchar *const message_string = g_strdup_printf("Error received from element %s: %s",
                                                  GST_OBJECT_NAME(message->src), error->message);
    g_clear_error(&error);
    g_free(debug_info);
    set_ui_message(message_string, get_jni_env_from_java_vm(data->java_vm), data->app);
    g_free(message_string);
}

static void state_changed_cb(GstBus *bus, GstMessage *message, CustomData *data) {
    GstState old_state;
    GstState new_state;
    GstState pending_state;
    gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
    // Only show messages coming from the pipeline, not its children
    if (GST_MESSAGE_SRC(message) == GST_OBJECT(data->pipeline)) {
        JNIEnv *const jni_env = get_jni_env_from_java_vm(data->java_vm);
        gchar *const message = g_strdup_printf("State changed to %s",
                                               gst_element_state_get_name(new_state));
        __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "%s", message);
        set_ui_message(message, jni_env, data->app);
        g_free(message);
    }
}


/////////////////////
//  DDS

#include <dds/dds.hpp>
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;

struct DomainParticipant {
    DomainParticipant(): dp{ domain::default_id() } {}
    dds::domain::DomainParticipant dp;
};
template <typename T>
struct DataWriter {
    DataWriter(const dds::pub::Publisher& pub,
        const ::dds::topic::Topic<T>& topic,
        const dds::pub::qos::DataWriterQos& qos,
        dds::pub::DataWriterListener<T>* listener = NULL,
        const dds::core::status::StatusMask& mask = ::dds::core::status::StatusMask::none()):
        data_writerr{ pub, topic, qos, listener, mask } {}

    dds::pub::DataWriter<S2E::Video> data_writerr;
};
DomainParticipant* domain_participant;
DataWriter<S2E::Video>* data_writer;


static GstFlowReturn pullSampleAndSendViaDDS(GstAppSink* appSink, gpointer userData)
{
    auto dataWriter = reinterpret_cast<dds::pub::DataWriter<S2E::Video>*>(userData);
    if (dataWriter == nullptr || dataWriter->is_nil())
    {
        GST_ERROR("DataWriter not valid");
        return GST_FLOW_ERROR;
    }

    const int userid = 0;
    // Count the samples that have arrived. Used also to define the
    // DDS msg number later
    static int frameNum = 0;
    frameNum++;

    // Pull a sample from the GStreamer pipeline
    auto sample = gst_app_sink_pull_sample(appSink);
    if (sample != nullptr)
    {
        auto sampleBuffer = gst_sample_get_buffer(sample);
        if (sampleBuffer != nullptr)
        {
            GstMapInfo mapInfo;
            gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

            const auto byteCount = int(mapInfo.size);
            const auto rawData = static_cast<uint8_t*>(mapInfo.data);
            const dds::core::ByteSeq frame{ rawData, rawData + byteCount };
            *dataWriter << S2E::Video{ userid, frameNum, frame };
            gst_buffer_unmap(sampleBuffer, &mapInfo);
        }
        gst_sample_unref(sample);
    }

    return GST_FLOW_OK;
}


//  END DDS
///////////////////


/* Main method for the native code. This is executed on its own thread. */
static void*
app_function(void* userdata)
{
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "app_function");

    CustomData* data = (CustomData*)userdata;

    JNIEnv* env;
    if (data->java_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        GST_ERROR("Failed to attach current thread");
        return NULL;
    }
    GMainContext* context = g_main_context_new();
    g_main_context_push_thread_default(context);


    GstBus* bus = gst_element_get_bus(data->pipeline);
    GSource* bus_source = gst_bus_create_watch(bus);

    // Instruct the bus to emit signals for each received message
    g_source_set_callback(bus_source, (GSourceFunc)gst_bus_async_signal_func,
                          NULL, NULL);
    g_source_attach(bus_source, context);
    g_source_unref(bus_source);

    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, data);
    g_signal_connect(G_OBJECT(bus), "message::state-changed", (GCallback)state_changed_cb, data);
    gst_object_unref(bus);

    set_ui_message("Entering main loop", env, data->app);
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "Entering main loop");
    data->main_loop = g_main_loop_new(context, FALSE);

    if (data->native_window) {
        __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "gst_video_overlay_set_window_handle in app_function");
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(data->video_sink),
                                        reinterpret_cast<guintptr>(data->native_window));
    }
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "gst_element_set_state GST_STATE_PLAYING in app_function");
    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);

    g_main_loop_run(data->main_loop);
    GST_DEBUG("Exited main loop");
    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;

    /* Free resources */
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
    gst_element_set_state(data->pipeline, GST_STATE_NULL);
    gst_object_unref(data->video_sink);
    gst_object_unref(data->pipeline);

    if (data->java_vm->DetachCurrentThread() != JNI_OK) {
        GST_ERROR("Failed to detach current thread");
        return NULL;
    }
    return NULL;
}

/*
 * Java Bindings
 */

extern "C" JNIEXPORT void JNICALL
nativeCustomDataInit(JNIEnv * env, jobject thiz) {
    CustomData* data = g_new0(CustomData, 1);
    jfieldID custom_data_field_id =
            env->GetFieldID(env->GetObjectClass(thiz), "native_custom_data", "J");
    env->SetLongField(thiz, custom_data_field_id, (jlong)data);
}

 /* Instruct the native code to create its internal data structure, pipeline and thread */
extern "C" JNIEXPORT void JNICALL
nativeLibInit(JNIEnv * env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "nativeLibInit");
    jfieldID custom_data_field_id =
            env->GetFieldID(env->GetObjectClass(thiz), "native_custom_data", "J");
    CustomData* data = (CustomData *)env->GetLongField(thiz, custom_data_field_id);

    GST_DEBUG_CATEGORY_INIT(debug_category, "MyGstreamerBuild", 0,
        "Android MyGstreamerBuild");
    gst_debug_set_threshold_for_name("MyGstreamerBuild", GST_LEVEL_DEBUG);
    GST_DEBUG("Created CustomData at %p", data);

    JavaVM* java_vm;
    env->GetJavaVM(&java_vm);
    data->java_vm = java_vm;

    data->app = env->NewGlobalRef(thiz);
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "Created GlobalRef for app object at %p for %p", data->app, thiz);

    GError* error = NULL;
    data->pipeline =
            gst_parse_launch("ahcsrc device=1 ! video/x-raw,format=NV21 ! videoconvert ! tee name=t ! queue leaky=2 ! autovideosink  t. ! queue leaky=2 ! videoconvert ! openh264enc ! appsink name=app_sink",
                             &error);
    if (error) {
        gchar* message =
                g_strdup_printf("Unable to build pipeline: %s", error->message);
        g_clear_error(&error);
        set_ui_message(message, env, data->app);
        g_free(message);
        return;
    }

    GstElement* app_sink = gst_bin_get_by_name(GST_BIN(data->pipeline), "app_sink");
    if (!app_sink) {
        GST_ERROR("Could not retrieve app_sink");
        return;
    }
    else {
        GST_DEBUG("app_sink found");
    }

    g_object_set(app_sink,
                 "emit-signals", true,
            //                 "caps", ddsSinkCaps,
                 "max-buffers", 1,
                 "drop", false,
                 "sync", false,
                 nullptr
    );

    setenv("CYCLONEDDS_URI", R"(<CycloneDDS><Domain><General><Interfaces>
        <NetworkInterface name="eth1" presence_required="false" />
        <NetworkInterface name="wlan0" presence_required="false" />
        </Interfaces></General></Domain></CycloneDDS>)", 1);

    try {
        const std::string topicName = "VideoStream";

        domain_participant = new DomainParticipant;
        const dds::topic::qos::TopicQos topicQos{ domain_participant->dp.default_topic_qos() };
        const dds::topic::Topic<S2E::Video> topic{ domain_participant->dp, topicName, topicQos };
        const dds::pub::qos::PublisherQos publisherQos{ domain_participant->dp.default_publisher_qos() };
        const dds::pub::Publisher publisher{ domain_participant->dp, publisherQos };
        dds::pub::qos::DataWriterQos dataWriterQos{ topic.qos() };
        dataWriterQos << dds::core::policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
        dataWriterQos << dds::core::policy::Ownership::Exclusive();
        dataWriterQos << dds::core::policy::Liveliness::ManualByTopic(dds::core::Duration::from_millisecs(5000));
        dataWriterQos << dds::core::policy::OwnershipStrength(1000);
        data_writer = new DataWriter<S2E::Video>{ publisher, topic, dataWriterQos };
    }
    catch (const dds::core::Exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, "DDS", "DDS initializaion failed with: %s", e.what());
        return;
    }

    g_signal_connect(app_sink, "new-sample", G_CALLBACK(pullSampleAndSendViaDDS),
                     reinterpret_cast<gpointer>(data_writer));

    /* Set the pipeline to READY, so it can already accept a window handle, if we have one */
    gst_element_set_state(data->pipeline, GST_STATE_READY);

    data->video_sink =
            gst_bin_get_by_interface(GST_BIN(data->pipeline),
                                     GST_TYPE_VIDEO_OVERLAY);
    if (!data->video_sink) {
        GST_ERROR("Could not retrieve video sink");
        return;
    }

    if (data->native_window) {
        __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "gst_video_overlay_set_window_handle in nativeLibInit");
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(data->video_sink),
                                            reinterpret_cast<guintptr>(data->native_window));
    }

    pthread_create(&data->gst_app_thread, NULL, &app_function, data);
}

/* Quit the main loop, remove the native thread and free resources */
extern "C" JNIEXPORT void JNICALL
nativeFinalize(JNIEnv * env, jobject thiz)
{
    jfieldID custom_data_field_id =
            env->GetFieldID(env->GetObjectClass(thiz), "native_custom_data", "J");
    CustomData* data = (CustomData *)env->GetLongField(thiz, custom_data_field_id);
    if (!data)
        return;
    GST_DEBUG("Quitting main loop...");
    g_main_loop_quit(data->main_loop);
    GST_DEBUG("Waiting for thread to finish...");
    pthread_join(data->gst_app_thread, NULL);
    GST_DEBUG("Deleting GlobalRef for app object at %p", data->app);
    env->DeleteGlobalRef(data->app);
    GST_DEBUG("Freeing CustomData at %p", data);
    g_free(data);
    env->SetLongField(thiz, custom_data_field_id, 0);
    delete data_writer;
    delete domain_participant;

    GST_DEBUG("Done finalizing");
}


extern "C" JNIEXPORT void JNICALL
nativeSurfaceInit(JNIEnv * env, jobject thiz, jobject surface)
{
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "nativeSurfaceInit");
    jfieldID custom_data_field_id =
            env->GetFieldID(env->GetObjectClass(thiz), "native_custom_data", "J");
    CustomData* data = (CustomData *)env->GetLongField(thiz, custom_data_field_id);
    if (!data)
        return;
    ANativeWindow* new_native_window = ANativeWindow_fromSurface(env, surface);
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "Received surface %p (native window %p)", surface, new_native_window);

//    if (data->native_window) {
//        ANativeWindow_release(data->native_window);
//        if (data->native_window == new_native_window) {
//            __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "New native window is the same as the previous one %p", data->native_window);
//            if (data->video_sink) {
//                gst_video_overlay_expose(GST_VIDEO_OVERLAY(data->video_sink));
//                gst_video_overlay_expose(GST_VIDEO_OVERLAY(data->video_sink));
//            }
//            return;
//        }
//    }
    data->native_window = new_native_window;

    if (data->video_sink) {
        __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "gst_video_overlay_set_window_handle in nativeSurfaceInit");
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(data->video_sink),
                                            reinterpret_cast<guintptr>(data->native_window));
    }
    __android_log_print(ANDROID_LOG_INFO, "MyGStreamer", "gst_element_set_state GST_STATE_PLAYING");

    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
}


extern "C" JNIEXPORT void JNICALL
nativeSurfaceFinalize(JNIEnv * env, jobject thiz)
{
    jfieldID custom_data_field_id =
            env->GetFieldID(env->GetObjectClass(thiz), "native_custom_data", "J");
    CustomData* data = (CustomData *)env->GetLongField(thiz, custom_data_field_id);
    if (!data)
        return;

    if (data->video_sink) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(data->video_sink),
            (guintptr)NULL);
        gst_element_set_state(data->pipeline, GST_STATE_READY);
    }

    ANativeWindow_release(data->native_window);
    data->native_window = NULL;
}


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    jclass c = env->FindClass("mainactivity/MainActivity");
    if (c == nullptr) return JNI_ERR;

    static const JNINativeMethod methods[] = {
    {"nativeCustomDataInit", "()V", reinterpret_cast<void*>(nativeCustomDataInit)},
    {"nativeLibInit", "()V", reinterpret_cast<void*>(nativeLibInit)},
    {"nativeFinalize", "()V", reinterpret_cast<void*>(nativeFinalize)},
    {"nativeSurfaceInit", "(Ljava/lang/Object;)V", reinterpret_cast<void*>(nativeSurfaceInit)},
    {"nativeSurfaceFinalize", "()V", reinterpret_cast<void*>(nativeSurfaceFinalize)},
    };
    int rc = env->RegisterNatives(c, methods, sizeof(methods)/sizeof(JNINativeMethod));
    if (rc != JNI_OK) return rc;

    return JNI_VERSION_1_4;
}
