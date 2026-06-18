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

#include <QtCore/QObject>

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
 * - **Telemetry** snapshots (transport position / BPM / peaks) are cached for the
 *   GUI's live readouts (the lock-free QSharedMemory channel, not the socket).
 *
 * Only inbound (engine → editor) synchronisation lives here; outbound commands
 * (editor → engine) are issued by #IpcEngineAccess.
 */
class EditorStateMirror : public QObject {
	Q_OBJECT
public:
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

private slots:
	void onMessageReceived( const H2Core::IpcMessage& msg );

private:
	/** Editor-side headless engine kept in sync; not owned. */
	Hydrogen* m_pMirror;
	PluginTelemetrySnapshot m_telemetry;
};

}

#endif
