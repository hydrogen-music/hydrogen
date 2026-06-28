
file(GLOB_RECURSE hydrogen_SRCS src/*.cpp src/*.h)

# add undiscovered dependencies to generated config.h
include(AddFileDependencies)
foreach( _file ${hydrogen_H} ${hydrogen_SRCS})
    file(READ "${_file}" _FILE_CONTENT)
    if( "${_FILE_CONTENT}" MATCHES "H2CORE_HAVE_" )
        add_file_dependencies( ${_file} ${CMAKE_BINARY_DIR}/src/core/config.h)
    endif()
endforeach()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}                 # ui_ headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src             # gui headers
    ${CMAKE_BINARY_DIR}/src                     # generated config.h
    ${CMAKE_SOURCE_DIR}/src                     # top level headers
    ${QT_INCLUDES}
    ${LIBARCHIVE_INCLUDE_DIRS}
    ${LIBSNDFILE_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${PULSEAUDIO_INCLUDE_DIRS}
)

if(APPLE)
	# set how it shows up in the Info.plist file
    set(MACOSX_BUNDLE_INFO_STRING "Hydrogen for Mac")
	set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "Hydrogen")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}")
	#this has to be short since it appears as the application name in the menu bar
	set(MACOSX_BUNDLE_BUNDLE_NAME "Hydrogen")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
	set(MACOSX_BUNDLE_COPYRIGHT "Hydrogen Developers, 2009")

  	# set where in the bundle to put the icns file
	set_source_files_properties(${CMAKE_SOURCE_DIR}/data/img/gray/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	# include the icns file in the target
	set(hydrogen_SRCS ${hydrogen_SRCS} ${CMAKE_SOURCE_DIR}/data/img/gray/icon.icns)
	
	#copy the data folder
    file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION hydrogen.app/Contents/Resources PATTERN ".svn" EXCLUDE PATTERN i18n EXCLUDE)
  	
endif()

add_executable(hydrogen WIN32 MACOSX_BUNDLE ${hydrogen_SRCS} ${hydrogen_MOC} ${hydrogen_UIS_H} ${CMAKE_SOURCE_DIR}/windows/icon.rc)
set_property(TARGET hydrogen PROPERTY CXX_STANDARD 17)
if(APPLE)
    add_custom_command(
        TARGET hydrogen
        POST_BUILD
        COMMAND plutil -replace NSPrincipalClass -string NSApplication hydrogen.app/Contents/Info.plist
    )
    add_custom_command(
        TARGET hydrogen
        POST_BUILD
        COMMAND plutil -replace NSHighResolutionCapable -bool true hydrogen.app/Contents/Info.plist
    )
    add_custom_command(
        TARGET hydrogen
        POST_BUILD
        COMMAND
            plutil -extract CFBundleDocumentTypes json
                   -o /dev/null
                   hydrogen.app/Contents/Info.plist >/dev/null
            || plutil -insert CFBundleDocumentTypes
                      -json
                         \"` cat ${CMAKE_SOURCE_DIR}/macos/CFBundleDocumentTypes.json `\"
                       hydrogen.app/Contents/Info.plist )

    add_file_dependencies(hydrogen ${CMAKE_SOURCE_DIR}/macos/CFBundleDocumentTypes.json)

endif()

target_link_libraries(hydrogen
	hydrogen-core-${VERSION}
    Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Svg
)

if(Backtrace_FOUND)
	target_link_libraries(hydrogen
		${Backtrace_LIBRARIES}
	)
endif()

# Precompiled headers for Qt and std::vector. These dramatically reduce GUI build times
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
	target_precompile_headers(hydrogen
		PUBLIC
			<QtGui>
			<QtWidgets>
			<vector> )
endif()


add_dependencies(hydrogen hydrogen-core-${VERSION})

install(TARGETS hydrogen RUNTIME DESTINATION ${H2_BIN_PATH} BUNDLE DESTINATION ${H2_BIN_PATH})

# GUI startup smoke test (ADR 0015/0016): launch the full GUI headless (Qt
# offscreen platform), let it construct MainForm/HydrogenApp and tear down via
# --quit-after-startup, and require a clean (zero) exit. Catches startup/teardown
# crashes such as a not-yet-ready HydrogenApp being dereferenced during
# construction. --quit-after-startup also selects the no-op Fake audio driver
# (no hardware needed) and suppresses the development-build warning dialog. HOME
# points at a throwaway dir so no user config interferes.
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/gui-startup-test-home)
add_test(
	NAME GuiStartup
	COMMAND hydrogen --nosplash --child --quit-after-startup
			-P ${CMAKE_SOURCE_DIR}/data
)
set_tests_properties( GuiStartup PROPERTIES
	ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_BINARY_DIR}/gui-startup-test-home"
	TIMEOUT 120
)

# Editor-mode abort smoke test (ADR 0016): `hydrogen --plugin-editor <endpoint>`
# against an endpoint with no engine must abort cleanly (exit 1), NOT crash, and
# must not run an audio-processing thread on the mirror (the regression that
# segfaulted via a Logger use-after-free). The wrapper asserts both. Shell script
# → Unix only, like WriteSurfaceGuard. (The deterministic unit-level guard is
# EditorModeTest::testMirrorUsesPassiveAudioDriver.)
if ( UNIX )
	# 3 = Reporter::EXIT_CODE_CLEAN_FAILURE (the editor's clean "couldn't connect"
	# abort code). Passed to the wrappers so the shell and C++ stay in sync.
	add_test(
		NAME EditorBadEndpoint
		COMMAND ${CMAKE_SOURCE_DIR}/tools/test_editor_bad_endpoint.sh
				$<TARGET_FILE:hydrogen> ${CMAKE_SOURCE_DIR}/data 3
	)
	set_tests_properties( EditorBadEndpoint PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_BINARY_DIR}/gui-startup-test-home"
		TIMEOUT 120
	)

	# The crash Reporter must treat that clean failed-connect as a normal exit
	# (propagate the code, no crash dialog), not a crash. Runs the full command
	# (no --child) so the Reporter wrapper is exercised end to end.
	add_test(
		NAME EditorReporterCleanFailure
		COMMAND ${CMAKE_SOURCE_DIR}/tools/test_editor_reporter.sh
				$<TARGET_FILE:hydrogen> ${CMAKE_SOURCE_DIR}/data 3
	)
	set_tests_properties( EditorReporterCleanFailure PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_BINARY_DIR}/gui-startup-test-home"
		TIMEOUT 120
	)
endif()

# Plugin-editor attach integration test (ADR 0016/0018): the success counterpart
# to EditorBadEndpoint and GuiStartup. A small host harness stands up the engine
# exactly as a DAW does — a HydrogenPlugin serving its engine over IPC
# (EngineSession, production serve path) — then launches the real GUI in
# out-of-process editor mode (`hydrogen --plugin-editor <endpoint>`) against that
# live endpoint. With --quit-after-startup the editor connects, builds MainForm
# over the IPC-backed engine handle, shows the window and quits cleanly; a clean
# (zero) exit proves the whole spawn->connect->show->teardown chain. The harness
# links the engine wrapper (hydrogen-plugin) and drives the spawn itself so it can
# pass --quit-after-startup; the GUI binary is the editor under test.
add_executable( plugin_editor_smoke PluginEditorSmoke.cpp )
set_property( TARGET plugin_editor_smoke PROPERTY CXX_STANDARD 17 )
target_link_libraries( plugin_editor_smoke
	hydrogen-plugin
	Qt${QT_VERSION_MAJOR}::Core
)
# Needs the GUI binary present (it is the editor that gets spawned).
add_dependencies( plugin_editor_smoke hydrogen )

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/editor-attach-test-home)
add_test(
	NAME EditorAttach
	COMMAND plugin_editor_smoke $<TARGET_FILE:hydrogen> ${CMAKE_SOURCE_DIR}/data
)
set_tests_properties( EditorAttach PROPERTIES
	ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_BINARY_DIR}/editor-attach-test-home"
	TIMEOUT 120
)

# Write-surface guard (ADR 0027, Phase 4.5 step 7): static source check that the
# GUI does not mutate engine-owned Basics objects directly but routes edits
# through CoreActionController. Pure source scan; needs no build artefacts.
# It is a bash script, so it is only registered on Unix (Linux + macOS); on
# Windows there is no shell to run it. The always-on Linux CI job runs it.
if ( UNIX )
	add_test(
		NAME WriteSurfaceGuard
		COMMAND ${CMAKE_SOURCE_DIR}/tools/check_write_surface
	)
	set_tests_properties( WriteSurfaceGuard PROPERTIES TIMEOUT 60 )
endif()
