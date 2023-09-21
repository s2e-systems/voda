#ifndef VODA_MAINACTIVITYBINDING_H
#define VODA_MAINACTIVITYBINDING_H

#include <jni.h>
#include <string>

class MainActivityBinding {
    jobject m_main_activity_global_ref;
    JavaVM* m_java_vm;
    JNIEnv* get_jni_interface_pointer() const {
        JNIEnv *jni_env = nullptr;
        if (m_java_vm->AttachCurrentThread(&jni_env, nullptr) != JNI_OK) {
            throw std::runtime_error("AttachCurrentThread failed");
        }
        return jni_env;
    }

public:
    MainActivityBinding(JavaVM* java_vm, jobject main_activity);

    virtual ~MainActivityBinding();

    void setUiMessage(const std::string &message) const;
};


#endif //VODA_MAINACTIVITYBINDING_H
