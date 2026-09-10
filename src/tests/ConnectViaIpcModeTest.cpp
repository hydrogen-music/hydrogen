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

#include "ConnectViaIpcModeTest.h"

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IO/AudioDriverInfo.h>
#include <core/IO/SoftwareDriver.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EditorStateMirror.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>
#include <core/IPC/EngineTelemetry.h>
#include <core/IPC/EngineTelemetryShm.h>
#include <core/LocalEngineAccess.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QCoreApplication>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <thread>

using namespace H2Core;

// --connect-via-ipc <endpoint>: the editor attaches to the engine's control socket
// and announces itself with a hello (ADR 0016/0018).
void ConnectViaIpcModeTest::testAttachesToEngineEndpoint() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( pSession->isConnected() );

	// Engine side accepts the editor and sees its hello.
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );
	IpcMessage hello;
	CPPUNIT_ASSERT( conn->receive( hello ) );
	CPPUNIT_ASSERT( hello.getOpcode() == IpcOpcode::Hello );
	CPPUNIT_ASSERT_EQUAL( IPC_PROTOCOL_VERSION, hello.helloProtocolVersion() );

	pSession.reset();
	delete pMirror;

	___INFOLOG( "passed" );
}

// A bad endpoint fails gracefully (nullptr), so main() can abort with a message
// instead of building a half-wired GUI.
void ConnectViaIpcModeTest::testFailedConnectionReported() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	// No server listening on this name → connection must fail fast.
	auto pSession = EditorSession::connect(
		TestHelper::uniqueEndpoint(), pMirror, 200 /*ms*/ );
	CPPUNIT_ASSERT( pSession == nullptr );

	delete pMirror;

	___INFOLOG( "passed" );
}

// The editor mirror uses the HEADLESS software driver (ADR 0031): it clocks the
// engine — so the local transport / playing-pattern display can advance — but
// produces no audio (producesAudio == false). This replaces both the old Fake
// driver (real scratch output) and the inert Null driver (no clock → frozen
// transport). Crash-safety on the --connect-via-ipc abort no longer relies on
// avoiding a thread: the driver's clock thread is joined in its destructor and
// the engine is torn down before the Logger.
void ConnectViaIpcModeTest::testMirrorUsesHeadlessDriver() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	auto pDriver = pMirror->getAudioDriver();
	CPPUNIT_ASSERT( pDriver != nullptr );
	auto pSoftware = std::dynamic_pointer_cast<SoftwareDriver>( pDriver );
	CPPUNIT_ASSERT( pSoftware != nullptr );
	// Headless: clocks the engine but feeds no real audio sink.
	CPPUNIT_ASSERT( ! pSoftware->getProducesAudio() );
	// A real clock with valid parameters, unlike the old inert Null driver
	// (which returned 0 / 0 / nullptr).
	CPPUNIT_ASSERT( pDriver->getSampleRate() > 0 );
	CPPUNIT_ASSERT( pDriver->getBufferSize() > 0 );
	CPPUNIT_ASSERT( pDriver->getOut_L() != nullptr );

	delete pMirror;

	___INFOLOG( "passed" );
}

// The GUI's engine-access handle forwards commands over the channel to the
// engine (here, the server end).
void ConnectViaIpcModeTest::testIssuesCommands() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );
	IpcMessage hello;
	CPPUNIT_ASSERT( conn->receive( hello ) ); // consume the handshake first

	auto pAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pAccess != nullptr );
	pAccess->sequencerStop();

	IpcMessage cmd;
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::Stop );

	pAccess.reset();
	pSession.reset();
	delete pMirror;

	___INFOLOG( "passed" );
}

// The engine keeps running when the editor disconnects/crashes: tearing down the
// session closes only the editor's end; the engine's server + connection survive.
void ConnectViaIpcModeTest::testEngineSurvivesEditorDisconnect() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	// Editor goes away (process exit / crash) → its session is destroyed.
	pSession.reset();
	delete pMirror;

	// The engine side is untouched: the server still listens and can hand out a
	// fresh connection to a reattaching editor.
	CPPUNIT_ASSERT( ! server.serverName().isEmpty() );
	auto* pMirror2 = TestHelper::makeMirror();
	auto pSession2 = EditorSession::connect( server.serverName(), pMirror2 );
	CPPUNIT_ASSERT( pSession2 != nullptr );
	IpcChannel* conn2 = server.waitForChannel();
	CPPUNIT_ASSERT( conn2 != nullptr );

	pSession2.reset();
	delete pMirror2;

	___INFOLOG( "passed" );
}

// Engine side (ADR 0031 T8.4): the transport telemetry snapshot reflects the live
// engine, and the block key is derived deterministically from the endpoint so the
// editor can attach knowing only the endpoint it connected to. A stopped engine
// reports playing == 0 with a valid (> 0) BPM; the snapshot round-trips through the
// shared block.
void ConnectViaIpcModeTest::testEngineBuildsTransportSnapshot() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeMirror(); // a headless engine stands in for the host
	auto snapshot = EngineSession::buildTelemetrySnapshot( pEngine );
	CPPUNIT_ASSERT( snapshot.playing == 0 );
	CPPUNIT_ASSERT( snapshot.bpm > 0.0f );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	const QString sKey = EngineTelemetryShm::keyForEndpoint( sEndpoint );
	// Same endpoint → same key on both sides (no separate negotiation).
	CPPUNIT_ASSERT_EQUAL(
		sKey.toStdString(),
		EngineTelemetryShm::keyForEndpoint( sEndpoint ).toStdString() );

	EngineTelemetryShm writer;
	CPPUNIT_ASSERT( writer.create( sKey ) );
	CPPUNIT_ASSERT( writer.store( snapshot ) );

	EngineTelemetryShm reader;
	CPPUNIT_ASSERT( reader.attach( sKey ) );
	EngineTelemetrySnapshot out;
	CPPUNIT_ASSERT( reader.load( out ) );
	CPPUNIT_ASSERT_EQUAL( snapshot.frame, out.frame );
	CPPUNIT_ASSERT( snapshot.playing == out.playing );

	delete pEngine;

	___INFOLOG( "passed" );
}

