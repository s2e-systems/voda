#include <jni.h>
#include <android/log.h>
#include <dds/dds.hpp>
#include "Subscriber.h"
#include "MainActivityBinding.h"

using namespace org::eclipse::cyclonedds;

Subscriber* NATIVE_SUBSCRIBER = nullptr;

jint JNI_OnLoad(JavaVM*, void*) {
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_s2e_1systems_MainActivity_nativeSubscriberInit(JNIEnv * env, jobject thiz, int orientation) {

    setenv("CYCLONEDDS_URI", R"(<CycloneDDS><Domain><General><Interfaces>
        <NetworkInterface name="eth1" presence_required="false" />
        <NetworkInterface name="wlan0" presence_required="false" />
        </Interfaces></General></Domain></CycloneDDS>)", 1);

    try {
        JavaVM* java_vm;
        env->GetJavaVM(&java_vm);
        std::unique_ptr<MainActivityBinding> main_activity_binding{
                new MainActivityBinding{java_vm, thiz} };
        NATIVE_SUBSCRIBER = new Subscriber(dds::domain::DomainParticipant{ domain::default_id() },
            std::move(main_activity_binding), orientation);
        return reinterpret_cast<jlong>(NATIVE_SUBSCRIBER->video_sink());
    }
    catch (const dds::core::Exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeSubscriber",
            "DDS initializaion failed with: %s", e.what());
    }
    catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, "NativeSubscriber", "SubscriberInit failed with: %s",
            e.what());
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_s2e_1systems_MainActivity_nativeSubscriberFinalize(JNIEnv*, jobject) {
    delete NATIVE_SUBSCRIBER;
}
