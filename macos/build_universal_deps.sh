#!/usr/bin/env bash
# Build all third-party dependencies of a portable, universal (arm64 + x86_64)
# macOS build of Hydrogen as static libraries.
#
# Homebrew can not be used for this purpose: its packages are built for a
# single architecture and the latest macOS release only. Instead, every
# dependency is built from pinned sources for the requested minimum macOS
# version. Since all of them end up as static libraries, the resulting app and
# plugin bundles only link against libraries shipped with macOS.
#
# - Qt is built once as a universal binary. It is static and uses the namespace
#   H2_<MAJOR>_<MINOR>_<PATCH> so the plugins can not clash with the Qt of a plugin host.
# - All other libraries are built once per architecture and merged using
#   `lipo`. Their build systems (autotools, assembly and intrinsics detection)
#   do not reliably support building for two architectures at once.
#
# Usage:
#   build_universal_deps.sh <source-dir> <deps-dir> <macos-target> [jobs]
#
#   source-dir    Downloaded tarballs, extracted sources and build folders.
#   deps-dir      Installation prefix. Qt goes to <deps-dir>/qt, all other
#                 libraries to <deps-dir>/lib and <deps-dir>/include.
#   macos-target  Minimum macOS version (e.g. 12.0). Qt 6.8 supports >= 12.
#
# Already built dependencies are skipped (stamp files in <deps-dir>/.stamps).
#
# Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
#
# http://www.hydrogen-music.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see https://www.gnu.org/licenses

set -euo pipefail

usage='usage: build_universal_deps.sh <source-dir> <deps-dir> <macos-target> [jobs]'
if [ "$#" -lt 3 ] || [ "$#" -gt 4 ]; then
    echo "$usage" >&2
    exit 2
fi
if [ "$(uname -s)" != "Darwin" ]; then
    echo "error: universal macOS builds require macOS" >&2
    exit 1
fi

mkdir -p "$1" "$2"
SOURCE_DIR=$(cd "$1" && pwd)
DEPS_DIR=$(cd "$2" && pwd)
TARGET="$3"
JOBS="${4:-$(sysctl -n hw.ncpu)}"

ARCHS="arm64 x86_64"
HOST_ARCH=$(uname -m)
QT_NAMESPACE="H2_2_0_0"
# Prefixes holding Homebrew, MacPorts and manually installed libraries. They
# must never end up in the dependency resolution.
IGNORED_PREFIXES="/opt/homebrew;/usr/local;/opt/local"

TARBALL_DIR="$SOURCE_DIR/tarballs"
SRC_DIR="$SOURCE_DIR/src"
BUILD_ROOT="$SOURCE_DIR/build-universal-$TARGET"
STAMP_DIR="$DEPS_DIR/.stamps"
QT_DIR="$DEPS_DIR/qt"
PATCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/patches"

################################################################################
# Pinned sources: name version url sha256

QT_VERSION=6.8.5
QT_URL="https://download.qt.io/archive/qt/6.8/$QT_VERSION/submodules"

SOURCES="
qtbase $QT_VERSION $QT_URL/qtbase-everywhere-opensource-src-$QT_VERSION.tar.xz fc95010e94d8b1ec342eda9281fd278d5f096cf9ad8f1a0adfba9575438b0abc
qtsvg $QT_VERSION $QT_URL/qtsvg-everywhere-opensource-src-$QT_VERSION.tar.xz 8ef4841a8b81d7192d908ea344137cf66a29cc110382588d91c62508e0e15e3a
qttools $QT_VERSION $QT_URL/qttools-everywhere-opensource-src-$QT_VERSION.tar.xz e8a1d632f9e9091852244b83c7b77f7dad0e1213e4e5b8b612071ae169659aa9
libogg 1.3.6 https://downloads.xiph.org/releases/ogg/libogg-1.3.6.tar.xz 5c8253428e181840cd20d41f3ca16557a9cc04bad4a3d04cce84808677fa1061
libvorbis 1.3.7 https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b
flac 1.5.0 https://downloads.xiph.org/releases/flac/flac-1.5.0.tar.xz f2c1c76592a82ffff8413ba3c4a1299b6c7ab06c734dee03fd88630485c2b920
opus 1.6.1 https://downloads.xiph.org/releases/opus/opus-1.6.1.tar.gz 6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1
mpg123 1.33.7 https://www.mpg123.de/download/mpg123-1.33.7.tar.bz2 31d0e35a4ca567ec9b5ebda6c3062bb4435d6d3eacd6ef0d95cadd7854dc03ee
lame 3.101 https://downloads.sourceforge.net/project/lame/lame/3.101/lame-3.101.tar.gz 7578af6eebd578b2bd64e468fac4ae1f03670a7e028166e67f855674b9b6aeac
libsndfile 1.2.2 https://github.com/libsndfile/libsndfile/releases/download/1.2.2/libsndfile-1.2.2.tar.xz 3799ca9924d3125038880367bf1468e53a1b7e3686a934f098b7e1d286cdb80e
libarchive 3.8.10 https://www.libarchive.org/downloads/libarchive-3.8.10.tar.xz 56cad765459d5ee25a3b761f90c47d99379d28297fbfb72d908880d6b875fb96
rubberband 4.0.0 https://breakfastquay.com/files/releases/rubberband-4.0.0.tar.bz2 af050313ee63bc18b35b2e064e5dce05b276aaf6d1aa2b8a82ced1fe2f8028e9
cppunit 1.15.1 https://dev-www.libreoffice.org/src/cppunit-1.15.1.tar.gz 89c5c6665337f56fd2db36bc3805a5619709d51fb136e51937072f63fcc717a7
"