// Editor side (ADR 0031 T8.4): the mirror follows the host transport carried by a
// telemetry snapshot — it starts/stops its own clock to match the host's rolling
// state, adopts the host tempo, and snaps to the host frame (exactly when stopped;
// on a large divergence while rolling). The mirror never initiates transport
// itself (ADR 0026), it only follows.
void ConnectViaIpcModeTest::testMirrorFollowsTransportTelemetry() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	EditorStateMirror mirror( pMirror );
	auto pAudioEngine = pMirror->getAudioEngine();

	// --- Play + tempo follow: host starts rolling at a new tempo. ---
	EngineTelemetrySnapshot rolling;
	rolling.playing = 1;
	rolling.frame = 40000;
	rolling.bpm = 140.0f;
	mirror.applyTransportSnapshot( rolling );

	// The play transition and tempo adoption land on the next clock cycle.
	bool bPlaying = false;
	for ( int ii = 0; ii < 200 && ! bPlaying; ++ii ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		bPlaying = pAudioEngine->getState() == AudioEngine::State::Playing;
	}
	CPPUNIT_ASSERT( bPlaying );

	bool bTempo = false;
	for ( int ii = 0; ii < 200 && ! bTempo; ++ii ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		bTempo = std::fabs( pAudioEngine->getPlayhead()->getBpm() - 140.0f ) < 0.5f;
	}
	CPPUNIT_ASSERT( bTempo );

	// --- Stop follow: host stops. ---
	EngineTelemetrySnapshot stopped;
	stopped.playing = 0;
	stopped.frame = 0;
	stopped.bpm = 140.0f;
	mirror.applyTransportSnapshot( stopped );

	bool bStopped = false;
	for ( int ii = 0; ii < 200 && ! bStopped; ++ii ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		bStopped = pAudioEngine->getState() != AudioEngine::State::Playing;
	}
	CPPUNIT_ASSERT( bStopped );

	// --- Frame follow (while stopped): the mirror relocates to the host frame. We
	// compare against a direct follow-relocate to the same frame rather than the
	// literal value, so the assertion is robust to the empty test song clamping /
	// wrapping a position (a real mirror shares the host song, so frames are valid).
	const long long nTarget = 40000;
	pMirror->getCoreActionController()->relocateToFrame( nTarget );
	const long long nExpected = pAudioEngine->getPlayhead()->getFrame();
	pMirror->getCoreActionController()->relocateToFrame( 0 ); // move away

	// Drain pending events, then apply the target through the telemetry path.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}
	EngineTelemetrySnapshot atTarget;
	atTarget.playing = 0;
	atTarget.frame = nTarget;
	atTarget.bpm = 140.0f;
	mirror.applyTransportSnapshot( atTarget );

	// It relocated (a Relocation event was queued) and landed on the canonical
	// target frame; a stopped clock does not advance it, so this is stable.
	bool bRelocated = false;
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		if ( pEvent->getType() == Event::Type::Relocation ) {
			bRelocated = true;
		}
	}
	CPPUNIT_ASSERT( bRelocated );
	CPPUNIT_ASSERT_EQUAL( nExpected, pAudioEngine->getPlayhead()->getFrame() );

	delete pMirror;

	___INFOLOG( "passed" );
}

