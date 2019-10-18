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

#include "videoddspublisher.h"

#include <QMainWindow>
#include <QDebug>
#include <QSize>

#include "videowidgetgst.h"
#include "qtgstreamer.h"
#include "pipelinedds.h"
#include "videowidgetpaintergst.h"

VideoDDSpublisher::VideoDDSpublisher(int& argc, char* argv[]) :
QApplication(argc, argv)
,m_mainwindow(nullptr)
,m_pipeline(nullptr)
,m_dataWriter(dds::core::null)
,m_useTestSrc(false)
,m_useOmx(false)
,m_useV4l(false)
,m_strength(0)
{
	setApplicationName("Video DDS Publisher");
}


void VideoDDSpublisher::initDDS(const QString& topicName)
{
	// OpenDplice uses an error throwing mechanism, some of the possible
	// error types that may be thrown from the used function are
	// catched below
	try
	{
		// Create a domain participant using the default ID configured on the XML file
		dds::domain::DomainParticipant dp(org::opensplice::domain::default_id());

		// Create a topic QoS with exclusive ownership and defined liveliness.
		// The exclusive ownership allows the use of the ownership strength to define which video source is used.
		// The liveliness topic determines how to long to wait until the source with lower strength is used
		// when messages are not received from the source with higher ownership strength.
		dds::topic::qos::TopicQos topicQos
		= dp.default_topic_qos();
		//	The dds::core::policy::Liveliness qos setting had been previously added here and is now
		// (probably) at the data writer QoS. This was done to prevent a crash that was caused
		// by having the dataReader without the Liveliness setting
		// Further option may be:
		//	<< dds::core::policy::Durability::Volatile()
		//	<< dds::core::policy::Reliability::BestEffort();

		dds::topic::Topic<S2E::Video> topic(dp, topicName.toStdString(), topicQos);

		dds::pub::qos::PublisherQos pubQos = dp.default_publisher_qos();
		dds::pub::Publisher pub(dp, pubQos);

		dds::pub::qos::DataWriterQos dwqos = topic.qos();
		dwqos << dds::core::policy::OwnershipStrength(m_strength);
		dwqos << dds::core::policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
		dwqos << dds::core::policy::Ownership::Exclusive();
		dwqos << dds::core::policy::Liveliness::ManualByTopic(dds::core::Duration::from_millisecs(1000));

		m_dataWriter = dds::pub::DataWriter<S2E::Video>(pub, topic, dwqos);
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


void VideoDDSpublisher::initGstreamer()
{
	auto widget = new VideoWidgetPainterGst();

	// The message handler must be installed before GStreamer
	// is initialized.
	QtGStreamer::instance()->installMessageHandler(3 /*log level*/);
	QtGStreamer::instance()->init();

	// No auto format discovery is implemented
	// Supported formats of the webcam may be gathered by using gst-launch, e.g. on Windows:
	// gst-launch-1.0 --gst-debug=*src:5 ksvideosrc num-buffers=1 ! fakesink
	// or on Linux:
	// gst-launch-1.0 --gst-debug=*src:5 v4l2src num-buffers=1 ! fakesink

	// Resolutions:
	const QSize srcResolution(1280, 720);
	// const QSize srcResolution(640, 480);
	//	const QSize aspectRatio(16, 9);
	//	const QSize srcResolution = aspectRatio * 40;
	//	qDebug() << srcResolution;

	// A framerate of 15 seems to be supported by most webcams
	const int framerate = 10;

	if (m_pipeline != nullptr)
	{
		m_pipeline->createPipeline("VideoDDSPublisher");
		if (m_useTestSrc == true)
		{
			m_pipeline->setSrcBinI(m_pipeline->createTestSrc());
		}
		else
		{
			if (m_useV4l == true)
			{
				m_pipeline->setSrcBinI(m_pipeline->createV4lSrc(srcResolution, framerate));
			}
			else
			{
				m_pipeline->setSrcBinI(m_pipeline->createCamSrc(srcResolution, framerate));
			}
			m_pipeline->setSrcBinII(m_pipeline->createScaleConvert(srcResolution));
		}


		const Pipeline::PackagingMode modeAfterEncoder = Pipeline::PACKAGINGMODE_UNDEFINED;
		const Pipeline::PackagingMode modeAfterParser = Pipeline::PACKAGINGMODE_BYTESTREAM_AU;

		if (m_useOmx == true)
		{
			m_pipeline->setSinkBinMainI(m_pipeline->createOmxEncoder(2000 /*bitrate*/, 12 /*intraInt*/, modeAfterEncoder, modeAfterParser));
		}
		else
		{
			// Possible invocation for a x264enc module, would require elevated licence!
			//m_pipeline->setSinkBinMainI(m_pipeline->createX264encoder(2000 /*bitrate*/, 2000 /*vbvBufCapacity*/, 10 /*keyIntMax*/, false /*intraRefresh*/, modeAfterEncoder, modeAfterParser, 1 /*num threads*/));
			m_pipeline->setSinkBinMainI(m_pipeline->createOpenEncoder(2000 /*bitrate*/, 12/*keyIntMax*/, modeAfterParser, 0 /*num threads*/));
		}

		m_pipeline->setSinkBinMainII(m_pipeline->createAppSinkForDDS());
		m_pipeline->setSinkBinSecondary(m_pipeline->createAppSink(true /*add converter*/));
		m_pipeline->linkPipeline();

		widget->installAppSink(m_pipeline->appSink("AppSink"));

		m_pipeline->setDataWriter(m_dataWriter);
		m_pipeline->startPipeline();
	}
	widget->show();
}

void VideoDDSpublisher::init()
{
	m_pipeline = new PipelineDDS();
	initDDS("VideoStream");
	initGstreamer();
}

bool VideoDDSpublisher::useTestSrc() const
{
	return m_useTestSrc;
}

void VideoDDSpublisher::setUseTestSrc(bool useTestSrc)
{
	m_useTestSrc = useTestSrc;
}

bool VideoDDSpublisher::useOmx() const
{
	return m_useOmx;
}

void VideoDDSpublisher::setUseOmx(bool useOmx)
{
	m_useOmx = useOmx;
}

int VideoDDSpublisher::strength() const
{
	return m_strength;
}

void VideoDDSpublisher::setStrength(int strength)
{
	m_strength = strength;
}

bool VideoDDSpublisher::useV4l() const
{
	return m_useV4l;
}

void VideoDDSpublisher::setUseV4l(bool useV4l)
{
	m_useV4l = useV4l;
}
