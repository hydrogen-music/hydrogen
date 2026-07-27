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

#ifndef H2C_IPC_EDITOR_SESSION_H
#define H2C_IPC_EDITOR_SESSION_H

#include <QtCore/QString>

#include <memory>

namespace H2Core {

class Hydrogen;
class IpcChannel;
class EditorStateMirror;
class IpcEngineAccess;
class Preferences;

/**
 * \ingroup docCore
 *
 * Editor-process side of the editor↔engine split (ADR 0016/0018/0030). Given the
 * engine's control endpoint and a headless #Hydrogen mirror, #connect():
 *  - connects an #IpcChannel to the engine,
 *  - attaches an #EditorStateMirror so inbound events / song snapshots are
 *    applied to the mirror (the GUI reads the mirror's live objects locally), and
 *  - sends the `hello` handshake.
 *
 * The GUI then builds its #IEngineAccess from #createEngineAccess(): reads resolve
 * to the mirror, writes are forwarded over the channel.
 *
 * Owns the channel and the state mirror; does **not** own the #Hydrogen mirror
 * (its lifetime is the GUI's, as in standalone — see HydrogenApp). The engine
 * keeps running if this session (the editor process) goes away: the channel just
 * closes on its end (ADR 0016 — engine survives editor crash).
 */
class EditorSession {
public:
	/** Configure @a pPreferences for the editor-side mirror: a passive audio
	 * driver (Null — it must NOT process audio or spawn a processing thread;
	 * the authoritative engine in the headless engine does that), no MIDI
	 * driver and no OSC server (the headless engine owns control surfaces, ADR
	 * 0016/0026). */
	static void configureMirrorPreferences(
		std::shared_ptr<Preferences> pPreferences );

	/** Connect to @a sEndpoint and wire the editor stack around @a pMirror.
	 * Returns nullptr if @a pMirror is null, @a sEndpoint is empty, or the socket
	 * connection fails within @a nTimeoutMs. */
	static std::unique_ptr<EditorSession> connect(
		const QString& sEndpoint, Hydrogen* pMirror, int nTimeoutMs = 3000 );

	~EditorSession();

	IpcChannel* getChannel() const { return m_pChannel; }
	bool isConnected() const;

	/** Build a fresh editor-mode engine-access handle bound to this session's
	 * channel and mirror. Ownership transfers to the caller (the GUI). */
	std::unique_ptr<IpcEngineAccess> createEngineAccess() const;

private:
	EditorSession( Hydrogen* pMirror, IpcChannel* pChannel,
				   const QString& sEndpoint );

	/** Editor-side headless engine mirror serving reads; not owned. */
	Hydrogen* m_pMirror;
	/** Control channel to the authoritative engine; owned. */
	IpcChannel* m_pChannel;
	/** Applies inbound (engine → editor) frames onto the mirror; owned. */
	std::unique_ptr<EditorStateMirror> m_pStateMirror;
};

}

#endif
