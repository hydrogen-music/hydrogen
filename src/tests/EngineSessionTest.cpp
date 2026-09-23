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
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Timeline.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EditorStateMirror.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcMessage.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
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
	// tmpFilePath() reserves the name by leaving an empty file behind;
	// removing it lets file existence below observe the save's actual
	// QSaveFile commit instead of the reservation.
	const QString sTmpAdopt = Filesystem::tmpFilePath(
		"2t-save-song-as-adopt.h2song" );
	Filesystem::rm( sTmpAdopt );
	const QString sTmpKeep = Filesystem::tmpFilePath(
		"2t-save-song-as-keep.h2song" );
	Filesystem::rm( sTmpKeep );

	pAccess->getCoreActionController()->saveSongAs(
		sTmpAdopt, true, CoreActionController::PathPolicy::Adopt );
	// The engine assigns the new path before writing the file
	// (CoreActionController::saveSongAs), so the path flip alone is an
	// intermediate state — the file landing is the settled one.
	const bool bAdopted = TestHelper::pumpUntil( [&]() {
		return pEngine->getSong() != nullptr &&
			pEngine->getSong()->getPath() == sTmpAdopt &&
			Filesystem::fileExists( sTmpAdopt, true );
	} );
	CPPUNIT_ASSERT( bAdopted );
	// The mirror adopts synchronously in the dual-apply — no full-song
	// re-sync round-trip.
	CPPUNIT_ASSERT( pMirror->getSong()->getPath() == sTmpAdopt );
	CPPUNIT_ASSERT( Filesystem::fileExists( sTmpAdopt, true ) );

	const QString sPathBeforeKeep = pEngine->getSong()->getPath();
	pAccess->getCoreActionController()->saveSongAs(
		sTmpKeep, true, CoreActionController::PathPolicy::Keep );
	// Keep points the song at the export target for the whole engine-side
	// save and restores the backing path only afterwards — the settled
	// state is the file landed AND the backing path back in place.
	const bool bKept = TestHelper::pumpUntil( [&]() {
		return Filesystem::fileExists( sTmpKeep, true ) &&
			pEngine->getSong()->getPath() == sPathBeforeKeep;
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

// Sound library rescans must cross the split (batch 2u): GUI-side library
// mutations (import, delete, save-to-library) rescan the editor's mirror
// database — but the authoritative engine owns one too and resolves
// songs/patterns through it, so the rescan has to reach it as well.
void EngineSessionTest::testSoundLibraryRescanCrossesSplit() {
	___INFOLOG( "" );

	// One artifact of each type lands in the user-level library dirs —
	// behind the back of both databases (they were built at construction
	// time). Unique names keep the run immune to leftovers of earlier runs.
	const QString sTestDataDir = TestHelper::get_instance()->getTestDataDir();
	// The user-level library dirs come back with a trailing separator;
	// cleanPath keeps the probe keys identical to the ones the database
	// scans register (a stray "//" would never match a map key).
	const QString sKitDir = QDir::cleanPath(
		Filesystem::userDrumkitsDir() + "/" +
		QString( "2u-crossing-kit-%1" ).arg(
			QCoreApplication::applicationPid() ) );
	const QString sPatternPath = QDir::cleanPath(
		Filesystem::userPatternsDir() + "/2u-crossing-pattern.h2pattern" );
	const QString sSongPath = QDir::cleanPath(
		Filesystem::userSongsDir() + "/2u-crossing-song.h2song" );

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

	// Now the artifacts land — behind the back of both databases, which
	// were built at construction time above.
	CPPUNIT_ASSERT( QDir().mkpath( sKitDir ) );
	for ( const auto& sFile : QDir( sTestDataDir + "/drumkits/baseKit" )
			  .entryList( QDir::Files ) ) {
		CPPUNIT_ASSERT( QFile::copy( sTestDataDir + "/drumkits/baseKit/" +
									 sFile, sKitDir + "/" + sFile ) );
	}
	CPPUNIT_ASSERT( QFile::copy( sTestDataDir + "/pattern/pattern.h2pattern",
								 sPatternPath ) );
	CPPUNIT_ASSERT( QFile::copy( sTestDataDir + "/song/AE_songSizeChanged.h2song",
								 sSongPath ) );

	// NB: map membership, not getDrumkit() — that one loads a missing kit
	// from file and inserts it (session-drumkit fallback), so it can never
	// report "unknown" for an on-disk kit and would mutate the db under
	// the probe.
	const auto fDrumkitKnown = [&]( H2Core::Hydrogen* pH ) {
		return pH->getSoundLibraryDatabase()->getDrumkitDatabase().count(
			Filesystem::drumkitPathFromDir( sKitDir ) ) > 0; };
	const auto fPatternKnown = [&]( H2Core::Hydrogen* pH ) {
		const auto& infos = pH->getSoundLibraryDatabase()->getPatternInfos();
		return std::any_of( infos.begin(), infos.end(),
			[&]( const std::shared_ptr<SoundLibraryInfo>& pInfo ) {
				return pInfo->getPath() == sPatternPath; } ); };
	const auto fSongKnown = [&]( H2Core::Hydrogen* pH ) {
		const auto& infos = pH->getSoundLibraryDatabase()->getSongInfos();
		return std::any_of( infos.begin(), infos.end(),
			[&]( const std::shared_ptr<SoundLibraryInfo>& pInfo ) {
				return pInfo->getPath() == sSongPath; } ); };

	// Neither database has rescanned since the artifacts landed.
	CPPUNIT_ASSERT( ! fDrumkitKnown( pEngine ) );
	CPPUNIT_ASSERT( ! fPatternKnown( pEngine ) );
	CPPUNIT_ASSERT( ! fSongKnown( pEngine ) );
	CPPUNIT_ASSERT( ! fDrumkitKnown( pMirror ) );
	CPPUNIT_ASSERT( ! fPatternKnown( pMirror ) );
	CPPUNIT_ASSERT( ! fSongKnown( pMirror ) );

	// The per-type rescan crosses: the engine applies it via the bridge
	// thread (pumped), the mirror synchronously in the dual-apply.
	pAccess->updateSoundLibrary( SoundLibraryInfo::Type::Drumkit );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fDrumkitKnown( pEngine ); } ) );
	CPPUNIT_ASSERT( fDrumkitKnown( pMirror ) );

	pAccess->updateSoundLibrary( SoundLibraryInfo::Type::Pattern );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fPatternKnown( pEngine ); } ) );
	CPPUNIT_ASSERT( fPatternKnown( pMirror ) );

	pAccess->updateSoundLibrary( SoundLibraryInfo::Type::Song );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fSongKnown( pEngine ); } ) );
	CPPUNIT_ASSERT( fSongKnown( pMirror ) );

	// The full rescan crosses too — and drops artifacts that went away in
	// the meantime (the pattern file is removed first).
	Filesystem::rm( sPatternPath );
	pAccess->rescanSoundLibrary();
	// SoundLibraryDatabase::update() rebuilds all three databases in
	// sequence, clearing each one before rescanning it. A negative
	// predicate on the pattern list is satisfied by the very first
	// clear — long before the drumkit and song databases are
	// repopulated — so the pump has to wait for the settled post-state
	// of the whole rescan, not just the pattern's disappearance.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! fPatternKnown( pEngine ) && fDrumkitKnown( pEngine ) &&
			fSongKnown( pEngine ); } ) );
	CPPUNIT_ASSERT( ! fPatternKnown( pMirror ) );
	// The other two survived the full rescan.
	CPPUNIT_ASSERT( fDrumkitKnown( pEngine ) );
	CPPUNIT_ASSERT( fSongKnown( pEngine ) );

	// Cleanup.
	Filesystem::rm( sKitDir, true );
	Filesystem::rm( sSongPath );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2v — the MIDI setup fix (reset every MIDI out note to
// its list-slot default) mutates the current drumkit; initiated from the
// GUI it has to reach the engine's drumkit as well, not just the
// editor's mirror copy.
void EngineSessionTest::testSetDefaultMidiOutNotesCrossesSplit() {
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

	// Break the MIDI setup on both sides: every instrument answers on the
	// same note — the condition that makes the GUI offer the fix.
	const auto fBreakMidiSetup = [&]( H2Core::Hydrogen* pH ) {
		const auto pInstruments =
			pH->getSong()->getDrumkit()->getInstruments();
		CPPUNIT_ASSERT( pInstruments->size() >= 2 );
		for ( int ii = 0; ii < pInstruments->size(); ii++ ) {
			pInstruments->get( ii )->setMidiOutNote(
				Midi::noteFromIntClamp( 60 ) );
		}
		CPPUNIT_ASSERT( pInstruments->hasAllMidiNotesSame() );
	};
	fBreakMidiSetup( pEngine );
	fBreakMidiSetup( pMirror );

	// A fresh song starts with a clean drumkit on both sides.
	CPPUNIT_ASSERT( ! pEngine->getSong()->getDrumkit()->getIsModified() );
	CPPUNIT_ASSERT( ! pMirror->getSong()->getDrumkit()->getIsModified() );

	const auto fNotesDefaulted = [&]( H2Core::Hydrogen* pH ) {
		const auto pInstruments =
			pH->getSong()->getDrumkit()->getInstruments();
		for ( int ii = 0; ii < pInstruments->size(); ii++ ) {
			if ( pInstruments->get( ii )->getMidiOutNote() !=
				 InstrumentList::defaultMidiOutNote( ii ) ) {
				return false;
			}
		}
		return true; };

	// The command crosses: the mirror is reset synchronously in the
	// dual-apply, the engine via the bridge thread (pumped).
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setDefaultMidiOutNotes() );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fNotesDefaulted( pEngine ); } ) );
	CPPUNIT_ASSERT( fNotesDefaulted( pMirror ) );

	// The fix flipped the modified flag on both drumkits (the engine side
	// without the SongIsModified echo — Suppress at the bridge).
	CPPUNIT_ASSERT( pEngine->getSong()->getDrumkit()->getIsModified() );
	CPPUNIT_ASSERT( pMirror->getSong()->getDrumkit()->getIsModified() );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2w — MIDI actions triggered from the GUI (keyboard
// shortcuts in MainForm::executeShortcut) used to run only on the
// editor's mirror; they have to reach the authoritative engine's song
// and drumkit too.
void EngineSessionTest::testMidiActionCrossesSplit() {
	___INFOLOG( "" );

	// Editor-local classification: undo/redo drive the editor's command
	// stack (the engine holds none) and must never cross; everything
	// else is engine-relevant.
	CPPUNIT_ASSERT( MidiActionManager::isEditorLocal(
		MidiAction::Type::UndoAction ) );
	CPPUNIT_ASSERT( MidiActionManager::isEditorLocal(
		MidiAction::Type::RedoAction ) );
	CPPUNIT_ASSERT( ! MidiActionManager::isEditorLocal(
		MidiAction::Type::StripMuteToggle ) );

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

	// A drumkit mutation: strip mute on instrument 0 crosses — the
	// engine applies it via the bridge thread (pumped), the mirror
	// synchronously in the dual-apply.
	auto pMuteAction = std::make_shared<MidiAction>(
		MidiAction::Type::StripMuteToggle );
	pMuteAction->setInstrument( 0 );
	CPPUNIT_ASSERT( pAccess->handleMidiAction( pMuteAction ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getDrumkit()->getInstruments()
			->get( 0 )->isMuted(); } ) );
	CPPUNIT_ASSERT( pMirror->getSong()->getDrumkit()->getInstruments()
		->get( 0 )->isMuted() );

	// A song mutation: BPM increment by the action's factor crosses the
	// same way (observable on both audio engines' next BPM).
	const float fBpmBefore = pEngine->getAudioEngine()->getNextBpm();
	auto pBpmAction = std::make_shared<MidiAction>(
		MidiAction::Type::BpmIncr );
	pBpmAction->setFactor( 1.5f );
	CPPUNIT_ASSERT( pAccess->handleMidiAction( pBpmAction ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return std::abs( pEngine->getAudioEngine()->getNextBpm()
						 - ( fBpmBefore + 1.5f ) ) < 0.5; } ) );
	// The mirror's bpmIncrease is a designed no-op in editor mode (its
	// tempo source is Remote, like handleBeatCounter) — the mirror's BPM
	// lands via the engine's TempoChanged echo and its telemetry-based
	// correction, not the dual-apply.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return std::abs( pMirror->getAudioEngine()->getNextBpm()
						 - ( fBpmBefore + 1.5f ) ) < 0.5; } ) );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2x — the rubberband batch recalculation (MainToolBar
// toggle) swapped only the editor's mirror samples; the engine's
// in-memory copies have to be swapped too.
void EngineSessionTest::testRecalculateRubberbandCrossesSplit() {
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

	// Arm the recalculation on both sides: batch mode on (in the app the
	// engine's copy lands via the SetPreferences sync) and the first
	// layer's sample of instrument 0 marked for rubberband processing.
	const auto fArmRubberband = [&]( H2Core::Hydrogen* pH ) {
		pH->getPreferences()->setRubberBandBatchMode( 1 );
		const auto pInstrument =
			pH->getSong()->getDrumkit()->getInstruments()->get( 0 );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		const auto pComponent = pInstrument->getComponent( 0 );
		CPPUNIT_ASSERT( pComponent != nullptr );
		const auto pLayer = pComponent->getLayer( 0 );
		CPPUNIT_ASSERT( pLayer != nullptr );
		CPPUNIT_ASSERT( pLayer->getSample() != nullptr );
		auto rubberband = pLayer->getSample()->getRubberband();
		rubberband.bUse = true;
		pLayer->getSample()->setRubberband( rubberband );
		return pLayer->getSample();
	};
	const auto pEngineSample = fArmRubberband( pEngine );
	const auto pMirrorSample = fArmRubberband( pMirror );

	const auto fLayerSample = [&]( H2Core::Hydrogen* pH ) {
		return pH->getSong()->getDrumkit()->getInstruments()
			->get( 0 )->getComponent( 0 )->getLayer( 0 )->getSample();
	};
	CPPUNIT_ASSERT( fLayerSample( pEngine ) == pEngineSample );
	CPPUNIT_ASSERT( fLayerSample( pMirror ) == pMirrorSample );

	// The command crosses: the engine swaps its sample under the bridge
	// thread (pumped), the mirror synchronously in the dual-apply.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->recalculateRubberband() );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fLayerSample( pEngine ) != pEngineSample; } ) );
	CPPUNIT_ASSERT( fLayerSample( pMirror ) != pMirrorSample );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2y — the rubberband batch mode toggle wrote the
// editor's mirror preferences directly; the engine's copy (read live
// by its transport and drumkit) stayed stale.
void EngineSessionTest::testSetRubberBandBatchModeCrossesSplit() {
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

	CPPUNIT_ASSERT( pEngine->getPreferences()->getRubberBandBatchMode() == 0 );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getRubberBandBatchMode() == 0 );

	// The command crosses: the engine's flag lands under the bridge
	// thread (pumped), the mirror's synchronously in the dual-apply.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setRubberBandBatchMode( 1 ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getRubberBandBatchMode() ==
			1; } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getRubberBandBatchMode() == 1 );

	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setRubberBandBatchMode( 0 ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getRubberBandBatchMode() ==
			0; } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getRubberBandBatchMode() == 0 );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2y — the punch-in/out markers were written by the GUI
