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

#include "TestHelper.h"

#include <core/Basics/Event.h>
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

void EditorMirrorTest::testEventSyncsToMirror() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
	EditorStateMirror mirror( pMirror );

	// An engine-origin event arrives over IPC and is re-posted onto the mirror's
	// EventQueue, where the GUI would pick it up.
	CPPUNIT_ASSERT( mirror.applyEvent(
		IpcMessage::fromEvent( Event::Type::Metronome, 7, -1 ) ) );

	std::unique_ptr<Event> pEvent = pMirror->getEventQueue()->popEvent();
	CPPUNIT_ASSERT( pEvent != nullptr );
	CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Metronome );
	CPPUNIT_ASSERT_EQUAL( 7, pEvent->getValue() );

	delete pMirror;

	___INFOLOG( "passed" );
}

void EditorMirrorTest::testEngineAccessReadsMirror() {
	___INFOLOG( "" );

	auto* pMirror = TestHelper::makeMirror();
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
	CPPUNIT_ASSERT( server.listen( TestHelper::uniqueEndpoint() ) );
	IpcChannel* client = IpcChannel::connectToServer( server.serverName() );
	CPPUNIT_ASSERT( client != nullptr );
	IpcChannel* conn = server.waitForChannel();
	CPPUNIT_ASSERT( conn != nullptr );

	auto* pMirror = TestHelper::makeMirror();
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
