#
# CMAKE SETUP
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
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
SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "0")
SET(VERSION_PATCH "0")
#SET(VERSION_SUFFIX "beta2")

SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

IF(VERSION_SUFFIX)
	SET(DISPLAY_VERSION "${VERSION}-${VERSION_SUFFIX}")
ELSE()
	EXECUTE_PROCESS(COMMAND git log --pretty=format:'%h' -n 1 OUTPUT_VARIABLE GIT_REVISION )
	SET(DISPLAY_VERSION "${VERSION}-${GIT_REVISION}")
ENDIF(VERSION_SUFFIX)

include(GNUInstallDirs)

SET(LIBSNDFILE_VERSION_PREV "1.0.17")

#
# CONFIG OPTIONS
#
SET(WANT_LIBTAR TRUE)
OPTION(WANT_DEBUG           "Build with debug information" ON)
IF(APPLE)
    OPTION(WANT_SHARED      "Build the core library shared." OFF)
    OPTION(WANT_ALSA        "Include ALSA (Advanced Linux Sound Architecture) support" OFF)
ELSE()
    OPTION(WANT_SHARED      "Build the core library shared." ON)
    OPTION(WANT_ALSA        "Include ALSA (Advanced Linux Sound Architecture) support" ON)
ENDIF()

OPTION(WANT_LIBARCHIVE      "Enable use of libarchive instead of libtar" ON)
OPTION(WANT_LADSPA          "Enable use of LADSPA plugins" ON)
OPTION(WANT_LILV            "Enable use of LV2 plugins" ON)

IF(APPLE)
	OPTION(WANT_OSC  "Enable OSC support" OFF)
	OPTION(WANT_JACKSESSION "Enable use of Jack-Session-Handler" OFF)
ELSE()
	OPTION(WANT_JACKSESSION "Enable use of Jack-Session-Handler" ON)
	OPTION(WANT_OSC  "Enable OSC support" ON)
ENDIF()

IF("${CMAKE_SYSTEM_NAME}" MATCHES "NetBSD")
	OPTION(WANT_OSS          "Include OSS (Open Sound System) support" ON)
ELSE()
	OPTION(WANT_OSS          "Include OSS (Open Sound System) support" OFF)
ENDIF()

IF(MINGW)
	OPTION(WANT_PORTAUDIO    "Include PortAudio support" ON)
	OPTION(WANT_PORTMIDI     "Include PortMidi support" ON)
ELSE()
	OPTION(WANT_PORTAUDIO    "Include PortAudio support" OFF)
	OPTION(WANT_PORTMIDI     "Include PortMidi support" OFF)
ENDIF()
OPTION(WANT_JACK         "Include JACK (Jack Audio Connection Kit) support" ON)
OPTION(WANT_PULSEAUDIO   "Include PulseAudio support" ON)
OPTION(WANT_LASH         "Include LASH (Linux Audio Session Handler) support" OFF)
OPTION(WANT_LRDF         "Include LRDF (Lightweight Resource Description Framework with special support for LADSPA plugins) support" OFF)
OPTION(WANT_RUBBERBAND   "Include RubberBand (Audio Time Stretcher Library) support" OFF)
IF(APPLE)
    OPTION(WANT_COREAUDIO   "Include CoreAudio support" ON)
    OPTION(WANT_COREMIDI    "Include CoreMidi support" ON)
    OPTION(WANT_BUNDLE      "Build a MAC OSX bundle application" ON)
ENDIF()

OPTION(WANT_CPPUNIT         "Include CppUnit test suite" ON)

