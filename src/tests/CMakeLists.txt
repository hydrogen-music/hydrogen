CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
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

FILE(GLOB_RECURSE TESTS_SRCS *.cpp)
link_directories()
add_executable(tests ${TESTS_SRCS})

SET_PROPERTY(TARGET tests PROPERTY CXX_STANDARD 17)

target_link_libraries(tests
	hydrogen-core-${VERSION}
	${CPPUNIT_LIBRARIES}
	Qt5::Core
	Qt5::Test
	Qt5::Network
)

IF(Backtrace_FOUND)
	target_link_libraries(tests
		${Backtrace_LIBRARIES}
	)
ENDIF()

add_dependencies(tests hydrogen-core-${VERSION})