// ruler on the mirror only; the engine's recording decision (Hydrogen's
// realtime loop) never saw them.
void EngineSessionTest::testSetPunchAreaCrossesSplit() {
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

	CPPUNIT_ASSERT( pEngine->getPreferences()->getPunchInPos() == 0 );
	CPPUNIT_ASSERT( pEngine->getPreferences()->getPunchOutPos() == -1 );

	// The pair crosses as one command: the engine's markers land under
	// the bridge thread (pumped), the mirror's synchronously.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setPunchArea( 4, 9 ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getPunchInPos() == 4 &&
			pEngine->getPreferences()->getPunchOutPos() == 9; } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getPunchInPos() == 4 );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getPunchOutPos() == 9 );
	CPPUNIT_ASSERT( pEngine->getPreferences()->inPunchArea( 6 ) );
	CPPUNIT_ASSERT( ! pEngine->getPreferences()->inPunchArea( 10 ) );

	// The unset semantics (out = -1) clear the area restriction on
	// both sides: with no area defined every position records.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setPunchArea( 0, -1 ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getPunchOutPos() == -1; } ) );
	CPPUNIT_ASSERT( pEngine->getPreferences()->inPunchArea( 0 ) );
	CPPUNIT_ASSERT( pEngine->getPreferences()->inPunchArea( 999 ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getPunchOutPos() == -1 );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2z — the MIDI control dialog's scalar settings were
// written on the editor's mirror preferences only; the engine's MIDI
// I/O (note-off handling and action dispatch in MidiInput, feedback
// and transport sends in AudioEngine/MidiOutput, note-off sending in
// Sampler) read them live and stayed stale.
void EngineSessionTest::testSetMidiControlSettingsCrossesSplit() {
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

	// Default (Preferences.h): incoming note-off events are ignored.
	CPPUNIT_ASSERT( pEngine->getPreferences()->m_bMidiNoteOffIgnore );
	CPPUNIT_ASSERT( pMirror->getPreferences()->m_bMidiNoteOffIgnore );

	// The tuple crosses as one command: the engine's settings land
	// under the bridge thread (pumped), the mirror's synchronously in
	// the dual-apply.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->setMidiControlSettings(
		false, Midi::channelFromInt( 3 ), true, true, true,
		Midi::channelFromInt( 5 ),
		Preferences::MidiSendNoteOff::Never ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getPreferences()->m_bMidiNoteOffIgnore &&
			pEngine->getPreferences()->m_midiActionChannel ==
				Midi::channelFromInt( 3 ) &&
			pEngine->getPreferences()->m_bEnableMidiFeedback &&
			pEngine->getPreferences()->getMidiTransportInputHandling() &&
			pEngine->getPreferences()->getMidiTransportOutputSend() &&
			pEngine->getPreferences()->getMidiFeedbackChannel() ==
				Midi::channelFromInt( 5 ) &&
			pEngine->getPreferences()->getMidiSendNoteOff() ==
				Preferences::MidiSendNoteOff::Never; } ) );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->m_bMidiNoteOffIgnore );
	CPPUNIT_ASSERT( pMirror->getPreferences()->m_midiActionChannel ==
		Midi::channelFromInt( 3 ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->m_bEnableMidiFeedback );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiTransportInputHandling() );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiTransportOutputSend() );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiFeedbackChannel() ==
		Midi::channelFromInt( 5 ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiSendNoteOff() ==
		Preferences::MidiSendNoteOff::Never );

	// A second round proves repeated crossing — back to the defaults.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->setMidiControlSettings(
		true, Midi::ChannelAll, false, false, false, Midi::ChannelOff,
		Preferences::MidiSendNoteOff::Always ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->m_bMidiNoteOffIgnore &&
			pEngine->getPreferences()->m_midiActionChannel ==
				Midi::ChannelAll &&
			pEngine->getPreferences()->getMidiSendNoteOff() ==
				Preferences::MidiSendNoteOff::Always; } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->m_bMidiNoteOffIgnore );
	CPPUNIT_ASSERT( pMirror->getPreferences()->m_midiActionChannel ==
		Midi::ChannelAll );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiSendNoteOff() ==
		Preferences::MidiSendNoteOff::Always );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2z — the clock commands early-returned in Editor
