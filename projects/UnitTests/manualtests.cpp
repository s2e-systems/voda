#include <chrono>
#include <thread>
#include <future>
#include <cmath>
#include <algorithm>
#include <vector>
#include <map>

#include <gtest/gtest.h>

#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/gstcapsfeatures.h>
#include <gst/gstbus.h>
#include <gst/codecparsers/gsth264parser.h>

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include "videowidgetpaintergst.h"
#include "elements.h"
#include "pipeline.h"
#include "cameracapabilities.h"

#include <bitset>

/**
 * @brief The ManualTestFeedback struct
 * Note that this class can be used without QApplication main eventloop:
 * "As a special case, modal widgets like QMessageBox can be used before calling exec(),
 * because modal widgets call exec() to start a local event loop."
 */
struct ManualTestFeedback : QDialog
{
	ManualTestFeedback(const QString& text)
	{
		setWindowTitle("TestWidgets");
		auto buttonBox{new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)};
		buttonBox->button(QDialogButtonBox::Ok)->setText("Yes: Pass");
		buttonBox->button(QDialogButtonBox::Cancel)->setText("No: Fail");
		auto layout{new QVBoxLayout};
		layout->addWidget(new QLabel(text));
		layout->addWidget(buttonBox);
		setLayout(layout);
		QObject::connect(buttonBox, &QDialogButtonBox::rejected, &loop, &QEventLoop::quit);
		QObject::connect(buttonBox, &QDialogButtonBox::accepted, &loop, &QEventLoop::quit);
		QObject::connect(buttonBox, &QDialogButtonBox::rejected, [](){
			GTEST_FAIL() << "Human tester rejected";
		});
		show();
		loop.exec();
	}
	QEventLoop loop;
	void open() override;
};
void ManualTestFeedback::open() {QDialog::open();};


GstBusSyncReply quitQAppOnGstMessage(GstBus* /*bus*/, GstMessage* msg, gpointer /*data*/)
{
	if (msg->type == GST_MESSAGE_EOS || msg->type == GST_MESSAGE_ERROR)
	{
		QMetaObject::invokeMethod(qApp, &QApplication::quit);
	}
	return GST_BUS_DROP;
}

template < class T >
std::ostream& operator << (std::ostream& os, const std::vector<T>& v)
{
	for (const auto& vi : v)
	{
		os << " " << vi;
	}
	return os;
}

TEST(ElementsTest, DISABLED_GhostPadSink)
{
	auto pipeline = gst_pipeline_new(nullptr);
	auto bus = gst_element_get_bus(pipeline);
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	auto bin = gst_bin_new(nullptr);
	gst_bin_add(GST_BIN(bin), sink);
	auto pad = gst_element_get_static_pad(sink, "sink");
	gst_element_add_pad(bin, gst_ghost_pad_new(nullptr, pad));
	gst_object_unref(GST_OBJECT(pad));
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	g_object_set(source, "num-buffers", 50, nullptr);
	gst_bin_add_many(GST_BIN(pipeline), source, bin, nullptr);
	gst_element_link(source, bin);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
}

TEST(ElementsTest, DISABLED_GhostPadSrc)
{
	auto pipeline = gst_pipeline_new(nullptr);
	auto bus = gst_element_get_bus(pipeline);
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	auto bin = gst_bin_new(nullptr);
	gst_bin_add(GST_BIN(bin), source);
	auto pad = gst_element_get_static_pad(source, "src");
	gst_element_add_pad(bin, gst_ghost_pad_new(nullptr, pad));
	gst_object_unref(GST_OBJECT(pad));
	g_object_set(source, "num-buffers", 50, nullptr);
	gst_bin_add_many(GST_BIN(pipeline), bin, sink, nullptr);
	gst_element_link(bin, sink);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
}

