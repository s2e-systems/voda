#include "MainActivityBinding.h"

#include <android/log.h>
#include <gst/gst.h>

MainActivityBinding::MainActivityBinding(JavaVM* java_vm, jobject main_activity) :
    m_java_vm{java_vm}
{
    m_main_activity_global_ref = get_jni_interface_pointer()->NewGlobalRef(main_activity);
}

void MainActivityBinding::setUiMessage(const std::string &message) const {
    const auto jni_env = get_jni_interface_pointer();
    const auto j_message = jni_env->NewStringUTF(message.c_str());
    const auto set_message_method_id = jni_env->GetMethodID(
            jni_env->GetObjectClass(m_main_activity_global_ref),
            "setMessage",
            "(Ljava/lang/String;)V");
    jni_env->CallVoidMethod(m_main_activity_global_ref, set_message_method_id,
                              j_message);
    if (jni_env->ExceptionCheck()) {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer", "Failed to call Java method");
        jni_env->ExceptionClear();
    }
    jni_env->DeleteLocalRef(j_message);
}

MainActivityBinding::~MainActivityBinding() {
    get_jni_interface_pointer()->DeleteGlobalRef(this->m_main_activity_global_ref);
}
