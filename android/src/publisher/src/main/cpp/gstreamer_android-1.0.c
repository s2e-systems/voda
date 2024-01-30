#include <jni.h>
#include <gst/gst.h>
#include <gio/gio.h>
#include <android/log.h>
#include <string.h>

static jobject _context = NULL;
static jobject _class_loader = NULL;
static JavaVM *_java_vm = NULL;

/* Declaration of static plugins */
GST_PLUGIN_STATIC_DECLARE(androidmedia);
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(autodetect);
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(openh264);
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(videofilter);
GST_PLUGIN_STATIC_DECLARE(videotestsrc);
GST_PLUGIN_STATIC_DECLARE(videorate);
/* Declaration of static gio modules */

/* This is called by gst_init() to register static plugins */
void gst_init_static_plugins(void)
{
    GST_PLUGIN_STATIC_REGISTER(androidmedia);
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(openh264);
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
    GST_PLUGIN_STATIC_REGISTER(videofilter);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(videorate);
}

jobject
gst_android_get_application_context(void)
{
    return _context;
}

jobject
gst_android_get_application_class_loader(void)
{
    return _class_loader;
}

JavaVM *
gst_android_get_java_vm(void)
{
    return _java_vm;
}


JNIEXPORT void JNICALL
Java_org_freedesktop_gstreamer_GStreamer_nativeInit(JNIEnv *env, jobject gstreamer, jobject context)
{
    GError *error = NULL;
    jmethodID get_class_loader_id = 0;

    jclass context_cls = (*env)->GetObjectClass(env, context);
    get_class_loader_id = (*env)->GetMethodID(env, context_cls,
                                              "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject class_loader = (*env)->CallObjectMethod(env, context, get_class_loader_id);
    _context = (*env)->NewGlobalRef(env, context);
    _class_loader = (*env)->NewGlobalRef(env, class_loader);

    gst_debug_set_active(TRUE);
    gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    gst_debug_remove_log_function(gst_debug_log_default);

    if (!gst_init_check(NULL, NULL, &error))
    {
        gchar *message = g_strdup_printf("GStreamer initialization failed: %s",
                                         error && error->message ? error->message : "(no message)");
        jclass exception_class = (*env)->FindClass(env, "java/lang/Exception");
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer", "%s", message);
        (*env)->ThrowNew(env, exception_class, message);
        g_free(message);
        return;
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    GModule *module;

    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer",
                            "Could not retrieve JNIEnv");
        return -1;
    }

    /* Remember Java VM */
    _java_vm = vm;

    /* Tell the androidmedia plugin about the Java VM if we can */
    module = g_module_open(NULL, G_MODULE_BIND_LOCAL);
    if (module)
    {
        void (*set_java_vm)(JavaVM *) = NULL;

        if (g_module_symbol(module, "gst_amc_jni_set_java_vm",
                            (gpointer *)&set_java_vm) &&
            set_java_vm)
        {
            set_java_vm(vm);
        }
        g_module_close(module);
    }

    return JNI_VERSION_1_4;
}

void JNI_OnUnload(JavaVM *vm, void *reversed)
{
    JNIEnv *env = NULL;

    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer",
                            "Could not retrieve JNIEnv");
        return;
    }

    if (_context)
    {
        (*env)->DeleteGlobalRef(env, _context);
        _context = NULL;
    }

    if (_class_loader)
    {
        (*env)->DeleteGlobalRef(env, _class_loader);
        _class_loader = NULL;
    }

    _java_vm = NULL;
}
