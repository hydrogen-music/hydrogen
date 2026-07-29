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

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>

#include <QtCore/QTimer>

#include <cmath>
#include <cstdlib>

namespace H2Core {

namespace {
/** Transport-affecting events whose arrival should trigger an immediate
 * telemetry-based correction (the others only need the GUI repaint that the
 * re-posted event already drives). */
bool isTransportEvent( Event::Type type ) {
	return type == Event::Type::State ||
		   type == Event::Type::Relocation ||
		   type == Event::Type::TempoChanged ||
		   type == Event::Type::BbtChanged;
}
}

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
		// A transport change on the engine: correct the mirror immediately from
		// the freshly-written telemetry (low latency), on top of the periodic
		// timer. No-op when no telemetry block is attached (events-only fallback).
		if ( isTransportEvent( type ) ) {
			syncTransportFromTelemetry();
		}
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

void EditorStateMirror::attachTelemetry( const QString& sEndpoint ) {
	if ( ! m_telemetryShm.attach(
			 EngineTelemetryShm::keyForEndpoint( sEndpoint ) ) ) {
		// No block (or version mismatch): the mirror still follows play/stop via
		// the event-triggered path; it just gets no frame/drift correction.
		return;
	}

	// Periodic forced re-sync (~5 s) bounds long-run drift between the mirror's
	// own free-running clock and the (remote) headless engine (ADR 0031).
	// Event-driven syncs handle the immediate cases (play/stop/seek) between
	// ticks.
	m_pResyncTimer = new QTimer( this );
	m_pResyncTimer->setInterval( EditorStateMirror::nResyncTimeoutMs );
	connect( m_pResyncTimer, &QTimer::timeout,
			 this, &EditorStateMirror::syncTransportFromTelemetry );
	m_pResyncTimer->start();
}

void EditorStateMirror::syncTransportFromTelemetry() {
	EngineTelemetrySnapshot snapshot;
	if ( ! m_telemetryShm.load( snapshot ) ) {
		return; // not attached / version mismatch
	}
	m_telemetry = snapshot;
	applyTransportSnapshot( snapshot );
}

void EditorStateMirror::forceTransportSync() {
	syncTransportFromTelemetry();
}

void EditorStateMirror::applyTransportSnapshot(
	const EngineTelemetrySnapshot& snapshot ) {
	if ( m_pMirror == nullptr ) {
		return;
	}
	auto pAudioEngine = m_pMirror->getAudioEngine();
	if ( pAudioEngine == nullptr ) {
		return;
	}

	const bool bRemoteEnginePlaying = snapshot.playing != 0;
	const bool bMirrorPlaying =
		pAudioEngine->getState() == AudioEngine::State::Playing;

	// 1. Play/stop follow. The headless engine is authoritative; the mirror
	//    never initiates transport (ADR 0026), it only matches the headless
	//    engine's rolling state so its own clock advances (or holds) the local
	//    playhead.
	if ( bRemoteEnginePlaying && ! bMirrorPlaying ) {
		m_pMirror->sequencerPlay();
	}
	else if ( ! bRemoteEnginePlaying && bMirrorPlaying ) {
		m_pMirror->sequencerStop();
	}

	// 2. Tempo follow (best-effort; a timeline on the mirror would override this).
	auto pPlayhead = pAudioEngine->getPlayhead();
	if ( snapshot.bpm > 0.0f && pPlayhead != nullptr &&
		 std::fabs( pPlayhead->getBpm() - snapshot.bpm ) > 0.01f ) {
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( snapshot.bpm );
		pAudioEngine->unlock();
	}

	// 3. Frame follow / drift correction. The telemetry frame is up to one
	//    engine write-cycle stale, so snapping it onto a *rolling* mirror would
	//    jerk the playhead backwards by that latency every sync. Only correct
	//    when the mirror is stopped (exact follow — no latency, frame is
	//    static) or when the divergence is large enough to be a real desync
	//    (headless engine seek / accumulated drift), never the routine read
	//    lag.
	if ( pPlayhead != nullptr ) {
		const long long nDrift =
			std::llabs( pPlayhead->getFrame() - snapshot.frame );
		long long nSampleRate = 48000;
		if ( pAudioEngine->getAudioDriver() != nullptr &&
			 pAudioEngine->getAudioDriver()->getSampleRate() > 0 ) {
			nSampleRate = pAudioEngine->getAudioDriver()->getSampleRate();
		}
		const long long nThreshold = nSampleRate / 2; // ~0.5 s
		if ( ( ! bRemoteEnginePlaying && nDrift > 0 ) || nDrift > nThreshold ) {
			m_pMirror->getCoreActionController()->relocateToFrame( snapshot.frame );
		}
	}
}

};
