#include <QApplication>

#include <gtest/gtest.h>
#include <gst/gst.h>

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	gst_init(nullptr, nullptr);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
