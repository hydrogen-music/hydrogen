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

#include <QtNetwork/QUdpSocket>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IO/AudioDriverInfo.h>
#include <core/IO/MidiBaseDriver.h>
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
#include <core/Midi/Midi.h>
#include <core/Midi/MidiMessage.h>
#include <core/NsmClient.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>

#include <QtCore/QCoreApplication>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <thread>

using namespace H2Core;

namespace {
// Binds an ephemeral UDP port and returns the owning socket (nullptr on
// failure). OSC travels over UDP, so occupying — or probing — a port
// requires a UDP bind; a TCP listener would not conflict with liblo.
// AnyIPv4 matches liblo's AF_INET wildcard and DontShareAddress binds
// exclusively (no SO_REUSEADDR), so the engine's server cannot claim the
// port while the socket lives; releasing the unique_ptr releases the port.
// Qt networking keeps this portable across the Linux/macOS/Windows test
// pipelines (raw POSIX sockets would not build on MSVC).
std::unique_ptr<QUdpSocket> bindEphemeralUdpPort() {
	auto pSocket = std::make_unique<QUdpSocket>();
	if ( ! pSocket->bind( QHostAddress::AnyIPv4, 0,
						  QAbstractSocket::DontShareAddress ) ) {
		return nullptr;
	}
	return pSocket;
}
} // namespace

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

// The engine owns the MIDI drivers (ADR 0029): port enumeration and the
// handled-message logs must cross the IPC boundary field-complete. The
// editor re-wraps the entries and the GUI tables show exactly what the
// engine logged.
void ConnectViaIpcModeTest::testMidiDriverReadsRoundTrip() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngineWithLoopBackMidi();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	auto pDriver = pEngine->getMidiDriver();
	CPPUNIT_ASSERT( pDriver != nullptr );

	// Seed the output log first: the LoopBack driver loops every sent
	// message back as input, so this entry also lands in the input log.
	pDriver->enqueueOutputMessage( MidiMessage(
		MidiMessage::Type::NoteOn, static_cast<Midi::Parameter>( 60 ),
		static_cast<Midi::Parameter>( 110 ), Midi::Channel( 1 ) ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pDriver->getHandledOutputs().size() == 1 &&
			pDriver->getHandledInputs().size() == 1; } ) );

	// Then a pure input entry with distinct field values (a marshaling swap
	// between positions must fail the comparison below).
	pDriver->enqueueInputMessage( MidiMessage(
		MidiMessage::Type::ControlChange, static_cast<Midi::Parameter>( 7 ),
		static_cast<Midi::Parameter>( 64 ), Midi::Channel( 10 ) ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pDriver->getHandledInputs().size() == 2; } ) );

	// Both logs are quiet now (no MIDI clock stream in this fixture), so
	// the engine-side snapshot taken while serving the query and the local
	// snapshot taken here are identical.
	const auto localInputs = pDriver->getHandledInputs();
	const auto ipcInputs = pIpcAccess->getHandledMidiInputs();
	CPPUNIT_ASSERT_EQUAL( localInputs.size(), ipcInputs.size() );
	for ( std::size_t ii = 0; ii < localInputs.size(); ++ii ) {
		CPPUNIT_ASSERT( localInputs[ ii ]->type == ipcInputs[ ii ]->type );
		CPPUNIT_ASSERT( localInputs[ ii ]->data1 == ipcInputs[ ii ]->data1 );
		CPPUNIT_ASSERT( localInputs[ ii ]->data2 == ipcInputs[ ii ]->data2 );
		CPPUNIT_ASSERT(
			localInputs[ ii ]->channel == ipcInputs[ ii ]->channel );
		// Engine and editor share the machine clock, so the epoch count
		// must survive the round-trip bit-exactly.
		CPPUNIT_ASSERT(
			localInputs[ ii ]->timePoint.time_since_epoch().count() ==
			ipcInputs[ ii ]->timePoint.time_since_epoch().count() );
		CPPUNIT_ASSERT( localInputs[ ii ]->actionTypes ==
			ipcInputs[ ii ]->actionTypes );
		CPPUNIT_ASSERT( localInputs[ ii ]->mappedInstruments ==
			ipcInputs[ ii ]->mappedInstruments );
	}

	const auto localOutputs = pDriver->getHandledOutputs();
	const auto ipcOutputs = pIpcAccess->getHandledMidiOutputs();
	CPPUNIT_ASSERT_EQUAL( localOutputs.size(), ipcOutputs.size() );
	for ( std::size_t ii = 0; ii < localOutputs.size(); ++ii ) {
		CPPUNIT_ASSERT( localOutputs[ ii ]->type == ipcOutputs[ ii ]->type );
		CPPUNIT_ASSERT( localOutputs[ ii ]->data1 == ipcOutputs[ ii ]->data1 );
		CPPUNIT_ASSERT( localOutputs[ ii ]->data2 == ipcOutputs[ ii ]->data2 );
		CPPUNIT_ASSERT(
			localOutputs[ ii ]->channel == ipcOutputs[ ii ]->channel );
		CPPUNIT_ASSERT(
			localOutputs[ ii ]->timePoint.time_since_epoch().count() ==
			ipcOutputs[ ii ]->timePoint.time_since_epoch().count() );
	}

	// Port enumeration is a live query against the engine's driver too; the
	// LoopBack driver exposes no external ports.
	CPPUNIT_ASSERT( pIpcAccess->getMidiPorts(
		MidiBaseDriver::PortType::Input ).empty() );
	CPPUNIT_ASSERT( pIpcAccess->getMidiPorts(
		MidiBaseDriver::PortType::Output ).empty() );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The MidiControlDialog clears the logs and re-queries right after. The
