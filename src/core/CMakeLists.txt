
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

file(GLOB_RECURSE hydrogen_INCLUDES *.h)
file(GLOB_RECURSE hydrogen_SOURCES *.cpp *.cc *.c)
list(APPEND hydrogen_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library( hydrogen-core-${VERSION} ${H2CORE_LIBRARY_TYPE} ${hydrogen_SOURCES})
include_directories( include
    ${CMAKE_SOURCE_DIR}/src                     # regular headers
    ${CMAKE_SOURCE_DIR}/include                 # regular headers
    ${CMAKE_CURRENT_BINARY_DIR}/../             # generated config.h
    ${QT_INCLUDES}
    ${LIBTAR_INCLUDE_DIRS}
    ${LIBARCHIVE_INCLUDE_DIRS}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${PULSEAUDIO_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${OSS_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${PORTMIDI_INCLUDE_DIRS}
    ${COREAUDIO_INCLUDE_DIRS}
    ${COREMIDI_INCLUDE_DIRS}
    ${LRDF_INCLUDE_DIRS}
    ${OSC_INCLUDE_DIRS}
    ${RUBBERBAND_INCLUDE_DIRS}
)

target_link_libraries(hydrogen-core-${VERSION}
    ${LIBTAR_LIBRARIES}
    ${LIBARCHIVE_LIBRARIES}
    ${LIBSNDFILE_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${OSS_LIBRARIES}
    ${JACK_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    ${PORTMIDI_LIBRARIES}
    ${AUDIOUNIT_LIBRARY}
    ${CORESERVICES_LIBRARY}
    ${COREAUDIO_LIBRARIES}
    ${COREMIDI_LIBRARIES}
    ${PULSEAUDIO_LIBRARIES}
    ${LRDF_LIBRARIES}
    ${RUBBERBAND_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${OSC_LIBRARIES}
)

target_link_libraries(hydrogen-core-${VERSION}
	Qt5::Core
	Qt5::Xml
	Qt5::XmlPatterns
    Qt5::Test
	Qt5::Gui # For QColor
)

#SET_TARGET_PROPERTIES(hydrogen-core-${VERSION} PROPERTIES PUBLIC_HEADER   "${hydrogen_INCLUDES}" )
set_property(TARGET hydrogen-core-${VERSION} PROPERTY CXX_STANDARD 17)


if(WIN32)
    # On Windows we do not want to install import library (.dll.a)
    install(TARGETS hydrogen-core-${VERSION}
        LIBRARY DESTINATION "${H2_LIB_PATH}"
        RUNTIME DESTINATION "${H2_LIB_PATH}"
    )
else()
    install(TARGETS hydrogen-core-${VERSION}
        LIBRARY DESTINATION "${H2_LIB_PATH}"
        RUNTIME DESTINATION "${H2_LIB_PATH}"
        ARCHIVE DESTINATION "${H2_LIB_PATH}"
    )
endif()

if(NOT APPLE AND NOT WIN32)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/core DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hydrogen" FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/config.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hydrogen" )
endif()
