#
# Native buildscript for Hydrogen on Windows (with msys2 / ucrt64)
#

param (
    [switch]$build=$false,
    [switch]$test=$false,
    [switch]$installdeps=$false,
    [switch]$deploy=$false,
    [switch]$qt5=$false
 )

if($qt5)
{
    $CMAKE_OPTIONS="-DWANT_QT6:BOOL=OFF"
    # Plain 'windeployqt.exe' is the Qt6 tool in the MSYS2 repos (it is
    # owned by qt6-base) and rejects Qt5 binaries with "does not seem to
    # be a Qt executable". The Qt5 tool ships as 'windeployqt-qt5.exe'.
    $windeployqt="windeployqt-qt5.exe"
    # The ucrt64 Qt5 packages do not ship the ANGLE and software-OpenGL
    # DLLs windeployqt wants to deploy next to Qt5Gui; without these flags
    # it aborts with "libGLESv2.dll does not exist" (exit code 1).
    $windeployqtArgs="--no-patchqt","--no-angle","--no-opengl-sw"
}
else
{
    $CMAKE_OPTIONS="-DWANT_QT6:BOOL=ON"
    $windeployqt="windeployqt6.exe"
    $windeployqtArgs="--no-patchqt"
}


$msys='C:\msys64\ucrt64'
$env:QTDIR=$msys
$env:CMAKE_PREFIX_PATH=$env:QTDIR
$env:PATH="$msys\bin;$env:PATH"
$env:PKG_CONFIG_PATH="$msys\lib\pkgconfig"
$build_type='Debug'
$msys_repo='ucrt64/mingw-w64-ucrt-x86_64'
# Wildcards on purpose: the OpenSSL major version (and with it the DLL names)
# drifts with the MSYS2 repos; copy whichever version is actually installed.
$libssl='libssl-*-x64.dll'
$libcrypto='libcrypto-*-x64.dll'

if($installdeps)
{
    Write-Host 'Installing msys2 dependencies'
    c:\msys64\usr\bin\pacman --noconfirm -S -q base-devel

    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-cmake
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-nsis
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-toolchain
    # 'make' is not part of the toolchain group but mingw32-make.exe is
    # required by the "MinGW Makefiles" CMake generator used below.
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-make
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-python
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-python-pip
    # The Python modules used by the bundling and CI helper scripts. Installed
    # via pacman because MSYS2's Python is externally managed (PEP 668) and
    # rejects a plain 'pip install'.
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-python-pefile
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-python-pytest

    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-openssl
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-libarchive
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-libsndfile
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-cppunit
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-portaudio
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-portmidi
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-libwinpthread
    # The kitchen-sink 'qt5'/'qt6' meta packages were dropped from the MSYS2
    # repos. Install the individual modules needed by Hydrogen instead (its
    # CMake requires Core/Gui/Widgets/Test/Xml/Network, Svg, LinguistTools).
    if ($qt5) {
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt5-base
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt5-svg
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt5-tools
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt5-translations
    } else {
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt6-base
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt6-svg
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt6-tools
       c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt6-translations
    }
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-jack2
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-liblo
}

if($build)
{
    Write-Host 'Building Hydrogen'
    Write-Host 'Using the following setting:'
    Write-Host "QTDIR: $env:QTDIR"
    Write-Host "PATH: $env:PATH"
    Write-Host "MSYS: $msys"

    Write-Host ' '
    cmake --version
    Write-Host ' '
    g++ --version

    cd ..
    if(!(test-path build))
    {
        mkdir build
    }

    cd build
    $arguments="-G","MinGW Makefiles","-DCMAKE_BUILD_TYPE=$build_type", "-DWANT_DEBUG:BOOL=ON","-DWIN64:BOOL=ON", $CMAKE_OPTIONS, ".."
    & cmake $arguments

    Write-Host 'Starting build'
    cmake --build . -j $env:NUMBER_OF_PROCESSORS
    if ( -not $? ) {
	   cd ../windows
	   throw 'build failed'
    }

    # Start from a clean slate: files left over from a previous bundling
    # run (e.g. of the other Qt flavor) must not end up in the installer.
    if (test-path "windows/extralibs")
    {
        Remove-Item -Recurse -Force "windows/extralibs"
    }
    mkdir windows/extralibs

    Write-Host 'Bundling libraries required for Qt'
    $arguments = $windeployqtArgs + @("--dir", "windows\extralibs", "src\gui\hydrogen.exe")
    $cmd="$env:QTDIR\bin\$windeployqt"
    & $cmd $arguments
    if ( -not $? ) {
        cd ../windows
        throw "windeployqt failed with exit code $LASTEXITCODE"
    }

    Write-Host 'Installing additional Python requirements'
    # --break-system-packages: MSYS2's Python is externally managed (PEP 668).
    # The requirements are preinstalled via pacman (see above); pip only fills
    # in any future additions to the requirements file.
    $arguments= "-m","pip","install","--break-system-packages","-r","..\windows\ci\requirements.txt"
    & python $arguments

    Write-Host 'Bundling additional libraries'
    $arguments="..\windows\ci\copy_thirdparty_dlls.py","--no-overwrite", "-V", "debug" ,"-L","$msys\bin","-d","windows\extralibs", "--ignore-missing", "src/gui/hydrogen.exe", "src/core/libhydrogen-core-2.0.0-pre-alpha.dll"
    & python $arguments

    # libcrypto and libssl are not picked up by the Python script
    # above and needs to be copied manually
    Write-Host 'Copy libssl and libcrypto libraries manually'
    cp $msys\bin\$libssl windows\extralibs
    cp $msys\bin\$libcrypto windows\extralibs

    Write-Host 'DONE Building Hydrogen'
    cd ../windows
}

if($test)
{
    $currentPath=Get-Location
    $env:path += ";$currentPath\..\build\src\core"

    ..\build\src\tests\tests.exe
}

if($deploy)
{
    Write-Host 'Creating installer'
    cd ../build
    cpack -G NSIS -V
    cd ../windows
}

if(!$deploy -and !$build -and !$installdeps -and !$test )
{
    Write-Host 'Usage: '
    Write-Host 'Build-WinNative -build: Build hydrogen (Qt6)'
    Write-Host 'Build-WinNative -build -qt5 : Build hydrogen (Qt5)'
    Write-Host 'Build-WinNative -test : Run unit tests after successful build'
    Write-Host 'Build-WinNative -installdeps: Install build dependencies via pacman (including Qt6 libraries)'
    Write-Host 'Build-WinNative -installdeps -qt5: Install build dependencies via pacman (including Qt5 libraries)'
    Write-Host 'Build-WinNative -deploy: Create installer'
    Write-Host 'Note: please delete the build folder when switching from Qt6 to Qt5 (or vice versa) or when switching the MSYS2 environment (e.g. mingw64 to ucrt64)'
}
