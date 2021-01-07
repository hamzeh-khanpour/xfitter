find_package(PkgConfig REQUIRED)
#save PKG_CONFIG_PATH from environment
set(PKG_CONFIG_PATH_SAVED "$ENV{PKG_CONFIG_PATH}")
if(EXISTS "$ENV{yaml_DIR}")
  #if yaml_DIR environment variable is set, make pkg-config search there too
  set(ENV{PKG_CONFIG_PATH} "$ENV{yaml_DIR}/lib/pkgconfig:$ENV{yaml_DIR}/pkgconfig:$ENV{yaml_DIR}:$ENV{PKG_CONFIG_PATH}")
endif()
pkg_check_modules(yaml QUIET yaml-0.1)
if(yaml_FOUND)
  if(APPLE)
    set(yaml_SO "lib${yaml_LIBRARIES}.dylib")
  endif()
  if(UNIX AND NOT APPLE)
    set(yaml_SO "lib${yaml_LIBRARIES}.so")
  endif()
  find_library(yaml_IMPORTED_LOCATION ${yaml_SO} HINTS "${yaml_LIBRARY_DIRS}")
  if(EXISTS ${yaml_IMPORTED_LOCATION})
    add_library(yaml SHARED IMPORTED)
    set_target_properties(yaml PROPERTIES IMPORTED_LOCATION "${yaml_IMPORTED_LOCATION}")
    set_target_properties(yaml PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${yaml_INCLUDE_DIRS}")
  else()
    message(WARNING "yaml found by pkg-config, but the actual library file \"${yaml_SO}\" could not be found")
    set(yaml_FOUND 0)
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${yaml_INCLUDE_DIRS}")
endif()
if(NOT yaml_FOUND AND yaml_FIND_REQUIRED)
  message(FATAL_ERROR "yaml not found
(If it is actually installed, set the environment variable yaml_DIR to its install prefix)")
endif()
if(NOT yaml_FIND_QUIETLY)
  message(STATUS "Found yaml ${yaml_VERSION}: ${yaml_IMPORTED_LOCATION}")
endif()
#restore PKG_CONFIG_PATH
set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH_SAVED}")