// channel is FIFO, so a query issued after the clear command must observe
// the cleared log — not a stale pre-clear snapshot.
void ConnectViaIpcModeTest::testHandledMidiLogClearOrdering() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngineWithLoopBackMidi();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	auto pDriver = pEngine->getMidiDriver();
	CPPUNIT_ASSERT( pDriver != nullptr );

	pDriver->enqueueOutputMessage( MidiMessage(
		MidiMessage::Type::NoteOn, static_cast<Midi::Parameter>( 60 ),
		static_cast<Midi::Parameter>( 110 ), Midi::Channel( 1 ) ) );
	pDriver->enqueueInputMessage( MidiMessage(
		MidiMessage::Type::ControlChange, static_cast<Midi::Parameter>( 7 ),
		static_cast<Midi::Parameter>( 64 ), Midi::Channel( 10 ) ) );
	// Wait for the loopback of the output message to settle so no entry can
	// sneak in behind the clears below.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pDriver->getHandledOutputs().size() == 1 &&
			pDriver->getHandledInputs().size() == 2; } ) );

	// The dialog clears both logs, then re-queries. The clears are async
	// commands; wait for the engine-side effect first (the bridge applies
	// them within its poll cadence).
	pIpcAccess->getCoreActionController()->clearMidiInputLog();
	pIpcAccess->getCoreActionController()->clearMidiOutputLog();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pDriver->getHandledInputs().empty() &&
			pDriver->getHandledOutputs().empty(); } ) );

	// The channel is FIFO, so queries issued after the clears must observe
	// the post-clear state too.
	CPPUNIT_ASSERT( pIpcAccess->getHandledMidiInputs().empty() );
	CPPUNIT_ASSERT( pIpcAccess->getHandledMidiOutputs().empty() );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// In editor mode CoreActionController::setPreferences() runs on the mirror
// as the dual-apply of the forwarded command. The mirror owns no audio/MIDI
// drivers (ADR 0016), so restarting them is a no-op that still pushes
// driver-changed events — noise the GUI would react to. The restarts belong
// to the authoritative engine alone.
void ConnectViaIpcModeTest::testSetPreferencesSkipsMirrorRestarts() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();

	// Drain the queue so only events pushed by the call below are examined.
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
	}

	pMirror->getCoreActionController()->setPreferences(
		pMirror->getPreferences() );

	bool bSpuriousDriverChange = false;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		if ( pEvent->getType() == Event::Type::MidiDriverChanged ||
			 pEvent->getType() == Event::Type::AudioDriverChanged ) {
			bSpuriousDriverChange = true;
		}
	}
	CPPUNIT_ASSERT( ! bSpuriousDriverChange );

	delete pMirror;

	___INFOLOG( "passed" );
}

// The editor's driver-restart affordances forward via setPreferences: the
// engine applies the new configuration and restarts its drivers, and the
// editor observes the restart through the forwarded MidiDriverChanged event.
void ConnectViaIpcModeTest::testSetPreferencesRestartsEngineDrivers() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngineWithLoopBackMidi();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	const auto pDriverBefore = pEngine->getMidiDriver();
	CPPUNIT_ASSERT( pDriverBefore != nullptr );

	// The dialog forwards its local preferences; the engine restarts its
	// MIDI driver while applying them. The mirror is configured with
	// MidiDriver::None (EditorSession::configureMirrorPreferences) — and
	// the test-only LoopBack driver is deliberately not parseable, so it
	// crosses as None — leaving the restarted engine driverless. What
	// matters here is that the restart cycle ran on the engine, not the
	// resulting driver kind.
	pIpcAccess->getCoreActionController()->setPreferences(
		pMirror->getPreferences() );

	// The engine cycled its MIDI driver ...
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getMidiDriver() != pDriverBefore; } ) );
	// ... and the editor can observe the restart.
	CPPUNIT_ASSERT( TestHelper::pumpUntilEvent(
		pMirror, Event::Type::MidiDriverChanged, 0 ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Audio device and host-API enumeration is served by the engine's driver
// stack. With the Fake driver running, the PortAudio branch must answer
// with a clean empty list — the previous mirror-local implementation
// dereferenced an empty device map here (UB) — and the ALSA branch must
// answer (its content is host-specific).
void ConnectViaIpcModeTest::testAudioDeviceQueriesRoundTrip() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	// The running Fake driver is not PortAudio: no host APIs, and a device
	// query for a foreign kind must come back cleanly empty.
	CPPUNIT_ASSERT( pIpcAccess->getAudioHostAPIs().empty() );
	CPPUNIT_ASSERT( pIpcAccess->getAudioDevices(
		Preferences::AudioDriver::PortAudio, QString( "nonexistent" )
	).empty() );
	CPPUNIT_ASSERT( pIpcAccess->getAudioDevices(
		Preferences::AudioDriver::Fake, QString() ).empty() );
	// ALSA enumerates statically, independent of the running driver; the
	// result depends on the host, so only require an answer.
	pIpcAccess->getAudioDevices( Preferences::AudioDriver::Alsa, QString() );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The editor process must never use OSC or NSM itself: the headless engine
// owns all control surfaces (ADR 0016/0026). The mirror therefore holds no
// OscServer/NsmClient objects at all — not inert placeholder ones — so a
// stray control-surface call fails loudly instead of silently no-oping.
void ConnectViaIpcModeTest::testMirrorHoldsNoOscOrNsmObjects() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	auto* pMirror = TestHelper::makeMirror();

#ifdef H2CORE_HAVE_OSC
	CPPUNIT_ASSERT( pMirror->getOscServer() == nullptr );
	CPPUNIT_ASSERT( pMirror->getNsmClient() == nullptr );

	// Recreate/toggle requests (e.g. from the preferences dialog) must stay
	// no-ops in the editor instead of building local objects.
	pMirror->recreateOscServer();
	pMirror->toggleOscServer( true );
	CPPUNIT_ASSERT( pMirror->getOscServer() == nullptr );

	// The headless engine keeps its own instances (per-instance ownership,
	// ADR 0015).
	CPPUNIT_ASSERT( pEngine->getOscServer() != nullptr );
	CPPUNIT_ASSERT( pEngine->getNsmClient() != nullptr );
#endif

	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Under NSM, Hydrogen::setSong() pins a replacement song to the current
// session song's path (the session owns the file location). The mirror has
// no NsmClient, so it must not run that pinning: an empty session folder
// would capture every song (QString::contains("") is always true). The
// engine re-applies the policy on its side when the change syncs.
void ConnectViaIpcModeTest::testMirrorSetSongDoesNotPinPathUnderNsm() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	pMirror->setCachedUnderSessionManagement( true );

	auto pSongA = Song::getEmptySong( pMirror );
	pSongA->setPath( "/tmp/nsm-session-a.h2song" );
	pMirror->setSong( pSongA );

	auto pSongB = Song::getEmptySong( pMirror );
	pSongB->setPath( "/tmp/nsm-session-b.h2song" );
	pMirror->setSong( pSongB );

	CPPUNIT_ASSERT( pMirror->getSong()->getPath() ==
					QString( "/tmp/nsm-session-b.h2song" ) );

	pMirror->setCachedUnderSessionManagement( false );
	delete pMirror;

	___INFOLOG( "passed" );
}

// setSongModified() reports the dirty state to the NSM server when under
// session management. In the editor split that reporting belongs to the
// engine's NsmClient; the mirror must tolerate the call (and still flag the
// song locally) even while the cached under-session-management state is
// true.
void ConnectViaIpcModeTest::testMirrorSetSongModifiedWithoutNsmClient() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	pMirror->setCachedUnderSessionManagement( true );

	pMirror->setSongModified( true );
	CPPUNIT_ASSERT( pMirror->getSongModified() );
	pMirror->setSongModified( false );
	CPPUNIT_ASSERT( ! pMirror->getSongModified() );

	pMirror->setCachedUnderSessionManagement( false );
	delete pMirror;

	___INFOLOG( "passed" );
}

