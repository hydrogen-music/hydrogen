# hydrogen-plugin-ui — the embedded basic plugin UI (ADR 0035, proposal 0006).
#
# Model/renderer split as the testability seam (TU1.3): view *models* are pure
# logic with no imgui types in their headers (they link into the unit suite
# headless); *renderers* are thin imgui draw calls covered by the smokes. No
# logic grows in the untestable layer.
#
# TU1.2: the vendored Dear ImGui + add-ons compile here — imgui core + the GL3
# backend on Linux/Windows (its embedded loader resolves GL at runtime, so no
# system GL headers/libs are needed at build time) and the osx/Metal backend
# on macOS. Windowing is our own (TU2.3 PluginUiWindow) — no GLFW/SDL2.

set( H2_IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui )
set( H2_IMGUI_KNOBS_DIR ${CMAKE_SOURCE_DIR}/extern/imgui-knobs )
set( H2_IFD_DIR ${CMAKE_SOURCE_DIR}/extern/ImGuiFileDialog )

# Vendored sources (extern/imgui @ v1.92.9, extern/imgui-knobs @ master
# 6b4b592 — no tags upstream, extern/ImGuiFileDialog @ v0.6.8).
set( H2_PLUGIN_UI_VENDORED_SOURCES
    ${H2_IMGUI_DIR}/imgui.cpp
    ${H2_IMGUI_DIR}/imgui_demo.cpp
    ${H2_IMGUI_DIR}/imgui_draw.cpp
    ${H2_IMGUI_DIR}/imgui_tables.cpp
    ${H2_IMGUI_DIR}/imgui_widgets.cpp
    ${H2_IMGUI_KNOBS_DIR}/imgui-knobs.cpp
    ${H2_IFD_DIR}/ImGuiFileDialog.cpp
)
if(APPLE)
    list( APPEND H2_PLUGIN_UI_VENDORED_SOURCES
        ${H2_IMGUI_DIR}/backends/imgui_impl_osx.mm )
else()
    list( APPEND H2_PLUGIN_UI_VENDORED_SOURCES
        ${H2_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp )
endif()

add_library( hydrogen-plugin-ui STATIC ${H2_PLUGIN_UI_VENDORED_SOURCES} )

target_include_directories( hydrogen-plugin-ui PUBLIC
    ${H2_IMGUI_DIR}
    ${H2_IMGUI_DIR}/backends
    ${H2_IMGUI_KNOBS_DIR}
    ${H2_IFD_DIR}
)

target_link_libraries( hydrogen-plugin-ui PUBLIC hydrogen-plugin )

set_target_properties( hydrogen-plugin-ui PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
)

# macOS: the osx/Metal backend needs the system frameworks.
if(APPLE)
    find_library( H2_METAL_FRAMEWORK Metal )
    find_library( H2_APPKIT_FRAMEWORK AppKit )
    target_link_libraries( hydrogen-plugin-ui PUBLIC
        ${H2_METAL_FRAMEWORK} ${H2_APPKIT_FRAMEWORK} )
endif()

# Plugin-UI dependency guard (ADR 0035, proposal 0006 TU1.3): static source
# check that the plugin UI layer never depends on the Qt-Widgets GUI
# application (no Qt application objects, no GUI-application-tree includes).
# Pure source scan; needs no build artefacts. It is a bash script, so it is
# only registered on Unix (Linux + macOS), like WriteSurfaceGuard
# (src/gui/CMakeLists.txt).
if(UNIX)
    add_test(
        NAME PluginUiGuard
        COMMAND ${CMAKE_SOURCE_DIR}/tools/check_plugin_ui
    )
    set_tests_properties( PluginUiGuard PROPERTIES TIMEOUT 60 )
endif()
