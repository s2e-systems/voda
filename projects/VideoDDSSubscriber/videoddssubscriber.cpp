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

VideoDDSsubscriber::VideoDDSsubscriber(bool useOmx)
{

	auto pipeline = gst_pipeline_new("subscriber");
	auto bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_set_sync_handler(bus, Pipeline::busCallBack /*function*/, nullptr /*user_data*/, nullptr /*notify function*/);
	gst_object_unref(bus);

	auto srcCaps = gst_caps_new_simple("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "au",
		nullptr
	);

	m_ddsAppSrc = gst_element_factory_make("appsrc", nullptr);
	g_object_set(m_ddsAppSrc,
		"caps", srcCaps,
		"is-live", true,
		"format", GST_FORMAT_TIME,
		nullptr
	);

	gst_bin_add(GST_BIN_CAST(pipeline), m_ddsAppSrc);

	GstElement* decoder = nullptr;
	if (useOmx == true)
	{
		decoder = gst_element_factory_make("omxh264dec", nullptr);
	}
	else
	{
		decoder = gst_element_factory_make("avdec_h264", nullptr);
	}

	gst_bin_add(GST_BIN_CAST(pipeline), decoder);
	gst_element_link(m_ddsAppSrc, decoder);

	auto converter = gst_element_factory_make("videoconvert", nullptr);
	m_displayAppSink = gst_element_factory_make("appsink", nullptr);
	gst_bin_add(GST_BIN_CAST(pipeline), converter);
	gst_bin_add(GST_BIN_CAST(pipeline), m_displayAppSink);
	auto displaySinkCaps = gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, "RGBA",
		nullptr
	);
	g_object_set(m_displayAppSink,
		"caps", displaySinkCaps,
		"max-buffers", 1,
		"drop", true,
		"sync", false,
		nullptr
	);
	gst_element_link(decoder, converter);
	gst_element_link(converter, m_displayAppSink);


	auto pipelineStartSucess = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (pipelineStartSucess == GST_STATE_CHANGE_FAILURE)
	{
		qWarning() << "Set pipeline to playing failed";
		gst_bus_set_flushing(bus, true);
	}
}

GstAppSink* VideoDDSsubscriber::displayAppSink()
{
	return GST_APP_SINK_CAST(m_displayAppSink);
}
GstAppSrc* VideoDDSsubscriber::ddsAppSrc()
{
	return GST_APP_SRC_CAST(m_ddsAppSrc);
}
