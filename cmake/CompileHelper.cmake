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

macro(COMPILE_HELPER suffix prj_dir prj_name )
    try_compile( ${suffix}_COMPILES ${CMAKE_BINARY_DIR}/try/${prj_name} ${prj_dir} ${prj_name} )
    SET(HAVE_${suffix} FALSE)
    if( ${suffix}_COMPILES )
        execute_process( COMMAND ${CMAKE_BINARY_DIR}/try/${prj_name}/${prj_name} RESULT_VARIABLE ${suffix}_RUNS)
        if( ${suffix}_COMPILES )
            SET(HAVE_${suffix} TRUE)
        endif()
    endif()
    MESSAGE(STATUS  "Checking ${suffix} useability - ${HAVE_${suffix}}" )
endmacro()