// mode before installing on the preferences: the toggle crossed to
// the engine, but the editor's mirror kept the stale value and the
// dialog's next config save reverted it.
void EngineSessionTest::testSetMidiClockInputHandlingCrossesSplit() {
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

	CPPUNIT_ASSERT( ! pEngine->getPreferences()->getMidiClockInputHandling() );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->getMidiClockInputHandling() );

	// The command crosses AND installs on the mirror: the engine's
	// flag lands under the bridge thread (pumped), the mirror's
	// synchronously — else the dialog's config save would revert the
	// toggle.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setMidiClockInputHandling( true ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getMidiClockInputHandling(); } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiClockInputHandling() );

	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setMidiClockInputHandling( false ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getPreferences()->getMidiClockInputHandling(); } ) );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->getMidiClockInputHandling() );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2z — same mirror-desync shape as the clock input
// handling above, for the outgoing clock stream toggle.
void EngineSessionTest::testSetMidiClockOutputSendCrossesSplit() {
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

	CPPUNIT_ASSERT( ! pEngine->getPreferences()->getMidiClockOutputSend() );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->getMidiClockOutputSend() );

	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setMidiClockOutputSend( true ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getMidiClockOutputSend(); } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getMidiClockOutputSend() );

	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setMidiClockOutputSend( false ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getPreferences()->getMidiClockOutputSend(); } ) );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->getMidiClockOutputSend() );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2aa — the automation path view used to mutate the
