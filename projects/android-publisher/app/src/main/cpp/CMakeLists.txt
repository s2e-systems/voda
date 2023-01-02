cmake_minimum_required(VERSION 3.18.1)

project("android_publisher")


find_library(log-lib log)

####  DDS  ###

set(CycloneDDS_DIR C:/Users/StefanKimmer/workcopies/build-cyclonedds/android_${ANDROID_ABI}/install/lib/cmake/CycloneDDS)
set(CycloneDDS-CXX_DIR C:/Users/StefanKimmer/workcopies/build-cyclonedds-cxx/android_${ANDROID_ABI}/install/lib/cmake/CycloneDDS-CXX)
find_package(CycloneDDS-CXX REQUIRED)


###  GStreamer   ###

set(gstreamer_abi_directory_name ${ANDROID_ABI})
if (${ANDROID_ABI} STREQUAL armeabi-v7a)
    set(gstreamer_abi_directory_name armv7)
endif()
if (${ANDROID_ABI} STREQUAL arm64-v8a)
    set(gstreamer_abi_directory_name arm64)
endif()

if(NOT DEFINED ENV{GSTREAMER_1_0_ROOT_ANDROID})
        message(FATAL_ERROR "GSTREAMER_1_0_ROOT_ANDROID environment variable not set")
endif()

set(gstreamer_path $ENV{GSTREAMER_1_0_ROOT_ANDROID}/${gstreamer_abi_directory_name})

set(PLUGINS_DECLARATION "
    GST_PLUGIN_STATIC_DECLARE(androidmedia);
    GST_PLUGIN_STATIC_DECLARE(app);
    GST_PLUGIN_STATIC_DECLARE(audiofx);
    GST_PLUGIN_STATIC_DECLARE(audiofxbad);
    GST_PLUGIN_STATIC_DECLARE(audiotestsrc);
    GST_PLUGIN_STATIC_DECLARE(autodetect);
    GST_PLUGIN_STATIC_DECLARE(camerabin);
    GST_PLUGIN_STATIC_DECLARE(coreelements);
    GST_PLUGIN_STATIC_DECLARE(effectv);
    GST_PLUGIN_STATIC_DECLARE(ipcpipeline);
    GST_PLUGIN_STATIC_DECLARE(opengl);
    GST_PLUGIN_STATIC_DECLARE(openh264);
    GST_PLUGIN_STATIC_DECLARE(opensles);
    GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
    GST_PLUGIN_STATIC_DECLARE(videoconvert);
    GST_PLUGIN_STATIC_DECLARE(videofilter);
    GST_PLUGIN_STATIC_DECLARE(videotestsrc);
    GST_PLUGIN_STATIC_DECLARE(videorate);
")
set(PLUGINS_REGISTRATION "
    GST_PLUGIN_STATIC_REGISTER(androidmedia);
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(audiofx);
    GST_PLUGIN_STATIC_REGISTER(audiofxbad);
    GST_PLUGIN_STATIC_REGISTER(audiotestsrc);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    GST_PLUGIN_STATIC_REGISTER(camerabin);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(effectv);
    GST_PLUGIN_STATIC_REGISTER(ipcpipeline);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(openh264);
    GST_PLUGIN_STATIC_REGISTER(opensles);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(videofilter);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(videorate);
")

set(G_IO_MODULES_DECLARE "")
set(G_IO_MODULES_LOAD "")

configure_file(
        ${gstreamer_path}/share/gst-android/ndk-build/gstreamer_android-1.0.c.in gstreamer_android-1.0.c
)

set_source_files_properties(gstreamer_android-1.0.c
        PROPERTIES GENERATED TRUE)

add_library(gstreamer_android SHARED
        gstreamer_android-1.0.c
        dummy.cpp
        )

target_link_libraries(gstreamer_android
        ${log-lib}
        EGL
        OpenSLES
        ${gstreamer_path}/lib/gstreamer-1.0/libgstandroidmedia.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstapp.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstaudiofx.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstaudiofxbad.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstaudiotestsrc.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstautodetect.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstcamerabin.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstcoreelements.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgsteffectv.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstipcpipeline.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstivorbisdec.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstopengl.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstopenh264.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstopensles.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgsttypefindfunctions.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstvideoconvert.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstvideofilter.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstvideotestsrc.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstvideorate.a
        ${gstreamer_path}/lib/gstreamer-1.0/libgstvorbis.a
        ${gstreamer_path}/lib/libandroid_support.a
        ${gstreamer_path}/lib/libavcodec.a
        ${gstreamer_path}/lib/libavutil.a
        ${gstreamer_path}/lib/libffi.a
        ${gstreamer_path}/lib/libgio-2.0.a
        ${gstreamer_path}/lib/libglib-2.0.a
        ${gstreamer_path}/lib/libgmodule-2.0.a
        ${gstreamer_path}/lib/libgobject-2.0.a
        ${gstreamer_path}/lib/libgraphene-1.0.a
        ${gstreamer_path}/lib/libgstadaptivedemux-1.0.a
        ${gstreamer_path}/lib/libgstapp-1.0.a
        ${gstreamer_path}/lib/libgstaudio-1.0.a
        ${gstreamer_path}/lib/libgstbadaudio-1.0.a
        ${gstreamer_path}/lib/libgstbase-1.0.a
        ${gstreamer_path}/lib/libgstbasecamerabinsrc-1.0.a
        ${gstreamer_path}/lib/libgstcodecparsers-1.0.a
        ${gstreamer_path}/lib/libgstcodecs-1.0.a
        ${gstreamer_path}/lib/libgstcontroller-1.0.a
        ${gstreamer_path}/lib/libgstfft-1.0.a
        ${gstreamer_path}/lib/libgstgl-1.0.a
        ${gstreamer_path}/lib/libgstpbutils-1.0.a
        ${gstreamer_path}/lib/libgstphotography-1.0.a
        ${gstreamer_path}/lib/libgstreamer-1.0.a
        ${gstreamer_path}/lib/libgsttag-1.0.a
        ${gstreamer_path}/lib/libgstvideo-1.0.a
        ${gstreamer_path}/lib/libiconv.a
        ${gstreamer_path}/lib/libintl.a
        ${gstreamer_path}/lib/libjpeg.a
        ${gstreamer_path}/lib/libopenh264.a
        ${gstreamer_path}/lib/liborc-0.4.a
        ${gstreamer_path}/lib/libpng16.a
        ${gstreamer_path}/lib/libvo-aacenc.a
        ${gstreamer_path}/lib/libvorbis.a
        ${gstreamer_path}/lib/libvorbisenc.a
        ${gstreamer_path}/lib/libvorbisfile.a
        ${gstreamer_path}/lib/libvorbisidec.a
        ${gstreamer_path}/lib/libz.a
        )

target_include_directories(gstreamer_android PUBLIC
        ${gstreamer_path}/include/gstreamer-1.0
        ${gstreamer_path}/include/glib-2.0
        ${gstreamer_path}/lib/glib-2.0/include
        )



###  App  ###


add_library(android_publisher SHARED
        native-lib.cpp
#        native-dds-lib.cpp
        VideoDDS.cpp
        VideoDDS.hpp
)

#idlcxx_generate(TARGET GeneratedIdl FILES ../common/VideoDDS.idl)

target_link_libraries(android_publisher
        CycloneDDS-CXX::ddscxx
        gstreamer_android
        android
#    GeneratedIdl
)
