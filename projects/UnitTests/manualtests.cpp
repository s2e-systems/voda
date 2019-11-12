#include <chrono>
#include <thread>
#include <future>
#include <cmath>
#include <algorithm>

#include <gtest/gtest.h>

#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/gstcapsfeatures.h>
#include <gst/gstbus.h>

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

TEST(ElementsTest, DISABLED_GhostPadSink)
{
	gst_init(nullptr, nullptr);

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
	gst_deinit();
}

TEST(ElementsTest, DISABLED_GhostPadSrc)
{
	gst_init(nullptr, nullptr);

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
	gst_deinit();
}

TEST(ElementsTest, DISABLED_Source)
{
	gst_init(nullptr, nullptr);
	auto pipeline = gst_pipeline_new("pipeline");

	auto bus = gst_element_get_bus(pipeline);
	gst_bus_set_sync_handler(bus, &Pipeline::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	Source s;
	auto sourceBin = s.bin();
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	gst_bin_add_many(GST_BIN_CAST(pipeline), GST_ELEMENT_CAST(sourceBin), sink, nullptr);
	gst_element_link(GST_ELEMENT_CAST(sourceBin), sink);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);
	gst_deinit();
}

TEST(ElementsTest, SourceAndEncoderAndDecoder)
{
	// gst-launch-1.0.exe ksvideosrc ! videoconvert ! x264enc bitrate=128 intra-refresh=true byte-stream=false vbv-buf-capacity=2000 key-int-max=10 threads=1 sliced-threads=false aud=false tune=zerolatency ! video/x-h264,stream-format=avc ! h264parse config-interval=1 ! h264parse ! video/x-h264,stream-format=avc ! avdec_h264 ! glimagesink sync=false
	gst_init(nullptr, nullptr);
	auto pipeline = gst_pipeline_new("pipeline");

	auto bus = gst_element_get_bus(pipeline);
	//gst_bus_set_sync_handler(bus, &Pipeline::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	Source s;
	Encoder e;
	Decoder d;
	auto sourceBin = s.bin();
	auto encoderBin = e.bin();
	auto decoderBin = d.bin();
	auto source = gst_bin_get_by_name(sourceBin, "videosource");
	g_object_set(source, "num-buffers", 50, nullptr);
	auto converter = gst_element_factory_make("videoconvert", nullptr);
	auto sink = gst_element_factory_make("glimagesink", nullptr);
	g_object_set(sink, "sync", false, nullptr);
	gst_bin_add_many(GST_BIN_CAST(pipeline), /*testsource, */GST_ELEMENT_CAST(sourceBin), converter, GST_ELEMENT_CAST(encoderBin), GST_ELEMENT_CAST(decoderBin), sink, /*fakesink, */nullptr);

	gst_element_link(GST_ELEMENT_CAST(sourceBin), converter);
	gst_element_link(converter, GST_ELEMENT_CAST(encoderBin));
	gst_element_link(GST_ELEMENT_CAST(encoderBin), GST_ELEMENT_CAST(decoderBin));
	gst_element_link(GST_ELEMENT_CAST(decoderBin), sink);

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

	gst_init(nullptr, nullptr);
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
	gst_deinit();


	ManualTestFeedback{"Did a running video appeared on screen?"};
}


TEST(PainterWidgetTest, DISABLED_TestSourceAppsink)
{
	// Test if the videowidget displays data

	gst_init(nullptr, nullptr);
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
	gst_deinit();

	ManualTestFeedback{"Did a running video appeared on screen?"};
}

TEST(GStreamerPipelineTest, DISABLED_Encoding)
{
	// Tests if h264 encoding is possible

	gst_init(nullptr, nullptr);
	auto pipeline = gst_pipeline_new("pipeline");
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	g_object_set(source, "num-buffers", 5, nullptr);
	auto sink = gst_element_factory_make("appsink", nullptr);

	auto encoderFactory = gst_element_factory_find("openh264enc");
	if (encoderFactory == nullptr)
	{
		encoderFactory = gst_element_factory_find("x264enc");
	}
	if (encoderFactory == nullptr)
	{
		encoderFactory = gst_element_factory_find("avenc_h264_omx");
	}
	auto encoder = gst_element_factory_create(encoderFactory, nullptr);

	gst_bin_add_many(GST_BIN(pipeline), source, encoder, sink, nullptr);
	gst_element_link_many(source, encoder, sink, nullptr);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	const GstClockTime timeOutNanoSeconds = 1000000000;
	for (int i = 0; i < 20; ++i)
	{
		auto sample = gst_app_sink_try_pull_sample(GST_APP_SINK_CAST(sink), timeOutNanoSeconds);
		if(sample == nullptr)
		{
			continue;
			//FAIL() << "No sample in sink";
		}
		auto sampleBuffer = gst_sample_get_buffer(sample);
		if(sampleBuffer == nullptr)
		{
			FAIL() << "No buffer in sample of sink";
		}

		GstMapInfo mapInfo;
		gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);
		auto data = reinterpret_cast<const char *>(mapInfo.data);
		const auto r = std::min(static_cast<int>(mapInfo.size), 128);
		std::string str(data, r);
		std::cout << str << std::endl;

		gst_buffer_unmap(sampleBuffer, &mapInfo);
		gst_sample_unref(sample);
	}

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	gst_object_unref(pipeline);
	gst_deinit();


	ManualTestFeedback{"Did H264 like packets arrive?"};
}

TEST(GStreamerPipelineTest, DISABLED_SourceNegotiation)
{
	// Tests if a camera source can be capsfiltered

	gst_init(nullptr, nullptr);
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
	gst_deinit();

	ManualTestFeedback{"Are appropriate caps displayed?"};
}

TEST(GStreamerPipelineTest, DISABLED_SourceCaps)
{
	// Tests if caps from a video source can be determined

	// gst-launch-1.0 --gst-debug=*videosrc:5 ksvideosrc ! fakesink
	gst_init(nullptr, nullptr);
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
	gst_deinit();

	ManualTestFeedback{"Are appropriate caps displayed?"};
}


TEST(GStreamerPipelineTest, DISABLED_ImageJpeg)
{
	// Tests if a camera source can deliver jpeg and
	// that it can be decoded and diplayed

	// gst-launch-1.0 --gst-debug=*videosrc:5 ksvideosrc ! fakesink
	gst_init(nullptr, nullptr);
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
	gst_deinit();

	ManualTestFeedback{"Did a running video appeared on screen?"};
}