// mirror's path directly and only crossed at mouse-release via the
// undo actions. The add/remove commands were untested so far and
// carried no validation: adding onto an occupied x silently
// overwrote the sitting point, removing an absent point was a silent
// no-op returning success.
void EngineSessionTest::testAddRemoveAutomationPointCrossesSplit() {
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

	auto pController = pAccess->getCoreActionController();

	CPPUNIT_ASSERT( pEngine->getSong()->getAutomationPath()->empty() );
	CPPUNIT_ASSERT( pMirror->getSong()->getAutomationPath()->empty() );

	// Add crosses: the engine's path fills under the bridge thread
	// (pumped), the mirror's synchronously in the dual-apply.
	CPPUNIT_ASSERT( pController->addAutomationPoint( 1.5f, 0.7f ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getSong()->getAutomationPath()->empty(); } ) );
	CPPUNIT_ASSERT( ! pMirror->getSong()->getAutomationPath()->empty() );
	CPPUNIT_ASSERT(
		pEngine->getSong()->getAutomationPath()->getValue( 1.5f ) == 0.7f );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getAutomationPath()->getValue( 1.5f ) == 0.7f );

	// Adding onto an occupied x would silently overwrite the sitting
	// point — refused.
	CPPUNIT_ASSERT( ! pController->addAutomationPoint( 1.5f, 0.1f ) );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getAutomationPath()->getValue( 1.5f ) == 0.7f );

	// Remove crosses; removing an absent point is refused instead of
	// being a silent no-op that still reports success.
	CPPUNIT_ASSERT( pController->removeAutomationPoint( 1.5f ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getAutomationPath()->empty(); } ) );
	CPPUNIT_ASSERT( pMirror->getSong()->getAutomationPath()->empty() );
	CPPUNIT_ASSERT( ! pController->removeAutomationPoint( 1.5f ) );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2aa — point moves used to cross as a remove+add
