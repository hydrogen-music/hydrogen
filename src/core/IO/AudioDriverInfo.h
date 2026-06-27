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

#ifndef H2C_AUDIO_DRIVER_INFO_H
#define H2C_AUDIO_DRIVER_INFO_H

#include <QString>

#include <core/Preferences/Preferences.h>

namespace H2Core {

/**
 * \ingroup docCore docAudioDriver
 *
 * Value description of the active audio driver (ADR 0029).
 *
 * This replaces GUI-side `std::dynamic_pointer_cast` on the live #AudioDriver
 * pointer: the GUI branches on #kind and reads #isRunning / #isPresent instead
 * of probing RTTI on an engine object that, in editor mode, lives in another
 * process and cannot be `dynamic_cast` across the wire. It is filled engine-side
 * — #LocalEngineAccess from the live driver (standalone), #IpcEngineAccess from a
 * query (editor mode) — and crosses IPC as plain fields.
 */
struct AudioDriverInfo {
	/** Concrete kind of the running driver (None when no driver object exists). */
	Preferences::AudioDriver kind = Preferences::AudioDriver::None;
	/** A driver object exists (including the #StubAudioDriver fallback). Mirrors the
	 * old `getAudioDriver() != nullptr` check. */
	bool isPresent = false;
	/** A real (non-#StubAudioDriver) audio driver is connected. Replaces the GUI's
	 * `dynamic_pointer_cast<StubAudioDriver>(...) == nullptr` "is it real?" gate. */
	bool isRunning = false;
	/** Device the running driver is connected to, where the driver exposes one
	 * (e.g. ALSA); empty otherwise. For display only. */
	QString connectedDevice;
};

}

#endif
