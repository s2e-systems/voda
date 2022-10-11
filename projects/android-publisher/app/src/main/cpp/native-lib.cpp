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

//#include <dds/dds.hpp>
//#include "VideoDDS.hpp"
//
//using namespace org::eclipse::cyclonedds;

extern "C" JNIEXPORT jstring JNICALL
Java_mainactivity_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    __android_log_print (ANDROID_LOG_INFO, "GStreamer",
                         "Called stringFromJNI of native-lib.cpp");
    char *version_utf8 = gst_version_string();
    jstring version_jstring = env->NewStringUTF(version_utf8);
    g_free(version_utf8);
    if (gst_is_initialized()) {
        return version_jstring;
    } else {
        return env->NewStringUTF("not initialized");
    }
}

/* List of implemented native methods */
//static JNINativeMethod native_methods[] = {
//        {"nativeFinalize", "()V", (void *) gst_native_finalize},
//        {"nativePlay", "()V", (void *) gst_native_play},
//        {"nativePause", "()V", (void *) gst_native_pause},
//        {"nativeSurfaceInit", "(Ljava/lang/Object;)V",
//                (void *) gst_native_surface_init},
//        {"nativeSurfaceFinalize", "()V", (void *) gst_native_surface_finalize},
//        {"nativeClassInit", "()Z", (void *) gst_native_class_init}
//};


GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category

/*
 * These macros provide a way to store the native pointer to CustomData, which might be 32 or 64 bits, into
 * a jlong, which is always 64 bits, without warnings.
 */
#if GLIB_SIZEOF_VOID_P == 8
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)env->GetLongField(thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) env->SetLongField(thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)env->GetLongField(thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) env->SetLongField(thiz, fieldID, (jlong)(jint)data)
#endif

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData
{
    jobject app;                  /* Application instance, used to call its methods. A global reference is kept. */
    GstElement *pipeline;         /* The running pipeline */
    GMainContext *context;        /* GLib context used to run the main loop */
    GMainLoop *main_loop;         /* GLib main loop */
    gboolean initialized;         /* To avoid informing the UI multiple times about the initialization */
    GstElement *video_sink;       /* The video sink element which receives XOverlay commands */
    ANativeWindow *native_window; /* The Android native window where video will be rendered */
} CustomData;

/* These global variables cache values which are not changing during execution */
static pthread_t gst_app_thread;
static pthread_key_t current_jni_env;
static JavaVM *java_vm;
static jfieldID custom_data_field_id;
static jmethodID set_message_method_id;
static jmethodID on_gstreamer_initialized_method_id;

/*
 * Private methods
 */

/* Register this thread with the VM */
static JNIEnv *
attach_current_thread (void)
{
    JNIEnv *env;
    JavaVMAttachArgs args;

    GST_DEBUG ("Attaching thread %p", g_thread_self ());
    args.version = JNI_VERSION_1_4;
    args.name = NULL;
    args.group = NULL;

    if (java_vm->AttachCurrentThread (&env, &args) < 0) {
        GST_ERROR ("Failed to attach current thread");
        return NULL;
    }

    return env;
}

/* Unregister this thread from the VM */
static void
detach_current_thread (void *env)
{
    GST_DEBUG ("Detaching thread %p", g_thread_self ());
    java_vm->DetachCurrentThread();
}

/* Retrieve the JNI environment for this thread */
static JNIEnv *
get_jni_env (void)
{
    JNIEnv *env = (JNIEnv *)pthread_getspecific (current_jni_env);

    if (env == NULL) {
        env = attach_current_thread ();
        pthread_setspecific (current_jni_env, env);
    }

    return env;
}

/* Change the content of the UI's TextView */
static void
set_ui_message (const gchar * message, CustomData * data)
{
    JNIEnv *env = get_jni_env ();
    GST_DEBUG ("Setting message to: %s", message);
    jstring jmessage = env->NewStringUTF(message);
    env->CallVoidMethod(data->app, set_message_method_id, jmessage);
    if (env->ExceptionCheck()) {
        GST_ERROR ("Failed to call Java method");
        env->ExceptionClear();
    }
    env->DeleteLocalRef(jmessage);
}

