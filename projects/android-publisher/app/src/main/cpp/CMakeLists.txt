# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.18.1)

project("mygstreamerbuild")

set(CycloneDDS_DIR C:/Users/StefanKimmer/workcopies/build-cyclonedds/android_${ANDROID_ABI}/install/lib/cmake/CycloneDDS)
find_package(CycloneDDS REQUIRED)
set(CycloneDDS-CXX_DIR C:/Users/StefanKimmer/workcopies/build-cyclonedds-cxx/android_${ANDROID_ABI}/install/lib/cmake/CycloneDDS-CXX)
find_package(CycloneDDS-CXX REQUIRED)


set(gstreamer_abi_directory_name ${ANDROID_ABI})
if (${ANDROID_ABI} STREQUAL armeabi-v7a)
    set(gstreamer_abi_directory_name armv7)
endif()
if (${ANDROID_ABI} STREQUAL arm64-v8a)
    set(gstreamer_abi_directory_name arm64)
endif()

set(gstreamer_path C:/gstreamer/1.0/android-universal-1.20.3)


set(PLUGINS_DECLARATION "
    GST_PLUGIN_STATIC_DECLARE(coreelements);
    GST_PLUGIN_STATIC_DECLARE(videofilter);
    GST_PLUGIN_STATIC_DECLARE(autodetect);
    GST_PLUGIN_STATIC_DECLARE(videoconvert);
    GST_PLUGIN_STATIC_DECLARE(videotestsrc);
    GST_PLUGIN_STATIC_DECLARE(effectv);
    GST_PLUGIN_STATIC_DECLARE(opensles);
    GST_PLUGIN_STATIC_DECLARE(ipcpipeline);
    GST_PLUGIN_STATIC_DECLARE(opengl);
    GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
    GST_PLUGIN_STATIC_DECLARE(audiotestsrc);
    GST_PLUGIN_STATIC_DECLARE(audiofx);
    GST_PLUGIN_STATIC_DECLARE(audiofxbad);
    GST_PLUGIN_STATIC_DECLARE(openh264);
")
set(PLUGINS_REGISTRATION "
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(videofilter);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(effectv);
    GST_PLUGIN_STATIC_REGISTER(opensles);
    GST_PLUGIN_STATIC_REGISTER(ipcpipeline);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(audiotestsrc);
    GST_PLUGIN_STATIC_REGISTER(audiofx);
    GST_PLUGIN_STATIC_REGISTER(audiofxbad);
    GST_PLUGIN_STATIC_REGISTER(openh264);
")

set(G_IO_MODULES_DECLARE "")
set(G_IO_MODULES_LOAD "")



configure_file(
    ${gstreamer_path}/${gstreamer_abi_directory_name}/share/gst-android/ndk-build/gstreamer_android-1.0.c.in gstreamer_android-1.0.c
)

set_source_files_properties(gstreamer_android-1.0.c
        PROPERTIES GENERATED TRUE)

find_library(log-lib log)

set(nativewindow_abi_directory_name ${ANDROID_ABI})
if (${ANDROID_ABI} STREQUAL armeabi-v7a)
    set(nativewindow_abi_directory_name i686-linux-android)
endif()
if (${ANDROID_ABI} STREQUAL arm64-v8a)
    set(nativewindow_abi_directory_name aarch64-linux-android)
endif()
if (${ANDROID_ABI} STREQUAL x86)
    set(nativewindow_abi_directory_name arm-linux-androideabi)
endif()
if (${ANDROID_ABI} STREQUAL x86_64)
    set(nativewindow_abi_directory_name x86_64-linux-android)
endif()

add_library(mygstreamerbuild SHARED
        VideoDDS.cpp
        native-lib.cpp)

target_link_libraries(mygstreamerbuild
        gstreamer_android
        android
        CycloneDDS::ddsc
        CycloneDDS-CXX::ddscxx
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstvideo-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libandroid_support.a
        )

target_compile_features(mygstreamerbuild PRIVATE cxx_std_17)

add_library(gstreamer_android SHARED
        gstreamer_android-1.0.c
        dummy.cpp
        )

target_link_libraries(gstreamer_android
        ${log-lib}
        EGL
        OpenSLES
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libopenh264.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstopenh264.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstfft-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstaudiofx.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstaudiofxbad.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstcamerabin.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstadaptivedemux-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libvo-aacenc.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstaudiotestsrc.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgsttag-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstivorbisdec.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstvorbis.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libvorbis.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libvorbisenc.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libvorbisfile.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libvorbisidec.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstpbutils-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libavutil.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstcodecs-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libavcodec.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstcodecparsers-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgsttypefindfunctions.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstcontroller-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libpng16.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libjpeg.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgraphene-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstipcpipeline.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstopengl.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstopensles.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstphotography-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstandroidmedia.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libandroid_support.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstaudio-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstbadaudio-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstgl-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstbase-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstvideo-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstcoreelements.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/liborc-0.4.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstvideofilter.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstvideoconvert.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstautodetect.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgstvideotestsrc.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/gstreamer-1.0/libgsteffectv.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgstreamer-1.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgmodule-2.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgobject-2.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libffi.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libintl.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libgio-2.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libglib-2.0.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libiconv.a
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/libz.a
        )

target_include_directories(gstreamer_android PUBLIC
        ${gstreamer_path}/${gstreamer_abi_directory_name}/include/gstreamer-1.0
        ${gstreamer_path}/${gstreamer_abi_directory_name}/include/glib-2.0
        ${gstreamer_path}/${gstreamer_abi_directory_name}/lib/glib-2.0/include
        )
