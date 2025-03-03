#
# CMAKE SETUP
#

cmake_minimum_required(VERSION 3.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
# The CMake Policy mechanism is designed to help keep existing projects building as new versions of CMake introduce changes in behavior.
# http://www.cmake.org/cmake/help/cmake2.6docs.html#command:cmake_policy
if(COMMAND CMAKE_POLICY)
    #	cmake_policy(SET CMP0005 NEW)
	cmake_policy(SET CMP0003 NEW)   # add_library
	cmake_policy(SET CMP0006 NEW)   # bundle destination property
endif()

project(hydrogen)

## Determine proper semantic version
set(VERSION_MAJOR "2")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")
set(VERSION_SUFFIX "pre-alpha")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if(VERSION_SUFFIX)
    set(VERSION "${VERSION}-${VERSION_SUFFIX}")
endif()

# Consider any tagged commit as a release build
execute_process(COMMAND git describe --exact-match --tags OUTPUT_VARIABLE GIT_ON_TAG)
if(GIT_ON_TAG)
    set(IS_DEVEL_BUILD "false")
else()
    set(IS_DEVEL_BUILD "true")
endif()

# In order to avoid for things to get out of sync, it is also possible to hand
# over a version (from e.g. the build pipeline) and bypass the version string
# composition in here. But this only affects display string. Major, minor, and
# patch version in here must still match the TARGET_VERSION of the pipeline!
set(DISLPAY_VERSION_PIPELINE "" CACHE INTERNAL "")

if(NOT ${DISPLAY_VERSION_PIPELINE} STREQUAL "")
    set(DISPLAY_VERSION "${DISPLAY_VERSION_PIPELINE}")
else()
    set(DISPLAY_VERSION "${VERSION}")

    if(NOT GIT_ON_TAG)
        # For developer builds we append some additional data to version.
        execute_process(COMMAND git describe --abbrev=0
            OUTPUT_VARIABLE GIT_LAST_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)

        if("${DISPLAY_VERSION}" STREQUAL "${GIT_LAST_TAG}")
            # If the last tag matches the display version (we are working on the
            # same branch the tag is located in - the release branch), we will use
            # `git describe` since it nicely adds the number of commits passed since
            # the tag next to the current commit id.
            execute_process(COMMAND git describe --tags
                OUTPUT_VARIABLE GIT_DESCRIPTION OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(DISPLAY_VERSION "${GIT_DESCRIPTION}")
            message(STATUS "MATCH")
        else()
            # We are working on a release which is not a patch release of the last
            # one (feature branch, develop branch, master after splitting of a
            # dedicated release branch). We just add the commit hash to the display
            # version.
            execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(DISPLAY_VERSION "${DISPLAY_VERSION}-${GIT_REVISION}")
            message(STATUS "MIS")
        endif()
    endif()
endif()
message(STATUS "${DISPLAY_VERSION}")

#
# CONFIG OPTIONS
#
set(WANT_LIBTAR TRUE)
option(WANT_DEBUG           "Build with debug information" ON)
if(APPLE)
    option(WANT_SHARED      "Build the core library shared." OFF)
    option(WANT_ALSA        "Include ALSA (Advanced Linux Sound Architecture) support" OFF)
else()
    option(WANT_SHARED      "Build the core library shared." ON)
    option(WANT_ALSA        "Include ALSA (Advanced Linux Sound Architecture) support" ON)
endif()

option(WANT_LIBARCHIVE      "Enable use of libarchive instead of libtar" ON)
option(WANT_LADSPA          "Enable use of LADSPA plugins" ON)

if(APPLE)
	option(WANT_OSC  "Enable OSC support" OFF)
else()
	option(WANT_OSC  "Enable OSC support" ON)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "NetBSD")
	option(WANT_OSS          "Include OSS (Open Sound System) support" ON)
else()
	option(WANT_OSS          "Include OSS (Open Sound System) support" OFF)
endif()

if(MINGW)
	option(WANT_PORTAUDIO    "Include PortAudio support" ON)
	option(WANT_PORTMIDI     "Include PortMidi support" ON)
else()
	option(WANT_PORTAUDIO    "Include PortAudio support" OFF)
	option(WANT_PORTMIDI     "Include PortMidi support" OFF)
endif()
option(WANT_JACK         "Include JACK (Jack Audio Connection Kit) support" ON)
option(WANT_PULSEAUDIO   "Include PulseAudio support" ON)
option(WANT_LRDF         "Include LRDF (Lightweight Resource Description Framework with special support for LADSPA plugins) support" OFF)
option(WANT_RUBBERBAND   "Include RubberBand (Audio Time Stretcher Library) support" OFF)
if(APPLE)
    option(WANT_COREAUDIO   "Include CoreAudio support" ON)
    option(WANT_COREMIDI    "Include CoreMidi support" ON)
    option(WANT_BUNDLE      "Build a MAC OSX bundle application" ON)
endif()

option(WANT_APPIMAGE "Build Linux AppImage" OFF)

option(WANT_CPPUNIT         "Include CppUnit test suite" ON)
option(WANT_INTEGRATION_TESTS "Include integration tests" OFF)

include(Sanitizers)
include(StatusSupportOptions)

if(WANT_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    set(H2CORE_HAVE_DEBUG TRUE)
else()
    set(CMAKE_BUILD_TYPE Release)
    set(H2CORE_HAVE_DEBUG FALSE)
endif()


option(WANT_CLANG_TIDY "Use clang-tidy to check the sourcecode" OFF)
find_program(CLANG_TIDY_CMD NAMES clang-tidy)
if(CLANG_TIDY_CMD)
  #use config from .clang-tidy
  set(CLANG_TIDY_LIBRARIES ${CLANG_TIDY_CMD}) # Required for summary
  set(CLANG_TIDY_FOUND TRUE) # Required for summary
else()
  set(CLANG_TIDY_FOUND FALSE) # Required for summary
endif()
if(CLANG_TIDY_CMD AND WANT_CLANG_TIDY)
  #use config from .clang-tidy
  set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()
compute_pkgs_flags(CLANG_TIDY) # Required for summary

if(WANT_BUNDLE)
    set(H2CORE_HAVE_BUNDLE TRUE)
else()
    set(H2CORE_HAVE_BUNDLE FALSE)
endif()

if(WANT_FAT_BUILD)
    set(H2CORE_HAVE_FAT_BUILD TRUE)
else()
    set(H2CORE_HAVE_FAT_BUILD FALSE)
endif()

if(WANT_SHARED)
    set(H2CORE_LIBRARY_TYPE SHARED)
else()
    set(H2CORE_LIBRARY_TYPE STATIC)
endif()

if(WANT_DEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math")
endif()

if(WANT_APPIMAGE)
  set(H2CORE_HAVE_APPIMAGE TRUE)
else()
  set(H2CORE_HAVE_APPIMAGE FALSE)
endif()

# Whether or not Hydrogen checks during runtime whether the JACK
# server is installed. As we can not ship the shared libs of the
# server ourselves, this feature is meant for build resulting in
# portable bundles.
if(WANT_DYNAMIC_JACK_CHECK)
    set(H2CORE_HAVE_DYNAMIC_JACK_CHECK TRUE)
else()
    set(H2CORE_HAVE_DYNAMIC_JACK_CHECK FALSE)
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-implement-inlines")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "")

set(CMAKE_CXX_FLAGS_DEBUG "-g ")#-Winline")

#
# MANDATORY PKGS AND DEFAULT OPTIONS
#
mandatory_pkg(Threads)
mandatory_pkg(LIBSNDFILE)

# http://doc.qt.io/qt-5/cmake-manual.html
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    include_directories("/opt/local/include")
    link_directories("/opt/local/lib")

    #Without setting this, installation would go into /usr/local, which does not exist per default
    set(CMAKE_INSTALL_PREFIX "/usr" )
else(APPLE)
    set(OSS_LIB_PATHS "${CMAKE_INSTALL_FULL_LIBDIR}/oss/lib" "/usr/local/lib${LIB_SUFFIX}/oss/lib" )
endif(APPLE)


#Installation paths
include(GNUInstallDirs)
if(WIN32)
    set(H2_BIN_PATH ".")
    set(H2_LIB_PATH ".")
    set(H2_DATA_PATH ".")
    set(H2_SYS_PATH "hydrogen")
else()
    set(H2_BIN_PATH ${CMAKE_INSTALL_BINDIR})
    set(H2_LIB_PATH ${CMAKE_INSTALL_LIBDIR})
    set(H2_DATA_PATH "${CMAKE_INSTALL_DATADIR}/hydrogen")
    set(H2_SYS_PATH "${CMAKE_INSTALL_FULL_DATAROOTDIR}/hydrogen")
endif()
set(H2_USR_PATH ".hydrogen")

set(MAX_INSTRUMENTS 1000 CACHE STRING "Maximum number of instruments")
set(MAX_COMPONENTS  32   CACHE STRING "Maximum number of components")
set(MAX_NOTES       192  CACHE STRING "Maximum number of notes")
set(MAX_FX          4    CACHE STRING "Maximum number of effects")
set(MAX_BUFFER_SIZE 8192 CACHE STRING "Maximum size of buffer")

#
# HEADER LIBRARY FUNCTIONS
#
include(CompileHelper)
include(FindHelper)
include(FindLadspa)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(FindZLIB)
include(FindThreads)
compile_helper(SSCANF ${CMAKE_SOURCE_DIR}/cmake/sscanf sscanf )
compile_helper(RTCLOCK ${CMAKE_SOURCE_DIR}/cmake/rtclock rtclock )
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(libtar.h HAVE_LIBTAR_H)
check_include_files(execinfo.h HAVE_EXECINFO_H)
find_package(Backtrace)
check_library_exists(tar tar_open "" HAVE_LIBTAR_OPEN)
check_library_exists(tar tar_close "" HAVE_LIBTAR_CLOSE)
check_library_exists(tar tar_extract_all "" HAVE_LIBTAR_EXTRACT_ALL)
if(HAVE_LIBTAR_H AND HAVE_LIBTAR_OPEN AND HAVE_LIBTAR_CLOSE AND HAVE_LIBTAR_EXTRACT_ALL)
    set(LIBTAR_OK TRUE)
else()
    set(LIBTAR_OK FALSE)
endif()
find_helper(LIBTAR tar tar.h tar)
if( NOT LIBTAR_FOUND OR NOT LIBTAR_OK OR NOT ZLIB_FOUND )
    set(WANT_LIBTAR FALSE)
    mandatory_pkg(LIBARCHIVE)
endif()
find_helper(LIBARCHIVE libarchive archive.h archive)
if( WANT_LIBARCHIVE AND LIBARCHIVE_FOUND)
    set(WANT_LIBTAR FALSE)
endif()
find_helper(LIBSNDFILE sndfile sndfile.h sndfile)
find_helper(ALSA alsa alsa/asoundlib.h asound )
find_ladspa(LADSPA ladspa.h noise)

find_helper(OSC liblo lo/lo.h lo)

if("${CMAKE_SYSTEM_NAME}" MATCHES "NetBSD")
	find_helper(OSS oss sys/soundcard.h ossaudio )
else()
	find_helper(OSS oss sys/soundcard.h OSSlib )
endif()

if(WIN64)
  #64bit Windows
  find_helper(JACK jack jack/jack.h jack64)
else()
  #32bit Windows and all other OS
  find_helper(JACK jack jack/jack.h jack)
endif()
if(APPLE)
    FIND_LIBRARY(AUDIOUNIT_LIBRARY AudioUnit)
    FIND_LIBRARY(CORESERVICES_LIBRARY CoreServices)
    find_helper(COREAUDIO CoreAudio-2.0 CoreAudio.h CoreAudio)
    find_helper(COREMIDI CoreMidi CoreMIDI.h CoreMIDI)
endif()
find_helper(PORTAUDIO portaudio-2.0 portaudio.h portaudio)
find_helper(PORTMIDI portmidi portmidi.h portmidi)
if ("${PC_PORTMIDI_VERSION}" STREQUAL "" AND WANT_PORTMIDI AND PORTMIDI_FOUND)
    color_message("Warning: No version information for PortMidi library found. It is most probably to old to be supported.")
endif()

find_helper(PULSEAUDIO libpulse pulse/pulseaudio.h pulse)
find_helper(LRDF lrdf lrdf.h lrdf)

find_helper(RUBBERBAND rubberband rubberband/RubberBandStretcher.h rubberband)
find_helper(CPPUNIT cppunit cppunit/TestCase.h cppunit)


# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
#set(CMAKE_AUTOMOC ON)


find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc_dir ALL COMMAND ${CMAKE_COMMAND}
		-E make_directory ${CMAKE_CURRENT_BINARY_DIR}/docs)
	add_custom_target(doxygen_dir ALL COMMAND ${CMAKE_COMMAND}
		-E make_directory ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		DEPENDS doc_dir doxygen_dir
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM)
endif(DOXYGEN_FOUND)

#
# COMPUTE H2CORE_HAVE_xxx xxx_STATUS_REPORT
#
set(STATUS_LIST LIBSNDFILE LIBTAR LIBARCHIVE LADSPA ALSA OSS JACK OSC COREAUDIO COREMIDI PORTAUDIO PORTMIDI PULSEAUDIO LRDF RUBBERBAND CPPUNIT )
foreach( _pkg ${STATUS_LIST})
    compute_pkgs_flags(${_pkg})
endforeach()

# Indention used for the second column.
set(TABLE_INDENT " 				 ")

# LIBSNDFILE CHECKS
# All these are the versions _prior_ to when the support was added.
set(LIBSNDFILE_VERSION_FLAC_OGG "1.0.17")
set(LIBSNDFILE_VERSION_OPUS "1.0.28")
set(LIBSNDFILE_VERSION_MP3 "1.0.31")

if (DEFINED PC_LIBSNDFILE_VERSION AND PC_LIBSNDFILE_VERSION STRGREATER "")
    string(COMPARE GREATER "${PC_LIBSNDFILE_VERSION}" "${LIBSNDFILE_VERSION_FLAC_OGG}" H2CORE_HAVE_FLAC_SUPPORT)
    string(COMPARE GREATER "${PC_LIBSNDFILE_VERSION}" "${LIBSNDFILE_VERSION_OPUS}" H2CORE_HAVE_OPUS_SUPPORT)
    string(COMPARE GREATER "${PC_LIBSNDFILE_VERSION}" "${LIBSNDFILE_VERSION_MP3}" H2CORE_HAVE_MP3_SUPPORT)
else()
    # Depending on how libsndfile was built and packaged, we can find it using
    # pkg-config and use the provided version or have to fall back to another
    # discovery scheme with no version information available. But since the
    # libsndfile offers MP3 support for quite a while, we are optimistic and
    # assume support for all formats in case we have no version.
    set(H2CORE_HAVE_FLAC_SUPPORT TRUE)
    set(H2CORE_HAVE_OPUS_SUPPORT TRUE)
    set(H2CORE_HAVE_MP3_SUPPORT TRUE)
endif()

if(H2CORE_HAVE_FLAC_SUPPORT)
    set(LIBSNDFILE_MSG "FLAC: supported, OGG/Vorbis: supported")
else()
	set(LIBSNDFILE_MSG "FLAC: not supported, OGG/Vorbis: not supported (>=${LIBSNDFILE_VERSION_FLAC_OGG} required)")
endif()
if(H2CORE_HAVE_OPUS_SUPPORT)
    set(LIBSNDFILE_MSG "${LIBSNDFILE_MSG}\n${TABLE_INDENT}OGG/Opus: supported")
else()
	set(LIBSNDFILE_MSG "${LIBSNDFILE_MSG}\n${TABLE_INDENT}OGG/Opus: not supported (>=${LIBSNDFILE_VERSION_OPUS} required)")
endif()
if(H2CORE_HAVE_MP3_SUPPORT)
    set(LIBSNDFILE_MSG "${LIBSNDFILE_MSG}\n${TABLE_INDENT}MP3: supported")
else()
	set(LIBSNDFILE_MSG "${LIBSNDFILE_MSG}\n${TABLE_INDENT}MP3: not supported (>=${LIBSNDFILE_VERSION_MP3} required)")
endif()

if(H2CORE_HAVE_CPPUNIT AND WANT_INTEGRATION_TESTS AND WANT_DEBUG AND NOT MINGW AND NOT APPLE)
    set(HAVE_INTEGRATION_TESTS TRUE)
else()
    set(HAVE_INTEGRATION_TESTS FALSE)
endif()

# RUBBERBAND information
set(LIBRUBBERBAND_MSG "The use of librubberband2 is marked as experimental.
${TABLE_INDENT}Because the current implementation produce wrong timing!
${TABLE_INDENT}So long this bug isn't solved, please disable this option.
${TABLE_INDENT}If rubberband-cli is installed, the hydrogen rubberband-function
${TABLE_INDENT}will work properly as expected.")

if(WIN32)
    set(CMAKE_COLOR_MAKEFILE OFF)
endif()

#
# CONFIG PROCESS SUMMARY
#
set(reset "${_escape}[0m")
set(red "${_escape}[1;31m")
set(purple "${_escape}[1;35m")
set(cyan "${_escape}[1;36m")

color_message("${cyan}Installation Summary${reset}
--------------------
* Install Directory            : ${CMAKE_INSTALL_PREFIX}
* User path                    : ${H2_USR_PATH}
* System path                  : ${H2_SYS_PATH}
* Bin path                     : ${H2_BIN_PATH}
* Lib path                     : ${H2_LIB_PATH}
* Data path                    : ${H2_DATA_PATH}
* core library build as        : ${H2CORE_LIBRARY_TYPE}
* debug capabilities           : ${H2CORE_HAVE_DEBUG}
* macOS bundle                 : ${H2CORE_HAVE_BUNDLE}
* Windows fat build            : ${H2CORE_HAVE_FAT_BUILD}
* AppImage build               : ${H2CORE_HAVE_APPIMAGE}
* Dynamic JACK support check   : ${H2CORE_HAVE_DYNAMIC_JACK_CHECK}
* Build integration tests      : ${HAVE_INTEGRATION_TESTS}\n"
)

color_message("${cyan}Main librarires${reset}")
color_message("* ${purple}libQt${reset}                        : ${Qt5Widgets_VERSION}")
color_message("* ${purple}libsndfile${reset}                   : ${LIBSNDFILE_STATUS}
*                                ${LIBSNDFILE_MSG}
* ${purple}libtar${reset}                       : ${LIBTAR_STATUS}
* ${purple}libarchive${reset}                   : ${LIBARCHIVE_STATUS}
* ${purple}ladspa${reset}                       : ${LADSPA_STATUS}\n"
)

color_message("${cyan}Supported audio interfaces${reset}
--------------------------
* ${purple}ALSA${reset}                         : ${ALSA_STATUS}
* ${purple}OSS${reset}                          : ${OSS_STATUS}
* ${purple}JACK${reset}                         : ${JACK_STATUS}
* ${purple}OSC${reset}                          : ${OSC_STATUS}
* ${purple}CoreAudio${reset}                    : ${COREAUDIO_STATUS}
* ${purple}CoreMidi${reset}                     : ${COREMIDI_STATUS}
* ${purple}PortAudio${reset}                    : ${PORTAUDIO_STATUS}
* ${purple}PortMidi${reset}                     : ${PORTMIDI_STATUS}
* ${purple}PulseAudio${reset}                   : ${PULSEAUDIO_STATUS}\n"
)

color_message("${cyan}Useful extensions${reset}
-----------------------------------------
* ${purple}LRDF${reset}                         : ${LRDF_STATUS}
* ${purple}RUBBERBAND${reset}                   : ${RUBBERBAND_STATUS}
*                                ${LIBRUBBERBAND_MSG}\n"
)

if(WANT_DEBUG)
    color_message("${cyan}Miscellaneous capabilities${reset}
-----------------------------------------
* realtime clock               : ${HAVE_RTCLOCK}
* working sscanf               : ${HAVE_SSCANF}
* unit tests                   : ${CPPUNIT_STATUS}
* clang tidy                   : ${CLANG_TIDY_STATUS}\n"
    )
endif()

color_message("-----------------------------------------------------------------
${red}IMPORTANT:${reset}
  after installing missing packages, remove ${CMAKE_BINARY_DIR}/CMakeCache.txt before
  running cmake again!
-----------------------------------------------------------------\n"
)

find_path( HYDROGEN_INSTALLED NAMES core/config.h )
if( HYDROGEN_INSTALLED )
    color_message("-----------------------------------------------------------------
${red}IMPORTANT${reset}:
  previously installed hydrogen headers found in ${HYDROGEN_INSTALLED}
  you should uninstall these files before building hydrogen unless you know what you are doing.
-----------------------------------------------------------------\n"
    )
endif()

if( NOT CMAKE_INSTALL_PREFIX MATCHES "/usr/?$")
    color_message("-----------------------------------------------------------------
${red}IMPORTANT${reset}:
CMAKE_INSTALL_PREFIX is set to '${CMAKE_INSTALL_PREFIX}',
depending of your system settings, you might end up with an installation
that does not work out of the box. If so, see INSTALL.md : Running Hydrogen
-----------------------------------------------------------------\n"
    )
endif()

#
# SET BUILD INFORMATION
#
add_subdirectory(src/core)
if(H2CORE_HAVE_CPPUNIT)
    add_subdirectory(src/tests)
endif()
add_subdirectory(data/i18n)
add_subdirectory(src/cli)
add_subdirectory(src/player)
add_subdirectory(src/gui)
if(EXISTS ${CMAKE_SOURCE_DIR}/data/doc/CMakeLists.txt)
	add_subdirectory(data/doc)
else()
    color_message("${red}ERROR while building documentation${reset}:\n\tSubmodule not initialized.${reset} Please run\n\t`git submodule update --init --recursive` first.\n")
endif()
if(HAVE_INTEGRATION_TESTS)
    add_subdirectory(tests/jackTimebase/h2JackTimebase)
endif()

install(DIRECTORY data DESTINATION ${H2_DATA_PATH} PATTERN ".git" EXCLUDE PATTERN "i18n" EXCLUDE PATTERN doc EXCLUDE)
if(NOT MINGW AND NOT APPLE)
	install(FILES ${CMAKE_SOURCE_DIR}/linux/org.hydrogenmusic.Hydrogen.metainfo.xml DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")
	install(FILES ${CMAKE_SOURCE_DIR}/linux/org.hydrogenmusic.Hydrogen.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
	install(FILES ${CMAKE_SOURCE_DIR}/data/img/gray/h2-icon.svg DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "org.hydrogenmusic.Hydrogen.svg")
	install(FILES ${CMAKE_SOURCE_DIR}/linux/hydrogen.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()

#
# CPack
#
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hydrogen : an advanced drum machine for GNU/Linux")
set(CPACK_PACKAGE_VENDOR "Hydrogen Developers")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
if(VERSION_SUFFIX)
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}-${VERSION_SUFFIX}")
endif(VERSION_SUFFIX)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Hydrogen")

if(MINGW)
    #Set the other files that will be used in CPack
    set(WIN64 "OFF" CACHE BOOL "Windows 64 Bit")
    if (CMAKE_BUILD_TYPE MATCHES Release)
         set(CPACK_STRIP_FILES TRUE)
    else()
         set(CPACK_STRIP_FILES FALSE)
    endif()
    #Program Files for Hydrogen
    set(WINDOWS_DIR "windows")
    #Install files from the extralibs dir
    install(DIRECTORY ${CMAKE_BINARY_DIR}/${WINDOWS_DIR}/extralibs/ DESTINATION ./)

	if(WANT_FAT_BUILD)
		install(DIRECTORY windows/jack_installer DESTINATION ./)
		install(DIRECTORY windows/plugins DESTINATION ./)
	endif()

    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/data\\\\img\\\\h2-icon.bmp")
    #Begin NSIS Customizations

    # Installers for 32- vs. 64-bit CMake:
    #  - Root install directory (displayed to end user at installer-run time)
    #  - "NSIS package/display name" (text used in the installer GUI)
    #  - Registry key used to store info about the installation
    if(WIN64)
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		set(CPACK_NSIS_PACKAGE_NAME "Hydrogen - ${DISPLAY_VERSION} 64Bit")
		set(CPACK_PACKAGE_FILE_NAME "Hydrogen-${DISPLAY_VERSION}-win64")
		#set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}${CPACK_PACKAGE_VERSION} 64Bit")
    else()
		set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
		set(CPACK_NSIS_PACKAGE_NAME "Hydrogen - ${DISPLAY_VERSION}")
		set(CPACK_PACKAGE_FILE_NAME "Hydrogen-${DISPLAY_VERSION}-win32")
		#set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}${CPACK_PACKAGE_VERSION}")
    endif()

    #Need the 2 following lines for the icon to work
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/data\\\\img\\\\h2-icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/data\\\\img\\\\h2-icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "hydrogen.exe")
    set(CPACK_NSIS_DISPLAY_NAME "Hydrogen (Advanced drum machine for GNU/Linux)")
    set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.hydrogen-music.org/")
    set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.hydrogen-music.org/")
    set(CPACK_NSIS_CONTACT "hydrogen-devel@lists.sourceforge.net")
    set(CPACK_PACKAGE_EXECUTABLES "hydrogen.exe;Hydrogen drum machine")
    set(CPACK_NSIS_MENU_LINKS "hydrogen.exe;Hydrogen drum machine")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    #end NSIS customizations
else(MINGW)
    #apple stuff was moved to src/gui/CMakeLists.txt
endif()

set(CPACK_SOURCE_PACKAGE_FILE_NAME "hydrogen")
set(CPACK_SOURCE_IGNORE_FILES ".*~;\\\\.git;\\\\.svn;${CMAKE_BINARY_DIR}")

#!The following 5 lines are copied from cmake's QtTest example

# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well
set(CPACK_BINARY_DRAGNDROP ON)
include(CPack)

#
# CUSTOM TARGETS
#
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

configure_file("${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake.in" "${CMAKE_BINARY_DIR}/uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/uninstall.cmake")
