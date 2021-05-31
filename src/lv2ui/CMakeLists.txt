IF(LV2_FOUND)

FILE(GLOB_RECURSE hydrogen_INCLUDES *.h)
FILE(GLOB_RECURSE hydrogen_SOURCES *.cpp)
LIST(APPEND hydrogen_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/include/hydrogen/config.h)

FIND_PACKAGE(PkgConfig REQUIRED)

INCLUDE_DIRECTORIES( include
    ${CMAKE_SOURCE_DIR}/include                 # regular headers
    ${CMAKE_CURRENT_BINARY_DIR}/include         # generated config.h
    ${CMAKE_SOURCE_DIR}/src/core/include        # core headers	
    ${QT_INCLUDES}
)

ADD_LIBRARY( hydrogen-lv2ui-${VERSION} ${H2CORE_LIBRARY_TYPE} ${hydrogen_SOURCES})

TARGET_LINK_LIBRARIES(hydrogen-lv2ui-${VERSION}
	hydrogen-core-${VERSION}
	Qt5::Core
	Qt5::Widgets
	Qt5::Xml
	Qt5::XmlPatterns
	${GTK3_LIBRARIES}
)
ADD_DEPENDENCIES(hydrogen hydrogen-core-${VERSION})

SET_PROPERTY(TARGET hydrogen-lv2ui-${VERSION} PROPERTY CXX_STANDARD 14)
INSTALL(TARGETS hydrogen-lv2ui-${VERSION} LIBRARY DESTINATION "/usr/lib/lv2/hydrogen.lv2/")

ENDIF()
