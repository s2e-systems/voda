cmake_minimum_required(VERSION 3.18.1)

project("android_subscriber")

find_library(log-lib log)

# ###  DDS  ###
if(NOT DEFINED ENV{CycloneDDS_ROOT_ANDROID})
  message(FATAL_ERROR "CycloneDDS_ROOT_ANDROID environment variable not set")
endif()
set(CycloneDDS_DIR
    $ENV{CycloneDDS_ROOT_ANDROID}/${ANDROID_ABI}/lib/cmake/CycloneDDS)
set(CycloneDDS-CXX_DIR
    $ENV{CycloneDDS_ROOT_ANDROID}/${ANDROID_ABI}/lib/cmake/CycloneDDS-CXX)
find_package(CycloneDDS-CXX REQUIRED)

# On a Windows host the _idlcxx_shared_lib cycloneddsidlcxx can not be found
# since only ".so" files with the "lib" prefix ae searched
list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "")
list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
idlcxx_generate(
  TARGET GeneratedIdl FILES ../../../../../../src/common/VideoDDS.idl
  DEFAULT_EXTENSIBILITY final)

# ##  GStreamer   ###
set(gstreamer_abi_directory_name ${ANDROID_ABI})

if(${ANDROID_ABI} STREQUAL armeabi-v7a)
  set(gstreamer_abi_directory_name armv7)
endif()

if(${ANDROID_ABI} STREQUAL arm64-v8a)
  set(gstreamer_abi_directory_name arm64)
endif()

if(NOT DEFINED ENV{GSTREAMER_1_0_ROOT_ANDROID})
  message(FATAL_ERROR "GSTREAMER_1_0_ROOT_ANDROID environment variable not set")
endif()

set(gstreamer_path
    $ENV{GSTREAMER_1_0_ROOT_ANDROID}/${gstreamer_abi_directory_name})

add_library(
  android_subscriber SHARED
  gstreamer_android-1.0.c subscriber_binding.cpp surface_holder_binding.cpp
  Subscriber.cpp VideoDDS.cpp VideoDDS.hpp)

set_target_properties(android_subscriber PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  android_subscriber
  PRIVATE
  PUBLIC ${gstreamer_path}/include/gstreamer-1.0
         ${gstreamer_path}/include/glib-2.0
         ${gstreamer_path}/lib/glib-2.0/include)

target_link_libraries(
  android_subscriber
  CycloneDDS-CXX::ddscxx
  android
  GeneratedIdl
  ${log-lib}
  EGL
  ${gstreamer_path}/lib/gstreamer-1.0/libgstapp.a
  ${gstreamer_path}/lib/gstreamer-1.0/libgstautodetect.a
  ${gstreamer_path}/lib/gstreamer-1.0/libgstcoreelements.a
  ${gstreamer_path}/lib/gstreamer-1.0/libgstopengl.a
  ${gstreamer_path}/lib/gstreamer-1.0/libgstopenh264.a
  ${gstreamer_path}/lib/gstreamer-1.0/libgstvideoconvertscale.a
  ${gstreamer_path}/lib/libffi.a
  ${gstreamer_path}/lib/libgio-2.0.a
  ${gstreamer_path}/lib/libglib-2.0.a
  ${gstreamer_path}/lib/libgmodule-2.0.a
  ${gstreamer_path}/lib/libgobject-2.0.a
  ${gstreamer_path}/lib/libgraphene-1.0.a
  ${gstreamer_path}/lib/libgstapp-1.0.a
  ${gstreamer_path}/lib/libgstbase-1.0.a
  ${gstreamer_path}/lib/libgstcontroller-1.0.a
  ${gstreamer_path}/lib/libgstgl-1.0.a
  ${gstreamer_path}/lib/libgstpbutils-1.0.a
  ${gstreamer_path}/lib/libgstreamer-1.0.a
  ${gstreamer_path}/lib/libgsttag-1.0.a
  ${gstreamer_path}/lib/libgstvideo-1.0.a
  ${gstreamer_path}/lib/libiconv.a
  ${gstreamer_path}/lib/libintl.a
  ${gstreamer_path}/lib/libjpeg.a
  ${gstreamer_path}/lib/libopenh264.a
  ${gstreamer_path}/lib/liborc-0.4.a
  ${gstreamer_path}/lib/libpcre2-8.a
  ${gstreamer_path}/lib/libpng16.a
  ${gstreamer_path}/lib/libz.a
  ${gstreamer_path}/lib/gstreamer-1.0/libgstlibav.a
  ${gstreamer_path}/lib/libavutil.a
  ${gstreamer_path}/lib/libavcodec.a
  ${gstreamer_path}/lib/libavformat.a
  ${gstreamer_path}/lib/libavfilter.a
  ${gstreamer_path}/lib/libswresample.a
  ${gstreamer_path}/lib/libgstaudio-1.0.a
  ${gstreamer_path}/lib/libbz2.a)
