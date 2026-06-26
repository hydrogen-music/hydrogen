# Plugin targets (ADR 0013/0014).
#
# `hydrogen-plugin` is the format-agnostic engine wrapper (HydrogenPlugin). It
# depends only on the core library - no plugin SDK - so it is always built and
# is exercised by the unit tests (PluginLifecycleTest). The native CLAP and LV2
# modules are thin C-ABI shims over it, gated behind WANT_CLAP / WANT_LV2.
#
# T3.4 (ADR 0013/0026): the standalone-only entry points (DiskWriterDriver, NSM
# session client, JACK session/transport) are unreachable under a plugin host at
# runtime (Plugin audio/MIDI drivers + Hydrogen::isUnderPluginHost()), so the
# core library is reused as-is rather than recompiled with sources excluded.

add_library( hydrogen-plugin STATIC HydrogenPlugin.cpp )

target_include_directories( hydrogen-plugin PUBLIC
    ${CMAKE_SOURCE_DIR}/src # top level headers
    ${CMAKE_BINARY_DIR}/src # generated config.h
    ${QT_INCLUDES}
)

target_link_libraries( hydrogen-plugin PUBLIC
    hydrogen-core-${VERSION}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)

set_target_properties( hydrogen-plugin PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
)

# ── Native CLAP plugin (ADR 0014) ─────────────────────────────────────────
if(WANT_CLAP)
    add_library( hydrogen-clap MODULE clap/HydrogenClap.cpp )
    target_include_directories( hydrogen-clap PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/clap/include
    )
    target_compile_definitions( hydrogen-clap PRIVATE
        H2_PLUGIN_OUTPUT_BUSES=${H2_PLUGIN_OUTPUT_BUSES}
    )
    target_link_libraries( hydrogen-clap PRIVATE hydrogen-plugin )
    # A CLAP plugin is a shared library with the `.clap` extension and no "lib"
    # prefix; the entry symbol is `clap_entry`.
    set_target_properties( hydrogen-clap PROPERTIES
        PREFIX ""
        SUFFIX ".clap"
        OUTPUT_NAME "Hydrogen"
        CXX_STANDARD 17
    )
    message(STATUS "Building CLAP plugin (buses=${H2_PLUGIN_OUTPUT_BUSES})")

    # Install into the conventional CLAP location (<prefix>/lib/clap; T6.2). The
    # editor binary at <prefix>/bin is found by HydrogenPlugin's relative search.
    install(TARGETS hydrogen-clap LIBRARY DESTINATION "${H2_LIB_PATH}/clap")

    # 8c smoke: run clap-validator on the built .clap if it is available (built
    # from extern/clap-validator or installed on PATH). This is the CI
    # conformance step from the implementation plan.
    find_program( CLAP_VALIDATOR clap-validator
        PATHS ${CMAKE_SOURCE_DIR}/extern/clap-validator/target/release )
    if(CLAP_VALIDATOR)
        add_test( NAME clap-validate
            COMMAND ${CLAP_VALIDATOR} validate "$<TARGET_FILE:hydrogen-clap>" )
        message(STATUS "  + clap-validator smoke test enabled (${CLAP_VALIDATOR})")
    else()
        message(STATUS "  - clap-validator not found; conformance test skipped")
    endif()
endif()