// The OSC port a remote control surface must dial is only knowable in the
// authoritative engine: when the configured port is unavailable its server
// falls back to a temporary one, and in the editor split the preferences
// dialog is the only place that value is shown to users. It must therefore
// travel across the IPC boundary (ADR 0029 query pattern).
void ConnectViaIpcModeTest::testOscTemporaryPortQueryRoundTrip() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	// Occupy the configured port so the engine's OSC server must fall back
	// to a temporary one.
	auto pOccupier = bindEphemeralUdpPort();
	CPPUNIT_ASSERT( pOccupier != nullptr );
	const int nConfiguredPort = pOccupier->localPort();
	CPPUNIT_ASSERT( nConfiguredPort > 0 );

	auto pPref = H2Core::Preferences::create_instance();
	pPref->m_audioDriver = H2Core::Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = H2Core::Preferences::MidiDriver::None;
	pPref->setOscServerEnabled( true );
	pPref->setOscServerPort( nConfiguredPort );
	auto* pEngine = new H2Core::Hydrogen(
		pPref, H2Core::ProcessMode::Headless, -1 );
	pEngine->setFullyOperational( true );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

#ifdef H2CORE_HAVE_OSC
	// The engine fell back from the occupied port...
	CPPUNIT_ASSERT( pEngine->getOscTemporaryPort() != -1 );
	CPPUNIT_ASSERT( pEngine->getOscTemporaryPort() != nConfiguredPort );
	// ...and the editor must see exactly that value across the split.
	CPPUNIT_ASSERT( pIpcAccess->getOscTemporaryPort() ==
					pEngine->getOscTemporaryPort() );
	// The mirror itself holds no OSC server (ADR 0016/0026).
	CPPUNIT_ASSERT( pMirror->getOscTemporaryPort() == -1 );
#endif

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The dialog-facing restart of the OSC server must reach the authoritative
// engine in the editor split: CoreActionController is the single write
// surface (ADR 0027) and forwards the command over IPC (ADR 0030), while
// the mirror — holding no server — stays untouched. Object identity is no
// observable proof of the restart (the allocator may hand the recycled
// block back), so the test drives the fallback state: the engine starts on
// an occupied configured port (temporary fallback in effect) and must end
// up serving the newly configured free port directly (no fallback).
void ConnectViaIpcModeTest::testRecreateOscServerForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	// Stage 1: occupy the configured port so the engine's OSC server must
	// fall back to a temporary one.
	auto pOccupier = bindEphemeralUdpPort();
	CPPUNIT_ASSERT( pOccupier != nullptr );
	const int nOccupiedPort = pOccupier->localPort();
	CPPUNIT_ASSERT( nOccupiedPort > 0 );

	auto pPref = H2Core::Preferences::create_instance();
	pPref->m_audioDriver = H2Core::Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = H2Core::Preferences::MidiDriver::None;
	pPref->setOscServerEnabled( true );
	pPref->setOscServerPort( nOccupiedPort );
	auto* pEngine = new H2Core::Hydrogen(
		pPref, H2Core::ProcessMode::Headless, -1 );
	pEngine->setFullyOperational( true );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

#ifdef H2CORE_HAVE_OSC
	// The engine fell back from the occupied port, and the editor sees
	// exactly that value across the split.
	const int nPortBefore = pEngine->getOscTemporaryPort();
	CPPUNIT_ASSERT( nPortBefore != -1 );
	CPPUNIT_ASSERT( nPortBefore != nOccupiedPort );
	CPPUNIT_ASSERT( pIpcAccess->getOscTemporaryPort() == nPortBefore );

	// Stage 2: the user picks a free port in the dialog and hits OK. The
	// engine's preferences carry the new port (the forwarded SetPreferences
	// applies them), and the dialog-facing restart command must make the
	// engine rebuild its server on that port.
	auto pProbe = bindEphemeralUdpPort();
	CPPUNIT_ASSERT( pProbe != nullptr );
	const int nFreePort = pProbe->localPort();
	CPPUNIT_ASSERT( nFreePort > 0 );
	pProbe.reset(); // release the port — the engine's new server binds it

	pEngine->getPreferences()->setOscServerPort( nFreePort );

	CPPUNIT_ASSERT(
		pIpcAccess->getCoreActionController()->recreateOscServer() );

	// The engine replaced its server: the new one binds the free configured
	// port directly, so no fallback is in effect anymore. The state is
	// polled through the IPC query rather than the engine object — the
	// recreate runs on the engine's bridge thread, and only that thread may
	// touch the server while it swaps it. FIFO ordering on the channel
	// guarantees the first query is answered after the recreate.
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pIpcAccess->getOscTemporaryPort() == -1; } ) );
	// The mirror never gains a server of its own (ADR 0016/0026).
	CPPUNIT_ASSERT( pMirror->getOscServer() == nullptr );