source_field() {
    local sName="$1" nField="$2"
    echo "$SOURCES" | awk -v name="$sName" -v field="$nField" \
        '$1 == name { print $field; exit }'
}

################################################################################
# Helpers

log() {
    echo ""
    echo "=== $* ==="
}

is_done() {
    [ -e "$STAMP_DIR/$1-$(source_field "$1" 2)-$TARGET" ]
}

mark_done() {
    mkdir -p "$STAMP_DIR"
    touch "$STAMP_DIR/$1-$(source_field "$1" 2)-$TARGET"
}

# Download (if missing) and verify the tarball of $1. Prints its path.
fetch() {
    local sName="$1"
    local sUrl sSha sFile
    sUrl=$(source_field "$sName" 3)
    sSha=$(source_field "$sName" 4)
    sFile="$TARBALL_DIR/${sUrl##*/}"
    mkdir -p "$TARBALL_DIR"
    if [ ! -s "$sFile" ]; then
        echo "Downloading $sUrl" >&2
        curl -fL --retry 3 -o "$sFile.part" "$sUrl" >&2
        mv "$sFile.part" "$sFile"
    fi
    if ! echo "$sSha  $sFile" | shasum -a 256 -c - > /dev/null; then
        echo "error: checksum mismatch for $sFile" >&2
        echo "       (delete the file to download it again)" >&2
        exit 1
    fi
    echo "$sFile"
}

# Extract the sources of $1 into $SRC_DIR and apply the patches in
# macos/patches/<name>-<version>-*.patch. Prints the resulting folder.
unpack() {
    local sName="$1"
    local sTarball sTop
    sTarball=$(fetch "$sName")
    sTop=$(tar -tf "$sTarball" | head -n1 | cut -d/ -f1)
    mkdir -p "$SRC_DIR"
    if [ ! -e "$SRC_DIR/$sTop/.h2-unpacked" ]; then
        echo "Extracting ${sTarball##*/}" >&2
        rm -rf "${SRC_DIR:?}/$sTop"
        tar -xf "$sTarball" -C "$SRC_DIR"
        # Fixes for the pinned versions, see the description in each file.
        local sPatch
        for sPatch in "$PATCH_DIR/$sName-$(source_field "$sName" 2)"-*.patch; do
            [ -e "$sPatch" ] || continue
            echo "Applying ${sPatch##*/}" >&2
            patch -d "$SRC_DIR/$sTop" -p1 --forward --quiet < "$sPatch" >&2
        done
        touch "$SRC_DIR/$sTop/.h2-unpacked"
    fi
    echo "$SRC_DIR/$sTop"
}

# Fresh, empty build folder for $1 and architecture $2. Prints its path.
build_dir() {
    local sDir="$BUILD_ROOT/$1-$2"
    rm -rf "$sDir"
    mkdir -p "$sDir"
    echo "$sDir"
}

staging_dir() {
    echo "$BUILD_ROOT/staging-$1"
}

# Environment for building a single-architecture dependency into the staging
# prefix of architecture $1. Nothing but the macOS SDK and the libraries built
# by this script must be found.
set_arch_env() {
    local sArch="$1"
    local sStaging
    sStaging=$(staging_dir "$sArch")
    export CC="clang -arch $sArch"
    export CXX="clang++ -arch $sArch"
    export CFLAGS="-O2 -mmacosx-version-min=$TARGET"
    export CXXFLAGS="$CFLAGS"
    export CPPFLAGS="-I$sStaging/include"
    export LDFLAGS="-L$sStaging/lib -mmacosx-version-min=$TARGET"
    export PKG_CONFIG_LIBDIR="$sStaging/lib/pkgconfig"
    unset PKG_CONFIG_PATH CPATH LIBRARY_PATH
    export MACOSX_DEPLOYMENT_TARGET="$TARGET"
}

