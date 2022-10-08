#include <iostream>
#include <dds/dds.hpp>
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;


int main(int argc, char *argv[])
{
    const std::string topicName = "VideoStream";

    const dds::domain::DomainParticipant dp{domain::default_id()};
    const dds::topic::qos::TopicQos topicQos{dp.default_topic_qos()};
    const dds::topic::Topic<S2E::Video> topic{dp, topicName, topicQos};
    const dds::sub::Subscriber subscriber{dp};
    dds::sub::DataReader<S2E::Video> dataReader{subscriber, topic};

    while (true) {
        const auto samples = dataReader.take();

        for (const auto& sample : samples)
        {
            if(sample.info().valid() == false)
            {
                continue;
            }
            std::cout << sample.data().frameNum()  << std::endl;
            std::cout << std::flush;
        }
    }
    return 0;
}