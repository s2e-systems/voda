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

#include <QDebug>
#include <QSize>

#include "videowidgetgst.h"
#include "qtgstreamer.h"
#include "pipelinedds.h"
#include "videowidgetpaintergst.h"
#include "elements.h"
#include "cameracapabilities.h"

VideoDDSpublisher::VideoDDSpublisher(int& argc, char* argv[])
	:QApplication(argc, argv)
	,m_mainwindow(nullptr)
	,m_pipeline(nullptr)
	,m_dataWriter(dds::core::null)
	,m_useTestSrc(false)
	,m_useOmx(false)
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


void VideoDDSpublisher::initGstreamer()
{
	auto widget = new VideoWidgetPainterGst();

	// The message handler must be installed before GStreamer
	// is initialized.
	QtGStreamer::instance()->installMessageHandler(3 /*log level*/);
	QtGStreamer::instance()->init();

	//////////////
	// Source

	// Supported formats of the webcam may be gathered by using gst-launch, e.g. on Windows:
	// gst-launch-1.0 --gst-debug=*src:5 ksvideosrc num-buffers=1 ! fakesink
	// or on Linux:
	// gst-launch-1.0 --gst-debug=*src:5 v4l2src num-buffers=1 ! fakesink

	std::vector<std::string> sourceCanditates;
	if (!m_useTestSrc)
	{
		sourceCanditates.push_back("ksvideosrc");
		sourceCanditates.push_back("v4l2src");
	}
	sourceCanditates.push_back("videotestsrc");

	ElementSelection sourceSelection{sourceCanditates, "source"};
	qDebug() << "selected source element: " << QString::fromStdString(sourceSelection.elementName());

	gst_element_set_state(sourceSelection.element(), GST_STATE_READY);
	gst_element_get_state(sourceSelection.element(), nullptr/*state*/, nullptr/*pending*/, GST_CLOCK_TIME_NONE);
	auto pad = gst_element_get_static_pad(sourceSelection.element(), "src");
	auto caps = gst_pad_query_caps(pad, nullptr);

	GstCaps* capsFilter = nullptr;
	if (m_useTestSrc || sourceSelection.elementName() == "videotestsrc")
	{
		capsFilter = gst_caps_new_simple("video/x-raw",
			"format", G_TYPE_STRING, "I420",
			"width", G_TYPE_INT, 640,
			"height", G_TYPE_INT, 480,
			"framerate", GST_TYPE_FRACTION, 30, 1,
			nullptr
		);
	}
	else
	{
		CapabilitySelection capsSelection{caps};
		const auto framerate = capsSelection.highestRawFrameRate();
		// If the framerate is very high, it is likely to be not supported by the
		// encoder or the detection went wrong. In this case try a conservative
		// video format
		if (framerate < 50.0)
		{
			capsFilter = capsSelection.highestRawArea(framerate);
		}
		else
		{
			capsFilter = gst_caps_new_simple("video/x-raw",
				"width", G_TYPE_INT, 640,
				"height", G_TYPE_INT, 480,
				"framerate", GST_TYPE_FRACTION, 30, 1,
				nullptr
			);
		}
	}
	qDebug() << "Capabilities for the source element:" <<  gst_caps_to_string(capsFilter);

	auto sourceBin = GST_BIN_CAST(gst_bin_new("sourceBin"));
	gst_bin_add(sourceBin, sourceSelection.element());
	auto filter = gst_element_factory_make("capsfilter", nullptr);
	g_object_set(filter, "caps", capsFilter, nullptr);
	gst_bin_add(sourceBin, filter);
	gst_element_link(sourceSelection.element(), filter);
	auto converter = gst_element_factory_make("videoconvert", nullptr);
	gst_bin_add(sourceBin, converter);
	gst_element_link(filter, converter);


	//////////////
	// Encoder

	GstElement* encoder = nullptr;
	// Test if the encoder works with the source caps
	bool tryOther = true;
	auto factory = gst_element_factory_find("avenc_h264_omx");
	if (factory != nullptr)
	{
		auto encoderTestBin = gst_bin_new("encoderTestBin");
		auto testsrc = gst_element_factory_make("videotestsrc", nullptr);
		encoder = gst_element_factory_create(factory, nullptr);
		auto fakesink = gst_element_factory_make("fakesink", nullptr);
		gst_bin_add(GST_BIN_CAST(encoderTestBin), testsrc);
		gst_bin_add(GST_BIN_CAST(encoderTestBin), encoder);
		gst_bin_add(GST_BIN_CAST(encoderTestBin), fakesink);
		const auto linkRet1 = gst_element_link_filtered(testsrc, encoder, capsFilter);
		const auto linkRet2 = gst_element_link(encoder, fakesink);
		const auto ret = gst_element_set_state(GST_ELEMENT_CAST(encoderTestBin), GST_STATE_PAUSED);
		if (linkRet1 && linkRet2 && ret == GST_STATE_CHANGE_SUCCESS)
		{
			tryOther = false;
		}
		gst_element_set_state(GST_ELEMENT_CAST(encoderTestBin), GST_STATE_NULL);
	}
	if (tryOther)
	{
		factory = gst_element_factory_find("x264enc");
		if (factory != nullptr)
		{
			encoder = gst_element_factory_create(factory, nullptr);
		}
		else
		{
			throw std::runtime_error{"No working encoder found"};
		}
	}

	const std::string encoderName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(gst_element_get_factory(encoder)));
	qDebug() << "selected encoder gstreamer element: " << QString::fromStdString(encoderName);

	const int kilobitrate = 1280;
	const int keyframedistance = 30;

	auto encodereBin = GST_BIN_CAST(gst_bin_new("encodereBin"));
	gst_bin_add(encodereBin, encoder);

	if (encoderName == "avenc_h264_omx")
	{
		g_object_set(encoder,
			"bitrate", gint64(kilobitrate * 1000), //set bitrate (in bits/s)
			"gop-size", gint(keyframedistance), //set the group of picture (GOP) size
			nullptr
		);
		// The avenc_h264_omx does not send the PPS/SPS with the IDR frames
		// the parser will do so
		auto parser = gst_element_factory_make("h264parse", nullptr);
		g_object_set(parser, "config-interval", gint(-1), nullptr);
		gst_bin_add(encodereBin, parser);
		gst_element_link(encoder, parser);
	}
	if (encoderName == "x264enc")
	{
		g_object_set(encoder,
			"bitrate", guint(kilobitrate), // Bitrate in kbit/sec
			"vbv-buf-capacity", guint(2000), // Size of the VBV buffer in milliseconds
			"key-int-max", guint(keyframedistance), // Maximal distance between two key-frames (0 for automatic)
			"threads", guint(1), // Number of threads used by the codec (0 for automatic)
			"sliced-threads", gboolean(false), // Low latency but lower efficiency threading
			"insert-vui", gboolean(false),
			"speed-preset", 1, // Preset name for speed/quality tradeoff options
			"trellis", gboolean(false),
			"aud", gboolean(false), // Use AU (Access Unit) delimiter
			nullptr
		);
	}

	///////////
	// Display and transmission

	if (m_pipeline != nullptr)
	{
		m_pipeline->createPipeline("VideoDDSPublisher");

		m_pipeline->setSrcBinI(sourceBin);
		m_pipeline->setSinkBinMainI(encodereBin);

		m_pipeline->setSinkBinMainII(m_pipeline->createAppSinkForDDS());
		m_pipeline->setSinkBinSecondary(m_pipeline->createAppSink());
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

int VideoDDSpublisher::strength() const
{
	return m_strength;
}

void VideoDDSpublisher::setStrength(int strength)
{
	m_strength = strength;
}

