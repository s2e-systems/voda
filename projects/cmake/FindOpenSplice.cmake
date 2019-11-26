
set(OpenSplice_ROOT_SEARCH_PATHS "${OSPL_HOME}" "$ENV{OSPL_HOME}" /opt/ospl /opt/ADLINK)

# Find the path for includes
find_path(OpenSplice_INCLUDE_DIR_BASE
	NAMES dcps/C++/isocpp2/dds/dds.hpp
	PATHS ${OpenSplice_ROOT_SEARCH_PATHS}
	PATH_SUFFIXES include
)

# Find libraries
find_library(OpenSplice_ddskernel_LIBRARY  NAMES ddskernel  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpssacpp_LIBRARY  NAMES dcpssacpp  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpssacpp_LIBRARY_DEBUG  NAMES dcpssacppd  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpsisocpp2_LIBRARY  NAMES dcpsisocpp2  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpsisocpp2_LIBRARY_DEBUG  NAMES dcpsisocpp2d  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)

find_library(OpenSplice_ddskernel_IMPLIB  NAMES ddskernel.lib  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpssacpp_IMPLIB  NAMES dcpssacpp.lib  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpssacpp_IMPLIB_DEBUG  NAMES dcpssacppd.lib  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpsisocpp2_IMPLIB  NAMES dcpsisocpp2.lib  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)
find_library(OpenSplice_dcpsisocpp2_IMPLIB_DEBUG  NAMES dcpsisocpp2d.lib  PATHS ${OpenSplice_ROOT_SEARCH_PATHS} PATH_SUFFIXES lib)

# Find the idlpp executable
find_file(OpenSplice_IDLPP_PATH
	NAMES idlpp idlpp.exe
	PATHS ${OpenSplice_ROOT_SEARCH_PATHS}
	PATH_SUFFIXES bin
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSplice
	REQUIRED_VARS OpenSplice_ddskernel_LIBRARY
	HANDLE_COMPONENTS
	)

