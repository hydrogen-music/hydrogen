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
        ${H2_IMGUI_DIR}/backends/imgui_impl_osx.mm
        ${H2_IMGUI_DIR}/backends/imgui_impl_metal.mm )
else()
    list( APPEND H2_PLUGIN_UI_VENDORED_SOURCES
        ${H2_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp )
endif()

# TU2.3: the platform window seam — X11 child window (Linux), Win32 child
# HWND, NSView + Metal (macOS). Win32 renders through the GL3 backend (WGL
# via its embedded loader); macOS gains the Metal render backend above.
set( H2_PLUGIN_UI_WINDOW_SOURCES PluginUiWindow.cpp )
if(APPLE)
    list( APPEND H2_PLUGIN_UI_WINDOW_SOURCES PluginUiBackendMacOS.mm )
elseif(WIN32)
    list( APPEND H2_PLUGIN_UI_WINDOW_SOURCES PluginUiBackendWin32.cpp )
else()
    list( APPEND H2_PLUGIN_UI_WINDOW_SOURCES PluginUiBackendX11.cpp )
endif()

add_library( hydrogen-plugin-ui STATIC
    ${H2_PLUGIN_UI_WINDOW_SOURCES}
    ${H2_PLUGIN_UI_VENDORED_SOURCES} )

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

# Platform window/GL libraries (TU2.3). The seam is internal to the library
# (pimpl), so everything links PRIVATE; a static library still carries the
# link line to its consumers.
if(APPLE)
    find_library( H2_METAL_FRAMEWORK Metal )
    find_library( H2_APPKIT_FRAMEWORK AppKit )
    find_library( H2_QUARTZCORE_FRAMEWORK QuartzCore )
    target_link_libraries( hydrogen-plugin-ui PRIVATE
        ${H2_METAL_FRAMEWORK} ${H2_APPKIT_FRAMEWORK}
        ${H2_QUARTZCORE_FRAMEWORK} )
elseif(WIN32)
    target_link_libraries( hydrogen-plugin-ui PRIVATE opengl32 gdi32 user32 )
else()
    find_package( X11 REQUIRED )
    find_package( OpenGL REQUIRED )
    target_link_libraries( hydrogen-plugin-ui PRIVATE X11::X11 OpenGL::GL )
endif()

# Window smoke (proposal 0006 TU2.1/TU2.2): embed on a test-created native
# parent, idle frames, destroy. Skips with a message on headless platforms
# (no DISPLAY / no window server) — CI cannot answer X11-GL-on-host-parent;
# the manual spike gate (TU2.4) does.
add_executable( hydrogen-plugin-ui-smoke PluginUiSmoke.cpp )
target_link_libraries( hydrogen-plugin-ui-smoke PRIVATE hydrogen-plugin-ui )
set_target_properties( hydrogen-plugin-ui-smoke PROPERTIES CXX_STANDARD 17 )
if(APPLE)
    # The smoke's macOS branch creates its NSWindow parent in Objective-C++:
    # the shared .cpp compiles as ObjC++ there (a strict superset — the C++
    # branches of the platform #if are preprocessed out on APPLE).
    set_source_files_properties( PluginUiSmoke.cpp PROPERTIES LANGUAGE OBJCXX )
    target_link_libraries( hydrogen-plugin-ui-smoke PRIVATE
        ${H2_METAL_FRAMEWORK} ${H2_APPKIT_FRAMEWORK}
        ${H2_QUARTZCORE_FRAMEWORK} )
elseif(NOT WIN32)
    target_link_libraries( hydrogen-plugin-ui-smoke PRIVATE X11::X11 )
endif()
add_test( NAME PluginUiSmoke COMMAND hydrogen-plugin-ui-smoke )

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
