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

#include <core/IPC/EngineSession.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>
#include <core/IPC/PluginTelemetry.h>
#include <core/IPC/PluginTelemetryShm.h>

#include <chrono>

#include <QtCore/QThread>

namespace H2Core {

EngineSession::EngineSession( Hydrogen* pEngine, const QString& sEndpoint )
	: m_pEngine( pEngine )
	, m_sEndpoint( sEndpoint )
	, m_bRunning( true )
	, m_pThread( nullptr )
	, m_pTelemetry( nullptr ) {
}

EngineSession::~EngineSession() {
	stop();
	delete m_pThread;
}

std::unique_ptr<EngineSession> EngineSession::start(
	Hydrogen* pEngine, const QString& sEndpoint, int nListenTimeoutMs ) {
	if ( pEngine == nullptr || sEndpoint.isEmpty() ) {
		return nullptr;
	}

	std::unique_ptr<EngineSession> pSession(
		new EngineSession( pEngine, sEndpoint ) );

	// The serve loop reports the listen outcome through this promise so start()
	// can fail fast (and synchronously) if the endpoint can't be bound.
	auto pListenResult = std::make_shared<std::promise<bool>>();
	auto listenFuture = pListenResult->get_future();

	EngineSession* pRaw = pSession.get();
	pSession->m_pThread = QThread::create(
		[pRaw, pListenResult]() { pRaw->serve( pListenResult ); } );
	pSession->m_pThread->start();

	if ( listenFuture.wait_for( std::chrono::milliseconds( nListenTimeoutMs ) ) !=
			 std::future_status::ready ||
		 ! listenFuture.get() ) {
		pSession->stop();
		return nullptr;
	}

	return pSession;
}

void EngineSession::stop() {
	m_bRunning.store( false );
	if ( m_pThread != nullptr ) {
		m_pThread->wait();
	}
}

void EngineSession::serve( std::shared_ptr<std::promise<bool>> pListenResult ) {
	// IpcServer + accepted channel are created and used only on this thread - a
	// QLocalSocket/Server is thread-affine (its QSocketNotifier may only be pumped
	// on the Qt-managed thread that owns it).
	IpcServer server;
	const bool bListening = server.listen( m_sEndpoint );
	pListenResult->set_value( bListening );
	if ( ! bListening ) {
		return;
	}

	// Publish a telemetry block keyed off the endpoint so an editor that knows
	// only `--connect-via-ipc <endpoint>` can attach and follow the headless
	// engine's playhead (ADR 0018/0031). Lives on this bridge thread; failure
	// is non-fatal (the editor then falls back to events-only sync).
	m_pTelemetry = std::make_unique<PluginTelemetryShm>();
	if ( ! m_pTelemetry->create(
			 PluginTelemetryShm::keyForEndpoint( m_sEndpoint ) ) ) {
		m_pTelemetry.reset();
	}

	while ( m_bRunning.load() ) {
		IpcChannel* pConn = server.waitForChannel( m_nPollTimeoutMs );
		if ( pConn == nullptr ) {
			// No editor attached: keep the EventQueue from overflowing.
			discardEvents();
			publishTelemetry();
			continue;
		}

		// An editor attached: prime its mirror with the current song, then serve
		// the connection until it drops (or we are stopped).
		___INFOLOG( QString( "Editor connected to endpoint [%1]" )
					.arg( m_sEndpoint ) );
		sendInitialState( pConn );
		while ( m_bRunning.load() && pConn->isConnected() ) {
			IpcMessage msg;
			if ( pConn->receive( msg, m_nPollTimeoutMs, false ) ) {
				handleMessage( pConn, msg );
			}
			forwardEvents( pConn );
			publishTelemetry();
		}

		// The editor went away; drop our end and loop back to accept a respawn
		// (the engine survives editor disconnect/crash, ADR 0016).
		___INFOLOG( "Editor disconnected" );
		delete pConn;
	}
}

void EngineSession::handleMessage( IpcChannel* pConn, const IpcMessage& msg ) {
	if ( msg.getOpcode() == IpcOpcode::Hello ) {
		pConn->send( IpcMessage::hello() );
	}
	else if ( msg.getRequestId() != 0 ) {
		// Request/response tier (ADR 0030): answer with a correlated Reply.
		pConn->send( IpcEngineBridge::handleRequest( msg, m_pEngine ) );
	}
	else {
		// Fire-and-forget command.
		IpcEngineBridge::dispatchCommand( msg, m_pEngine );
	}
}

void EngineSession::forwardEvents( IpcChannel* pConn ) {
	if ( m_pEngine == nullptr ) {
		return;
	}
	EventQueue* pQueue = m_pEngine->getEventQueue();
	if ( pQueue == nullptr ) {
		return;
	}
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pQueue->popEvent() ) != nullptr ) {
		// forwardEvent() drops editor-internal events itself.
		IpcEngineBridge::forwardEvent( *pConn, pEvent->getType(),
									   pEvent->getValue(), pEvent->getId() );
	}
}

void EngineSession::discardEvents() {
	if ( m_pEngine == nullptr ) {
		return;
	}
	EventQueue* pQueue = m_pEngine->getEventQueue();
	if ( pQueue == nullptr ) {
		return;
	}
	while ( pQueue->popEvent() != nullptr ) {
		// no editor to forward to; just keep the queue drained
	}
}

void EngineSession::sendInitialState( IpcChannel* pConn ) {
	if ( m_pEngine == nullptr || m_pEngine->getSong() == nullptr ) {
		return;
	}
	IpcMessage msg( IpcOpcode::SetSong );
	msg.setPayload( m_pEngine->getSong()->toXmlBuffer() );
	pConn->send( msg );
}

PluginTelemetrySnapshot EngineSession::buildTransportSnapshot( Hydrogen* pEngine ) {
	PluginTelemetrySnapshot snapshot;
	if ( pEngine == nullptr ) {
		return snapshot;
	}
	auto pAudioEngine = pEngine->getAudioEngine();
	if ( pAudioEngine == nullptr ) {
		return snapshot;
	}
	auto pPlayhead = pAudioEngine->getPlayhead();
	if ( pPlayhead != nullptr ) {
		snapshot.frame = pPlayhead->getFrame();
		snapshot.tick = static_cast<int32_t>( pPlayhead->getTick() );
		snapshot.bpm = pPlayhead->getBpm();
	}
	snapshot.playing =
		( pAudioEngine->getState() == AudioEngine::State::Playing ) ? 1 : 0;
	return snapshot;
}

void EngineSession::publishTelemetry() {
	if ( m_pTelemetry == nullptr ) {
		return;
	}
	// Read off the bridge thread without the engine lock: the values are advisory
	// (the editor only uses them for a coarse ~5 s drift correction) and the
	// seqlock store keeps the reader's copy tear-free.
	m_pTelemetry->store( buildTransportSnapshot( m_pEngine ) );
}

}
