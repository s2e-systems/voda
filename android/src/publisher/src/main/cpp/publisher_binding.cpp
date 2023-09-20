#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/gstbin.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"
#include <string>
#include <stdint.h>
#include <thread>
#include "Publisher.h"
#include "MainActivityBinding.h"

using namespace org::eclipse::cyclonedds;

//static JavaVM *JAVA_VM;
Publisher *NATIVE_PUBLISHER = nullptr;

//static JNIEnv *get_jni_env_from_java_vm(JavaVM *java_vm) {
//    JNIEnv *jni_env = nullptr;
//    java_vm->GetEnv(reinterpret_cast<void **>(&jni_env), JNI_VERSION_1_4);
//    return jni_env;
//}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
//    JAVA_VM = vm;
    return JNI_VERSION_1_4;
}



extern "C"
JNIEXPORT jlong JNICALL
Java_com_s2e_1systems_MainActivity_nativePublisherInit(JNIEnv *env, jobject thiz) {

    setenv("CYCLONEDDS_URI", R"(<CycloneDDS><Domain><General><Interfaces>
        <NetworkInterface name="eth1" presence_required="false" />
        <NetworkInterface name="wlan0" presence_required="false" />
        </Interfaces></General></Domain></CycloneDDS>)", 1);

    try {
        NATIVE_PUBLISHER = new Publisher(dds::domain::DomainParticipant{domain::default_id()}, env, thiz);
        return reinterpret_cast<jlong>(NATIVE_PUBLISHER->video_sink());
    } catch (const dds::core::Exception &e) {
        __android_log_print(ANDROID_LOG_ERROR, "DDS", "DDS initializaion failed with: %s", e.what());
    } catch (const std::runtime_error &e) {
        __android_log_print(ANDROID_LOG_ERROR, "NativePublisher", "PublisherInit failed with: %s", e.what());
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_s2e_1systems_SurfaceHolderCallback_nativeSurfaceInit(JNIEnv *env, jobject,
                                                          jobject surface, jlong video_sink) {
    auto native_window = reinterpret_cast<guintptr>(ANativeWindow_fromSurface(env, surface));
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), native_window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_s2e_1systems_SurfaceHolderCallback_nativeSurfaceFinalize(JNIEnv *env, jobject,
                                                              jobject surface) {
    ANativeWindow_release(ANativeWindow_fromSurface(env, surface));
}
extern "C"
JNIEXPORT void JNICALL
Java_com_s2e_1systems_MainActivity_nativePublisherFinalize(JNIEnv *env, jobject thiz) {
    delete NATIVE_PUBLISHER;
}