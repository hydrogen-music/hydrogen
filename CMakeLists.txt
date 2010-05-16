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
OPTION(WANT_LIBARCHIVE  "Enable use of libarchive instead of libtar" OFF)
OPTION(WANT_LADSPA      "Enable use of LADSPA plugins" ON)
OPTION(WANT_OSS         "Include OSS (Open Sound System) support" OFF)
OPTION(WANT_ALSA        "Include ALSA (Advanced Linux Sound Architecture) support" OFF)
OPTION(WANT_JACK        "Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_PORTAUDIO   "Include PortAudio support" OFF)
OPTION(WANT_PORTMIDI    "Include PortMidi support" OFF)
OPTION(WANT_LASH        "Include LASH (Linux Audio Session Handler) support" OFF)
OPTION(WANT_LRDF        "Include LRDF (Lightweight Resource Description Framework with special support for LADSPA plugins) support" OFF)
OPTION(WANT_FLAC        "Include FLAC (Free Lossless Audio Codec) support" OFF)
IF(APPLE)
    OPTION(WANT_COREAUDIO   "Include CoreAudio support" ON)
    OPTION(WANT_COREMIDI    "Include CoreMidi support" ON)
    OPTION(WANT_BUNDLE  "Build a MAC OSX bundle application" OFF)
ENDIF()

IF(WANT_SHARED)
    SET(H2CORE_LIBRARY_TYPE SHARED)
ELSE()
    SET(H2CORE_LIBRARY_TYPE STATIC)
ENDIF()
IF(WANT_DEBUG)
    SET(H2CORE_HAVE_DEBUG TRUE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
ELSE()
    SET(H2CORE_HAVE_DEBUG FALSE)
ENDIF()
IF(WANT_BUNDLE)
    SET(H2CORE_HAVE_BUNDLE TRUE)
ELSE()
    SET(H2CORE_HAVE_BUNDLE FALSE)
ENDIF()

#
# MANDATORY PKGS AND DEFAULT OPTIONS
#
INCLUDE(StatusSupportOptions)
MANDATORY_PKG(QT4)
MANDATORY_PKG(Threads)
MANDATORY_PKG(LIBSNDFILE)

SET(DATA_PATH "${CMAKE_INSTALL_PREFIX}/share/hydrogen/data")
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
        #EXECUTE_PROCESS(COMMAND svnversion -n OUTPUT_VARIABLE SVN_VERSION )
    ENDIF(APPLE)
ENDIF(WIN32)

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
FIND_HELPER(FLAC flac++ FLAC++/all.h FLAC++)
# CHECK FOR Qt4
SET(QT_USE_QTNETWORK true)
SET(QT_USE_QTXML true)
FIND_PACKAGE(Qt4 4.3.0 REQUIRED)

#
# COMPUTE H2CORE_HAVE_xxx xxx_STATUS_REPORT
#
SET(STATUS_LIST LIBSNDFILE LIBTAR LIBARCHIVE LADSPA ALSA OSS JACK COREAUDIO COREMIDI PORTAUDIO PORTMIDI LASH LRDF FLAC )
FOREACH( _pkg ${STATUS_LIST})
    COMPUTE_PKGS_FLAGS(${_pkg})
ENDFOREACH()

#
# CONFIG PROCESS SUMMARY
#
MESSAGE("\n"
"Installation Summary\n"
"--------------------\n"
"* Install Directory            : ${CMAKE_INSTALL_PREFIX}\n"
"* Data path                    : ${DATA_PATH}\n"
"* core library build as        : ${H2CORE_LIBRARY_TYPE}\n"
"* debug capabilities           : ${H2CORE_HAVE_DEBUG}\n"
"* macosx bundle                : ${H2CORE_HAVE_BUNDLE}\n"
)

MESSAGE("\n"
"Main librarires\n"
"* Qt4 version                  : ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}\n"
"* libsndfile                   : ${LIBSNDFILE_STATUS}\n"
"* libtar                       : ${LIBTAR_STATUS}\n"
"* libarchive                   : ${LIBARCHIVE_STATUS}\n"
"* ladspa                       : ${LADSPA_STATUS}\n"
)

MESSAGE(
"Supported audio interfaces\n"
"--------------------------\n"
"* ALSA                         : ${ALSA_STATUS}\n"
"* OSS                          : ${OSS_STATUS}\n"
"* JACK                         : ${JACK_STATUS}\n"
"* CoreAudio                    : ${COREAUDIO_STATUS}\n"
"* CoreMidi                     : ${COREMIDI_STATUS}\n"
"* PortAudio                    : ${PORTAUDIO_STATUS}\n"
"* PortMidi                     : ${PORTMIDI_STATUS}\n"
)

MESSAGE(
"Usefull extensions\n"
"-----------------------------------------\n"
"* LASH                         : ${LASH_STATUS}\n"
"* LRDF                         : ${LRDF_STATUS}\n"
)

MESSAGE(
"Supported file formats for samples\n"
"-----------------------------------------\n"
"* WAVE                         : builtin\n"
"* FLAC                         : ${FLAC_STATUS}\n"
)
IF(WANT_DEBUG)
    MESSAGE(
    "Debug capabilities\n"
    "-----------------------------------------\n"
    "* realtime clock           : ${HAVE_RTCLOCK}\n"
    "* working sscanf           : ${HAVE_SSCANF}\n"
    )
ENDIF()

MESSAGE(
"\n"
"-----------------------------------------------------------------\n"
"IMPORTANT:\n"
"  after installing missing packages, remove ${CMAKE_BINARY_DIR}/CMakeCache.txt before\n"
"  running cmake again!\n"
"-----------------------------------------------------------------\n"
)

find_path( HYDROGEN_INSTALLED NAMES hydrogen/config.h )
IF( HYDROGEN_INSTALLED )
    MESSAGE(
    "-----------------------------------------------------------------\n"
    "IMPORTANT:\n"
    "  previously installed hydrogen headers found in ${HYDROGEN_INSTALLED}\n"
    "  you should uninstall these files before building hydrogen unless you know what you are doing.\n"
    "-----------------------------------------------------------------\n"
    )
ENDIF()

#
# SET BUILD INFORMATION
#
ADD_SUBDIRECTORY(libs/hydrogen)
ADD_SUBDIRECTORY(gui)
#ADD_SUBDIRECTORY(src/core)
#ADD_SUBDIRECTORY(src/cli)
#ADD_SUBDIRECTORY(src/player)
#ADD_SUBDIRECTORY(src/synth)
#ADD_SUBDIRECTORY(src/gui)
INSTALL(DIRECTORY data DESTINATION ${DATA_PATH}/..)
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
SET(CPACK_SOURCE_IGNORE_FILES ".*~;\\\\.git;${CMAKE_BINARY_DIR}")
INCLUDE(CPack)

#
# CUSTOM TARGETS
#
ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

SET(DOXYGEN_OUTPUT "html")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY )
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_BINARY_DIR}/${DOXYGEN_OUTPUT}/index.html
    COMMAND doxygen ${CMAKE_BINARY_DIR}/Doxyfile
    DEPENDS ${CMAKE_BINARY_DIR}/Doxyfile
)
ADD_CUSTOM_TARGET(doxygen DEPENDS ${CMAKE_BINARY_DIR}/${DOXYGEN_OUTPUT}/index.html)

CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake.in" "${CMAKE_BINARY_DIR}/uninstall.cmake" IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/uninstall.cmake")
