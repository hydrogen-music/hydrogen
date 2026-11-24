# Fail on the first error, on unset variables and on failing pipes in every
# recipe. Recipes with a shebang line (like `headless` below) run as a single
# script and bring their own `set` line instead.
set shell := ["bash", "-euo", "pipefail", "-c"]

# ── Settings of the universal macOS builds (see DEVELOPERS.md) ─────────────
#
# Every setting can be overridden via an environment variable of the same name.

# Downloaded tarballs, extracted sources and build folders of the third-party
# dependencies (Qt, libsndfile, ...). Can be deleted once `macos-deps` is done.
MACOS_SOURCE_DIR := env_var_or_default("MACOS_SOURCE_DIR", home_dir() + "/software/src")

# Oldest macOS release the universal bundles run on. Qt 6.8 supports >= 12.0.
MACOS_TARGET := env_var_or_default("MACOS_TARGET", "12.0")

# Static, universal Qt and libraries built by `macos-deps`.
MACOS_DEPS_DIR := env_var_or_default("MACOS_DEPS_DIR", home_dir() + "/software/h2-macos-universal-" + MACOS_TARGET)

# Build directory, separate from those of build.sh and the plugin targets.
MACOS_BUILD_DIR := env_var_or_default("MACOS_BUILD_DIR", "build-universal")

# Staging prefix holding the finished bundles: <prefix>/bin/hydrogen.app and
# <prefix>/lib/{clap,lv2,vst3}.
MACOS_PREFIX := env_var_or_default("MACOS_PREFIX", "dist-universal")

# Folder `macos-install` copies Hydrogen.app into.
MACOS_APP_DIR := env_var_or_default("MACOS_APP_DIR", "/Applications")

# Start a headless engine serving a demo song and connect the GUI to it via
# IPC. Both processes are killed on Ctrl+C.
headless:
    #!/usr/bin/env bash
    set -euo pipefail

    mkdir -p /tmp/hydrogen

    # h2player stdout is captured to a temp file so we can poll for the
    # "Endpoint:" line. The -L flag also writes a structured log.
    TMP_OUT=$(mktemp)

    ./build/src/player/h2player \
        -VIpc "$PWD/data/demo_songs/GM_kit_demo1.h2song" \
        -T -L /tmp/hydrogen/headless.log > "$TMP_OUT" 2>&1 &
    HEADLESS_PID=$!

    GUI_PID=""

    cleanup() {
        [ -n "$GUI_PID" ] && kill "$GUI_PID" 2>/dev/null || true
        kill "$HEADLESS_PID" 2>/dev/null || true
        wait 2>/dev/null || true
        rm -f "$TMP_OUT"
    }
    trap cleanup INT TERM EXIT

    # Poll the captured stdout for the endpoint line (up to ~10 s).
    ENDPOINT=""
    for _ in $(seq 1 100); do
        if ! kill -0 "$HEADLESS_PID" 2>/dev/null; then
            echo "ERROR: h2player exited before providing an endpoint" >&2
            cat "$TMP_OUT" >&2
            exit 1
        fi
        ENDPOINT=$(grep -m1 '^Endpoint: ' "$TMP_OUT" 2>/dev/null \
                   | sed 's/^Endpoint: //') || true
        if [ -n "$ENDPOINT" ]; then
            break
        fi
        sleep 0.1
    done

    if [ -z "$ENDPOINT" ]; then
        echo "ERROR: Could not find IPC endpoint in h2player output" >&2
        cat "$TMP_OUT" >&2
        exit 1
    fi

    echo ">>> IPC endpoint: $ENDPOINT"
    echo ">>> Starting GUI..."

    ./build/src/gui/hydrogen \
        -VIpc --connect-via-ipc "$ENDPOINT" \
        -T -L /tmp/hydrogen/editor.log --child &
    GUI_PID=$!

    # Block until either process exits; the trap cleans up the survivor.
    wait

# ── Plugin artifacts (CLAP, LV2, VST3) against a static Qt ────────────────
#
# build.sh remains the entry point for the regular application builds against
# the shared system Qt. The targets below build the plugin artifacts against a
# *static* Qt using QT_NAMESPACE instead, so the resulting bundles won't result
# in symbol clashes when loaded into a plugin host or alongside other plugins
# using another Qt version.
#
# Process flow - every step fails fast and does not start unless its
# prerequisites are in place:
#
#     plugin-preflight -> configure -> compile -> unit tests -> install
#
# The VST3 SDK is fetched automatically at configure time (CPM). The
# conformance tools (clap-validator, lv2lint) are picked up when they have
# been built (see build.sh's `deps` command); their tests are skipped
# otherwise.

