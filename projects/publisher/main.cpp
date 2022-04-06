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

#include "videoddspublisher.h"
#include "videowidgetpaintergst.h"

#include "dds/dds.hpp"
#include "VideoDDS.hpp"

using namespace org::eclipse::cyclonedds;

int main(int argc, char *argv[])
{
	try
	{
		QApplication application(argc, argv);
		application.setApplicationName("Video DDS Publisher");

		GError* error = nullptr;

		gboolean use_testsrc = FALSE;
		gboolean use_omx = FALSE;
		gboolean use_fixed = FALSE;
		gint strength = 1000;

		GOptionEntry entries[] =
		{
			{"testsrc", 't', 0, G_OPTION_ARG_NONE, &use_testsrc, "Use test src instead of camera", nullptr},
			{"omx", 'o', 0, G_OPTION_ARG_NONE, &use_omx, "Use omx as the encoder", nullptr},
			{"fixed", 'f', 0, G_OPTION_ARG_NONE, &use_fixed, "Use fixed capabilities for the camera source", nullptr},
			{"strength", 's', 0, G_OPTION_ARG_INT, &strength, "DDS ownership strength", "S"},
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
		const dds::domain::DomainParticipant dp{domain::default_id()};
		const dds::topic::qos::TopicQos topicQos{dp.default_topic_qos()};
		const dds::topic::Topic<S2E::Video> topic{dp, topicName, topicQos};
		const dds::pub::qos::PublisherQos publisherQos{dp.default_publisher_qos()};
		const dds::pub::Publisher publisher{dp, publisherQos};
		// Create a topic QoS with exclusive ownership and defined liveliness.
		// The exclusive ownership allows the use of the ownership strength to define which video source is used.
		// The liveliness topic determines how to long to wait until the source with lower strength is used
		// when messages are not received from the source with higher ownership strength.
		dds::pub::qos::DataWriterQos dataWriterQos{topic.qos()};
		dataWriterQos << dds::core::policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
		dataWriterQos << dds::core::policy::Ownership::Exclusive();
		dataWriterQos << dds::core::policy::Liveliness::ManualByTopic(dds::core::Duration::from_millisecs(1000));
		dataWriterQos << dds::core::policy::OwnershipStrength(strength);
		dds::pub::DataWriter<S2E::Video> dataWriter{publisher, topic, dataWriterQos};

		VideoDDSpublisher videoPublisher{dataWriter, bool(use_testsrc), bool(use_omx), bool(use_fixed)};


		VideoWidgetPainterGst widget(videoPublisher.appsink());
		widget.show();

		return application.exec();
	}
	catch (const dds::core::OutOfResourcesError& e)
	{
		std::cerr << "DDS OutOfResourcesError: " << e.what() << std::endl;
	}
	catch (const dds::core::InvalidArgumentError& e)
	{
		std::cerr << "DDS InvalidArgumentError: " << e.what() << std::endl;
	}
	catch (const dds::core::NullReferenceError& e)
	{
		std::cerr << "DDS NullReferenceError: " << e.what() << std::endl;
	}
	catch (const dds::core::Error& e)
	{
		std::cerr << "DDS Error: " << e.what() << std::endl;
	}
	catch (const std::range_error& e)
	{
		std::cerr << "range_error: " << e.what() << std::endl;
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "runtime_error: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Initialization failed with unhandled exception" << std::endl;
	}
}
