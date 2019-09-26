cmake_minimum_required(VERSION 3.1.0)
project(VoDA)

option(BUILD_UNIT_TESTS "Weather to build unit tests (requires GoogleTest)" OFF)

add_subdirectory(VideoDDSPublisher)
add_subdirectory(VideoDDSSubscriber)

if (BUILD_UNIT_TESTS)
	add_subdirectory(UnitTests)
endif()