// pair (two commands, non-atomic) and the direct view mutations never
// crossed at all. The new atomic move command has to carry its own
// validation: a stale source, an absent source, or an occupied
// destination must fail cleanly instead of corrupting the path.
void EngineSessionTest::testMoveAutomationPointCrossesSplit() {
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

	auto pController = pAccess->getCoreActionController();

	CPPUNIT_ASSERT( pController->addAutomationPoint( 1.0f, 0.3f ) );
	CPPUNIT_ASSERT( pController->addAutomationPoint( 4.0f, 0.9f ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getAutomationPath()->getValue( 4.0f ) ==
			0.9f; } ) );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getAutomationPath()->getValue( 4.0f ) == 0.9f );

	// The move crosses as one atomic command: source gone, destination
	// set — on both sides.
	CPPUNIT_ASSERT(
		pController->moveAutomationPoint( 1.0f, 0.3f, 2.0f, 0.6f ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		auto pPath = pEngine->getSong()->getAutomationPath();
		return pPath->find( 2.0f ) != pPath->end() &&
			pPath->find( 2.0f )->second == 0.6f; } ) );
	{
		auto pPath = pMirror->getSong()->getAutomationPath();
		CPPUNIT_ASSERT( pPath->find( 1.0f ) == pPath->end() );
		CPPUNIT_ASSERT( pPath->find( 2.0f ) != pPath->end() );
		CPPUNIT_ASSERT( pPath->find( 2.0f )->second == 0.6f );
		CPPUNIT_ASSERT( pPath->find( 4.0f ) != pPath->end() );
	}

	// Stale source coordinates (wrong y) — refused, nothing moves.
	CPPUNIT_ASSERT(
		! pController->moveAutomationPoint( 2.0f, 0.123f, 3.0f, 0.5f ) );
	// Absent source — refused.
	CPPUNIT_ASSERT(
		! pController->moveAutomationPoint( 9.0f, 0.5f, 3.0f, 0.5f ) );
	// Occupied destination — refused: AutomationPath::move() erases
	// before inserting and std::map::insert does not overwrite, so the
	// moved point would be silently dropped.
	CPPUNIT_ASSERT(
		! pController->moveAutomationPoint( 2.0f, 0.6f, 4.0f, 0.5f ) );
	{
		auto pPath = pMirror->getSong()->getAutomationPath();
		CPPUNIT_ASSERT( pPath->find( 2.0f ) != pPath->end() );
		CPPUNIT_ASSERT( pPath->find( 2.0f )->second == 0.6f );
		CPPUNIT_ASSERT( pPath->find( 4.0f ) != pPath->end() );
		CPPUNIT_ASSERT( pPath->find( 4.0f )->second == 0.9f );
	}

	// A y-only move (same x) is a plain erase+insert of the same key.
	CPPUNIT_ASSERT(
		pController->moveAutomationPoint( 2.0f, 0.6f, 2.0f, 0.8f ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		auto pPath = pEngine->getSong()->getAutomationPath();
		return pPath->find( 2.0f ) != pPath->end() &&
			pPath->find( 2.0f )->second == 0.8f; } ) );
	CPPUNIT_ASSERT(
		pMirror->getSong()->getAutomationPath()->find( 2.0f )->second ==
		0.8f );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2ab — virtual pattern relationships are song structure
// shared by both sides: the editor dialog used to clear/add them on its
// mirror only, so the authoritative engine kept playing the old set. The
// command crosses as a whole-set replacement (clear + add + recompute of
// the flattened sets), validated atomically before anything is touched.
void EngineSessionTest::testSetVirtualPatternsCrossesSplit() {
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

	// The empty song ships ten named patterns; "Pattern 1" becomes the
	// virtual container of "Pattern 2" and "Pattern 3".
	const QStringList virtuals = { "Pattern 2", "Pattern 3" };
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->setVirtualPatterns(
		0, virtuals ) );

	// Settled post-state: the engine's pattern holds the direct set, its
	// flattened set was recomputed by the engine-side
	// updateVirtualPatterns(), and the song is dirty. Pumping on the
	// direct set alone could sample the state between the add and the
	// recompute.
	const auto fEngineSettled = [&]( const QStringList& expected ) {
		auto pPattern = pEngine->getSong()->getPatternList()->get( 0 );
		return pPattern != nullptr &&
			pPattern->getVirtualPatterns()->size() ==
				static_cast<int>( expected.size() ) &&
			pPattern->getFlattenedVirtualPatterns()->size() ==
				static_cast<int>( expected.size() ) &&
			pEngine->getSong()->getIsModified();
	};
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fEngineSettled( virtuals ); } ) );
	// The mirror applied synchronously in the dual-apply.
	{
		auto pPattern = pMirror->getSong()->getPatternList()->get( 0 );
		CPPUNIT_ASSERT( pPattern->getVirtualPatterns()->size() == 2 );
		CPPUNIT_ASSERT( pPattern->getFlattenedVirtualPatterns()->size() == 2 );
	}

	// A second call replaces the set instead of merging into it.
	const QStringList fewer = { "Pattern 2" };
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->setVirtualPatterns(
		0, fewer ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return fEngineSettled( fewer ); } ) );
	CPPUNIT_ASSERT( pMirror->getSong()->getPatternList()->get( 0 )
					->getVirtualPatterns()->size() == 1 );

	// Refusals are atomic — the engine-side set is left untouched.
	// Out-of-range pattern number.
	CPPUNIT_ASSERT( ! pAccess->getCoreActionController()->setVirtualPatterns(
		42, virtuals ) );
	// Unknown virtual pattern name.
	CPPUNIT_ASSERT( ! pAccess->getCoreActionController()->setVirtualPatterns(
		0, { "Pattern 2", "no such pattern" } ) );
	// A virtual pattern must not contain itself.
	CPPUNIT_ASSERT( ! pAccess->getCoreActionController()->setVirtualPatterns(
		0, { "Pattern 1" } ) );
	CPPUNIT_ASSERT( pEngine->getSong()->getPatternList()->get( 0 )
					->getVirtualPatterns()->size() == 1 );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2ac — the pattern editor panel's quantize toggle and