// ADR 0018 metering, engine side: the snapshot carries the full telemetry
// payload, not just transport — master / per-instrument / playback-track
// peaks, process time, and BBT. The peaks are read consume-style (read +
// reset, ADR 0027): in the headless engine process no GUI consumes them, so a
// plain read would latch at the running maximum (the pre-existing
// playback-track bug).
void ConnectViaIpcModeTest::testEngineBuildsFullSnapshot() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeMirror(); // a headless engine stands in for the host
	auto pAudioEngine = pEngine->getAudioEngine();
	auto pSong = pEngine->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pSong->getDrumkit() != nullptr );
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments->size() > 0 );
	auto pInstr0 = pInstruments->get( 0 );
	CPPUNIT_ASSERT( pInstr0 != nullptr );

	// The playback-track instrument is created lazily; stand in for
	// loadPlaybackTrack().
	auto pPlaybackTrack = pSong->getPlaybackTrackInstrument();
	if ( pPlaybackTrack == nullptr ) {
		pPlaybackTrack = std::make_shared<Instrument>( Instrument::PlaybackTrackId );
		pSong->setPlaybackTrackInstrument( pPlaybackTrack );
	}

	// Move off frame 0 so the BBT fields are non-trivial, then prime the
	// engine-side meter state the audio thread would normally write.
	pEngine->getCoreActionController()->relocateToFrame( 40000 );
	while ( pEngine->getEventQueue()->popEvent() != nullptr ) {}
	pAudioEngine->setMasterPeak_L( 0.7f );
	pAudioEngine->setMasterPeak_R( 0.5f );
	pInstr0->setPeak_L( 0.3f );
	pInstr0->setPeak_R( 0.2f );
	pPlaybackTrack->setPeak_L( 0.4f );
	pPlaybackTrack->setPeak_R( 0.35f );

	// A degraded build (lock contention with the clock thread) is a legitimate
	// runtime outcome — the editor just polls again 50ms later — so the test
	// retries instead of failing. A degraded build does not consume the primed
	// peaks (the consume reads sit inside the locked section), so no re-prime
	// is needed between attempts.
	auto snapshot = EngineSession::buildTelemetrySnapshot( pEngine );
	for ( int ii = 0; ii < 10 && snapshot.masterPeakL == 0.0f; ++ii ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		snapshot = EngineSession::buildTelemetrySnapshot( pEngine );
	}
	auto pPlayhead = pAudioEngine->getPlayhead();

	// Transport fields (carried over from the transport-only era).
	CPPUNIT_ASSERT( snapshot.playing == 0 );
	CPPUNIT_ASSERT( snapshot.bpm > 0.0f );
	CPPUNIT_ASSERT_EQUAL( pPlayhead->getFrame(),
						  static_cast<long long>( snapshot.frame ) );

	// BBT crosses as well (the editor derives it locally from the frame, but
	// the fields must not be dead).
	CPPUNIT_ASSERT_EQUAL( pPlayhead->getBar(), static_cast<int>( snapshot.bar ) );
	CPPUNIT_ASSERT_EQUAL( pPlayhead->getBeat(), static_cast<int>( snapshot.beat ) );
	CPPUNIT_ASSERT_EQUAL( static_cast<int32_t>( pPlayhead->getTick() ),
						  snapshot.tick );

	// Master peaks.
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.7, snapshot.masterPeakL, 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.5, snapshot.masterPeakR, 0.001 );

	// Per-instrument peaks, in drumkit order.
	CPPUNIT_ASSERT( snapshot.instPeakCount > 0 );
	CPPUNIT_ASSERT( static_cast<int>( snapshot.instPeakCount ) <=
					static_cast<int>( pInstruments->size() ) );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.3, snapshot.peakL[0], 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.2, snapshot.peakR[0], 0.001 );

	// Playback-track peaks.
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.4, snapshot.playbackTrackPeakL, 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.35, snapshot.playbackTrackPeakR, 0.001 );

	// Process time of the authoritative engine (plain reads).
	CPPUNIT_ASSERT( snapshot.procTimeCur >= 0.0f );
	CPPUNIT_ASSERT( snapshot.procTimeMax >= 0.0f );

	// Consume semantics: building the snapshot reset the engine-side
	// accumulators, so a headless engine (no GUI consumer) can not latch at
	// the running maximum.
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, pAudioEngine->getMasterPeak_L(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, pAudioEngine->getMasterPeak_R(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, pInstr0->getPeak_L(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, pPlaybackTrack->getPeak_L(), 0.001 );

	delete pEngine;

	___INFOLOG( "passed" );
}

// The per-instrument peak array is capped at ENGINE_TELEMETRY_MAX_INSTRUMENTS
// (ADR 0018): instruments beyond the cap get no meter and instPeakCount
// reports the actual number of valid entries.
void ConnectViaIpcModeTest::testEngineSnapshotCapsInstrumentPeaks() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeMirror();
	auto pAudioEngine = pEngine->getAudioEngine();
	auto pInstruments = pEngine->getSong()->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );

	// Grow the kit past the cap (under the engine lock — the mirror's clock
	// thread still runs the transport loop).
	pAudioEngine->lock( RIGHT_HERE );
	while ( pInstruments->size() < ENGINE_TELEMETRY_MAX_INSTRUMENTS + 4 ) {
		pInstruments->add( std::make_shared<Instrument>() );
	}
	pInstruments->get( 5 )->setPeak_L( 0.21f );
	pInstruments->get( ENGINE_TELEMETRY_MAX_INSTRUMENTS + 1 )
		->setPeak_L( 0.9f );
	pAudioEngine->unlock();

	// Retry around degraded builds (lock contention), as above. The primed
	// peaks survive a degraded attempt untouched.
	auto snapshot = EngineSession::buildTelemetrySnapshot( pEngine );
	for ( int ii = 0; ii < 10 && snapshot.instPeakCount == 0; ++ii ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		snapshot = EngineSession::buildTelemetrySnapshot( pEngine );
	}

	CPPUNIT_ASSERT_EQUAL( ENGINE_TELEMETRY_MAX_INSTRUMENTS,
						  static_cast<int>( snapshot.instPeakCount ) );
	// In-cap instrument crossed; the over-cap one is simply not represented.
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.21, snapshot.peakL[5], 0.001 );

	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0018 metering, editor side: applyMeterSnapshot() pushes the meter half
