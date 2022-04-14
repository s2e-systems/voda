set(GLib_RootSearchPath "" CACHE PATH "Additional root path to search GLib")

list(APPEND GLib_ROOT_SEARCH_PATHS ${GLib_RootSearchPath})

# On Windows the glib is delivered with a GStreamer instalation,
# hence add this to the root search path
list(APPEND GLib_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86_64})
list(APPEND GLib_ROOT_SEARCH_PATHS $ENV{GStreamer_1_0_ROOT_X86})

if(UNIX)
find_package(PkgConfig)
  pkg_check_modules(PkgConfig_glib glib-2.0)
endif()

find_path(GLib_glib_INCLUDE_DIR NAMES glib.h
  PATHS ${GLib_ROOT_SEARCH_PATHS} ${PkgConfig_glib_INCLUDE_DIRS}
  PATH_SUFFIXES include/glib-2.0
)
find_library(GLib_glib_LIBRARY NAMES glib-2.0
  PATHS ${GLib_ROOT_SEARCH_PATHS} ${PkgConfig_GLib_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)



# Find the config header file of the glib. Which is for some odd reason in the
# library dir (in win and linux)
find_path(GLib_glibConfig_INCLUDE_DIR NAMES glibconfig.h
  HINTS ${GLib_ROOT_SEARCH_PATHS} ${PkgConfig_glib_INCLUDE_DIRS}
  PATH_SUFFIXES lib/glib-2.0/include
)

set(GLib_glib_INCLUDE_DIRS ${GLib_glib_INCLUDE_DIR} ${GLib_glibConfig_INCLUDE_DIR})


# Find GObject
if(UNIX)
  pkg_check_modules(PkgConfig_gobject gobject-2.0)
endif()

find_path(GLib_gobject_INCLUDE_DIR NAMES gobject.h
  PATHS ${GLib_ROOT_SEARCH_PATHS} ${PkgConfig_gobject_INCLUDE_DIRS}
  PATH_SUFFIXES include/glib-2.0/gobject
)
find_library(GLib_gobject_LIBRARY NAMES gobject-2.0
  PATHS ${GLib_ROOT_SEARCH_PATHS} ${PkgConfig_gobject_LIBRARY_DIRS}
  PATH_SUFFIXES lib
)


if(NOT TARGET GLib::glib)
  add_library(GLib::glib UNKNOWN IMPORTED)
  set_target_properties(GLib::glib PROPERTIES
    IMPORTED_LOCATION "${GLib_glib_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLib_glib_INCLUDE_DIRS}"
  )
  add_library(GLib::gobject UNKNOWN IMPORTED)
  set_target_properties(GLib::gobject PROPERTIES
    IMPORTED_LOCATION "${GLib_gobject_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLib_gobject_INCLUDE_DIRS}"
  )
endif()


