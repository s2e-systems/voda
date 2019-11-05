#include <gtest/gtest.h>

#include "cameracapabilities.h"

TEST(CapabilitySelectionTest, highestPixelRate)
{
	gst_init(nullptr, nullptr);
	{
	auto expectedStruct = gst_structure_new("image/jpeg",
		"width", G_TYPE_INT, 1920,
		"height", G_TYPE_INT, 1080,
		"framerate", GST_TYPE_FRACTION, 30, 1,
	nullptr);

	auto expected = gst_caps_new_full(expectedStruct, nullptr);
	gst_caps_set_features_simple(expected, gst_caps_features_new_any());

	auto caps = gst_caps_new_full(
		gst_structure_new("video/x-raw",
				"format", G_TYPE_STRING, "YUY2",
				"width", G_TYPE_INT, 1,
				"height", G_TYPE_INT, 1,
				"framerate", GST_TYPE_FRACTION, 1, 1,
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
	const auto result = c.highestPixelRate();
	const std::string expectedCaps{gst_caps_to_string(expected)};
	const std::string resultCaps{gst_caps_to_string(result)};
	EXPECT_TRUE(gst_caps_is_equal(expected, result)) << "Expected caps are: " << expectedCaps << "\nResulting caps are: " << resultCaps;

	gst_caps_unref(expected);
	gst_caps_unref(result);
	gst_caps_unref(caps);
	}
	gst_deinit();
}

TEST(CapabilitySelectionTest, highestPixelRateWithMinimumFramerate)
{
	gst_init(nullptr, nullptr);
	{
	// Pixelrate high but low framerate
	auto expectedStructA = gst_structure_new("image/jpeg", "width", G_TYPE_INT, 100, "height", G_TYPE_INT, 10, "framerate", GST_TYPE_FRACTION, 10, 1, nullptr);//Pxps=10000
	auto expectedA = gst_caps_new_full(expectedStructA, nullptr);
	gst_caps_set_features_simple(expectedA, gst_caps_features_new_any());
	// Pixelrate low but high framerate
	auto expectedStructB = gst_structure_new("image/jpeg", "width", G_TYPE_INT, 100, "height", G_TYPE_INT, 1, "framerate", GST_TYPE_FRACTION, 12, 1, nullptr);//Pxps=1200
	auto expectedB = gst_caps_new_full(expectedStructB, nullptr);
	gst_caps_set_features_simple(expectedB, gst_caps_features_new_any());

	auto caps = gst_caps_new_full(
		gst_structure_new("video/x-raw", "width", G_TYPE_INT, 100, "height", G_TYPE_INT, 1, "framerate", GST_TYPE_FRACTION, 1, 1, nullptr), //Pxps=100
		gst_structure_copy(expectedStructA),
		gst_structure_copy(expectedStructB),
		nullptr
	);
	gst_caps_set_features_simple(caps, gst_caps_features_new_any());

	CapabilitySelection c{caps};
	const auto resultA = c.highestPixelRate();
	const std::string expectedCapsA{gst_caps_to_string(expectedA)};
	const std::string resultCapsA{gst_caps_to_string(resultA)};
	EXPECT_TRUE(gst_caps_is_equal(expectedA, resultA)) << "A, Expected caps are: " << expectedCapsA << "\nResulting caps are: " << resultCapsA;

	const auto resultB = c.highestPixelRate(12.0);
	const std::string expectedCapsB{gst_caps_to_string(expectedB)};
	const std::string resultCapsB{gst_caps_to_string(resultB)};
	EXPECT_TRUE(gst_caps_is_equal(expectedB, resultB)) << "B, Expected caps are: " << expectedCapsB << "\nResulting caps are: " << resultCapsB;

	const auto resultC = c.highestPixelRate(100.0);
	EXPECT_EQ(resultC, nullptr) << "C must be null";


	gst_caps_unref(expectedA);
	gst_caps_unref(expectedB);
	gst_caps_unref(resultA);
	gst_caps_unref(resultB);
	gst_caps_unref(caps);
	} // scope-end to de-init CapabilitySelection before gst_deint()
	gst_deinit();
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