// of the telemetry onto the mirror's regular state holders — AudioEngine
// master peaks, per-instrument peaks in drumkit order, playback-track peaks —
// so the GUI's existing consumers (Mixer faders, playback-track fader) work
// unchanged. Values are max-merged so a blip between two applies can not be
// lost before the GUI consumes it, and the GUI's consumePeaks() reset
// semantics stay intact. Transport fields in the same snapshot are ignored:
// they belong to the hybrid transport sync (ADR 0031), which must never run
// at meter cadence.
void ConnectViaIpcModeTest::testMirrorAppliesMeterTelemetry() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	EditorStateMirror mirror( pMirror );
	auto pAudioEngine = pMirror->getAudioEngine();
	auto pSong = pMirror->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );
	CPPUNIT_ASSERT( pInstruments->size() >= 2 );
	auto pPlaybackTrack = pSong->getPlaybackTrackInstrument();
	if ( pPlaybackTrack == nullptr ) {
		pPlaybackTrack = std::make_shared<Instrument>( Instrument::PlaybackTrackId );
		pSong->setPlaybackTrackInstrument( pPlaybackTrack );
	}

	EngineTelemetrySnapshot snapshot;
	snapshot.playing = 1;          // must NOT be applied by the meter path
	snapshot.frame = 12345;        // ditto
	snapshot.bpm = 180.0f;         // ditto
	snapshot.masterPeakL = 0.7f;
	snapshot.masterPeakR = 0.5f;
	snapshot.instPeakCount = 2;
	snapshot.peakL[0] = 0.3f;  snapshot.peakR[0] = 0.2f;
	snapshot.peakL[1] = 0.1f;  snapshot.peakR[1] = 0.15f;
	snapshot.playbackTrackPeakL = 0.4f;
	snapshot.playbackTrackPeakR = 0.35f;

	mirror.applyMeterSnapshot( snapshot );

	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.7, pAudioEngine->getMasterPeak_L(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.5, pAudioEngine->getMasterPeak_R(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.3, pInstruments->get( 0 )->getPeak_L(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.2, pInstruments->get( 0 )->getPeak_R(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.1, pInstruments->get( 1 )->getPeak_L(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.4, pPlaybackTrack->getPeak_L(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.35, pPlaybackTrack->getPeak_R(), 0.001 );

	// Meter-only: the transport fields of the same snapshot were not applied
	// (a fresh mirror sits at frame 0, stopped).
	CPPUNIT_ASSERT( pAudioEngine->getState() != AudioEngine::State::Playing );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 0 ),
						  pAudioEngine->getPlayhead()->getFrame() );

	// Max-merge: a smaller follow-up value can not erase a blip the GUI has
	// not consumed yet.
	EngineTelemetrySnapshot lower;
	lower.instPeakCount = 1;
	lower.peakL[0] = 0.05f;
	mirror.applyMeterSnapshot( lower );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.3, pInstruments->get( 0 )->getPeak_L(), 0.001 );

	// The GUI consume (read + reset, ADR 0027) still works on top.
	float fPeakL = 0.0f, fPeakR = 0.0f;
	pInstruments->get( 0 )->consumePeaks( fPeakL, fPeakR );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.3, fPeakL, 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.0, pInstruments->get( 0 )->getPeak_L(), 0.001 );

	delete pMirror;

	___INFOLOG( "passed" );
}

// End-to-end metering (ADR 0018): peaks written engine-side travel the
// shared-memory telemetry block onto the editor mirror's state holders — the
// engine's bridge thread publishes, the mirror's meter-cadence timer (driven
// by the GUI event loop) applies — without any widget involvement.
void ConnectViaIpcModeTest::testTelemetryMetersFlowEngineToEditor() {
	___INFOLOG( "" );

	const QString sEndpoint = TestHelper::uniqueEndpoint();

	// Engine side: served by the production EngineSession serve loop.
	auto* pEngine = TestHelper::makeMirror();
	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	// serve() reports the listening endpoint before it creates the telemetry
	// block; wait for the block so the editor's attach does not lose that
	// startup race.
	EngineTelemetryShm probe;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return probe.attach( EngineTelemetryShm::keyForEndpoint( sEndpoint ) );
	} ) );
	probe.detach();

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );

	const auto pInstruments = pMirror->getSong()->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );
	CPPUNIT_ASSERT( pInstruments->size() > 0 );

	// Engine-side meters move. The bridge thread consumes each publish cycle
	// (~50 ms), so keep re-priming like a rolling audio thread would; the
	// editor's meter timer must pick the values up.
	auto pEngineInstruments =
		pEngine->getSong()->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pEngineInstruments->size() > 0 );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		pEngine->getAudioEngine()->setMasterPeak_L( 0.6f );
		pEngineInstruments->get( 0 )->setPeak_L( 0.25f );
		return pMirror->getAudioEngine()->getMasterPeak_L() > 0.5f &&
			pInstruments->get( 0 )->getPeak_L() > 0.2f;
	} ) );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Late telemetry attach: when the editor connects within the engine serve()
// startup window (listen reported before the telemetry block exists), the
// initial attach fails silently (events-only fallback). Once the block
// appears, the next forced sync must re-attach and bring the meter pipeline
// alive instead of staying events-only for the whole session.
void ConnectViaIpcModeTest::testTelemetryLateAttach() {
	___INFOLOG( "" );

	// Engine stand-in WITHOUT a telemetry block: a plain IpcServer, standing
	// for the window between listen() and block creation in serve().
	IpcServer server;
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );
	IpcMessage hello;
	CPPUNIT_ASSERT( conn->receive( hello ) ); // drain the handshake

	// The block appears late.
	EngineTelemetryShm engineBlock;
	CPPUNIT_ASSERT( engineBlock.create(
		EngineTelemetryShm::keyForEndpoint( server.serverName() ) ) );

	EngineTelemetrySnapshot snapshot;
	snapshot.masterPeakL = 0.6f;
	snapshot.instPeakCount = 1;
	snapshot.peakL[0] = 0.25f;

	auto pStateMirror = pSession->getStateMirror();
	CPPUNIT_ASSERT( pStateMirror != nullptr );
	const auto pInstruments = pMirror->getSong()->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );
	CPPUNIT_ASSERT( pInstruments->size() > 0 );

	// Forced syncs (reconnect, transport events) retry the attach; once it
	// succeeds, the meter timer applies the published peaks.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		engineBlock.store( snapshot );
		pStateMirror->forceTransportSync();
		return pMirror->getAudioEngine()->getMasterPeak_L() > 0.5f &&
			pInstruments->get( 0 )->getPeak_L() > 0.2f;
	} ) );

	pSession.reset();
	delete pMirror;

	___INFOLOG( "passed" );
}

