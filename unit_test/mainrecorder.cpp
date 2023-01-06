#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <gst/gst.h>
#include <glib.h>


static void print_messages_on_bus(GstBus* bus) {
	GError* err = nullptr;
	gchar* debug = nullptr;
	GstMessage* message = nullptr;
	do {
		message = gst_bus_pop_filtered(bus, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_INFO));
		if (message != nullptr){
			const std::string messageType{GST_MESSAGE_TYPE_NAME(message)};
			switch (GST_MESSAGE_TYPE(message)) {
				case GST_MESSAGE_ERROR:
					gst_message_parse_error(message, &err, &debug);
					break;
				case GST_MESSAGE_WARNING:
					gst_message_parse_warning(message, &err, &debug);
					break;
				case GST_MESSAGE_INFO:
					gst_message_parse_info(message, &err, &debug);
					break;
				default:
					break;
			}
			std::cout << "Got "<< messageType << " message";
			if (err != nullptr) {
				std::cout << ": " << err->message;
				g_error_free(err);
			}
			if (debug != nullptr) {
				std::cout << " (" << debug << ")";
				g_free(debug);
			}
			std::cout << "\n";
		}
	} while (message != nullptr);
}


int main(int argc, char **argv)
{
	if (g_setenv("GST_DEBUG_DUMP_DOT_DIR", ".", 1) == FALSE) {
		std::cout << "Error setenv" << std::endl;
		return -1;
	};
	gst_init(&argc, &argv);
	//gst-launch-1.0.exe --verbose videotestsrc num-buffers=10 pattern=smpte100 horizontal-speed=6 ! video/x-raw,format=I420,width=112,height=32,framerate=10/1 ! x264enc aud=false bitrate=128 speed-preset=1 ! video/x-h264,stream-format=byte-stream ! multifilesink location=buffers/test%05d.264
	//gst-launch-1.0 --verbose videotestsrc num-buffers=10 pattern=smpte100 horizontal-speed=6 ! video/x-raw,format=I420,width=112,height=32,framerate=10/1 ! avenc_h264_omx gop-size=3 ! video/x-h264,stream-format=byte-stream ! multifilesink location=buffers/test%05d.264
	//gst-launch-1.0 --verbose videotestsrc num-buffers=10 pattern=smpte100 horizontal-speed=6 ! video/x-raw,format=I420,width=112,height=32 ! v4l2h264enc extra-controls=c,video-bitrate=128000,video_bitrate_mode=1,h264_i_frame_period=3 ! video/x-h264,level=\(string\)4,profile=constrained-baseline ! multifilesink location=buffers/test%05d.264

	// Convert the dot to png can be done like so:
	// dot pipeline_graph.dot -Tpng -o pipeline_graph.png

	const std::vector<std::string> candidates{"x264enc", "openh264enc", "avenc_h264_omx", "omxh264enc", "v4l2h264enc"};

	// Width and height should be multiple of 16 for the encoder's natural macroblocks
	// Width should be multiple of 14 to avoid testsource issues (if smpte: 7 color bars) while producing I420
	const gint width = 112;
	const gint height = 32;
	const std::string result_directory = "./results";
	const std::string filenamePattern = "%05d.h264";
	std::ostringstream filenamePatternReference;
	filenamePatternReference << "%05d_RGBA_w" << width << "_h" << height << ".raw";
	const std::string graphFileBaseName = "pipeline_graph";
	const int number_of_frames = 10;
	const int horizontal_speed = 6; // Should me multiple of 2 to avoid I420 resampling issues
	const std::string pattern = "smpte100";
	const int intraInt = 3;
	const int bitrate = 128;

	const GstClockTime state_change_timeout_ns = number_of_frames * 100ull /*ms*/ * 1'000'000ull;
	const GstClockTime eos_timeout_ns = number_of_frames * 100ull /*ms*/ * 1'000'000ull;

	// Create reference images
	std::cout << "\n====================================\n";
	std::cout << "Create referece images\n";

	const std::string locationReference = result_directory + '/' + filenamePatternReference.str();

	const auto pipeline_r = gst_pipeline_new("pipeline");
	const auto bus = gst_element_get_bus(pipeline_r);
	const auto videotestsrc = gst_element_factory_make("videotestsrc", nullptr);
	g_object_set(videotestsrc,
		"num-buffers", number_of_frames,
		"horizontal-speed", horizontal_speed,
		nullptr
	);
	gst_util_set_object_arg(G_OBJECT(videotestsrc), "pattern", pattern.c_str());

	const auto converter = gst_element_factory_make("videoconvert", nullptr);
	const auto videosrc_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height,  nullptr);
	const auto multifilesink = gst_element_factory_make("multifilesink", nullptr);
	const auto multifilesink_reference_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", nullptr);
	g_object_set(multifilesink, "location", locationReference.c_str(), nullptr);
	gst_bin_add_many(GST_BIN(pipeline_r), videotestsrc, converter, multifilesink, nullptr);
	const auto videotestsrc_link_result = gst_element_link_filtered(videotestsrc, converter, videosrc_caps);
	if (videotestsrc_link_result == false) {
		std::cout << "Linking videotestsrc, converter pipeline failed.\n";
		return -1;
	}
	const auto converter_link_result = gst_element_link_filtered(converter, multifilesink, multifilesink_reference_caps);
	if (converter_link_result == false) {
		std::cout << "Linking converter, multifilesink pipeline failed.\n";
		return -1;
	}
	const auto mkdir_result = g_mkdir_with_parents(result_directory.c_str(), 0700);
	if (mkdir_result != 0 ) {
		std::cout << "Creating result directory \""<< result_directory << "\" failed.\n";
		return -1;
	}

	const auto set_state_playing_result = gst_element_set_state(pipeline_r, GST_STATE_PLAYING);
	if (set_state_playing_result == GST_STATE_CHANGE_FAILURE) {
		std::cout << "set_state to GST_STATE_PLAYING failed. Skipping.\n";
		print_messages_on_bus(bus);
		gst_object_unref(bus);
		return -1;
	};
	std::cout << "Saving frames to: \"" << locationReference << "\"\n";
	std::cout << "Waiting for finish (EOS)\n";
	const auto eos_return = gst_bus_timed_pop_filtered(bus, eos_timeout_ns, GST_MESSAGE_EOS);
	gst_object_unref(bus);
	if (eos_return == nullptr) {
		std::cout << "Timeout oocured while waiting for EOS after " << eos_timeout_ns/1'000'000 << "ms\n";
		gst_object_unref(pipeline_r);
		return -1;
	} else {
		std::cout << "Finished (got EOS)\n";
	}
	gst_object_unref(pipeline_r);

	// Loop over encoder candidates

	GstElementFactory* factory = nullptr;
	GstElement* pipeline = nullptr;

	for (const auto& candidate : candidates) {
		std::cout << "\n====================================\n";
		std::cout << "Probing  element: \""<< candidate << "\"\n";

		factory = gst_element_factory_find(candidate.c_str());
		if (factory == nullptr) {
			std::cout << "Element: \""<< candidate << "\" could not by found by gst_element_factory_find. Skipping.\n";
			continue;
		}

		const std::string resultDirectory = result_directory + '/' + candidate;
		const std::string location = resultDirectory + '/' + filenamePattern;
		const std::string graphFilePath = resultDirectory + '/' + graphFileBaseName;

		if (pipeline != nullptr) {
			gst_element_set_state(pipeline, GST_STATE_NULL);
			gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE /*infinite*/);
			gst_object_unref(pipeline);
		}
		pipeline = gst_pipeline_new("pipeline");
		const auto bus = gst_element_get_bus(pipeline);
		const auto videotestsrc = gst_element_factory_make("videotestsrc", nullptr);
		g_object_set(videotestsrc,
			"num-buffers", number_of_frames,
			"horizontal-speed", horizontal_speed,
			nullptr
		);
		gst_util_set_object_arg(G_OBJECT(videotestsrc), "pattern", pattern.c_str());
		auto encoder = gst_element_factory_create(factory, nullptr);
		const auto videosrc_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height,  nullptr);
		const auto encoderSrcCaps = gst_caps_new_simple("video/x-h264",
			"stream-format", G_TYPE_STRING, "byte-stream",
			"alignment", G_TYPE_STRING, "au",
			nullptr);
		if (candidate == "x264enc") {
			g_object_set(encoder, "bitrate", bitrate, "key-int-max", intraInt, "insert-vui", false, "speed-preset", 1, "aud", false, "trellis", false, nullptr);
		}
		if (candidate == "openh264enc") {
			g_object_set(encoder, "bitrate", bitrate * 1000, "rate-control", 1, "gop-size", intraInt, nullptr);
		}
		if (candidate == "omxh264enc") {
			g_object_set(encoder, "target-bitrate", bitrate * 1000, "periodicty-idr", intraInt,  "interval-intraframes", intraInt, nullptr);
		}
		if (candidate == "avenc_h264_omx") {
			g_object_set(encoder, "bitrate", bitrate * 1000, "gop-size", intraInt, nullptr);
		}
		if (candidate == "v4l2h264enc")
		{
 			const auto extra = gst_structure_new("recorder", "video_bitrate", G_TYPE_INT, bitrate * 1000, "h264_i_frame_period", G_TYPE_INT, intraInt, nullptr);
			g_object_set(encoder, "extra-controls", extra, nullptr);
			gst_caps_set_simple(encoderSrcCaps,
				"profile", G_TYPE_STRING, "constrained-baseline",
				"level", G_TYPE_STRING, "4",
				nullptr);
		}
		const auto sink = gst_element_factory_make("multifilesink", nullptr);
		g_object_set(sink, "location", location.c_str(), nullptr);



		gst_bin_add_many(GST_BIN(pipeline), videotestsrc, encoder, sink, nullptr);

		const auto source_tee_link_result = gst_element_link_filtered(videotestsrc, encoder, videosrc_caps);
		if (source_tee_link_result == false) {
			std::cout << "Linking source and encoder failed. Skipping.\n";
			continue;
		}

		const auto encoder_sink_link_result = gst_element_link_filtered(encoder, sink, encoderSrcCaps);
		if (encoder_sink_link_result == false) {
			std::cout << "Linking encoder sink failed. Skipping.\n";
			continue;
		}

		const auto set_state_result = gst_element_set_state(pipeline, GST_STATE_PAUSED);
		if (set_state_result == GST_STATE_CHANGE_FAILURE) {
			std::cout << "set_state to GST_STATE_PAUSED failed. Skipping.\n";
			print_messages_on_bus(bus);
			continue;
		};
		const auto get_state_result = gst_element_get_state(pipeline, nullptr, nullptr, state_change_timeout_ns);
		if (get_state_result != GST_STATE_CHANGE_SUCCESS) {
			std::cout << "Waiting for GST_STATE_PAUSED timed out after " << state_change_timeout_ns/1'000'000 << "ms. Skipping.\n";
			print_messages_on_bus(bus);
			continue;
		}

		const auto mkdir_result = g_mkdir_with_parents(resultDirectory.c_str(), 0700);
		if (mkdir_result != 0 ) {
			std::cout << "Creating result directory \""<< resultDirectory << "\" failed. Skipping.\n";
			continue;
		}

		const auto set_state_playing_result = gst_element_set_state(pipeline, GST_STATE_PLAYING);
		if (set_state_playing_result == GST_STATE_CHANGE_FAILURE) {
			std::cout << "set_state to GST_STATE_PLAYING failed. Skipping.\n";
			print_messages_on_bus(bus);
			continue;
		};
		const auto get_state_playing_result = gst_element_get_state(pipeline, nullptr, nullptr, state_change_timeout_ns);
		if (get_state_playing_result != GST_STATE_CHANGE_SUCCESS) {
			std::cout << "Waiting for GST_STATE_PLAYING timed out after " << state_change_timeout_ns/1'000'000 << "ms. Skipping.\n";
			print_messages_on_bus(bus);
			continue;
		}

		std::cout << "Saving frames to: \"" << location << "\"\n";
		std::cout << "Saving pipeline graph to: \"" << graphFilePath << ".dot\"\n";
		std::cout << "Waiting for finish (EOS)\n";
		const auto eos_return = gst_bus_timed_pop_filtered(bus, eos_timeout_ns, GST_MESSAGE_EOS);
		if (eos_return == nullptr) {
			std::cout << "Timeout occurred while waiting for EOS after " << eos_timeout_ns/1'000'000 << "ms\n";
		} else {
			std::cout << "Finished (got EOS)\n";
		}

		GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN_CAST(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, graphFilePath.c_str());
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE /*infinite*/);
		std::cout << "pipeline stopped.\n";
		print_messages_on_bus(bus);
	}
	return 0;
}

