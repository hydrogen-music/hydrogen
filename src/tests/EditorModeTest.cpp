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

#include "EditorModeTest.h"

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
#include <core/IPC/PluginTelemetry.h>
#include <core/IPC/PluginTelemetryShm.h>
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
	pHydrogen->setGUIState( Hydrogen::GUIState::headless );
	return pHydrogen;
}

} // namespace

// --plugin-editor <endpoint>: the editor attaches to the engine's control socket
// and announces itself with a hello (ADR 0016/0018).
void EditorModeTest::testAttachesToEngineEndpoint() {
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
void EditorModeTest::testFailedConnectionReported() {
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
// transport). Crash-safety on the --plugin-editor abort no longer relies on
// avoiding a thread: the driver's clock thread is joined in its destructor and
// the engine is torn down before the Logger.
void EditorModeTest::testMirrorUsesHeadlessDriver() {
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
void EditorModeTest::testReceivesEngineState() {
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
void EditorModeTest::testIssuesCommands() {
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
void EditorModeTest::testEngineSurvivesEditorDisconnect() {
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
void EditorModeTest::testEngineBuildsTransportSnapshot() {
	___INFOLOG( "" );

	auto* pEngine = makeMirrorEngine(); // a headless engine stands in for the host
	auto snapshot = EngineSession::buildTransportSnapshot( pEngine );
	CPPUNIT_ASSERT( snapshot.playing == 0 );
	CPPUNIT_ASSERT( snapshot.bpm > 0.0f );

	const QString sEndpoint = uniqueServerName();
	const QString sKey = PluginTelemetryShm::keyForEndpoint( sEndpoint );
	// Same endpoint → same key on both sides (no separate negotiation).
	CPPUNIT_ASSERT_EQUAL(
		sKey.toStdString(),
		PluginTelemetryShm::keyForEndpoint( sEndpoint ).toStdString() );

	PluginTelemetryShm writer;
	CPPUNIT_ASSERT( writer.create( sKey ) );
	CPPUNIT_ASSERT( writer.store( snapshot ) );

	PluginTelemetryShm reader;
	CPPUNIT_ASSERT( reader.attach( sKey ) );
	PluginTelemetrySnapshot out;
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
void EditorModeTest::testMirrorFollowsTransportTelemetry() {
	___INFOLOG( "" );

	auto* pMirror = makeMirrorEngine();
	EditorStateMirror mirror( pMirror );
	auto pAudioEngine = pMirror->getAudioEngine();

	// --- Play + tempo follow: host starts rolling at a new tempo. ---
	PluginTelemetrySnapshot rolling;
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
	PluginTelemetrySnapshot stopped;
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
	PluginTelemetrySnapshot atTarget;
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