# ── Native LV2 plugin (ADR 0014) ──────────────────────────────────────────
if(WANT_LV2)
    add_library( hydrogen-lv2 MODULE lv2/HydrogenLv2.cpp )
    target_include_directories( hydrogen-lv2 PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/lv2/include
    )
    target_compile_definitions( hydrogen-lv2 PRIVATE
        H2_PLUGIN_OUTPUT_BUSES=${H2_PLUGIN_OUTPUT_BUSES}
    )
    target_link_libraries( hydrogen-lv2 PRIVATE hydrogen-plugin )
    set_target_properties( hydrogen-lv2 PROPERTIES
        PREFIX ""
        OUTPUT_NAME "hydrogen"
        CXX_STANDARD 17
    )

    set(H2_LV2_BUNDLE "${CMAKE_CURRENT_BINARY_DIR}/hydrogen.lv2")

    # manifest.ttl (no looped content -> configure_file).
    configure_file( lv2/manifest.ttl.in "${H2_LV2_BUNDLE}/manifest.ttl" @ONLY )

    # hydrogen.ttl: generate the audio output ports for master + N buses with a
    # foreach (configure_file cannot loop). Port layout matches HydrogenLv2.cpp:
    #   0 = MIDI in, 1/2 = master L/R, 3.. = bus i L/R.
    set(H2_LV2_PORTS "")
    string(APPEND H2_LV2_PORTS
        "        a lv2:AudioPort , lv2:OutputPort ;\n"
        "        lv2:index 1 ;\n        lv2:symbol \"master_l\" ;\n        lv2:name \"Master L\"\n    ] , [\n"
        "        a lv2:AudioPort , lv2:OutputPort ;\n"
        "        lv2:index 2 ;\n        lv2:symbol \"master_r\" ;\n        lv2:name \"Master R\"\n")
    math(EXPR H2_LV2_LAST_BUS "${H2_PLUGIN_OUTPUT_BUSES} - 1")
    foreach(bus RANGE 0 ${H2_LV2_LAST_BUS})
        math(EXPR idxL "3 + 2 * ${bus}")
        math(EXPR idxR "${idxL} + 1")
        math(EXPR busName "${bus} + 1")
        string(APPEND H2_LV2_PORTS
            "    ] , [\n"
            "        a lv2:AudioPort , lv2:OutputPort ;\n"
            "        lv2:index ${idxL} ;\n        lv2:symbol \"bus_${bus}_l\" ;\n        lv2:name \"Bus ${busName} L\"\n"
            "    ] , [\n"
            "        a lv2:AudioPort , lv2:OutputPort ;\n"
            "        lv2:index ${idxR} ;\n        lv2:symbol \"bus_${bus}_r\" ;\n        lv2:name \"Bus ${busName} R\"\n")
    endforeach()

    # Platform-native UI type for the out-of-process editor (ADR 0016). The UI is
    # not embedded (shown via ui:showInterface), but LV2 still requires a concrete
    # window-toolkit type; pick the one matching the build platform.
    if(APPLE)
        set(H2_LV2_UI_TYPE "CocoaUI")
    elseif(WIN32)
        set(H2_LV2_UI_TYPE "WindowsUI")
    else()
        set(H2_LV2_UI_TYPE "X11UI")
    endif()

    configure_file( lv2/hydrogen.ttl.in "${H2_LV2_BUNDLE}/hydrogen.ttl" @ONLY )

    # Drop the built module into the bundle next to the manifests.
    add_custom_command( TARGET hydrogen-lv2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${H2_LV2_BUNDLE}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:hydrogen-lv2>" "${H2_LV2_BUNDLE}/"
        COMMENT "Assembling hydrogen.lv2 bundle" )

    message(STATUS "Building LV2 plugin (buses=${H2_PLUGIN_OUTPUT_BUSES})")

    # Install the assembled .lv2 bundle into the conventional LV2 location
    # (<prefix>/lib/lv2/hydrogen.lv2; T6.2).
    install(DIRECTORY "${H2_LV2_BUNDLE}/"
        DESTINATION "${H2_LIB_PATH}/lv2/hydrogen.lv2")

    # 8c smoke: a tiny in-process LV2 host that drives the built module through
    # its full lifecycle (stands in for lv2lint where meson/lilv are absent).
    add_executable( hydrogen-lv2-smoke lv2/lv2_smoke.cpp )
    target_include_directories( hydrogen-lv2-smoke PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/lv2/include )
    target_link_libraries( hydrogen-lv2-smoke PRIVATE ${CMAKE_DL_LIBS} )
    set_target_properties( hydrogen-lv2-smoke PROPERTIES CXX_STANDARD 17 )
    add_test( NAME lv2-smoke
        COMMAND hydrogen-lv2-smoke "$<TARGET_FILE:hydrogen-lv2>" )

    # If lv2lint is available, run the proper conformance linter on the bundle.
    # `-I` points it at the bundle directory (the one holding manifest.ttl); the
    # plugin URI is the positional argument.
    find_program( LV2LINT lv2lint
        PATHS ${CMAKE_SOURCE_DIR}/extern/lv2lint/build )
    if(LV2LINT)
        add_test( NAME lv2lint
            COMMAND ${LV2LINT} -I "${H2_LV2_BUNDLE}"
                "https://hydrogen-music.org/lv2/hydrogen" )
        message(STATUS "  + lv2lint conformance test enabled (${LV2LINT})")
    else()
        message(STATUS "  - lv2lint not found; using in-process LV2 smoke only")
    endif()
endif()

