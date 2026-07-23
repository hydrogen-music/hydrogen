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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_IPC_HEADLESS_ENGINE_LAUNCHER_H
#define H2C_IPC_HEADLESS_ENGINE_LAUNCHER_H

#include <QtCore/QString>

namespace H2Core {

class Hydrogen;

/**
 * Shared utilities for launching headless Hydrogen engines with IPC capability.
 * Used by plugins, h2player, and future tools that need to serve an engine over IPC.
 */
class HeadlessEngineLauncher {
public:
	/**
	 * Create a headless Hydrogen instance configured for IPC serving.
	 * Uses Null audio driver (no audio processing), no MIDI, no OSC.
	 * Caller owns the returned Hydrogen instance.
	 */
	static Hydrogen* createHeadlessEngine();

	/**
	 * Generate a unique IPC endpoint name for this process.
	 * Format: "hydrogen-headless-<pid>-<counter>"
	 */
	static QString makeEndpoint();

	/**
	 * Format connection information message for the user.
	 * Returns a multi-line string with the endpoint and connection command.
	 */
	static QString formatConnectionInfo( const QString& sEndpoint );

private:
	HeadlessEngineLauncher() = delete;
};

} // namespace H2Core

#endif