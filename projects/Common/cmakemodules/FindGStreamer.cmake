# Try to find GStreamer and its plugins
#
# INPUT:
#	64BIT or GSTREAMER_64BIT - Weather to search for 64bit version (TRUE) or 32bit version (FALSE)
#		Does select env vars GSTREAMER_1_0_ROOT_X86_64 or GSTREAMER_1_0_ROOT_X86 in Windows
#	GStreamer_ROOT_PATH - Add to search paths
#
# OUTPUT:
#
#  GStreamer_FOUND - system has base GStreamer
#  GStreamer_INCLUDE_DIRS - All combined (core, plugins, ...) GStreamer include directories
#  GStreamer_LIBRARIES - All combined (core, plugins, ...)
#  GStreamer_DIR - GStreamer base dir
#  GStreamer_VERSION - The version of the found GStreamer
#
#
# Optionally, the COMPONENTS keyword can be passed to find_package()
# and GStreamer plugins can be looked for.  Currently, the following
# plugins can be searched, and they define the following variables if
# found:
#
#  gobject:	GStreamer_GOBJECT_INCLUDE_DIR and GStreamer_GOBJECT_LIBRARY
#  glib:	GStreamer_GLIB_INCLUDE_DIR and GStreamer_GLIB_LIBRARY
#
#  base:	GStreamer_BASE_INCLUDE_DIR and GStreamer_BASE_LIBRARY
#  app:		GStreamer_APP_INCLUDE_DIR and GStreamer_APP_LIBRARY
#  audio:	GStreamer_AUDIO_INCLUDE_DIR and GStreamer_AUDIO_LIBRARY
#  fft:		GStreamer_FFT_INCLUDE_DIR and GStreamer_FFT_LIBRARY
#  pbutils:	GStreamer_PBUTILS_INCLUDE_DIR and GStreamer_PBUTILS_LIBRARY
#  video:	GStreamer_VIDEO_INCLUDE_DIR and GStreamer_VIDEO_LIBRARY
#  codecparsers:	GStreamer_CODECPARSERS_INCLUDE_DIR and GStreamer_CODECPARSERS_LIBRARY

if(UNIX)
	find_package(PkgConfig)
endif()

list(APPEND GStreamer_ROOT_SEARCH_PATHS ${GStreamer_ROOT_PATH})

if(WIN32)
	if(GStreamer_64BIT OR 64BIT)
		list(APPEND GStreamer_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86_64})
	else()
		list(APPEND GStreamer_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86})
	endif()
endif()




# find_gstreamer_component(componentPrefix header_filename libray_name [pkgconfig_name])
#
# Helper macro to find a GStreamer component (or GStreamer itself)
#   componentPrefix is prepended to the _INCLUDE_DIR and _LIBRARY variables (eg. "GStreamer_AUDIO")
# Note: For WIN32 the extra argument pkgconfig_name will be ignored
#
# OUTPUT:
#   componentPrefix - <component_prefix>_INCLUDE_DIR
#					   <component_prefix>_LIBRARY
# INPUT:
#   headerFileName - base filename of the primary header file (will be added to the <component_prefix>_INCLUDE_DIR)
#   librayFileName - base filename of the library (with out postfix .so or .lib and prefix lib as cmake standard serach)
#				will be added to the  <component_prefix>_LIBRARY
#   pkgconfigName - optional, only for linux version
#			component's pkg-config name (eg. "gstreamer-1.0", or "gstreamer-video-1.0").
#			note that this is unfortunatly not the always the same as the library name
if(WIN32)
	macro(find_gstreamer_component   componentPrefix  headerFileName  libraryFileName)

		find_path(${componentPrefix}_INCLUDE_DIR
			NAMES ${headerFileName}
			PATHS ${GStreamer_ROOT_SEARCH_PATHS}
			PATH_SUFFIXES include include/gstreamer-1.0 include/glib-2.0 include/glib-2.0/gobject
		)

		find_library(${componentPrefix}_LIBRARY
			NAMES ${libraryFileName}
			PATHS ${GStreamer_ROOT_SEARCH_PATHS}
			PATH_SUFFIXES lib
		)
	endmacro()
