# Finds OSPL includes and libraries
#
# Input:
#  OSPL_HOME - Base directory hint
#
# Output:
#  OSPL_INCLUDE_DIRS
#  OSPL_LIBRARIES
#  OSPL_FOUND
#
#
#  OSPL_IDLCPP_GEN - macro to generate source files from idl files
#   for usage see the MACRO definition below
#


##########
## Find include directories

# Find the path for includes
find_path(OSPL_INCLUDE_DIR_BASE
	NAMES dcps/C++/isocpp2/dds/dds.hpp
	PATHS "${OSPL_HOME}/include" "$ENV{OSPL_HOME}/include" /opt/ospl/include
)


set(OSPL_INCLUDE_DIRS
	${OSPL_INCLUDE_DIR_BASE}
	${OSPL_INCLUDE_DIR_BASE}/sys
	${OSPL_INCLUDE_DIR_BASE}/dcps/C++/isocpp2
	${OSPL_INCLUDE_DIR_BASE}/dcps/C++/SACPP
)

##########
## Find libraries

# Set the path where to search for the libraries
set(OSPL_LIB_SEARCH_PATHS
	"${OSPL_HOME}/lib"
	"$ENV{OSPL_HOME}/lib"
	/opt/ospl/lib
)

message(STATUS "Searching for OSPL libs in: \"${OSPL_LIB_SEARCH_PATHS}\"")


find_library(OSPL_LIBDDSKERNEL_RELEASE	NAMES ddskernel PATHS ${OSPL_LIB_SEARCH_PATHS})
find_library(OSPL_LIBDCPSSACPP_RELEASE  NAMES dcpssacpp PATHS ${OSPL_LIB_SEARCH_PATHS})
find_library(OSPL_LIBDCPSISOCPP2_RELEASE  NAMES dcpsisocpp2 PATHS  ${OSPL_LIB_SEARCH_PATHS})

find_library(OSPL_LIBDDSKERNEL_DEBUG  NAMES ddskernel PATHS ${OSPL_LIB_SEARCH_PATHS})
find_library(OSPL_LIBDCPSSACPP_DEBUG  NAMES dcpssacppd PATHS ${OSPL_LIB_SEARCH_PATHS})
find_library(OSPL_LIBDCPSISOCPP2_DEBUG  NAMES dcpsisocpp2d PATHS  ${OSPL_LIB_SEARCH_PATHS})


#set(OSPL_LIBRARIES
#	optimized ${OSPL_LIBDDSKERNEL_RELEASE}
#	debug ${OSPL_LIBDDSKERNEL_DEBUG}
#	optimized ${OSPL_LIBDCPSSACPP_RELEASE}
#	debug ${OSPL_LIBDCPSSACPP_DEBUG}
#	optimized ${OSPL_LIBDCPSISOCPP2_RELEASE}
#	debug ${OSPL_LIBDCPSISOCPP2_DEBUG}
#)
# TODO: Debug and Release version should be handled apropriate
# and not depended on operating system
if (WIN32)
	set(OSPL_LIBRARIES
		${OSPL_LIBDDSKERNEL_DEBUG}
		${OSPL_LIBDCPSSACPP_DEBUG}
		${OSPL_LIBDCPSISOCPP2_DEBUG}
	)
else(WIN32)
	set(OSPL_LIBRARIES
		${OSPL_LIBDDSKERNEL_RELEASE}
		${OSPL_LIBDCPSSACPP_RELEASE}
		${OSPL_LIBDCPSISOCPP2_RELEASE}
	)
endif(WIN32)

# Find the idlpp bin
find_path(OSPL_IDLPP_PATH
	NAMES idlpp idlpp.exe
	PATHS "${OSPL_HOME}" "$ENV{OSPL_HOME}" /opt/ospl
	PATH_SUFFIXES bin
)

message(STATUS "OSPL: idlpp found at: \"${OSPL_IDLPP_PATH}\"")

