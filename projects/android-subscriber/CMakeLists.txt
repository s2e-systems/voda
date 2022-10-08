cmake_minimum_required(VERSION 3.21)

project(android-subscriber)

find_package(CycloneDDS-CXX REQUIRED)

add_executable(android-subscriber
    main.cpp
    VideoDDS.cpp
)

# idlcxx_generate(TARGET GeneratedIdl FILES ../common/VideoDDS.idl)

target_link_libraries(android-subscriber PRIVATE
  CycloneDDS-CXX::ddscxx
  # GeneratedIdl
)

# target_include_directories(android-subscriber PRIVATE
#   ../build
# )