else(WIN32)
	macro(find_gstreamer_component   componentPrefix  headerFileName  libraryFileName  pkgconfigName)
		pkg_check_modules(PC_${componentPrefix} QUIET ${pkgconfigName})
		find_path(${componentPrefix}_INCLUDE_DIR
			NAMES ${headerFileName}
			PATHS ${PC_${componentPrefix}_INCLUDE_DIRS} ${PC_${componentPrefix}_INCLUDEDIR}
			PATH_SUFFIXES gstreamer-1.0 gobject
		)

		find_library(${componentPrefix}_LIBRARY
			NAMES ${libraryFileName}
			HINTS ${PC_${componentPrefix}_LIBRARY_DIRS} ${PC_${componentPrefix}_LIBDIR}
		)
	endmacro()

endif(WIN32)



############################
# Find GStreamer core


# Find headers and libraries
find_gstreamer_component(GStreamer_CORE gst/gst.h gstreamer-1.0 gstreamer-1.0)
# TODO: check if following line is neccessary
#list(APPEND GStreamer_CORE_INCLUDE_DIR ${GStreamer_CORE_INCLUDE_DIR}/include)

if(WIN32)
	# Find the config header file of the gtreamer lib. Which may be for some odd reason in the library dir
	find_path(GStreamer_CONFIG_INCLUDE_DIR
		NAMES gst/gstconfig.h
		PATHS ${GStreamer_ROOT_SEARCH_PATHS}
		PATH_SUFFIXES lib/gstreamer-1.0/include include/gstreamer-1.0
	)
endif(WIN32)


# Addidiotnal output for the user
set(GStreamer_DIR ${GStreamer_CORE_INCLUDE_DIR})


##############
# Find GLib

find_gstreamer_component(GStreamer_GLIB   glib.h glib-2.0 glib-2.0)

# Find the config header file of the glib. Which is for some odd reason in the library dir (in win and linux)
if(WIN32)
	find_path(GStreamer_GLIB_CONFIG_INCLUDE_DIR
		NAMES glibconfig.h
		PATHS ${GStreamer_ROOT_SEARCH_PATHS}
		PATH_SUFFIXES lib/glib-2.0/include
	)
else(WIN32)
	find_path(GStreamer_GLIB_CONFIG_INCLUDE_DIR
		NAMES glibconfig.h
		HINTS ${PC_GStreamer_GLIB_LIBRARY_DIR} ${PC_GStreamer_GLIB_LIBDIR}
		PATHS /usr/lib/x86_64-linux-gnu
		PATH_SUFFIXES glib-2.0/include
	)
endif(WIN32)

list(APPEND GStreamer_GLIB_INCLUDE_DIR ${GStreamer_GLIB_CONFIG_INCLUDE_DIR})



##############
# Find GObject

find_gstreamer_component(GStreamer_GOBJECT   gobject.h gobject-2.0 gobject-2.0)


#############################
# Find GStreamer plugins

find_gstreamer_component(GStreamer_BASE   gst/base/gstbasesink.h gstbase-1.0 gstreamer-base-1.0)
find_gstreamer_component(GStreamer_APP   gst/app/gstappsink.h gstapp-1.0 gstreamer-app-1.0)
find_gstreamer_component(GStreamer_AUDIO   gst/audio/audio.h gstaudio-1.0 gstreamer-audio-1.0)
find_gstreamer_component(GStreamer_RIFF   gst/riff/riff.h gstriff-1.0 gstreamer-riff-1.0)
find_gstreamer_component(GStreamer_VIDEO   gst/video/video.h gstvideo-1.0 gstreamer-video-1.0)
find_gstreamer_component(GStreamer_FFT   gst/fft/fft.h gstfft-1.0 gstreamer-fft-1.0)
find_gstreamer_component(GStreamer_PBUTILS   gst/pbutils/pbutils.h gstpbutils-1.0 gstreamer-pbutils-1.0)
find_gstreamer_component(GStreamer_CODECPARSERS   gst/codecparsers/gsth264parser.h gstcodecparsers-1.0 gstcodecparsers-1.0)



###
# Generate uppercase version of the input components

foreach (component ${GStreamer_FIND_COMPONENTS})
	string(TOUPPER ${component} componentUpperCase)
	list(APPEND GStreamer_FIND_COMPONENTS_UPPERCASE ${componentUpperCase})
endforeach ()


#####################
# Add everything to one var
list(APPEND GStreamer_INCLUDE_DIRS   ${GStreamer_CORE_INCLUDE_DIR})
list(APPEND GStreamer_INCLUDE_DIRS  ${GStreamer_CONFIG_INCLUDE_DIR})
list(APPEND GStreamer_LIBRARIES   ${GStreamer_CORE_LIBRARY})
foreach (component ${GStreamer_FIND_COMPONENTS_UPPERCASE})
	list(APPEND GStreamer_INCLUDE_DIRS   ${GStreamer_${component}_INCLUDE_DIR})
	list(APPEND GStreamer_LIBRARIES   ${GStreamer_${component}_LIBRARY})
