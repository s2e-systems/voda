#ifndef VODA_SUBSCRIBER_H
#define VODA_SUBSCRIBER_H

#include <memory>
#include <thread>
#include <gst/gst.h>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"
#include "MainActivityBinding.h"

class VideoListener;

using namespace org::eclipse::cyclonedds;

class Subscriber {
    dds::sub::DataReader<S2E::Video> m_data_reader;
    GstElement *m_pipeline;
    GMainLoop *m_main_loop;
    GMainContext *m_context;
    std::thread m_thread;
    std::unique_ptr<MainActivityBinding> m_main_activity_binding;
    VideoListener* m_video_listener;

public:
    Subscriber(const dds::domain::DomainParticipant& domain_participant, std::unique_ptr<MainActivityBinding> main_activity_binding);
    virtual ~Subscriber();
    GstElement *video_sink();
};


#endif //VODA_SUBSCRIBER_H
