#
# Native buildscript for Hydrogen on Windows (with msys2 / mingw)
#

param (
    [switch]$build=$false,
    [switch]$installdeps=$false,
    [switch]$deploy=$false,
    [switch]$x86_64=$false
 )

if($x86_64)
{
     $64bit_string = "ON"
     $msys_repo='mingw64/mingw-w64-x86_64'
     $msys='C:\msys64\mingw64'
}
else
{
    $64bit_string = "OFF"
    $msys_repo='mingw32/mingw-w64-i686'
    $msys='C:\msys64\mingw32'
}


$env:QTDIR=$msys
$env:CMAKE_PREFIX_PATH=$env:QTDIR
$env:PATH="$msys\bin;$env:PATH"
$env:PKG_CONFIG_PATH="$msys\lib\pkgconfig"
$python_exe='python'
$build_type='Debug'

if($installdeps)
{
    Write-Host "Installing python-pip"
    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    python get-pip.py

    Write-Host 'Installing msys2 dependencies'
    c:\msys64\usr\bin\pacman --noconfirm -S -q openssh
    c:\msys64\usr\bin\pacman --noconfirm -S -q base-devel

    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-cmake
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-nsis
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-toolchain

    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-libarchive
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-libsndfile
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-cppunit
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-portaudio
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-portmidi
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-libwinpthread-git
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-qt5
    c:\msys64\usr\bin\pacman --noconfirm -S -q $msys_repo-ladspa-sdk
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
    If(!(test-path build))
    {
        mkdir build
    }

    cd build
    $arguments="-G","MinGW Makefiles","-DCMAKE_BUILD_TYPE=$build_type", "-DWANT_DEBUG:BOOL=ON","-DWIN64:BOOL=$64bit_string",".."
    & cmake $arguments

    Write-Host 'Starting build'
    cmake --build . -j $env:NUMBER_OF_PROCESSORS

    If(!(test-path "windows/extralibs"))
    {
        mkdir windows/extralibs
    }

    $arguments="-xmlpatterns", "--no-patchqt", "--dir", "windows\extralibs", "src\gui\hydrogen.exe"
    $cmd="$env:QTDIR\bin\windeployqt.exe"
    & $cmd $arguments

    $arguments= "-m","pip","install","-r","..\windows\ci\requirements.txt"
    & $python_exe $arguments

    $arguments="..\windows\ci\copy_thirdparty_dlls.py","--no-overwrite", "-V", "debug" ,"-L","$msys\bin","-d","windows\extralibs", "src/gui/hydrogen.exe", "src/core/libhydrogen-core-1.1.0.dll"
    & $python_exe $arguments
    
    cd ../windows
}

if($deploy) 
{
    Write-Host 'Creating installer'
    cd ../build
    cpack -G NSIS -v
    cd ../windows
} 
else
{
    Write-Host 'Usage: '
    Write-Host 'Build-WinNative -build: Build hydrogen (32bit)'
    Write-Host 'Build-WinNative -build -x86_64 : Build hydrogen (64bit)'
    Write-Host 'Build-WinNative -installdeps: Install build dependencies via pacman'
    Write-Host 'Build-WinNative -deploy: Create installer'
}
