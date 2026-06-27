#!/bin/sh
# Regression test for the --plugin-editor bootstrap (ADR 0016).
#
# `hydrogen --plugin-editor <endpoint>` starts the out-of-process editor, which
# connects to the engine over IPC. When the endpoint has no engine listening, the
# editor must abort cleanly. Historically it segfaulted: the headless mirror was
# created with the *Fake* audio driver, which spawns a processing thread that
# continuously logs "Failed to lock audioEngine"; the abort path then freed the
# Logger while that thread was still logging → use-after-free.
#
# This test runs the (child) editor against a non-existent endpoint and asserts:
#   1. it exits cleanly with code 1 (the "couldn't connect" abort) — NOT a crash,
#   2. it did NOT run an audio-processing thread on the mirror (no "Failed to lock
#      audioEngine" output) — the precondition for the use-after-free.
#
# Usage: test_editor_bad_endpoint.sh <hydrogen-binary> <data-dir> [expected-code]
# expected-code defaults to 3 = Reporter::EXIT_CODE_CLEAN_FAILURE (keep in sync).
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

BIN="$1"
DATA="$2"
EXPECTED="${3:-3}"
if [ -z "$BIN" ] || [ -z "$DATA" ]; then
	echo "usage: $0 <hydrogen-binary> <data-dir> [expected-code]" >&2
	exit 2
fi

OUT=$( "$BIN" --nosplash --child \
		--plugin-editor h2-nonexistent-editor-endpoint-for-test \
		-P "$DATA" 2>&1 )
EC=$?

printf '%s\n' "$OUT"

if [ "$EC" -ne "$EXPECTED" ]; then
	echo "FAIL: expected a clean exit code $EXPECTED (could not connect), got $EC" \
		"(a crash/signal is 128+; the editor bootstrap aborted abnormally)" >&2
	exit 1
fi

if printf '%s\n' "$OUT" | grep -q 'Failed to lock audioEngine'; then
	echo "FAIL: the editor mirror ran an audio-processing thread (it should use" \
		"the passive Null driver); this is the thread that raced the Logger" \
		"teardown and caused the segfault." >&2
	exit 1
fi

echo "OK: editor aborted cleanly (exit 1), no rogue audio-processing thread"
exit 0
