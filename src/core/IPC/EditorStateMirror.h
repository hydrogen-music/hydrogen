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

#ifndef H2C_IPC_EDITOR_STATE_MIRROR_H
#define H2C_IPC_EDITOR_STATE_MIRROR_H

#include <core/IPC/IpcMessage.h>
#include <core/IPC/EngineTelemetry.h>
#include <core/IPC/EngineTelemetryShm.h>
#include <core/Object.h>

#include <QtCore/QObject>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace H2Core {

class Hydrogen;
class IpcChannel;

/**
 * \ingroup docCore
 *
 * Keeps an editor-process state mirror in sync with the authoritative engine
 * living in the plugin host or (remote) headless engine process (ADR 0016). The
 * editor runs a (local) *headless* #Hydrogen whose Song / Preferences /
 * Playlist / SoundLibraryDatabase the GUI reads locally (via #IpcEngineAccess),
 * and this class applies the inbound IPC stream onto that mirror so it tracks
 * the engine:
 *
 * - **Events** (engine → editor) are re-posted onto the mirror's #EventQueue, so
 *   the GUI reacts exactly as it would to a local engine.
 * - **Song / state snapshots** (#IpcOpcode::SetSong / ::LoadState) replace the
 *   mirror's song, so structural edits made engine-side appear in the editor.
 * - **Telemetry** snapshots (transport position / BPM / peaks / process time)
 *   are read from the lock-free QSharedMemory block (not the socket) and used
 *   at two cadences (ADR 0018): the meter half (master / per-instrument /
 *   playback-track peaks) is applied onto the mirror's regular state holders
 *   at ~20 Hz so the GUI's existing meters work unchanged, while the transport
 *   half keeps the mirror following the host/headless engine (ADR 0031 hybrid
 *   sync): the mirror's own clock free-runs the playhead for smoothness,
 *   inbound transport events trigger an immediate correction, and a periodic
 *   timer bounds long-run drift.
 *
 * Only inbound (engine → editor) synchronisation lives here; outbound commands
 * (editor → engine) are issued by #IpcEngineAccess. The mirror never
 * *initiates* transport — play/stop/relocate are host/headless engine-only and
 * read-only here (ADR 0026); it only follows.
 */
class EditorStateMirror : public QObject,
						  public H2Core::Object<EditorStateMirror> {
	H2_OBJECT( EditorStateMirror )
	Q_OBJECT
public:

	/** Number of milliseconds between forced resyncs of the mirrors transport
	 * state with respect to the telemetry data. */
	static constexpr int nResyncTimeoutMs = 5000;

	/** Number of milliseconds between meter syncs (peaks / process time) from
	 * the telemetry block — matching the GUI's meter refresh cadence (ADR
	 * 0018). Transport sync deliberately does NOT run at this rate: snapping
	 * a rolling playhead at 20 Hz would jerk it (ADR 0031). */
	static constexpr int nMeterSyncTimeoutMs = 50;

	/** \param pMirror the editor-side headless engine to keep in sync; not owned. */
	explicit EditorStateMirror( Hydrogen* pMirror, QObject* pParent = nullptr );
	~EditorStateMirror() override;

	/** Connect to a channel so inbound messages are applied automatically. */
	void attach( IpcChannel* pChannel );

	/** Apply one inbound engine event to the mirror. Returns true if the event
	 * was posted to the mirror's EventQueue; false for non-Event messages. */
	bool applyEvent( const IpcMessage& msg );

	void setTelemetry( const EngineTelemetrySnapshot& snapshot ) {
		m_telemetry = snapshot; }
	const EngineTelemetrySnapshot& getTelemetry() const { return m_telemetry; }

	/** Attach to the engine's telemetry block (keyed off the IPC endpoint) and
	 * start the periodic timers: the ~20 Hz meter sync and the ~5 s transport
	 * drift correction. When the block is not there yet (the engine's serve
	 * loop reports its endpoint before creating the block), only the drift
	 * timer runs and every forced / event-triggered sync retries the attach —
	 * a late block still brings the meters alive. A layout-version mismatch
	 * keeps the events-only fallback. */
	void attachTelemetry( const QString& sEndpoint );

	/** Apply one transport snapshot to the mirror: follow play/stop, tempo, and
	 * (only when stopped, or on a large divergence) the absolute frame. Public so
	 * it is unit-testable with an injected snapshot, bypassing shared memory. */
	void applyTransportSnapshot( const EngineTelemetrySnapshot& snapshot );

	/** Apply the meter half of a telemetry snapshot to the mirror's regular
	 * state holders — #AudioEngine master peaks, per-instrument peaks in
	 * drumkit order, playback-track peaks — so the GUI's existing meter
	 * consumers (Mixer faders, playback-track fader) work unchanged in editor
	 * mode (ADR 0018). Values are max-merged (a blip between two applies can
	 * not be lost before the GUI consumes it); the reset stays with the GUI's
	 * consume calls (ADR 0027). Transport fields are deliberately ignored —
	 * they belong to #applyTransportSnapshot and its hybrid cadence (ADR
	 * 0031). Public so it is unit-testable with an injected snapshot. */
	void applyMeterSnapshot( const EngineTelemetrySnapshot& snapshot );

	/** Force an immediate transport re-sync from the telemetry block. Public so
	 * the GUI can trigger it right after connecting, without waiting for the
	 * periodic timer. No-op when no telemetry block is attached. */
	void forceTransportSync();

private slots:
	void onMessageReceived( const H2Core::IpcMessage& msg );
	/** Load the latest telemetry and apply it (timer tick / transport event). */
	void syncTransportFromTelemetry();
	/** Load the latest telemetry and apply its meter half (meter timer tick). */
	void syncMetersFromTelemetry();

private:
	/** Attach the shm block if not yet attached and an endpoint is known, and
	 * start the meter timer on success. Called from attachTelemetry() and
	 * retried from syncTransportFromTelemetry() so a block appearing late
	 * (engine serve() startup race) still activates telemetry. */
	void tryAttachTelemetry();

	/** Editor-side headless engine kept in sync; not owned. */
	Hydrogen* m_pMirror;
	EngineTelemetrySnapshot m_telemetry;
	/** Reader for the engine's telemetry block; invalid until attachTelemetry(). */
	EngineTelemetryShm m_telemetryShm;
	/** Endpoint the telemetry block is keyed off; remembered by
	 * attachTelemetry() for the attach retries in #tryAttachTelemetry. */
	QString m_sTelemetryEndpoint;
	/** Periodic forced re-sync (~5 s) to bound drift; owned via QObject parenting. */
	QTimer* m_pResyncTimer = nullptr;
	/** Meter-cadence (~20 Hz) peak / process-time sync; created by
	 * tryAttachTelemetry() once the block is attached; owned via QObject
	 * parenting. */
	QTimer* m_pMeterTimer = nullptr;
};
}

#endif
