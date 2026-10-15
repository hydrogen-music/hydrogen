cmake_minimum_required(VERSION 3.16)

add_definitions()
include_directories(
    ${CMAKE_SOURCE_DIR}/src/core/include            # core headers
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src                         # generated config.h
    ${CMAKE_SOURCE_DIR}/src/gui/src                # Skin.h conversion helpers
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
	hydrogen-plugin
	hydrogen-plugin-ui-models
	hydrogen-core-${VERSION}
	${CPPUNIT_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Test
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Xml
)

# KeySequenceTest pins the key-name table against the qnamespace.h of
# the Qt version this test binary is built against.
get_target_property(QT_CORE_INCLUDE_DIRS Qt${QT_VERSION_MAJOR}::Core
	INTERFACE_INCLUDE_DIRECTORIES)
find_path(QT_QNAMESPACE_H_PATH qnamespace.h HINTS ${QT_CORE_INCLUDE_DIRS})
if(NOT QT_QNAMESPACE_H_PATH)
	message(FATAL_ERROR "qnamespace.h not found for KeySequenceTest")
endif()
target_compile_definitions(tests PRIVATE
	QT_QNAMESPACE_H="${QT_QNAMESPACE_H_PATH}/qnamespace.h")

if(Backtrace_FOUND)
	target_link_libraries(tests
		${Backtrace_LIBRARIES}
	)
endif()

add_dependencies(tests hydrogen-core-${VERSION} hydrogen-plugin)
