
FILE(GLOB_RECURSE hydrogen_UIS src/*.ui)
FILE(GLOB_RECURSE hydrogen_SRCS src/*.cpp)
FILE(GLOB_RECURSE hydrogen_H src/*.h)

# add undiscovered dependencies to generated config.h
INCLUDE(AddFileDependencies)
SET( CORE_DEP_FILES_LIST
    src/main.cpp
    src/HydrogenApp.cpp
    src/LadspaFXProperties.cpp
    src/LadspaFXSelector.cpp
    src/MainForm.cpp
    src/Mixer/Mixer.cpp
    src/PatternEditor/DrumPatternEditor.cpp
    src/PlayerControl.cpp
    src/PreferencesDialog.cpp )
FOREACH( _file ${CORE_DEP_FILES_LIST})
    ADD_FILE_DEPENDENCIES( ${_file} ${CMAKE_BINARY_DIR}/libs/hydrogen/include/hydrogen/config.h)
ENDFOREACH()

QT4_WRAP_UI(hydrogen_UIS_H ${hydrogen_UIS}) # ui_ files are generated in CMAKE_CURRENT_BINARY_DIR
QT4_WRAP_CPP(hydrogen_MOC ${hydrogen_H} OPTIONS -nw) 

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}                                 # TODO remove when gui and core are nicely splitted
    ${CMAKE_CURRENT_BINARY_DIR}                         # ui_ headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src                     # gui headers 
    ${CMAKE_SOURCE_DIR}/libs/hydrogen/include           # core headers
    ${LASH_INCLUDE_DIR}
    ${QT_INCLUDES}
)

ADD_EXECUTABLE(hydrogen WIN32 MACOSX_BUNDLE ${hydrogen_SRCS} ${hydrogen_MOC} ${hydrogen_UIS_H})
TARGET_LINK_LIBRARIES(hydrogen
    hydrogen-core
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
)

ADD_DEPENDENCIES(hydrogen hydrogen-core)

INSTALL(TARGETS hydrogen RUNTIME DESTINATION bin BUNDLE DESTINATION bin )
