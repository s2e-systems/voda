#include <string>
#include <ostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <stdio.h>
#include <gtest/gtest.h>

#include "videoddssubscriber.h"
#include "config.h"

class VideoDDSsubscriberTestFixture : public ::testing::TestWithParam<std::pair<std::string, std::string>>
{
};

INSTANTIATE_TEST_SUITE_P(
	VideoDDSsubscriberTests,
	VideoDDSsubscriberTestFixture,
	::testing::Values(
		std::make_pair("raspbian_gst1_18", "v4l2h264enc"),
		std::make_pair("raspbian_gst1_18", "x264enc"),
		std::make_pair("windows_gst1_18", "openh264enc"),
		std::make_pair("windows_gst1_18", "x264enc"),
		std::make_pair("ubuntu_gst1_16", "x264enc")));

TEST_P(VideoDDSsubscriberTestFixture, FrameOutcome)
{
	const std::pair<std::string, std::string> encoder = GetParam();
	const std::string encoderDirectory = std::get<0>(encoder);
	const std::string encoderName = std::get<1>(encoder);

	constexpr int expectedWidth = 112;
	constexpr int expectedHeight = 32;

	VideoDDSsubscriber v{false};

	constexpr int numberOfSamples = 10;
	g_object_set(v.displayAppSink(),
				 "max-buffers", numberOfSamples,
				 "drop", false,
				 nullptr);

	// Wait for pipeline to be playing
	auto pipeline = GST_ELEMENT(gst_element_get_parent(v.displayAppSink()));
	auto stateResult = gst_element_get_state(pipeline, nullptr, nullptr, 3'000'000'000 /*timeout ns*/);
	if (stateResult == GST_STATE_CHANGE_FAILURE)
	{
		gst_object_unref(pipeline);
		FAIL() << "Pipeline 1 not running. Result: " << stateResult;
	};

	const auto unitTestDataDirectory = std::filesystem::canonical(std::filesystem::path{VODA_UNITTEST_DATA_DIRECTORY});

	//////////////  Push into pipeline
	auto sampleIndex = 0;
	for (; sampleIndex < numberOfSamples; ++sampleIndex)
	{
		std::ostringstream sampleFileName;
		sampleFileName << std::setfill('0') << std::setw(5) << sampleIndex << ".h264";
		const auto sampleFilePath = unitTestDataDirectory / encoderDirectory / encoderName / sampleFileName.str();
		const auto length = std::filesystem::file_size(sampleFilePath);
		std::ifstream file(sampleFilePath, std::ios::binary | std::ios::in);
		GstMapInfo mapInfo;
		auto gstBuffer = gst_buffer_new_allocate(nullptr /* no allocator */, length, nullptr /* no parameter */);
		gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_WRITE);
		file.read(reinterpret_cast<char *>(mapInfo.data), length);
		gst_buffer_unmap(gstBuffer, &mapInfo);

		const auto ret = gst_app_src_push_buffer(v.ddsAppSrc(), gstBuffer);
		if (ret != GST_FLOW_OK)
		{
			FAIL() << "Something went wrong while injecting frame data into the pipeline";
		}
	}

	gst_object_unref(pipeline);

	//////////////  Pull from pipeline

	for (auto frameIndex = 0; frameIndex < numberOfSamples; ++frameIndex)
	{
		const auto frame = gst_app_sink_try_pull_sample(v.displayAppSink(), 3'000'000'000 /*timeout ns*/);
		EXPECT_NE(frame, nullptr) << "gst_app_sink_try_pull_sample failed after timeout on frame index " << frameIndex;

		const auto caps = gst_sample_get_caps(frame);
		const auto capsStruct = gst_caps_get_structure(caps, 0 /*index*/);
		EXPECT_NE(capsStruct, nullptr) << "Failed getting structure from caps.";
		int resultWidth = 0;
		int resultHeight = 0;
		gst_structure_get_int(capsStruct, "width", &resultWidth);
		gst_structure_get_int(capsStruct, "height", &resultHeight);
		EXPECT_EQ(resultWidth, expectedWidth) << "Width of frame not correct";
		EXPECT_EQ(resultHeight, expectedHeight) << "Height of frame not correct";

		const auto sampleBuffer = gst_sample_get_buffer(frame);
		EXPECT_NE(sampleBuffer, nullptr);

		GstMapInfo mapInfo;
		gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

		auto dataResult = const_cast<const uint8_t *>(mapInfo.data);
		const auto lengthResult = static_cast<int>(mapInfo.size);

		std::ostringstream outFileName;
		outFileName << "decoded" << std::setfill('0') << std::setw(5) << frameIndex << ".rgba.raw";
		auto wf = std::ofstream{outFileName.str(), std::ios::out | std::ios::binary};
		wf.write(reinterpret_cast<const char *>(mapInfo.data), lengthResult);
		wf.close();

		std::ostringstream expectedSampleFileName;
		expectedSampleFileName << std::setfill('0') << std::setw(5) << frameIndex << std::setw(0);
		expectedSampleFileName << "_RGBA_w" << expectedWidth << "_h" << expectedHeight << ".raw";
		const auto expectedSampleFilePath = unitTestDataDirectory / expectedSampleFileName.str();
		const auto lengthExpected = std::filesystem::file_size(expectedSampleFilePath);
		EXPECT_EQ(lengthResult, lengthExpected) << "Length of expected and result not equal of frame index " << frameIndex;
		std::ifstream expectedFile(expectedSampleFilePath, std::ios::binary | std::ios::in);
		std::istream_iterator<uint8_t> start(expectedFile), end;
		const std::vector<uint8_t> expectedData{start, end};

		const std::vector<uint8_t> resultData{&dataResult[0], &dataResult[lengthResult]};
		EXPECT_EQ(resultData.size(), expectedData.size()) << "Byte size of expected and result not equal of frame index " << frameIndex;

		std::vector<int16_t> absoluteDifference(lengthResult);
		std::transform(resultData.begin(), resultData.end(), expectedData.begin(), absoluteDifference.begin(),
					   [](int16_t a, int16_t b)
					   { return std::abs(a - b); });
		const auto sum = std::accumulate(absoluteDifference.begin(), absoluteDifference.end(), 0);
		constexpr auto max = expectedWidth * expectedHeight * 4 /*bytes per pixel*/ * 15 /* Max diff per byte */;
		EXPECT_LT(sum, max) << "Difference of frame index " << frameIndex << " too large";

		gst_buffer_unmap(sampleBuffer, &mapInfo);
		gst_sample_unref(frame);
	}
}
