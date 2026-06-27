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

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IO/SoftwareDriver.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QCoreApplication>

#include <memory>

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
