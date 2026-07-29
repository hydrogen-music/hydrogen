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

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IO/SoftwareDriver.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EditorStateMirror.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcEngineBridge.h>
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

namespace {

int g_nNameCounter = 0;

QString uniqueServerName() {
	return QString( "h2-editor-mode-test-%1-%2" )
		.arg( QCoreApplication::applicationPid() )
		.arg( g_nNameCounter++ );
}

// A standalone headless engine standing in for the editor-side mirror that the
// GUI would read from. Caller owns it.
Hydrogen* makeMirrorEngine() {
	auto pPref = Preferences::create_instance();
	// Same headless-mirror configuration main()'s editor branch uses (passive
	// Null audio driver — no processing thread, no MIDI, no OSC).
	EditorSession::configureMirrorPreferences( pPref );
	auto* pHydrogen = new Hydrogen( pPref, -1 );
	pHydrogen->setProcessMode( Hydrogen::ProcessMode::Headless );
	return pHydrogen;
}

} // namespace

// --connect-via-ipc <endpoint>: the editor attaches to the engine's control socket
// and announces itself with a hello (ADR 0016/0018).
void ConnectViaIpcModeTest::testAttachesToEngineEndpoint() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );

	auto* pMirror = makeMirrorEngine();
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

	auto* pMirror = makeMirrorEngine();
	// No server listening on this name → connection must fail fast.
	auto pSession = EditorSession::connect(
		uniqueServerName(), pMirror, 200 /*ms*/ );
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

	auto* pMirror = makeMirrorEngine();
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

// Inbound engine state (song snapshot + events) is applied to the mirror, where
// the GUI would read it locally.
void ConnectViaIpcModeTest::testReceivesEngineState() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );

	auto* pMirror = makeMirrorEngine();
	auto pSession = EditorSession::connect( server.serverName(), pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	// The engine pushes a fresh song snapshot; the editor reconstructs it onto
	// the mirror as a genuinely new object.
	auto pOldSong = pMirror->getSong();
	CPPUNIT_ASSERT( pOldSong != nullptr );
	pOldSong->setName( "EDITORMODE" );
	IpcMessage snapshot( IpcOpcode::SetSong );
	snapshot.setPayload( pOldSong->toXmlBuffer() );
	CPPUNIT_ASSERT( conn->send( snapshot ) );

	// The engine also forwards an event.
	CPPUNIT_ASSERT( IpcEngineBridge::forwardEvent(
		*conn, Event::Type::Metronome, 7, -1 ) );

	// Drive the editor channel: receive() pumps the socket, which emits
	// messageReceived → EditorStateMirror applies each frame to the mirror.
	IpcMessage drained;
	CPPUNIT_ASSERT( pSession->getChannel()->receive( drained ) ); // song snapshot
	CPPUNIT_ASSERT( pSession->getChannel()->receive( drained ) ); // event

	auto pNewSong = pMirror->getSong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	CPPUNIT_ASSERT( pNewSong != pOldSong );
	CPPUNIT_ASSERT_EQUAL( std::string( "EDITORMODE" ),
						  pNewSong->getName().toStdString() );

	std::unique_ptr<Event> pEvent = pMirror->getEventQueue()->popEvent();
	CPPUNIT_ASSERT( pEvent != nullptr );
	CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Metronome );
	CPPUNIT_ASSERT_EQUAL( 7, pEvent->getValue() );

	pSession.reset();
	delete pMirror;

	___INFOLOG( "passed" );
}

// The GUI's engine-access handle forwards commands over the channel to the
// engine (here, the server end).
void ConnectViaIpcModeTest::testIssuesCommands() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );

	auto* pMirror = makeMirrorEngine();
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
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );

	auto* pMirror = makeMirrorEngine();
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
	auto* pMirror2 = makeMirrorEngine();
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

	auto* pEngine = makeMirrorEngine(); // a headless engine stands in for the host
	auto snapshot = EngineSession::buildTransportSnapshot( pEngine );
	CPPUNIT_ASSERT( snapshot.playing == 0 );
	CPPUNIT_ASSERT( snapshot.bpm > 0.0f );

	const QString sEndpoint = uniqueServerName();
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

	auto* pMirror = makeMirrorEngine();
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

// When the IPC connection is lost, HydrogenApp::onIpcConnectionLost() drops the
// IpcEngineAccess and pEngine() falls back to a LocalEngineAccess wrapping the
// same mirror engine (ADR 0016). This test verifies that core mechanism: after
// destroying the IPC handle, a LocalEngineAccess on the same mirror serves
// reads and commands without crashing — the GUI stays responsive in degraded
// mode (no audio, mirror-only commands) until the user reconnects.
void ConnectViaIpcModeTest::testEngineAccessFallsBackToLocal() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );

	auto* pMirror = makeMirrorEngine();
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

	const QString sEndpoint = uniqueServerName();

	// Engine side: a headless engine with known state, served via the
	// production EngineSession serve loop on its own QThread.
	auto* pEngine = makeMirrorEngine();
	auto pSong = pEngine->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	pSong->setName( "SYNC_TEST" );
	pEngine->setSelectedPatternNumber( 2 );
	pEngine->setSelectedInstrumentNumber( 1 );
	pEngine->setRecordEnabled( true );

	auto pEngineSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pEngineSession != nullptr );

	// Editor side: a fresh mirror with default state.
	auto* pMirror = makeMirrorEngine();
	CPPUNIT_ASSERT( pMirror->getSong() != nullptr );
	CPPUNIT_ASSERT( pMirror->getSong()->getName().toStdString() != "SYNC_TEST" );

	auto pSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( pSession->isConnected() );

	auto pChannel = pSession->getChannel();
	CPPUNIT_ASSERT( pChannel != nullptr );

	// EngineSession::serve() sends the initial song snapshot on connect; drain
	// it so it doesn't sit in the pending queue ahead of our replies.
	IpcMessage initialState;
	pChannel->receive( initialState, 500, false );

	// 1. GetSong — reply carries the song XML payload.
	{
		IpcMessage reply;
		CPPUNIT_ASSERT( pChannel->request(
			IpcMessage( IpcOpcode::GetSong ), reply, 3000 ) );
		CPPUNIT_ASSERT( ! reply.getPayload().isEmpty() );

		// Apply to the mirror via the state mirror.
		IpcMessage setSongMsg( IpcOpcode::SetSong );
		setSongMsg.setPayload( reply.getPayload() );
		pSession->getStateMirror()->applyMessage( setSongMsg );

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