IF(WANT_DEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
    SET(H2CORE_HAVE_DEBUG TRUE)
ELSE()
    SET(CMAKE_BUILD_TYPE Release)
    SET(H2CORE_HAVE_DEBUG FALSE)
ENDIF()

OPTION(WANT_CLANG_TIDY "Use clang-tidy to check the sourcecode" OFF)
IF(WANT_CLANG_TIDY)
	find_program(CLANG_TIDY_CMD NAMES clang-tidy)
	IF(CLANG_TIDY_CMD)
		#use config from .clang-tidy
		SET(CMAKE_CXX_CLANG_TIDY clang-tidy)
	ENDIF()
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

IF(WANT_DEBUG)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
ELSE()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
ENDIF()

IF (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-implement-inlines")
ENDIF()

SET(CMAKE_CXX_FLAGS_RELEASE "")

SET(CMAKE_CXX_FLAGS_DEBUG "-g ")#-Winline")


IF(APPLE)
	#Compile for 32bit and 64bit intel macs
	SET (CMAKE_OSX_ARCHITECTURES "x86_64;")
ENDIF()
#
# MANDATORY PKGS AND DEFAULT OPTIONS
#
INCLUDE(StatusSupportOptions)
MANDATORY_PKG(Threads)
MANDATORY_PKG(LIBSNDFILE)

# http://doc.qt.io/qt-5/cmake-manual.html
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Test REQUIRED)
FIND_PACKAGE(Qt5Xml REQUIRED)
FIND_PACKAGE(Qt5XmlPatterns REQUIRED)
FIND_PACKAGE(Qt5LinguistTools REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

IF(APPLE)
    INCLUDE_DIRECTORIES("/opt/local/include")
    LINK_DIRECTORIES("/opt/local/lib")

    #Without setting this, installation would go into /usr/local, which does not exist per default
    SET(CMAKE_INSTALL_PREFIX "/usr" )
ELSE(APPLE)
    SET(OSS_LIB_PATHS "${CMAKE_INSTALL_FULL_LIBDIR}/oss/lib" "/usr/local/lib${LIB_SUFFIX}/oss/lib" )
ENDIF(APPLE)


#Installation paths
# TODO remove data from path
SET(H2_SYS_PATH "${CMAKE_INSTALL_FULL_DATAROOTDIR}/hydrogen")
# TODO remove data from path, could be .config/hydrogen
SET(H2_USR_PATH ".hydrogen")

SET(MAX_INSTRUMENTS 1000 CACHE STRING "Maximum number of instruments")
SET(MAX_COMPONENTS  32   CACHE STRING "Maximum number of components")
SET(MAX_NOTES       192  CACHE STRING "Maximum number of notes")
SET(MAX_FX          4    CACHE STRING "Maximum number of effects")
SET(MAX_BUFFER_SIZE 8192 CACHE STRING "Maximum size of buffer")

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
FIND_HELPER(LIBARCHIVE libarchive archive.h archive)
IF( WANT_LIBARCHIVE AND LIBARCHIVE_FOUND)
    SET(WANT_LIBTAR FALSE)
ENDIF()
FIND_HELPER(LIBSNDFILE sndfile sndfile.h sndfile)
FIND_HELPER(ALSA alsa alsa/asoundlib.h asound )
FIND_LADSPA(LADSPA ladspa.h noise)
FIND_HELPER(LILV lilv-0 lilv-0/lilv/lilv.h lilv-0)

FIND_HELPER(OSC liblo lo/lo.h lo)

IF("${CMAKE_SYSTEM_NAME}" MATCHES "NetBSD")
	FIND_HELPER(OSS oss sys/soundcard.h ossaudio )
ELSE()
	FIND_HELPER(OSS oss sys/soundcard.h OSSlib )
ENDIF()

FIND_HELPER(JACK jack jack/jack.h jack)
CHECK_LIBRARY_EXISTS(jack jack_port_rename "" HAVE_JACK_PORT_RENAME)
FIND_HELPER(JACKSESSION jack jack/session.h jack)
IF(APPLE)
    FIND_LIBRARY(AUDIOUNIT_LIBRARY AudioUnit)
    FIND_LIBRARY(CORESERVICES_LIBRARY CoreServices)
    FIND_HELPER(COREAUDIO CoreAudio-2.0 coreaudio.h coreaudio)
    FIND_HELPER(COREMIDI CoreMidi coremidi.h coremidi)
ENDIF()
FIND_HELPER(PORTAUDIO portaudio-2.0 portaudio.h portaudio)
FIND_HELPER(PORTMIDI portmidi portmidi.h portmidi)
FIND_HELPER(PULSEAUDIO libpulse pulse/pulseaudio.h pulse)
FIND_HELPER(LASH lash-1.0 lash/lash.h lash)
FIND_HELPER(LRDF lrdf lrdf.h lrdf)

FIND_HELPER(RUBBERBAND rubberband rubberband/RubberBandStretcher.h rubberband)
FIND_HELPER(CPPUNIT cppunit cppunit/TestCase.h cppunit)


# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
#set(CMAKE_AUTOMOC ON)


find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc_dir ALL COMMAND ${CMAKE_COMMAND}
		-E make_directory ${CMAKE_CURRENT_BINARY_DIR}/docs)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		DEPENDS doc_dir
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM)
endif(DOXYGEN_FOUND)

