#
# - Try to compile and run a given source file, success if returns 0
#
# Once done this will define
#
#  HAVE_${suffix}
#
# Copyright (c) 2009, Jérémy Zurcher, <jeremy@asynk.ch>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(COMPILE_HELPER suffix src_file)
    try_run( ${suffix}_RUNS ${suffix}_COMPILES ${CMAKE_BINARY_DIR}/tests ${src_file} )
    if( ( ${suffix}_COMPILES )  AND ( ${suffix}_RUNS EQUAL 0 ) )
        SET(HAVE_${suffix} TRUE)
    else()
        SET(HAVE_${suffix} FALSE)
    endif()
    MESSAGE(STATUS  "Checking ${suffix} useability - ${HAVE_${suffix}}" )
endmacro()