#endif

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// NSM judges "unsaved changes" by the dirty state the engine's NsmClient
// reports, but modifications happen in the editor process. The flip must
// cross the split (ADR 0030 command pattern): the engine's copy follows
// (and its client reports to the session manager when one is connected),
// while the mirror applies locally so the GUI title updates immediately.
void ConnectViaIpcModeTest::testSongModifiedForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT( pEngine->getSong() != nullptr );
	CPPUNIT_ASSERT( ! pEngine->getSong()->getIsModified() );
	CPPUNIT_ASSERT( ! pMirror->getSong()->getIsModified() );

	// An edit marks the song modified: the mirror reflects it at once ...
	pIpcAccess->setSongModified( true );
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );
	// ... and the engine's copy follows via the forwarded command.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsModified(); } ) );

	// Saving clears it on both sides.
	pIpcAccess->setSongModified( false );
	CPPUNIT_ASSERT( ! pMirror->getSong()->getIsModified() );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getSong()->getIsModified(); } ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The NSM session folder lives engine-side (only the engine's NsmClient
// talks to the session manager); the editor's drumkit-export path needs it
// across the split (ADR 0032 state-sync query). No live NSM server exists
// in tests — the client's folder is set directly, as the NSM open callback
// does in production.
void ConnectViaIpcModeTest::testSessionFolderQueryRoundTrip() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	const QString sSessionFolder = "nsm-session-folder-roundtrip";
#ifdef H2CORE_HAVE_OSC
	CPPUNIT_ASSERT( pEngine->getNsmClient() != nullptr );
	pEngine->getNsmClient()->setSessionFolderPath( sSessionFolder );
#endif

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

#ifdef H2CORE_HAVE_OSC
	// The editor sees the engine's session folder across the split ...
	CPPUNIT_ASSERT(
		pIpcAccess->getSessionFolderPath() == sSessionFolder );
	// ... while the mirror holds no client of its own (ADR 0016/0026).
	CPPUNIT_ASSERT( pMirror->getNsmClient() == nullptr );
#else
	// Without OSC support there is no NSM session at all.
	CPPUNIT_ASSERT( pIpcAccess->getSessionFolderPath().isEmpty() );
#endif

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The song XML carries isModified across the split (ipc-isModified), but
// CoreActionController::setSong() resets the flag unconditionally on the
// receiving side — every re-pull (HydrogenApp::ipcSyncSong, the remote
// UpdateSong handler) wipes the engine's dirty state on the mirror. The
// installer must preserve the pulled flag (ADR 0026 point 10).
void ConnectViaIpcModeTest::testPulledSongPreservesModifiedFlag() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pChannel = pSession->getChannel();
	CPPUNIT_ASSERT( pChannel != nullptr );

	// An engine-side edit (OSC/MIDI-reachable CAC surface) marks the
	// engine's song modified. (The call's return value only reflects
	// OSC feedback, which is absent in this harness.)
	pEngine->getCoreActionController()->setMasterIsMuted( true );
	CPPUNIT_ASSERT( pEngine->getSong()->getIsModified() );

	// The editor re-pulls the song exactly as HydrogenApp::ipcSyncSong
	// does when handling a remote UpdateSong/SongIsModified event.
	IpcMessage reply;
	CPPUNIT_ASSERT( pChannel->request(
		IpcMessage( IpcOpcode::GetSong ), reply, 3000 ) );
	CPPUNIT_ASSERT( ! reply.getPayload().isEmpty() );
	auto pSong = Song::fromXmlBuffer(
		reply.getPayload(), Xml::Flag::Ipc, true, pMirror );
	CPPUNIT_ASSERT( pSong != nullptr );
	// The XML carries the engine's dirty state ...
	CPPUNIT_ASSERT( pSong->getIsModified() );
	pMirror->getCoreActionController()->setSong( pSong );
	// ... and the mirror keeps it after installing the song.
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Engine-origin dirty flips (OSC/MIDI-reachable CAC commands) push
// SongIsModified on the engine queue; the generic event forward carries
// it to the editor tagged with the Headless origin — the signal
// HydrogenApp::handleRemoteEvent re-syncs the mirror on (ADR 0026
// point 10).
void ConnectViaIpcModeTest::testEngineFlipEventCrosses() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	// Drain the mirror queue of connect-time noise first.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// An engine-side edit (OSC/MIDI-reachable CAC surface) marks the
	// engine's song modified. (The call's return value only reflects
	// OSC feedback, which is absent in this harness.)
	pEngine->getCoreActionController()->setMasterIsMuted( true );
	CPPUNIT_ASSERT( pEngine->getSong()->getIsModified() );

	// The flip event crosses tagged as engine-origin. The found state
	// is sticky in the outer scope: pumpUntil() re-invokes the
	// condition after the loop, and a draining lambda would re-run on
	// an already-emptied queue.
	bool bFound = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pEvent;
		while ( ( pEvent = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pEvent->getType() == Event::Type::SongIsModified &&
				 pEvent->getOrigin() == H2Core::ProcessMode::Headless ) {
				bFound = true;
			}
		}
		return bFound;
	} ) );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Editor-originated flips apply on the mirror directly and forward as
// the SetSongModified command; the engine must not echo the flip back as
// an event — the editor already knows, and the echo would trigger a
// full song re-pull per edit once handleRemoteEvent re-syncs on
// SongIsModified (ADR 0026 point 10).
void ConnectViaIpcModeTest::testEditorFlipEchoSuppressed() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	// Drain the mirror queue of connect-time noise first.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	pIpcAccess->setSongModified( true );
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );

	// Give a (suppressed) echo ample time to not arrive.
	TestHelper::pumpUntil( []() { return false; }, 300 );

	// No engine-origin SongIsModified may sit in the mirror queue. The
	// mirror-local apply pushes one tagged Editor — that one is fine.
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		CPPUNIT_ASSERT( ! ( pEvent->getType() ==
								Event::Type::SongIsModified &&
							pEvent->getOrigin() ==
								H2Core::ProcessMode::Headless ) );
	}

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// A forwarded edit command (IpcCoreActionController: forward + mirror-local
// apply) must not echo SongIsModified back from the engine: the editor
// initiated and already applied the change, and the echo would trigger a
// redundant full song re-pull per edit in HydrogenApp::handleRemoteEvent
// (ADR 0026 point 13). Engine-local edits, on the other hand, are the
// editor's only signal to re-pull — their SongIsModified must fire on the
// headless engine even when the song is already dirty (the unchanged-flag
// early-return would swallow every edit after the first).
void ConnectViaIpcModeTest::testForwardedEditDoesNotEchoSongModified() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT( ! pEngine->getSong()->getIsModified() );

	// Drain the mirror queue of connect-time noise first.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// A forwarded edit: the mirror reflects it at once ...
	pIpcAccess->getCoreActionController()->setMasterIsMuted( true );
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );
	// ... and the engine's copy follows via the forwarded command.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsModified(); } ) );

	// Give a (suppressed) echo ample time to not arrive.
	TestHelper::pumpUntil( []() { return false; }, 300 );

	// No engine-origin SongIsModified may sit in the mirror queue. The
	// mirror-local apply pushes one tagged Editor — that one is fine.
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		CPPUNIT_ASSERT( ! ( pEvent->getType() ==
								Event::Type::SongIsModified &&
							pEvent->getOrigin() ==
								H2Core::ProcessMode::Headless ) );
	}

	// An engine-local edit (OSC/MIDI-reachable CAC surface) while the song
	// is already dirty: the flip event must still cross — it is the
	// editor's signal to re-pull the changed content. The found state is
	// sticky in the outer scope: pumpUntil() re-invokes the condition
	// after its loop, and a draining lambda would re-run on an
	// already-emptied queue.
	CPPUNIT_ASSERT( pEngine->getCoreActionController()->setBpm( 140.f ) );
	bool bFound = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pQueued;
		while ( ( pQueued = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pQueued->getType() == Event::Type::SongIsModified &&
				 pQueued->getOrigin() == H2Core::ProcessMode::Headless ) {
				bFound = true;
			}
		}
		return bFound;
	} ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Event::Trigger::Force promises the event is queued "regardless whether
