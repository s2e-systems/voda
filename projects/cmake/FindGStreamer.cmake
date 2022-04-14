
set(GStreamer_RootSearchPath "" CACHE PATH "Additional root path to search GStreamer")

list(APPEND GStreamer_ROOT_SEARCH_PATHS ${GStreamer_RootSearchPath})

list(APPEND GStreamer_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86_64})
list(APPEND GStreamer_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86})

if(UNIX)
  find_package(PkgConfig)
endif()

# find_gstreamer_component(componentPrefix header_filename libray_name
# [pkgconfig_name])
#
# Helper macro to find a GStreamer component (or GStreamer itself)
# componentPrefix is prepended to the _INCLUDE_DIR and _LIBRARY variables (eg.
# "GStreamer_AUDIO") Note: For WIN32 the extra argument pkgconfig_name will be
# ignored
#
# OUTPUT: componentPrefix - <component_prefix>_INCLUDE_DIR
# <component_prefix>_LIBRARY INPUT: headerFileName - base filename of the
# primary header file (will be added to the <component_prefix>_INCLUDE_DIR)
# librayFileName - base filename of the library (with out postfix .so or .lib
# and prefix lib as cmake standard serach) will be added to the
# <component_prefix>_LIBRARY pkgconfigName - optional, only for linux version
# component's pkg-config name (eg. "gstreamer-1.0", or "gstreamer-video-1.0").
# note that this is unfortunatly not the always the same as the library name
if(WIN32)
  macro(find_gstreamer_component componentPrefix headerFileName libraryFileName)

    find_path(
      ${componentPrefix}_INCLUDE_DIR
      NAMES ${headerFileName}
      PATHS ${GStreamer_ROOT_SEARCH_PATHS}
      PATH_SUFFIXES include include/gstreamer-1.0 include/glib-2.0
                    include/glib-2.0/gobject)

    find_library(
      ${componentPrefix}_LIBRARY
      NAMES ${libraryFileName}
      PATHS ${GStreamer_ROOT_SEARCH_PATHS}
      PATH_SUFFIXES lib)
    if(${componentPrefix}_INCLUDE_DIR AND ${componentPrefix}_LIBRARY)
      set(${componentPrefix}_FOUND TRUE)
    endif()
  endmacro()
else()
  macro(find_gstreamer_component componentPrefix headerFileName libraryFileName
        pkgconfigName)
    pkg_check_modules(PC_${componentPrefix} ${pkgconfigName})
    find_path(
      ${componentPrefix}_INCLUDE_DIR
      NAMES ${headerFileName}
      PATHS ${PC_${componentPrefix}_INCLUDE_DIRS}
      PATH_SUFFIXES gstreamer-1.0 gobject)

    find_library(
      ${componentPrefix}_LIBRARY
      NAMES ${libraryFileName}
      HINTS ${PC_${componentPrefix}_LIBRARY_DIRS})
    if(${componentPrefix}_INCLUDE_DIR AND ${componentPrefix}_LIBRARY)
      set(${componentPrefix}_FOUND TRUE)
    endif()
  endmacro()

endif()

# ##############################################################################
# Find GStreamer core

# Find headers and libraries
find_gstreamer_component(GStreamer_core gst/gst.h gstreamer-1.0 gstreamer-1.0)

if(WIN32)
  # Find the config header file of the gtreamer lib. Which may be for some odd
  # reason in the library dir
  find_path(
    GStreamer_core_CONFIG_INCLUDE_DIR
    NAMES gst/gstconfig.h
    PATHS ${GStreamer_ROOT_SEARCH_PATHS}
    PATH_SUFFIXES lib/gstreamer-1.0/include include/gstreamer-1.0)
endif(WIN32)


# ##############################################################################
# Find GStreamer plugins

find_gstreamer_component(GStreamer_base gst/base/gstbasesink.h gstbase-1.0
                         gstreamer-base-1.0)
find_gstreamer_component(GStreamer_app gst/app/gstappsink.h gstapp-1.0
                         gstreamer-app-1.0)
find_gstreamer_component(GStreamer_video gst/video/video.h gstvideo-1.0
                         gstreamer-video-1.0)
find_gstreamer_component(
  GStreamer_codecparsers gst/codecparsers/gsth264parser.h gstcodecparsers-1.0
  gstreamer-codecparsers-1.0)
find_gstreamer_component(
  GStreamer_gl gst/gl/gstglsl.h gstgl-1.0 gstreamer-gl-1.0)


# ##############################################################################
# Handle imported targets


if(NOT TARGET GStreamer::GStreamer)
  add_library(GStreamer::gstreamer UNKNOWN IMPORTED)
  set_target_properties(
    GStreamer::gstreamer
    PROPERTIES IMPORTED_LOCATION ${GStreamer_core_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_core_INCLUDE_DIR}")

  add_library(GStreamer::base UNKNOWN IMPORTED)
  set_target_properties(
    GStreamer::base
    PROPERTIES IMPORTED_LOCATION ${GStreamer_base_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_base_INCLUDE_DIR}")

  add_library(GStreamer::app UNKNOWN IMPORTED)
  set_target_properties(
    GStreamer::app
    PROPERTIES IMPORTED_LOCATION ${GStreamer_app_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_app_INCLUDE_DIR}")

  add_library(GStreamer::video UNKNOWN IMPORTED)
  set_target_properties(
    GStreamer::video
    PROPERTIES IMPORTED_LOCATION ${GStreamer_video_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_video_INCLUDE_DIR}")

  add_library(GStreamer::codecparsers UNKNOWN IMPORTED)
  set_target_properties(
    GStreamer::codecparsers
    PROPERTIES IMPORTED_LOCATION ${GStreamer_codecparsers_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES
                "${GStreamer_codecparsers_INCLUDE_DIR}")

endif()