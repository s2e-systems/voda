cmake_minimum_required(VERSION 3.1.0)
project(VideoDDSPublisher)

set(CommonDir "../Common")
set(SourceDir ".")

# TODO: The cmake module path seems not to accept relative pathes
# hence the temporary hard coded path here
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Common/cmakemodules")
list(APPEND CMAKE_MODULE_PATH "/home/dev/Common/cmakemodules")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

###########
# Qt
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

#########
# Gstreamer
find_package(GStreamer REQUIRED glib gobject base app video)

#########
# OSPL DDS
find_package(OSPL REQUIRED)
message(STATUS "OSPL_INCLUDE_DIRS: \"${OSPL_INCLUDE_DIRS}\"")
message(STATUS "OSPL_LIBRARIES: \"${OSPL_LIBRARIES}\"")

set(IDL_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})
OSPL_IDLCPP_GEN(${CommonDir} VideoDDS ${IDL_GENERATED_DIR})

########
# Sources
set(VideoDDSPublisherSources
	${OSPL_GENERATED_SOURCES}
	${SourceDir}/main.cpp
	${SourceDir}/pipelinedds.cpp
	${SourceDir}/videoddspublisher.cpp
	${CommonDir}/videowidgetgst.cpp
	${CommonDir}/pipeline.cpp
	${CommonDir}/qtgstreamer.cpp
	${CommonDir}/videowidgetgles2.cpp
)

add_executable(VideoDDSPublisher ${VideoDDSPublisherSources})

include_directories(VideoDDSPublisher
	${SourceDir}
	${CommonDir}
	${GStreamer_INCLUDE_DIRS}
	${OSPL_INCLUDE_DIRS}
	${IDL_GENERATED_DIR}
)

target_link_libraries(VideoDDSPublisher
	Qt5::Widgets
	${GStreamer_LIBRARIES}
	${OSPL_LIBRARIES}
)
