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

#ifndef H2C_IPC_ENGINE_EVENT_SINK_H
#define H2C_IPC_ENGINE_EVENT_SINK_H

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>

#include <QtCore/QString>

#include <vector>

namespace H2Core {

struct EngineTelemetrySnapshot;

/**
 * One info snapshot (ADR 0035 UI-3): the song-level state the embedded
 * UI's info section shows — identity, mode, per-mode counts and
 * playback position — built on the engine session's bridge thread
 * under the same small-budget engine lock as the telemetry publish
 * (amendment to ADR 0018), so a UI consumer never takes the engine
 * lock itself. Delivered via EngineEventSink::onInfoSnapshot on the
 * publish cycle; a degraded cycle (lock contention) is skipped
 * entirely rather than delivered blank — the consumer keeps its last
 * snapshot. Field names mirror the landing model's former SongInfo.
 */
struct EngineInfoSnapshot {
	QString sSongName;
	QString sDrumkitName;
	Song::Mode mode = Song::Mode::None;
	int nPatternCount = 0;
	int nColumnCount = 0;
	int nCurrentColumn = -1;
	std::vector<QString> playingPatterns;
	QString sPatternName;
	int nPatternLength = 0;
	int nPatternNoteCount = 0;

	bool operator==( const EngineInfoSnapshot& other ) const {
		return sSongName == other.sSongName &&
			sDrumkitName == other.sDrumkitName && mode == other.mode &&
			nPatternCount == other.nPatternCount &&
			nColumnCount == other.nColumnCount &&
			nCurrentColumn == other.nCurrentColumn &&
			playingPatterns == other.playingPatterns &&
			sPatternName == other.sPatternName &&
			nPatternLength == other.nPatternLength &&
			nPatternNoteCount == other.nPatternNoteCount;
	}
	bool operator!=( const EngineInfoSnapshot& other ) const {
		return ! ( *this == other );
	}
};

/**
 * \ingroup docCore
 *
 * Engine-process consumer of the authoritative engine's event stream and
 * telemetry (ADR 0035 UI-3) — the seam the embedded plugin UI (and any
 * other engine-process observer) attaches to. A sink registered via
 * EngineSession::setEventSink() receives exactly the engine-origin events
 * and telemetry snapshots the IPC editor receives, whether or not an
 * editor is attached.
 *
 * Threading contract: all callbacks fire on the engine session's bridge
 * thread — implementations must be cheap and non-blocking (in particular,
 * never take the audio engine lock). The sink is called live from
 * registration onward; error events are additionally retained for the
 * editor-attach replay (ADR 0026 point 9), which the sink does not see
 * again. The owner must unregister (pass nullptr) before the
 * implementing object is destroyed.
 */
class EngineEventSink {
public:
	virtual ~EngineEventSink() = default;

	/** One engine-origin event, as drained off the engine's EventQueue. */
	virtual void onEvent( Event::Type type, int nValue, long nId ) = 0;

	/** One telemetry snapshot (transport / peaks / process time, ADR
	 *  0018), as built for the periodic publish — the same snapshot the
	 *  SHM block stores. */
	virtual void onMeterSnapshot(
		const EngineTelemetrySnapshot& snapshot ) = 0;

	/** One info snapshot (see #EngineInfoSnapshot), as built for the
	 *  periodic publish. Skipped for a degraded cycle (engine-lock
	 *  contention): the consumer keeps its last snapshot — a skipped
	 *  cycle must never read as "no song". Default: not interested. */
	virtual void onInfoSnapshot( const EngineInfoSnapshot& ) {}
};

}

#endif
