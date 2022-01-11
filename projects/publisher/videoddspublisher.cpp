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
#include <stdexcept>
#include <string>
#include <vector>

#include "qtgstreamer.h"
#include "videowidgetpaintergst.h"
#include "elements.h"
#include "cameracapabilities.h"

static GstFlowReturn pullSampleAndSendViaDDS(GstAppSink* appSink, gpointer userData)
{
	auto dataWriter = reinterpret_cast<dds::pub::DataWriter<S2E::Video>* >(userData);
	if (dataWriter == nullptr || dataWriter->is_nil())
	{
		qCritical() << "Data writer for dds app sink not valid";
		return GST_FLOW_ERROR;
	}

	const int userid = 0;
	// Count the samples that have arrived. Used also to define the
	// DDS msg number later
	static int frameNum = 0;
	frameNum++;

	// Pull a sample from the GStreamer pipeline
	auto sample = gst_app_sink_pull_sample(appSink);
	if(sample != nullptr)
	{
		auto sampleBuffer = gst_sample_get_buffer(sample);
		if(sampleBuffer != nullptr)
		{
			GstMapInfo mapInfo;
			gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

			const auto byteCount = int(mapInfo.size);
			auto rawData = static_cast<uint8_t* >(mapInfo.data);
			const std::vector<uint8_t> frame(rawData, rawData + byteCount);
			const S2E::Video sample(userid, frameNum, frame);
			*dataWriter << sample;
			gst_buffer_unmap(sampleBuffer, &mapInfo);

			qDebug() << "Send DDS msg" << frameNum << "with size of" << byteCount << "Bytes";

		}
		// The S2E::Video sample and the std::vector<uint8_t> frame are destroyed here.
		// How ever the DDS system takes care that the necessary data stays alive.
		// As such some data is compied around here.
		// TODO: the copying may be avoided by using other function.
		gst_sample_unref(sample);
	}

	return GST_FLOW_OK;
}

