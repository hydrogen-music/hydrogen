#
# - Try to find a ladspa header and plugins
#
# Copyright (c) 2009, Jérémy Zurcher, <jeremy@asynk.ch>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(FIND_LADSPA prefix header lib)

    if(${prefix}_INCLUDE_DIR AND ${prefix}_LIBRARIES)
        set(${prefix}_FIND_QUIETLY TRUE)
    else()
        MESSAGE(STATUS "Checking for module 'LADSPA'")
        
        set(CMAKE_PREFIX_PATH ${${prefix}_PATH} )

        find_path(${prefix}_INCLUDE_DIR
            NAMES ${header}
            PATHS ${${prefix}_INCLUDE_DIRS} ${${prefix}_INCLUDEDIR}
        )
        
        find_program( LISTPLUGINS NAMES listplugins )
        
        if( LISTPLUGINS )
            execute_process( OUTPUT_FILE "${CMAKE_BINARY_DIR}/ladspa_listplugins" COMMAND ${LISTPLUGINS} )
            file( STRINGS "${CMAKE_BINARY_DIR}/ladspa_listplugins" LADSPA_PLUGINS REGEX "ladspa/.*\\.so:" )
            list( LENGTH LADSPA_PLUGINS PLUGINS_N )
            if( PLUGINS_N GREATER 0 )
                STRING(REGEX REPLACE ":" "" LADSPA_PLUGINS "${LADSPA_PLUGINS}")
                SET(${prefix}_LIBRARIES "${LADSPA_PLUGINS}" )
                MESSAGE(STATUS "  ${PLUGINS_N} plugins found.")
            endif( )
        endif()

    endif()

    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(${prefix} DEFAULT_MSG ${prefix}_LIBRARIES ${prefix}_INCLUDE_DIR)
    mark_as_advanced(${prefix}_INCLUDE_DIR  ${prefix}_LIBRARIES)

endmacro()
