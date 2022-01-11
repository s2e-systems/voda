#include "cameracapabilities.h"
#include <gtest/gtest.h>

TEST(CapabilitySelectionTest, highestRawFrameRate)
{
	const int expectedNominator = 63 * 10000000;
	const int expectedDenominator =  88 * 455 * 525;
	const double expected = static_cast<double>(expectedNominator) / static_cast<double>(expectedDenominator);

	auto caps = gst_caps_new_full(
		gst_structure_new("image/jpeg",
				"width", G_TYPE_INT, 1920,
				"height", G_TYPE_INT, 1080,
				"framerate", GST_TYPE_FRACTION, 20, 1,
			nullptr),
		gst_structure_new("video/x-raw",
				"format", G_TYPE_STRING, "YUY2",
				"width", G_TYPE_INT, 1920,
				"height", G_TYPE_INT, 1080,
				"framerate", GST_TYPE_FRACTION, expectedNominator, expectedDenominator,
				 nullptr
		),
		gst_structure_new("image/jpeg",
				"width", G_TYPE_INT, 1920,
				"height", G_TYPE_INT, 1080,
				"framerate", GST_TYPE_FRACTION, 15, 1,
				 nullptr
		),
		nullptr
	);
	gst_caps_set_features_simple(caps, gst_caps_features_new_any());

	CapabilitySelection c{caps};
	const auto result = c.highestRawFrameRate();

	EXPECT_EQ(expected, result);

	gst_caps_unref(caps);
}


TEST(CapabilitySelectionTest, highestRawArea)
{
	auto expectedStruct = gst_structure_new("video/x-raw",
		"format", G_TYPE_STRING, "YUY2",
		"width", G_TYPE_INT, 1920,
		"height", G_TYPE_INT, 1080,
		"framerate", GST_TYPE_FRACTION, 20, 1,
	nullptr);

	auto expected = gst_caps_new_full(expectedStruct, nullptr);
	gst_caps_set_features_simple(expected, gst_caps_features_new_any());

	auto caps = gst_caps_new_full(
		gst_structure_new("image/jpeg",
				"width", G_TYPE_INT, 1920,
				"height", G_TYPE_INT, 1080,
				"framerate", GST_TYPE_FRACTION, 30, 1,
				 nullptr
		),
		gst_structure_new("video/x-raw",
				"format", G_TYPE_STRING, "YUY2",
				"width", G_TYPE_INT, 640,
				"height", G_TYPE_INT, 480,
				"framerate", GST_TYPE_FRACTION, 10, 1,
				 nullptr
		),
		gst_structure_copy(expectedStruct),
		nullptr
	);
	gst_caps_set_features_simple(caps, gst_caps_features_new_any());

	CapabilitySelection c{caps};
	const auto result = c.highestRawArea();
	const std::string expectedCaps{gst_caps_to_string(expected)};
	const std::string resultCaps{gst_caps_to_string(result)};
	EXPECT_TRUE(gst_caps_is_equal(expected, result)) << "Expected caps are: " << expectedCaps << "\nResulting caps are: " << resultCaps;

	gst_caps_unref(expected);
	gst_caps_unref(result);
	gst_caps_unref(caps);
}

TEST(CapabilitySelectionTest, highestRawAreaWithMinimumFramerate)
{
	auto expectedStruct = gst_structure_new("video/x-raw",
		"format", G_TYPE_STRING, "YUY2",
		"width", G_TYPE_INT, 640,
		"height", G_TYPE_INT, 480,
		"framerate", GST_TYPE_FRACTION, 30, 1,
	nullptr);

	auto expected = gst_caps_new_full(expectedStruct, nullptr);
	gst_caps_set_features_simple(expected, gst_caps_features_new_any());

	auto caps = gst_caps_new_full(
		gst_structure_new("image/jpeg",
				"width", G_TYPE_INT, 1920,
				"height", G_TYPE_INT, 1080,
				"framerate", GST_TYPE_FRACTION, 35, 1,
				 nullptr
		),
		gst_structure_new("video/x-raw",
				"format", G_TYPE_STRING, "YUY2",
				"width", G_TYPE_INT, 1920,
				"height", G_TYPE_INT, 1080,
				"framerate", GST_TYPE_FRACTION, 10, 1,
				 nullptr
		),
		gst_structure_copy(expectedStruct),
		nullptr
	);
	gst_caps_set_features_simple(caps, gst_caps_features_new_any());

	CapabilitySelection c{caps};
	const auto result = c.highestRawArea(c.highestRawFrameRate());
	const std::string expectedCaps{gst_caps_to_string(expected)};
	const std::string resultCaps{gst_caps_to_string(result)};
	EXPECT_TRUE(gst_caps_is_equal(expected, result)) << "Expected caps are: " << expectedCaps << "\nResulting caps are: " << resultCaps;

	gst_caps_unref(expected);
	gst_caps_unref(result);
	gst_caps_unref(caps);
}