// grid resolution combo wrote the editor's mirror preferences only.
// The authoritative engine's addRealtimeNote() reads all three live
// (the quantize flag and the resolution×triplets pair forming the
// quantization grid), so incoming keyboard/MIDI notes kept being
// quantized on the stale grid.
void EngineSessionTest::testSetQuantizeEventsCrossesSplit() {
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

	// Baseline (src/tests/data/preferences/current.conf, loaded by
	// create_instance()): incoming events are quantized.
	CPPUNIT_ASSERT( pEngine->getPreferences()->getQuantizeEvents() );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getQuantizeEvents() );

	// The flag crosses as one command: the engine's copy lands under
	// the bridge thread (pumped), the mirror's synchronously in the
	// dual-apply.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setQuantizeEvents( false ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getPreferences()->getQuantizeEvents(); } ) );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->getQuantizeEvents() );

	// A second round proves repeated crossing — back to the default.
	CPPUNIT_ASSERT(
		pAccess->getCoreActionController()->setQuantizeEvents( true ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getQuantizeEvents(); } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()->getQuantizeEvents() );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2ac — the resolution×triplets pair crosses as one
// command (the setPunchArea precedent): addRealtimeNote() computes the
// quantization grid from both, so the engine must never see a
// half-updated pair. The "off" resolution (a full quarter of ticks)
// crosses like any other value.
void EngineSessionTest::testSetPatternEditorGridCrossesSplit() {
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

	// Baseline (src/tests/data/preferences/current.conf, loaded by
	// create_instance()): 1/16 grid, no triplets.
	CPPUNIT_ASSERT(
		pEngine->getPreferences()->getPatternEditorGridResolution() == 16 );
	CPPUNIT_ASSERT(
		! pEngine->getPreferences()->isPatternEditorUsingTriplets() );
	CPPUNIT_ASSERT(
		pMirror->getPreferences()->getPatternEditorGridResolution() == 16 );
	CPPUNIT_ASSERT(
		! pMirror->getPreferences()->isPatternEditorUsingTriplets() );

	// 1/16T — the pair lands atomically on both sides.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()
					->setPatternEditorGrid( 32, true ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()
				->getPatternEditorGridResolution() == 32 &&
			pEngine->getPreferences()->isPatternEditorUsingTriplets(); } ) );
	CPPUNIT_ASSERT(
		pMirror->getPreferences()->getPatternEditorGridResolution() == 32 );
	CPPUNIT_ASSERT(
		pMirror->getPreferences()->isPatternEditorUsingTriplets() );

	// 1/64 — a second round proves repeated crossing.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()
					->setPatternEditorGrid( 64, false ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()
				->getPatternEditorGridResolution() == 64 &&
			! pEngine->getPreferences()->isPatternEditorUsingTriplets(); } ) );
	CPPUNIT_ASSERT(
		pMirror->getPreferences()->getPatternEditorGridResolution() == 64 );
	CPPUNIT_ASSERT(
		! pMirror->getPreferences()->isPatternEditorUsingTriplets() );

	// "off" — the combo's free-hand mode parks the cursor on a full
	// quarter of ticks; that resolution crosses like any other.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->setPatternEditorGrid(
		4 * H2Core::nTicksPerQuarter, false ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getPatternEditorGridResolution() ==
			4 * H2Core::nTicksPerQuarter; } ) );
	CPPUNIT_ASSERT( pMirror->getPreferences()
					->getPatternEditorGridResolution() ==
		4 * H2Core::nTicksPerQuarter );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0030 batch 2ad — replaceInstrument() crosses for the playback track
// combos the SongEditorPanel undo stack produces: discarding the current
// track (nullptr new instrument, the delete button's redo) and restoring
// one (nullptr old instrument, its undo). The old playback track is engine
// state — the bridge derives it from the authoritative song instead of
// marshaling the editor's copy. Regular drumkit replacements (both
// instruments non-null) keep crossing by id.
void EngineSessionTest::testReplaceInstrumentCrossesSplit() {
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

	// --- Drumkit replacement (both instruments non-null) keeps crossing
	// by id: the engine swaps its own copy for the deserialized new one.
	auto pOldMirror =
		pMirror->getSong()->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pOldMirror != nullptr );
	auto pNewMirror =
		std::make_shared<Instrument>( pOldMirror ); // copy (same id)
	pNewMirror->setName( "IPC-REPLACED" );
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->replaceInstrument(
		pNewMirror, pOldMirror ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		const auto pReplaced = pEngine->getSong()->getDrumkit()
			->getInstruments()
			->find( pOldMirror->getId() );
		return pReplaced != nullptr && pReplaced->getName() == "IPC-REPLACED";
	} ) );
	CPPUNIT_ASSERT( pMirror->getSong()->getDrumkit()->getInstruments()
		->find( pOldMirror->getId() )
		->getName() == "IPC-REPLACED" );

	// --- A playback track to discard: loadPlaybackTrack() crosses via its
	// own opcode and installs the engine-side track.
	const QString sTrackFile = H2TEST_FILE( "song/res/playbackTrack.flac" );
	pAccess->loadPlaybackTrack( sTrackFile );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getPlaybackTrackInstrument() != nullptr;
	} ) );

	// The editor-side old instrument, as the delete handler holds it: the
	// mirror's copy of the track. Only its identity — not its samples —
	// crosses.
	auto pTrackMirror = Instrument::from( Sample::load( sTrackFile ), pMirror );
	CPPUNIT_ASSERT( pTrackMirror != nullptr );
	pTrackMirror->setId( Instrument::PlaybackTrackId );
	pTrackMirror->setName( "PlaybackTrack" );
	pMirror->getSong()->setPlaybackTrackInstrument( pTrackMirror );

	// --- Discard (nullptr new): the delete button's redo() — used to stop
	// at the editor because of the null instrument.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->replaceInstrument(
		nullptr, pTrackMirror ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong()->getPlaybackTrackInstrument() == nullptr;
	} ) );
	CPPUNIT_ASSERT( pMirror->getSong()->getPlaybackTrackInstrument() == nullptr );

	// --- Restore (nullptr old): the delete's undo() re-installs the track.
	CPPUNIT_ASSERT( pAccess->getCoreActionController()->replaceInstrument(
		pTrackMirror, nullptr ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		const auto pTrack = pEngine->getSong()->getPlaybackTrackInstrument();
		return pTrack != nullptr &&
			pTrack->getId() == Instrument::PlaybackTrackId;
	} ) );
	CPPUNIT_ASSERT( pMirror->getSong()->getPlaybackTrackInstrument() != nullptr );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// ADR 0031: while the authoritative engine plays, the mirror's transport
