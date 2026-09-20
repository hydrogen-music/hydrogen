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
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
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

	// Pattern::setIsModified() only sticks for file-backed patterns (a
	// non-empty path); give pattern 0 one so its flag is observable.
	pEngine->getSong()->getPatternList()->get( 0 )->setPath(
		QString( "/tmp/h2-2s-pattern-flag-test.h2pattern" ) );

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

	// The metronome volume chosen in the editor's preferences dialog must
	// be applied by the authoritative engine — not only the editor's own
	// mirror (batch 2r): both its preferences copy and the runtime volume
	// of its metronome instrument.
	pAccess->getCoreActionController()->setMetronomeVolume( 0.25f );
	const bool bMetronomeApplied = TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->m_fMetronomeVolume == 0.25f &&
			pEngine->getAudioEngine()->getMetronomeInstrument()->getVolume()
				== 0.25f;
	} );
	CPPUNIT_ASSERT( bMetronomeApplied );

	// The save-clears-dirty flips must cross the split (batch 2s): the
	// pattern, drumkit, and playlist flags are dual-applied through the
	// engine access — MainForm's save handlers route through these
	// instead of poking the mirror's objects directly. Both directions
	// are exercised: marking and clearing.
	pAccess->setPatternModified( true, 0 );
	pAccess->setDrumkitModified( true );
	pAccess->setPlaylistIsModified( true );
	const bool bFlagsApplied = TestHelper::pumpUntil( [&]() {
		auto pEngineSong = pEngine->getSong();
		auto pEnginePlaylist = pEngine->getPlaylist();
		return pEngineSong != nullptr &&
			pEngineSong->getPatternList()->get( 0 ) != nullptr &&
			pEngineSong->getPatternList()->get( 0 )->getIsModified() &&
			pEngineSong->getDrumkit() != nullptr &&
			pEngineSong->getDrumkit()->getIsModified() &&
			pEnginePlaylist != nullptr && pEnginePlaylist->getIsModified();
	} );
	CPPUNIT_ASSERT( bFlagsApplied );

	pAccess->setPatternModified( false, 0 );
	pAccess->setDrumkitModified( false );
	pAccess->setPlaylistIsModified( false );
	const bool bFlagsCleared = TestHelper::pumpUntil( [&]() {
		auto pEngineSong = pEngine->getSong();
		auto pEnginePlaylist = pEngine->getPlaylist();
		return pEngineSong != nullptr &&
			pEngineSong->getPatternList()->get( 0 ) != nullptr &&
			! pEngineSong->getPatternList()->get( 0 )->getIsModified() &&
			pEngineSong->getDrumkit() != nullptr &&
			! pEngineSong->getDrumkit()->getIsModified() &&
			pEnginePlaylist != nullptr && ! pEnginePlaylist->getIsModified();
	} );
	CPPUNIT_ASSERT( bFlagsCleared );

	// The save command is the single writer of the song path (batch 2t):
	// saveSongAs assigns it on the authoritative engine AND on the
	// mirror (the file write itself stays engine-only), and the Keep
	// policy writes a copy while both sides keep their backing path
	// (NSM export-from-session).
	const QString sTmpAdopt = Filesystem::tmpFilePath(
		"2t-save-song-as-adopt.h2song" );
	const QString sTmpKeep = Filesystem::tmpFilePath(
		"2t-save-song-as-keep.h2song" );

	pAccess->getCoreActionController()->saveSongAs(
		sTmpAdopt, true, CoreActionController::PathPolicy::Adopt );
	const bool bAdopted = TestHelper::pumpUntil( [&]() {
		return pEngine->getSong() != nullptr &&
			pEngine->getSong()->getPath() == sTmpAdopt;
	} );
	CPPUNIT_ASSERT( bAdopted );
	// The mirror adopts synchronously in the dual-apply — no full-song
	// re-sync round-trip.
	CPPUNIT_ASSERT( pMirror->getSong()->getPath() == sTmpAdopt );
	CPPUNIT_ASSERT( Filesystem::fileExists( sTmpAdopt, true ) );

	const QString sPathBeforeKeep = pEngine->getSong()->getPath();
	pAccess->getCoreActionController()->saveSongAs(
		sTmpKeep, true, CoreActionController::PathPolicy::Keep );
	const bool bKept = TestHelper::pumpUntil( [&]() {
		return Filesystem::fileExists( sTmpKeep, true );
	} );
	CPPUNIT_ASSERT( bKept );
	CPPUNIT_ASSERT( pEngine->getSong()->getPath() == sPathBeforeKeep );
	CPPUNIT_ASSERT( pMirror->getSong()->getPath() == sPathBeforeKeep );

	Filesystem::rm( sTmpAdopt );
	Filesystem::rm( sTmpKeep );

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
	// The engine primes the attached editor's mirror with its current
	// selection state (attach-time push over the event pipeline).
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror1->getSelectedPatternNumber() == nSelectedPattern; } ) );
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
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror2->getSelectedPatternNumber() == nSelectedPattern; } ) );
	auto pAccess2 = pEditor2->createEngineAccess();
	CPPUNIT_ASSERT( pAccess2->getSelectedPatternNumber() == nSelectedPattern );

	pEditor2.reset();
	pServer->stop();
	delete pMirror2;
	delete pEngine;

	___INFOLOG( "passed" );
}
