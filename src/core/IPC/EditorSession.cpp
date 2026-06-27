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

#include <core/IPC/EditorSession.h>

#include <core/IPC/EditorStateMirror.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcMessage.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {

void EditorSession::configureMirrorPreferences(
	std::shared_ptr<Preferences> pPreferences ) {
	if ( pPreferences == nullptr ) {
		return;
	}
	// Passive, threadless audio driver: the mirror reflects state the host engine
	// pushes over IPC and must not run an audio process loop. The Fake driver
	// spawns a processing thread (FakeAudioDriver::connect) whose late logging
	// raced — and crashed against — the Logger teardown on editor abort.
	pPreferences->m_audioDriver = Preferences::AudioDriver::Null;
	pPreferences->m_midiDriver = Preferences::MidiDriver::None;
	pPreferences->setOscServerEnabled( false );
}

EditorSession::EditorSession( Hydrogen* pMirror, IpcChannel* pChannel )
	: m_pMirror( pMirror )
	, m_pChannel( pChannel )
	, m_pStateMirror( std::make_unique<EditorStateMirror>( pMirror ) ) {
	m_pStateMirror->attach( pChannel );
}

EditorSession::~EditorSession() {
	// Tear down the inbound sync before the channel so no late frame is applied
	// to a mirror the GUI may already be destroying.
	m_pStateMirror.reset();
	delete m_pChannel;
}

std::unique_ptr<EditorSession> EditorSession::connect(
	const QString& sEndpoint, Hydrogen* pMirror, int nTimeoutMs ) {
	if ( pMirror == nullptr || sEndpoint.isEmpty() ) {
		return nullptr;
	}
	IpcChannel* pChannel = IpcChannel::connectToServer( sEndpoint, nTimeoutMs );
	if ( pChannel == nullptr ) {
		return nullptr;
	}
	// Attach inbound sync first, then announce ourselves: the engine's hello
	// reply and its initial song snapshot then flow straight onto the mirror.
	std::unique_ptr<EditorSession> pSession(
		new EditorSession( pMirror, pChannel ) );
	pChannel->send( IpcMessage::hello() );
	return pSession;
}

bool EditorSession::isConnected() const {
	return m_pChannel != nullptr && m_pChannel->isConnected();
}

std::unique_ptr<IpcEngineAccess> EditorSession::createEngineAccess() const {
	return std::make_unique<IpcEngineAccess>( m_pMirror, m_pChannel );
}

}
