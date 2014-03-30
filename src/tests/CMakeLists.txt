
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
add_definitions()
include_directories(
    ${CMAKE_SOURCE_DIR}/src/core/include            # core headers
    ${CMAKE_BINARY_DIR}/src/core/include            # generated config.h
    ${QT_INCLUDES}                                  # TODO be able to remove this
	${CPPUNIT_INCLUDE_DIR}
)
FILE(GLOB_RECURSE TESTS_SRCS *.cpp)
link_directories()
add_executable(tests ${TESTS_SRCS})
target_link_libraries(tests
    hydrogen-core-${VERSION}
	${CPPUNIT_LIBRARIES}
)
add_dependencies(tests hydrogen-core-${VERSION})

