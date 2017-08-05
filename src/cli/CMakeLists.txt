
FILE(GLOB_RECURSE h2cli_SRCS *.cpp)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/include        # core headers
    ${CMAKE_BINARY_DIR}/src/core/include        # generated config.h
    ${QT_INCLUDES}
    ${LASH_INCLUDE_DIRS}
    ${OSC_INCLUDE_DIRS}
)

ADD_EXECUTABLE(h2cli WIN32 MACOSX_BUNDLE ${h2cli_SRCS} )

TARGET_LINK_LIBRARIES(h2cli
	hydrogen-core-${VERSION}
	Qt5::Widgets
	${LASH_LIBRARIES}
	${OSC_LIBRARIES}
	)

ADD_DEPENDENCIES(h2cli hydrogen-core-${VERSION})

INSTALL(TARGETS h2cli RUNTIME DESTINATION bin BUNDLE DESTINATION bin )
