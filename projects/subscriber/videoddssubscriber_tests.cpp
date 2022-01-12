#include <string>
#include <ostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <gtest/gtest.h>

#include "videoddssubscriber.h"
#include "config.h"

TEST(VideoDDSsubscriberTest, Construction)
{
	VideoDDSsubscriber v{false};

	const auto unitTestDataDirectory = std::filesystem::canonical(std::filesystem::path{VODA_UNITTEST_DATA_DIRECTORY});
	const auto sampleFile = unitTestDataDirectory / "windows_gst1_16_0" / "x264enc00001.h264";
	std::cout << sampleFile.string() << std::endl;

	const auto length = std::filesystem::file_size(sampleFile);

	std::ifstream file;
	file.open(sampleFile, std::ios::binary|std::ios::in);

	GstMapInfo mapInfo;
	auto gstBuffer = gst_buffer_new_allocate(nullptr /* no allocator */, length, nullptr /* no parameter */);
	gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_WRITE);
	file.read(reinterpret_cast<char*>(mapInfo.data), length);

	for (int i = 0; i < length; ++i)
		std::cout << std::hex << std::setfill('0') << std::setw(2) << int(mapInfo.data[i]) << " ";
	std::cout << std::endl;

	gst_buffer_unmap(gstBuffer, &mapInfo);

	const auto ret = gst_app_src_push_buffer(v.ddsAppSrc(), gstBuffer);
	if (ret != GST_FLOW_OK)
	{
		FAIL() << "Something went wrong while injecting fram data into the pipeline";
	}
}
