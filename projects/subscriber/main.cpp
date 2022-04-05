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

#include <stdexcept>
#include <string>

#include <glib.h>

#include "videoddssubscriber.h"
#include "videolistener.h"
#include "videowidgetpaintergst.h"

#include "dds/dds.hpp"
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;

int main(int argc, char *argv[])
{
	try
	{
		QApplication application{argc, argv};
		application.setApplicationName("Video DDS Subscriber");

		GError* error = nullptr;
		gboolean use_omx = FALSE;
		GOptionEntry entries[] =
		{
			{"omx", 'o', 0, G_OPTION_ARG_NONE, &use_omx, "Use omx as the encoder", nullptr},
			{nullptr}
		};

		gst_debug_set_color_mode(GST_DEBUG_COLOR_MODE_OFF);
		gst_debug_set_active(TRUE);
		if (!gst_init_check(&argc, &argv, &error))
		{
			const std::string error_message = "Could not initialize GStreamer: " + std::string{error->message};
			g_error_free(error);
			throw std::runtime_error{error_message};
		}
		gst_debug_set_default_threshold(GST_LEVEL_FIXME);

		GOptionContext* option_context = g_option_context_new("[APPLICATION OPTIONS]");
		g_option_context_add_main_entries(option_context, entries, nullptr);
		g_option_context_add_group(option_context, gst_init_get_option_group());
		if (!g_option_context_parse(option_context, &argc, &argv, &error))
		{
			const auto error_message = std::string{error->message};
			g_error_free(error);
			throw std::runtime_error{error_message};
		}
		g_option_context_free(option_context);


		const std::string topicName = "VideoStream";

		// Create a domain participant using the default ID configured on the XML file
		dds::domain::DomainParticipant dp{domain::default_id()};
		// Create a topic QoS with exclusive ownership. The exclusive ownership allows
		// the use of the ownership strength to define which video source is used.
		const dds::topic::qos::TopicQos topicQos{dp.default_topic_qos()};
		const dds::topic::Topic<S2E::Video> topic{dp, topicName, topicQos};
		const dds::sub::qos::SubscriberQos subscriberQos{dp.default_subscriber_qos()};
		const dds::sub::Subscriber subscriber{dp, subscriberQos};
		dds::sub::qos::DataReaderQos dataReaderQos{topic.qos()};
		dataReaderQos << dds::core::policy::Ownership::Exclusive();
		dataReaderQos << dds::core::policy::Liveliness::Automatic();
		dataReaderQos << dds::core::policy::History{dds::core::policy::HistoryKind::KEEP_LAST, 20};
		// Trigger the callback functions when data becomes available or when the
		// requested deadline is missed (currently not used)
		dds::core::status::StatusMask mask;
		mask << dds::core::status::StatusMask::data_available();
		mask << dds::core::status::StatusMask::requested_deadline_missed();

		const VideoDDSsubscriber videoSubscriber{bool(use_omx)};

		// Create a video listener which triggers the callbacks necessary for showing
		// the video data when a new message is received
		VideoListener videoListener{videoSubscriber.ddsAppSrc()};

		// Create the data reader for the video topic
		dds::sub::DataReader<S2E::Video> dataReader{subscriber, topic, dataReaderQos, &videoListener, mask};

		VideoWidgetPainterGst widget{videoSubscriber.displayAppSink()};
		widget.show();

		return application.exec();
	}
	catch(const dds::core::OutOfResourcesError& e)
	{
		std::cerr << "DDS OutOfResourcesError: " << e.what() << std::endl;
	}
	catch(const dds::core::InvalidArgumentError& e)
	{
		std::cerr << "DDS InvalidArgumentError: " << e.what() << std::endl;
	}
	catch(const dds::core::NullReferenceError& e)
	{
		std::cerr << "DDS NullReferenceError: " << e.what() << std::endl;
	}
	catch(const dds::core::Error& e)
	{
		std::cerr << "DDS Error: " << e.what() << std::endl;
	}
	catch(const std::runtime_error& e)
	{
		std::cerr << "Runtime error: " << e.what() << std::endl;
	}
	catch(...)
	{
		std::cerr << "DDS initialization failed with unhandled exception" << std::endl;
	}
}