# --host is only passed for the architecture not matching the build machine.
# It puts autotools into cross-compiling mode, so it does not try to run the
# test programs it compiled.
autotools_host() {
    local sArch="$1"
    if [ "$sArch" = "$HOST_ARCH" ]; then
        return
    fi
    case "$sArch" in
        arm64) echo "--host=aarch64-apple-darwin" ;;
        x86_64) echo "--host=x86_64-apple-darwin" ;;
    esac
}

cmake_arch() {
    local sArch="$1" sSrc="$2" sBuild="$3"
    shift 3
    local sStaging
    sStaging=$(staging_dir "$sArch")
    cmake -S "$sSrc" -B "$sBuild" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$sStaging" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_PREFIX_PATH="$sStaging" \
        -DCMAKE_IGNORE_PREFIX_PATH="$IGNORED_PREFIXES" \
        -DCMAKE_SYSTEM_IGNORE_PREFIX_PATH="$IGNORED_PREFIXES" \
        -DCMAKE_FIND_FRAMEWORK=LAST \
        -DCMAKE_OSX_ARCHITECTURES="$sArch" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$TARGET" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG" \
        "$@"
    cmake --build "$sBuild" --parallel "$JOBS"
    cmake --install "$sBuild"
}

################################################################################
# Single-architecture dependencies

build_libogg() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack libogg)
    sBuild=$(build_dir libogg "$sArch")
    cmake_arch "$sArch" "$sSrc" "$sBuild" -DINSTALL_DOCS=OFF
}

build_libvorbis() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack libvorbis)
    sBuild=$(build_dir libvorbis "$sArch")
    cmake_arch "$sArch" "$sSrc" "$sBuild"
}

build_flac() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack flac)
    sBuild=$(build_dir flac "$sArch")
    cmake_arch "$sArch" "$sSrc" "$sBuild" \
        -DWITH_OGG=ON -DBUILD_CXXLIBS=OFF -DBUILD_PROGRAMS=OFF \
        -DBUILD_EXAMPLES=OFF -DBUILD_DOCS=OFF -DINSTALL_MANPAGES=OFF
}

build_opus() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack opus)
    sBuild=$(build_dir opus "$sArch")
    cmake_arch "$sArch" "$sSrc" "$sBuild" \
        -DOPUS_BUILD_PROGRAMS=OFF -DOPUS_BUILD_TESTING=OFF \
        -DOPUS_INSTALL_PKG_CONFIG_MODULE=ON -DOPUS_INSTALL_CMAKE_CONFIG_MODULE=ON
}

build_mpg123() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack mpg123)
    sBuild=$(build_dir mpg123 "$sArch")
    (
        cd "$sBuild"
        # shellcheck disable=SC2046
        "$sSrc/configure" --prefix="$(staging_dir "$sArch")" \
            $(autotools_host "$sArch") \
            --enable-static --disable-shared \
            --disable-components --enable-libmpg123 \
            --with-pic
        make -j "$JOBS"
        make install
    )
}

build_lame() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack lame)
    sBuild=$(build_dir lame "$sArch")
    (
        cd "$sBuild"
        # shellcheck disable=SC2046
        "$sSrc/configure" --prefix="$(staging_dir "$sArch")" \
            $(autotools_host "$sArch") \
            --enable-static --disable-shared \
            --disable-frontend --disable-gtktest --disable-decoder \
            --with-pic
        make -j "$JOBS"
        make install
    )
}

build_libsndfile() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack libsndfile)
    sBuild=$(build_dir libsndfile "$sArch")
    cmake_arch "$sArch" "$sSrc" "$sBuild" \
        -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_REGTEST=OFF \
        -DENABLE_CPACK=OFF -DENABLE_EXTERNAL_LIBS=ON -DENABLE_MPEG=ON \
        -DINSTALL_MANPAGES=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Speex=ON \
        -DCMAKE_DISABLE_FIND_PACKAGE_SQLite3=ON
}

