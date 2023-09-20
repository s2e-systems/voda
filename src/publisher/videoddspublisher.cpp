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

#include <stdexcept>
#include <string>
#include <vector>

#include "elements.h"
#include "cameracapabilities.h"

static GstFlowReturn pullSampleAndSendViaDDS(GstAppSink* appSink, gpointer userData)
{
	auto dataWriter = reinterpret_cast<dds::pub::DataWriter<S2E::Video>*>(userData);
	if (dataWriter == nullptr || dataWriter->is_nil())
	{
		return GST_FLOW_ERROR;
	}

	const int userid = 0;
	// Count the samples that have arrived. Used also to define the
	// DDS msg number later
	static int frameNum = 0;
	frameNum++;

	// Pull a sample from the GStreamer pipeline
	auto sample = gst_app_sink_pull_sample(appSink);
	if (sample != nullptr)
	{
		auto sampleBuffer = gst_sample_get_buffer(sample);
		if (sampleBuffer != nullptr)
		{
			GstMapInfo mapInfo;
			gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

			const auto byteCount = int(mapInfo.size);
			const auto rawData = static_cast<uint8_t*>(mapInfo.data);
			const dds::core::ByteSeq frame{ rawData, rawData + byteCount };
			*dataWriter << S2E::Video{ userid, frameNum, frame };
			gst_buffer_unmap(sampleBuffer, &mapInfo);

			std::cout << "sent frame number " << frameNum << std::endl;
		}
		gst_sample_unref(sample);
	}

	return GST_FLOW_OK;
}

VideoDDSpublisher::~VideoDDSpublisher()
{
	auto bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
	auto pipelineStartSuccess = gst_element_set_state(m_pipeline, GST_STATE_NULL);
	if (pipelineStartSuccess == GST_STATE_CHANGE_FAILURE)
	{
		gst_bus_set_flushing(bus, true);
	}
	// Switch off emitting of signals by the dds appsink. This
	// is to prevent calling of the callback function and using
	// dds resources after destruction of this class (in case the
	// above setting of the pipeline to null did fail)
	auto ddsAppSink = gst_bin_get_by_name(GST_BIN_CAST(m_pipeline), "ddsAppSink");
	g_object_set(ddsAppSink, "emit-signals", false, nullptr);
}

void identity_handoff(GstElement* object, GstBuffer* buffer, gpointer user_data)
{
	static int previous_pts = -1;
	static int previous_dts = -1;
	static int previous_time = -1;

	std::cout << "identity_handoff " << GST_ELEMENT_NAME(object) << ": ";
	if (GST_BUFFER_PTS_IS_VALID(buffer))
	{
		const int pts = GST_BUFFER_DTS(buffer) / 1000000;
		std::cout << "pts: " << pts << "ms ";
		std::cout << "pts_diff: " << pts - previous_pts << "ms ";
		previous_pts = pts;
	}
	if (GST_BUFFER_DTS_IS_VALID(buffer))
	{
		const int dts = GST_BUFFER_DTS(buffer) / 1000000;
		std::cout << "dts: " << dts << "ms ";
		std::cout << "dts_diff: " << dts - previous_dts << "ms ";
		previous_dts = dts;
	}
	if (GST_BUFFER_DURATION_IS_VALID(buffer))
	{
		const int duration = GST_BUFFER_DURATION(buffer) / 1000000;
		std::cout << "duration: " << duration << "ms ";
	}

	auto pipeline_element = gst_element_get_parent(object);
	if (!GST_IS_PIPELINE(pipeline_element))
	{
		pipeline_element = gst_element_get_parent(pipeline_element);
	}
	const auto pipeline = GST_PIPELINE_CAST(pipeline_element);

	const auto time = gst_clock_get_time(gst_pipeline_get_clock(pipeline)) / 1000000;
	const auto time_diff = time - previous_time;
	const auto fps = time_diff > 0 ? 1000 / time_diff : 0;
	std::cout << "time: " << time << "ms ";
	std::cout << "time_diff: " << time_diff << "ms " << "fps: " << fps;
	previous_time = time;

	std::cout << std::endl;
}