#
# COMPUTE H2CORE_HAVE_xxx xxx_STATUS_REPORT
#
SET(STATUS_LIST LIBSNDFILE LIBTAR LIBARCHIVE LADSPA ALSA OSS JACK JACKSESSION LILV OSC COREAUDIO COREMIDI PORTAUDIO PORTMIDI PULSEAUDIO LASH LRDF RUBBERBAND CPPUNIT )
FOREACH( _pkg ${STATUS_LIST})
    COMPUTE_PKGS_FLAGS(${_pkg})
ENDFOREACH()

# LIBSNDFILE CHECKS
STRING( COMPARE GREATER "${LIBSNDFILE_VERSION}" "${LIBSNDFILE_VERSION_PREV}" LIBSNDFILE_VERSION_OK)
IF(LIBSNDFILE_VERSION_OK)
    SET(LIBSNDFILE_MSG "libsndfile supports FLAC and OGG sound file formats")
ELSE()
    SET(LIBSNDFILE_MSG "libsndfile version must be greater than ${LIBSNDFILE_VERSION_PREV} to support FLAC and OGG sound file formats")
ENDIF()

# RUBBERBAND information
SET(LIBRUBBERBAND_MSG "The use of librubberband2 is marked as experimental.
*				 Because the current implementation produce wrong timing!
*				 So long this bug isn't solved, please disable this option.
*				 If rubberband-cli is installed, the hydrogen rubberband-function
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
* User path                    : ${H2_USR_PATH}
* System path                  : ${H2_SYS_PATH}
* core library build as        : ${H2CORE_LIBRARY_TYPE}
* debug capabilities           : ${H2CORE_HAVE_DEBUG}
* macosx bundle                : ${H2CORE_HAVE_BUNDLE}
* fat build                    : ${WANT_FAT_BUILD}\n"
)

COLOR_MESSAGE("${cyan}Main librarires${reset}")
COLOR_MESSAGE("* ${purple}libQt${reset}                        : ${Qt5Widgets_VERSION}")
COLOR_MESSAGE("* ${purple}libsndfile${reset}                   : ${LIBSNDFILE_STATUS}
*                                ${LIBSNDFILE_MSG}
* ${purple}libtar${reset}                       : ${LIBTAR_STATUS}
* ${purple}libarchive${reset}                   : ${LIBARCHIVE_STATUS}
* ${purple}ladspa${reset}                       : ${LADSPA_STATUS}\n
* ${purple}lv2${reset}                          : ${LILV_STATUS}\n"
)

COLOR_MESSAGE("${cyan}Supported audio interfaces${reset}
--------------------------
* ${purple}ALSA${reset}                         : ${ALSA_STATUS}
* ${purple}OSS${reset}                          : ${OSS_STATUS}
* ${purple}JACK${reset}                         : ${JACK_STATUS}
* ${purple}JACKSESSION${reset}                  : ${JACKSESSION_STATUS}
* ${purple}OSC${reset}                          : ${OSC_STATUS}
* ${purple}CoreAudio${reset}                    : ${COREAUDIO_STATUS}
* ${purple}CoreMidi${reset}                     : ${COREMIDI_STATUS}
* ${purple}PortAudio${reset}                    : ${PORTAUDIO_STATUS}
* ${purple}PortMidi${reset}                     : ${PORTMIDI_STATUS}
* ${purple}PulseAudio${reset}                   : ${PULSEAUDIO_STATUS}\n"
)

