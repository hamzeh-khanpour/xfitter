set(PineAPPL_FIND_REQUIRED 1)
find_package(PkgConfig REQUIRED)
pkg_check_modules(pineappl_capi QUIET pineappl_capi)
if(pineappl_capi_FOUND)
  message(STATUS "pkg-config exists")
  set(PineAPPL_FOUND 1)
  #Check include dirs
  if(NOT EXISTS ${pineappl_capi_INCLUDE_DIRS})
    message(STATUS "pineappl_capi include dir not found")
    set(PineAPPL_FOUND 0)
  endif()
  #Check lib dirs
  if(NOT EXISTS ${pineappl_capi_LIBRARY_DIRS})
    message(STATUS "pineappl_capi lib dir not found")
    set(PineAPPL_FOUND 0)
  endif()
  execute_process(COMMAND ${pkg-config} --version pineappl_capi OUTPUT_VARIABLE PineAPPL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${pkg-config} --libs pineappl_capi OUTPUT_VARIABLE PineAPPL_LIBRARY_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${pkg-config} --cflags pineappl_capi OUTPUT_VARIABLE PineAPPL_COMPILE_OPTIONS OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(PineAPPL_COMPILE_OPTIONS UNIX_COMMAND ${PineAPPL_COMPILE_OPTIONS}) #convert space-separated list of compiler flags into a ';'-separated cmake list
  add_library(PineAPPL SHARED IMPORTED)
  if(APPLE)
    set_target_properties(PineAPPL PROPERTIES IMPORTED_LOCATION "${pineappl_capi_LIBRARY_DIRS}/libpineappl_capi.dylib")
  endif()
  if(UNIX AND NOT APPLE)
    set_target_properties(PineAPPL PROPERTIES IMPORTED_LOCATION "${pineappl_capi_LIBRARY_DIRS}/libpineappl_capi.so")
  endif()
  set_target_properties(PineAPPL PROPERTIES INTERFACE_COMPILE_OPTIONS "${pineappl_capi_CFLAGS}")
elseif(PineAPPL_FIND_REQUIRED)
  message(FATAL_ERROR "PineAPPL not found")
else()
  message(FATAL_ERROR "pkg_check_modules cannot find pineappl capi")  
endif()
if(NOT PineAPPL_FIND_QUIETLY)
  if(PineAPPL_FOUND)
    message(STATUS "Found PineAPPL ${pineappl_capi_VERSION}")
  else()
    message(STATUS "PineAPPL not found")
  endif()
endif()
