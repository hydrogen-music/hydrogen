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

#include <core/IPC/EditorStateMirror.h>

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>

namespace H2Core {

EditorStateMirror::EditorStateMirror( Hydrogen* pMirror, QObject* pParent )
	: QObject( pParent )
	, m_pMirror( pMirror ) {
}

EditorStateMirror::~EditorStateMirror() = default;

void EditorStateMirror::attach( IpcChannel* pChannel ) {
	if ( pChannel == nullptr ) {
		return;
	}
	connect( pChannel, &IpcChannel::messageReceived,
			 this, &EditorStateMirror::onMessageReceived );
}

void EditorStateMirror::onMessageReceived( const IpcMessage& msg ) {
	applyMessage( msg );
}

bool EditorStateMirror::applyMessage( const IpcMessage& msg ) {
	if ( m_pMirror == nullptr ) {
		return false;
	}

	switch ( msg.getOpcode() ) {
	case IpcOpcode::Event: {
		// Re-post the engine event onto the mirror's queue so the GUI reacts
		// exactly as with a local engine. The id is regenerated locally.
		Event::Type type;
		int nValue = 0;
		long nId = 0;
		if ( ! msg.toEventFields( type, nValue, nId ) ) {
			return false;
		}
		EventQueue* pQueue = m_pMirror->getEventQueue();
		if ( pQueue == nullptr ) {
			return false;
		}
		pQueue->pushEvent( type, nValue );
		return true;
	}

	case IpcOpcode::SetSong:
	case IpcOpcode::LoadState: {
		auto pSong = Song::fromXmlBuffer( msg.getPayload(), QString(),
										  true /*bSilent*/, m_pMirror );
		if ( pSong == nullptr ) {
			return false;
		}
		m_pMirror->setSong( pSong );
		return true;
	}

	default:
		return false;
	}
}

};
