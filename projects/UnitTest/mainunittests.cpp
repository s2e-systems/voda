#include <gtest/gtest.h>
#include <gst/gst.h>

int main(int argc, char **argv)
{
	GError * gerr = nullptr;
	const auto gret = gst_init_check(&argc, &argv, &gerr);
	if (gret == FALSE)
	{
		const std::string err{"Could not initialize GStreamer: " + std::string{gerr->message}};
		g_error_free(gerr);
		FAIL() << err;
	}

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}