# Build directory. Deliberately separate from build.sh's `build` and
# `build-appimage` directories: those cache a configuration against the
# shared system Qt, and mixing in the static-Qt configuration would
# invalidate their caches (and vice versa).
BUILD_DIR := env_var_or_default("BUILD_DIR", "build-plugins")

# Root of the static Qt installation the plugin artifacts are linked
# against. The plugin-preflight target checks that this points at a Qt built
# as a static library.
QT6_STATIC_DIR := env_var_or_default("QT6_STATIC_DIR", home_dir() + "/software/Qt6.11-static")

# Prefix the `plugin-install` target installs into. The plugin bundles land in
# <PREFIX>/lib/{clap,lv2,vst3} and the editor binary in <PREFIX>/bin (the
# CLAP artifact locates it via a relative path, so the prefix layout must be
# kept intact). The prefix is baked into the build directory at configure
# time (it also determines the system data path the docs are installed to),
# so changing it requires re-running `plugin-build`.
PREFIX := env_var_or_default("PREFIX", "dist")

# Number of parallel compile jobs.
JOBS := env_var_or_default("JOBS", num_cpus())

# CMake options, mirroring the plugin-relevant defaults of build.sh - except
# for WANT_SHARED: with a static Qt the core library must be linked statically
# too. A shared core would embed its own copy of the static Qt while every
# final binary (tests, plugin bundles) embeds another one, putting two
# separate Qt libraries into one process. Their event dispatchers and
# signal/slot systems do not know about each other, which silently breaks
# everything that crosses the library boundary (event loops spin forever,
# signals never arrive). A static core makes each final binary link core and
# Qt together into a single Qt instance - and self-contained plugin bundles.
CMAKE_OPTIONS := "-DCMAKE_COLOR_MAKEFILE=1 -DWANT_SHARED=0 -DWANT_DEBUG=1 -DWANT_QT6=1 -DWANT_JACK=1 -DWANT_ALSA=1 -DWANT_LIBARCHIVE=1 -DWANT_RUBBERBAND=1 -DWANT_CLAP=1 -DWANT_LV2=1 -DWANT_VST3=1 -DWANT_PORTAUDIO=1 -DWANT_PORTMIDI=1 -DWANT_COREAUDIO=1 -DWANT_COREMIDI=1 -DWANT_INTEGRATION_TESTS=1"

# Hardening flags, mirroring build.sh.
CXXFLAGS := "-fstrict-enums -fstack-protector-strong -Werror=format-security -Wformat -Wunused-result -D_FORTIFY_SOURCE=2"

# List the available targets.
default:
    @just --list

# Remove the plugin build directory.
plugin-clean:
    rm -rf "{{BUILD_DIR}}"

# Configure and compile the CLAP, LV2 and VST3 plugins against the static
# Qt.
plugin-build: plugin-preflight
    cmake -S . -B "{{BUILD_DIR}}" \
        -DCMAKE_PREFIX_PATH="{{QT6_STATIC_DIR}}" \
        -DCMAKE_INSTALL_PREFIX="{{PREFIX}}" \
        -DCMAKE_CXX_FLAGS="{{CXXFLAGS}}" \
        {{CMAKE_OPTIONS}}
    cmake --build "{{BUILD_DIR}}" --parallel "{{JOBS}}"

# Run the unit test suite and the CTest suite (GUI smoke test plus the
# plugin conformance tests registered by the build).
plugin-test:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ ! -x "{{BUILD_DIR}}/src/tests/tests" ]; then
        echo "ERROR: no test binary in {{BUILD_DIR}} - run 'just plugin-build' first" >&2
        exit 1
    fi
    # The `-o` file captures the (verbose) logger output for post-mortem
    # inspection; the CppUnit summary itself is printed to stdout.
    "{{BUILD_DIR}}/src/tests/tests" -VIpc -o "{{BUILD_DIR}}/test.log"
    cd "{{BUILD_DIR}}" && ctest --output-on-failure

