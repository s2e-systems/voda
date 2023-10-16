#ifndef VODA_PUBLISHER_H
#define VODA_PUBLISHER_H

#include <memory>
#include <thread>
#include <gst/gst.h>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"
#include "MainActivityBinding.h"

using namespace org::eclipse::cyclonedds;

class Publisher {
    dds::pub::DataWriter<S2E::Video> m_data_writer;
    GstElement *m_pipeline;
    GMainLoop *m_main_loop;
    GMainContext *m_context;
    std::thread m_thread;
    std::unique_ptr<MainActivityBinding> m_main_activity_binding;
public:
    Publisher(const dds::domain::DomainParticipant& domain_participant, std::unique_ptr<MainActivityBinding> main_activity_binding, int orientation);
    virtual ~Publisher();
    GstElement *video_sink();
};


#endif //VODA_PUBLISHER_H
