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
#include <QCommandLineParser>
#include <QDebug>

#include "videoddspublisher.h"
#include "videowidgetpaintergst.h"
#include "qtgstreamer.h"

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setApplicationName("Video DDS Publisher");
	qDebug() << "This is" << application.applicationName();

	QCommandLineParser parser;
	parser.setApplicationDescription("VideoDDSpublisher");
	parser.addHelpOption();
	parser.addVersionOption();
	QCommandLineOption useTestSrcOption("testsrc", "Use test src, instead of autovideosrc.");
	parser.addOption(useTestSrcOption);
	QCommandLineOption useOmxOption("omx", "Use omx as the encoder.");
	parser.addOption(useOmxOption);
	QCommandLineOption useFixedCapsOption("fixed", "Use fixed capabilities for the camera source.");
	parser.addOption(useFixedCapsOption);
	QCommandLineOption strengthOption("strength", "Set DDS OwnershipStrength (The higher the number the more stregnth).", "[0 1500]", "1000");
	parser.addOption(strengthOption);
	parser.process(application);

	const std::string topicName = "VideoStream";

	try
	{
		// Create a domain participant using the default ID configured on the XML file
		dds::domain::DomainParticipant dp(org::opensplice::domain::default_id());

		// Create a topic QoS with exclusive ownership and defined liveliness.
		// The exclusive ownership allows the use of the ownership strength to define which video source is used.
		// The liveliness topic determines how to long to wait until the source with lower strength is used
		// when messages are not received from the source with higher ownership strength.
		dds::topic::qos::TopicQos topicQos = dp.default_topic_qos();
		//	The dds::core::policy::Liveliness qos setting had been previously added here and is now
		// (probably) at the data writer QoS. This was done to prevent a crash that was caused
		// by having the dataReader without the Liveliness setting
		// Further option may be:
		//	<< dds::core::policy::Durability::Volatile()
		//	<< dds::core::policy::Reliability::BestEffort();

		dds::topic::Topic<S2E::Video> topic(dp, topicName, topicQos);

		dds::pub::qos::PublisherQos pubQos = dp.default_publisher_qos();
		dds::pub::Publisher pub(dp, pubQos);

		dds::pub::qos::DataWriterQos dwqos = topic.qos();
		dwqos << dds::core::policy::OwnershipStrength(parser.value(strengthOption).toInt());
		dwqos << dds::core::policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
		dwqos << dds::core::policy::Ownership::Exclusive();
		dwqos << dds::core::policy::Liveliness::ManualByTopic(dds::core::Duration::from_millisecs(1000));

		auto dataWriter = dds::pub::DataWriter<S2E::Video>(pub, topic, dwqos);

		// The message handler must be installed before GStreamer
		// is initialized.
		QtGStreamer::instance()->installMessageHandler(3 /*log level*/);
		QtGStreamer::instance()->init();

		VideoDDSpublisher publisher{dataWriter, parser.isSet(useTestSrcOption), parser.isSet(useOmxOption), parser.isSet(useFixedCapsOption)};

		VideoWidgetPainterGst widget(publisher.appsink());
		widget.show();

		return application.exec();
	}
	catch (const dds::core::OutOfResourcesError& e)
	{
		qFatal("DDS OutOfResourcesError: %s", e.what());
	}
	catch (const dds::core::InvalidArgumentError& e)
	{
		qFatal("DDS InvalidArgumentError: %s", e.what());
	}
	catch (const dds::core::NullReferenceError& e)
	{
		qFatal("DDS NullReferenceError: %s", e.what());
	}
	catch (const dds::core::Error& e)
	{
		qFatal("DDS Error: %s", e.what());
	}
	catch (...)
	{
		qFatal("DDS initialization failed with unhandled exeption");
	}
}