TEST(ElementsTest, DISABLED_Source)
{
	auto pipeline = gst_pipeline_new("pipeline");

	auto bus = gst_element_get_bus(pipeline);
	gst_bus_set_sync_handler(bus, &Pipeline::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);
	ElementSelection selection{{"videotestsrc", "v4l2src", "ksvideosrc"}, "videosource"};
	Source source{selection.element(), "sourcebin"};
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	voda::add(GST_BIN_CAST(pipeline), {source, sink});
	voda::link({source, sink});
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);
}

TEST(EncoderTest, DISABLED_Performance)
{
	auto pipeline = gst_pipeline_new("pipeline");
	auto bus = gst_element_get_bus(pipeline);
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	g_object_set(source, "num-buffers", 500, nullptr);
	auto sink = gst_element_factory_make("fakesink", nullptr);
	g_object_set(sink, "sync", false, nullptr);

	auto encoder = gst_element_factory_make("x264enc", nullptr);
	g_object_set(encoder,
		"bitrate", 1000, // Bitrate in kbit/sec
		"intra-refresh", false, // Use Periodic Intra Refresh instead of IDR frames
		"byte-stream", false, //Generate byte stream format of NALU
		"vbv-buf-capacity", 2000, //Size of the VBV buffer in milliseconds
		"key-int-max", 10, //Maximal distance between two key-frames (0 for automatic)
		"threads", 5, //Number of threads used by the codec (0 for automatic)
		"sliced-threads", false, //Low latency but lower efficiency threading
		"aud", false, //Use AU (Access Unit) delimiter
		"speed-preset", 1,
	nullptr);
	gst_util_set_object_arg(G_OBJECT(source), "tune", "zerolatency");

//	auto encoder = gst_element_factory_make("openh264enc", nullptr);
//	g_object_set(encoder,
//		"bitrate", 1000, // Bitrate in kbit/sec
//		"gop-size", 10, // Number of frames between intra frames
//		"multi-thread", 5, //The number of threads.
//		"rate-control", 1, //Rate control mode: (0): quality (1): bitrate  (2): buffer (No bitrate control, just using buffer status) (-1): off
//	nullptr);

	gst_bin_add_many(GST_BIN(pipeline), source, encoder, sink, nullptr);
	gst_element_link_many(source, encoder, sink, nullptr);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	const auto start = std::chrono::system_clock::now();
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
	const auto end = std::chrono::system_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	FAIL() << "Elapsed: " << elapsed.count();
}


