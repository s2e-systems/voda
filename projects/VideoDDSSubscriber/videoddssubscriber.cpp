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

#include "videoddssubscriber.h"

#include <QDebug>


#include "videowidgetgst.h"
#include "qtgstreamer.h"
#include "pipeline.h"
#include "videolistener.h"

#include "videowidgetpaintergst.h"

VideoDDSsubscriber::VideoDDSsubscriber(int& argc, char** argv) :
	QApplication(argc, argv)
	,m_videoListener(nullptr)
	,m_pipeline(nullptr)
	,m_dr(dds::core::null)
	,m_useOmx(false)
{
	setApplicationName("Video DDS Subscriber");
}

void VideoDDSsubscriber::init()
{
	initDDS("VideoStream");

	auto widget = new VideoWidgetPainterGst();

	// Message handler must be installed before GStreamer init()
	QtGStreamer::instance()->installMessageHandler(3 /*log level*/);
	QtGStreamer::instance()->init();

	m_pipeline = new Pipeline();
	m_pipeline->createPipeline("VideoDDSsubscriber");

	m_pipeline->setSrcBinI(m_pipeline->createAppSrc());

	if (m_useOmx == true)
	{
		m_pipeline->setSrcBinII(m_pipeline->createOmxDecoder());
	}
	else
	{
//		m_pipeline->setSrcBinII(m_pipeline->createAvDecoder(Pipeline::PACKAGINGMODE_UNDEFINED));
		// Following line may be used to use openh264dec:
		// Note: the createOpenDecoder does not seem to work with the omxh264enc, however
		// with the openh264dec it sworks. Difficult to find out why the omxh264enc does not work,
		// Stopss with GstWarn: AppSrc:error: streaming stopped, reason not-negotiated (-4)
		m_pipeline->setSrcBinII(m_pipeline->createOpenDecoder(Pipeline::PACKAGINGMODE_UNDEFINED));
	}

	m_pipeline->setSinkBinMainI(m_pipeline->createAppSink(true /*converter*/));
	m_pipeline->linkPipeline();

	widget->installAppSink(m_pipeline->appSink());

	if (m_videoListener != nullptr)
	{
		m_videoListener->installAppSrc(m_pipeline->appSrc());
	}


	widget->show();

	m_pipeline->startPipeline();
}

void VideoDDSsubscriber::initDDS(const QString& topicName)
{
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
		dds::topic::Topic<S2E::Video> topic(dp, topicName.toStdString(), topicQos);

		// Create a subscriber with a default QoS
		dds::sub::qos::SubscriberQos subQos = dp.default_subscriber_qos();
		dds::sub::Subscriber sub(dp, subQos);

		dds::sub::qos::DataReaderQos drqos = topic.qos();
		drqos	<< dds::core::policy::Ownership::Exclusive()
				<< dds::core::policy::Liveliness::Automatic();

		// Trigger the callback functions when data becomes available or when the
		// requested deadline is missed (currently not used)
		dds::core::status::StatusMask mask;
		mask << dds::core::status::StatusMask::data_available()
			 << dds::core::status::StatusMask::requested_deadline_missed();

		// Create a video listener which triggers the callbacks necessary for showing
		// the video data when a new message is received
		m_videoListener = new VideoListener();

		// Create the data reader for the video topic
		m_dr = dds::sub::DataReader<S2E::Video>(sub, topic, drqos, m_videoListener, mask);
	}
	catch(dds::core::Error e)
	{
		qCritical("DDS Error: %s", e.what());
	}
	catch(dds::core::OutOfResourcesError e)
	{
		qCritical("DDS OutOfResourcesError: %s", e.what());
	}
	catch(dds::core::InvalidArgumentError e)
	{
		qCritical("DDS InvalidArgumentError: %s", e.what());
	}
	catch(dds::core::NullReferenceError e)
	{
		qCritical("DDS NullReferenceError: %s", e.what());
	}
	catch(...)
	{
		qCritical("DDS initialization failed with unhandled exeption");
	}
}

bool VideoDDSsubscriber::useOmx() const
{
	return m_useOmx;
}

void VideoDDSsubscriber::setUseOmx(bool useOmx)
{
	m_useOmx = useOmx;
}

