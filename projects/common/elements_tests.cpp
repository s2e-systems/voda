#include "elements.h"
#include <gtest/gtest.h>

TEST(ElementSelectionTest, Selection)
{
	GError* gerr = nullptr;
	auto gret = gst_init_check(nullptr, nullptr, &gerr);
	if (!gret)
	{
		const std::string err = "Could not initialize GStreamer: " + std::string(gerr->message);
		g_error_free(gerr);
		FAIL() << err;
	}
	ElementSelection e{{"videotestsrc", "noneexistent", "fakesink"}, "name"};
	EXPECT_EQ(e.elementName(), "videotestsrc");

	EXPECT_THROW((ElementSelection{{"noneexistent"}, "name"}), std::runtime_error);

	gst_deinit();
}