/* Retrieve errors from the bus and show them on the UI */
static void
error_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
    GError *err;
    gchar *debug_info;
    gchar *message_string;

    gst_message_parse_error (msg, &err, &debug_info);
    message_string =
            g_strdup_printf ("Error received from element %s: %s",
                             GST_OBJECT_NAME (msg->src), err->message);
    g_clear_error (&err);
    g_free (debug_info);
    set_ui_message (message_string, data);
    g_free (message_string);
    gst_element_set_state (data->pipeline, GST_STATE_NULL);
}

/* Notify UI about pipeline state changes */
static void
state_changed_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    /* Only pay attention to messages coming from the pipeline, not its children */
    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        gchar *message = g_strdup_printf ("State changed to %s",
                                          gst_element_state_get_name (new_state));
        set_ui_message (message, data);
        g_free (message);
    }
}

/* Check if all conditions are met to report GStreamer as initialized.
 * These conditions will change depending on the application */
static void
check_initialization_complete (CustomData * data)
{
    JNIEnv *env = get_jni_env ();
    if (!data->initialized && data->native_window && data->main_loop) {
        GST_DEBUG
        ("Initialization complete, notifying application. native_window:%p main_loop:%p",
         data->native_window, data->main_loop);

        /* The main loop is running and we received a native window, inform the sink about it */
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->video_sink),
                                             (guintptr) data->native_window);

        env->CallVoidMethod(data->app, on_gstreamer_initialized_method_id);
        if (env->ExceptionCheck()) {
            GST_ERROR ("Failed to call Java method");
            env->ExceptionClear();
        }
        data->initialized = TRUE;
    }
}




/////////////////////
//  DDS
//
#include <dds/dds.hpp>
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;

struct DDSwrapper {
    DDSwrapper(const std::string topicName = "VideoStream") :
            dp{domain::default_id()},
            topicQos{dp.default_topic_qos()},
            topic{dp, topicName, topicQos},
            publisher{dp},
            dataWriter{publisher, topic}
    {

    }
    dds::domain::DomainParticipant dp;
    const dds::topic::qos::TopicQos topicQos;
    const dds::topic::Topic<S2E::Video> topic;
    const dds::pub::Publisher publisher;
    dds::pub::DataWriter<S2E::Video> dataWriter;
};

DDSwrapper* dds_wrapper = nullptr;

static GstFlowReturn pullSampleAndSendViaDDS(GstAppSink* appSink, gpointer userData)
{
    auto dataWriter = reinterpret_cast<dds::pub::DataWriter<S2E::Video>* >(userData);
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
    if(sample != nullptr)
    {
        auto sampleBuffer = gst_sample_get_buffer(sample);
        if(sampleBuffer != nullptr)
        {
            GstMapInfo mapInfo;
            gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

            const auto byteCount = int(mapInfo.size);
            const auto rawData = static_cast<uint8_t* >(mapInfo.data);
            const dds::core::ByteSeq frame{rawData, rawData + byteCount};
            *dataWriter << S2E::Video{userid, frameNum, frame};
            gst_buffer_unmap(sampleBuffer, &mapInfo);
        }
        gst_sample_unref(sample);
    }

    return GST_FLOW_OK;
}


//  END DDS
///////////////////