// free-runs on its own SoftwareDriver clock — the playhead must advance
// steadily, with telemetry only correcting drift. If the mirror relied on
// telemetry snapshots alone, the playhead would stand still between syncs
// and jump on each one.
void EngineSessionTest::testMirrorTransportFreeRuns() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getSong() != nullptr; } ) );

	// The engine starts rolling; the mirror follows the state via telemetry.
	pEngine->sequencerPlay();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getAudioEngine()->getState() ==
			AudioEngine::State::Playing; } ) );

	// Sampled every ~100 ms, a free-running playhead advances by roughly
	// that interval worth of frames: it must move (not frozen) but never
	// jump by half a second or more (a telemetry resync snap).
	const auto pPlayhead = pMirror->getAudioEngine()->getPlayhead();
	const unsigned nSampleRate =
		pMirror->getAudioEngine()->getAudioDriver()->getSampleRate();
	long long nLastFrame = pPlayhead->getFrame();
	QElapsedTimer timer;
	timer.start();
	qint64 nLastSampleAt = 0;
	while ( timer.elapsed() < 1500 ) {
		QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
		QThread::msleep( 5 );
		if ( timer.elapsed() - nLastSampleAt >= 100 ) {
			const long long nAdvanced = pPlayhead->getFrame() - nLastFrame;
			CPPUNIT_ASSERT( nAdvanced > 0 );
			CPPUNIT_ASSERT( nAdvanced < static_cast<long long>( nSampleRate ) / 2 );
			nLastFrame += nAdvanced;
			nLastSampleAt = timer.elapsed();
		}
	}

	// The engine stops; the mirror follows back to Ready.
	pEngine->sequencerStop();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getAudioEngine()->getState() ==
			AudioEngine::State::Ready; } ) );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// A JACK-transport engine must not stall the mirror's clock. The mirror has
// no JACK client (the authoritative engine owns the server transport), and
// its engine core must not consume the GUI-facing cached AudioDriverInfo:
// hasJackTransport() asks the local driver only — false on the mirror by
// construction — so play()/stop() roll the local state machine (ADR 0031's
// free-running playhead) and the telemetry correction keeps it aligned.
// Regression: AudioEngine::play()/stop() used to branch on
// hasJackTransport() while that still answered the cross-process question
// in editor mode, bailing out without setNextState() and freezing the
// mirror playhead except for per-beat BbtChanged resync snaps.
void EngineSessionTest::testMirrorTransportFreeRunsUnderJackTransport() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getSong() != nullptr; } ) );

	// Simulate an engine whose driver uses JACK transport. The mirror knows
	// about this only through the IPC-cached AudioDriverInfo (ADR 0029) —
	// the coreUses*() question. The engine-local hasJack*() question must
	// stay unaffected by it.
	auto info = pMirror->getCachedAudioDriverInfo();
	info.jackTransportEnabled = true;
	pMirror->setCachedAudioDriverInfo( info );

	// The layering contract: the cached info answers the GUI's
	// cross-process question; the mirror engine's own transport branching
	// asks the local driver only.
	CPPUNIT_ASSERT( ! pMirror->hasJackTransport() );
	CPPUNIT_ASSERT( pMirror->coreUsesJackTransport() );

	// The engine starts rolling; the mirror must follow into Playing —
	// its own clock, not a JACK server it cannot talk to.
	pEngine->sequencerPlay();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getAudioEngine()->getState() ==
			AudioEngine::State::Playing; } ) );

	// Same steady-advance contract as the non-JACK case.
	const auto pPlayhead = pMirror->getAudioEngine()->getPlayhead();
	const unsigned nSampleRate =
		pMirror->getAudioEngine()->getAudioDriver()->getSampleRate();
	long long nLastFrame = pPlayhead->getFrame();
	QElapsedTimer timer;
	timer.start();
	qint64 nLastSampleAt = 0;
	while ( timer.elapsed() < 1500 ) {
		QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
		QThread::msleep( 5 );
		if ( timer.elapsed() - nLastSampleAt >= 100 ) {
			const long long nAdvanced = pPlayhead->getFrame() - nLastFrame;
			CPPUNIT_ASSERT( nAdvanced > 0 );
			CPPUNIT_ASSERT( nAdvanced < static_cast<long long>( nSampleRate ) / 2 );
			nLastFrame += nAdvanced;
			nLastSampleAt = timer.elapsed();
		}
	}

	// The engine stops; the mirror must follow back to Ready instead of
	// rolling on forever.
	pEngine->sequencerStop();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getAudioEngine()->getState() ==
			AudioEngine::State::Ready; } ) );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// What BpmSpinBox displays is the mirror playhead's BPM. With the Timeline
