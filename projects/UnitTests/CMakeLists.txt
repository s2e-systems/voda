cmake_minimum_required(VERSION 3.14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(FetchContent)
FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929 # release-1.11.0
)
FetchContent_MakeAvailable(googletest)

find_package(GStreamer REQUIRED glib gobject base app video codecparsers)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

add_definitions( -DGST_USE_UNSTABLE_API)

include(GoogleTest)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(CommonDir "../Common")
include_directories(${CommonDir})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(tests_automatic automatictests.cpp)

#set_source_files_properties(manualtests.cpp PROPERTIES COMPILE_FLAGS -Wold-style-cast)
set(VODA_UNITTEST_DATA_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data")
configure_file("config.h.in" "config.h")

add_executable(tests_manual
	mainmanualtests.cpp
	manualtests.cpp
	${CommonDir}/qtgstreamer.cpp
	${CommonDir}/videowidgetpaintergst.cpp
)

add_executable(tests_recorder
	mainrecorder.cpp
)

target_link_libraries(tests_recorder PRIVATE
	GStreamer::gstreamer GStreamer::glib GStreamer::gobject GStreamer::base GStreamer::app GStreamer::video
)

target_link_libraries(tests_automatic PRIVATE
	gtest
	GStreamer::gstreamer GStreamer::glib GStreamer::gobject GStreamer::base GStreamer::app GStreamer::video
)
gtest_discover_tests(tests_automatic)

target_link_libraries(tests_manual PRIVATE
	gtest
	GStreamer::gstreamer GStreamer::glib GStreamer::gobject GStreamer::base GStreamer::app GStreamer::video
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
)
gtest_discover_tests(tests_manual)