// ADR 0018/0029: the GetAudioDriverInfo reply must carry the engine's actual
// sample rate (plus buffer size and latency, for display). The editor needs
// the rate to keep its mirror engine's frame<->tick conversion on the
// engine's clock — see testMirrorSyncsSampleRate.
void ConnectViaIpcModeTest::testAudioDriverInfoCarriesSampleRate() {
	___INFOLOG( "" );

	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto* pEngine = TestHelper::makeMirror();
	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );

	IpcMessage reply;
	CPPUNIT_ASSERT( pSession->getChannel()->request(
		IpcMessage( IpcOpcode::GetAudioDriverInfo ), reply, 3000 ) );
	const auto& args = reply.getArgs();
	CPPUNIT_ASSERT( args.size() >= 9 );

	const auto pEngineDriver = pEngine->getAudioDriver();
	CPPUNIT_ASSERT( pEngineDriver != nullptr );
	CPPUNIT_ASSERT_EQUAL(
		static_cast<int>( pEngineDriver->getSampleRate() ), args[6].toInt() );
	CPPUNIT_ASSERT_EQUAL(
		static_cast<int>( pEngineDriver->getBufferSize() ), args[7].toInt() );
	CPPUNIT_ASSERT_EQUAL(
		static_cast<int>( pEngineDriver->getLatency() ), args[8].toInt() );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The mirror engine must run at the authoritative engine's sample rate: its
// frame<->tick conversion (Transport::computeTickFromFrame via the local
// driver rate) turns every telemetry frame into a tick/BBT position, so a
// rate mismatch skews the whole transport display. The buffer size, on the
// other hand, only paces the mirror's own clock loop and deliberately stays
// local. Hydrogen::setCachedAudioDriverInfo() — fed by the GUI's
// refreshCachedAudioDriverInfo() — enforces the invariant.
void ConnectViaIpcModeTest::testMirrorSyncsSampleRate() {
	___INFOLOG( "" );

	const QString sEndpoint = TestHelper::uniqueEndpoint();

	// The editor mirror runs at its local (config-file) rate ...
	auto* pMirror = TestHelper::makeMirror();
	auto pMirrorDriver = std::dynamic_pointer_cast<SoftwareDriver>(
		pMirror->getAudioDriver() );
	CPPUNIT_ASSERT( pMirrorDriver != nullptr );
	const int nEditorRate = static_cast<int>( pMirrorDriver->getSampleRate() );
	const auto nMirrorBufferSize = pMirrorDriver->getBufferSize();

	// ... while the host imposes a different rate on the engine. Preferences
	// instances are per-engine (ADR 0015, no process singleton), so the
	// engine stand-in is built directly with a rate-override config — like
	// TestHelper::makeEngine(), but headless-clocked and at 32000.
	const int nEngineRate = 32000;
	CPPUNIT_ASSERT( nEngineRate != nEditorRate );
	auto pEnginePref = Preferences::create_instance();
	EditorSession::configureMirrorPreferences( pEnginePref );
	pEnginePref->m_nSampleRate = nEngineRate;
	auto* pEngine = new Hydrogen( pEnginePref, ProcessMode::Headless, -1 );
	pEngine->setFullyOperational( true );
	CPPUNIT_ASSERT_EQUAL( nEngineRate,
						  static_cast<int>(
							  pEngine->getAudioDriver()->getSampleRate() ) );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );

	// The mirror applies the initial song snapshot asynchronously; the
	// transport math below needs a song.
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	// Pre-sync: the mirror still runs on the editor's local rate.
	CPPUNIT_ASSERT_EQUAL( nEditorRate,
						  static_cast<int>( pMirrorDriver->getSampleRate() ) );

	// What HydrogenApp::refreshCachedAudioDriverInfo() does: fetch, parse,
	// cache. setCachedAudioDriverInfo() must re-rate the mirror's driver.
	IpcMessage reply;
	CPPUNIT_ASSERT( pSession->getChannel()->request(
		IpcMessage( IpcOpcode::GetAudioDriverInfo ), reply, 3000 ) );
	const auto& args = reply.getArgs();
	CPPUNIT_ASSERT( args.size() >= 9 );
	AudioDriverInfo info;
	info.kind = static_cast<Preferences::AudioDriver>( args[0].toInt() );
	info.isPresent = args[1].toBool();
	info.isRunning = args[2].toBool();
	info.connectedDevice = args[3].toString();
	info.timebaseState = static_cast<JackDriver::Timebase>( args[4].toInt() );
	info.jackTransportEnabled = args[5].toBool();
	info.sampleRate = args[6].toInt();
	info.bufferSize = args[7].toInt();
	info.latencyFrames = args[8].toInt();
	pMirror->setCachedAudioDriverInfo( info );

	// The mirror's clock driver now runs at the engine's rate ...
	CPPUNIT_ASSERT_EQUAL( nEngineRate,
						  static_cast<int>( pMirrorDriver->getSampleRate() ) );
	// ... the cache carries the engine's values ...
	CPPUNIT_ASSERT_EQUAL( nEngineRate,
						  pMirror->getCachedAudioDriverInfo().sampleRate );
	// ... while the buffer size stays local (it only paces the mirror's
	// own loop).
	CPPUNIT_ASSERT_EQUAL( nMirrorBufferSize, pMirrorDriver->getBufferSize() );

	// Transport math: a telemetry frame authored on the engine's clock maps
	// to the same tick the engine would compute for it. getTick() is the
	// rounded public accessor; a rate mismatch (32000 vs the editor rate)
	// would skew the tick by hundreds, far beyond the rounding tolerance.
	const long long nFrame = static_cast<long long>( nEngineRate ) * 2; // 2 s
	pMirror->getCoreActionController()->relocateToFrame( nFrame );
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}
	const double fExpectedTick =
		Transport::computeTickFromFrame( nFrame, nEngineRate, pMirror );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		fExpectedTick,
		static_cast<double>(
			pMirror->getAudioEngine()->getPlayhead()->getTick() ),
		1.0 );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// EditorStateMirror::applyEvent() used to only queue forwarded events as
