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
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>

#include <QtCore/QTimer>

#include <algorithm>
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
	applyEvent( msg );
}

bool EditorStateMirror::applyEvent( const IpcMessage& msg ) {
	if ( m_pMirror == nullptr ) {
		return false;
	}

	// A MIDI note recorded by the engine: re-queue it on the mirror's
	// EventQueue so the editor's HydrogenApp::onEventQueueTimer integrates
	// it as an undoable pattern edit — exactly as in standalone, where the
	// GUI drains the local engine's vector (ADR 0030 batch 2n).
	if ( msg.getOpcode() == IpcOpcode::MidiNoteRecorded ) {
		auto pQueue = m_pMirror->getEventQueue();
		if ( pQueue == nullptr ) {
			return false;
		}
		EventQueue::AddMidiNoteVector noteAction;
		if ( ! msg.toMidiNoteFields( noteAction ) ) {
			return false;
		}
		pQueue->pushMidiNoteAction( noteAction );
		return true;
	}

	if ( msg.getOpcode() != IpcOpcode::Event ) {
		return false;
	}

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

	// Selection and record state ride on the event payload (the engine-side
	// producers push their stored values). Applying them here keeps the
	// mirror's own state — and with it the object-flavor reads of
	// IEngineAccess — consistent with the engine without a blocking query.
	// Suppress: the forwarded event is already queued above; applying must
	// not push a second, local-origin event.
	switch ( type ) {
	case Event::Type::SelectedInstrumentChanged:
		m_pMirror->setSelectedInstrumentNumber(
			nValue, Event::Trigger::Suppress );
		break;
	case Event::Type::SelectedPatternChanged:
		m_pMirror->setSelectedPatternNumber(
			nValue, true, Event::Trigger::Suppress );
		break;
	case Event::Type::RecordModeChanged:
		m_pMirror->setRecordEnabled( nValue != 0 );
		break;
	case Event::Type::BeatCounter:
		// The engine's beat-counter pushes carry their event count as
		// the value: applying it keeps the mirror's "n/total" display in
		// step without a blocking query (ADR 0026 point 12).
		m_pMirror->setBeatCounterEventCount( nValue );
		break;
	default:
		break;
	}

	// Tag as Headless so the editor's onEventQueueTimer() can distinguish
	// remote-origin events from local mirror events.
	pQueue->pushEvent( type, nValue, nId, H2Core::ProcessMode::Headless );

	// A transport change on the engine: correct the mirror immediately from
	// the freshly-written telemetry (low latency), on top of the periodic
	// timer. No-op when no telemetry block is attached (events-only fallback).
	if ( isTransportEvent( type ) ) {
		syncTransportFromTelemetry();
	}
	return true;
}

void EditorStateMirror::attachTelemetry( const QString& sEndpoint ) {
	m_sTelemetryEndpoint = sEndpoint;

	// Periodic forced re-sync (~5 s) bounds long-run drift between the mirror's
	// own free-running clock and the (remote) headless engine (ADR 0031).
	// Event-driven syncs handle the immediate cases (play/stop/seek) between
	// ticks. The timer runs even without an attached block: every sync
	// retries the attach (see tryAttachTelemetry), so a block appearing late
	// still activates telemetry.
	m_pResyncTimer = new QTimer( this );
	m_pResyncTimer->setInterval( EditorStateMirror::nResyncTimeoutMs );
	connect( m_pResyncTimer, &QTimer::timeout,
			 this, &EditorStateMirror::syncTransportFromTelemetry );
	m_pResyncTimer->start();

	tryAttachTelemetry();
}

void EditorStateMirror::tryAttachTelemetry() {
	if ( m_telemetryShm.isValid() || m_sTelemetryEndpoint.isEmpty() ) {
		return;
	}
	if ( ! m_telemetryShm.attach(
			 EngineTelemetryShm::keyForEndpoint( m_sTelemetryEndpoint ) ) ) {
		// No block yet (or a version mismatch): the mirror still follows
		// play/stop via the event-triggered path; it just gets no
		// frame/drift correction and no meters until a retry succeeds.
		return;
	}

	// Meter sync at the GUI's meter cadence (ADR 0018): peaks and process
	// time are lossy-tolerant "latest value wins" data, unlike the ordered
	// event stream on the socket.
	m_pMeterTimer = new QTimer( this );
	m_pMeterTimer->setInterval( EditorStateMirror::nMeterSyncTimeoutMs );
	connect( m_pMeterTimer, &QTimer::timeout,
			 this, &EditorStateMirror::syncMetersFromTelemetry );
	m_pMeterTimer->start();
}

