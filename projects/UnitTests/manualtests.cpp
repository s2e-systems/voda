#include <chrono>
#include <thread>
#include <future>
#include <cmath>
#include <algorithm>
#include <vector>
#include <map>
#include <bitset>

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

#include "config.h"
#include "videowidgetpaintergst.h"
#include "elements.h"
#include "cameracapabilities.h"
#include "qtgstreamer.h"

static const auto dataDirectory = VODA_UNITTEST_DATA_DIRECTORY;

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


TEST(PainterWidgetTest, TestSourceAppsink)
{
	// Test if the videowidget displays data

	auto pipeline = gst_pipeline_new("pipeline");
	auto source = gst_element_factory_make("videotestsrc", nullptr);
	g_object_set(source, "num-buffers", 50, "horizontal-speed", 10, nullptr);

	auto sink = gst_element_factory_make("appsink", nullptr);
	auto caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, "framerate", GST_TYPE_FRACTION, 10, 1, nullptr);
	g_object_set(sink, "caps", caps, "wait-on-eos", false, nullptr);

	VideoWidgetPainterGst widget(GST_APP_SINK_CAST(sink));
	widget.show();

	gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
	gst_element_link(source, sink);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	ManualTestFeedback{"TestSourceAppsink: Did a running video appeared on screen?"};

	gst_object_unref(pipeline);
}



TEST(EncoderRecorder, ReplayData)
{
	const int startIndex = 0;

	const std::vector<std::string> platforms{"ubuntu_gst1_15_9", "windows_gst1_16_0"};
	std::map<std::string, std::vector<std::string>> fileBaseNames;
	fileBaseNames["ubuntu_gst1_15_9"] = {"x264enc"};
	fileBaseNames["windows_gst1_16_0"] = {"x264enc"};

	const std::vector<std::string> decoderNames{"avdec_h264", "omxh264dec", "openh264dec"};
	const std::string decoderName = decoderNames.at(0);

	const std::string platform = platforms.at(1);
	const std::string baseName = fileBaseNames[platform].at(0);

	const std::string baseDirectory = dataDirectory;
	const std::string filenamePattern = "%05d.h264";

	const std::string location = baseDirectory + '/' + platform + '/' + baseName + filenamePattern;

	GstElementFactory* factory = nullptr;

	factory = gst_element_factory_find(decoderName.c_str());
	if (factory == nullptr)
	{
		FAIL() << "Decoder" << decoderName << "not available";
	}

	auto pipeline = gst_pipeline_new("pipeline");
	auto bus = gst_element_get_bus(pipeline);
	gst_bus_set_sync_handler(bus, &QtGStreamer::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	auto source = gst_element_factory_make("multifilesrc", nullptr);

	auto sourceCaps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "byte-stream", "alignment", G_TYPE_STRING, "au", "framerate", GST_TYPE_FRACTION, 1, 1, nullptr);
	g_object_set(source, "location", location.c_str(), "start-index", startIndex, "caps", sourceCaps, nullptr);

	auto decoder = gst_element_factory_create(factory, nullptr);

	auto sink = gst_element_factory_make("autovideosink", nullptr);
	g_object_set(sink, "sync", true, nullptr);

	gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, nullptr);
	gst_element_link(source, decoder);
	gst_element_link(decoder, sink);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	ManualTestFeedback{"ReplayData: Did a litlle running video appeared on screen?"};

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);
}



TEST(EncoderRecorder, ReplayOmxData)
{
	// Omx encoded data requires an additional parser, hence the extra test here

	const int startIndex = 0;
	const std::vector<std::string> decoderNames{"avdec_h264", "omxh264dec", "openh264dec"};
	const std::string decoderName = decoderNames.at(0);

	const std::vector<std::string> platforms{"raspbian_gst1_14_2"};
	std::map<std::string, std::vector<std::string>> fileBaseNames;
	fileBaseNames["raspbian_gst1_14_2"] = {"avenc_h264_omx"};

	const std::string platform = platforms.at(0);
	const std::string baseName = fileBaseNames[platform].at(0);

	const std::string baseDirectory = dataDirectory;
	const std::string filenamePattern = "%05d.h264";

	const std::string location = baseDirectory + '/' + platform + '/' + baseName + filenamePattern;

	auto factory = gst_element_factory_find(decoderName.c_str());
	if (factory == nullptr)
	{
		FAIL() << "Decoder" << decoderName << "not available";
	}

	auto pipeline = gst_pipeline_new("pipeline");
	auto bus = gst_element_get_bus(pipeline);
	gst_bus_set_sync_handler(bus, &QtGStreamer::busCallBack /*function*/, static_cast<gpointer>(this), nullptr /*notify function*/);

	auto source = gst_element_factory_make("multifilesrc", nullptr);

	auto sourceCaps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "byte-stream", "alignment", G_TYPE_STRING, "au", "framerate", GST_TYPE_FRACTION, 1, 1, nullptr);
	g_object_set(source, "location", location.c_str(), "start-index", startIndex, "caps", sourceCaps, nullptr);

	auto decoder = gst_element_factory_create(factory, nullptr);
	auto parser = gst_element_factory_make("h264parse", nullptr);
	g_object_set(parser, "config-interval", -1, "disable-passthrough", true, nullptr);

	auto sink = gst_element_factory_make("autovideosink", nullptr);
	g_object_set(sink, "sync", true, nullptr);

	gst_bin_add_many(GST_BIN(pipeline), source, parser, decoder, sink, nullptr);
	gst_element_link(source, parser);
	gst_element_link(parser, decoder);
	gst_element_link(decoder, sink);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	ManualTestFeedback{"ReplayOmxData: Did a litlle running video appeared on screen?"};

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
	gst_object_unref(pipeline);
}


