
FILE(GLOB_RECURSE h2cli_SRCS *.cpp)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src                     # top level headers
    ${CMAKE_BINARY_DIR}/src                     # generated config.h
    ${QT_INCLUDES}
    ${LASH_INCLUDE_DIRS}
    ${OSC_INCLUDE_DIRS}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
)

ADD_EXECUTABLE(h2cli WIN32 MACOSX_BUNDLE ${h2cli_SRCS} )

SET_PROPERTY(TARGET h2cli PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(h2cli
	hydrogen-core-${VERSION}
	Qt5::Widgets
	${LASH_LIBRARIES}
	${OSC_LIBRARIES}
	)

ADD_DEPENDENCIES(h2cli hydrogen-core-${VERSION})

INSTALL(TARGETS h2cli RUNTIME DESTINATION ${H2_BIN_PATH} BUNDLE DESTINATION ${H2_BIN_PATH})
