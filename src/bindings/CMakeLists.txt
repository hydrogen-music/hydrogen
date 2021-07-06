cmake_minimum_required(VERSION 3.4...3.18)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

set(core_bindings_library "h2core")
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)

set(core_wrapped_header ${CMAKE_CURRENT_SOURCE_DIR}/h2core_module.hpp)
# Add custom target to run binder to generate the binding cpp files.
set(core_generated_sources
${CMAKE_CURRENT_SOURCE_DIR}/h2core_module.cpp
)

add_library(${core_bindings_library} MODULE ${core_generated_sources})
target_link_libraries(${core_bindings_library} PRIVATE
    pybind11::module pybind11::lto pybind11::windows_extras
    hydrogen-core-${VERSION}
	Qt5::Core
	Qt5::Xml
	Qt5::XmlPatterns
)

target_include_directories(${core_bindings_library} PRIVATE
    ${QT_INCLUDES}
# Apply relevant include and link flags.
${python_include_dir}
${CMAKE_SOURCE_DIR}/src
${CMAKE_BINARY_DIR}/src
${CMAKE_SOURCE_DIR}/src/core
${CMAKE_SOURCE_DIR}/src/bindings/include
)

execute_process(COMMAND
    "${_Python_EXECUTABLE}"
    "-c"
    "from distutils import sysconfig as s;print(s.get_python_lib(plat_specific=True), end='');"
    RESULT_VARIABLE _PYTHON_SUCCESS
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES)
message("python site packages: [${PYTHON_SITE_PACKAGES}]")

pybind11_extension(${core_bindings_library})
install(TARGETS ${core_bindings_library}
        LIBRARY DESTINATION "${PYTHON_SITE_PACKAGES}"
)