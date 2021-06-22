cmake_minimum_required(VERSION 3.4...3.18)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_program(binder binder REQUIRED)

set(core_bindings_library "hydra")
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)

set(core_wrapped_header ${CMAKE_CURRENT_SOURCE_DIR}/core_bindings.h)
set(core_config_file ${CMAKE_CURRENT_SOURCE_DIR}/core_bindings.conf)
set(core_generated_sources_dependencies ${core_wrapped_header} ${core_config_file})
set(binder_options 
    -c -fPIC -std=c++14 -I/home/rebelcat/Hack/hydrogen/src
    -I/usr/include/c++/10/
    -I/usr/include/x86_64-linux-gnu/c++/10/
    -I/home/rebelcat/Hack/hydrogen/build/src
    -I/usr/include/x86_64-linux-gnu/qt5/
    -I/usr/include/x86_64-linux-gnu/qt5/QtCore
    -I/usr/include/x86_64-linux-gnu/qt5/QtGui
    -I/usr/include/x86_64-linux-gnu/qt5/QtXml
)
set(binder_outdir ${CMAKE_CURRENT_SOURCE_DIR})
set(binder_namespace H2Core)
# Add custom target to run binder to generate the binding cpp files.
set(core_generated_sources

${binder_outdir}/core/Object.cpp
${binder_outdir}/core/Logger.cpp
${binder_outdir}/core/Preferences.cpp
${binder_outdir}/core/Basics/Sample.cpp

${binder_outdir}/hydra.cpp
)
file(GLOB_RECURSE core_generated_sources *.cpp)


set(core_generated_headers

)

add_custom_command(OUTPUT ${core_generated_sources} ${core_generated_headers}
                    COMMAND ${binder}
                    --trace
                    -v
                    # --flat
                    --annotate-includes
                    --config ${core_config_file}
                    --root-module ${core_bindings_library}
                    --prefix ${CMAKE_CURRENT_SOURCE_DIR}
                    --bind ${binder_namespace} # namespace to build
                    ${core_wrapped_header}
                    -- ${binder_options}
                    DEPENDS ${core_generated_sources_dependencies}
                    IMPLICIT_DEPENDS CXX ${core_wrapped_header}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Running generator for ${core_bindings_library}.")

# =============================== h2core PySide bindings ============================================
# pybind11_add_module(hydra ${core_generated_sources})

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
)

pybind11_extension(${core_bindings_library})