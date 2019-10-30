if(UNIX)
	find_package(PkgConfig)
endif()

list(APPEND GStreamer_ROOT_SEARCH_PATHS ${GStreamer_ROOT_PATH})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(GStreamer_64BIT True)
endif()

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
		if(${componentPrefix}_INCLUDE_DIR AND ${componentPrefix}_LIBRARY)
			set(${componentPrefix}_FOUND TRUE)
		endif()
	endmacro()
else()
	macro(find_gstreamer_component   componentPrefix  headerFileName  libraryFileName  pkgconfigName)
		pkg_check_modules(PC_${componentPrefix} QUIET ${pkgconfigName})
		find_path(${componentPrefix}_INCLUDE_DIR
			NAMES ${headerFileName}
			PATHS ${PC_${componentPrefix}_INCLUDE_DIRS}
			PATH_SUFFIXES gstreamer-1.0 gobject
		)

		find_library(${componentPrefix}_LIBRARY
			NAMES ${libraryFileName}
			HINTS ${PC_${componentPrefix}_LIBRARY_DIRS}
		)
		if(${componentPrefix}_INCLUDE_DIR AND ${componentPrefix}_LIBRARY)
			set(${componentPrefix}_FOUND TRUE)
		endif()
	endmacro()

endif()



############################
# Find GStreamer core


# Find headers and libraries
find_gstreamer_component(GStreamer_core gst/gst.h gstreamer-1.0 gstreamer-1.0)

if(WIN32)
	# Find the config header file of the gtreamer lib. Which may be for some odd reason in the library dir
	find_path(GStreamer_core_CONFIG_INCLUDE_DIR
		NAMES gst/gstconfig.h
		PATHS ${GStreamer_ROOT_SEARCH_PATHS}
		PATH_SUFFIXES lib/gstreamer-1.0/include include/gstreamer-1.0
	)
endif(WIN32)



##############
# Find GLib

find_gstreamer_component(GStreamer_glib   glib.h glib-2.0 glib-2.0)

# Find the config header file of the glib. Which is for some odd reason in the library dir (in win and linux)
if(WIN32)
	find_path(GStreamer_glib_CONFIG_INCLUDE_DIR
		NAMES glibconfig.h
		PATHS ${GStreamer_ROOT_SEARCH_PATHS}
		PATH_SUFFIXES lib/glib-2.0/include
	)
else()
	find_path(GStreamer_glib_CONFIG_INCLUDE_DIR
		NAMES glibconfig.h
		HINTS ${PC_GStreamer_glib_LIBRARY_DIR} ${PC_GStreamer_glib_LIBDIR}
		PATHS /usr/lib/x86_64-linux-gnu
		PATH_SUFFIXES glib-2.0/include
	)
endif()


##############
# Find GObject

find_gstreamer_component(GStreamer_gobject   gobject.h gobject-2.0 gobject-2.0)


#############################
# Find GStreamer plugins

find_gstreamer_component(GStreamer_base   gst/base/gstbasesink.h gstbase-1.0 gstreamer-base-1.0)
find_gstreamer_component(GStreamer_app   gst/app/gstappsink.h gstapp-1.0 gstreamer-app-1.0)
find_gstreamer_component(GStreamer_video   gst/video/video.h gstvideo-1.0 gstreamer-video-1.0)
find_gstreamer_component(GStreamer_codecparsers   gst/codecparsers/gsth264parser.h gstcodecparsers-1.0 gstcodecparsers-1.0)

#message(STATUS "++  ${GStreamer_base_LIBRARY}  ${GStreamer_base_INCLUDE_DIR}")

#############################
# Find and check GStreamer version

if(GStreamer_core_INCLUDE_DIR)
	if(EXISTS "${GStreamer_core_INCLUDE_DIR}/gst/gstversion.h")
		file(READ "${GStreamer_core_INCLUDE_DIR}/gst/gstversion.h" GStreamer_VERSION_CONTENTS)

		string(REGEX MATCH "#define +GST_VERSION_MAJOR +\\(([0-9]+)\\)" _dummy "${GStreamer_VERSION_CONTENTS}")
		set(GStreamer_VERSION_MAJOR "${CMAKE_MATCH_1}")

		string(REGEX MATCH "#define +GST_VERSION_MINOR +\\(([0-9]+)\\)" _dummy "${GStreamer_VERSION_CONTENTS}")
		set(GStreamer_VERSION_MINOR "${CMAKE_MATCH_1}")

		string(REGEX MATCH "#define +GST_VERSION_MICRO +\\(([0-9]+)\\)" _dummy "${GStreamer_VERSION_CONTENTS}")
		set(GStreamer_VERSION_MICRO "${CMAKE_MATCH_1}")

		set(GStreamer_VERSION "${GStreamer_VERSION_MAJOR}.${GStreamer_VERSION_MINOR}.${GStreamer_VERSION_MICRO}")

	endif ()
