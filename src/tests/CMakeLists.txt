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
# the Qt version this test binary is built against, so it needs the
# header holding the actual declarations. QtGuiGateTest needs the
# include root with one folder per module (<root>/QtGui/QWidget).
get_target_property(QT_CORE_INCLUDE_DIRS Qt${QT_VERSION_MAJOR}::Core
	INTERFACE_INCLUDE_DIRECTORIES)
if(APPLE)
	# In a framework build of Qt (e.g. Homebrew) <prefix>/include/QtCore
	# only holds forwarding stubs that `#include_next` the real header
	# inside QtCore.framework/Headers. That Headers folder, however, lacks
	# the per-module layout, so the two lookups differ. A non-framework
	# build (e.g. the static plugin Qt) resolves both to the plain layout.
	set(QT_CORE_HEADER_DIRS "")
	set(QT_CORE_PLAIN_INCLUDE_DIRS "")
	foreach(DIR IN LISTS QT_CORE_INCLUDE_DIRS)
		if(DIR MATCHES "\\.framework$")
			list(APPEND QT_CORE_HEADER_DIRS "${DIR}/Headers")
		else()
			list(APPEND QT_CORE_PLAIN_INCLUDE_DIRS "${DIR}")
		endif()
	endforeach()
	list(APPEND QT_CORE_HEADER_DIRS ${QT_CORE_PLAIN_INCLUDE_DIRS})
	# NO_DEFAULT_PATH: /opt/homebrew/include or /usr/local/include must not
	# shadow the Qt we link against.
	find_path(QT_QNAMESPACE_H_DIR qnamespace.h
		PATHS ${QT_CORE_HEADER_DIRS} NO_DEFAULT_PATH)
	# Forwarding stubs are fine here since only their existence matters.
	find_path(QT_CORE_MODULE_INCLUDE_DIR qnamespace.h
		PATHS ${QT_CORE_PLAIN_INCLUDE_DIRS} ${QT_CORE_HEADER_DIRS}
		NO_DEFAULT_PATH)
else()
	find_path(QT_QNAMESPACE_H_DIR qnamespace.h HINTS ${QT_CORE_INCLUDE_DIRS})
	set(QT_CORE_MODULE_INCLUDE_DIR "${QT_QNAMESPACE_H_DIR}")
endif()
if(NOT QT_QNAMESPACE_H_DIR OR NOT QT_CORE_MODULE_INCLUDE_DIR)
	message(FATAL_ERROR "qnamespace.h not found for KeySequenceTest")
endif()
get_filename_component(QT_MODULE_INCLUDE_ROOT
	"${QT_CORE_MODULE_INCLUDE_DIR}" DIRECTORY)

target_compile_definitions(tests PRIVATE
	QT_QNAMESPACE_H="${QT_QNAMESPACE_H_DIR}/qnamespace.h"
	QT_MODULE_INCLUDE_ROOT="${QT_MODULE_INCLUDE_ROOT}")

if(Backtrace_FOUND)
	target_link_libraries(tests
		${Backtrace_LIBRARIES}
	)
endif()

add_dependencies(tests hydrogen-core-${VERSION} hydrogen-plugin)
