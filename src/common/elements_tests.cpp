#include "elements.h"
#include <gtest/gtest.h>

TEST(ElementSelectionTest, Selection)
{
	ElementSelection e{{"videotestsrc", "noneexistent", "fakesink"}, "name"};
	EXPECT_EQ(e.elementName(), "videotestsrc");

	EXPECT_THROW((ElementSelection{{"noneexistent"}, "name"}), std::runtime_error);
}

