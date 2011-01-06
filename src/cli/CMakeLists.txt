
FILE(GLOB_RECURSE h2cli_SRCS *.cpp)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/include        # core headers
    ${CMAKE_BINARY_DIR}/src/core/include        # generated config.h
    ${QT_INCLUDES}
    ${LASH_LIBRARIES}
)

ADD_EXECUTABLE(h2cli WIN32 MACOSX_BUNDLE ${h2cli_SRCS} )
TARGET_LINK_LIBRARIES(h2cli
    hydrogen-core
    ${QT_QTGUI_LIBRARY}
)

ADD_DEPENDENCIES(h2cli hydrogen-core)

INSTALL(TARGETS h2cli RUNTIME DESTINATION bin BUNDLE DESTINATION bin )
