# Fail on the first error, on unset variables and on failing pipes in every
# recipe. Recipes with a shebang line (like `headless` below) run as a single
# script and bring their own `set` line instead.
set shell := ["bash", "-euo", "pipefail", "-c"]

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
    cp -r dist/lib/lv2/hydrogen.lv2 ${HOME}/.lv2
    cp -r dist/lib/clap/Hydrogen.clap ${HOME}/.clap
    cp -r dist/lib/vst3/Hydrogen.vst3 ${HOME}/.vst3
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
