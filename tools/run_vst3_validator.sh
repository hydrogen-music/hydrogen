#!/bin/sh
# Build and run the official Steinberg VST3 SDK validator against a built .vst3
# bundle (T6.3 conformance step for the Hydrogen VST3 plugin).
#
# The validator lives in the Steinberg VST3 SDK
# (public.sdk/samples/vst-hosting/validator). clap-wrapper fetches that SDK via
# CPM at configure time (-DWANT_VST3=1) but only builds a curated subset, so the
# validator target is never produced. This script configures the *already
# fetched* SDK source as a standalone project — which builds the validator
# unconditionally — with the VSTGUI and plug-in samples disabled (they need GUI
# toolkits the validator does not), then runs the validator on the bundle.
#
# Usage:
#   run_vst3_validator.sh <vst3-sdk-source-dir> <bundle.vst3> [validator-build-dir]
#
# Typical values from inside a plugin build tree configured with -DWANT_VST3=1:
#   vst3-sdk-source-dir = <build>/clap-wrapper/cpm/vst3sdk
#   bundle.vst3         = <build>/<Config>/Hydrogen.vst3  (or <build>/Hydrogen.vst3)
#   validator-build-dir = <build>/vst3-validator          (default)
#
# Exits non-zero if the SDK source or bundle is missing, the validator fails to
# build, or the validator reports any failed tests.
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

set -eu

usage='usage: run_vst3_validator.sh <vst3-sdk-source-dir> <bundle.vst3> [validator-build-dir]'
SDK_SRC="${1:?$usage}"
BUNDLE="${2:?$usage}"
BUILD_DIR="${3:-vst3-validator}"

if [ ! -f "$SDK_SRC/CMakeLists.txt" ]; then
    echo "error: VST3 SDK source not found at '$SDK_SRC'" >&2
    echo "       (expected the tree clap-wrapper fetches at <build>/clap-wrapper/cpm/vst3sdk)" >&2
    exit 1
fi
# BUNDLE may be the .vst3 bundle directly, or a directory to search for it (so
# callers — e.g. the CTest wrapper — can pass the build root without guessing the
# per-config subdir clap-wrapper emits into, Debug/ vs Release/). Search for our
# specific bundle name (the target's OUTPUT_NAME): clap-wrapper also builds its
# own example wrapper (clapasvst3.vst3) under the same tree, so a bare '*.vst3'
# match can pick that instead — and its .so isn't built, so the validator's
# dlopen fails.
if [ -d "$BUNDLE" ] && [ "${BUNDLE##*.}" != "vst3" ]; then
    found=$(find "$BUNDLE" -name 'Hydrogen.vst3' -type d 2>/dev/null | head -n1 || true)
    if [ -n "$found" ]; then
        BUNDLE="$found"
    fi
fi
if [ ! -e "$BUNDLE" ]; then
    echo "error: VST3 bundle not found at '$BUNDLE'" >&2
    exit 1
fi

# clap-wrapper fetches the SDK with only the source submodules (base, public.sdk,
# pluginterfaces) — it builds its own subset and never uses the SDK's CMake build
# system, so the 'cmake' submodule that defines the SMTG_* helpers is absent. We
# do need it to build the validator, so fetch it on demand.
if [ ! -f "$SDK_SRC/cmake/modules/SMTG_VST3_SDK.cmake" ]; then
    echo "=== Fetching the SDK 'cmake' submodule (SMTG CMake helpers) ==="
    if ! git -C "$SDK_SRC" submodule update --init cmake; then
        echo "error: could not fetch the VST3 SDK 'cmake' submodule (needs git + network)" >&2
        exit 1
    fi
fi

# Locate a validator binary (lets re-runs skip the rebuild). The SDK emits
# executables under <build>/bin, optionally in a per-config subdir.
find_validator() {
    for c in \
        "$BUILD_DIR/bin/validator" \
        "$BUILD_DIR/bin/Release/validator" \
        "$BUILD_DIR/bin/RelWithDebInfo/validator" \
        "$BUILD_DIR/bin/validator.exe" \
        "$BUILD_DIR/bin/Release/validator.exe"; do
        if [ -x "$c" ]; then echo "$c"; return 0; fi
    done
    found=$(find "$BUILD_DIR" -name 'validator' -type f 2>/dev/null | head -n1 || true)
    if [ -n "$found" ] && [ -x "$found" ]; then echo "$found"; return 0; fi
    found=$(find "$BUILD_DIR" -name 'validator.exe' -type f 2>/dev/null | head -n1 || true)
    if [ -n "$found" ]; then echo "$found"; return 0; fi
    return 1
}

if VALIDATOR=$(find_validator); then
    echo "Using existing validator: $VALIDATOR"
else
    echo "=== Building Steinberg VST3 validator from $SDK_SRC ==="
    # The SDK builds the validator only when it is the top-level project. Keep the
    # heavy/optional pieces off so only the hosting libs + validator compile:
    #   SMTG_ADD_VSTGUI=OFF             - no VSTGUI/X11 toolkit
    #   SMTG_ADD_VST3_PLUGINS_SAMPLES=OFF - no example plug-ins
    #   SMTG_ADD_VST3_HOSTING_SAMPLES=OFF - drops the editorhost sample (needs
    #                                       GTK3/GTKMM3 via pkg-config); the
    #                                       validator itself is built unconditionally
    # -include cstdint: the pinned SDK (v3.7.6) uses uint32_t in
    # public.sdk/.../moduleinfo/moduleinfo.h without including <cstdint>. Older
    # libstdc++ pulled it in transitively, but GCC 13+/newer libstdc++ do not, so
    # the validator fails to compile ("'uint32_t' does not name a type"). Force-
    # including <cstdint> fixes it without patching the fetched SDK source; it is
    # harmless on toolchains that already have it.
    cmake -S "$SDK_SRC" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSMTG_ADD_VSTGUI=OFF \
        -DSMTG_ADD_VST3_PLUGINS_SAMPLES=OFF \
        -DSMTG_ADD_VST3_HOSTING_SAMPLES=OFF \
        "-DCMAKE_CXX_FLAGS=-include cstdint"
    cmake --build "$BUILD_DIR" --target validator --config Release \
        -j "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
    if ! VALIDATOR=$(find_validator); then
        echo "error: validator was not produced under '$BUILD_DIR'" >&2
        exit 1
    fi
fi

echo "=== Running Steinberg VST3 validator on $BUNDLE ==="
# Run the validator directly. It is chatty (thousands of per-test "Info:" lines),
# but as a CTest the output is captured by CTest — hidden on success, shown on
# failure — so there is no need to filter it here. The exit code (non-zero on any
# failed test) becomes this script's exit code.
exec "$VALIDATOR" "$BUNDLE"