# ── Native VST3 plugin via clap-wrapper (ADR 0014, T6.1) ───────────────────
# Wraps the native CLAP (clap/HydrogenClap.cpp → `clap_entry`) as a VST3 with
# free-audio/clap-wrapper (extern/clap-wrapper submodule, v0.14.0). The VST3 SDK
# is fetched at configure time via CPM (CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES); the
# CLAP SDK is reused from our own submodule rather than re-downloaded. Opt-in
# (WANT_VST3=OFF by default) because pulling + building the Steinberg VST3 SDK is
# heavy; enable with -DWANT_VST3=1.
if(WANT_VST3)
    # Point clap-wrapper at our vendored CLAP SDK; let it fetch the VST3 SDK.
    set(CLAP_SDK_ROOT "${CMAKE_SOURCE_DIR}/extern/clap" CACHE PATH "" FORCE)
    set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)
    set(CLAP_WRAPPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    # clap-wrapper's static libs (VST3 SDK + wrapper) get linked into our shared
    # VST3 module, so they must be position-independent.
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    add_subdirectory(
        "${CMAKE_SOURCE_DIR}/extern/clap-wrapper"
        "${CMAKE_BINARY_DIR}/clap-wrapper"
        EXCLUDE_FROM_ALL )

    # clap-wrapper forces a blanket -Werror on its own targets via the INTERFACE
    # target clap-wrapper-compile-options (no toggle). That breaks on newer
    # compilers against its vendored fmt v9 — e.g. GCC 13/14 flag
    # -Wdangling-reference / -Wtautological-compare inside fmt and -Werror turns
    # them fatal. A per-target -Wno-error can't fix it: INTERFACE options land
    # after the target's own flags on the command line, so -Werror wins. Strip the
    # blanket -Werror from the interface instead — vendored code must not gate our
    # build (it propagates to every clap-wrapper target).
    if(TARGET clap-wrapper-compile-options)
        get_target_property(_cw_opts clap-wrapper-compile-options
            INTERFACE_COMPILE_OPTIONS)
        if(_cw_opts)
            list(REMOVE_ITEM _cw_opts "-Werror")
            set_target_properties(clap-wrapper-compile-options PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${_cw_opts}")
        endif()
    endif()

    # Compile the CLAP entry (clap_entry) in its own static lib with OUR flags.
    # If it were a source of the wrapped module it would inherit clap-wrapper's
    # strict -Wall/-Werror (PUBLIC on its wrapper lib), which flags unused helpers
    # in our shared headers — and interface -Werror can't be overridden per-target
    # since it lands last on the command line.
    add_library( hydrogen-vst3-entry STATIC clap/HydrogenClap.cpp )
    target_include_directories( hydrogen-vst3-entry PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/clap/include )
    target_compile_definitions( hydrogen-vst3-entry PRIVATE
        H2_PLUGIN_OUTPUT_BUSES=${H2_PLUGIN_OUTPUT_BUSES} )
    target_link_libraries( hydrogen-vst3-entry PUBLIC hydrogen-plugin )
    set_target_properties( hydrogen-vst3-entry PROPERTIES
        CXX_STANDARD 17 POSITION_INDEPENDENT_CODE ON )

    # The VST3 module: clap-wrapper adds its export glue (which references
    # clap_entry, pulled from the entry lib) -> one CLAP plugin, one VST3.
    add_library( hydrogen-vst3 MODULE )
    target_link_libraries( hydrogen-vst3 PRIVATE hydrogen-vst3-entry )

    target_add_vst3_wrapper(
        TARGET hydrogen-vst3
        OUTPUT_NAME "Hydrogen" )

    # The vendored Steinberg VST3 SDK + clap-wrapper sources don't compile cleanly
    # under our strict warning policy. Relaxations for these third-party targets
    # only (created by target_add_vst3_wrapper above):
    #   * -Wno-format-security: the SDK builds with -Wno-format, which against our
    #     global -Wformat -Werror=format-security leaves -Wformat-security "ignored
    #     without -Wformat". Disabling -Wformat-security removes the diagnostic
    #     entirely - merely demoting it (-Wno-error=format-security) would leave a
    #     warning spamming every SDK TU.
    #   * -Wno-error: relaxes clap-wrapper's own blanket -Werror (GCC) for the
    #     vendored code.
    # hydrogen-vst3 needs it too: base-sdk-vst3 exports -Wno-format PUBLICly, so
    # the wrapper export entry compiled into the module hits the same clash.
    set(_cw_relax -Wno-error -Wno-format-security)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC 13/14 flag clap-wrapper's vendored fmt v9 (-Wdangling-reference /
        # -Wtautological-compare). Silence them so third-party code is quiet as
        # well as non-fatal. Harmless on older GCC, which ignores unknown -Wno-*.
        list(APPEND _cw_relax -Wno-dangling-reference -Wno-tautological-compare)
    endif()
    foreach(cw_target
            base-sdk-vst3
            hydrogen-vst3-clap-wrapper-vst3-lib
            clap-wrapper-shared-detail
            hydrogen-vst3)
        if(TARGET ${cw_target})
            target_compile_options(${cw_target} PRIVATE ${_cw_relax})
        endif()
    endforeach()

    # Smoke test: the built VST3 module exports the VST3 entry point. A full
    # Steinberg validator run is left to CI (T6.3), which builds the SDK's
    # validator; this catches a broken wrap cheaply in any build.
    if(UNIX AND NOT APPLE)
        add_test( NAME vst3-smoke
            COMMAND sh -c
                "nm -D '$<TARGET_FILE:hydrogen-vst3>' | grep -q GetPluginFactory" )
    endif()

    # Install the .vst3 bundle (a folder on Linux/Windows, a bundle on macOS)
    # into the conventional VST3 location (<prefix>/lib/vst3; T6.2). clap-wrapper
    # emits the bundle under <build>/<Config>/Hydrogen.vst3.
    install(DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/Hydrogen.vst3"
        DESTINATION "${H2_LIB_PATH}/vst3")

    message(STATUS "Building VST3 plugin via clap-wrapper (buses=${H2_PLUGIN_OUTPUT_BUSES})")
endif()