// there are changes or not" (Event.h). setSongModified() must honor that
// outside the headless engine too: the transition-only early-return may
// not swallow an explicit Force (ADR 0026 point 13).
void ConnectViaIpcModeTest::testSongModifiedForceFiresWhenUnchanged() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	CPPUNIT_ASSERT( ! pMirror->getSongModified() );

	// Clean -> dirty is a transition: Default fires.
	pMirror->setSongModified( true );
	CPPUNIT_ASSERT( pMirror->getSongModified() );
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// Dirty -> dirty with Default: no event — the local GUI already
	// knows the state.
	pMirror->setSongModified( true );
	bool bFound = false;
	std::unique_ptr<Event> pQueued;
	while ( ( pQueued = pMirror->getEventQueue()->popEvent() )
			!= nullptr ) {
		if ( pQueued->getType() == Event::Type::SongIsModified ) {
			bFound = true;
		}
	}
	CPPUNIT_ASSERT( ! bFound );

	// Dirty -> dirty with Force: the event must be queued anyway.
	pMirror->setSongModified( true, Event::Trigger::Force );
	bFound = false;
	while ( ( pQueued = pMirror->getEventQueue()->popEvent() )
			!= nullptr ) {
		if ( pQueued->getType() == Event::Type::SongIsModified ) {
			bFound = true;
		}
	}
	CPPUNIT_ASSERT( bFound );

	delete pMirror;

	___INFOLOG( "passed" );
}

// The setDrumkitModified()/setPatternModified() wrappers must forward
// their dirty flip to setSongModified() even while the song is already
// dirty: on the headless engine the SongIsModified echo is the attached
// editor's only signal to re-pull engine-local edits (OSC/MIDI-reachable
// surface), and a clean->dirty-only guard would swallow every edit after
// the first (ADR 0026 point 13).
void ConnectViaIpcModeTest::testEngineWrapperFlipWhileDirtyEchoes() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	// Pattern::setIsModified() only sticks for file-backed patterns (a
	// non-empty path); give pattern 0 one so its flag is observable.
	pEngine->getSong()->getPatternList()->get( 0 )->setPath(
		QString( "/tmp/h2-pattern-modified-test.h2pattern" ) );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT( ! pEngine->getSong()->getIsModified() );

	// Drain the mirror queue of connect-time noise first.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// Make the engine's song dirty with an engine-local edit and let its
	// echo cross, so the pumps below can only match the wrappers' own
	// echoes.
	pEngine->getCoreActionController()->setMasterIsMuted( true );
	CPPUNIT_ASSERT( pEngine->getSong()->getIsModified() );
	TestHelper::pumpUntil( []() { return false; }, 300 );
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// A drumkit flip while already dirty: the echo must still cross. The
	// found state is sticky in the outer scope: pumpUntil() re-invokes
	// the condition after its loop, and a draining lambda would re-run
	// on an already-emptied queue.
	pEngine->setDrumkitModified( true );
	CPPUNIT_ASSERT( pEngine->getSong()->getDrumkit()->getIsModified() );
	bool bFound = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pQueued;
		while ( ( pQueued = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pQueued->getType() == Event::Type::SongIsModified &&
				 pQueued->getOrigin() == H2Core::ProcessMode::Headless ) {
				bFound = true;
			}
		}
		return bFound;
	} ) );

	TestHelper::pumpUntil( []() { return false; }, 300 );
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// Same for a pattern flip (pattern 0 exists in the empty song).
	pEngine->setPatternModified( true, 0 );
	CPPUNIT_ASSERT(
		pEngine->getSong()->getPatternList()->get( 0 )->getIsModified() );
	bFound = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pQueued;
		while ( ( pQueued = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pQueued->getType() == Event::Type::SongIsModified &&
				 pQueued->getOrigin() == H2Core::ProcessMode::Headless ) {
				bFound = true;
			}
		}
		return bFound;
	} ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Errors raised while the editor is attached cross via the generic
