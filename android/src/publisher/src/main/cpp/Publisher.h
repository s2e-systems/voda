#ifndef VODA_PUBLISHER_H
#define VODA_PUBLISHER_H

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


class Publisher {
    dds::pub::DataWriter<S2E::Video> m_data_writer;
    GstElement *m_pipeline;
    GMainLoop *m_main_loop;
    GMainContext *m_context;
    std::thread* m_thread;
    MainActivityBinding m_main_activity_binding;
public:
    Publisher(dds::domain::DomainParticipant domain_participant, JNIEnv *jni_env, jobject main_activity);
    virtual ~Publisher();
    GstElement *video_sink();
};


#endif //VODA_PUBLISHER_H
