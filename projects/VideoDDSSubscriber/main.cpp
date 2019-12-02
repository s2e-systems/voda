// Copyright 2017 S2E Software, Systems and Control
//
// Licensed under the Apache License, Version 2.0 the "License";
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>

#include "qtgstreamer.h"
#include "videoddssubscriber.h"
#include "videolistener.h"
#include "videowidgetpaintergst.h"


int main(int argc, char *argv[])
{
	QApplication application{argc, argv};
	application.setApplicationName("Video DDS Subscriber");
	qDebug() << "This is" << application.applicationName();

	QCommandLineParser parser;
	parser.setApplicationDescription("Test VideoDDSsubscriber");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption useOmxOption("omx", "Use omx as the decoder");
	parser.addOption(useOmxOption);
	parser.process(application);


	const std::string topicName = "VideoStream";

	try
	{
		// Create a domain participant using the default ID configured on the XML file
		dds::domain::DomainParticipant dp(org::opensplice::domain::default_id());

		// Create a topic QoS with exclusive ownership. The exclusive ownership allows
		// the use of the ownership strength to define which video source is used.
		dds::topic::qos::TopicQos topicQos = dp.default_topic_qos();
//				<< dds::core::policy::Liveliness::ManualByTopic(dds::core::Duration::from_millisecs(1000));
		// Following settings might be interesting for other usecases:
		//	<< dds::core::policy::Durability::Transient()
		//	<< dds::core::policy::Reliability::BestEffort();

		// Create a topic
		dds::topic::Topic<S2E::Video> topic(dp, topicName, topicQos);

		// Create a subscriber with a default QoS
		dds::sub::qos::SubscriberQos subQos = dp.default_subscriber_qos();
		dds::sub::Subscriber sub(dp, subQos);

		dds::sub::qos::DataReaderQos drqos = topic.qos();
		drqos	<< dds::core::policy::Ownership::Exclusive()
				<< dds::core::policy::Liveliness::Automatic()
				<< dds::core::policy::History(dds::core::policy::HistoryKind::KEEP_LAST, 20);

		// Trigger the callback functions when data becomes available or when the
		// requested deadline is missed (currently not used)
		dds::core::status::StatusMask mask;
		mask << dds::core::status::StatusMask::data_available()
			 << dds::core::status::StatusMask::requested_deadline_missed();


		// Message handler must be installed before GStreamer init()
		QtGStreamer::instance()->installMessageHandler(3 /*log level*/);
		QtGStreamer::instance()->init();

		VideoDDSsubscriber subscriber(parser.isSet(useOmxOption));

		// Create a video listener which triggers the callbacks necessary for showing
		// the video data when a new message is received
		auto videoListener = new VideoListener();
		videoListener->installAppSrc(subscriber.ddsAppSrc());

		// Create the data reader for the video topic
		auto dataReader = dds::sub::DataReader<S2E::Video>(sub, topic, drqos, videoListener, mask);


		auto widget = new VideoWidgetPainterGst(subscriber.displayAppSink());
		widget->show();

		return application.exec();
	}
	catch(const dds::core::OutOfResourcesError& e)
	{
		qCritical("DDS OutOfResourcesError: %s", e.what());
	}
	catch(const dds::core::InvalidArgumentError& e)
	{
		qCritical("DDS InvalidArgumentError: %s", e.what());
	}
	catch(const dds::core::NullReferenceError& e)
	{
		qCritical("DDS NullReferenceError: %s", e.what());
	}
	catch(const dds::core::Error& e)
	{
		qCritical("DDS Error: %s", e.what());
	}
	catch(...)
	{
		qCritical("DDS initialization failed with unhandled exeption");
	}
}
