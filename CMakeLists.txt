#
# CMAKE SETUP
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
# The CMake Policy mechanism is designed to help keep existing projects building as new versions of CMake introduce changes in behavior.
# http://www.cmake.org/cmake/help/cmake2.6docs.html#command:cmake_policy
IF(COMMAND CMAKE_POLICY)
#	CMAKE_POLICY(SET CMP0005 NEW)
	CMAKE_POLICY(SET CMP0003 NEW)   # add_library
	CMAKE_POLICY(SET CMP0006 NEW)   # bundle destination property
ENDIF()

#
# PROJECT INFO
#
PROJECT(hydrogen)
SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "9")
SET(VERSION_PATCH "6")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
IF(VERSION_SUFFIX)
    SET(VERSION "${VERSION}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)
SET(LIBSNDFILE_VERSION_PREV "1.0.17")

#
# CONFIG OPTIONS
#
SET(WANT_LIBTAR TRUE)
OPTION(WANT_DEBUG       "build with debug information" ON)
IF(APPLE)
    OPTION(WANT_SHARED      "Build the core library shared." OFF)
ELSE()
    OPTION(WANT_SHARED      "Build the core library shared." ON)
ENDIF()
OPTION(WANT_LIBARCHIVE   "Enable use of libarchive instead of libtar" OFF)
OPTION(WANT_LADSPA       "Enable use of LADSPA plugins" ON)
OPTION(WANT_JACKSESSION "Enable use of Jack-Session-Handler" ON)
OPTION(WANT_OSS          "Include OSS (Open Sound System) support" OFF)
OPTION(WANT_ALSA         "Include ALSA (Advanced Linux Sound Architecture) support" ON)
OPTION(WANT_JACK         "Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_PORTAUDIO    "Include PortAudio support" OFF)
OPTION(WANT_PORTMIDI     "Include PortMidi support" OFF)
OPTION(WANT_LASH         "Include LASH (Linux Audio Session Handler) support" OFF)
OPTION(WANT_LRDF         "Include LRDF (Lightweight Resource Description Framework with special support for LADSPA plugins) support" OFF)
OPTION(WANT_RUBBERBAND   "Include RUbberBand (Audio Time Stretcher Library) support" OFF)
IF(APPLE)
    OPTION(WANT_COREAUDIO   "Include CoreAudio support" ON)
    OPTION(WANT_COREMIDI    "Include CoreMidi support" ON)
    OPTION(WANT_BUNDLE  "Build a MAC OSX bundle application" OFF)
ENDIF()

IF(WANT_DEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
    SET(H2CORE_HAVE_DEBUG TRUE)
ELSE()
    SET(CMAKE_BUILD_TYPE Release)
    SET(H2CORE_HAVE_DEBUG FALSE)
ENDIF()
IF(WANT_BUNDLE)
    SET(H2CORE_HAVE_BUNDLE TRUE)
ELSE()
    SET(H2CORE_HAVE_BUNDLE FALSE)
ENDIF()
IF(WANT_SHARED)
    SET(H2CORE_LIBRARY_TYPE SHARED)
ELSE()
    SET(H2CORE_LIBRARY_TYPE STATIC)
ENDIF()
#SET(CMAKE_CXX_FLAGS "-fno-implement-inlines -finline-small-functions -findirect-inlining -fpartial-inlining")
SET(CMAKE_CXX_FLAGS "-O2 -fno-implement-inlines")
SET(CMAKE_CXX_FLAGS_RELEASE "")
SET(CMAKE_CXX_FLAGS_DEBUG "-g ")#-Winline")
#
# MANDATORY PKGS AND DEFAULT OPTIONS
#
INCLUDE(StatusSupportOptions)
MANDATORY_PKG(QT4)
MANDATORY_PKG(Threads)
MANDATORY_PKG(LIBSNDFILE)

# TODO remove data from path
SET(SYS_DATA_PATH "${CMAKE_INSTALL_PREFIX}/share/hydrogen/data")
# TODO remove data from path, could be .config/hydrogen
SET(USR_DATA_PATH ".hydrogen/data")
IF(WIN32)
    SET(WANT_JACK FALSE)
    SET(Threads_PATH "C:/Program Files/GnuWin32")
    SET(LIBARCHIVE_PATH "C:/Program Files/GnuWin32")
    SET(LIBSNDFILE_PATH "C:/Program Files/Mega-Nerd/libsndfile")
ELSE(WIN32)
    IF(APPLE)
        LINK_DIRECTORIES("/opt/local/lib")
    ELSE(APPLE)
        SET(OSS_LIB_PATHS "/usr/lib/oss/lib" "/usr/local/lib/oss/lib" )
        SET(RUBBERBAND_INCLUDE_PATHS "/usr/include/rubberband" )
    ENDIF(APPLE)
ENDIF(WIN32)

EXECUTE_PROCESS(COMMAND svnversion -n OUTPUT_VARIABLE REVISION )

#
# HEADER LIBRARY FUNCTIONS
#
INCLUDE(CompileHelper)
INCLUDE(FindHelper)
INCLUDE(FindLadspa)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(FindZLIB)
INCLUDE(FindThreads)
COMPILE_HELPER(SSCANF ${CMAKE_SOURCE_DIR}/cmake/sscanf sscanf )
COMPILE_HELPER(RTCLOCK ${CMAKE_SOURCE_DIR}/cmake/rtclock rtclock )
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(libtar.h HAVE_LIBTAR_H)
CHECK_LIBRARY_EXISTS(tar tar_open "" HAVE_LIBTAR_OPEN)
CHECK_LIBRARY_EXISTS(tar tar_close "" HAVE_LIBTAR_CLOSE)
CHECK_LIBRARY_EXISTS(tar tar_extract_all "" HAVE_LIBTAR_EXTRACT_ALL)
IF(HAVE_LIBTAR_H AND HAVE_LIBTAR_OPEN AND HAVE_LIBTAR_CLOSE AND HAVE_LIBTAR_EXTRACT_ALL)
    SET(LIBTAR_OK TRUE)
ELSE()
    SET(LIBTAR_OK FALSE)
ENDIF()
FIND_HELPER(LIBTAR tar tar.h tar)
IF( NOT LIBTAR_FOUND OR NOT LIBTAR_OK OR NOT ZLIB_FOUND )
    SET(WANT_LIBTAR FALSE)
    MANDATORY_PKG(LIBARCHIVE)
ENDIF()
FIND_HELPER(LIBARCHIVE archive archive.h archive)
IF( WANT_LIBARCHIVE AND LIBARCHIVE_FOUND)
    SET(WANT_LIBTAR FALSE)
ENDIF()
FIND_HELPER(LIBSNDFILE sndfile sndfile.h sndfile)
FIND_HELPER(ALSA alsa alsa/asoundlib.h asound )
FIND_LADSPA(LADSPA ladspa.h noise)
FIND_HELPER(OSS oss sys/soundcard.h OSSlib )
FIND_HELPER(JACK jack jack/jack.h jack)
FIND_HELPER(JACKSESSION jack jack/session.h jack)
IF(APPLE)
    FIND_LIBRARY(AUDIOUNIT_LIBRARY AudioUnit)
    FIND_LIBRARY(CORESERVICES_LIBRARY CoreServices)
    FIND_HELPER(COREAUDIO CoreAudio-2.0 coreaudio.h coreaudio)
    FIND_HELPER(COREMIDI CoreMidi coremidi.h coremidi)
ENDIF()
FIND_HELPER(PORTAUDIO portaudio-2.0 portaudio.h portaudio)
FIND_HELPER(PORTMIDI portmidi portmidi.h portmidi)
FIND_HELPER(LASH lash-1.0 lash/lash.h lash)
FIND_HELPER(LRDF lrdf lrdf.h lrdf)
FIND_HELPER(RUBBERBAND rubberband RubberBandStretcher.h rubberband)
# CHECK FOR Qt4
SET(QT_USE_QTNETWORK true)
SET(QT_USE_QTXML true)
SET(QT_USE_QTXMLPATTERNS true)
FIND_PACKAGE(Qt4 4.3.0 REQUIRED)

# DOXYGEN
SET(ENABLE_LATEX false)
SET(DOXYFILE_HTML_DIR   "${CMAKE_BINARY_DIR}/html" )
SET(DOXYFILE_LATEX_DIR  "${CMAKE_BINARY_DIR}/latex" )
include(doxygen OPTIONAL)
#
# COMPUTE H2CORE_HAVE_xxx xxx_STATUS_REPORT
#
SET(STATUS_LIST LIBSNDFILE LIBTAR LIBARCHIVE LADSPA ALSA OSS JACK JACKSESSION COREAUDIO COREMIDI PORTAUDIO PORTMIDI LASH LRDF RUBBERBAND )
FOREACH( _pkg ${STATUS_LIST})
    COMPUTE_PKGS_FLAGS(${_pkg})
ENDFOREACH()

# LIBSNDFILE CHECKS
STRING( COMPARE GREATER "${LIBSNDFILE_VERSION}" "${LIBSNDFILE_VERSION_PREV}" LIBSNDFILE_VERSION_OK)
IF(LIBSNDFILE_VERSION_OK)
    SET(LIBSNDFILE_MSG "libsndfile supports FLAC and OGG sound file formats")
ELSE()
    SET(LIBSNDFILE_MSG "libsndfile version must be greater than ${LIBSNDFILE_VERSION_PREV} to support LFAC and OGG sound file formats")
ENDIF()

# RUBBERBAND information
SET(LIBRUBBERBAND_MSG "The use of librubberband2 is marked as experimental.
*				 Because the current implementation produce wrong timing!
*				 So long this bug isn't solved, please disable this option.
*				 If rubberband-cli is installed, the hydrogen rubberband-fuction
*				 will work properly as expected.")

#
# CONFIG PROCESS SUMMARY
#
SET(reset "${_escape}[0m")
SET(red "${_escape}[1;31m")
SET(purple "${_escape}[1;35m")
SET(cyan "${_escape}[1;36m")

COLOR_MESSAGE("${cyan}Installation Summary${reset}
--------------------
* Install Directory            : ${CMAKE_INSTALL_PREFIX}
* User data path               : ${USR_DATA_PATH}
* System data path             : ${SYS_DATA_PATH}
* core library build as        : ${H2CORE_LIBRARY_TYPE}
* debug capabilities           : ${H2CORE_HAVE_DEBUG}
* macosx bundle                : ${H2CORE_HAVE_BUNDLE}\n"
)

COLOR_MESSAGE("${cyan}Main librarires${reset}
* ${purple}libQt${reset}                        : ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}
* ${purple}libsndfile${reset}                   : ${LIBSNDFILE_STATUS}
*                                ${LIBSNDFILE_MSG}
* ${purple}libtar${reset}                       : ${LIBTAR_STATUS}
* ${purple}libarchive${reset}                   : ${LIBARCHIVE_STATUS}
* ${purple}ladspa${reset}                       : ${LADSPA_STATUS}\n"
)

COLOR_MESSAGE("${cyan}Supported audio interfaces${reset}
--------------------------
* ${purple}ALSA${reset}                         : ${ALSA_STATUS}
* ${purple}OSS${reset}                          : ${OSS_STATUS}
* ${purple}JACK${reset}                         : ${JACK_STATUS}
* ${purple}JACKSESSION${reset}                  : ${JACKSESSION_STATUS}
* ${purple}CoreAudio${reset}                    : ${COREAUDIO_STATUS}
* ${purple}CoreMidi${reset}                     : ${COREMIDI_STATUS}
* ${purple}PortAudio${reset}                    : ${PORTAUDIO_STATUS}
* ${purple}PortMidi${reset}                     : ${PORTMIDI_STATUS}\n"
)

COLOR_MESSAGE("${cyan}Usefull extensions${reset}
-----------------------------------------
* ${purple}LASH${reset}                         : ${LASH_STATUS}
* ${purple}LRDF${reset}                         : ${LRDF_STATUS}
* ${purple}RUBBERBAND${reset}                   : ${RUBBERBAND_STATUS}
*                                ${LIBRUBBERBAND_MSG}\n"
)

IF(WANT_DEBUG)
    COLOR_MESSAGE("${cyan}Miscellaneous capabilities${reset}
-----------------------------------------
* realtime clock               : ${HAVE_RTCLOCK}
* working sscanf               : ${HAVE_SSCANF}\n"
    )
ENDIF()

COLOR_MESSAGE("-----------------------------------------------------------------
${red}IMPORTANT:${reset}
  after installing missing packages, remove ${CMAKE_BINARY_DIR}/CMakeCache.txt before
  running cmake again!
-----------------------------------------------------------------\n"
)

find_path( HYDROGEN_INSTALLED NAMES hydrogen/config.h )
IF( HYDROGEN_INSTALLED )
    COLOR_MESSAGE("-----------------------------------------------------------------
${red}IMPORTANT${reset}:
  previously installed hydrogen headers found in ${HYDROGEN_INSTALLED}
  you should uninstall these files before building hydrogen unless you know what you are doing.
-----------------------------------------------------------------\n"
    )
ENDIF()

#
# SET BUILD INFORMATION
#
ADD_SUBDIRECTORY(src/core)
ADD_SUBDIRECTORY(src/tests)
ADD_SUBDIRECTORY(src/cli)
ADD_SUBDIRECTORY(src/player)
ADD_SUBDIRECTORY(src/synth)
ADD_SUBDIRECTORY(src/gui)

INSTALL(DIRECTORY data DESTINATION ${SYS_DATA_PATH}/.. PATTERN ".svn" EXCLUDE)
IF(NOT WIN32 AND NOT APPLE)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/linux/hydrogen.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
ENDIF()

#
# CPack
#
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hydrogen : an advanced drum machine for GNU/Linux")
SET(CPACK_PACKAGE_VENDOR "Hydrogen Developers")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")

SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
IF(VERSION_SUFFIX)
    SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Hydrogen ${VERSION}")

IF(WIN32)
    # TODO
    #SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/data\\\\nsis_branding.bmp")
    SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/data\\\\hydrogen.ico")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "hydrogen.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "Hydrogen (Advanced drum machine for GNU/Linux)")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.hydrogen-music.org/")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.hydrogen-music.org/")
    SET(CPACK_NSIS_CONTACT "hydrogen-devel@lists.sourceforge.net")
    SET(CPACK_PACKAGE_EXECUTABLES "hydrogen-gui.exe;Advanced drum machine for GNU/Linux")
    SET(CPACK_NSIS_MENU_LINKS "hydrogen-gui.exe;Advanced drum machine for GNU/Linux")
    SET(CPACK_SOURCE_GENERATOR "ZIP")
ELSE(WIN32)
    IF(APPLE)
        IF(WANT_BUNDLE)
            #http://www.cmake.org/cmake/help/cmake-2-8-docs.html#prop_tgt:MACOSX_BUNDLE
            SET(MACOSX_BUNDLE_INFO_STRING "wazza?!?")
            SET(MACOSX_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/data/img/gray/icon.svg")
            SET(MACOSX_BUNDLE_GUI_IDENTIFIER "Hydrogen")
            SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}")
            SET(MACOSX_BUNDLE_BUNDLE_NAME "Hydrogen an andvanced drum machine for GNU/Linux")
            SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
            SET(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
            SET(MACOSX_BUNDLE_COPYRIGHT "Hydrogen Developers, 2009")
        ELSE()
            SET(CPACK_STRIP_FILES "")
            SET(CPACK_PACKAGE_EXECUTABLES "hydrogen-gui" "Hydrogen binary")
            SET(CPACK_SOURCE_GENERATOR "TBZ2")
        ENDIF()
    ELSE()
        SET(CPACK_STRIP_FILES "")
        SET(CPACK_PACKAGE_EXECUTABLES "hydrogen-gui" "Hydrogen binary")
        SET(CPACK_SOURCE_GENERATOR "TBZ2")
    ENDIF()
ENDIF()

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "hydrogen-${VERSION}")
SET(CPACK_SOURCE_IGNORE_FILES ".*~;\\\\.git;\\\\.svn;${CMAKE_BINARY_DIR}")

INCLUDE(CPack)

#
# CUSTOM TARGETS
#
ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake.in" "${CMAKE_BINARY_DIR}/uninstall.cmake" IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/uninstall.cmake")
