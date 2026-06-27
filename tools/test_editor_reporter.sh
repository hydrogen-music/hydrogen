#!/bin/sh
# Regression test for the crash Reporter vs. a clean editor failed-connect.
#
# `hydrogen --plugin-editor <endpoint>` (NO --child) runs through the crash
# Reporter, which re-spawns the real app as a child and watches it. When the
# editor cannot reach its engine endpoint it aborts cleanly with
# Reporter::EXIT_CODE_CLEAN_FAILURE — a NORMAL exit, not a crash. The Reporter
# must propagate that code and must NOT pop the "Hydrogen exited abnormally"
# crash dialog (which, headless, blocks forever — and on a desktop wrongly
# presents a user error as a crash to report).
#
# Asserts the full command exits with the clean-failure code, without hanging on
# the dialog (timeout) or crashing (signal).
#
# Usage: test_editor_reporter.sh <hydrogen-binary> <data-dir> [expected-code]
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

timeout 30 "$BIN" --nosplash \
	--plugin-editor h2-nonexistent-editor-endpoint-for-test \
	-P "$DATA" >/dev/null 2>&1
EC=$?

if [ "$EC" -eq 124 ]; then
	echo "FAIL: the command hung — the Reporter popped the crash dialog for a" \
		"clean failed-connect (it blocks with no display)" >&2
	exit 1
fi
if [ "$EC" -ge 128 ]; then
	echo "FAIL: terminated by a signal (exit $EC) — a crash, not a clean abort" >&2
	exit 1
fi
if [ "$EC" -ne "$EXPECTED" ]; then
	echo "FAIL: expected the clean-failure exit code $EXPECTED to propagate" \
		"through the Reporter, got $EC" >&2
	exit 1
fi

echo "OK: Reporter propagated the clean failed-connect (exit $EXPECTED)," \
	"no crash dialog"
exit 0
