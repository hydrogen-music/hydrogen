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

#include "IpcTransportTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcServer.h>
#include <core/IPC/PluginTelemetry.h>
#include <core/IPC/PluginTelemetryShm.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QCoreApplication>

#include <memory>

using namespace H2Core;

namespace {

int g_nNameCounter = 0;

QString uniqueServerName() {
	return QString( "h2-ipc-test-%1-%2" )
		.arg( QCoreApplication::applicationPid() )
		.arg( g_nNameCounter++ );
}

// A standalone (non-plugin) headless engine where transport/tempo commands take
// effect (plugin mode would cede tempo to the host). Caller owns it.
Hydrogen* makeStandaloneEngine() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->setOscServerEnabled( false );
	auto* pHydrogen = new Hydrogen( pPref, -1 );
	pHydrogen->setGUIState( Hydrogen::GUIState::headless );
	return pHydrogen;
}

} // namespace

void IpcTransportTest::testHelloHandshakeOverSocket() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );

	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	// Editor → engine hello, engine → editor hello reply, both over the wire.
	CPPUNIT_ASSERT( client->send( IpcMessage::hello() ) );
	IpcMessage received;
	CPPUNIT_ASSERT( conn->receive( received ) );
	CPPUNIT_ASSERT( received.getOpcode() == IpcOpcode::Hello );
	CPPUNIT_ASSERT_EQUAL( IPC_PROTOCOL_VERSION,
						  received.helloProtocolVersion() );

	CPPUNIT_ASSERT( conn->send( IpcMessage::hello() ) );
	IpcMessage reply;
	CPPUNIT_ASSERT( client->receive( reply ) );
	CPPUNIT_ASSERT_EQUAL( IPC_PROTOCOL_VERSION, reply.helloProtocolVersion() );

	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testCommandReachesEngine() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pEngine = makeStandaloneEngine();

	// Editor issues SetBpm; it travels the socket and the engine applies it.
	CPPUNIT_ASSERT( client->send(
		IpcMessage( IpcOpcode::SetBpm ).arg( 152.0f ) ) );
	IpcMessage cmd;
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SetBpm );

	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		152.0, pEngine->getAudioEngine()->getNextBpm(), 0.001 );

	delete pEngine;
	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testEventForwardingOverSocket() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	// An engine-origin event is forwarded to the editor ...
	CPPUNIT_ASSERT( IpcEngineBridge::forwardEvent(
		*conn, Event::Type::Metronome, 7, -1 ) );
	IpcMessage received;
	CPPUNIT_ASSERT( client->receive( received ) );
	Event::Type type;
	int nValue = 0;
	long nId = 0;
	CPPUNIT_ASSERT( received.toEventFields( type, nValue, nId ) );
	CPPUNIT_ASSERT( type == Event::Type::Metronome );
	CPPUNIT_ASSERT_EQUAL( 7, nValue );

	// ... while an editor-internal event is NOT marshalled (nothing arrives).
	CPPUNIT_ASSERT( ! IpcEngineBridge::forwardEvent(
		*conn, Event::Type::OnlineImportProgress, 50, -1 ) );
	IpcMessage shouldNotArrive;
	CPPUNIT_ASSERT( ! client->receive( shouldNotArrive, 200 ) );

	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testTelemetrySharedMemory() {
	___INFOLOG( "" );

	const QString sKey = QString( "h2-telemetry-test-%1" )
		.arg( QCoreApplication::applicationPid() );

	PluginTelemetryShm writer;
	CPPUNIT_ASSERT( writer.create( sKey ) );

	PluginTelemetrySnapshot in;
	in.frame = 987654;
	in.bar = 4; in.beat = 2; in.tick = 11;
	in.bpm = 124.0f;
	in.playing = 1;
	in.masterPeakL = 0.6f; in.masterPeakR = 0.4f;
	in.instPeakCount = 2;
	in.peakL[1] = 0.33f;
	CPPUNIT_ASSERT( writer.store( in ) );

	// A second attachment (the editor side) reads a consistent snapshot.
	PluginTelemetryShm reader;
	CPPUNIT_ASSERT( reader.attach( sKey ) );

	PluginTelemetrySnapshot out;
	CPPUNIT_ASSERT( reader.load( out ) );
	CPPUNIT_ASSERT_EQUAL( in.frame, out.frame );
	CPPUNIT_ASSERT_EQUAL( in.bar, out.bar );
	CPPUNIT_ASSERT_EQUAL( in.bpm, out.bpm );
	CPPUNIT_ASSERT_EQUAL( in.instPeakCount, out.instPeakCount );
	CPPUNIT_ASSERT_EQUAL( in.peakL[1], out.peakL[1] );

	reader.detach();
	writer.detach();

	___INFOLOG( "passed" );
}
