cmake_minimum_required(VERSION 3.1.0)
project(VideoDDSPublisher)

set(CommonDir "../Common")
set(SourceDir ".")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets CONFIG REQUIRED)

find_package(GStreamer REQUIRED glib gobject base app video)

find_package(OpenSplice REQUIRED)
OpenSplice_IDLCPP_GEN(${CommonDir}/VideoDDS.idl ${CMAKE_CURRENT_BINARY_DIR})

set_property(SOURCE ${OpenSplice_GENERATED_SOURCES} PROPERTY SKIP_AUTOMOC ON)

add_executable(VideoDDSPublisher
	${OpenSplice_GENERATED_SOURCES}
	${SourceDir}/main.cpp
	${SourceDir}/videoddspublisher.cpp
	${CommonDir}/videowidgetgst.cpp
	${CommonDir}/qtgstreamer.cpp
	${CommonDir}/pipeline.cpp
	${CommonDir}/videowidgetgles2.cpp
	${CommonDir}/videowidgetpaintergst.cpp
)

include_directories(VideoDDSPublisher
	${SourceDir}
	${CommonDir}
)

target_link_libraries(VideoDDSPublisher
	Qt5::Widgets
	GStreamer::gstreamer GStreamer::glib GStreamer::gobject GStreamer::base GStreamer::app GStreamer::video
	OpenSplice::ddskernel OpenSplice::dcpsisocpp2 OpenSplice::dcpssacpp
)
