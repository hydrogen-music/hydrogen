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
        # try pkg-config
        if(NOT WIN32)
            FIND_PACKAGE(PkgConfig)
        endif()
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(${prefix} ${pkg_name})
        else()
            MESSAGE(STATUS "Checking for module '${pkg_name}'")
        endif()
        if(NOT ${prefix}_FOUND)
            # try find_path and find_library
            find_path(${prefix}_INCLUDE_DIRS
                NAMES ${header}
                ENV ${prefix}_INCLUDE
            )
            find_library(${prefix}_LIBRARIES
                NAMES ${lib}
                ENV ${prefix}_PATH
            )
            include(FindPackageHandleStandardArgs)
            FIND_PACKAGE_HANDLE_STANDARD_ARGS(${prefix} DEFAULT_MSG ${prefix}_LIBRARIES ${prefix}_INCLUDE_DIRS)
        endif()
    endif()
    #MESSAGE(STATUS  " FOUND     ${${prefix}_FOUND}" )
    #MESSAGE(STATUS  " INCLUDE_DIRS  ${${prefix}_INCLUDE_DIRS}" )
    #MESSAGE(STATUS  " LIBRARIES     ${${prefix}_LIBRARIES}" )
endmacro()