void identity_handoff2(GstElement* object, GstBuffer* buffer, gpointer user_data)
{
	static int previous_pts = -1;
	static int previous_dts = -1;
	static int previous_time = -1;

	std::cout << "identity_handoff " << GST_ELEMENT_NAME(object) << ": ";
	if (GST_BUFFER_PTS_IS_VALID(buffer))
	{
		const int pts = GST_BUFFER_DTS(buffer) / 1000000;
		std::cout << "pts: " << pts << "ms ";
		std::cout << "pts_diff: " << pts - previous_pts << "ms ";
		previous_pts = pts;
	}
	if (GST_BUFFER_DTS_IS_VALID(buffer))
	{
		const int dts = GST_BUFFER_DTS(buffer) / 1000000;
		std::cout << "dts: " << dts << "ms ";
		std::cout << "dts_diff: " << dts - previous_dts << "ms ";
		previous_dts = dts;
	}
	if (GST_BUFFER_DURATION_IS_VALID(buffer))
	{
		const int duration = GST_BUFFER_DURATION(buffer) / 1000000;
		std::cout << "duration: " << duration << "ms ";
	}

	auto pipeline_element = gst_element_get_parent(object);
	if (!GST_IS_PIPELINE(pipeline_element))
	{
		pipeline_element = gst_element_get_parent(pipeline_element);
	}
	const auto pipeline = GST_PIPELINE_CAST(pipeline_element);

	const auto time = gst_clock_get_time(gst_pipeline_get_clock(pipeline)) / 1000000;
	const auto time_diff = time - previous_time;
	const auto fps = time_diff > 0 ? 1000 / time_diff : 0;
	std::cout << "time: " << time << "ms ";
	std::cout << "time_diff: " << time_diff << "ms " << "fps: " << fps;
	previous_time = time;

	std::cout << std::endl;
}