endif()


##############################
# Process the COMPONENTS passed to FIND_PACKAGE

set(GStreamer_RequiredVars 
	GStreamer_core_LIBRARY GStreamer_core_INCLUDE_DIR
	GStreamer_glib_LIBRARY GStreamer_glib_INCLUDE_DIR
	GStreamer_gobject_LIBRARY GStreamer_gobject_INCLUDE_DIR
	GStreamer_glib_CONFIG_INCLUDE_DIR
)
if(WIN32)
	list(APPEND GStreamer_RequiredVars GStreamer_core_CONFIG_INCLUDE_DIR)
endif()


include(FindPackageHandleStandardArgs)
# Note: HANDLE_COMPONENTS is switched on here. This does check if a <component>_FOUND is set 
# to determine if the component was found. This means that the component finding must 
# be done elsewhere and it is not sensible to add component variables to the REQUIRED_VARS
find_package_handle_standard_args(GStreamer
	REQUIRED_VARS GStreamer_RequiredVars
	VERSION_VAR GStreamer_VERSION
	HANDLE_COMPONENTS
	)

# TODO: add mark_as_advanced here


###########
## Handle imported targets
set(GStreamer_glib_INCLUDE_DIRS ${GStreamer_glib_INCLUDE_DIR} ${GStreamer_glib_CONFIG_INCLUDE_DIR})
set(GStreamer_gobject_INCLUDE_DIRS ${GStreamer_gobject_INCLUDE_DIR} ${GStreamer_core_CONFIG_INCLUDE_DIR})

#Note: GStreamer_FOUND is set by find_package_handle_standard_args in case everything in GStreamer_RequiredVars is set
if(GStreamer_FOUND)
	if(NOT TARGET GStreamer::GStreamer)
		add_library(GStreamer::gstreamer UNKNOWN IMPORTED)
		set_target_properties(GStreamer::gstreamer PROPERTIES IMPORTED_LOCATION ${GStreamer_core_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_core_INCLUDE_DIR}")

		add_library(GStreamer::glib UNKNOWN IMPORTED)
		set_target_properties(GStreamer::glib PROPERTIES IMPORTED_LOCATION ${GStreamer_glib_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_glib_INCLUDE_DIRS}" )

		add_library(GStreamer::gobject UNKNOWN IMPORTED)
		set_target_properties(GStreamer::gobject PROPERTIES IMPORTED_LOCATION ${GStreamer_gobject_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_gobject_INCLUDE_DIRS}")

		add_library(GStreamer::base UNKNOWN IMPORTED)
		set_target_properties(GStreamer::base PROPERTIES IMPORTED_LOCATION ${GStreamer_base_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_base_INCLUDE_DIR}")

		add_library(GStreamer::app UNKNOWN IMPORTED)
		set_target_properties(GStreamer::app PROPERTIES IMPORTED_LOCATION ${GStreamer_app_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_app_INCLUDE_DIR}")

		add_library(GStreamer::video UNKNOWN IMPORTED)
		set_target_properties(GStreamer::video PROPERTIES IMPORTED_LOCATION ${GStreamer_video_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_video_INCLUDE_DIR}")

		add_library(GStreamer::codecparsers UNKNOWN IMPORTED)
		set_target_properties(GStreamer::codecparsers PROPERTIES IMPORTED_LOCATION ${GStreamer_codecparsers_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_codecparsers_INCLUDE_DIR}")

	endif()
endif()


include(CMakePrintHelpers)
cmake_print_properties(TARGETS GStreamer::gstreamer GStreamer::glib GStreamer::gobject GStreamer::base GStreamer::app GStreamer::video GStreamer::codecparsers
	PROPERTIES IMPORTED_LOCATION INTERFACE_INCLUDE_DIRECTORIES)

