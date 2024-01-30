#include <jni.h>
#include <gst/gst.h>

static jobject CONTEXT = NULL;
static jobject CLASS_LOADER = NULL;
static JavaVM *JAVA_VM = NULL;

/* Declaration of static plugins */
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(autodetect);
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(openh264);
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(libav);

/* This is called by gst_init() to register static plugins */
void gst_init_static_plugins(void)
{
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(openh264);
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
    GST_PLUGIN_STATIC_REGISTER(libav);
}

/* AMC plugin uses g_module_symbol() to find this symbol */
jobject
gst_android_get_application_context(void)
{
    return CONTEXT;
}

/* AMC plugin uses g_module_symbol() to find this symbol */
jobject
gst_android_get_application_class_loader(void)
{
    return CLASS_LOADER;
}

/* AMC plugin uses g_module_symbol() to find this symbol */
JavaVM *
gst_android_get_java_vm(void)
{
    return JAVA_VM;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_gstreamer_GStreamer_nativeInit(JNIEnv *env, jobject gstreamer, jobject context)
{
    GError *error = NULL;
    CONTEXT = (*env)->NewGlobalRef(env, context);
    jclass context_cls = (*env)->GetObjectClass(env, context);
    jmethodID get_class_loader_id = (*env)->GetMethodID(env, context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject class_loader = (*env)->CallObjectMethod(env, context, get_class_loader_id);

    CLASS_LOADER = (*env)->NewGlobalRef(env, class_loader);

    if (!gst_init_check(NULL, NULL, &error))
    {
        gchar *message = g_strdup_printf("GStreamer initialization failed: %s",
                                         error && error->message ? error->message : "(no message)");
        jclass exception_class = (*env)->FindClass(env, "java/lang/Exception");
        (*env)->ThrowNew(env, exception_class, message);
        g_free(message);
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JAVA_VM = vm;
    return JNI_VERSION_1_4;
}

void JNI_OnUnload(JavaVM *vm, void *reversed)
{
    JNIEnv *env = NULL;
    (*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_4);
    (*env)->DeleteGlobalRef(env, CLASS_LOADER);
    (*env)->DeleteGlobalRef(env, CONTEXT);
}