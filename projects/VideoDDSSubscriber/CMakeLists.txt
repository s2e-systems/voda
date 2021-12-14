cmake_minimum_required(VERSION 3.1.0)
project(VideoDDSSubscriber LANGUAGES CXX)

set(CommonDir "../Common")
set(SourceDir ".")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(GStreamer REQUIRED COMPONENTS glib gobject base app video)

if(NOT TARGET CycloneDDS-CXX::ddscxx)
	find_package(CycloneDDS-CXX REQUIRED)
endif()
if(NOT TARGET VideoDDS)
	idlcxx_generate(TARGET VideoDDS FILES ../Common/VideoDDS.idl)
endif()

add_executable(VideoDDSSubscriber
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
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
	PRIVATE GStreamer::gstreamer
	PRIVATE GStreamer::gobject
	PRIVATE GStreamer::glib
	PRIVATE GStreamer::base
	PRIVATE GStreamer::app
	PRIVATE GStreamer::video
	PRIVATE CycloneDDS-CXX::ddscxx
	PRIVATE VideoDDS
)
