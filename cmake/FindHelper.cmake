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
        set(${prefix}_FOUND TRUE)
    else()
        # use pkg-config if available to set find_path and find_library hints
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PC_${prefix} ${pkg_name})
        else()
            message(STATUS "Checking for module '${pkg_name}' without pkgconfig")
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
        find_package_handle_standard_args(${prefix} DEFAULT_MSG ${prefix}_LIBRARIES ${prefix}_INCLUDE_DIRS)
        # A static library does not record its own dependencies, e.g. the
        # codec libraries of libsndfile. Link the ones pkg-config lists for
        # static linking as well. pkg-config splits "-framework X" into two
        # items, which CMake would turn into "-framework" and "-lX".
        if("${${prefix}_LIBRARIES}" MATCHES "\\.a$" AND PC_${prefix}_STATIC_LDFLAGS)
            set(_FIND_HELPER_STATIC_DEPS "")
            set(_FIND_HELPER_IS_FRAMEWORK FALSE)
            foreach(_FIND_HELPER_FLAG IN LISTS PC_${prefix}_STATIC_LDFLAGS)
                if(_FIND_HELPER_IS_FRAMEWORK)
                    list(APPEND _FIND_HELPER_STATIC_DEPS "-Wl,-framework,${_FIND_HELPER_FLAG}")
                    set(_FIND_HELPER_IS_FRAMEWORK FALSE)
                elseif(_FIND_HELPER_FLAG STREQUAL "-framework")
                    set(_FIND_HELPER_IS_FRAMEWORK TRUE)
                else()
                    list(APPEND _FIND_HELPER_STATIC_DEPS "${_FIND_HELPER_FLAG}")
                endif()
            endforeach()
            list(APPEND ${prefix}_LIBRARIES ${_FIND_HELPER_STATIC_DEPS})
        endif()
        if(NOT "${PC_${prefix}_INCLUDE_DIRS}" STREQUAL "")
            set(${prefix}_INCLUDE_DIRS "${${prefix}_INCLUDE_DIRS};${PC_${prefix}_INCLUDE_DIRS}")
        endif()
        # Update variables in the cache (these are set by find_path/find_library) after modifying
        set(${prefix}_INCLUDE_DIRS "${${prefix}_INCLUDE_DIRS}" CACHE PATH "Path to ${pkg_name} include files" FORCE)
        set(${prefix}_LIBRARIES "${${prefix}_LIBRARIES}" CACHE FILEPATH "Path to ${pkg_name} libraries" FORCE)
    endif()
endmacro()