// Headless-tagged GUI notifications; the mirror's own selection state was
// never touched. That left the two IEngineAccess read flavors inconsistent:
// getSelectedInstrumentNumber() asked the engine (fresh) while
// getSelectedInstrument() read the mirror (stale). After the fix the payload
// of a forwarded selection event becomes mirror state directly.
void ConnectViaIpcModeTest::testMirrorAppliesSelectionEvents() {
	___INFOLOG( "" );
	auto* pMirror = TestHelper::makeMirror();
	EditorStateMirror mirror( pMirror );

	auto pSong = pMirror->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments->size() >= 2 );
	auto pPatternList = pSong->getPatternList();
	pMirror->getAudioEngine()->lock( RIGHT_HERE );
	while ( pPatternList->size() < 2 ) {
		pPatternList->add( std::make_shared<Pattern>() );
	}
	pMirror->getAudioEngine()->unlock();

	// Instrument: the forwarded event's payload becomes mirror state ...
	mirror.applyEvent( IpcMessage::fromEvent(
		Event::Type::SelectedInstrumentChanged, 1, 0 ) );
	CPPUNIT_ASSERT_EQUAL( 1, pMirror->getSelectedInstrumentNumber() );
	// ... so the object flavor (the one ~40 GUI callers use) agrees with the
	// number flavor.
	CPPUNIT_ASSERT( pMirror->getSelectedInstrument() == pInstruments->get( 1 ) );

	// Pattern.
	mirror.applyEvent( IpcMessage::fromEvent(
		Event::Type::SelectedPatternChanged, 1, 0 ) );
	CPPUNIT_ASSERT_EQUAL( 1, pMirror->getSelectedPatternNumber() );

	// Record mode (applied via the bare setter — no local event expected).
	mirror.applyEvent( IpcMessage::fromEvent(
		Event::Type::RecordModeChanged, 1, 0 ) );
	CPPUNIT_ASSERT( pMirror->getRecordEnabled() );

	// Non-Event messages are still refused.
	CPPUNIT_ASSERT( ! mirror.applyEvent(
		IpcMessage( IpcOpcode::GetSelectedInstrument ) ) );

	delete pMirror;

	___INFOLOG( "passed" );
}

// The engine-side producers used to push selection events with a hard-coded
// value of -1 (Hydrogen::setSelectedInstrumentNumber/setSelectedPatternNumber)
// or 0 (CoreActionController::renameComponent/setPattern). Once the mirror
// applies event payloads, those placeholders would corrupt the editor's
// selection. Every producer must carry the stored (post-clamp) state instead.
void ConnectViaIpcModeTest::testSelectionEventsCarryStoredState() {
	___INFOLOG( "" );
	auto* pEngine = TestHelper::makeMirror();
	auto pQueue = pEngine->getEventQueue();

	auto pSong = pEngine->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments->size() >= 2 );
	auto pPatternList = pSong->getPatternList();
	pEngine->getAudioEngine()->lock( RIGHT_HERE );
	while ( pPatternList->size() < 2 ) {
		pPatternList->add( std::make_shared<Pattern>() );
	}
	pEngine->getAudioEngine()->unlock();

	// Pops the queue until an event of @a type shows up; reports whether one
	// was found and returns its value.
	bool bFound = false;
	auto popValue = [&]( Event::Type type ) -> int {
		bFound = false;
		while ( auto pEvent = pQueue->popEvent() ) {
			if ( pEvent->getType() == type ) {
				bFound = true;
				return pEvent->getValue();
			}
		}
		return 0;
	};

	// Instrument: the event carries the stored number, not -1.
	pEngine->setSelectedInstrumentNumber( 1, Event::Trigger::Default );
	int nValue = popValue( Event::Type::SelectedInstrumentChanged );
	CPPUNIT_ASSERT( bFound );
	CPPUNIT_ASSERT_EQUAL( 1, nValue );
	CPPUNIT_ASSERT_EQUAL( 1, pEngine->getSelectedInstrumentNumber() );

	// Force on an unchanged value still refreshes, carrying the stored number.
	pEngine->setSelectedInstrumentNumber( 1, Event::Trigger::Force );
	nValue = popValue( Event::Type::SelectedInstrumentChanged );
	CPPUNIT_ASSERT( bFound );
	CPPUNIT_ASSERT_EQUAL( 1, nValue );

	// Out-of-range clamps to "no selection" and the event reports that.
	pEngine->setSelectedInstrumentNumber( 42, Event::Trigger::Default );
	nValue = popValue( Event::Type::SelectedInstrumentChanged );
	CPPUNIT_ASSERT( bFound );
	CPPUNIT_ASSERT_EQUAL( -1, nValue );
	CPPUNIT_ASSERT_EQUAL( -1, pEngine->getSelectedInstrumentNumber() );

	// Restore a valid selection for the component rename below.
	pEngine->setSelectedInstrumentNumber( 1, Event::Trigger::Default );
	popValue( Event::Type::SelectedInstrumentChanged );
	CPPUNIT_ASSERT( bFound );

	// Pattern: same invariant.
	pEngine->setSelectedPatternNumber( 1, true, Event::Trigger::Default );
	nValue = popValue( Event::Type::SelectedPatternChanged );
	CPPUNIT_ASSERT( bFound );
	CPPUNIT_ASSERT_EQUAL( 1, nValue );

	// CoreActionController producers must not clobber the payload either
	// (renameComponent used to push a hard-coded 0).
	CPPUNIT_ASSERT( pEngine->getCoreActionController()->renameComponent(
		1, 0, "renamed" ) );
	nValue = popValue( Event::Type::SelectedInstrumentChanged );
	CPPUNIT_ASSERT( bFound );
	CPPUNIT_ASSERT_EQUAL( 1, nValue );

	delete pEngine;

	___INFOLOG( "passed" );
}

