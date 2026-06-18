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

#ifndef H2C_PLUGIN_TELEMETRY_SHM_H
#define H2C_PLUGIN_TELEMETRY_SHM_H

#include <core/Object.h>
#include <core/IPC/PluginTelemetry.h>

#include <QtCore/QSharedMemory>
#include <QtCore/QString>

namespace H2Core {

/**
 * Shared-memory carrier for the PluginTelemetry block (ADR 0018). The engine
 * side create()s the segment and store()s a snapshot every audio buffer; the
 * editor side attach()es and load()s it at its own refresh rate. Reads are
 * tear-free via the seqlock in PluginTelemetry (no mutex on the audio thread);
 * a formatVersion mismatch makes load() fail so the editor falls back to
 * events-only telemetry.
 *
 * \ingroup docCore
 */
class PluginTelemetryShm : public H2Core::Object<PluginTelemetryShm> {
	H2_OBJECT(PluginTelemetryShm)
public:
	PluginTelemetryShm();
	~PluginTelemetryShm();

	/** Engine side: create and zero-initialize the segment. */
	bool create( const QString& sKey );
	/** Editor side: attach to an existing segment (read-only). */
	bool attach( const QString& sKey );
	void detach();
	bool isValid() const;

	/** Engine side: publish a snapshot (wait-free, audio-thread safe). */
	bool store( const PluginTelemetrySnapshot& snapshot );
	/** Editor side: read a tear-free snapshot; false on version mismatch. */
	bool load( PluginTelemetrySnapshot& out ) const;

private:
	QSharedMemory m_shm;
	bool m_bOwner;
};

};

#endif
