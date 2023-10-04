#include <jni.h>
#include <android/native_window_jni.h>
#include <gst/video/video.h>

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