/* Main method for the native code. This is executed on its own thread. */
static void *
app_function (void *userdata)
{
    JavaVMAttachArgs args;
    GstBus *bus;
    CustomData *data = (CustomData *) userdata;
    GSource *bus_source;
    GError *error = NULL;

    GST_DEBUG ("Creating pipeline in CustomData at %p", data);

    /* Create our own GLib Main Context and make it the default one */
    data->context = g_main_context_new ();
    g_main_context_push_thread_default (data->context);

    /* Build pipeline */
    data->pipeline =
            gst_parse_launch("ahcsrc ! videoconvert ! tee name=t ! queue ! autovideosink  t. ! queue ! openh264enc ! appsink name=app_sink",
                              &error);
    if (error) {
        gchar *message =
                g_strdup_printf ("Unable to build pipeline: %s", error->message);
        g_clear_error (&error);
        set_ui_message (message, data);
        g_free (message);
        return NULL;
    }

    GstElement* app_sink = gst_bin_get_by_name(GST_BIN(data->pipeline), "app_sink");
    if (!app_sink) {
        GST_ERROR ("Could not retrieve app_sink");
        return NULL;
    } else{
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

    putenv("CYCLONEDDS_URI=<CycloneDDS><Domain><General><Interfaces><NetworkInterface name=\"eth1\" /></Interfaces></General></Domain></CycloneDDS>");
    try {
        dds_wrapper = new DDSwrapper{};
    } catch (...) {
        GST_ERROR ("Could not create DDS stuff");
        return NULL;
    }

    g_signal_connect(app_sink, "new-sample", G_CALLBACK(pullSampleAndSendViaDDS),
                     reinterpret_cast<gpointer>(&dds_wrapper->dataWriter));

    /* Set the pipeline to READY, so it can already accept a window handle, if we have one */
    gst_element_set_state (data->pipeline, GST_STATE_READY);

    data->video_sink =
            gst_bin_get_by_interface (GST_BIN (data->pipeline),
                                      GST_TYPE_VIDEO_OVERLAY);
    if (!data->video_sink) {
        GST_ERROR ("Could not retrieve video sink");
        return NULL;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus (data->pipeline);
    bus_source = gst_bus_create_watch (bus);
    g_source_set_callback (bus_source, (GSourceFunc) gst_bus_async_signal_func,
                           NULL, NULL);
    g_source_attach (bus_source, data->context);
    g_source_unref (bus_source);
    g_signal_connect (G_OBJECT (bus), "message::error", (GCallback) error_cb,
                      data);
    g_signal_connect (G_OBJECT (bus), "message::state-changed",
                      (GCallback) state_changed_cb, data);
    gst_object_unref (bus);

    /* Create a GLib Main Loop and set it to run */
    GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
    data->main_loop = g_main_loop_new (data->context, FALSE);
    check_initialization_complete (data);
    g_main_loop_run (data->main_loop);
    GST_DEBUG ("Exited main loop");
    g_main_loop_unref (data->main_loop);
    data->main_loop = NULL;

    /* Free resources */
    g_main_context_pop_thread_default (data->context);
    g_main_context_unref (data->context);
    gst_element_set_state (data->pipeline, GST_STATE_NULL);
    gst_object_unref (data->video_sink);
    gst_object_unref (data->pipeline);

    return NULL;
}

/*
 * Java Bindings
 */

/* Instruct the native code to create its internal data structure, pipeline and thread */
extern "C" JNIEXPORT void JNICALL
        Java_mainactivity_MainActivity_nativeInit (JNIEnv * env, jobject thiz)
{
    CustomData *data = g_new0 (CustomData, 1);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
    GST_DEBUG_CATEGORY_INIT (debug_category, "MyGstreamerBuild", 0,
                             "Android MyGstreamerBuild");
    gst_debug_set_threshold_for_name ("MyGstreamerBuild", GST_LEVEL_DEBUG);
    GST_DEBUG ("Created CustomData at %p", data);
    data->app = env->NewGlobalRef(thiz);
    GST_DEBUG ("Created GlobalRef for app object at %p", data->app);
    pthread_create (&gst_app_thread, NULL, &app_function, data);
}

/* Quit the main loop, remove the native thread and free resources */
extern "C" JNIEXPORT void JNICALL
Java_mainactivity_MainActivity_nativeFinalize(JNIEnv * env, jobject thiz)
{
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data)
        return;
    GST_DEBUG ("Quitting main loop...");
    g_main_loop_quit (data->main_loop);
    GST_DEBUG ("Waiting for thread to finish...");
    pthread_join (gst_app_thread, NULL);
    GST_DEBUG ("Deleting GlobalRef for app object at %p", data->app);
    env->DeleteGlobalRef(data->app);
    GST_DEBUG ("Freeing CustomData at %p", data);
    g_free (data);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
    GST_DEBUG ("Done finalizing");
}

/* Set pipeline to PLAYING state */
extern "C" JNIEXPORT void JNICALL
Java_mainactivity_MainActivity_nativePlay(JNIEnv * env, jobject thiz)
{
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data)
        return;
    GST_DEBUG ("Setting state to PLAYING");
    gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

/* Set pipeline to PAUSED state */
extern "C" JNIEXPORT void JNICALL
Java_mainactivity_MainActivity_nativePause (JNIEnv * env, jobject thiz)
{
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data)
        return;
    GST_DEBUG ("Setting state to PAUSED");
    gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
}


