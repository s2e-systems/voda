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

#include "qtgstreamer.h"
#include "videolistener.h"
#include "pipeline.h"

#include "videowidgetpaintergst.h"

VideoDDSsubscriber::VideoDDSsubscriber(int& argc, char** argv) :
	QApplication(argc, argv)
	,m_videoListener(nullptr)
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

	auto pipeline = gst_pipeline_new("subscriber");
	auto bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_set_sync_handler(bus, Pipeline::busCallBack /*function*/, nullptr /*user_data*/, nullptr /*notify function*/);
	gst_object_unref(bus);

	auto srcCaps = gst_caps_new_simple("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "au",
		nullptr
	);

	auto appSrc = gst_element_factory_make("appsrc", nullptr);
	g_object_set(appSrc,
		"caps", srcCaps,
		"is-live", true,
		"format", GST_FORMAT_TIME,
		nullptr
	);

	gst_bin_add(GST_BIN_CAST(pipeline), appSrc);

	GstElement* decoder = nullptr;
	if (m_useOmx == true)
	{
		decoder = gst_element_factory_make("omxh264dec", nullptr);
	}
	else
	{
		decoder = gst_element_factory_make("avdec_h264", nullptr);
	}

	gst_bin_add(GST_BIN_CAST(pipeline), decoder);
	gst_element_link(appSrc, decoder);

	auto converter = gst_element_factory_make("videoconvert", nullptr);
	auto appSink = gst_element_factory_make("appsink", nullptr);
	gst_bin_add(GST_BIN_CAST(pipeline), converter);
	gst_bin_add(GST_BIN_CAST(pipeline), appSink);
	auto displaySinkCaps = gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, "RGBA",
		nullptr
	);
	g_object_set(appSink,
		"caps", displaySinkCaps,
		"max-buffers", 1,
		"drop", true,
		"sync", false,
		nullptr
	);
	gst_element_link(decoder, converter);
	gst_element_link(converter, appSink);

	widget->installAppSink(GST_APP_SINK_CAST(appSink));

	if (m_videoListener != nullptr)
	{
		m_videoListener->installAppSrc(GST_APP_SRC_CAST(appSrc));
	}


	widget->show();

	auto pipelineStartSucess = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (pipelineStartSucess == GST_STATE_CHANGE_FAILURE)
	{
		qWarning() << "Set pipeline to playing failed";
		gst_bus_set_flushing(bus, true);
	}
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
				<< dds::core::policy::Liveliness::Automatic()
				<< dds::core::policy::History(dds::core::policy::HistoryKind::KEEP_LAST, 20);

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

