
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
