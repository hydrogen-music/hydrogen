
FILE(GLOB_RECURSE hydrogen_UIS src/*.ui)
FILE(GLOB_RECURSE hydrogen_SRCS src/*.cpp)
FILE(GLOB_RECURSE hydrogen_H src/*.h)

QT4_WRAP_UI(hydrogen_UIS_H ${hydrogen_UIS}) # ui_ files are generated in CMAKE_CURRENT_BINARY_DIR
QT4_WRAP_CPP(hydrogen_MOC ${hydrogen_H} OPTIONS -nw) 

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}                         # ui_ headers
    ${CMAKE_SOURCE_DIR}                                 # TODO remove when gui and core are nicely splitted
    ${CMAKE_SOURCE_DIR}/libs/hydrogen/include           # core headers
    #${CMAKE_BINARY_DIR}/libs/hydrogen/include/hydrogen  # TODO remove
    ${CMAKE_CURRENT_SOURCE_DIR}/src                     # gui headers 
    #${CMAKE_SOURCE_DIR}/src/core/include                # core headers TODO enable
    #${CMAKE_BINARY_DIR}/src/core/include                # core built headers TODO enable
    ${LASH_INCLUDE_DIR}
    ${QT_INCLUDES}
)

ADD_EXECUTABLE(hydrogen WIN32 MACOSX_BUNDLE ${hydrogen_SRCS} ${hydrogen_MOC} ${hydrogen_UIS_H})
TARGET_LINK_LIBRARIES(hydrogen
    hydrogen-core
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
)

INSTALL(TARGETS hydrogen RUNTIME DESTINATION bin)
