#
# Copyright (c) 2009, Jérémy Zurcher, <jeremy@asynk.ch>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(MANDATORY_PKG pkg_name)
    set(WANT_${pkg_name} TRUE)
    set(${pkg_name}_FIND_REQUIRED TRUE)
endmacro()

string( ASCII 27 _escape)

function(COLOR_MESSAGE TEXT)
    if(CMAKE_COLOR_MAKEFILE)
        MESSAGE(${TEXT})
    else()
        string(REGEX REPLACE "${_escape}.[0123456789;]*m" "" __TEXT ${TEXT})
        MESSAGE(${__TEXT})
    endif()
endfunction()

macro(COMPUTE_PKGS_FLAGS prefix)
    if(NOT ${prefix}_FOUND)
        SET(${prefix}_LIBRARIES "" )
        SET(${prefix}_INCLUDE_DIRS "" )
    endif()
    if(${prefix}_NOT_NEEDED)
        if(${prefix}_FOUND)
            SET(${prefix}_STATUS "${_escape}[0;33m| available\${_escape}[0m but not needed ${${prefix}_VERSION} ( ${${prefix}_LIBRARIES} )")
        else()
            SET(${prefix}_STATUS "${_escape}[1;33m| not found\${_escape}[0m but not needed" )
        endif()
    elseif(${prefix}_FOUND)
        if(NOT WANT_${prefix})
            SET(${prefix}_STATUS "${_escape}[0;32m? available${_escape}[0m but not desired ${${prefix}_VERSION} ( ${${prefix}_LIBRARIES} )")
        else()
            SET(${prefix}_STATUS "${_escape}[1;32m+ used${_escape}[0m ${${prefix}_VERSION} ( ${${prefix}_LIBRARIES} )")
            SET(H2CORE_HAVE_${prefix} TRUE)
        endif()
    else()
        if(NOT WANT_${prefix})
            SET(${prefix}_STATUS "${_escape}[0;31m- not found${_escape}[0m and not desired" )
        else()
            SET(${prefix}_STATUS "${_escape}[1;31m-- not found${_escape}[0m but desired ..." )
        endif()
    endif()
endmacro()

