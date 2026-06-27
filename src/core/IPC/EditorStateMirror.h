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
#include <core/IPC/PluginTelemetry.h>
#include <core/IPC/PluginTelemetryShm.h>

#include <QtCore/QObject>
#include <QtCore/QString>

class QTimer;

namespace H2Core {

class Hydrogen;
class IpcChannel;

/**
 * \ingroup docCore
 *
 * Keeps an editor-process state mirror in sync with the authoritative engine
 * living in the plugin host process (ADR 0016). The editor runs a *headless*
 * #Hydrogen whose Song / Preferences / Playlist / SoundLibraryDatabase the GUI
 * reads locally (via #IpcEngineAccess), and this class applies the inbound IPC
 * stream onto that mirror so it tracks the engine:
 *
 * - **Events** (engine → editor) are re-posted onto the mirror's #EventQueue, so
 *   the GUI reacts exactly as it would to a local engine.
 * - **Song / state snapshots** (#IpcOpcode::SetSong / ::LoadState) replace the
 *   mirror's song, so structural edits made engine-side appear in the editor.
 * - **Telemetry** snapshots (transport position / BPM / peaks) are read from the
 *   lock-free QSharedMemory block (not the socket) and used to keep the mirror's
 *   transport following the host (ADR 0031 hybrid sync): the mirror's own clock
 *   free-runs the playhead for smoothness, inbound transport events trigger an
 *   immediate correction, and a periodic timer bounds long-run drift.
 *
 * Only inbound (engine → editor) synchronisation lives here; outbound commands
 * (editor → engine) are issued by #IpcEngineAccess. The mirror never *initiates*
 * transport — play/stop/relocate are host-only and read-only here (ADR 0026); it
 * only follows.
 */
class EditorStateMirror : public QObject {
	Q_OBJECT
public:

	/** Number of milliseconds between forced resyncs of the mirrors transport
	 * state with respect to the telemetry data. */
	static constexpr int nResyncTimeoutMs = 5000;

	/** \param pMirror the editor-side headless engine to keep in sync; not owned. */
	explicit EditorStateMirror( Hydrogen* pMirror, QObject* pParent = nullptr );
	~EditorStateMirror() override;

	/** Connect to a channel so inbound messages are applied automatically. */
	void attach( IpcChannel* pChannel );

	/** Apply one inbound message to the mirror. Returns true if it changed
	 * mirror state (event posted / song replaced); false for messages this
	 * mirror does not consume (e.g. Hello, command opcodes). */
	bool applyMessage( const IpcMessage& msg );

	void setTelemetry( const PluginTelemetrySnapshot& snapshot ) {
		m_telemetry = snapshot; }
	const PluginTelemetrySnapshot& getTelemetry() const { return m_telemetry; }

	/** Attach to the engine's telemetry block (keyed off the IPC endpoint) and
	 * start the periodic drift-correction timer. No-op / events-only fallback if
	 * the block is absent or its layout version mismatches. */
	void attachTelemetry( const QString& sEndpoint );

	/** Apply one transport snapshot to the mirror: follow play/stop, tempo, and
	 * (only when stopped, or on a large divergence) the absolute frame. Public so
	 * it is unit-testable with an injected snapshot, bypassing shared memory. */
	void applyTransportSnapshot( const PluginTelemetrySnapshot& snapshot );

private slots:
	void onMessageReceived( const H2Core::IpcMessage& msg );
	/** Load the latest telemetry and apply it (timer tick / transport event). */
	void syncTransportFromTelemetry();

private:
	/** Editor-side headless engine kept in sync; not owned. */
	Hydrogen* m_pMirror;
	PluginTelemetrySnapshot m_telemetry;
	/** Reader for the engine's telemetry block; invalid until attachTelemetry(). */
	PluginTelemetryShm m_telemetryShm;
	/** Periodic forced re-sync (~5 s) to bound drift; owned via QObject parenting. */
	QTimer* m_pResyncTimer = nullptr;
};

}

#endif