VideoDDSpublisher::VideoDDSpublisher(dds::pub::DataWriter<S2E::Video>& dataWriter, bool useTestSrc, bool useOmx, bool useFixedCaps)
	: m_dataWriter(dataWriter)
{
	//////////////
	// Source

	// Supported formats of the webcam may be gathered by using gst-launch, e.g. on Windows:
	// gst-launch-1.0 --gst-debug=*src:5 ksvideosrc num-buffers=1 ! fakesink
	// or on Linux:
	// gst-launch-1.0 --gst-debug=*src:5 v4l2src num-buffers=1 ! fakesink

	std::vector<std::string> sourceCandidates;
	if (!useTestSrc)
	{
		sourceCandidates.push_back("ksvideosrc");
		sourceCandidates.push_back("v4l2src");
	}
	sourceCandidates.push_back("videotestsrc");

	ElementSelection sourceSelection{ sourceCandidates, "source" };

	gst_element_set_state(sourceSelection.element(), GST_STATE_READY);
	gst_element_get_state(sourceSelection.element(), nullptr /*state*/, nullptr /*pending*/, GST_CLOCK_TIME_NONE);
	const auto caps = gst_pad_query_caps(gst_element_get_static_pad(sourceSelection.element(), "src"), nullptr);

	GstCaps* capsFilter = nullptr;

	if (useFixedCaps)
	{
		capsFilter = gst_caps_new_simple("video/x-raw",
			"width", G_TYPE_INT, 640,
			"height", G_TYPE_INT, 480,
			"framerate", GST_TYPE_FRACTION, 30, 1,
			nullptr);
	}
	else if (useTestSrc || sourceSelection.elementName() == "videotestsrc")
	{
		capsFilter = gst_caps_new_simple("video/x-raw",
			"format", G_TYPE_STRING, "I420",
			"width", G_TYPE_INT, 640,
			"height", G_TYPE_INT, 480,
			"framerate", GST_TYPE_FRACTION, 30, 1,
			nullptr);
		g_object_set(sourceSelection.element(), "horizontal-speed", 5, nullptr);
	}
	else
	{
		CapabilitySelection capsSelection{ caps };
		const auto framerate = capsSelection.highestRawFrameRate();
		capsFilter = capsSelection.highestRawArea(framerate);

		const auto selected_caps_structure = gst_caps_get_structure(capsFilter, 0);
		const auto fieldtype = gst_structure_get_field_type(selected_caps_structure, "framerate");

		const auto value = gst_structure_get_value(selected_caps_structure, "framerate");
		volatile auto denom = gst_value_get_fraction_denominator(value);
		volatile auto num = gst_value_get_fraction_numerator(value);
	}

	g_object_set(sourceSelection.element(), "do-timestamp", true, nullptr);

	auto sourceBin = GST_BIN_CAST(gst_bin_new("sourceBin"));
	gst_bin_add(sourceBin, sourceSelection.element());
	auto filter = gst_element_factory_make("capsfilter", nullptr);
	g_object_set(filter, "caps", capsFilter, nullptr);
	gst_bin_add(sourceBin, filter);
	volatile auto ret1 = gst_element_link(sourceSelection.element(), filter);
	auto converter = gst_element_factory_make("videoconvert", nullptr);
	gst_bin_add(sourceBin, converter);
	volatile auto ret2 = gst_element_link(filter, converter);

	auto padLastSource = gst_element_get_static_pad(converter, "src");
	gst_element_add_pad(GST_ELEMENT_CAST(sourceBin), gst_ghost_pad_new("src", padLastSource));
	gst_object_unref(GST_OBJECT(padLastSource));

	//////////////
	// DDS

	auto encoderBin = GST_BIN_CAST(gst_bin_new("encoderBin"));

	auto ddsAppSink = gst_element_factory_make("appsink", "ddsAppSink");

	const auto identity2 = gst_element_factory_make("identity", "identity2");
	g_signal_connect(identity2, "handoff", G_CALLBACK(identity_handoff2), nullptr);

	gst_bin_add(encoderBin, ddsAppSink);
	gst_bin_add(encoderBin, identity2);
	auto ddsSinkCaps = gst_caps_new_simple("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "au",
		nullptr);

	g_object_set(ddsAppSink,
		"emit-signals", true,
		"caps", ddsSinkCaps,
		"max-buffers", 1,
		"drop", false,
		"sync", false,
		nullptr);

	// Encoder

	GstElementFactory* factory = nullptr;
	if (useOmx)
	{
		factory = gst_element_factory_find("avenc_h264_omx");
	}
	else
	{
		// factory = gst_element_factory_find("x264enc");
		factory = gst_element_factory_find("openh264enc");
	}
	if (factory == nullptr)
	{
		throw std::runtime_error{ "No existing encoder found" };
	}
	auto encoder = gst_element_factory_create(factory, nullptr);
	const std::string encoderName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(gst_element_get_factory(encoder)));

	const int kilobitrate = 1280;
	const int keyframedistance = 30;

	gst_bin_add(encoderBin, encoder);
	if (encoderName == "avenc_h264_omx")
	{
		g_object_set(encoder,
			"bitrate", gint64(kilobitrate * 1000), // set bitrate (in bits/s)
			"gop-size", gint(keyframedistance),	// set the group of picture (GOP) size
			nullptr);
		// The avenc_h264_omx does not send the PPS/SPS with the IDR frames
		// the parser will do so
		auto parser = gst_element_factory_make("h264parse", nullptr);
		g_object_set(parser, "config-interval", gint(-1), nullptr);
		gst_bin_add(encoderBin, parser);
		gst_element_link(encoder, parser);
		gst_element_link(parser, identity2);
	}
	else if (encoderName == "x264enc")
	{
		g_object_set(encoder,
			"bitrate", guint(kilobitrate),			 // Bitrate in kbit/sec
			"vbv-buf-capacity", guint(2000),		 // Size of the VBV buffer in milliseconds
			"key-int-max", guint(keyframedistance), // Maximal distance between two key-frames (0 for automatic)
			"threads", guint(1),					 // Number of threads used by the codec (0 for automatic)
			"sliced-threads", gboolean(false),		 // Low latency but lower efficiency threading
			"insert-vui", gboolean(false),
			"speed-preset", 1, // Preset name for speed/quality tradeoff options
			"trellis", gboolean(false),
			"aud", gboolean(false), // Use AU (Access Unit) delimiter
			nullptr);
		gst_element_link(encoder, identity2);
	}
	else if (encoderName == "openh264enc")
	{
		gst_element_link(encoder, identity2);
	}
	else
	{
		throw std::runtime_error("Encoder not valid");
	}

	gst_element_link(identity2, ddsAppSink);

	const auto padFirstEncoder = gst_element_get_static_pad(encoder, "sink");
	gst_element_add_pad(GST_ELEMENT_CAST(encoderBin), gst_ghost_pad_new("sink", padFirstEncoder));
	gst_object_unref(GST_OBJECT(padFirstEncoder));

	g_signal_connect(ddsAppSink, "new-sample", G_CALLBACK(pullSampleAndSendViaDDS),
		reinterpret_cast<gpointer>(&m_dataWriter));

	///////////
	// Display

	auto displayBin = GST_BIN_CAST(gst_bin_new("displayBin"));
	auto displayConverter = gst_element_factory_make("videoconvert", nullptr);
	auto imageSink = gst_element_factory_make("glimagesink", "displayAppSink");
	gst_bin_add(displayBin, displayConverter);
	gst_bin_add(displayBin, imageSink);
	auto displaySinkCaps = gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, "RGBA",
		nullptr);
	g_object_set(imageSink,
		"sync", false,
		nullptr);
	gst_element_link(displayConverter, imageSink);

	const auto padFirstDisplay = gst_element_get_static_pad(displayConverter, "sink");
	gst_element_add_pad(GST_ELEMENT_CAST(displayBin), gst_ghost_pad_new("sink", padFirstDisplay));
	gst_object_unref(GST_OBJECT(padFirstDisplay));

	///////////
	// Create pipeline and bring the bins together

	m_pipeline = gst_pipeline_new("publisher");

	const auto tee = gst_element_factory_make("tee", nullptr);
	const auto queue0 = gst_element_factory_make("queue", nullptr);
	const auto queue1 = gst_element_factory_make("queue", nullptr);

	g_object_set(queue0,
		"leaky", 0,
		"max-size-buffers", 1,
		nullptr);
	g_object_set(queue1,
		"leaky", 0,
		"max-size-buffers", 1,
		nullptr);

	const auto identity = gst_element_factory_make("identity", "identity1");
	gst_bin_add(GST_BIN_CAST(m_pipeline), identity);
	g_signal_connect(identity, "handoff", G_CALLBACK(identity_handoff), nullptr);

	gst_bin_add(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(sourceBin));
	gst_bin_add(GST_BIN_CAST(m_pipeline), tee);
	gst_bin_add(GST_BIN_CAST(m_pipeline), queue0);
	gst_bin_add(GST_BIN_CAST(m_pipeline), queue1);
	gst_bin_add(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(encoderBin));
	gst_bin_add(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(displayBin));

	volatile auto ret7 = gst_element_link(GST_ELEMENT_CAST(sourceBin), identity);
	volatile auto ret8 = gst_element_link(identity, tee);
	volatile auto ret3 = gst_element_link(queue0, GST_ELEMENT_CAST(encoderBin));
	volatile auto ret4 = gst_element_link(queue1, GST_ELEMENT_CAST(displayBin));
	volatile auto ret5 = gst_element_link_pads(tee, "src_0", queue0, "sink");
	volatile auto ret6 = gst_element_link_pads(tee, "src_1", queue1, "sink");

	// volatile auto ret3 = gst_element_link(identity, GST_ELEMENT_CAST(displayBin));

	// if (boolret != gboolean(true))
	// {
	// 	throw std::runtime_error("Linking pipeline failed");
	// }

	auto pipelineStartSuccess = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
	if (pipelineStartSuccess == GST_STATE_CHANGE_FAILURE)
	{
		throw std::runtime_error("Linking pipeline failed");
	}
}

GstAppSink* VideoDDSpublisher::appsink()
{
	return GST_APP_SINK_CAST(gst_bin_get_by_name(GST_BIN_CAST(m_pipeline), "displayAppSink"));
}

GstPipeline* VideoDDSpublisher::pipeline()
{
	return GST_PIPELINE_CAST(m_pipeline);
}