TEST(EncoderTest, ByteStream)
{
	constexpr std::size_t numBuffers = 10;
	constexpr int maxDisplaySize = 128;
	std::vector<bool> settings{true, false};

	std::vector<std::map<bool, std::vector<unsigned char>>> output{numBuffers};

	for (const auto setting : settings)
	{
		auto pipeline = gst_pipeline_new("pipeline");
		auto bus = gst_element_get_bus(pipeline);

		auto source = gst_element_factory_make("videotestsrc", nullptr);
		g_object_set(source, "num-buffers", numBuffers, nullptr);
		auto sink = gst_element_factory_make("appsink", nullptr);
		auto caps = gst_caps_new_simple("video/x-h264", nullptr);
		g_object_set(sink, "sync", false, "caps", caps, "wait-on-eos", false, nullptr);
		auto encoder = gst_element_factory_make("x264enc", nullptr);
		gst_bin_add_many(GST_BIN(pipeline), source, encoder, sink, nullptr);
		 gst_element_link_many(source, encoder, sink, nullptr);

		const gboolean bytestream = setting;
		g_object_set(encoder,
			"bitrate", 30, // Bitrate in kbit/sec
			"intra-refresh", false, // Use Periodic Intra Refresh instead of IDR frames
			"byte-stream", bytestream, //Generate byte stream format of NALU
			"key-int-max", 20, //Maximal distance between two key-frames (0 for automatic)
			"threads", 1, //Number of threads used by the codec (0 for automatic)
			"sliced-threads", false, //Low latency but lower efficiency threading
			"aud", false, //Use AU (Access Unit) delimiter
			"vbv-buf-capacity", 10, //Size of the VBV buffer in milliseconds
			"speed-preset", 9,
			"insert-vui", false,
			"sps-id", 8,
		nullptr);
		gst_util_set_object_arg(G_OBJECT(encoder), "tune", "zerolatency");

		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
		gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
		std::size_t n = 0;
		for(;;)
		{
			auto sample = gst_app_sink_pull_sample(GST_APP_SINK_CAST(sink));
			if(sample == nullptr)
			{
				break;
			}
			auto sampleBuffer = gst_sample_get_buffer(sample);
			if(sampleBuffer != nullptr)
			{
				GstMapInfo mapInfo;
				gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);
				const auto size = static_cast<int>(mapInfo.size);

				const std::vector<unsigned char> b{mapInfo.data, mapInfo.data+size};
				output[n].emplace(setting, b);
				n = n + 1;
				gst_buffer_unmap(sampleBuffer, &mapInfo);
			}
			gst_sample_unref(sample);
		}

		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

		gst_object_unref(bus);
		gst_object_unref(pipeline);
	}
	int n = 0;
	for (const auto& v : output)
	{
		EXPECT_EQ(v.at(true), v.at(false)) << "NALUS must be equal till the very last bit.";
		n++;
		for (const auto setting : settings)
		{
			const auto vi = v.at(setting);
			const auto displaySize = std::min(int(vi.size()), maxDisplaySize);
			const std::vector<int> vs(vi.cbegin(), vi.cbegin() + displaySize);
			std::cout << std::setw(5) << std::boolalpha << setting << ":" << std::dec << std::setw(2) << n << " " << std::setw(4) << vi.size() << "B: " << std::hex << vs << std::endl;
		}

	}
}

TEST(ElementsTest, DISABLED_TestSourceJpeg)
{
	auto pipeline = gst_pipeline_new("pipeline");

	auto bus = gst_element_get_bus(pipeline);
	gst_bus_set_sync_handler(bus, &Pipeline::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	TestSourceJpeg source{"sourcebin"};
	auto pad = gst_element_get_static_pad(source, "src");
	auto caps = gst_pad_query_caps(pad, nullptr);
	CapabilitySelection s{caps};
	auto jpegCaps = s.highestJpegPixelRate();
	CapsFilter f{jpegCaps};

	auto sourceElement = gst_bin_get_by_name(source, "videosource");
	g_object_set(sourceElement, "num-buffers", 50, nullptr);
	auto decoder = gst_element_factory_make("jpegdec", nullptr);
	auto sink = gst_element_factory_make("glimagesink", nullptr);

	voda::add(GST_BIN_CAST(pipeline), {source, f, decoder, sink});
	voda::link({source, f, decoder, sink});

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	gst_object_unref(pipeline);
}

TEST(ElementsTest, DISABLED_Encoder)
{
	// gst-launch-1.0.exe ksvideosrc ! videoconvert ! x264enc bitrate=128 intra-refresh=true byte-stream=false vbv-buf-capacity=2000 key-int-max=10 threads=1 sliced-threads=false aud=false tune=zerolatency ! video/x-h264,stream-format=avc ! h264parse config-interval=1 ! h264parse ! video/x-h264,stream-format=avc ! avdec_h264 ! glimagesink sync=false
	auto pipeline = gst_pipeline_new("pipeline");

	auto bus = gst_element_get_bus(pipeline);
//	gst_bus_set_sync_handler(bus, &Pipeline::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	ElementSelection selection{{"videotestsrc", "v4l2src", "ksvideosrc"}, "videosource"};
	Source source{selection.element(), "sourcebin"};
	Encoder element{"encoderbin"};
	Decoder decoder{"decoderbin"};
	auto sourceElement = gst_bin_get_by_name(source, "videosource");
	g_object_set(sourceElement, "num-buffers", 50, nullptr);
	auto converter = gst_element_factory_make("videoconvert", nullptr);
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	g_object_set(sink, "sync", false, nullptr);

	voda::add(GST_BIN_CAST(pipeline), {source, converter, element,  decoder, sink});
	voda::link({source, converter, element,  decoder, sink});

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);
}

