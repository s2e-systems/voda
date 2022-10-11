#include <sstream>
#include <jni.h>
#include <unistd.h>
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

    //putenv("CYCLONEDDS_URI=<CycloneDDS><Domain><General><Interfaces><NetworkInterface name=\"eth1\" /></Interfaces></General></Domain></CycloneDDS>");

    try {
        dds_wrapper = new DDSwrapper{};
        for (int i = 0; i < 10; i++)
        {
            const S2E::Video v{1, i, std::vector<uint8_t>{1, 2, 3, 4}};
            dds_wrapper->dataWriter.write(v);
            usleep(200000);
        }
        ss << "very good";
    } catch (...) {
        ss << "NOT good";
    }

    return env->NewStringUTF(ss.str().c_str());
}

//jint JNI_OnLoad(JavaVM *vm, void *reserved)
//{
//    JNIEnv *env;
//    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
//    {
//        return JNI_ERR;
//    }
//    jclass c = env->FindClass("mainactivity/MainActivity");
//    if (c == nullptr)
//    {
//        return JNI_ERR;
//    }
//    return JNI_VERSION_1_6;
//}
