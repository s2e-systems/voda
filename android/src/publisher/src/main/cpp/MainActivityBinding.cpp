#include "MainActivityBinding.h"

#include <android/log.h>
#include <gst/gst.h>

MainActivityBinding::MainActivityBinding(JNIEnv *jni_env, jobject main_activity) :
        m_main_activity_global_ref{jni_env->NewGlobalRef(main_activity)},
        m_jni_env{jni_env} {}

void MainActivityBinding::setUiMessage(const std::string &message) {
    const jstring j_message = m_jni_env->NewStringUTF(message.c_str());
    const jmethodID set_message_method_id = m_jni_env->GetMethodID(
            m_jni_env->GetObjectClass(m_main_activity_global_ref),
            "setMessage",
            "(Ljava/lang/String;)V");
    m_jni_env->CallVoidMethod(m_main_activity_global_ref, set_message_method_id,
                              j_message);
    if (m_jni_env->ExceptionCheck()) {
        __android_log_print(ANDROID_LOG_ERROR, "GStreamer", "Failed to call Java method");
        m_jni_env->ExceptionClear();
    }
    m_jni_env->DeleteLocalRef(j_message);
}


JNIEnv *MainActivityBinding::jniEnv() {
    return m_jni_env;
}

MainActivityBinding::~MainActivityBinding() {
//    m_jni_env->DeleteGlobalRef(this->m_main_activity_global_ref);
}
