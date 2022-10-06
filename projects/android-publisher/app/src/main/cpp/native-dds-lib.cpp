#include <sstream>
#include <jni.h>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;

//static dds_entity_t dp;

struct DDSwrapper {
    DDSwrapper(const std::string topicName = "VideoStream") :
        dp{domain::default_id()},
        topicQos{dp.default_topic_qos()},
        topic{dp, topicName, topicQos},
        publisher{dp},
        dataWriter{publisher, topic}
    {}
    dds::domain::DomainParticipant dp;
    const dds::topic::qos::TopicQos topicQos;
    const dds::topic::Topic<S2E::Video> topic;
    const dds::pub::Publisher publisher;
    dds::pub::DataWriter<S2E::Video> dataWriter;
};

DDSwrapper* dds_wrapper = nullptr;

extern "C" JNIEXPORT jstring JNICALL
Java_mainactivity_MainActivity_nativeDdsInit(JNIEnv *env, jobject thiz)
{

    std::stringstream ss;
    ss << "Domain participant: ";
//    dp = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);

    try {
        dds_wrapper = new DDSwrapper{};
//        const dds::domain::DomainParticipant dp{domain::default_id()};
//        const dds::topic::qos::TopicQos topicQos{dp.default_topic_qos()};
//        const dds::topic::Topic<S2E::Video> topic{dp, topicName, topicQos};
//        const dds::pub::Publisher publisher{dp};
//        dds::pub::DataWriter<S2E::Video> dataWriter{publisher, topic};
        ss << "good";
    } catch (...) {
        ss << "NOT good";
    }

    return env->NewStringUTF(ss.str().c_str());
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        return JNI_ERR;
    }
    jclass c = env->FindClass("mainactivity/MainActivity");
    if (c == nullptr)
    {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
