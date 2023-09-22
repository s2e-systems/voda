#ifndef VODA_MAINACTIVITYBINDING_H
#define VODA_MAINACTIVITYBINDING_H

#include <jni.h>
#include <string>

class MainActivityBinding {
    jobject m_main_activity_global_ref;
    JavaVM* m_java_vm;

public:
    MainActivityBinding(JavaVM* java_vm, jobject main_activity);

    virtual ~MainActivityBinding();

    void setUiMessage(const std::string &message) const;
};


#endif //VODA_MAINACTIVITYBINDING_H
