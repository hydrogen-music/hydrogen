cmake_minimum_required(VERSION 3.8)

add_definitions()
include_directories(
    ${CMAKE_SOURCE_DIR}/src/core/include            # core headers
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src                         # generated config.h
    ${QT_INCLUDES}
    ${JACK_INCLUDE_DIRS}
    ${CPPUNIT_INCLUDE_DIRS}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${RUBBERBAND_INCLUDE_DIRS}
)

file(GLOB_RECURSE TESTS_SRCS *.cpp)
link_directories()
add_executable(tests ${TESTS_SRCS})

set_property(TARGET tests PROPERTY CXX_STANDARD 17)

target_link_libraries(tests
	hydrogen-core-${VERSION}
	${CPPUNIT_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Test
	Qt${QT_VERSION_MAJOR}::Network
)

if(Backtrace_FOUND)
	target_link_libraries(tests
		${Backtrace_LIBRARIES}
	)
endif()

add_dependencies(tests hydrogen-core-${VERSION})
