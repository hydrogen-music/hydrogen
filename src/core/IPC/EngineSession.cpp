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
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>
#include <core/IPC/EngineTelemetry.h>
#include <core/IPC/EngineTelemetryShm.h>

#include <algorithm>
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
	m_pTelemetry = std::make_unique<EngineTelemetryShm>();
	if ( ! m_pTelemetry->create(
			 EngineTelemetryShm::keyForEndpoint( m_sEndpoint ) ) ) {
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

		// An editor attached: prime its mirror with the current selection and
		// record state, then serve the connection until it drops (or we are
		// stopped). The runtime event pipeline only carries *changes* — a
		// selection made before the editor attached would otherwise never
		// reach it. GUI editors re-pull the full state (song first) via
		// HydrogenApp::syncViaIpc; this push gives every editor an immediate
		// baseline. (An instrument number beyond the mirror's local kit clamps
		// to "no selection" until the song syncs — syncViaIpc corrects it.)
		IPCLOG( QString( "Editor connected to endpoint [%1]" )
					.arg( m_sEndpoint ) );
		// Drain anything still queued first (bounded by the poll window since
		// the last discardEvents), so the priming below supersedes stale
		// events — e.g. one queued before a Suppress-writer clamped the
		// stored selection without pushing a new event.
		forwardEvents( pConn );
		// Plain unsynchronized reads on this bridge thread, like the telemetry
		// snapshot: word-sized values, benign tearing-wise.
		pConn->send( IpcMessage::fromEvent(
			Event::Type::SelectedPatternChanged,
			m_pEngine->getSelectedPatternNumber(), 0 ) );
		pConn->send( IpcMessage::fromEvent(
			Event::Type::SelectedInstrumentChanged,
			m_pEngine->getSelectedInstrumentNumber(), 0 ) );
		pConn->send( IpcMessage::fromEvent(
			Event::Type::RecordModeChanged,
			static_cast<int>( m_pEngine->getRecordEnabled() ), 0 ) );
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
		IPCLOG( "Editor disconnected" );
		delete pConn;
	}
}

void EngineSession::handleMessage( IpcChannel* pConn, const IpcMessage& msg ) {
	if ( pConn == nullptr ) {
		return;
	}
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

EngineTelemetrySnapshot EngineSession::buildTelemetrySnapshot( Hydrogen* pEngine ) {
	EngineTelemetrySnapshot snapshot;
	if ( pEngine == nullptr ) {
		return snapshot;
	}
	auto pAudioEngine = pEngine->getAudioEngine();
	if ( pAudioEngine == nullptr ) {
		return snapshot;
	}

	// The peaks are read consume-style (read + reset, ADR 0027): in the
	// headless engine process no GUI consumes them, so a plain read would
	// latch at the running maximum. The consume is a read-modify-write that
	// must not race the audio thread's max-hold, so take the engine lock with
	// a small budget (the audio thread itself only try-locks, so this can not
	// starve it). On contention the snapshot degrades to the transport-only
	// fields — meters blip for one cycle, which the lossy-tolerant telemetry
	// classification (ADR 0018) accepts.
	const bool bLocked = pAudioEngine->tryLockFor(
		std::chrono::microseconds( 2000 ), RIGHT_HERE );

	auto pPlayhead = pAudioEngine->getPlayhead();
	if ( pPlayhead != nullptr ) {
		snapshot.frame = pPlayhead->getFrame();
		snapshot.tick = static_cast<int32_t>( pPlayhead->getTick() );
		snapshot.bar = static_cast<int32_t>( pPlayhead->getBar() );
		snapshot.beat = static_cast<int32_t>( pPlayhead->getBeat() );
		snapshot.bpm = pPlayhead->getBpm();
	}
	snapshot.playing =
		( pAudioEngine->getState() == AudioEngine::State::Playing ) ? 1 : 0;

	// Process time of the authoritative engine (plain reads; advisory).
	snapshot.procTimeCur = pAudioEngine->getProcessTime();
	snapshot.procTimeMax = pAudioEngine->getMaxProcessTime();

	if ( bLocked ) {
		pAudioEngine->consumeMasterPeaks( snapshot.masterPeakL,
										  snapshot.masterPeakR );

		auto pSong = pEngine->getSong();
		if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
			// Per-instrument peaks in drumkit order — the same order the
			// editor's mirror applies them in. Instruments beyond the fixed
			// cap get no meter (ADR 0018).
			const auto pInstruments = pSong->getDrumkit()->getInstruments();
			const int nCount = std::min( static_cast<int>( pInstruments->size() ),
										 ENGINE_TELEMETRY_MAX_INSTRUMENTS );
			for ( int ii = 0; ii < nCount; ++ii ) {
				auto pInstrument = pInstruments->get( ii );
				if ( pInstrument == nullptr ) {
					continue;
				}
				pInstrument->consumePeaks( snapshot.peakL[ ii ],
										   snapshot.peakR[ ii ] );
			}
			snapshot.instPeakCount = static_cast<uint16_t>( nCount );

			// Playback-track peaks, consumed for the same latch reason as
			// above: without a GUI consumer in the headless process a plain
			// read would never fall.
			auto pPlaybackTrack = pSong->getPlaybackTrackInstrument();
			if ( pPlaybackTrack != nullptr ) {
				pPlaybackTrack->consumePeaks( snapshot.playbackTrackPeakL,
											  snapshot.playbackTrackPeakR );
			}
		}

		pAudioEngine->unlock();
	}

	return snapshot;
}

void EngineSession::publishTelemetry() {
	if ( m_pTelemetry == nullptr ) {
		return;
	}
	// Published from the bridge thread at the serve-loop cadence (~50 ms), not
	// from the audio thread per buffer (amendment to ADR 0018): 20 Hz covers
	// the editor's meter refresh while keeping the audio thread 100% IPC-free.
	// The seqlock store keeps the editor's copy tear-free; values the audio
	// thread races on (playhead, peaks) are advisory and lossy-tolerant.
	m_pTelemetry->store( buildTelemetrySnapshot( m_pEngine ) );
}

}