// event forward and reach the editor's error popup path
// (MainForm::errorEvent) — only their origin tag distinguishes them
// from local ones (ADR 0026 point 9).
void ConnectViaIpcModeTest::testRuntimeErrorForwardsToEditor() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	// Drain the mirror queue of connect-time noise first.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	pEngine->getEventQueue()->pushEvent(
		Event::Type::Error, Hydrogen::ERROR_STARTING_DRIVER );

	// The error crosses tagged as engine-origin. The found state is
	// sticky in the outer scope: pumpUntil() re-invokes the condition
	// after the loop, and a draining lambda would re-run on an
	// already-emptied queue.
	bool bFound = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pEvent;
		while ( ( pEvent = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pEvent->getType() == Event::Type::Error &&
				 pEvent->getValue() == Hydrogen::ERROR_STARTING_DRIVER &&
				 pEvent->getOrigin() == H2Core::ProcessMode::Headless ) {
				bFound = true;
			}
		}
		return bFound;
	} ) );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Errors raised before the editor attaches (engine boot, OSC port
// conflicts) used to vanish: the engine's serve loop discards queued
// events while no client is connected. The session retains the last
// errors and replays them on accept, so the editor user still sees
// e.g. the OSC port-busy popup (ADR 0026 point 9).
void ConnectViaIpcModeTest::testBootErrorReplayedOnConnect() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	// An error "raised" while no editor is attached. Wait past several
	// serve poll windows so the no-client branch has processed the
	// queue (retaining the error) before the editor connects.
	pEngine->getEventQueue()->pushEvent(
		Event::Type::Error, Hydrogen::OSC_CANNOT_CONNECT_TO_PORT );
	TestHelper::pumpUntil( []() { return false; }, 300 );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	// The retained error is replayed to the freshly attached editor ...
	int nSeen = 0;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pEvent;
		while ( ( pEvent = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pEvent->getType() == Event::Type::Error &&
				 pEvent->getOrigin() == H2Core::ProcessMode::Headless ) {
				++nSeen;
			}
		}
		return nSeen >= 1;
	} ) );
	// ... exactly once (no duplicate from the live pipeline).
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		if ( pEvent->getType() == Event::Type::Error &&
			 pEvent->getOrigin() == H2Core::ProcessMode::Headless ) {
			++nSeen;
		}
	}
	CPPUNIT_ASSERT_EQUAL( 1, nSeen );

	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The file browser, sound library, and sample editor audition instruments
// that are not part of the current song's kit — the number-based preview
// command can not address them, and the mirror's sampler can not render
// audio anyway. The ad-hoc instrument and its preview note cross as XML;
// the engine reloads the samples from their (shared-disk) paths and plays
// the note (ADR 0026 point 11).
void ConnectViaIpcModeTest::testAdhocInstrumentPreviewForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	// An ad-hoc preview instrument like SoundLibraryTree builds for
	// auditioning a library kit: flagged as preview, without a kit id, so
	// the engine can not resolve it from the song's drumkit either.
	auto pSample = Sample::load( H2TEST_FILE( "/drumkits/baseKit/hh.wav" ) );
	CPPUNIT_ASSERT( pSample != nullptr );
	auto pInstrument = Instrument::from( pSample, pMirror );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	pInstrument->setIsPreviewInstrument( true );
	pInstrument->setId( Instrument::EmptyId );

	auto pNote = std::make_shared<Note>(
		pInstrument, 0, VELOCITY_MAX, PAN_DEFAULT, LENGTH_ENTIRE_SAMPLE );

	CPPUNIT_ASSERT( pEngine->getAudioEngine()->getSampler()
		->getPlayingNotesNumber() == 0 );

	pIpcAccess->getCoreActionController()->previewInstrument(
		pInstrument, pNote );

	// The preview note must arrive in the engine's sampler queue. The
	// found state is sticky in the outer scope: the sample is short and
	// may finish playing before pumpUntil() re-invokes the condition
	// after its loop.
	bool bSawPreviewNote = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		if ( pEngine->getAudioEngine()->getSampler()
				 ->getPlayingNotesNumber() > 0 ) {
			bSawPreviewNote = true;
		}
		return bSawPreviewNote;
	} ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Beat-counter/tap-tempo config is editor-owned: the BpmTap buttons write
// the mirror's Hydrogen members and the mode actions write the mirror's
// preferences — neither syncs to the engine on its own (the engine's
// prefs copy goes stale, and its TapAndPlay branch reads it).
// updateBeatCounterSettings() therefore crosses as a config snapshot of
// the mirror's current state (ADR 0026 point 12).
void ConnectViaIpcModeTest::testUpdateBeatCounterSettingsForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	// Engine-side defaults (Hydrogen constructor).
	CPPUNIT_ASSERT( pEngine->getBeatCounterTotalBeats() == 4 );
	CPPUNIT_ASSERT( pEngine->getBeatCounterBeatLength() == 1.f );

	// Editor-side config: non-default values on the mirror's members and
	// preferences.
	auto pMirrorPrefs = pMirror->getPreferences();
	pMirrorPrefs->m_nBeatCounterDriftCompensation = 17;
	pMirrorPrefs->m_nBeatCounterStartOffset = 5;
	pMirrorPrefs->m_beatCounter = Preferences::BeatCounter::TapAndPlay;
	pMirror->setBeatCounterTotalBeats( 7 );
	pMirror->setBeatCounterBeatLength( 0.75f );

	pIpcAccess->updateBeatCounterSettings();

	// The engine adopts the snapshot: the members plus the TapAndPlay
	// mode preference its beat-counter completion branch reads.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getBeatCounterTotalBeats() == 7 &&
			std::abs( pEngine->getBeatCounterBeatLength() - 0.75f ) <
				0.001 &&
			pEngine->getPreferences()->m_beatCounter ==
				Preferences::BeatCounter::TapAndPlay;
	} ) );
	// Drift compensation and start offset cross in the same message (no
	// getters): the drift argument is pinned by the shifted BPM math in
	// testHandleBeatCounterForwardsToEngine (a swap with another
	// argument changes the result); the start offset only affects the
	// TapAndPlay lead-in sleep, not the BPM.

	// The engine's BeatCounter echo — carrying its event count — crosses
	// tagged as engine-origin. The found state is sticky in the outer
	// scope: pumpUntil() re-invokes the condition after its loop, and a
	// draining lambda would re-run on an already-emptied queue.
	bool bSawEcho = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		std::unique_ptr<Event> pEvent;
		while ( ( pEvent = pMirror->getEventQueue()->popEvent() )
				!= nullptr ) {
			if ( pEvent->getType() == Event::Type::BeatCounter &&
				 pEvent->getOrigin() == H2Core::ProcessMode::Headless ) {
				bSawEcho = true;
			}
		}
		return bSawEcho;
	} ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Taps are engine-authoritative: the mirror's handleBeatCounter() is a
