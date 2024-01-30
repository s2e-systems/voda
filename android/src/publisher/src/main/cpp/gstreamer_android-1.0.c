#include <jni.h>
#include <gst/gst.h>
#include <gio/gio.h>
#include <android/log.h>
#include <string.h>


static jobject _context = NULL;
static jobject _class_loader = NULL;
static JavaVM *_java_vm = NULL;
static GstClockTime _priv_gst_info_start_time;


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

static void
gst_debug_logcat(GstDebugCategory *category, GstDebugLevel level,
                 const gchar *file, const gchar *function, gint line,
                 GObject *object, GstDebugMessage *message, gpointer unused)
{
    GstClockTime elapsed;
    gint android_log_level;
    gchar *tag;

    if (level > gst_debug_category_get_threshold(category))
        return;

    elapsed = GST_CLOCK_DIFF(_priv_gst_info_start_time,
                             gst_util_get_timestamp());

    switch (level)
    {
    case GST_LEVEL_ERROR:
        android_log_level = ANDROID_LOG_ERROR;
        break;
    case GST_LEVEL_WARNING:
        android_log_level = ANDROID_LOG_WARN;
        break;
    case GST_LEVEL_INFO:
        android_log_level = ANDROID_LOG_INFO;
        break;
    case GST_LEVEL_DEBUG:
        android_log_level = ANDROID_LOG_DEBUG;
        break;
    default:
        android_log_level = ANDROID_LOG_VERBOSE;
        break;
    }

    tag = g_strdup_printf("GStreamer+%s",
                          gst_debug_category_get_name(category));

    if (object)
    {
        gchar *obj;

        if (GST_IS_PAD(object) && GST_OBJECT_NAME(object))
        {
            obj = g_strdup_printf("<%s:%s>", GST_DEBUG_PAD_NAME(object));
        }
        else if (GST_IS_OBJECT(object) && GST_OBJECT_NAME(object))
        {
            obj = g_strdup_printf("<%s>", GST_OBJECT_NAME(object));
        }
        else if (G_IS_OBJECT(object))
        {
            obj = g_strdup_printf("<%s@%p>", G_OBJECT_TYPE_NAME(object), object);
        }
        else
        {
            obj = g_strdup_printf("<%p>", object);
        }

        __android_log_print(android_log_level, tag,
                            "%" GST_TIME_FORMAT " %p %s:%d:%s:%s %s\n",
                            GST_TIME_ARGS(elapsed), g_thread_self(),
                            file, line, function, obj, gst_debug_message_get(message));

        g_free(obj);
    }
    else
    {
        __android_log_print(android_log_level, tag,
                            "%" GST_TIME_FORMAT " %p %s:%d:%s %s\n",
                            GST_TIME_ARGS(elapsed), g_thread_self(),
                            file, line, function, gst_debug_message_get(message));
    }
    g_free(tag);
}

static gboolean
get_application_dirs(JNIEnv *env, jobject context, gchar **cache_dir,
                     gchar **files_dir)
{
    jclass context_class;
    jmethodID get_cache_dir_id, get_files_dir_id;
    jclass file_class;
    jmethodID get_absolute_path_id;
    jobject dir;
    jstring abs_path;
    const gchar *abs_path_str;

    *cache_dir = *files_dir = NULL;

    context_class = (*env)->GetObjectClass(env, context);
    if (!context_class)
    {
        return FALSE;
    }
    get_cache_dir_id =
        (*env)->GetMethodID(env, context_class, "getCacheDir",
                            "()Ljava/io/File;");
    get_files_dir_id =
        (*env)->GetMethodID(env, context_class, "getFilesDir",
                            "()Ljava/io/File;");
    if (!get_cache_dir_id || !get_files_dir_id)
    {
        (*env)->DeleteLocalRef(env, context_class);
        return FALSE;
    }

    file_class = (*env)->FindClass(env, "java/io/File");
    if (!file_class)
    {
        (*env)->DeleteLocalRef(env, context_class);
        return FALSE;
    }
    get_absolute_path_id =
        (*env)->GetMethodID(env, file_class, "getAbsolutePath",
                            "()Ljava/lang/String;");
    if (!get_absolute_path_id)
    {
        (*env)->DeleteLocalRef(env, context_class);
        (*env)->DeleteLocalRef(env, file_class);
        return FALSE;
    }

    dir = (*env)->CallObjectMethod(env, context, get_cache_dir_id);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, context_class);
        (*env)->DeleteLocalRef(env, file_class);
        return FALSE;
    }

    if (dir)
    {
        abs_path = (*env)->CallObjectMethod(env, dir, get_absolute_path_id);
        if ((*env)->ExceptionCheck(env))
        {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, dir);
            (*env)->DeleteLocalRef(env, context_class);
            (*env)->DeleteLocalRef(env, file_class);
            return FALSE;
        }
        abs_path_str = (*env)->GetStringUTFChars(env, abs_path, NULL);
        if ((*env)->ExceptionCheck(env))
        {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, abs_path);
            (*env)->DeleteLocalRef(env, dir);
            (*env)->DeleteLocalRef(env, context_class);
            (*env)->DeleteLocalRef(env, file_class);
            return FALSE;
        }
        *cache_dir = abs_path ? g_strdup(abs_path_str) : NULL;

        (*env)->ReleaseStringUTFChars(env, abs_path, abs_path_str);
        (*env)->DeleteLocalRef(env, abs_path);
        (*env)->DeleteLocalRef(env, dir);
    }

    dir = (*env)->CallObjectMethod(env, context, get_files_dir_id);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, context_class);
        (*env)->DeleteLocalRef(env, file_class);
        return FALSE;
    }
    if (dir)
    {
        abs_path = (*env)->CallObjectMethod(env, dir, get_absolute_path_id);
        if ((*env)->ExceptionCheck(env))
        {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, dir);
            (*env)->DeleteLocalRef(env, context_class);
            (*env)->DeleteLocalRef(env, file_class);
            return FALSE;
        }
        abs_path_str = (*env)->GetStringUTFChars(env, abs_path, NULL);
        if ((*env)->ExceptionCheck(env))
        {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, abs_path);
            (*env)->DeleteLocalRef(env, dir);
            (*env)->DeleteLocalRef(env, context_class);
            (*env)->DeleteLocalRef(env, file_class);
            return FALSE;
        }
        *files_dir = files_dir ? g_strdup(abs_path_str) : NULL;

        (*env)->ReleaseStringUTFChars(env, abs_path, abs_path_str);
        (*env)->DeleteLocalRef(env, abs_path);
        (*env)->DeleteLocalRef(env, dir);
    }

    (*env)->DeleteLocalRef(env, file_class);
    (*env)->DeleteLocalRef(env, context_class);

    return TRUE;
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

