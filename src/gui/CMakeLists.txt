
FILE(GLOB_RECURSE hydrogen_H src/*.h)
FILE(GLOB_RECURSE hydrogen_UIS src/*.ui)
FILE(GLOB_RECURSE hydrogen_SRCS src/*.cpp)

# add undiscovered dependencies to generated config.h
INCLUDE(AddFileDependencies)
FOREACH( _file ${hydrogen_H} ${hydrogen_SRCS})
    FILE(READ "${_file}" _FILE_CONTENT)
    IF( "${_FILE_CONTENT}" MATCHES "H2CORE_HAVE_" )
        ADD_FILE_DEPENDENCIES( ${_file} ${CMAKE_BINARY_DIR}/src/core/include/hydrogen/config.h)
    ENDIF()
ENDFOREACH()

QT4_WRAP_UI(hydrogen_UIS_H ${hydrogen_UIS})             # ui_ files are generated in CMAKE_CURRENT_BINARY_DIR
QT4_WRAP_CPP(hydrogen_MOC ${hydrogen_H} OPTIONS -nw) 

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}                 # ui_ headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src             # gui headers
    ${CMAKE_SOURCE_DIR}/src/core/include        # core headers
    ${CMAKE_BINARY_DIR}/src/core/include        # generated config.h
    ${LASH_INCLUDE_DIR}
    ${QT_INCLUDES}
)

IF(APPLE)
	# set how it shows up in the Info.plist file
     	SET(MACOSX_BUNDLE_INFO_STRING "Hydrogen for Mac")
	SET(MACOSX_BUNDLE_ICON_FILE "icon.icns")
	SET(MACOSX_BUNDLE_GUI_IDENTIFIER "Hydrogen")
	SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}")
	#this has to be short since it appears as the application name in the menu bar
	SET(MACOSX_BUNDLE_BUNDLE_NAME "Hydrogen")
	SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
	SET(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
	SET(MACOSX_BUNDLE_COPYRIGHT "Hydrogen Developers, 2009")

  	# set where in the bundle to put the icns file
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/data/img/gray/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	# include the icns file in the target
	SET(hydrogen_SRCS ${hydrogen_SRCS} ${CMAKE_SOURCE_DIR}/data/img/gray/icon.icns)
	
	#copy the data folder
	FILE(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION hydrogen.app/Contents/Resources PATTERN ".svn" EXCLUDE)
  	
ENDIF()


ADD_EXECUTABLE(hydrogen WIN32 MACOSX_BUNDLE ${hydrogen_SRCS} ${hydrogen_MOC} ${hydrogen_UIS_H} ${CMAKE_SOURCE_DIR}/windows/icon.rc)
TARGET_LINK_LIBRARIES(hydrogen
    hydrogen-core-${VERSION}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
)

ADD_DEPENDENCIES(hydrogen hydrogen-core-${VERSION})

INSTALL(TARGETS hydrogen RUNTIME DESTINATION bin BUNDLE DESTINATION bin )