// designed no-op in editor mode (getTempoSource() == Tempo::Remote), so
// the tap must cross and the engine's beat counter must run on the
// forwarded timestamps. The engine's BeatCounter echoes carry its event
// count so the editor's "n/total" display keeps working (ADR 0026
// point 12).
void ConnectViaIpcModeTest::testHandleBeatCounterForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	// The beat counter only runs when the engine owns the tempo.
	CPPUNIT_ASSERT( pEngine->getTempoSource() == Hydrogen::Tempo::Song );

	// Distinguishable baseline: the taps below land on 130.43 BPM —
	// distinct from both the baseline and the 120 default song tempo.
	CPPUNIT_ASSERT( pEngine->getCoreActionController()->setBpm( 100.f ) );

	// Editor-side config: count 3 taps, 40 ms drift compensation, no
	// start offset, TapAndPlay — the completion branch must start
	// playback on the engine. The nonzero drift pins the snapshot's
	// drift argument: swapping it with another argument would shift the
	// resulting BPM.
	auto pMirrorPrefs = pMirror->getPreferences();
	pMirrorPrefs->m_nBeatCounterDriftCompensation = 40;
	pMirrorPrefs->m_nBeatCounterStartOffset = 0;
	pMirrorPrefs->m_beatCounter = Preferences::BeatCounter::TapAndPlay;
	pMirror->setBeatCounterTotalBeats( 3 );
	pIpcAccess->updateBeatCounterSettings();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getBeatCounterTotalBeats() == 3 &&
			pEngine->getPreferences()->m_beatCounter ==
				Preferences::BeatCounter::TapAndPlay;
	} ) );

	// Explicit, evenly spaced stamps 0.5 s apart (exact in binary). With
	// the 40 ms drift compensation the effective deltas are 0.46 s.
	// Non-zero stamps: the epoch is the "no stamp" sentinel.
	const auto tFirst = Clock::now();
	pIpcAccess->handleBeatCounter( tFirst );
	pIpcAccess->handleBeatCounter(
		tFirst + std::chrono::milliseconds( 500 ) );

	// Two taps in: the engine's count (3) must have crossed back as the
	// BeatCounter echo value and been applied to the mirror.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getBeatCounterEventCount() == 3; } ) );

	pIpcAccess->handleBeatCounter(
		tFirst + std::chrono::milliseconds( 1000 ) );

	// Completion: 0.46 s effective average per beat at beat length 1 →
	// floor(60 / 0.46 · 100) / 100 = 130.43 BPM.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return std::abs( pEngine->getSong()->getBpm() - 130.43f ) < 0.05; } ) );
	// TapAndPlay: after the one-beat lead-in sleep the engine starts
	// playback.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getAudioEngine()->getState() ==
			AudioEngine::State::Playing; } ) );
	// The post-completion reset (count back to 1) crosses as well.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getBeatCounterEventCount() == 1; } ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Tap tempo crosses like the beat counter: the mirror's
// onTapTempoAccelEvent() is a designed no-op in editor mode
// (getTempoSource() == Tempo::Remote). No echo events: the running
// average feeds setBpm() directly (ADR 0026 point 12).
void ConnectViaIpcModeTest::testOnTapTempoAccelEventForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT( pEngine->getTempoSource() == Hydrogen::Tempo::Song );
	CPPUNIT_ASSERT( pEngine->getCoreActionController()->setBpm( 100.f ) );

	const auto tFirst = Clock::now();
	// The first tap only initializes the reference point (the stale
	// epoch default forces an average reset); the second sets the tempo.
	pIpcAccess->onTapTempoAccelEvent( tFirst );
	pIpcAccess->onTapTempoAccelEvent(
		tFirst + std::chrono::milliseconds( 300 ) );

	// 60 / 0.3 s = 200 BPM.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return std::abs( pEngine->getSong()->getBpm() - 200.f ) < 0.05; } ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Timeline activation is song state the GUI reads on the mirror —
// dual-apply: immediate local reflection plus the forwarded command
// (ADR 0026 point 12).
void ConnectViaIpcModeTest::testSetIsTimelineActivatedForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	const bool bInitial = pMirror->getSong()->getIsTimelineActivated();
	CPPUNIT_ASSERT(
		pEngine->getSong()->getIsTimelineActivated() == bInitial );

	// Toggle to the opposite so the command always changes state.
	pIpcAccess->setIsTimelineActivated( ! bInitial );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getIsTimelineActivated() == ! bInitial );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsTimelineActivated() == ! bInitial;
	} ) );

	// And back again.
	pIpcAccess->setIsTimelineActivated( bInitial );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getIsTimelineActivated() == bInitial );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsTimelineActivated() == bInitial;
	} ) );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// Pattern mode is engine-authoritative song state: forwarded, plus a
