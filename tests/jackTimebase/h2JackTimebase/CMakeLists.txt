
file(GLOB_RECURSE h2JackTimebase_SRCS *.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/src                     # top level headers
    ${CMAKE_SOURCE_DIR}/tests/jackTimebase/     # dedicated test sources
    ${CMAKE_BINARY_DIR}/src                     # generated config.h
    ${QT_INCLUDES}
    ${OSC_INCLUDE_DIRS}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${CPPUNIT_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
)

add_executable(h2JackTimebase WIN32 MACOSX_BUNDLE ${h2JackTimebase_SRCS} )

set_property(TARGET h2JackTimebase PROPERTY CXX_STANDARD 17)
target_link_libraries(h2JackTimebase
	hydrogen-core-${VERSION}
	Qt${QT_VERSION_MAJOR}::Widgets
	${CPPUNIT_LIBRARIES}
	${OSC_LIBRARIES}
	)

add_dependencies(h2JackTimebase hydrogen-core-${VERSION})

install(TARGETS h2JackTimebase RUNTIME DESTINATION ${H2_BIN_PATH} BUNDLE DESTINATION ${H2_BIN_PATH})