VideoDDSpublisher::~VideoDDSpublisher()
{
	auto bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
	auto pipelineStartSucess = gst_element_set_state(m_pipeline, GST_STATE_NULL);
	if (pipelineStartSucess == GST_STATE_CHANGE_FAILURE)
	{
		qWarning() << "Set pipeline to null failed";
		gst_bus_set_flushing(bus, true);
	}
	// Switch off emitting of signals by the dds appsink. This
	// is to prevent calling of the callback function and using
	// dds resources after destruction of this class (in case the
	// above setting of the pipeline to null did fail)
	auto ddsAppSink = gst_bin_get_by_name(GST_BIN_CAST(m_pipeline), "ddsAppSink");
	g_object_set(ddsAppSink, "emit-signals", false, nullptr);
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

	std::vector<std::string> sourceCanditates;
	if (!useTestSrc)
	{
		sourceCanditates.push_back("ksvideosrc");
		sourceCanditates.push_back("v4l2src");
	}
	sourceCanditates.push_back("videotestsrc");

	ElementSelection sourceSelection{sourceCanditates, "source"};
	qDebug() << "selected source element: " << QString::fromStdString(sourceSelection.elementName());

	gst_element_set_state(sourceSelection.element(), GST_STATE_READY);
	gst_element_get_state(sourceSelection.element(), nullptr /*state*/, nullptr /*pending*/, GST_CLOCK_TIME_NONE);
	auto caps = gst_pad_query_caps(gst_element_get_static_pad(sourceSelection.element(), "src"), nullptr);

	GstCaps *capsFilter = nullptr;

	if (useFixedCaps)
	{
		qDebug() << "Using fixed capabilities";
		capsFilter = gst_caps_new_simple("video/x-raw",
										 "width", G_TYPE_INT, 640,
										 "height", G_TYPE_INT, 480,
										 "framerate", GST_TYPE_FRACTION, 30, 1,
										 nullptr);
	}
	else
	if (useTestSrc || sourceSelection.elementName() == "videotestsrc")
	{
		qDebug() << "Using fixed capabilities for test source";
		capsFilter = gst_caps_new_simple("video/x-raw",
										 "format", G_TYPE_STRING, "I420",
										 "width", G_TYPE_INT, 640,
										 "height", G_TYPE_INT, 480,
										 "framerate", GST_TYPE_FRACTION, 30, 1,
										 nullptr);
	}
	else
	{
		CapabilitySelection capsSelection{caps};
		const auto framerate = capsSelection.highestRawFrameRate();
		qDebug() << "Detected highest framerate as:" << framerate << "and use this to determine highest pixel area";
		capsFilter = capsSelection.highestRawArea(framerate);
	}
	qDebug() << "Usiing following capabilities for the source element:" << gst_caps_to_string(capsFilter);

	auto sourceBin = GST_BIN_CAST(gst_bin_new("sourceBin"));
	gst_bin_add(sourceBin, sourceSelection.element());
	auto filter = gst_element_factory_make("capsfilter", nullptr);
	g_object_set(filter, "caps", capsFilter, nullptr);
	gst_bin_add(sourceBin, filter);
	gst_element_link(sourceSelection.element(), filter);
	auto converter = gst_element_factory_make("videoconvert", nullptr);
	gst_bin_add(sourceBin, converter);
	gst_element_link(filter, converter);

	auto padLastSource = gst_element_get_static_pad(converter, "src");
	gst_element_add_pad(GST_ELEMENT_CAST(sourceBin), gst_ghost_pad_new("src", padLastSource));
	gst_object_unref(GST_OBJECT(padLastSource));


	//////////////
	// DDS

	auto encodereBin = GST_BIN_CAST(gst_bin_new("encodereBin"));

	auto ddsAppSink  = gst_element_factory_make("appsink", "ddsAppSink");
	gst_bin_add(encodereBin, ddsAppSink);
	auto ddsSinkCaps = gst_caps_new_simple("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "au",
		nullptr
	);

	g_object_set(ddsAppSink,
		"emit-signals", true,
		"caps", ddsSinkCaps,
		"max-buffers", 1,
		"drop", false,
		"sync", false,
		nullptr
	);

	// Encoder

	GstElementFactory* factory = nullptr;
	if (useOmx)
	{
		factory = gst_element_factory_find("avenc_h264_omx");
	}
	else
	{
		factory = gst_element_factory_find("x264enc");
	}
	if (factory == nullptr)
	{
		throw std::runtime_error{"No existing encoder found"};
	}
	auto encoder = gst_element_factory_create(factory, nullptr);
	const std::string encoderName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(gst_element_get_factory(encoder)));

	const int kilobitrate = 1280;
	const int keyframedistance = 30;

	gst_bin_add(encodereBin, encoder);
	if (encoderName == "avenc_h264_omx")
	{
		g_object_set(encoder,
			"bitrate", gint64(kilobitrate * 1000), //set bitrate (in bits/s)
			"gop-size", gint(keyframedistance),	//set the group of picture (GOP) size
			nullptr
		);
		// The avenc_h264_omx does not send the PPS/SPS with the IDR frames
		// the parser will do so
		auto parser = gst_element_factory_make("h264parse", nullptr);
		g_object_set(parser, "config-interval", gint(-1), nullptr);
		gst_bin_add(encodereBin, parser);
		gst_element_link(encoder, parser);
		gst_element_link(parser, ddsAppSink);
	}
	else
	if (encoderName == "x264enc")
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
			nullptr
		);
		gst_element_link(encoder, ddsAppSink);
	}
	else
	{
		throw std::runtime_error("Encoder not valid");
	}



	auto padFirstEncoder = gst_element_get_static_pad(encoder, "sink");
	gst_element_add_pad(GST_ELEMENT_CAST(encodereBin), gst_ghost_pad_new("sink", padFirstEncoder));
	gst_object_unref(GST_OBJECT(padFirstEncoder));

	g_signal_connect(ddsAppSink, "new-sample", G_CALLBACK(pullSampleAndSendViaDDS),
					 reinterpret_cast<gpointer>(&m_dataWriter));

	///////////
	// Display

	auto displayBin = GST_BIN_CAST(gst_bin_new("displayBin"));
	auto displayConverter = gst_element_factory_make("videoconvert", nullptr);
	auto appSink = gst_element_factory_make("appsink", "displayAppSink");
	gst_bin_add(displayBin, displayConverter);
	gst_bin_add(displayBin, appSink);
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
	gst_element_link(displayConverter, appSink);

	auto padFirstDisplay = gst_element_get_static_pad(displayConverter, "sink");
	gst_element_add_pad(GST_ELEMENT_CAST(displayBin), gst_ghost_pad_new("sink", padFirstDisplay));
	gst_object_unref(GST_OBJECT(padFirstDisplay));

	///////////
	// Create pipeline and bring the bins together

	m_pipeline = gst_pipeline_new("publisher");
	auto bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
	gst_bus_set_sync_handler(bus, QtGStreamer::busCallBack /*function*/, nullptr /*user_data*/, nullptr /*notify function*/);
	gst_object_unref(bus);

	auto tee = gst_element_factory_make("tee", nullptr);
	auto queue0 = gst_element_factory_make("queue", nullptr);
	auto queue1 = gst_element_factory_make("queue", nullptr);

	gst_bin_add(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(sourceBin));
	gst_bin_add(GST_BIN_CAST(m_pipeline), tee);
	gst_bin_add(GST_BIN_CAST(m_pipeline), queue0);
	gst_bin_add(GST_BIN_CAST(m_pipeline), queue1);
	gst_bin_add(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(encodereBin));
	gst_bin_add(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(displayBin));

	auto boolret = gst_element_link(GST_ELEMENT_CAST(sourceBin), tee);
	boolret &= gst_element_link(queue0, GST_ELEMENT_CAST(encodereBin));
	boolret &= gst_element_link(queue1, GST_ELEMENT_CAST(displayBin));
	boolret &= gst_element_link_pads(tee, "src_0", queue0, "sink");
	boolret &= gst_element_link_pads(tee, "src_1", queue1, "sink");

	if (boolret != gboolean(true))
	{
		qWarning() << "Linking pipeline failed!";
	}

	auto pipelineStartSucess = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
	if (pipelineStartSucess == GST_STATE_CHANGE_FAILURE)
	{
		qWarning() << "Set pipeline to playing failed";
		gst_bus_set_flushing(bus, true);
	}
}


GstAppSink* VideoDDSpublisher::appsink()
{
	return GST_APP_SINK_CAST(gst_bin_get_by_name(GST_BIN_CAST(m_pipeline), "displayAppSink"));
}
