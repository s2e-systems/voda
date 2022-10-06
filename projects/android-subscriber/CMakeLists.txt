cmake_minimum_required(VERSION 3.21)

project(android-subscriber)

find_package(CycloneDDS REQUIRED)

add_executable(android-subscriber
    main.cpp
)

target_link_libraries(android-subscriber PRIVATE
  CycloneDDS::ddsc
)