TEST(ElementsTest, DISABLED_SourceAndEncoderAndDecoder)
{
	// gst-launch-1.0.exe ksvideosrc ! videoconvert ! x264enc bitrate=128 intra-refresh=true byte-stream=false vbv-buf-capacity=2000 key-int-max=10 threads=1 sliced-threads=false aud=false tune=zerolatency ! video/x-h264,stream-format=avc ! h264parse config-interval=1 ! h264parse ! video/x-h264,stream-format=avc ! avdec_h264 ! glimagesink sync=false
	gst_init(nullptr, nullptr);
	auto pipeline = gst_pipeline_new("pipeline");

	auto bus = gst_element_get_bus(pipeline);
	//gst_bus_set_sync_handler(bus, &Pipeline::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	ElementSelection selection{{"videotestsrc", "v4l2src", "ksvideosrc"}, "videosource"};
	Source source{selection.element(), "sourcebin"};
	Encoder encoder{"encoderbin"};
	Decoder decoder{"decoderbin"};
	auto sourceElement = gst_bin_get_by_name(source, "videosource");
	g_object_set(sourceElement, "num-buffers", 50, nullptr);
	auto converter = gst_element_factory_make("videoconvert", nullptr);
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	g_object_set(sink, "sync", false, nullptr);

	voda::add(GST_BIN_CAST(pipeline), {source, converter, encoder,  decoder, sink});
	voda::link({source, converter, encoder,  decoder, sink});

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);
	gst_deinit();
}


TEST(PainterWidgetTest, DISABLED_TestSourceGlimagesink)
{
	// This test is to check if a pure gstreamer video pipeline runs.
	// Pure means without own classes and without external dependencies
	// other then gstreamer itsself

	auto pipeline = gst_pipeline_new("pipeline");
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	g_object_set(source, "num-buffers", 100, "horizontal-speed", 10, nullptr);
	auto capsfilter = gst_element_factory_make("capsfilter", nullptr);
	auto caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, "framerate", GST_TYPE_FRACTION, 10, 1, nullptr);
	g_object_set(capsfilter, "caps", caps, NULL);

	gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, sink, nullptr);
	gst_element_link_many(source, capsfilter, sink, nullptr);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	auto bus = gst_element_get_bus(pipeline);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);

	ManualTestFeedback{"Did a running video appeared on screen?"};
}


TEST(PainterWidgetTest, DISABLED_TestSourceAppsink)
{
	// Test if the videowidget displays data

	auto pipeline = gst_pipeline_new("pipeline");
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	auto sink = gst_element_factory_make("appsink", nullptr);
	g_object_set(source, "num-buffers", 50, "horizontal-speed", 10, nullptr);
	auto caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, "framerate", GST_TYPE_FRACTION, 10, 1, nullptr);
	g_object_set(sink, "caps", caps, NULL);

	VideoWidgetPainterGst widget;
	widget.installAppSink(GST_APP_SINK_CAST(sink));
	widget.show();

	gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
	gst_element_link_many(source, sink, nullptr);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	auto bus = gst_element_get_bus(pipeline);
	gst_bus_set_sync_handler(bus, quitQAppOnGstMessage /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	qApp->exec();

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	gst_object_unref(pipeline);
	gst_object_unref(bus);

	ManualTestFeedback{"Did a running video appeared on screen?"};
}


