
set(GStreamer_RootSearchPath "" CACHE PATH "Additional root path to search GStreamer")

list(APPEND GStreamer_ROOT_SEARCH_PATHS ${GStreamer_RootSearchPath})

list(APPEND GStreamer_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86_64})
list(APPEND GStreamer_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86})


if(UNIX)
find_package(PkgConfig)
  pkg_check_modules(PkgConfig_gstreamer gstreamer-1.0)
  pkg_check_modules(PkgConfig_base gstreamer-base-1.0)
  pkg_check_modules(PkgConfig_app gstreamer-app-1.0)
  pkg_check_modules(PkgConfig_gl gstreamer-gl-1.0)
endif()

# gstreamer
find_path(GStreamer_gstreamer_INCLUDE_DIR NAMES gst/gst.h
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_gstreamer_INCLUDE_DIRS}
  PATH_SUFFIXES include/gstreamer-1.0
)
find_path(GStreamer_gstreamerConfig_INCLUDE_DIR NAMES gst/gstconfig.h
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_base_INCLUDE_DIRS}
  PATH_SUFFIXES lib/gstreamer-1.0/include include/gstreamer-1.0
)
set(GStreamer_gstreamer_INCLUDE_DIRS ${GStreamer_gstreamer_INCLUDE_DIR} ${GStreamer_gstreamerConfig_INCLUDE_DIR})

find_library(GStreamer_gstreamer_LIBRARY NAMES gstreamer-1.0
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_gstreamer_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)

# base
find_path(GStreamer_base_INCLUDE_DIR NAMES gst/base/base.h
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_base_INCLUDE_DIRS}
  PATH_SUFFIXES include/gstreamer-1.0
)
find_library(GStreamer_base_LIBRARY NAMES gstbase-1.0
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_base_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)

# app
find_path(GStreamer_app_INCLUDE_DIR NAMES gst/app/app.h
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_app_INCLUDE_DIRS}
  PATH_SUFFIXES include/gstreamer-1.0
)
find_library(GStreamer_app_LIBRARY NAMES gstapp-1.0
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_app_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)

# gl
find_path(GStreamer_gl_INCLUDE_DIR NAMES gst/gl/gl.h
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_gl_INCLUDE_DIRS}
  PATH_SUFFIXES include/gstreamer-1.0
)
find_library(GStreamer_gl_LIBRARY NAMES gstgl-1.0
  PATHS ${GStreamer_ROOT_SEARCH_PATHS} ${PkgConfig_gl_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)


# Handle imported targets

if(NOT TARGET GStreamer::GStreamer)
  add_library(GStreamer::gstreamer UNKNOWN IMPORTED)
  set_target_properties(GStreamer::gstreamer PROPERTIES
    IMPORTED_LOCATION "${GStreamer_gstreamer_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_gstreamer_INCLUDE_DIRS}"
  )

  add_library(GStreamer::base UNKNOWN IMPORTED)
  set_target_properties(GStreamer::base PROPERTIES
    IMPORTED_LOCATION "${GStreamer_base_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_base_INCLUDE_DIR}"
  )

  add_library(GStreamer::app UNKNOWN IMPORTED)
  set_target_properties(GStreamer::app PROPERTIES
    IMPORTED_LOCATION "${GStreamer_app_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_app_INCLUDE_DIR}"
  )

  add_library(GStreamer::gl UNKNOWN IMPORTED)
  set_target_properties(GStreamer::gl PROPERTIES
    IMPORTED_LOCATION "${GStreamer_gl_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_gl_INCLUDE_DIR}"
  )
endif()