# Install the build results into {{PREFIX}}.
plugin-install:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ ! -e "{{BUILD_DIR}}/CMakeCache.txt" ]; then
        echo "ERROR: {{BUILD_DIR}} is not configured - run 'just plugin-build' first" >&2
        exit 1
    fi
    cmake --install "{{BUILD_DIR}}"
    # Per-user plugin folders scanned by hosts on the current platform.
    if [ "$(uname)" = "Darwin" ]; then
        PLUGIN_ROOT="${HOME}/Library/Audio/Plug-Ins"
        LV2_DIR="${PLUGIN_ROOT}/LV2"
        CLAP_DIR="${PLUGIN_ROOT}/CLAP"
        VST3_DIR="${PLUGIN_ROOT}/VST3"
    else
        LV2_DIR="${HOME}/.lv2"
        CLAP_DIR="${HOME}/.clap"
        VST3_DIR="${HOME}/.vst3"
    fi
    # The folders must exist: `cp -r` into a missing destination would
    # rename the bundle to the folder name instead of copying it inside.
    # Old copies are removed first so no stale files survive in a bundle.
    mkdir -p "${LV2_DIR}" "${CLAP_DIR}" "${VST3_DIR}"
    rm -rf "${LV2_DIR}/hydrogen.lv2" "${CLAP_DIR}/Hydrogen.clap" \
        "${VST3_DIR}/Hydrogen.vst3"
    cp -R "{{PREFIX}}/lib/lv2/hydrogen.lv2" "${LV2_DIR}/"
    cp -R "{{PREFIX}}/lib/clap/Hydrogen.clap" "${CLAP_DIR}/"
    cp -R "{{PREFIX}}/lib/vst3/Hydrogen.vst3" "${VST3_DIR}/"
    echo "Installed plugins into ${LV2_DIR}, ${CLAP_DIR} and ${VST3_DIR}"

# plugin-clean + plugin-build + plugin-test + plugin-install in one go.
plugin: plugin-clean plugin-build plugin-test plugin-install

# Fail early on missing prerequisites: the toolchain, a static Qt at
# QT6_STATIC_DIR and the plugin submodules. Every check names the knob to
# turn before giving up.
[private]
plugin-preflight:
    #!/usr/bin/env bash
    set -euo pipefail
    command -v cmake > /dev/null || { echo "ERROR: cmake not found" >&2; exit 1; }
    if [ ! -f "{{QT6_STATIC_DIR}}/lib/cmake/Qt6/Qt6Config.cmake" ]; then
        echo "ERROR: no Qt installation found at {{QT6_STATIC_DIR}}" >&2
        echo "       Set QT6_STATIC_DIR to the root of a Qt installation." >&2
        exit 1
    fi
    if [ ! -f "{{QT6_STATIC_DIR}}/lib/libQt6Core.a" ]; then
        echo "ERROR: the Qt at {{QT6_STATIC_DIR}} is not a static build (no lib/libQt6Core.a)" >&2
        echo "       Set QT6_STATIC_DIR to the root of a static Qt installation." >&2
        exit 1
    fi
    for canary in extern/clap/include extern/lv2/include extern/clap-wrapper/CMakeLists.txt; do
        if [ ! -e "$canary" ]; then
            echo "ERROR: submodule $canary is missing - run 'git submodule update --init'" >&2
            exit 1
        fi
    done

# ── Portable universal macOS bundles (arm64 + x86_64) ─────────────────────
#
# Developer entry point for release-grade macOS artifacts. Regular local builds
# (a single architecture, linked against Homebrew) stay with build.sh.
#
# All dependencies - including Qt - are built from pinned sources as static,
# universal libraries for MACOS_TARGET (see macos/build_universal_deps.sh).
# The resulting app and plugin bundles therefore only link against libraries
# shipped with macOS and run on every Mac with MACOS_TARGET or newer.
#
#     macos-deps -> macos-universal -> macos-test -> macos-install / macos-dmg
#
# JACK is disabled: on macOS it can only be linked dynamically and the app
# would refuse to start on systems without it.

# Options of the portable builds. Unlike CMAKE_OPTIONS no driver requiring
# third-party libraries beyond the ones built by `macos-deps` is enabled.
MACOS_CMAKE_OPTIONS := "-DCMAKE_COLOR_MAKEFILE=1 -DCMAKE_BUILD_TYPE=Release -DWANT_SHARED=0 -DWANT_DEBUG=0 -DWANT_QT6=1 -DWANT_JACK=0 -DWANT_ALSA=0 -DWANT_PULSEAUDIO=0 -DWANT_OSC=0 -DWANT_PORTAUDIO=0 -DWANT_PORTMIDI=0 -DWANT_COREAUDIO=1 -DWANT_COREMIDI=1 -DWANT_LIBARCHIVE=1 -DWANT_RUBBERBAND=1 -DWANT_CLAP=1 -DWANT_LV2=1 -DWANT_VST3=1 -DWANT_CPPUNIT=1 -DWANT_INTEGRATION_TESTS=0"

