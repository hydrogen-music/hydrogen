
file(GLOB_RECURSE h2player_SRCS *.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/src                     # top level headers
    ${CMAKE_BINARY_DIR}/src                     # generated config.h
    ${QT_INCLUDES}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
)

add_executable(h2player WIN32 MACOSX_BUNDLE ${h2player_SRCS} )

set_property(TARGET h2player PROPERTY CXX_STANDARD 17)
target_link_libraries(h2player
	hydrogen-core-${VERSION}
	Qt${QT_VERSION_MAJOR}::Widgets
	)

add_dependencies(h2player hydrogen-core-${VERSION})

install(TARGETS h2player RUNTIME DESTINATION ${H2_BIN_PATH} BUNDLE DESTINATION ${H2_BIN_PATH})
