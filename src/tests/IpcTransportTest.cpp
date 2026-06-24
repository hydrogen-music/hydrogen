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
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcCoreActionController.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcChannel.h>
#include <core/Midi/Midi.h>
#include <core/IPC/IpcServer.h>
#include <core/IPC/PluginTelemetry.h>
#include <core/IPC/PluginTelemetryShm.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <thread>

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

void IpcTransportTest::testProxyCommandReachesEngine() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();   // editor-side read mirror
	auto* pEngine = makeStandaloneEngine();   // authoritative engine

	// The GUI calls the CoreActionController surface; in editor mode that is the
	// IPC proxy (ADR 0030). One call must (a) reflect on the local mirror and
	// (b) forward to the authoritative engine.
	IpcCoreActionController proxy( pMirror, client );
	CPPUNIT_ASSERT( proxy.setBpm( 152.0f ) );

	// (a) local mirror updated for snappy UI
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		152.0, pMirror->getAudioEngine()->getNextBpm(), 0.001 );

	// (b) the SetBpm opcode travelled the socket; the bridge applies it to the
	// authoritative engine.
	IpcMessage cmd;
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SetBpm );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		152.0, pEngine->getAudioEngine()->getNextBpm(), 0.001 );

	delete pEngine;
	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testProxyMarshalsParameterCommands() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();
	IpcCoreActionController proxy( pMirror, client );

	// A 2-arg instrument setter marshals opcode + args over the socket.
	proxy.setInstrumentGain( 3, 0.75f );
	IpcMessage m1;
	CPPUNIT_ASSERT( conn->receive( m1 ) );
	CPPUNIT_ASSERT( m1.getOpcode() == IpcOpcode::SetInstrumentGain );
	CPPUNIT_ASSERT_EQUAL( 3, m1.getArgs()[0].toInt() );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.75, m1.getArgs()[1].toDouble(), 0.001 );

	// A 4-arg layer setter too.
	proxy.setLayerGain( 1, 2, 0, 0.5f );
	IpcMessage m2;
	CPPUNIT_ASSERT( conn->receive( m2 ) );
	CPPUNIT_ASSERT( m2.getOpcode() == IpcOpcode::SetLayerGain );
	CPPUNIT_ASSERT_EQUAL( 4, static_cast<int>( m2.getArgs().size() ) );
	CPPUNIT_ASSERT_EQUAL( 2, m2.getArgs()[1].toInt() );

	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testRequestResponseRoundTrip() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	// Engine-side responder: read the request and answer with a Reply that echoes
	// the request id and carries a computed result (ADR 0030 tier 3).
	std::thread responder( [conn]() {
		IpcMessage req;
		if ( conn->receive( req, 5000 ) && req.getRequestId() != 0 ) {
			IpcMessage reply( IpcOpcode::Reply );
			reply.setRequestId( req.getRequestId() );
			reply.arg( req.getArgs()[0].toInt() * 2 );
			conn->send( reply );
		}
	} );

	// Editor side: blocking request() correlates the reply by id.
	IpcMessage reply;
	const bool bOk = client->request(
		IpcMessage( IpcOpcode::LocateToColumn ).arg( 21 ), reply, 5000 );
	responder.join();

	CPPUNIT_ASSERT( bOk );
	CPPUNIT_ASSERT( reply.getOpcode() == IpcOpcode::Reply );
	CPPUNIT_ASSERT( reply.getRequestId() != 0 );      // id survived the wire
	CPPUNIT_ASSERT_EQUAL( 42, reply.getArgs()[0].toInt() );

	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testProxyMidiOutRequestResponse() {
	___INFOLOG( "" );

	// (A) Bridge handleRequest produces a Reply echoing the request id + an arg.
	auto* pEngine = makeStandaloneEngine();
	IpcMessage req( IpcOpcode::SetInstrumentMidiOutNote );
	req.setRequestId( 5 );
	req.arg( 0 ).arg( static_cast<int>( Midi::noteFromIntClamp( 60 ) ) );
	const IpcMessage reply = IpcEngineBridge::handleRequest( req, pEngine );
	CPPUNIT_ASSERT( reply.getOpcode() == IpcOpcode::Reply );
	CPPUNIT_ASSERT_EQUAL( static_cast<quint32>( 5 ), reply.getRequestId() );
	CPPUNIT_ASSERT_EQUAL( 1, static_cast<int>( reply.getArgs().size() ) );
	delete pEngine;

	// (B) The proxy issues a request when the caller needs the engine's event id
	// and fills it from the reply (ADR 0030 tier 3).
	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();
	std::thread responder( [conn]() {
		IpcMessage r;
		if ( conn->receive( r, 5000 ) &&
			 r.getOpcode() == IpcOpcode::SetInstrumentMidiOutNote &&
			 r.getRequestId() != 0 ) {
			IpcMessage rep( IpcOpcode::Reply );
			rep.setRequestId( r.getRequestId() );
			rep.arg( static_cast<qlonglong>( 909 ) );
			conn->send( rep );
		}
	} );

	IpcCoreActionController proxy( pMirror, client );
	long nEventId = -123; // sentinel: must be overwritten by the engine's reply
	proxy.setInstrumentMidiOutNote( 0, Midi::noteFromIntClamp( 60 ), &nEventId );
	responder.join();

	CPPUNIT_ASSERT_EQUAL( static_cast<long>( 909 ), nEventId );

	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testProxySetSongPayload() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();
	auto* pEngine = makeStandaloneEngine();

	auto pSong = Song::getEmptySong( pMirror );
	CPPUNIT_ASSERT( pSong != nullptr );
	pSong->setName( "IPCPAYLOAD" );

	// The proxy serialises the song into the SetSong payload and forwards it; the
	// engine reconstructs and applies it (ADR 0030 object-payload).
	IpcCoreActionController proxy( pMirror, client );
	proxy.setSong( pSong );

	IpcMessage cmd;
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SetSong );
	CPPUNIT_ASSERT( ! cmd.getPayload().isEmpty() );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT( pEngine->getSong() != nullptr );
	CPPUNIT_ASSERT( pEngine->getSong()->getName() == QString( "IPCPAYLOAD" ) );

	delete pEngine;
	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2f: the object-payload commands (setDrumkit / setPattern /
// replaceInstrument) serialise their object into the message payload; the engine
// reconstructs it via the XML-buffer serialisers and applies it.
void IpcTransportTest::testProxyObjectPayloadCommands() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();
	auto* pEngine = makeStandaloneEngine();
	// Both ends start from an empty song so they share a drumkit context.
	pMirror->setSong( Song::getEmptySong( pMirror ) );
	pEngine->setSong( Song::getEmptySong( pEngine ) );
	CPPUNIT_ASSERT( pMirror->getSong() != nullptr );
	CPPUNIT_ASSERT( pEngine->getSong() != nullptr );

	IpcCoreActionController proxy( pMirror, client );
	IpcMessage cmd;

	// --- setDrumkit: drumkit XML payload, reconstructed engine-side ---
	auto pKit = pMirror->getSong()->getDrumkit();
	const int nKitInstruments = pKit->getInstruments()->size();
	CPPUNIT_ASSERT( nKitInstruments > 0 );
	proxy.setDrumkit( pKit );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SetDrumkit );
	CPPUNIT_ASSERT( ! cmd.getPayload().isEmpty() );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT_EQUAL(
		nKitInstruments,
		pEngine->getSong()->getDrumkit()->getInstruments()->size() );

	// --- setPattern: pattern XML payload + (number, replace) args ---
	auto pPattern = std::make_shared<Pattern>();
	pPattern->setName( "IPCPAT" );
	const int nPatternsBefore = pEngine->getSong()->getPatternList()->size();
	proxy.setPattern( pPattern, 0, false );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SetPattern );
	CPPUNIT_ASSERT( ! cmd.getPayload().isEmpty() );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT(
		pEngine->getSong()->getPatternList()->size() > nPatternsBefore );

	// --- replaceInstrument: new instrument payload + old id arg ---
	auto pOldMirror = pMirror->getSong()->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pOldMirror != nullptr );
	auto pNew = std::make_shared<Instrument>( pOldMirror ); // copy (same id)
	proxy.replaceInstrument( pNew, pOldMirror );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::ReplaceInstrument );
	CPPUNIT_ASSERT( ! cmd.getPayload().isEmpty() );
	CPPUNIT_ASSERT_EQUAL( static_cast<int>( pOldMirror->getId() ),
						  cmd.getArgs()[0].toInt() );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );

	// --- saveSong: engine-only file op (no mirror write) ---
	proxy.saveSongAs( "/tmp/ipc-save-test.h2song", true );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SaveSongAs );
	CPPUNIT_ASSERT_EQUAL( std::string( "/tmp/ipc-save-test.h2song" ),
						  cmd.getArgs()[0].toString().toStdString() );

	delete pEngine;
	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2f: addInstrument is request/response when the caller needs the
