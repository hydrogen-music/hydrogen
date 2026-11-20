/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_MIDI_DRIVER_INFO_H
#define H2C_MIDI_DRIVER_INFO_H

namespace H2Core {

/**
 * \ingroup docCore docMIDI
 *
 * Value status of the active MIDI driver (ADR 0029, MIDI counterpart).
 *
 * Replaces GUI-side `getMidiDriver() != nullptr` / `isInputActive()` /
 * `isOutputActive()` probes on the live #MidiBaseDriver pointer, which in editor
 * mode lives in another process. Filled engine-side (#LocalEngineAccess from the
 * live driver, #IpcEngineAccess from a query) and crosses IPC as plain fields.
 */
struct MidiDriverInfo {
	/** A MIDI driver object exists (mirrors the old `getMidiDriver() != nullptr`
	 * check). */
	bool isPresent = false;
	/** The driver's input is open/connected. */
	bool isInputActive = false;
	/** The driver's output is open/connected. */
	bool isOutputActive = false;
};

}

#endif