COLOR_MESSAGE("${cyan}Useful extensions${reset}
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
* working sscanf               : ${HAVE_SSCANF}
* unit tests                   : ${CPPUNIT_STATUS}\n"
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
IF(H2CORE_HAVE_CPPUNIT)
    ADD_SUBDIRECTORY(src/tests)
ENDIF()
ADD_SUBDIRECTORY(data/i18n)
ADD_SUBDIRECTORY(src/cli)
ADD_SUBDIRECTORY(src/player)
ADD_SUBDIRECTORY(src/gui)
IF(EXISTS ${CMAKE_SOURCE_DIR}/data/doc/CMakeLists.txt)
	ADD_SUBDIRECTORY(data/doc)
ENDIF()
IF (MINGW)
	INSTALL(DIRECTORY data DESTINATION . PATTERN ".git" EXCLUDE PATTERN "i18n" EXCLUDE PATTERN doc EXCLUDE)
ELSE()
	INSTALL(DIRECTORY data DESTINATION ${H2_SYS_PATH} PATTERN ".git" EXCLUDE PATTERN "i18n" EXCLUDE PATTERN doc EXCLUDE)
ENDIF()
IF(NOT MINGW AND NOT APPLE)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/linux/org.hydrogenmusic.Hydrogen.appdata.xml DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/appdata")
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/linux/org.hydrogenmusic.Hydrogen.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/data/img/gray/h2-icon.svg DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "org.hydrogenmusic.Hydrogen.svg")
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/linux/hydrogen.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
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
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Hydrogen")

IF(MINGW)
    #Set the other files that will be used in CPack
    SET(WIN64 "OFF" CACHE BOOL "Windows 64 Bit")
	SET(CPACK_STRIP_FILES "${WINDOWS_DIR}/src/cli/h2cli.exe" "${WINDOWS_DIR}/src/gui/hydrogen.exe" "${WINDOWS_DIR}/src/player/h2player.exe")
    #Program Files for Hydrogen
    SET(WINDOWS_DIR "windows")
    INSTALL(FILES "${WINDOWS_DIR}/src/cli/h2cli.exe" "${WINDOWS_DIR}/src/core/libhydrogen-core-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.dll" "${WINDOWS_DIR}/src/gui/hydrogen.exe" "${WINDOWS_DIR}/src/player/h2player.exe" DESTINATION ./)
    #Install files from the extralibs dir
    INSTALL(DIRECTORY ${WINDOWS_DIR}/extralibs/ DESTINATION ./)

	IF(WANT_FAT_BUILD)
		INSTALL(DIRECTORY windows/jack_installer DESTINATION ./)
		INSTALL(DIRECTORY windows/plugins DESTINATION ./)
	ENDIF()

    SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/data\\\\img\\\\h2-icon.bmp")
    #Begin NSIS Customizations
    # Installers for 32- vs. 64-bit CMake:
    #  - Root install directory (displayed to end user at installer-run time)
    #  - "NSIS package/display name" (text used in the installer GUI)
    #  - Registry key used to store info about the installation
    IF(WIN64)
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		SET(CPACK_NSIS_PACKAGE_NAME "Hydrogen - ${DISPLAY_VERSION} 64Bit")
		SET(CPACK_PACKAGE_FILE_NAME "Hydrogen-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-win64")
		#SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}${CPACK_PACKAGE_VERSION} 64Bit")
    ELSE()
		SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
		SET(CPACK_NSIS_PACKAGE_NAME "Hydrogen - ${DISPLAY_VERSION}")
		SET(CPACK_PACKAGE_FILE_NAME "Hydrogen-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-win32")
		#SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}${CPACK_PACKAGE_VERSION}")
    ENDIF()

    #Need the 2 following lines for the icon to work
    SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/data\\\\img\\\\h2-icon.ico")
    SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/data\\\\img\\\\h2-icon.ico")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "hydrogen.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "Hydrogen (Advanced drum machine for GNU/Linux)")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.hydrogen-music.org/")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.hydrogen-music.org/")
    SET(CPACK_NSIS_CONTACT "hydrogen-devel@lists.sourceforge.net")
    SET(CPACK_PACKAGE_EXECUTABLES "hydrogen.exe;Hydrogen drum machine")
    SET(CPACK_NSIS_MENU_LINKS "hydrogen.exe;Hydrogen drum machine")
    SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ON)
    SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    #end NSIS customizations
ELSE(MINGW)
    #apple stuff was moved to src/gui/CMakeLists.txt
ENDIF()

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "hydrogen")
SET(CPACK_SOURCE_IGNORE_FILES ".*~;\\\\.git;\\\\.svn;${CMAKE_BINARY_DIR}")

#!The following 5 lines are copied from cmake's QtTest example

# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well
set(CPACK_BINARY_DRAGNDROP ON)
INCLUDE(CPack)

#
# CUSTOM TARGETS
#
ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake.in" "${CMAKE_BINARY_DIR}/uninstall.cmake" IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/uninstall.cmake")
