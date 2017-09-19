#
# - Try to find a library using pkg-config if available,
#   than portable macros FIND_PATH and FIND_LIBRARY
#
# The following variables will be set :
#
#  ${prefix}_FOUND          - set to 1 or TRUE if found
#  ${prefix}_INCLUDE_DIRS   - to be used in INCLUDE_DIRECTORIES(...)
#  ${prefix}_LIBRARIES      - to be used in TARGET_LINK_LIBRARIES(...)
#
# Copyright (c) 2009-2017, Jérémy Zurcher, <jeremy@asynk.ch>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(FIND_HELPER prefix pkg_name header lib)
    if(${prefix}_INCLUDE_DIRS AND ${prefix}_LIBRARIES)
        # use cached variables
        set(${prefix}_FIND_QUIETLY TRUE)
    else()
        # use pkg-config if available to set find_path and find_library hints
        if(NOT WIN32)
            FIND_PACKAGE(PkgConfig)
        endif()
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PC_${prefix} ${pkg_name})
        else()
            MESSAGE(STATUS "Checking for module '${pkg_name}'")
        endif()
        # find_path
        find_path(${prefix}_INCLUDE_DIRS
            NAMES ${header}
            HINTS ${PC_${prefix}_INCLUDEDIR} ${PC_${prefix}_INCLUDE_DIRS}
            ENV ${prefix}_INCLUDE
            )
        # find_library
        find_library(${prefix}_LIBRARIES
            NAMES ${lib}
            HINTS ${PC_${prefix}_LIBDIR} ${PC_${prefix}_LIBRARY_DIRS}
            ENV ${prefix}_PATH
            )
        include(FindPackageHandleStandardArgs)
        if ("${${prefix}_INCLUDE_DIRS}" STREQUAL "")
          set(${prefix}_INCLUDE_DIRS "/usr/include")
        endif ()
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(${prefix} DEFAULT_MSG ${prefix}_LIBRARIES ${prefix}_INCLUDE_DIRS)
        if(NOT "${PC_${prefix}_INCLUDE_DIRS}" STREQUAL "")
            set(${prefix}_INCLUDE_DIRS "${${prefix}_INCLUDE_DIRS};${PC_${prefix}_INCLUDE_DIRS}")
        endif()
    endif()
endmacro()