if(OpenSplice_FOUND)
	add_library(OpenSplice::ddskernel SHARED IMPORTED)
	set_target_properties(OpenSplice::ddskernel PROPERTIES
		IMPORTED_LOCATION ${OpenSplice_ddskernel_LIBRARY}
		IMPORTED_LOCATION_DEBUG ${OpenSplice_ddskernel_LIBRARY}
		IMPORTED_LOCATION_RELEASE ${OpenSplice_ddskernel_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES "${OpenSplice_INCLUDE_DIR_BASE};${OpenSplice_INCLUDE_DIR_BASE}/sys"
		IMPORTED_IMPLIB ${OpenSplice_ddskernel_IMPLIB}
		IMPORTED_NO_SONAME TRUE
	)

	add_library(OpenSplice::dcpssacpp SHARED IMPORTED)
	set_target_properties(OpenSplice::dcpssacpp PROPERTIES
		IMPORTED_LOCATION ${OpenSplice_dcpssacpp_LIBRARY}
		IMPORTED_LOCATION_DEBUG ${OpenSplice_dcpssacpp_LIBRARY_DEBUG}
		INTERFACE_INCLUDE_DIRECTORIES "${OpenSplice_INCLUDE_DIR_BASE}/dcps/C++/SACPP"
		IMPORTED_IMPLIB ${OpenSplice_dcpssacpp_IMPLIB}
		IMPORTED_IMPLIB_DEBUG ${OpenSplice_dcpssacpp_IMPLIB_DEBUG}
		IMPORTED_NO_SONAME TRUE
	)

	add_library(OpenSplice::dcpsisocpp2 SHARED IMPORTED)
	set_target_properties(OpenSplice::dcpsisocpp2 PROPERTIES
		IMPORTED_LOCATION ${OpenSplice_dcpsisocpp2_LIBRARY}
		IMPORTED_LOCATION_DEBUG ${OpenSplice_dcpsisocpp2_LIBRARY_DEBUG}
		INTERFACE_INCLUDE_DIRECTORIES "${OpenSplice_INCLUDE_DIR_BASE}/dcps/C++/isocpp2"
		IMPORTED_IMPLIB ${OpenSplice_dcpsisocpp2_IMPLIB}
		IMPORTED_IMPLIB_DEBUG ${OpenSplice_dcpsisocpp2_IMPLIB_DEBUG}
		IMPORTED_NO_SONAME TRUE
	)

endif()






include(CMakePrintHelpers)
cmake_print_properties(TARGETS OpenSplice::ddskernel OpenSplice::dcpssacpp OpenSplice::dcpsisocpp2
	PROPERTIES IMPORTED_LOCATION IMPORTED_LOCATION_DEBUG INTERFACE_INCLUDE_DIRECTORIES)



###########
## OpenSplice_IDLCPP_GEN Macro
# Creates a directory if it does not yet exist.
#
# Input:
#  OpenSplice_IDL_FILEPATH - Path of the idl file
#  OpenSplice_GENERATED_DIR - where the cpp, hpp and h files will be created
#
# Output:
#   The lists will be append by each call of OpenSplice_IDLCPP_GEN:
#  OpenSplice_GENERATED_SOURCES
#  OpenSplice_GENERATED_HEADERS
#
macro(OpenSplice_IDLCPP_GEN OpenSplice_IDL_FILEPATH OpenSplice_GENERATED_DIR)

	if(NOT OpenSplice_IDLPP_PATH)
		message(FATAL_ERROR "OpenSplice_IDLPP_PATH must be found")
	endif()

	file(MAKE_DIRECTORY ${OpenSplice_GENERATED_DIR})

	# Get abulute dir since the marking of the generate does not
	# work with some IDEs
	get_filename_component(OpenSplice_IDL_ABSOLUTE_FILEPATH ${OpenSplice_IDL_FILEPATH} REALPATH)
	# Note the following should be NAME_WLE instead of NAME_WE, but NAME_WLE was introduced only in CMake version 3.14
	# As such idl filenames including multiple dots in the filename will fail here
	get_filename_component(OpenSplice_IDL_FILEBASENAME  ${OpenSplice_IDL_FILEPATH} NAME_WE)

	set(OpenSplice_GENERATED_SOURCES_I
		${OpenSplice_GENERATED_DIR}/${OpenSplice_IDL_FILEBASENAME}.cpp
		${OpenSplice_GENERATED_DIR}/${OpenSplice_IDL_FILEBASENAME}SplDcps.cpp
	)
	set(OpenSplice_GENERATED_HEADERS_I
		${OpenSplice_GENERATED_DIR}/${OpenSplice_IDL_FILEBASENAME}.h
		${OpenSplice_GENERATED_DIR}/${OpenSplice_IDL_FILEBASENAME}SplDcps.h
		${OpenSplice_GENERATED_DIR}/${OpenSplice_IDL_FILEBASENAME}_DCPS.hpp
	)

	message(STATUS "Generating cpp files from \"${OpenSplice_IDL_ABSOLUTE_FILEPATH}\" to
		\"${OpenSplice_GENERATED_DIR}\"")

	add_custom_command(OUTPUT ${OpenSplice_GENERATED_SOURCES_I} ${OpenSplice_GENERATED_HEADERS_I}
		COMMAND ${OpenSplice_IDLPP_PATH} -S -l isocpp2 ${OpenSplice_IDL_ABSOLUTE_FILEPATH}
		DEPENDS ${OpenSplice_IDL_ABSOLUTE_FILEPATH}
		COMMENT "Executing idlpp to generate the required C++ source files"
	)

	message(STATUS "Will generate source files: \"${OpenSplice_GENERATED_SOURCES_I}\"")
	message(STATUS "Will generate header files: \"${OpenSplice_GENERATED_HEADERS_I}\"")

	list(APPEND OpenSplice_GENERATED_SOURCES ${OpenSplice_GENERATED_SOURCES_I})
	list(APPEND OpenSplice_GENERATED_HEADERS ${OpenSplice_GENERATED_HEADERS_I})

	# Make the director available for use outside this macro
	set(OpenSplice_GENERATED_DIR ${OpenSplice_GENERATED_DIR})


endmacro(OpenSplice_IDLCPP_GEN)

