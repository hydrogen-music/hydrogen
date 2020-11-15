
FILE(GLOB_RECURSE h2player_SRCS *.cpp)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/src/core/include        # core headers
    ${CMAKE_SOURCE_DIR}/src                     # top level headers
    ${CMAKE_BINARY_DIR}/src/core/include        # generated config.h
    ${QT_INCLUDES}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
)

ADD_EXECUTABLE(h2player WIN32 MACOSX_BUNDLE ${h2player_SRCS} )

SET_PROPERTY(TARGET h2player PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(h2player
	hydrogen-core-${VERSION}
	Qt5::Widgets
	)

ADD_DEPENDENCIES(h2player hydrogen-core-${VERSION})

INSTALL(TARGETS h2player RUNTIME DESTINATION ${H2_BIN_PATH} BUNDLE DESTINATION ${H2_BIN_PATH})
