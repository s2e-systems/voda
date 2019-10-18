#include <chrono>
#include <thread>
#include <future>
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
#include <QVBoxLayout>


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
};

TEST(GStreamerPipelineTest, DISABLED_Encoding)
{
	gst_init(nullptr, nullptr);
	auto pipeline = gst_pipeline_new("pipeline");
	auto srcfactory = gst_element_factory_find("v4l2src");
	if (srcfactory == nullptr)
	{
		srcfactory = gst_element_factory_find("ksvideosrc");
	}
	auto source = gst_element_factory_create(srcfactory, "videosource");
	auto sink = gst_element_factory_make("fakesink", "fakesink");
	gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
	gst_element_set_state(pipeline, GST_STATE_READY);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	auto pad = gst_element_get_static_pad(source, "src");

	auto videoscale = gst_element_factory_make("videoscale", "videoscale");
	auto videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
	auto openh264enc = gst_element_factory_make("openh264enc", "openh264enc");

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	gst_deinit();
}

TEST(GStreamerPipelineTest, DISABLED_SourceNegotiation)
{
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

	auto caps = gst_caps_new_simple("video/x-raw",
	//"format", G_TYPE_STRING, "YUY2",
	//"width", G_TYPE_INT, 640,
	//"height", G_TYPE_INT, 480,
	nullptr);
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
}

TEST(GStreamerPipelineTest, DISABLED_SourceCaps)
{
	// gst-launch-1.0 --gst-debug=*videosrc:5 ksvideosrc ! fakesink
	gst_init(nullptr, nullptr);
	auto source = gst_element_factory_make("autovideosrc", nullptr);
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
}


TEST(GStreamerPipelineTest, DISABLED_ImageJpeg)
{
	// gst-launch-1.0 --gst-debug=*videosrc:5 ksvideosrc ! fakesink
	gst_init(nullptr, nullptr);
	auto pipeline = gst_pipeline_new("pipeline");
	auto srcfactory = gst_element_factory_find("v4l2src");
	if (srcfactory == nullptr)
	{
		srcfactory = gst_element_factory_find("ksvideosrc");
	}
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

int main(int argc, char **argv)
{
	QApplication app{argc, argv};
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
