cmake_minimum_required(VERSION 3.1.0)
project(VideoDDSPublisher LANGUAGES CXX)

set(CommonDir "../Common")
set(SourceDir ".")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(GStreamer REQUIRED glib gobject base app video)

find_package(CycloneDDS-CXX REQUIRED)
idlcxx_generate(TARGET GeneratedIdlPublisher FILES ${CommonDir}/VideoDDS.idl)

add_executable(VideoDDSPublisher
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
	PRIVATE GStreamer::gstreamer
	PRIVATE GStreamer::glib
	PRIVATE GStreamer::gobject
	PRIVATE GStreamer::base
	PRIVATE GStreamer::app
	PRIVATE GStreamer::video
	PRIVATE CycloneDDS-CXX::ddscxx
	PRIVATE GeneratedIdlPublisher
)