/* Static class initializer: retrieve method and field IDs */
extern "C" JNIEXPORT jboolean JNICALL
Java_mainactivity_MainActivity_nativeClassInit(JNIEnv * env, jclass klass)
{
    custom_data_field_id =
            env->GetFieldID(klass, "native_custom_data", "J");
    set_message_method_id =
            env->GetMethodID(klass, "setMessage", "(Ljava/lang/String;)V");
    on_gstreamer_initialized_method_id =
            env->GetMethodID(klass, "onGStreamerInitialized", "()V");

    if (!custom_data_field_id || !set_message_method_id
        || !on_gstreamer_initialized_method_id) {
        /* We emit this message through the Android log instead of the GStreamer log because the later
         * has not been initialized yet.
         */
        __android_log_print (ANDROID_LOG_ERROR, "MyGstreamerBuild",
                             "The calling class does not implement all necessary interface methods");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_mainactivity_MainActivity_nativeSurfaceInit(JNIEnv * env, jobject thiz, jobject surface)
{
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data)
        return;
    ANativeWindow *new_native_window = ANativeWindow_fromSurface (env, surface);
    GST_DEBUG ("Received surface %p (native window %p)", surface,
               new_native_window);

    if (data->native_window) {
        ANativeWindow_release (data->native_window);
        if (data->native_window == new_native_window) {
            GST_DEBUG ("New native window is the same as the previous one %p",
                       data->native_window);
            if (data->video_sink) {
                gst_video_overlay_expose (GST_VIDEO_OVERLAY (data->video_sink));
                gst_video_overlay_expose (GST_VIDEO_OVERLAY (data->video_sink));
            }
            return;
        } else {
            GST_DEBUG ("Released previous native window %p", data->native_window);
            data->initialized = FALSE;
        }
    }
    data->native_window = new_native_window;

    check_initialization_complete (data);
}


extern "C" JNIEXPORT void JNICALL
Java_mainactivity_MainActivity_nativeSurfaceFinalize(JNIEnv * env, jobject thiz)
{
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data)
        return;
    GST_DEBUG ("Releasing Native Window %p", data->native_window);

    if (data->video_sink) {
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->video_sink),
                                             (guintptr) NULL);
        gst_element_set_state (data->pipeline, GST_STATE_READY);
    }

    ANativeWindow_release (data->native_window);
    data->native_window = NULL;
    data->initialized = FALSE;
}

/* Library initializer */
jint
JNI_OnLoad (JavaVM * vm, void *reserved)
{
    JNIEnv *env = NULL;

    java_vm = vm;

    if (vm->GetEnv ((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print (ANDROID_LOG_ERROR, "MyGstreamerBuild",
                             "Could not retrieve JNIEnv");
        return 0;
    }

    putenv("GST_DEBUG=3");

//    jclass klass = (*env)->FindClass (env,
//                                      "org/freedesktop/gstreamer/tutorials/tutorial_3/Tutorial3");
//    (*env)->RegisterNatives (env, klass, native_methods,
//                             G_N_ELEMENTS (native_methods));

    pthread_key_create (&current_jni_env, detach_current_thread);

    return JNI_VERSION_1_4;
}
