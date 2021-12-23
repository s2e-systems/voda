cmake_minimum_required(VERSION 3.14.0)
project(VoDA LANGUAGES CXX)

option(BUILD_UNIT_TESTS "Weather to build unit tests (requires GoogleTest)" OFF)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(GStreamer REQUIRED COMPONENTS glib gobject base app video)

find_package(CycloneDDS-CXX REQUIRED)
idlcxx_generate(TARGET GeneratedIdl FILES Common/VideoDDS.idl)

add_executable(VideoDDSSubscriber
	VideoDDSSubscriber/main.cpp
	VideoDDSSubscriber/videoddssubscriber.h
	VideoDDSSubscriber/videoddssubscriber.cpp
	VideoDDSSubscriber/videolistener.h
	VideoDDSSubscriber/videolistener.cpp
	Common/qtgstreamer.h
	Common/qtgstreamer.cpp
	Common/videowidgetpaintergst.h
	Common/videowidgetpaintergst.cpp
)
target_include_directories(VideoDDSSubscriber PRIVATE
	VideoDDSSubscriber
	Common
)
target_link_libraries(VideoDDSSubscriber
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
	PRIVATE GStreamer::gstreamer
	PRIVATE GStreamer::gobject
	PRIVATE GStreamer::glib
	PRIVATE GStreamer::base
	PRIVATE GStreamer::app
	PRIVATE GStreamer::video
	PRIVATE CycloneDDS-CXX::ddscxx
	PRIVATE GeneratedIdl
)
target_compile_features(VideoDDSSubscriber PRIVATE cxx_std_17)


add_executable(VideoDDSPublisher
	VideoDDSPublisher/main.cpp
	VideoDDSPublisher/videoddspublisher.h
	VideoDDSPublisher/videoddspublisher.cpp
	Common/qtgstreamer.h
	Common/qtgstreamer.cpp
	Common/videowidgetpaintergst.h
	Common/videowidgetpaintergst.cpp
	Common/cameracapabilities.h
	Common/cameracapabilities.cpp
	Common/elements.h
	Common/elements.cpp
)
target_include_directories(VideoDDSPublisher PRIVATE
	VideoDDSPublisher
	Common
)
target_link_libraries(VideoDDSPublisher
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
	PRIVATE GStreamer::gstreamer
	PRIVATE GStreamer::glib
	PRIVATE GStreamer::gobject
	PRIVATE GStreamer::base
	PRIVATE GStreamer::app
	PRIVATE GStreamer::video
	PRIVATE CycloneDDS-CXX::ddscxx
	PRIVATE GeneratedIdl
)
target_compile_features(VideoDDSSubscriber PRIVATE cxx_std_17)


if (${BUILD_UNIT_TESTS})

	include(FetchContent)
	FetchContent_Declare(googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929 # release-1.11.0
	)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)

	set(VODA_UNITTEST_DATA_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/UnitTests/data")
	configure_file(UnitTest/config.h.in config.h)


	add_executable(UnitTests
		UnitTest/mainunittests.cpp
		Common/cameracapabilities.h
		Common/cameracapabilities.cpp
		Common/cameracapabilities_tests.cpp
		Common/elements.h
		Common/elements.cpp
		Common/elements_tests.cpp
	)
	target_link_libraries(UnitTests
		PRIVATE gtest
		PRIVATE GStreamer::gstreamer
		PRIVATE GStreamer::glib
		PRIVATE GStreamer::gobject
	)
	target_include_directories(UnitTests
		PRIVATE Common
	)
	target_compile_features(UnitTests PRIVATE cxx_std_17)
	include(GoogleTest)



	add_executable(UnitTests_recorder
		UnitTest/mainrecorder.cpp
	)
	target_link_libraries(UnitTests_recorder
		PRIVATE GStreamer::gstreamer
		PRIVATE GStreamer::glib
		PRIVATE GStreamer::gobject
		PRIVATE GStreamer::base
		PRIVATE GStreamer::app
		PRIVATE GStreamer::video
	)
	target_compile_features(UnitTests_recorder PRIVATE cxx_std_17)

endif()
