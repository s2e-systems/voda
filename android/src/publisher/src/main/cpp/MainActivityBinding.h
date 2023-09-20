#ifndef VODA_MAINACTIVITYBINDING_H
#define VODA_MAINACTIVITYBINDING_H

#include <jni.h>
#include <string>

class MainActivityBinding {
    jobject m_main_activity_global_ref;
    JNIEnv *m_jni_env;

public:
    MainActivityBinding(JNIEnv *jni_env, jobject main_activity);

    virtual ~MainActivityBinding();

    void setUiMessage(const std::string &message);

    JNIEnv *jniEnv();
};


#endif //VODA_MAINACTIVITYBINDING_H
