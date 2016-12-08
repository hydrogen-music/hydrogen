
FILE(GLOB_RECURSE h2synth_SRCS *.cpp)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/include        # core headers
    ${CMAKE_BINARY_DIR}/src/core/include        # generated config.h
    ${QT_INCLUDES}
)

ADD_EXECUTABLE(h2synth WIN32 MACOSX_BUNDLE ${h2synth_SRCS} )
IF(WANT_QT5)
	TARGET_LINK_LIBRARIES(h2synth
		hydrogen-core-${VERSION}
		Qt5::Widgets
	)
ELSE()
	TARGET_LINK_LIBRARIES(h2synth
		hydrogen-core-${VERSION}
		${QT_QTGUI_LIBRARY}
	)
ENDIF()

ADD_DEPENDENCIES(h2synth hydrogen-core-${VERSION})

INSTALL(TARGETS h2synth RUNTIME DESTINATION bin BUNDLE DESTINATION bin )
