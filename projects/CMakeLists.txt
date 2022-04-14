cmake_minimum_required(VERSION 3.14.0)
project(VoDA LANGUAGES CXX)

option(BUILD_UNIT_TESTS "Weather to build unit tests (requires GoogleTest)" OFF)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(GLib MODULE REQUIRED COMPONENTS glib gobject)
find_package(GStreamer MODULE REQUIRED COMPONENTS gstreamer base app)
find_package(CycloneDDS-CXX REQUIRED)
idlcxx_generate(TARGET GeneratedIdl FILES common/VideoDDS.idl)


include(CMakePrintHelpers)
cmake_print_properties(TARGETS GLib::glib
  PROPERTIES IMPORTED_LOCATION INTERFACE_INCLUDE_DIRECTORIES
)
cmake_print_properties(TARGETS GStreamer::gstreamer GStreamer::base GStreamer::app GStreamer::gl
  PROPERTIES IMPORTED_LOCATION INTERFACE_INCLUDE_DIRECTORIES
)
cmake_print_properties(TARGETS CycloneDDS-CXX::ddscxx
  PROPERTIES LOCATION INTERFACE_INCLUDE_DIRECTORIES
)

add_executable(VideoDDSSubscriber
  subscriber/main.cpp
  subscriber/videoddssubscriber.h
  subscriber/videoddssubscriber.cpp
  subscriber/videolistener.h
  subscriber/videolistener.cpp
)
target_include_directories(VideoDDSSubscriber PRIVATE
  VideoDDSSubscriber
)
target_link_libraries(VideoDDSSubscriber PRIVATE
  GLib::glib
  GLib::gobject
  GStreamer::gstreamer
  GStreamer::base
  GStreamer::app
  CycloneDDS-CXX::ddscxx
  GeneratedIdl
)
target_compile_features(VideoDDSSubscriber PRIVATE cxx_std_17)


add_executable(VideoDDSPublisher
  publisher/main.cpp
  publisher/videoddspublisher.h
  publisher/videoddspublisher.cpp
  common/cameracapabilities.h
  common/cameracapabilities.cpp
  common/elements.h
  common/elements.cpp
)
target_include_directories(VideoDDSPublisher PRIVATE
  VideoDDSPublisher
  common
)
target_link_libraries(VideoDDSPublisher PRIVATE
  GLib::glib
  GLib::gobject
  GStreamer::gstreamer
  GStreamer::base
  GStreamer::app
  CycloneDDS-CXX::ddscxx
  GeneratedIdl
)
target_compile_features(VideoDDSSubscriber PRIVATE cxx_std_17)


if (${BUILD_UNIT_TESTS})

  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
    GIT_SHALLOW true
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  set(VODA_UNITTEST_DATA_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/unit_test/data")
  configure_file(unit_test/config.h.in config.h)


  add_executable(run_unit_tests
    unit_test/mainunittests.cpp
    common/cameracapabilities.h
    common/cameracapabilities.cpp
    common/cameracapabilities_tests.cpp
    common/elements.h
    common/elements.cpp
    common/elements_tests.cpp
    subscriber/videoddssubscriber.cpp
    subscriber/videoddssubscriber_tests.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
  )
  target_link_libraries(run_unit_tests PRIVATE
    gtest
    GLib::glib
    GLib::gobject
    GStreamer::gstreamer
    GStreamer::app
  )
  target_include_directories(run_unit_tests PRIVATE
    common
    ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_compile_features(run_unit_tests PRIVATE cxx_std_17)


  add_executable(unit_tests_recorder
    unit_test/mainrecorder.cpp
  )
  target_link_libraries(unit_tests_recorder PRIVATE
    GLib::glib
    GLib::gobject
    GStreamer::gstreamer
  )

endif()