endforeach ()



#############################
# Find and check GStreamer version

if (GStreamer_CORE_INCLUDE_DIR)
	if (EXISTS "${GStreamer_CORE_INCLUDE_DIR}/gst/gstversion.h")
		file(READ "${GStreamer_CORE_INCLUDE_DIR}/gst/gstversion.h" GStreamer_VERSION_CONTENTS)

		string(REGEX MATCH "#define +GST_VERSION_MAJOR +\\(([0-9]+)\\)" _dummy "${GStreamer_VERSION_CONTENTS}")
		set(GStreamer_VERSION_MAJOR "${CMAKE_MATCH_1}")

		string(REGEX MATCH "#define +GST_VERSION_MINOR +\\(([0-9]+)\\)" _dummy "${GStreamer_VERSION_CONTENTS}")
		set(GStreamer_VERSION_MINOR "${CMAKE_MATCH_1}")

		string(REGEX MATCH "#define +GST_VERSION_MICRO +\\(([0-9]+)\\)" _dummy "${GStreamer_VERSION_CONTENTS}")
		set(GStreamer_VERSION_MICRO "${CMAKE_MATCH_1}")

		set(GStreamer_VERSION "${GStreamer_VERSION_MAJOR}.${GStreamer_VERSION_MINOR}.${GStreamer_VERSION_MICRO}")

	endif ()
endif (GStreamer_CORE_INCLUDE_DIR)



if ("${GStreamer_FIND_VERSION}" VERSION_GREATER "${GStreamer_VERSION}")
	message(FATAL_ERROR "Required version (${GStreamer_FIND_VERSION}) is higher than found version (${GStreamer_VERSION})")
endif ()




##############################
# Process the COMPONENTS passed to FIND_PACKAGE

set(GStreamerRequiredVars  GStreamer_INCLUDE_DIR GStreamer_LIBRARIES GStreamer_VERSION)

## Append the required variable list by the ones passed as components
# TODO: may be replaced by using the HANDLE_COMPONENTS agrument in find_package_handle_standard_args
# In the variable GStreamer_FIND_COMPONENTS all components as stored as they were passed by the user
#  This is a feature of find_package_handle_standard_args
foreach (component ${GStreamer_FIND_COMPONENTS_UPPERCASE})
	list(APPEND GStreamerRequiredVars   GStreamer_${component}_INCLUDE_DIR  GStreamer_${component}_LIBRARIES)
endforeach ()

include(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(<PackageName>
#  [FOUND_VAR <result-var>]
#  [REQUIRED_VARS <required-var>...]
#  [VERSION_VAR <version-var>]
#  [HANDLE_COMPONENTS]
#  [CONFIG_MODE]
#  [FAIL_MESSAGE <custom-failure-message>]
#  )

find_package_handle_standard_args(GStreamer
	REQUIRED_VARS GStreamerRequiredVars
	VERSION_VAR GStreamer_VERSION
	)

# TODO: add mark_as_advanced here


#######
## Message the results

message(STATUS "FindGStreamer has the following to say:")

message(STATUS "  Searched in system paths and in \"${GStreamer_ROOT_SEARCH_PATHS}\"")

message(STATUS "Search results:")
message(STATUS "Found GStreamer version \"${GStreamer_VERSION}\"")
message(STATUS "  GStreamer_CORE_INCLUDE_DIR: \"${GStreamer_CORE_INCLUDE_DIR}\"")
message(STATUS "  GStreamer_CORE_LIBRARY: \"${GStreamer_CORE_LIBRARY}\"")
foreach (component ${GStreamer_FIND_COMPONENTS_UPPERCASE})
	message(STATUS "  GStreamer_${component}_INCLUDE_DIR: \"${GStreamer_${component}_INCLUDE_DIR}\"")
	message(STATUS "  GStreamer_${component}_LIBRARY: \"${GStreamer_${component}_LIBRARY}\"")
endforeach ()

message(STATUS "  GStreamer_INCLUDE_DIRS: \"${GStreamer_INCLUDE_DIRS}\"")
message(STATUS "  GStreamer_LIBRARIES: \"${GStreamer_LIBRARIES}\"")