build_libarchive() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack libarchive)
    sBuild=$(build_dir libarchive "$sArch")
    # Drumkits are gzip compressed tarballs. zlib, bzip2 and iconv are part of
    # macOS. Everything else is not needed by Hydrogen.
    cmake_arch "$sArch" "$sSrc" "$sBuild" \
        -DENABLE_TAR=OFF -DENABLE_CPIO=OFF -DENABLE_CAT=OFF \
        -DENABLE_UNZIP=OFF -DENABLE_TEST=OFF \
        -DENABLE_ZLIB=ON -DENABLE_BZip2=ON -DENABLE_ICONV=ON \
        -DENABLE_OPENSSL=OFF -DENABLE_MBEDTLS=OFF -DENABLE_NETTLE=OFF \
        -DENABLE_LIBB2=OFF -DENABLE_LZ4=OFF -DENABLE_LZO=OFF \
        -DENABLE_LZMA=OFF -DENABLE_ZSTD=OFF -DENABLE_LIBXML2=OFF \
        -DENABLE_EXPAT=OFF -DENABLE_WIN32_XMLLITE=OFF -DENABLE_PCREPOSIX=OFF \
        -DENABLE_PCRE2POSIX=OFF -DENABLE_LIBGCC=OFF -DENABLE_CNG=OFF
    # Linking against a static libarchive requires LIBARCHIVE_STATIC on
    # Windows only. On macOS the .pc file is all we need.
}