// local apply on the mirror. The engine-side apply flips the dirty flag
// with the Suppress trigger — the editor initiated the change and
// already applied it on its mirror, so an engine-origin SongIsModified
// echo would only trigger a redundant full song re-pull
// (ADR 0026 point 13).
void ConnectViaIpcModeTest::testSetPatternModeForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	const auto initialMode = pMirror->getPatternMode();
	CPPUNIT_ASSERT( pEngine->getPatternMode() == initialMode );
	CPPUNIT_ASSERT( ! pEngine->getSong()->getIsModified() );

	// Drain connect-time noise before the echo check below.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	const auto targetMode = initialMode == Song::PatternMode::Stacked
		? Song::PatternMode::Selected
		: Song::PatternMode::Stacked;
	pIpcAccess->setPatternMode( targetMode );

	// Immediate local reflection ...
	CPPUNIT_ASSERT( pMirror->getPatternMode() == targetMode );
	// ... the engine-side apply ...
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPatternMode() == targetMode; } ) );
	// ... including the dirty flip ...
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsModified(); } ) );

	// The engine-side apply is Suppress-triggered: the dirty flag flipped
	// (asserted above) but no engine-origin SongIsModified echo may
	// cross — the editor initiated the change and already applied it on
	// its mirror; the echo would trigger a redundant full song re-pull
	// per edit (ADR 0026 point 13).
	TestHelper::pumpUntil( []() { return false; }, 300 );
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		CPPUNIT_ASSERT( ! ( pEvent->getType() ==
								Event::Type::SongIsModified &&
							pEvent->getOrigin() ==
								H2Core::ProcessMode::Headless ) );
	}
	// The mirror-local apply flipped the mirror's own flag (its
	// Editor-origin event is filtered by the origin gate in the GUI).
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The playback track is engine-audible state: the local apply gives the
// GUI its immediate waveform, the forwarded command loads the sample
// engine-side (ADR 0026 point 12).
void ConnectViaIpcModeTest::testLoadPlaybackTrackForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT(
		pEngine->getSong()->getPlaybackTrackInstrument() == nullptr );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getPlaybackTrackInstrument() == nullptr );

	pIpcAccess->loadPlaybackTrack(
		H2TEST_FILE( "/drumkits/baseKit/hh.wav" ) );

	// Immediate local reflection: the mirror loads its own copy for the
	// waveform display.
	CPPUNIT_ASSERT(
		pMirror->getSong()->getPlaybackTrackInstrument() != nullptr );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getPlaybackTrackInstrument() != nullptr;
	} ) );
	CPPUNIT_ASSERT( pEngine->getSong()->getPlaybackTrackInstrument()
					->getId() == Instrument::PlaybackTrackId );

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The drumkit-modified flip is Class C song state (ADR 0026 point 13):
// the GUI writes it through the engine-access handle — dual-apply, with
// the engine-side apply under Suppress so no SongIsModified echo crosses
// (the editor initiated and already applied the flip on its mirror).
void ConnectViaIpcModeTest::testSetDrumkitModifiedForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT( ! pEngine->getSong()->getIsModified() );
	CPPUNIT_ASSERT( ! pEngine->getSong()->getDrumkit()->getIsModified() );

	// Drain connect-time noise before the echo check below.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	pIpcAccess->setDrumkitModified( true );

	// Immediate local reflection: the mirror's drumkit and song flags
	// flip at once (the mirror-local apply pushes an Editor-origin
	// SongIsModified — filtered by the origin gate in the GUI).
	CPPUNIT_ASSERT( pMirror->getSong()->getDrumkit()->getIsModified() );
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );

	// The engine follows via the forwarded command.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getDrumkit()->getIsModified() &&
			pEngine->getSong()->getIsModified();
	} ) );

	// No engine-origin SongIsModified echo may cross: the engine-side
	// apply runs under Suppress.
	TestHelper::pumpUntil( []() { return false; }, 300 );
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		CPPUNIT_ASSERT( ! ( pEvent->getType() ==
								Event::Type::SongIsModified &&
							pEvent->getOrigin() ==
								H2Core::ProcessMode::Headless ) );
	}

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The pattern-modified flip is Class C song state (ADR 0026 point 13):
// dual-apply through the engine-access handle, engine-side under
// Suppress (no SongIsModified echo — the editor already applied the
// flip on its mirror).
void ConnectViaIpcModeTest::testSetPatternModifiedForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	// Pattern::setIsModified() only sticks for file-backed patterns (a
	// non-empty path); give pattern 0 one so its flag is observable. Set
	// before connecting so the mirror's pulled copy carries the path
	// too (the ipc-path attribute round-trips).
	pEngine->getSong()->getPatternList()->get( 0 )->setPath(
		QString( "/tmp/h2-pattern-modified-test.h2pattern" ) );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	// The harness has no HydrogenApp, so no connect-time GetSong pull
	// runs: the mirror keeps its own (path-less) empty song. Model the
	// post-sync state — a real editor's pulled song carries the engine's
	// ipc-path — by backing the mirror's pattern 0 with a path too.
	pMirror->getSong()->getPatternList()->get( 0 )->setPath(
		QString( "/tmp/h2-pattern-modified-test.h2pattern" ) );
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	CPPUNIT_ASSERT( ! pEngine->getSong()->getIsModified() );
	CPPUNIT_ASSERT(
		! pEngine->getSong()->getPatternList()->get( 0 )->getIsModified() );

	// Drain connect-time noise before the echo check below.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	pIpcAccess->setPatternModified( true, 0 );

	// Immediate local reflection: the mirror's pattern and song flags
	// flip at once.
	CPPUNIT_ASSERT(
		pMirror->getSong()->getPatternList()->get( 0 )->getIsModified() );
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );

	// The engine follows via the forwarded command.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getPatternList()->get( 0 )
			->getIsModified() &&
			pEngine->getSong()->getIsModified();
	} ) );

	// No engine-origin SongIsModified echo may cross: the engine-side
	// apply runs under Suppress.
	TestHelper::pumpUntil( []() { return false; }, 300 );
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		CPPUNIT_ASSERT( ! ( pEvent->getType() ==
								Event::Type::SongIsModified &&
							pEvent->getOrigin() ==
								H2Core::ProcessMode::Headless ) );
	}

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The pattern-editor lock is Class C song state (ADR 0026 point 13):
// dual-apply through the engine-access handle, engine-side under
// Suppress (no SongIsModified echo). The song-level flag is asserted —
// Hydrogen::isPatternEditorLocked() additionally gates on song mode,
// which the empty song does not satisfy.
void ConnectViaIpcModeTest::testSetIsPatternEditorLockedForwardsToEngine() {
	___INFOLOG( "" );
	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto* pEngine = TestHelper::makeEngine();

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return pMirror->getSong() != nullptr; } ) );

	auto pIpcAccess = pSession->createEngineAccess();
	CPPUNIT_ASSERT( pIpcAccess != nullptr );

	const bool bInitial =
		pMirror->getSong()->getIsPatternEditorLocked();
	CPPUNIT_ASSERT(
		pEngine->getSong()->getIsPatternEditorLocked() == bInitial );

	// Drain connect-time noise before the echo check below.
	while ( pMirror->getEventQueue()->popEvent() != nullptr ) {}

	// Toggle to the opposite so the command always changes state.
	pIpcAccess->setIsPatternEditorLocked( ! bInitial );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getIsPatternEditorLocked() == ! bInitial );
	CPPUNIT_ASSERT( pMirror->getSong()->getIsModified() );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsPatternEditorLocked() == ! bInitial
			&& pEngine->getSong()->getIsModified();
	} ) );

	// And back again.
	pIpcAccess->setIsPatternEditorLocked( bInitial );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getIsPatternEditorLocked() == bInitial );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getIsPatternEditorLocked() == bInitial;
	} ) );

	// No engine-origin SongIsModified echo may cross: the engine-side
	// applies run under Suppress (the PatternEditorLocked events do
	// cross — they are the state mirror's regular feed).
	TestHelper::pumpUntil( []() { return false; }, 300 );
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pMirror->getEventQueue()->popEvent() ) != nullptr ) {
		CPPUNIT_ASSERT( ! ( pEvent->getType() ==
								Event::Type::SongIsModified &&
							pEvent->getOrigin() ==
								H2Core::ProcessMode::Headless ) );
	}

	pIpcAccess.reset();
	pSession.reset();
	pEngineSession->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}