// End-to-end engine -> editor sync: an engine-side selection change (e.g. the
// headless engine's MIDI-to-selected-instrument routing) must reach the
// mirror, and both IEngineAccess read flavors must agree afterwards.
void ConnectViaIpcModeTest::testEngineSelectionChangesReachMirror() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeMirror();

	auto pSong = pEngine->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments->size() >= 2 );
	auto pPatternList = pSong->getPatternList();
	pEngine->getAudioEngine()->lock( RIGHT_HERE );
	while ( pPatternList->size() < 2 ) {
		pPatternList->add( std::make_shared<Pattern>() );
	}
	pEngine->getAudioEngine()->unlock();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	// Instrument selection.
	pEngine->setSelectedInstrumentNumber( 1, Event::Trigger::Default );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getSelectedInstrumentNumber() == 1; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );
	CPPUNIT_ASSERT_EQUAL( 1, pIpcAccess->getSelectedInstrumentNumber() );
	CPPUNIT_ASSERT( pIpcAccess->getSelectedInstrument() ==
		pMirror->getSong()->getDrumkit()->getInstruments()->get( 1 ) );

	// Pattern selection.
	pEngine->setSelectedPatternNumber( 1, true, Event::Trigger::Default );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getSelectedPatternNumber() == 1; } ) );
	CPPUNIT_ASSERT_EQUAL( 1, pIpcAccess->getSelectedPatternNumber() );

	// Record mode via the controller (the engine-side producer).
	pEngine->getCoreActionController()->activateRecordMode( true );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getRecordEnabled(); } ) );
	CPPUNIT_ASSERT( pIpcAccess->getRecordEnabled() );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// End-to-end editor -> engine sync: the editor's selection change must reach
// the engine (MIDI-to-selected-instrument routing runs there), and the echo
// back must keep the mirror consistent.
void ConnectViaIpcModeTest::testEditorSelectionReachesEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeMirror();

	auto pSong = pEngine->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstruments->size() >= 2 );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	pIpcAccess->setSelectedInstrumentNumber( 1 );
	// Pump until both sides agree: the engine applies the forwarded command
	// and its echo event re-applies the value on the mirror. (The mirror
	// briefly reads 0 in between: the attach-time priming push can arrive
	// after the local apply of the command.)
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSelectedInstrumentNumber() == 1 &&
			pMirror->getSelectedInstrumentNumber() == 1; } ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// An editor channel delivers frames via the messageReceived signal; its
// pending queue is reserved for correlated replies (consumed by request()).
// Queueing signal-delivered frames too would accumulate them for the whole
// session — the editor has no polling receive() consumer to drain them.
void ConnectViaIpcModeTest::testEditorChannelDeliversViaSignalNotQueue() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( pSession->isConnected() );

	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	// The engine forwards an event ...
	conn->send( IpcMessage::fromEvent(
		Event::Type::SelectedPatternChanged, 1, 0 ) );

	// ... the editor's mirror applies it via the signal path ...
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getSelectedPatternNumber() == 1; } ) );

	// ... and the channel must not ALSO queue it.
	IpcMessage msg;
	CPPUNIT_ASSERT( ! pSession->getChannel()->receive( msg, 50, false ) );
	CPPUNIT_ASSERT_EQUAL( 0, pSession->getChannel()->pendingCount() );

	pSession.reset();
	delete pMirror;

	___INFOLOG( "passed" );
}

