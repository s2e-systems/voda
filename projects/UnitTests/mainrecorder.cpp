#include <vector>
#include <string>

#include <gst/gst.h>

int main(int argc, char **argv)
{
	gst_init(&argc, &argv);
	//gst-launch-1.0.exe --verbose videotestsrc num-buffers=100 horizontal-speed=1 ! video/x-raw,format=I420,width=48,height=32,framerate=10/1 !
	// x264enc aud=false bitrate=128 speed-preset=1 ! video/x-h264,stream-format=byte-stream ! multifilesink location=buffers/test%05d.264

	//gst-launch-1.0 --verbose videotestsrc num-buffers=100 horizontal-speed=1 ! video/x-raw,format=I420,width=48,height=32,framerate=10/1 ! avenc_h264_omx gop-size=3 ! video/x-h264,stream-format=byte-stream ! multifilesink location=buffers/test%05d.264

	std::vector<std::string> candidates{"x264enc"};//, "openh264enc", "avenc_h264_omx", "omxh264enc", "v4l2h264enc"};

	const std::string baseDirectory = ".";
	const std::string filenamePattern = "%05d.h264";

	GstElementFactory* factory = nullptr;
	for (const auto& candidate : candidates)
	{
		factory = gst_element_factory_find(candidate.c_str());
		if (factory == nullptr)
		{
			continue;
		}

		const std::string baseName = candidate;
		const std::string location = baseDirectory + '/' + baseName + filenamePattern;

		// Tests if a camera source can be capsfiltered
		auto pipeline = gst_pipeline_new("pipeline");
		GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN_CAST(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline.dot");

		auto bus = gst_element_get_bus(pipeline);
		auto source = gst_element_factory_make("videotestsrc", nullptr);
		g_object_set(source, "num-buffers", 100, "horizontal-speed", 1, nullptr);
		auto encoder = gst_element_factory_create(factory, nullptr);
		const int bitrate = 128;
		const int intraInt = 3;
		if (candidate == "x264enc")
		{
			g_object_set(encoder, "bitrate", bitrate, "key-int-max", intraInt, "insert-vui", false, "speed-preset", 1, "aud", false, "trellis", false, nullptr);
		}
		if (candidate == "openh264enc")
		{
			g_object_set(encoder, "bitrate", bitrate * 1000, "rate-control", 1, "gop-size", intraInt, nullptr);
		}
		if (candidate == "omxh264enc")
		{
			g_object_set(encoder, "target-bitrate", bitrate * 1000, "periodicty-idr", intraInt,  "interval-intraframes", intraInt, nullptr);
		}
		if (candidate == "avenc_h264_omx")
		{
			g_object_set(encoder, "bitrate", bitrate * 1000, "gop-size", intraInt, nullptr);
		}
		if (candidate == "v4l2h264enc")
		{
			//?
		}
		auto sink = gst_element_factory_make("multifilesink", nullptr);
		g_object_set(sink, "location", location.c_str(), nullptr);

		auto encoderSinkCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, 48, "height", G_TYPE_INT, 32,  nullptr);
		auto encoderSrcCaps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "byte-stream", "alignment", G_TYPE_STRING, "au", nullptr);


		gst_bin_add_many(GST_BIN(pipeline), source, encoder, sink, nullptr);
		gst_element_link_filtered(source, encoder, encoderSinkCaps);
		gst_element_link_filtered(encoder, sink, encoderSrcCaps);

		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
		gst_object_unref(pipeline);
	}
	return 0;
}
