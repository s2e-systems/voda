cmake_minimum_required(VERSION 3.1.0)
project(VideoDDSSubscriber)

set(CommonDir "../Common")
set(SourceDir ".")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

find_package(GStreamer REQUIRED COMPONENTS glib gobject base app video)

find_package(OpenSplice REQUIRED)
OpenSplice_IDLCPP_GEN(${CommonDir}/VideoDDS.idl ${CMAKE_CURRENT_BINARY_DIR})

set_property(SOURCE ${OpenSplice_GENERATED_SOURCES} PROPERTY SKIP_AUTOMOC ON)

add_executable(VideoDDSSubscriber
	${OpenSplice_GENERATED_SOURCES}
	${SourceDir}/main.cpp
	${SourceDir}/videoddssubscriber.cpp
	${SourceDir}/videolistener.cpp
	${CommonDir}/qtgstreamer.cpp
	${CommonDir}/videowidgetpaintergst.cpp
)

include_directories(VideoDDSSubscriber
	${SourceDir}
	${CommonDir}
)

target_link_libraries(VideoDDSSubscriber
	PRIVATE Qt6::Widgets
	PRIVATE GStreamer::gstreamer PRIVATE GStreamer::gobject PRIVATE GStreamer::glib PRIVATE GStreamer::base PRIVATE GStreamer::app PRIVATE GStreamer::video
	PRIVATE OpenSplice::ddskernel PRIVATE OpenSplice::dcpsisocpp2 PRIVATE OpenSplice::dcpssacpp
)
