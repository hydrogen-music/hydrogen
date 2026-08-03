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