build_rubberband() {
    local sArch="$1" sSrc sBuild sStaging
    sSrc=$(unpack rubberband)
    sBuild=$(build_dir rubberband "$sArch")
    sStaging=$(staging_dir "$sArch")
    # The single-file build does not require meson. On macOS it uses the FFT
    # of the Accelerate framework (HAVE_VDSP).
    $CXX $CXXFLAGS -std=c++14 -DNDEBUG -I"$sSrc" \
        -c "$sSrc/single/RubberBandSingle.cpp" -o "$sBuild/RubberBandSingle.o"
    mkdir -p "$sStaging/lib/pkgconfig" "$sStaging/include/rubberband"
    rm -f "$sStaging/lib/librubberband.a"
    ar rcs "$sStaging/lib/librubberband.a" "$sBuild/RubberBandSingle.o"
    cp "$sSrc"/rubberband/*.h "$sStaging/include/rubberband/"
    cat > "$sStaging/lib/pkgconfig/rubberband.pc" <<EOF
prefix=$sStaging
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: rubberband
Version: $(source_field rubberband 2)
Description: Audio time-stretching and pitch-shifting library
Libs: -L\${libdir} -lrubberband
Libs.private: -framework Accelerate -lc++
Cflags: -I\${includedir}
EOF
}

build_cppunit() {
    local sArch="$1" sSrc sBuild
    sSrc=$(unpack cppunit)
    sBuild=$(build_dir cppunit "$sArch")
    (
        cd "$sBuild"
        # shellcheck disable=SC2046
        "$sSrc/configure" --prefix="$(staging_dir "$sArch")" \
            $(autotools_host "$sArch") \
            --enable-static --disable-shared \
            --disable-doxygen --disable-html-docs --disable-werror
        make -j "$JOBS"
        make install
    )
}

# Order matters: later libraries are built against the earlier ones.
LIBRARIES="libogg libvorbis flac opus mpg123 lame libsndfile libarchive rubberband cppunit"

################################################################################
# Merging the single-architecture builds

merge_architectures() {
    log "Merging $ARCHS into $DEPS_DIR"
    local sFirst=""
    local sArch
    for sArch in $ARCHS; do
        if [ -z "$sFirst" ]; then
            sFirst="$sArch"
        elif ! diff -r "$(staging_dir "$sFirst")/include" \
                       "$(staging_dir "$sArch")/include" > /dev/null; then
            # Merging the libraries is only valid if both architectures
            # share the same headers (e.g. no generated, arch-specific
            # config headers).
            diff -r "$(staging_dir "$sFirst")/include" \
                    "$(staging_dir "$sArch")/include" >&2 || true
            echo "error: headers differ between $sFirst and $sArch" >&2
            exit 1
        fi
    done

    local sFirstStaging
    sFirstStaging=$(staging_dir "$sFirst")
    rm -rf "$DEPS_DIR/include" "$DEPS_DIR/lib"
    mkdir -p "$DEPS_DIR"
    cp -R "$sFirstStaging/include" "$DEPS_DIR/include"
    # Static libraries and the build system metadata only. Binaries (e.g.
    # mpg123's helpers) are not required.
    mkdir -p "$DEPS_DIR/lib"
    (cd "$sFirstStaging/lib" && find . \( -name '*.a' -o -name '*.pc' \
        -o -name '*.cmake' \) -print | cpio -pdm "$DEPS_DIR/lib" 2>/dev/null)

    local sLib
    while IFS= read -r sLib; do
        local aInputs=()
        for sArch in $ARCHS; do
            aInputs+=("$(staging_dir "$sArch")/lib/$sLib")
        done
        lipo -create "${aInputs[@]}" -output "$DEPS_DIR/lib/$sLib"
    done < <(cd "$sFirstStaging/lib" && find . -name '*.a' | sed 's|^\./||')

    # pkg-config and CMake files refer to the staging prefix.
    grep -rlF "$sFirstStaging" "$DEPS_DIR/lib" | while IFS= read -r sFile; do
        sed -i '' "s|$sFirstStaging|$DEPS_DIR|g" "$sFile"
    done
}

################################################################################
# Qt

build_qt() {
    local sCommon=(
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$TARGET"
        -DCMAKE_IGNORE_PREFIX_PATH="$IGNORED_PREFIXES"
        -DCMAKE_SYSTEM_IGNORE_PREFIX_PATH="$IGNORED_PREFIXES"
        # Only the Command Line Tools are required (no Xcode app).
        -DQT_NO_XCODE_MIN_VERSION_CHECK=ON
    )
    local sSrc sBuild

    if ! is_done qtbase; then
        log "Building qtbase $QT_VERSION (universal)"
        sSrc=$(unpack qtbase)
        sBuild=$(build_dir qtbase universal)
        # Bundled third-party libraries only and no pkg-config, so no
        # Homebrew library is picked up.
        (
            cd "$sBuild"
            "$sSrc/configure" -static -no-framework -release \
                -qtnamespace "$QT_NAMESPACE" -prefix "$QT_DIR" \
                -nomake examples -nomake tests \
                -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-harfbuzz \
                -qt-freetype -no-zstd -no-dbus -no-openssl -no-icu \
                -no-pkg-config \
                -- "${sCommon[@]}"
            cmake --build . --parallel "$JOBS"
            cmake --install .
        )
        mark_done qtbase
    fi

    if ! is_done qtsvg; then
        log "Building qtsvg $QT_VERSION (universal)"
        sSrc=$(unpack qtsvg)
        sBuild=$(build_dir qtsvg universal)
        (
            cd "$sBuild"
            "$QT_DIR/bin/qt-configure-module" "$sSrc" -- "${sCommon[@]}"
            cmake --build . --parallel "$JOBS"
            cmake --install .
        )
        mark_done qtsvg
    fi

    if ! is_done qttools; then
        log "Building qttools $QT_VERSION (universal)"
        sSrc=$(unpack qttools)
        sBuild=$(build_dir qttools universal)
        # Hydrogen only needs the Linguist tools (lrelease, lupdate).
        (
            cd "$sBuild"
            "$QT_DIR/bin/qt-configure-module" "$sSrc" \
                -no-feature-assistant -no-feature-designer \
                -no-feature-distancefieldgenerator -no-feature-kmap2qmap \
                -no-feature-pixeltool -no-feature-qdbus -no-feature-qdoc \
                -no-feature-qev -no-feature-qtattributionsscanner \
                -no-feature-qtdiag -no-feature-qtplugininfo \
                -no-feature-clang \
                -- "${sCommon[@]}"
            cmake --build . --parallel "$JOBS"
            cmake --install .
        )
        mark_done qttools
    fi
}

################################################################################
# Main

PKG_CONFIG=$(command -v pkg-config || command -v pkgconf || true)
if [ -z "$PKG_CONFIG" ]; then
    echo "error: pkg-config is required (e.g. 'brew install pkgconf')" >&2
    exit 1
fi
for sTool in cmake curl lipo vtool make; do
    if ! command -v "$sTool" > /dev/null; then
        echo "error: $sTool is required" >&2
        exit 1
    fi
done

# Flags from the calling environment would leak into every build.
unset CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS CPATH LIBRARY_PATH
unset PKG_CONFIG_PATH PKG_CONFIG_LIBDIR
export MACOSX_DEPLOYMENT_TARGET="$TARGET"

echo "Sources:      $SOURCE_DIR"
echo "Dependencies: $DEPS_DIR"
echo "macOS target: $TARGET"
echo "Jobs:         $JOBS"

bMerge=0
for sLib in $LIBRARIES; do
    if is_done "$sLib"; then
        continue
    fi
    for sArch in $ARCHS; do
        log "Building $sLib $(source_field "$sLib" 2) ($sArch)"
        (
            set_arch_env "$sArch"
            "build_$sLib" "$sArch"
        )
    done
    mark_done "$sLib"
    bMerge=1
done
if [ "$bMerge" -eq 1 ] || [ ! -d "$DEPS_DIR/lib" ]; then
    merge_architectures
fi

build_qt

log "Done"
echo "Qt:        $QT_DIR"
echo "Libraries: $DEPS_DIR"