// engine-assigned event id; the proxy fills it from the Reply.
void IpcTransportTest::testProxyAddInstrumentRequestResponse() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();
	pMirror->setSong( Song::getEmptySong( pMirror ) );
	auto pInstrument = std::make_shared<Instrument>(
		pMirror->getSong()->getDrumkit()->getInstruments()->get( 0 ) );

	std::thread responder( [conn]() {
		IpcMessage r;
		if ( conn->receive( r, 5000 ) &&
			 r.getOpcode() == IpcOpcode::AddInstrument &&
			 r.getRequestId() != 0 ) {
			IpcMessage rep( IpcOpcode::Reply );
			rep.setRequestId( r.getRequestId() );
			rep.arg( static_cast<qlonglong>( 4242 ) );
			conn->send( rep );
		}
	} );

	IpcCoreActionController proxy( pMirror, client );
	long nEventId = -123; // sentinel: must be overwritten by the engine's reply
	proxy.addInstrument( pInstrument, -1, &nEventId );
	responder.join();

	CPPUNIT_ASSERT_EQUAL( static_cast<long>( 4242 ), nEventId );

	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2g: setPlaylist marshals the playlist XML payload;
// add/removeFromPlaylist marshal a single entry as mime text + index.
void IpcTransportTest::testProxyPlaylistCommands() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeStandaloneEngine();
	auto* pEngine = makeStandaloneEngine();

	IpcCoreActionController proxy( pMirror, client );
	IpcMessage cmd;

	// --- setPlaylist: playlist XML payload, reconstructed engine-side ---
	auto pPlaylist = std::make_shared<Playlist>();
	CPPUNIT_ASSERT( pPlaylist->add(
		std::make_shared<PlaylistEntry>( "/tmp/song1.h2song", "", false ) ) );
	proxy.setPlaylist( pPlaylist );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::SetPlaylist );
	CPPUNIT_ASSERT( ! cmd.getPayload().isEmpty() );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT( pEngine->getPlaylist() != nullptr );
	CPPUNIT_ASSERT_EQUAL( 1, pEngine->getPlaylist()->size() );

	// --- addToPlaylist: entry mime text + index ---
	auto pEntry = std::make_shared<PlaylistEntry>( "/tmp/song2.h2song", "", false );
	proxy.addToPlaylist( pEntry, -1 );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::AddToPlaylist );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT_EQUAL( 2, pEngine->getPlaylist()->size() );

	// --- removeFromPlaylist: same entry (matched by value) ---
	proxy.removeFromPlaylist( pEntry, -1 );
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::RemoveFromPlaylist );
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );
	CPPUNIT_ASSERT_EQUAL( 1, pEngine->getPlaylist()->size() );

	delete pEngine;
	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}

void IpcTransportTest::testRescanCommandReachesEngine() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pEngine = makeStandaloneEngine();

	// After an editor-side install, the editor asks the engine to refresh its
	// SoundLibraryDatabase (ADR 0016) so newly installed kits become loadable.
	CPPUNIT_ASSERT( client->send( IpcMessage( IpcOpcode::RescanSoundLibrary ) ) );
	IpcMessage cmd;
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::RescanSoundLibrary );

	// The engine dispatches the rescan onto its database.
	CPPUNIT_ASSERT( IpcEngineBridge::dispatchCommand( cmd, pEngine ) );

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
