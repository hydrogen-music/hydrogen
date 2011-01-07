
FILE(GLOB_RECURSE h2player_SRCS *.cpp)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/include        # core headers
    ${CMAKE_BINARY_DIR}/src/core/include        # generated config.h
    ${QT_INCLUDES}
)

ADD_EXECUTABLE(h2player WIN32 MACOSX_BUNDLE ${h2player_SRCS} )
TARGET_LINK_LIBRARIES(h2player
    hydrogen-core-${VERSION}
    ${QT_QTGUI_LIBRARY}
)

ADD_DEPENDENCIES(h2player hydrogen-core-${VERSION})

INSTALL(TARGETS h2player RUNTIME DESTINATION bin BUNDLE DESTINATION bin )
