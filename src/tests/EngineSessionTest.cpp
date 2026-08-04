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

#include "EngineSessionTest.h"

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <cmath>
#include <functional>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>

using namespace H2Core;

// Invalid arguments are rejected synchronously (nullptr), so a host can react
// instead of believing it is serving. (A name collision is NOT a failure:
// IpcServer::listen clears a stale socket from a crashed run and re-binds.)
void EngineSessionTest::testRejectsInvalidArguments() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();

	CPPUNIT_ASSERT(
		EngineSession::start( nullptr, TestHelper::uniqueEndpoint() ) == nullptr
	);
	CPPUNIT_ASSERT( EngineSession::start( pEngine, QString() ) == nullptr );

	// A valid pair does start and serve.
	auto pSession =
		EngineSession::start( pEngine, TestHelper::uniqueEndpoint() );
	CPPUNIT_ASSERT( pSession != nullptr );
	CPPUNIT_ASSERT( pSession->isRunning() );
	pSession->stop();

	delete pEngine;

	___INFOLOG( "passed" );
}

// A command issued on the editor side reaches the engine and is applied there.
void EngineSessionTest::testCommandDispatchedToEngine() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	pMirror->setSong( Song::getEmptySong( pMirror ) );
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	auto pAccess = pEditor->createEngineAccess();
	CPPUNIT_ASSERT( pAccess != nullptr );
	CPPUNIT_ASSERT( pAccess->getCoreActionController() != nullptr );

	// setBpm flows editor → engine; the bridge thread dispatches it onto the
	// authoritative engine.
	pAccess->getCoreActionController()->setBpm( 152.0f );
	const bool bApplied = TestHelper::pumpUntil( [&]() {
		return std::abs( pEngine->getAudioEngine()->getNextBpm() - 152.0 ) < 0.5;
	} );
	CPPUNIT_ASSERT( bApplied );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// An engine-origin event is drained off the engine's EventQueue by the bridge
// thread and re-posted onto the editor's mirror queue.
void EngineSessionTest::testEventForwardedToEditor() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	// Let the initial handshake/state settle first.
	TestHelper::pumpUntil( [&]() { return pMirror->getSong() != nullptr; } );

	// The engine emits an event; it must surface on the editor's mirror.
	pEngine->getEventQueue()->pushEvent( Event::Type::Metronome, 7 );
	CPPUNIT_ASSERT(
		TestHelper::pumpUntilEvent( pMirror, Event::Type::Metronome, 7 )
	);

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// The engine keeps serving after the editor disconnects: a respawned editor
// re-attaches and is primed again.
void EngineSessionTest::testEngineSurvivesEditorReconnect() {
	___INFOLOG( "" );

	const int nSelectedPattern = 4;
	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );
	pEngine->setSelectedPatternNumber( nSelectedPattern );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	// First editor attaches and gets the song.
	auto* pMirror1 = TestHelper::makeMirror();
	CPPUNIT_ASSERT( pMirror1->getSelectedPatternNumber() != nSelectedPattern );
	auto pEditor1 = EditorSession::connect( sEndpoint, pMirror1 );
	CPPUNIT_ASSERT( pEditor1 != nullptr );
	auto pAccess1 = pEditor1->createEngineAccess();
	CPPUNIT_ASSERT( pAccess1->getSelectedPatternNumber() == nSelectedPattern );

	// Editor goes away (crash/exit).
	pEditor1.reset();
	delete pMirror1;
	CPPUNIT_ASSERT( pServer->isRunning() );

	// A respawned editor re-attaches and is primed anew.
	auto* pMirror2 = TestHelper::makeMirror();
	CPPUNIT_ASSERT( pMirror2->getSelectedPatternNumber() != nSelectedPattern );
	auto pEditor2 = EditorSession::connect( sEndpoint, pMirror2, 5000 );
	CPPUNIT_ASSERT( pEditor2 != nullptr );
	auto pAccess2 = pEditor2->createEngineAccess();
	CPPUNIT_ASSERT( pAccess2->getSelectedPatternNumber() == nSelectedPattern );

	pEditor2.reset();
	pServer->stop();
	delete pMirror2;
	delete pEngine;

	___INFOLOG( "passed" );
}