static gboolean
init(JNIEnv *env, jobject context)
{
    jclass context_cls = NULL;
    jmethodID get_class_loader_id = 0;

    jobject class_loader = NULL;

    context_cls = (*env)->GetObjectClass(env, context);
    if (!context_cls)
    {
        return FALSE;
    }

    get_class_loader_id = (*env)->GetMethodID(env, context_cls,
                                              "getClassLoader", "()Ljava/lang/ClassLoader;");
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return FALSE;
    }

    class_loader = (*env)->CallObjectMethod(env, context, get_class_loader_id);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return FALSE;
    }

    if (_context)
    {
        (*env)->DeleteGlobalRef(env, _context);
    }
    _context = (*env)->NewGlobalRef(env, context);

    if (_class_loader)
    {
        (*env)->DeleteGlobalRef(env, _class_loader);
    }
    _class_loader = (*env)->NewGlobalRef(env, class_loader);

    return TRUE;
}

void gst_android_init(JNIEnv *env, jobject context)
{
    gchar *cache_dir;
    gchar *files_dir;
    gchar *registry;
    GError *error = NULL;

    if (!init(env, context))
    {
        __android_log_print(ANDROID_LOG_INFO, "GStreamer",
                            "GStreamer failed to initialize");
    }

    if (gst_is_initialized())
    {
        __android_log_print(ANDROID_LOG_INFO, "GStreamer",
                            "GStreamer already initialized");
        return;
    }

    if (!get_application_dirs(env, context, &cache_dir, &files_dir))
    {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer",
                            "Failed to get application dirs");
    }

    if (cache_dir)
    {
        g_setenv("TMP", cache_dir, TRUE);
        g_setenv("TEMP", cache_dir, TRUE);
        g_setenv("TMPDIR", cache_dir, TRUE);
        g_setenv("XDG_RUNTIME_DIR", cache_dir, TRUE);
        g_setenv("XDG_CACHE_HOME", cache_dir, TRUE);
        registry = g_build_filename(cache_dir, "registry.bin", NULL);
        g_setenv("GST_REGISTRY", registry, TRUE);
        g_free(registry);
        g_setenv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", "no", TRUE);
        /* TODO: Should probably also set GST_PLUGIN_SCANNER and GST_PLUGIN_SYSTEM_PATH */
    }
    if (files_dir)
    {
        gchar *fontconfig, *certs;

        g_setenv("HOME", files_dir, TRUE);
        g_setenv("XDG_DATA_DIRS", files_dir, TRUE);
        g_setenv("XDG_CONFIG_DIRS", files_dir, TRUE);
        g_setenv("XDG_CONFIG_HOME", files_dir, TRUE);
        g_setenv("XDG_DATA_HOME", files_dir, TRUE);

        fontconfig = g_build_filename(files_dir, "fontconfig", NULL);
        g_setenv("FONTCONFIG_PATH", fontconfig, TRUE);
        g_free(fontconfig);

        certs = g_build_filename(files_dir, "ssl", "certs", "ca-certificates.crt", NULL);
        g_setenv("CA_CERTIFICATES", certs, TRUE);
        g_free(certs);
    }
    g_free(cache_dir);
    g_free(files_dir);

    /* Disable this for releases if performance is important
     * or increase the threshold to get more information */
    gst_debug_set_active(TRUE);
    gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    gst_debug_remove_log_function(gst_debug_log_default);
    gst_debug_add_log_function((GstLogFunction)gst_debug_logcat, NULL, NULL);

    /* get time we started for debugging messages */
    _priv_gst_info_start_time = gst_util_get_timestamp();

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
    __android_log_print(ANDROID_LOG_INFO, "GStreamer",
                        "GStreamer initialization complete");
}

JNIEXPORT void JNICALL
Java_org_freedesktop_gstreamer_GStreamer_nativeInit(JNIEnv *env, jobject gstreamer, jobject context)
{
    gst_android_init(env, context);
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