###########
## OSPL_IDLCPP_GEN Macro
# THIS DOES NOT EXCCUTE THE IDLCPP PROGRAM IF THE IDL FILE HAS CHANGED! THE PROJECT MUST BE REBUILD
# Creates a directory if it does not yet exist.
#
# Input:
#  OSPL_IDL_FILEDIR - Directory of the idl file
#  OSPL_IDL_FILEBASENAME - file must have file ending *.idl but must be given here without fileending. E.g. datainterface if the filename is (datainterface.idl)
#  OSPL_GENERATED_DIR - where the cpp, hpp and h files will be created
#
# Output:
#   The lists will be append by each call of OSPL_IDLCPP_GEN:
#  OSPL_GENERATED_SOURCES
#  OSPL_GENERATED_HEADERS
#   For convenience:
#  OSPL_GENERATED_DIR
#  OSPL_IDL_ABSOLUTE_FILEPATH
#
macro(OSPL_IDLCPP_GEN OSPL_IDL_FILEDIR OSPL_IDL_FILEBASENAME OSPL_GENERATED_DIR)

	file(MAKE_DIRECTORY ${OSPL_GENERATED_DIR})

	# Get abulute dir since the marking of the generate does not
	# work with some IDEs
	get_filename_component(OSPL_IDL_ABSOLUTE_FILEDIR ${OSPL_IDL_FILEDIR} REALPATH)

	set(OSPL_GENERATED_SOURCES_I
		${OSPL_GENERATED_DIR}/${OSPL_IDL_FILEBASENAME}.cpp
		${OSPL_GENERATED_DIR}/${OSPL_IDL_FILEBASENAME}SplDcps.cpp
	)
    set(OSPL_GENERATED_HEADERS_I
		${OSPL_GENERATED_DIR}/${OSPL_IDL_FILEBASENAME}.h
		${OSPL_GENERATED_DIR}/${OSPL_IDL_FILEBASENAME}SplDcps.h
		${OSPL_GENERATED_DIR}/${OSPL_IDL_FILEBASENAME}_DCPS.hpp
	)
    set(OSPL_IDL_ABSOLUTE_FILEPATH
		${OSPL_IDL_ABSOLUTE_FILEDIR}/${OSPL_IDL_FILEBASENAME}.idl
	)

    message(STATUS "Generating cpp files from \"${OSPL_IDL_ABSOLUTE_FILEPATH}\" to
		\"${OSPL_GENERATED_DIR}\"")

	set_source_files_properties(${OSPL_GENERATED_SOURCES_I} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${OSPL_GENERATED_HEADERS_I} PROPERTIES GENERATED TRUE)

	add_custom_command(OUTPUT ${OSPL_GENERATED_SOURCES_I} ${OSPL_GENERATED_HEADERS_I}
		COMMAND ${OSPL_IDLPP_PATH}/idlpp -S -l isocpp2 ${OSPL_IDL_ABSOLUTE_FILEPATH}
		COMMENT "Executing idlpp to generate the required C++ source files"
	)

    message(STATUS "Will generate source files: \"${OSPL_GENERATED_SOURCES_I}\"")
	message(STATUS "Will generate header files: \"${OSPL_GENERATED_HEADERS_I}\"")

	list(APPEND OSPL_GENERATED_SOURCES ${OSPL_GENERATED_SOURCES_I})
	list(APPEND OSPL_GENERATED_HEADERS ${OSPL_GENERATED_HEADERS_I})

	# Make the director available for use outside this macro
	set(OSPL_GENERATED_DIR ${OSPL_GENERATED_DIR})


endmacro(OSPL_IDLCPP_GEN)



# handle the QUIETLY and REQUIRED arguments and set OSPL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSPL DEFAULT_MSG
	OSPL_LIBRARIES OSPL_INCLUDE_DIRS
)

MARK_AS_ADVANCED(
	OSPL_LIBDDSKERNEL_RELEASE
	OSPL_LIBDCPSSACPP_RELEASE
	OSPL_LIBDCPSISOCPP2_RELEASE
	OSPL_LIBDDSKERNEL_DEBUG
	OSPL_LIBDCPSSACPP_DEBUG
	OSPL_LIBDCPSISOCPP2_DEBUG
)
