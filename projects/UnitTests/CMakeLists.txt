list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(FetchContent)
FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG 2fe3bd994b3189899d93f1d5a881e725e046fdc2 # release-1.8.1
)
# NOTE: "A more concise way of expressing the following is available with
# CMake 3.14 or later" [Professional CMake]
FetchContent_GetProperties(googletest)
if(NOT GoogleTest_POPULATED)
	FetchContent_Populate(googletest)
	add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

find_package(GStreamer REQUIRED glib gobject base app video codecparsers)
find_package(Qt5Widgets REQUIRED)

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
	Qt5::Widgets
)
gtest_discover_tests(tests_manual)