void EditorStateMirror::syncTransportFromTelemetry() {
	if ( ! m_telemetryShm.isValid() ) {
		// The initial attach may have lost the race against the engine's
		// serve() startup (the endpoint is reported before the block is
		// created); retry rather than staying events-only for the whole
		// session.
		tryAttachTelemetry();
		if ( ! m_telemetryShm.isValid() ) {
			return;
		}
	}
	EngineTelemetrySnapshot snapshot;
	if ( ! m_telemetryShm.load( snapshot ) ) {
		return; // version mismatch
	}
	m_telemetry = snapshot;
	applyTransportSnapshot( snapshot );
}

void EditorStateMirror::syncMetersFromTelemetry() {
	EngineTelemetrySnapshot snapshot;
	if ( ! m_telemetryShm.load( snapshot ) ) {
		return; // not attached / version mismatch
	}
	m_telemetry = snapshot;
	applyMeterSnapshot( snapshot );
}

void EditorStateMirror::forceTransportSync() {
	syncTransportFromTelemetry();
}

void EditorStateMirror::applyMeterSnapshot(
	const EngineTelemetrySnapshot& snapshot ) {
	if ( m_pMirror == nullptr ) {
		return;
	}
	auto pAudioEngine = m_pMirror->getAudioEngine();
	if ( pAudioEngine == nullptr ) {
		return;
	}

	// Max-merge: the engine consumed its accumulators when publishing, so a
	// smaller follow-up value would erase a blip the GUI has not consumed
	// yet. The reset stays with the GUI's consume calls (ADR 0027). In editor
	// mode these members have no other writer — the mirror's render path is
	// gated off in AudioEngine::audioEngine_process() — so this GUI-thread
	// write is race-free.
	pAudioEngine->setMasterPeak_L(
		std::max( pAudioEngine->getMasterPeak_L(), snapshot.masterPeakL ) );
	pAudioEngine->setMasterPeak_R(
		std::max( pAudioEngine->getMasterPeak_R(), snapshot.masterPeakR ) );

	auto pSong = m_pMirror->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	// Per-instrument peaks in drumkit order — the same order the engine
	// filled them in. The mirror's song tracks the engine's via the SetSong
	// snapshots; a transient index mismatch while edits are in flight only
	// blips a meter (lossy-tolerant, ADR 0018).
	const auto pInstruments = pSong->getDrumkit()->getInstruments();
	const int nCount = std::min( {
		static_cast<int>( snapshot.instPeakCount ),
		static_cast<int>( pInstruments->size() ),
		ENGINE_TELEMETRY_MAX_INSTRUMENTS } );
	for ( int ii = 0; ii < nCount; ++ii ) {
		auto pInstrument = pInstruments->get( ii );
		if ( pInstrument == nullptr ) {
			continue;
		}
		pInstrument->setPeak_L(
			std::max( pInstrument->getPeak_L(), snapshot.peakL[ ii ] ) );
		pInstrument->setPeak_R(
			std::max( pInstrument->getPeak_R(), snapshot.peakR[ ii ] ) );
	}

	// Playback-track peaks, same max-merge.
	auto pPlaybackTrack = pSong->getPlaybackTrackInstrument();
	if ( pPlaybackTrack != nullptr ) {
		pPlaybackTrack->setPeak_L(
			std::max( pPlaybackTrack->getPeak_L(),
					  snapshot.playbackTrackPeakL ) );
		pPlaybackTrack->setPeak_R(
			std::max( pPlaybackTrack->getPeak_R(),
					  snapshot.playbackTrackPeakR ) );
	}
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
