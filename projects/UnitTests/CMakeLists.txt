include(FetchContent)
FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG 2fe3bd994b3189899d93f1d5a881e725e046fdc2 # release-1.8.1
)
# NOTE: A more concise way of expressing the following is available with
# CMake 3.14 or later
FetchContent_GetProperties(googletest)
if(NOT GoogleTest_POPULATED)
	FetchContent_Populate(googletest)
	add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()



include(GoogleTest)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include_directories("${googletest_SOURCE_DIR}/include")

add_executable(tests_automatic automatictests.cpp)
add_executable(tests_manual manualtests.cpp)

target_link_libraries(tests_automatic PRIVATE gtest)
gtest_discover_tests(tests_automatic)
target_link_libraries(tests_manual PRIVATE gtest)
gtest_discover_tests(tests_manual)
