#
# Copyright (c) 2009, Jérémy Zurcher, <jeremy@asynk.ch>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(MANDATORY_PKG pkg_name)
    set(WANT_${pkg_name} TRUE)
    set(${pkg_name}_FIND_REQUIRED TRUE)
endmacro()

macro(COMPUTE_PKGS_FLAGS prefix)
    if(${prefix}_NOT_NEEDED)
        if(${prefix}_FOUND)
            SET(${prefix}_STATUS "| available but not needed ${${prefix}_VERSION} ( ${${prefix}_LIBRARIES} )")
        else()
            SET(${prefix}_STATUS "| not found but not needed" )
        endif()
    elseif(${prefix}_FOUND)
        if(NOT WANT_${prefix})
            SET(${prefix}_STATUS "? available but not desired ${${prefix}_VERSION} ( ${${prefix}_LIBRARIES} )")
            SET(${prefix}_LIBRARIES "" )
            SET(${prefix}_INCLUDE_DIR "" )
        else()
            SET(${prefix}_STATUS "+ used ${${prefix}_VERSION} ( ${${prefix}_LIBRARIES} )")
            SET(H2CORE_HAVE_${prefix} TRUE)
        endif()
    else()
        SET(${prefix}_LIBRARIES "" )
        SET(${prefix}_INCLUDE_DIR "" )
        if(NOT WANT_${prefix})
            SET(${prefix}_STATUS "- not found and not desired" )
        else()
            SET(${prefix}_STATUS "-- not found but desired ..." )
        endif()
    endif()
endmacro()

