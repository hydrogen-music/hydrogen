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

#include "EditorMirrorTest.h"

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/EditorStateMirror.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcServer.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QCoreApplication>

#include <memory>

using namespace H2Core;

namespace {

int g_nNameCounter = 0;

QString uniqueServerName() {
	return QString( "h2-mirror-test-%1-%2" )
		.arg( QCoreApplication::applicationPid() )
		.arg( g_nNameCounter++ );
}

// A standalone headless engine standing in for the editor-side mirror. Caller
// owns it.
Hydrogen* makeMirrorEngine() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->setOscServerEnabled( false );
	auto* pHydrogen = new Hydrogen( pPref, -1 );
	pHydrogen->setProcessMode( Hydrogen::ProcessMode::Headless );
	return pHydrogen;
}

} // namespace

void EditorMirrorTest::testEventSyncsToMirror() {
	___INFOLOG( "" );

	auto* pMirror = makeMirrorEngine();
	EditorStateMirror mirror( pMirror );

	// An engine-origin event arrives over IPC and is re-posted onto the mirror's
	// EventQueue, where the GUI would pick it up.
	CPPUNIT_ASSERT( mirror.applyMessage(
		IpcMessage::fromEvent( Event::Type::Metronome, 7, -1 ) ) );

	std::unique_ptr<Event> pEvent = pMirror->getEventQueue()->popEvent();
	CPPUNIT_ASSERT( pEvent != nullptr );
	CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Metronome );
	CPPUNIT_ASSERT_EQUAL( 7, pEvent->getValue() );

	delete pMirror;

	___INFOLOG( "passed" );
}

void EditorMirrorTest::testSongSnapshotSyncsToMirror() {
	___INFOLOG( "" );

	auto* pMirror = makeMirrorEngine();
	EditorStateMirror mirror( pMirror );

	auto pOldSong = pMirror->getSong();
	CPPUNIT_ASSERT( pOldSong != nullptr );

	// The engine sends a fresh song snapshot; encode one with a marker name.
	pOldSong->setName( "MIRRORED" );
	const QByteArray xml = pOldSong->toXmlBuffer();
	IpcMessage snapshot( IpcOpcode::SetSong );
	snapshot.setPayload( xml );

	CPPUNIT_ASSERT( mirror.applyMessage( snapshot ) );

	auto pNewSong = pMirror->getSong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	// A genuinely new song object reconstructed from the wire, not the original.
	CPPUNIT_ASSERT( pNewSong != pOldSong );
	CPPUNIT_ASSERT_EQUAL( std::string( "MIRRORED" ),
						  pNewSong->getName().toStdString() );

	delete pMirror;

	___INFOLOG( "passed" );
}

void EditorMirrorTest::testEngineAccessReadsMirror() {
	___INFOLOG( "" );

	auto* pMirror = makeMirrorEngine();
	IpcEngineAccess access( pMirror, nullptr );

	// Reads resolve to the local mirror, so the GUI sees live local objects.
	CPPUNIT_ASSERT( access.getSong() == pMirror->getSong() );
	CPPUNIT_ASSERT( access.getAudioEngine() == pMirror->getAudioEngine() );
	CPPUNIT_ASSERT( access.getEventQueue() == pMirror->getEventQueue() );
	CPPUNIT_ASSERT( access.getMode() == pMirror->getMode() );
	CPPUNIT_ASSERT( access.getPreferences() == pMirror->getPreferences() );

	delete pMirror;

	___INFOLOG( "passed" );
}

void EditorMirrorTest::testCommandForwardedOverIpc() {
	___INFOLOG( "" );

	IpcServer server;
	CPPUNIT_ASSERT( server.listen( uniqueServerName() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = makeMirrorEngine();
	IpcEngineAccess access( pMirror, client );

	// A transport command issued on the editor side travels the socket to the
	// engine (here, the server end) ...
	access.sequencerStop();
	IpcMessage cmd;
	CPPUNIT_ASSERT( conn->receive( cmd ) );
	CPPUNIT_ASSERT( cmd.getOpcode() == IpcOpcode::Stop );

	delete pMirror;
	delete client;

	___INFOLOG( "passed" );
}
