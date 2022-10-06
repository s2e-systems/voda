cmake_minimum_required(VERSION 3.18.1)

project("android_publisher")

set(CycloneDDS_DIR C:/Users/StefanKimmer/workcopies/build-cyclonedds/android_${ANDROID_ABI}/install/lib/cmake/CycloneDDS)
#find_package(CycloneDDS REQUIRED)
set(CycloneDDS-CXX_DIR C:/Users/StefanKimmer/workcopies/build-cyclonedds-cxx/android_${ANDROID_ABI}/install/lib/cmake/CycloneDDS-CXX)
find_package(CycloneDDS-CXX REQUIRED)

add_library(android_publisher SHARED
        native-dds-lib.cpp
        VideoDDS.cpp
        VideoDDS.hpp
)

#idlcxx_generate(TARGET GeneratedIdl FILES ../common/VideoDDS.idl)

target_link_libraries(android_publisher
#    CycloneDDS::ddsc
    CycloneDDS-CXX::ddscxx
#    GeneratedIdl
)