// When the IPC connection is lost, HydrogenApp::onIpcConnectionLost() drops the
// IpcEngineAccess and pEngine() falls back to a LocalEngineAccess wrapping the
// same mirror engine (ADR 0016). This test verifies that core mechanism: after
// destroying the IPC handle, a LocalEngineAccess on the same mirror serves
// reads and commands without crashing — the GUI stays responsive in degraded
// mode (no audio, mirror-only commands) until the user reconnects.
void ConnectViaIpcModeTest::testEngineAccessFallsBackToLocal() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );
	IpcMessage hello;
	CPPUNIT_ASSERT( conn->receive( hello ) ); // consume handshake

	// IPC-backed access works while connected.
	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );
	CPPUNIT_ASSERT( pIpcAccess->getSong() != nullptr );

	// Simulate connection loss: the engine closes its end, then
	// onIpcConnectionLost() drops the IPC handle.
	conn->close();
	pIpcAccess.reset();

	// The fallback: LocalEngineAccess wrapping the same mirror. pEngine()
	// creates this lazily when m_pEngineAccess is null.
	LocalEngineAccess localAccess( pMirror );
	CPPUNIT_ASSERT( localAccess.getSong() != nullptr );
	CPPUNIT_ASSERT( localAccess.getSong().get() == pMirror->getSong().get() );
	CPPUNIT_ASSERT( localAccess.getCoreActionController() != nullptr );
	CPPUNIT_ASSERT( localAccess.getEventQueue() != nullptr );

	// Commands must not crash in degraded mode (they apply to the mirror only).
	localAccess.sequencerStop();

	pSession.reset();
	delete pMirror;

	___INFOLOG( "passed" );
}

// The editor pulls the full engine state via IPC request/response (ADR 0032):
// GetSong, GetSelectedPattern, GetSelectedInstrument, GetRecordEnabled. Each
// request is answered by IpcEngineBridge::handleRequest() on the engine side;
// the editor applies the replies to its mirror so the GUI reads consistent
// state. This test verifies the round-trip for the scalar state and the song
// payload.
//
// The engine side uses EngineSession::start() — the production serve loop that
// creates the IpcServer and IpcChannel on a Qt-managed bridge thread. A
// QLocalSocket is thread-affine (its QSocketNotifier may only be pumped on the
// thread that owns it), so the server and channel must live on the same
// QThread; using EngineSession ensures this rather than manually spinning a
// std::thread that Qt cannot manage.
void ConnectViaIpcModeTest::testSyncViaIpc() {
	___INFOLOG( "" );

	const QString sEndpoint = TestHelper::uniqueEndpoint();

	// Engine side: a headless engine with known state, served via the
	// production EngineSession serve loop on its own QThread.
	auto* pEngine = TestHelper::makeMirror();
	auto pSong = pEngine->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	pSong->setName( "SYNC_TEST" );
	pEngine->setSelectedPatternNumber( 2 );
	pEngine->setSelectedInstrumentNumber( 1 );
	pEngine->setRecordEnabled( true );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	// Editor side: a fresh mirror with default state.
	auto* pMirror = TestHelper::makeMirror();
	CPPUNIT_ASSERT( pMirror->getSong() != nullptr );
	CPPUNIT_ASSERT( pMirror->getSong()->getName().toStdString() != "SYNC_TEST" );

	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( pSession->isConnected() );

	auto pChannel = pSession->getChannel();
	CPPUNIT_ASSERT( pChannel != nullptr );

	// 1. GetSong — reply carries the song XML payload.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetSong ), reply, 3000 ) );
		CPPUNIT_ASSERT( ! reply.getPayload().isEmpty() );

		// Deserialize the reply XML and apply to the mirror.
		auto pSong = Song::fromXmlBuffer(
			reply.getPayload(), Xml::Flag::Ipc, true, pMirror
		);
		CPPUNIT_ASSERT( pSong != nullptr );
		pMirror->getCoreActionController()->setSong( pSong );

		CPPUNIT_ASSERT_EQUAL(
			std::string( "SYNC_TEST" ),
			pMirror->getSong()->getName().toStdString() );
	}

	// 2. GetSelectedPattern — reply carries an int arg.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetSelectedPattern ), reply, 3000 ) );
		const auto& args = reply.getArgs();
		CPPUNIT_ASSERT( ! args.isEmpty() );
		CPPUNIT_ASSERT_EQUAL( 2, args[0].toInt() );
	}

	// 3. GetSelectedInstrument — reply carries an int arg.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetSelectedInstrument ), reply, 3000 ) );
		const auto& args = reply.getArgs();
		CPPUNIT_ASSERT( ! args.isEmpty() );
		CPPUNIT_ASSERT_EQUAL( 1, args[0].toInt() );
	}

	// 4. GetRecordEnabled — reply carries a bool arg.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetRecordEnabled ), reply, 3000 ) );
		const auto& args = reply.getArgs();
		CPPUNIT_ASSERT( ! args.isEmpty() );
		CPPUNIT_ASSERT( args[0].toBool() );
	}

	// 5. GetCorePreferences — reply carries an XML payload.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetCorePreferences ), reply, 3000 ) );
		CPPUNIT_ASSERT( ! reply.getPayload().isEmpty() );
	}

	// 6. GetSoundLibraryInfo — reply carries three QStringLists.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetSoundLibraryInfo ), reply, 3000 ) );
		const auto& args = reply.getArgs();
		CPPUNIT_ASSERT( args.size() >= 3 );
		// drumkitFolders, customDrumkitFolders, customDrumkitPaths
		CPPUNIT_ASSERT( ! args[0].toStringList().isEmpty() );
	}

	// Clean up: stop the serve loop (joins the bridge thread) before deleting
	// the engines.
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}
