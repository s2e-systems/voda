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

#include  <stdexcept>

VideoDDSsubscriber::VideoDDSsubscriber(bool useOmx)
{
	m_pipeline = gst_pipeline_new("subscriber");
	if (m_pipeline == nullptr)
	{
		throw std::runtime_error{"gst_pipeline_new failed"};
	}
	auto srcCaps = gst_caps_new_simple("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "au",
		nullptr
	);

	m_ddsAppSrc = gst_element_factory_make("appsrc", nullptr);
	if (m_ddsAppSrc == nullptr)
	{
		throw std::runtime_error{"gst_element_factory_make appsrc failed"};
	}
	g_object_set(m_ddsAppSrc,
		"caps", srcCaps,
		"is-live", true,
		"format", GST_FORMAT_TIME,
		nullptr
	);

	gst_bin_add(GST_BIN_CAST(m_pipeline), m_ddsAppSrc);

	GstElement* decoder = nullptr;
	if (useOmx == true)
	{
		decoder = gst_element_factory_make("omxh264dec", nullptr);
	}
	else
	{
		decoder = gst_element_factory_make("avdec_h264", nullptr);
	}

	gst_bin_add(GST_BIN_CAST(m_pipeline), decoder);
	gst_element_link(m_ddsAppSrc, decoder);

	auto converter = gst_element_factory_make("videoconvert", nullptr);
	m_displayAppSink = gst_element_factory_make("glimagesink", nullptr);
	gst_bin_add(GST_BIN_CAST(m_pipeline), converter);
	gst_bin_add(GST_BIN_CAST(m_pipeline), m_displayAppSink);
	auto displaySinkCaps = gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, "RGBA",
		nullptr
	);
	g_object_set(m_displayAppSink,
		"sync", false,
		nullptr
	);
	gst_element_link(decoder, converter);
	gst_element_link(converter, m_displayAppSink);


	auto pipelineStartSucess = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
	if (pipelineStartSucess == GST_STATE_CHANGE_FAILURE)
	{
		auto bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
		gst_bus_set_flushing(bus, true);
		gst_object_unref(bus);
		throw std::runtime_error{"Set pipeline to playing failed"};
	}
}

GstAppSink* VideoDDSsubscriber::displayAppSink() const
{
	return GST_APP_SINK_CAST(m_displayAppSink);
}
GstAppSrc* VideoDDSsubscriber::ddsAppSrc() const
{
	return GST_APP_SRC_CAST(m_ddsAppSrc);
}

GstPipeline* VideoDDSsubscriber::pipeline() const
{
	return GST_PIPELINE_CAST(m_pipeline);
}