// active and the transport in the special first marker's region (before the
// first user marker), that BPM is the special marker's tempo —
// Timeline::m_fDefaultBpm, captured from the song BPM at activation. Both
// processes must agree on it: the engine's Timeline (authoritative
// playback), the mirror's Timeline (what the timeline editor labels the
// special marker with), and both playheads (what the BPM widgets display).
void EngineSessionTest::testMirrorBpmShowsSpecialTempoMarker() {
	___INFOLOG( "" );

	const float fSpecialBpm = 100.0f;
	const float fChangedSongBpm = 130.0f;
	const float fUserMarkerBpm = 140.0f;

	auto* pEngine = TestHelper::makeEngine();
	auto pSong = Song::getEmptySong( pEngine );
	pSong->setMode( Song::Mode::Song );
	pSong->setBpm( fSpecialBpm );

	pEngine->getAudioEngine()->lock( RIGHT_HERE );
	// First user marker at column 4 — everything before it is the special
	// first marker's region.
	pSong->getTimeline()->addTempoMarker( 4, fUserMarkerBpm );
	pEngine->getAudioEngine()->unlock();

	pEngine->setSong( pSong );

	// The Timeline is already active when the editor attaches — activating
	// it captured the song's BPM as the special first marker's tempo.
	pEngine->setIsTimelineActivated( true );
	CPPUNIT_ASSERT( pEngine->getSong()->getTimeline()->getDefaultBpm()
					== fSpecialBpm );

	// An external tempo change (MIDI/OSC/BeatCounter/TapTempo) is stored in
	// the song while the Timeline is active, but the transport keeps
	// following the Timeline: the special marker retains the captured tempo
	// and the engine's playhead in the special region keeps showing it.
	pEngine->getAudioEngine()->lock( RIGHT_HERE );
	pSong->setBpm( fChangedSongBpm );
	pEngine->getAudioEngine()->unlock();

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	auto* pMirror = TestHelper::makeMirror();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	// The editor pulls the song exactly like HydrogenApp::syncViaIpc() does:
	// a GetSong request, deserializing the reply, and a dual-apply setSong
	// (the mirror takes the live object; the engine is re-set from the same
	// buffer). The connect-time priming only covers selection/record state.
	IpcMessage songReply;
	CPPUNIT_ASSERT( pEditor->getChannel()->request(
		IpcMessage( IpcOpcode::GetSong ), songReply, 3000 ) );
	CPPUNIT_ASSERT( ! songReply.getPayload().isEmpty() );
	auto pSyncedSong = Song::fromXmlBuffer(
		songReply.getPayload(), Xml::Flag::Ipc, true, pMirror );
	CPPUNIT_ASSERT( pSyncedSong != nullptr );
	pMirror->getCoreActionController()->setSong( pSyncedSong );
	CPPUNIT_ASSERT( pMirror->getSong()->getBpm() == fChangedSongBpm );

	// syncViaIpc() step 8: force an immediate transport re-sync from
	// telemetry — the periodic resync only runs at a 5 s cadence.
	pEditor->getStateMirror()->forceTransportSync();

	// The engine's playhead in the special region shows the captured tempo,
	// not the changed song BPM.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return std::fabs( pEngine->getAudioEngine()->getPlayhead()->getBpm()
						  - fSpecialBpm ) < 0.01f; } ) );

	// The editor's BPM widget displays the mirror playhead's BPM — it must
	// follow the engine's (special) tempo via telemetry, not the changed
	// song BPM.
	const bool bMirrorFollowed = TestHelper::pumpUntil( [&]() {
		return std::fabs( pMirror->getAudioEngine()->getPlayhead()->getBpm()
						  - fSpecialBpm ) < 0.01f; } );
	CPPUNIT_ASSERT_MESSAGE(
		QString( "mirror playhead bpm: %1, engine playhead bpm: %2, "
				 "mirror songBpm: %3, mirror defaultBpm: %4, "
				 "mirror telemetry bpm: %5" )
			.arg( pMirror->getAudioEngine()->getPlayhead()->getBpm() )
			.arg( pEngine->getAudioEngine()->getPlayhead()->getBpm() )
			.arg( pMirror->getSong()->getBpm() )
			.arg( pMirror->getSong()->getTimeline()->getDefaultBpm() )
			.arg( pEditor->getStateMirror()->getTelemetry().bpm )
			.toStdString(),
		bMirrorFollowed );

	// Crossing fidelity: the mirror's Timeline carries the engine's captured
	// special tempo, not a value re-derived from the changed song BPM.
	CPPUNIT_ASSERT( pEngine->getSong()->getTimeline()->getDefaultBpm()
					== fSpecialBpm );
	CPPUNIT_ASSERT_MESSAGE(
		QString( "engine defaultBpm: %1, mirror defaultBpm: %2, "
				 "engine songBpm: %3, mirror songBpm: %4" )
			.arg( pEngine->getSong()->getTimeline()->getDefaultBpm() )
			.arg( pMirror->getSong()->getTimeline()->getDefaultBpm() )
			.arg( pEngine->getSong()->getBpm() )
			.arg( pMirror->getSong()->getBpm() )
			.toStdString(),
		pMirror->getSong()->getTimeline()->getDefaultBpm() == fSpecialBpm );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}