TEST(GStreamerPipelineTest, DISABLED_SourceNegotiation)
{
	// Tests if a camera source can be capsfiltered
	auto pipeline = gst_pipeline_new("pipeline");
	auto srcfactory = gst_element_factory_find("v4l2src");
	if (srcfactory == nullptr)
	{
		srcfactory = gst_element_factory_find("ksvideosrc");
	}
	auto source = gst_element_factory_create(srcfactory, "videosource");
	auto capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
	auto sink = gst_element_factory_make("fakesink", "fakesink");

	auto caps = gst_caps_new_simple("video/x-raw",  nullptr);
	g_object_set(capsfilter, "caps", caps, NULL);

	gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, sink, nullptr);
	gst_element_link_many(source, capsfilter, sink, nullptr);

	gst_element_set_state(pipeline, GST_STATE_READY);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	auto pad = gst_element_get_static_pad(source, "src");
	auto currentCaps = gst_pad_get_allowed_caps(pad);

	g_print("Caps: %s\n", gst_caps_to_string(currentCaps));
	gst_caps_unref(currentCaps);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);

	ManualTestFeedback{"Are appropriate caps displayed?"};
}

TEST(GStreamerPipelineTest, DISABLED_SourceCaps)
{
	// Tests if caps from a video source can be determined

	// gst-launch-1.0 --gst-debug=*videosrc:5 ksvideosrc ! fakesink
	auto source = gst_element_factory_make("autovideosrc", nullptr);
	if (source == nullptr)
	{
		gst_deinit();
		FAIL() << "No auto video source found";
	}
	gst_element_set_state(source, GST_STATE_READY);
	gst_element_get_state(source, nullptr/*state*/, nullptr/*pending*/, GST_CLOCK_TIME_NONE);
	auto pad = gst_element_get_static_pad(source, "src");
	auto caps = gst_pad_query_caps(pad, nullptr);
	auto nCaps = gst_caps_get_size(caps);

	for(unsigned int n = 0; n < nCaps; ++n)
	{
		auto capn = gst_caps_get_structure(caps, n);
		int width;
		int height;
		auto name = gst_structure_get_name(capn);
		gst_structure_get_int(capn, "width", &width);
		gst_structure_get_int(capn, "height", &height);
		const auto format = gst_structure_get_string(capn, "format");
		int framerateNominator;
		int framerateDenominator;
		gst_structure_get_fraction(capn, "framerate", &framerateNominator, &framerateDenominator);
		const auto framerate = static_cast<double>(framerateNominator)/static_cast<double>(framerateDenominator);
		const auto pixels = std::round(static_cast<double>(width * height) / 1000.0);
		const auto pixelrate = pixels * framerate;
		g_print("Caps %d \"%s\": %s, width=%d, height=%d, framerate=%.0f fps, pixels=%.0f kPx, pixelrate=%.0f kPxps", n, name, format, width, height, framerate, pixels, pixelrate);

		g_print("\n");
	}
	gst_element_set_state(source, GST_STATE_NULL);
	gst_element_get_state(source, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	ManualTestFeedback{"Are appropriate caps displayed?"};
}


TEST(GStreamerPipelineTest, DISABLED_ImageJpeg)
{
	// Tests if a camera source can deliver jpeg and
	// that it can be decoded and diplayed

	// gst-launch-1.0 --gst-debug=*videosrc:5 ksvideosrc ! fakesink
	auto srcfactory = gst_element_factory_find("v4l2src");
	if (srcfactory == nullptr)
	{
		srcfactory = gst_element_factory_find("ksvideosrc");
	}
	if (srcfactory == nullptr)
	{
		gst_deinit();
		FAIL() << "No camera source could be found";
	}
	auto pipeline = gst_pipeline_new("pipeline");
	auto source = gst_element_factory_create(srcfactory, "source");

	// Note: autovideosrc does not negotiate with image/jpeg caps
	auto decoder = gst_element_factory_make("jpegdec", nullptr);
	auto sink = gst_element_factory_make("glimagesink", nullptr);

	g_object_set(source, "num-buffers", 60, nullptr);
	g_object_set(sink, "sync", false, nullptr);

	gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, nullptr);
	gst_element_link_many(source, decoder, sink, nullptr);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	auto bus = gst_element_get_bus(pipeline);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);

	ManualTestFeedback{"Did a running video appeared on screen?"};
}

