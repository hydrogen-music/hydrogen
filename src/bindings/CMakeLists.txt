# ================================== Shiboken detection ======================================
# Use provided python interpreter if given.
if(NOT python_interpreter)
    find_program(python_interpreter "python")
endif()
message(STATUS "Using python interpreter: ${python_interpreter}")

# Macro to get various pyside / python include / link flags and paths.
# Uses the not entirely supported utils/pyside2_config.py file.
macro(pyside_config option output_var)
    if(${ARGC} GREATER 1)
        set(is_list ${ARGV1})
    else()
        set(is_list "")
    endif()

    execute_process(
      COMMAND ${python_interpreter} "${CMAKE_CURRENT_SOURCE_DIR}/utils/pyside_config.py"
              ${option}
      OUTPUT_VARIABLE ${output_var}
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if ("${${output_var}}" STREQUAL "")
        message(FATAL_ERROR "Error: Calling pyside_config.py ${option} returned no output.")
    endif()
    if(is_list)
        string (REPLACE " " ";" ${output_var} "${${output_var}}")
    endif()
    message(INFO "${green}option: ${option}, output_var: ${output_var}; value: ${${output_var}}")
endmacro()

# Query for the shiboken generator path, Python path, include paths and linker flags.
pyside_config(--shiboken-path shiboken_path)
pyside_config(--shiboken-module-path shiboken_module_path)
pyside_config(--shiboken-generator-path shiboken_generator_path)
pyside_config(--python-include-path python_include_dir)
#pyside_config(--pyside-typesystems-path pyside_typesystems)
pyside_config(--pyside-include-path pyside_include_dir)
pyside_config(--shiboken-generator-include-path shiboken_include_dir 1)
pyside_config(--shiboken-module-shared-libraries-cmake shiboken_shared_libraries 0)
pyside_config(--python-link-flags-cmake python_linking_data 0)
pyside_config(--pyside-shared-libraries-cmake pyside_shared_libraries 0)

set(shiboken_path "${shiboken_path}${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT EXISTS ${shiboken_path})
    message(FATAL_ERROR "Shiboken executable not found at path: ${shiboken_path}")
endif()

message(STATUS "Shiboken: ${shiboken_path}")

# =============================== h2core PySide bindings ============================================
set(core_bindings_library "h2core")

set(core_wrapped_header ${CMAKE_SOURCE_DIR}/src/bindings/core_bindings.h)
set(core_typesystem_file ${CMAKE_SOURCE_DIR}/src/bindings/core_bindings.xml)


set(core_generated_sources
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_module_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_object_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_sample_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_sample_loops_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_sample_rubberband_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_envelopepoint_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_instrument_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_instrumentcomponent_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_instrumentlayer_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_instrumentlist_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_drumkit_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_song_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_songreader_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_adsr_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_logger_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_hydrogen_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_filesystem_wrapper.cpp
    ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/h2core_preferences_wrapper.cpp
)

# ====================== Shiboken target for generating binding C++ files  ====================
message(STATUS "QtIncl ${QT_INCLUDES}")


# Set up the options to pass to shiboken.
set(shiboken_options 
    --language-level=c++17
    --generator-set=shiboken
    --enable-parent-ctor-heuristic
    # --enable-return-value-heuristic
    --use-isnull-as-nb_nonzero
    --enable-pyside-extensions
    --avoid-protected-hack
    -T/home/rebelcat/.virtualenvs/pyside/lib/python3.9/site-packages/PySide2/typesystems/
    -I${CMAKE_SOURCE_DIR}/src
    -I${CMAKE_SOURCE_DIR}/src/core
    -I${CMAKE_BINARY_DIR}/src
    -I${CMAKE_BINARY_DIR}/src/core
    -I/usr/include/x86_64-linux-gnu/qt5                 # FIXME: how to get Qt includes ?
    -I/usr/include/x86_64-linux-gnu/qt5/QtCore
    -I/usr/include/x86_64-linux-gnu/qt5/QtGui
    -I/usr/include/x86_64-linux-gnu/qt5/QDebug
    -I/usr/include/x86_64-linux-gnu/qt5/QtXml
    --output-directory=${CMAKE_CURRENT_BINARY_DIR}
    )
set(core_generated_sources_dependencies ${core_wrapped_header} ${core_typesystem_file})


# Add custom target to run shiboken to generate the binding cpp files.
add_custom_command(OUTPUT ${core_generated_sources}
                    COMMAND ${shiboken_path}
                    ${shiboken_options} ${core_wrapped_header} ${core_typesystem_file}
                    DEPENDS ${core_generated_sources_dependencies}
                    IMPLICIT_DEPENDS CXX ${core_wrapped_header}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/bindings
                    COMMENT "Running generator for ${core_typesystem_file}.")



# =============================== CMake target - core_bindings_library =============================

# Set the cpp files which will be used for the bindings library.
set(${core_bindings_library}_sources ${core_generated_sources})

# Define and build the bindings library.
add_library(${core_bindings_library} MODULE ${${core_bindings_library}_sources})
INCLUDE_DIRECTORIES(
    ${QT_INCLUDES}
    ${pyside_include_dir}
    ${pyside_include_dir}/QtCore
)
# Apply relevant include and link flags.
target_include_directories(${core_bindings_library} PRIVATE ${python_include_dir})
target_include_directories(${core_bindings_library} PRIVATE ${shiboken_include_dir})
target_include_directories(${core_bindings_library} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${core_bindings_library} PRIVATE ${CMAKE_BINARY_DIR}/src)
target_include_directories(${core_bindings_library} PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(${core_bindings_library} PRIVATE ${pyside_include_dir})
target_include_directories(${core_bindings_library} PRIVATE ${pyside_include_dir}/QtCore)

target_link_libraries(${core_bindings_library} PRIVATE ${shiboken_shared_libraries})
target_link_libraries(${core_bindings_library} PRIVATE hydrogen-core-${VERSION})
target_link_libraries(${core_bindings_library} PRIVATE ${pyside_shared_libraries})
#target_link_libraries(${core_bindings_library} PRIVATE pyside6)

TARGET_LINK_LIBRARIES(${core_bindings_library}
	Qt5::Core
	Qt5::Xml
	Qt5::XmlPatterns
)

# Adjust the name of generated module.
set_property(TARGET ${core_bindings_library} PROPERTY PREFIX "")
set_property(TARGET ${core_bindings_library} PROPERTY OUTPUT_NAME
             "${core_bindings_library}${PYTHON_EXTENSION_SUFFIX}")
if(WIN32)
    set_property(TARGET ${core_bindings_library} PROPERTY SUFFIX ".pyd")
endif()

# Make sure the linker doesn't complain about not finding Python symbols on macOS.
if(APPLE)
  set_target_properties(${core_bindings_library} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

# Find and link to the python import library only on Windows.
# On Linux and macOS, the undefined symbols will get resolved by the dynamic linker
# (the symbols will be picked up in the Python executable).
if (WIN32)
    list(GET python_linking_data 0 python_libdir)
    list(GET python_linking_data 1 python_lib)
    find_library(python_link_flags ${python_lib} PATHS ${python_libdir} HINTS ${python_libdir})
    target_link_libraries(${core_bindings_library} PRIVATE ${python_link_flags})
endif()

# ================================= Dubious deployment section ================================

set(windows_shiboken_shared_libraries)

if(WIN32)
    # =========================================================================================
    # !!! (The section below is deployment related, so in a real world application you will
    # want to take care of this properly (this is simply to eliminate errors that users usually
    # encounter.
    # =========================================================================================
    # Circumvent some "#pragma comment(lib)"s in "include/pyconfig.h" which might force to link
    # against a wrong python shared library.

    set(python_versions_list 3 32 33 34 35 36 37 38)
    set(python_additional_link_flags "")
    foreach(ver ${python_versions_list})
        set(python_additional_link_flags
            "${python_additional_link_flags} /NODEFAULTLIB:\"python${ver}_d.lib\"")
        set(python_additional_link_flags
            "${python_additional_link_flags} /NODEFAULTLIB:\"python${ver}.lib\"")
    endforeach()

    set_target_properties(${core_bindings_library}
                           PROPERTIES LINK_FLAGS "${python_additional_link_flags}")

    # Compile a list of shiboken shared libraries to be installed, so that
    # the user doesn't have to set the PATH manually to point to the PySide2 package.
    foreach(library_path ${shiboken_shared_libraries})
        string(REGEX REPLACE ".lib$" ".dll" library_path ${library_path})
        file(TO_CMAKE_PATH ${library_path} library_path)
        list(APPEND windows_shiboken_shared_libraries "${library_path}")
    endforeach()
    # =========================================================================================
    # !!! End of dubious section.
    # =========================================================================================
endif()

# =============================================================================================
# !!! (The section below is deployment related, so in a real world application you will want to
# take care of this properly with some custom script or tool).
# =============================================================================================
# Install the library and the bindings module into the source folder near the main.py file, so
# that the Python interpeter successfully imports the used module.
install(TARGETS ${core_bindings_library} 
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        )    
install(FILES ${windows_shiboken_shared_libraries} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
# =============================================================================================
# !!! End of dubious section.
# =============================================================================================



# =============================== CMake target - gui_bindings_library =============================
set(gui_bindings_library "h2gui")
set(gui_wrapped_header ${CMAKE_SOURCE_DIR}/src/bindings/gui_bindings.h)
set(gui_typesystem_file ${CMAKE_SOURCE_DIR}/src/bindings/gui_bindings.xml)

set(gui_generated_sources
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/h2gui_module_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/aboutdialogcontributorlist_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/aboutdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/audioengineinfoform_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/audiofilebrowser_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/automationpathview_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/button_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/clickablelabel_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/colorselectionbutton_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/cpuloadwidget_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/detailwavedisplay_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/director_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/downloadwidget_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/drumpatterneditor_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/eventlistener_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/exportmididialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/exportsongdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/fader_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/filebrowser_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/filesysteminfoform_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/hydrogenapp_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/infobar_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/instrumenteditorpanel_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/instrumenteditor_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/instrumentrack_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/ladspafxproperties_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/ladspafxselector_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/layerpreview_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/lcdcombo_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/mainform_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/mainsamplewavedisplay_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/midiactivitywidget_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/midilearnable_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/midisensewidget_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/miditable_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/mixerline_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/mixersettingsdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/mixer_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/notepropertiesruler_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/patterneditorinstrumentlist_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/patterneditorpanel_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/patterneditorruler_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/patterneditor_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/patternfilldialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/patternpropertiesdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/pianorolleditor_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/pixmapwidget_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/playbacktrackwavedisplay_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/playercontrol_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/playlistdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/preferencesdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/rotary_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/sampleeditor_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/ui_sampleeditor_ui_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/samplewavedisplay_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/selection_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/shotlist_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/skin_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/songeditorpanel_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/songeditor_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/songpropertiesdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibraryexportdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibraryimportdialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibraryopendialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibrarypanel_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibraryrepositorydialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibrarysavedialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/soundlibrarytree_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/splashscreen_wrapper.cpp
${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/targetwavedisplay_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/virtualpatterndialog_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/wavedisplay_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/widgetscrollarea_wrapper.cpp
# ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/widgetwithscalablefont_wrapper.cpp

)
set_property(SOURCE
    ${CMAKE_BINARY_DIR}/src/bindings/${gui_bindings_library}/ui_sampleeditor_ui_wrapper.cpp
    PROPERTY SKIP_AUTOUIC ON)

# ================================== Shiboken detection ======================================
set(shiboken_options 
    --language-level=c++17
    --generator-set=shiboken
    --enable-parent-ctor-heuristic
    # --enable-return-value-heuristic
    --use-isnull-as-nb_nonzero
    --enable-pyside-extensions
    --avoid-protected-hack
    -T/home/rebelcat/.virtualenvs/pyside/lib/python3.9/site-packages/PySide2/typesystems/
    -I${CMAKE_SOURCE_DIR}/src
    -I${CMAKE_SOURCE_DIR}/src/core
    -I${CMAKE_SOURCE_DIR}/src/gui/src
    -I${CMAKE_BINARY_DIR}/src
    -I${CMAKE_BINARY_DIR}/src/gui/src
    -I${CMAKE_BINARY_DIR}/src/gui/hydrogen-gui-${VERSION}_autogen/include
    -I/usr/include/x86_64-linux-gnu/qt5                 # FIXME: how to get Qt includes ?
    -I/usr/include/x86_64-linux-gnu/qt5/QtCore
    -I/usr/include/x86_64-linux-gnu/qt5/QtGui
    -I/usr/include/x86_64-linux-gnu/qt5/QDebug
    -I/usr/include/x86_64-linux-gnu/qt5/QtXml
    -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets
    -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets
    -I/usr/include/x86_64-linux-gnu/qt5/QtNetwork
    --output-directory=${CMAKE_CURRENT_BINARY_DIR}
    )
set(gui_generated_sources_dependencies ${gui_wrapped_header} ${gui_typesystem_file})
add_custom_command(OUTPUT ${gui_generated_sources}
                    COMMAND ${shiboken_path}
                    ${shiboken_options} ${gui_wrapped_header} ${gui_typesystem_file}
                    DEPENDS ${gui_generated_sources_dependencies}
                    IMPLICIT_DEPENDS CXX ${gui_wrapped_header}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/bindings
                    COMMENT "Running generator for ${gui_typesystem_file}.")
# file(GLOB_RECURSE core_generated_sources 
#     ${CMAKE_BINARY_DIR}/src/bindings/${core_bindings_library}/*.cpp)

# # ==================================== RPATH configuration ====================================


# # =============================================================================================
# # !!! (The section below is deployment related, so in a real world application you will want to
# # take care of this properly with some custom script or tool).
# # =============================================================================================
# # Enable rpaths so that the built shared libraries find their dependencies.
# set(CMAKE_SKIP_BUILD_RPATH FALSE)
# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
# set(CMAKE_INSTALL_RPATH ${shiboken2_module_path} ${CMAKE_CURRENT_SOURCE_DIR})
# set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# # =============================================================================================
# # !!! End of dubious section.
# # ======


# Set the cpp files which will be used for the bindings library.
set(${gui_bindings_library}_sources ${gui_generated_sources})

# Define and build the bindings library.
add_library(${gui_bindings_library} MODULE ${${gui_bindings_library}_sources})
INCLUDE_DIRECTORIES(
    ${QT_INCLUDES}
    ${pyside_include_dir}
    ${pyside_include_dir}/QtCore
    ${pyside_include_dir}/QtGui
    ${pyside_include_dir}/QtWidgets
    /usr/include/x86_64-linux-gnu/qt5
    /usr/include/x86_64-linux-gnu/qt5/QtWidgets
    ${CMAKE_SOURCE_DIR}/src/gui/src
    ${CMAKE_BINARY_DIR}/src/bindings/h2core
    ${CMAKE_BINARY_DIR}/src/gui/hydrogen-gui-${VERSION}_autogen/include
)
# Apply relevant include and link flags.
target_include_directories(${gui_bindings_library} PRIVATE ${python_include_dir})
target_include_directories(${gui_bindings_library} PRIVATE ${shiboken_include_dir})
target_include_directories(${gui_bindings_library} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${gui_bindings_library} PRIVATE ${CMAKE_BINARY_DIR}/src)
target_include_directories(${gui_bindings_library} PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(${gui_bindings_library} PRIVATE ${pyside_include_dir})
target_include_directories(${gui_bindings_library} PRIVATE ${pyside_include_dir}/QtCore)

target_link_libraries(${gui_bindings_library} PRIVATE ${shiboken_shared_libraries})
target_link_libraries(${gui_bindings_library} PRIVATE ${pyside_shared_libraries})
target_link_libraries(${gui_bindings_library} PRIVATE hydrogen-gui-${VERSION})
#target_link_libraries(${gui_bindings_library} PRIVATE pyside6)

TARGET_LINK_LIBRARIES(${gui_bindings_library}
	Qt5::Core
	Qt5::Xml
	Qt5::XmlPatterns
    Qt5::Gui
)

# Adjust the name of generated module.
set_property(TARGET ${gui_bindings_library} PROPERTY PREFIX "")
set_property(TARGET ${gui_bindings_library} PROPERTY OUTPUT_NAME
             "${gui_bindings_library}${PYTHON_EXTENSION_SUFFIX}")
if(WIN32)
    set_property(TARGET ${gui_bindings_library} PROPERTY SUFFIX ".pyd")
endif()

# Make sure the linker doesn't complain about not finding Python symbols on macOS.
if(APPLE)
  set_target_properties(${gui_bindings_library} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

# Find and link to the python import library only on Windows.
# On Linux and macOS, the undefined symbols will get resolved by the dynamic linker
# (the symbols will be picked up in the Python executable).
if (WIN32)
    list(GET python_linking_data 0 python_libdir)
    list(GET python_linking_data 1 python_lib)
    find_library(python_link_flags ${python_lib} PATHS ${python_libdir} HINTS ${python_libdir})
    target_link_libraries(${gui_bindings_library} PRIVATE ${python_link_flags})
endif()

# ================================= Dubious deployment section ================================

set(windows_shiboken_shared_libraries)

if(WIN32)
    # =========================================================================================
    # !!! (The section below is deployment related, so in a real world application you will
    # want to take care of this properly (this is simply to eliminate errors that users usually
    # encounter.
    # =========================================================================================
    # Circumvent some "#pragma comment(lib)"s in "include/pyconfig.h" which might force to link
    # against a wrong python shared library.

    set(python_versions_list 3 32 33 34 35 36 37 38)
    set(python_additional_link_flags "")
    foreach(ver ${python_versions_list})
        set(python_additional_link_flags
            "${python_additional_link_flags} /NODEFAULTLIB:\"python${ver}_d.lib\"")
        set(python_additional_link_flags
            "${python_additional_link_flags} /NODEFAULTLIB:\"python${ver}.lib\"")
    endforeach()

    set_target_properties(${core_bindings_library}
                           PROPERTIES LINK_FLAGS "${python_additional_link_flags}")

    # Compile a list of shiboken shared libraries to be installed, so that
    # the user doesn't have to set the PATH manually to point to the PySide2 package.
    foreach(library_path ${shiboken_shared_libraries})
        string(REGEX REPLACE ".lib$" ".dll" library_path ${library_path})
        file(TO_CMAKE_PATH ${library_path} library_path)
        list(APPEND windows_shiboken_shared_libraries "${library_path}")
    endforeach()
    # =========================================================================================
    # !!! End of dubious section.
    # =========================================================================================
endif()

# =============================================================================================
# !!! (The section below is deployment related, so in a real world application you will want to
# take care of this properly with some custom script or tool).
# =============================================================================================
# Install the library and the bindings module into the source folder near the main.py file, so
# that the Python interpeter successfully imports the used module.
install(TARGETS ${gui_bindings_library} 
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        )    
install(FILES ${windows_shiboken_shared_libraries} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
# =============================================================================================
# !!! End of dubious section.
# =============================================================================================


