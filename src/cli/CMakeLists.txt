
file(GLOB_RECURSE h2cli_SRCS *.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/src                     # top level headers
    ${CMAKE_BINARY_DIR}/src                     # generated config.h
    ${QT_INCLUDES}
    ${OSC_INCLUDE_DIRS}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
)

add_executable(h2cli WIN32 MACOSX_BUNDLE ${h2cli_SRCS} )

set_property(TARGET h2cli PROPERTY CXX_STANDARD 17)
target_link_libraries(h2cli
	hydrogen-core-${VERSION}
	Qt5::Widgets
	${OSC_LIBRARIES}
	)

add_dependencies(h2cli hydrogen-core-${VERSION})

install(TARGETS h2cli RUNTIME DESTINATION ${H2_BIN_PATH} BUNDLE DESTINATION ${H2_BIN_PATH})
