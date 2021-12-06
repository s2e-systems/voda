cmake_minimum_required(VERSION 3.1.0)
project(VideoDDSPublisher)

set(CommonDir "../Common")
set(SourceDir ".")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(GStreamer REQUIRED glib gobject base app video)

find_package(OpenSplice REQUIRED)
OpenSplice_IDLCPP_GEN(${CommonDir}/VideoDDS.idl ${CMAKE_CURRENT_BINARY_DIR})

set_property(SOURCE ${OpenSplice_GENERATED_SOURCES} PROPERTY SKIP_AUTOMOC ON)

add_executable(VideoDDSPublisher
	${OpenSplice_GENERATED_SOURCES}
	${SourceDir}/main.cpp
	${SourceDir}/videoddspublisher.cpp
	${CommonDir}/qtgstreamer.cpp
	${CommonDir}/videowidgetpaintergst.cpp
)

include_directories(VideoDDSPublisher
	${SourceDir}
	${CommonDir}
)

target_link_libraries(VideoDDSPublisher
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
	PRIVATE GStreamer::gstreamer PRIVATE GStreamer::glib PRIVATE GStreamer::gobject PRIVATE GStreamer::base PRIVATE GStreamer::app PRIVATE GStreamer::video
	PRIVATE OpenSplice::ddskernel PRIVATE OpenSplice::dcpsisocpp2 PRIVATE OpenSplice::dcpssacpp
)