# Prefixes of Homebrew, MacPorts and manually installed libraries. Nothing
# must be picked up from there.
MACOS_IGNORED_PREFIXES := "/opt/homebrew;/usr/local;/opt/local"

# Finished dependencies are skipped, so only the first run takes long (~1h).
# Build static, universal Qt and libraries into MACOS_DEPS_DIR.
macos-deps:
    macos/build_universal_deps.sh "{{MACOS_SOURCE_DIR}}" "{{MACOS_DEPS_DIR}}" "{{MACOS_TARGET}}" "{{JOBS}}"

# Remove the universal build directory and staging prefix (keeps MACOS_DEPS_DIR).
macos-clean:
    rm -rf "{{MACOS_BUILD_DIR}}" "{{MACOS_PREFIX}}"

# Build the universal app and plugins, stage them in MACOS_PREFIX and verify them.
macos-universal: macos-preflight
    #!/usr/bin/env bash
    set -euo pipefail
    # Only the libraries of MACOS_DEPS_DIR are visible to pkg-config, also
    # when CMake re-runs the configuration during the build.
    export PKG_CONFIG_LIBDIR="{{MACOS_DEPS_DIR}}/lib/pkgconfig"
    unset PKG_CONFIG_PATH CPATH LIBRARY_PATH
    export MACOSX_DEPLOYMENT_TARGET="{{MACOS_TARGET}}"
    cmake -S . -B "{{MACOS_BUILD_DIR}}" \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="{{MACOS_TARGET}}" \
        -DCMAKE_PREFIX_PATH="{{MACOS_DEPS_DIR}}/qt;{{MACOS_DEPS_DIR}}" \
        -DCMAKE_IGNORE_PREFIX_PATH="{{MACOS_IGNORED_PREFIXES}}" \
        -DCMAKE_SYSTEM_IGNORE_PREFIX_PATH="{{MACOS_IGNORED_PREFIXES}}" \
        -DPKG_CONFIG_EXECUTABLE="$(command -v pkg-config || command -v pkgconf)" \
        -DH2_MACOS_EXTRA_PREFIX="" \
        -DCMAKE_INSTALL_PREFIX="{{MACOS_PREFIX}}" \
        -DCMAKE_CXX_FLAGS="{{CXXFLAGS}}" \
        {{MACOS_CMAKE_OPTIONS}}
    cmake --build "{{MACOS_BUILD_DIR}}" --parallel "{{JOBS}}"

    rm -rf "{{MACOS_PREFIX}}"
    cmake --install "{{MACOS_BUILD_DIR}}"
    APP="{{MACOS_PREFIX}}/bin/hydrogen.app"
    # The translations are generated into the source tree and not part of
    # the bundle in the build directory.
    mkdir -p "$APP/Contents/Resources/data/i18n"
    cp data/i18n/*.qm "$APP/Contents/Resources/data/i18n/"
    # Installing rewrites the binaries, which invalidates the signature the
    # linker created. Apple Silicon refuses to run unsigned code.
    for item in "$APP" \
                "{{MACOS_PREFIX}}/lib/clap/Hydrogen.clap" \
                "{{MACOS_PREFIX}}/lib/vst3/Hydrogen.vst3" \
                "{{MACOS_PREFIX}}/lib/lv2/hydrogen.lv2/hydrogen.so"; do
        codesign --force --sign - "$item"
    done
    just macos-check

# Verify the staged bundles are portable universal binaries.
macos-check:
    tools/check_macos_portable "{{MACOS_TARGET}}" \
        "{{MACOS_PREFIX}}/bin/hydrogen.app" \
        "{{MACOS_PREFIX}}/lib/clap/Hydrogen.clap" \
        "{{MACOS_PREFIX}}/lib/vst3/Hydrogen.vst3" \
        "{{MACOS_PREFIX}}/lib/lv2/hydrogen.lv2"

# Run the unit tests for arm64 and x86_64 (Rosetta), the CTest suite, and verify the bundles.
macos-test:
    #!/usr/bin/env bash
    set -euo pipefail
    TESTS="{{MACOS_BUILD_DIR}}/src/tests/tests"
    if [ ! -x "$TESTS" ]; then
        echo "ERROR: no test binary in {{MACOS_BUILD_DIR}} - run 'just macos-universal' first" >&2
        exit 1
    fi
    if ! arch -x86_64 /usr/bin/true 2> /dev/null; then
        echo "ERROR: Rosetta is required for the x86_64 tests - run 'softwareupdate --install-rosetta'" >&2
        exit 1
    fi
    for ARCH in arm64 x86_64; do
        echo "=== Unit tests ($ARCH) ==="
        arch "-$ARCH" "$TESTS" -VIpc -o "{{MACOS_BUILD_DIR}}/test-$ARCH.log"
    done
    # GUI startup and plugin conformance tests (native architecture only).
    echo "=== CTest suite ==="
    (cd "{{MACOS_BUILD_DIR}}" && ctest --output-on-failure)
    just macos-check

# Install the staged app into MACOS_APP_DIR and the plugins into ~/Library/Audio/Plug-Ins.
macos-install:
    #!/usr/bin/env bash
    set -euo pipefail
    APP="{{MACOS_PREFIX}}/bin/hydrogen.app"
    if [ ! -d "$APP" ]; then
        echo "ERROR: no staged bundles in {{MACOS_PREFIX}} - run 'just macos-universal' first" >&2
        exit 1
    fi
    rm -rf "{{MACOS_APP_DIR}}/Hydrogen.app"
    ditto "$APP" "{{MACOS_APP_DIR}}/Hydrogen.app"
    # The plugins find the editor and the shipped data via LaunchServices.
    /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister \
        -f "{{MACOS_APP_DIR}}/Hydrogen.app"
    PLUGIN_ROOT="${HOME}/Library/Audio/Plug-Ins"
    mkdir -p "${PLUGIN_ROOT}/LV2" "${PLUGIN_ROOT}/CLAP" "${PLUGIN_ROOT}/VST3"
    rm -rf "${PLUGIN_ROOT}/LV2/hydrogen.lv2" "${PLUGIN_ROOT}/CLAP/Hydrogen.clap" \
        "${PLUGIN_ROOT}/VST3/Hydrogen.vst3"
    ditto "{{MACOS_PREFIX}}/lib/lv2/hydrogen.lv2" "${PLUGIN_ROOT}/LV2/hydrogen.lv2"
    ditto "{{MACOS_PREFIX}}/lib/clap/Hydrogen.clap" "${PLUGIN_ROOT}/CLAP/Hydrogen.clap"
    ditto "{{MACOS_PREFIX}}/lib/vst3/Hydrogen.vst3" "${PLUGIN_ROOT}/VST3/Hydrogen.vst3"
    echo "Installed {{MACOS_APP_DIR}}/Hydrogen.app and the plugins into ${PLUGIN_ROOT}"

# Pack the staged app and plugins into a disk image in MACOS_BUILD_DIR.
macos-dmg:
    #!/usr/bin/env bash
    set -euo pipefail
    APP="{{MACOS_PREFIX}}/bin/hydrogen.app"
    if [ ! -d "$APP" ]; then
        echo "ERROR: no staged bundles in {{MACOS_PREFIX}} - run 'just macos-universal' first" >&2
        exit 1
    fi
    VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$APP/Contents/Info.plist")
    DMG="{{MACOS_BUILD_DIR}}/Hydrogen-${VERSION}-universal.dmg"
    rm -f "$DMG"
    macos/build_dmg.sh -v -p "{{MACOS_PREFIX}}/lib" "$APP" "$DMG"
    echo "Created $DMG"

# macos-deps + macos-universal + macos-test in one go.
macos: macos-deps macos-universal macos-test

# Fail early on missing prerequisites: the dependencies of `macos-deps`, the
# tools and the plugin submodules.
[private]
macos-preflight:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ "$(uname)" != "Darwin" ]; then
        echo "ERROR: universal macOS builds require macOS" >&2
        exit 1
    fi
    for tool in cmake codesign; do
        command -v "$tool" > /dev/null || { echo "ERROR: $tool not found" >&2; exit 1; }
    done
    if ! command -v pkg-config > /dev/null && ! command -v pkgconf > /dev/null; then
        echo "ERROR: pkg-config not found (e.g. 'brew install pkgconf')" >&2
        exit 1
    fi
    for lib in qt/lib/libQt6Core.a lib/libsndfile.a lib/libarchive.a lib/librubberband.a lib/libcppunit.a; do
        if [ ! -f "{{MACOS_DEPS_DIR}}/$lib" ]; then
            echo "ERROR: {{MACOS_DEPS_DIR}}/$lib missing - run 'just macos-deps' first" >&2
            exit 1
        fi
    done
    for canary in extern/clap/include extern/lv2/include extern/clap-wrapper/CMakeLists.txt; do
        if [ ! -e "$canary" ]; then
            echo "ERROR: submodule $canary is missing - run 'git submodule update --init'" >&2
            exit 1
        fi
    done
