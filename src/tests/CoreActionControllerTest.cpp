/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "CoreActionControllerTest.h"

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Helpers/Filesystem.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>

#include <chrono>
#include <thread>

using namespace H2Core;

void CoreActionControllerTest::testCountIn() {
	___INFOLOG( "" );
	auto pSongSizeChanged = Song::load(
		QString( H2TEST_FILE( "song/AE_songSizeChanged.h2song" ) ), false, pTestHydrogen() );
	ASSERT_SONG( pSongSizeChanged );
	pTestHydrogen()->getCoreActionController()->setSong( pSongSizeChanged );
	pTestHydrogen()->getCoreActionController()->activateSongMode( true );

	// Move to different columns in song mode and start the count in. Since
	// patterns of different length are present in those columns, we should see
	// different numbers of count in ticks.

	auto countInTicksForColumn = []( int nColumn ) {
		auto pHydrogen = pTestHydrogen();
		auto pAudioEngine = pHydrogen->getAudioEngine();

		CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->locateToColumn( nColumn ) );
		CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->startCountIn() );
		CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->setBpm( MAX_BPM ) );

		// Right away the AudioEngine should be in State::CountIn.
		pAudioEngine->lock( RIGHT_HERE );
		const auto state = pAudioEngine->getState();
		pAudioEngine->unlock();
		CPPUNIT_ASSERT( state == AudioEngine::State::CountIn );

		// Wait till count in is done.
		int nnTry = 0;
		const int nMaxTries = 50;
		while( nnTry < nMaxTries ) {
			pAudioEngine->lock( RIGHT_HERE );
			const auto currentState = pAudioEngine->getState();
			pAudioEngine->unlock();

			if ( currentState != AudioEngine::State::CountIn ) {
				break;
			}

			++nnTry;
			std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		}
		CPPUNIT_ASSERT( nnTry < nMaxTries );

		return pAudioEngine->getCountInMetronomeTicks();
	};

	std::vector< std::pair<int, int> > results{ {0, 1}, {1, 9}, {2,4} };
	for ( const auto [ nnColumn, nnTicks ] : results ) {
		const auto nTicksReal = countInTicksForColumn( nnColumn );
		___INFOLOG( QString( "column: %1, ticks: %2, reference: %3" )
					.arg( nnColumn ).arg( nTicksReal ).arg( nnTicks ) );
		CPPUNIT_ASSERT( nnTicks == nTicksReal );
	}

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSetPatternSize() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	pCAC->setSong( Song::getEmptySong( pHydrogen ) );
	auto pPatternList = pHydrogen->getSong()->getPatternList();
	CPPUNIT_ASSERT( pPatternList->size() > 0 );
	auto pPattern = pPatternList->get( 0 );
	CPPUNIT_ASSERT( pPattern != nullptr );

	const int nNewLength = 384;
	const int nNewDenominator = 8;
	CPPUNIT_ASSERT( pCAC->setPatternSize( nNewLength, nNewDenominator, 0 ) );
	CPPUNIT_ASSERT_EQUAL( nNewLength, pPattern->getLength() );
	CPPUNIT_ASSERT_EQUAL( nNewDenominator, pPattern->getDenominator() );

	// An out-of-range pattern index fails gracefully without mutating state.
	CPPUNIT_ASSERT( ! pCAC->setPatternSize( 192, 4, 999 ) );
	CPPUNIT_ASSERT_EQUAL( nNewLength, pPattern->getLength() );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testEditNoteProperty() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	pCAC->setSong( Song::getEmptySong( pHydrogen ) );
	auto pSong = pHydrogen->getSong();
	auto pInstrument = pSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	auto pPattern = pSong->getPatternList()->get( 0 );
	CPPUNIT_ASSERT( pPattern != nullptr );

	auto pNote = std::make_shared<Note>( pInstrument, 0, 0.5f, 0.f, -1 );
	pPattern->insertNote( pNote );

	const auto nId = static_cast<int>( pInstrument->getId() );
	const auto& sType = pInstrument->getType();
	const auto nKey = static_cast<int>( pNote->getKey() );
	const auto nOctave = static_cast<int>( pNote->getOctave() );

	// A real change is applied and reported.
	const float fNewVel = 0.9f;
	CPPUNIT_ASSERT( pCAC->editNoteProperty(
		NoteProperty::Velocity, 0, 0, nId, nId, sType, sType,
		fNewVel, 0.f, 0.f, 0.f, -1, nKey, nKey, nOctave, nOctave ) );
	CPPUNIT_ASSERT_EQUAL( fNewVel, pNote->getVelocity() );

	// Re-applying the same value is a no-op (returns false).
	CPPUNIT_ASSERT( ! pCAC->editNoteProperty(
		NoteProperty::Velocity, 0, 0, nId, nId, sType, sType,
		fNewVel, 0.f, 0.f, 0.f, -1, nKey, nKey, nOctave, nOctave ) );

	// A missing note fails gracefully.
	CPPUNIT_ASSERT( ! pCAC->editNoteProperty(
		NoteProperty::Velocity, 0, 4242, nId, nId, sType, sType,
		0.3f, 0.f, 0.f, 0.f, -1, nKey, nKey, nOctave, nOctave ) );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testAddOrRemoveNote() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	pCAC->setSong( Song::getEmptySong( pHydrogen ) );
	auto pSong = pHydrogen->getSong();
	auto pInstrument = pSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pPattern = pSong->getPatternList()->get( 0 );
	CPPUNIT_ASSERT( pPattern != nullptr );

	const int nId = static_cast<int>( pInstrument->getId() );
	const auto& sType = pInstrument->getType();
	// A fresh note carries the default key/octave used to address it.
	auto pProbe = std::make_shared<Note>( pInstrument, 0, 1.0f, 0.f, -1 );
	const int nKey = static_cast<int>( pProbe->getKey() );
	const int nOctave = static_cast<int>( pProbe->getOctave() );
	const int nNotesBefore = static_cast<int>( pPattern->getNotes()->size() );

	// Add a note.
	Uuid newNoteUuid;
	CPPUNIT_ASSERT( pCAC->addOrRemoveNote(
		12 /*position*/, nId, sType, 0 /*pattern*/, -1 /*length*/,
		0.8f, 0.f, 0.f, nKey, nOctave, 1.0f,
		false /*delete*/, false /*noteOff*/, true /*mapped*/, &newNoteUuid ) );
	CPPUNIT_ASSERT_EQUAL( nNotesBefore + 1,
						  static_cast<int>( pPattern->getNotes()->size() ) );
	CPPUNIT_ASSERT( pPattern->findNote(
						12, pInstrument->getId(), sType, pProbe->getKey(),
						pProbe->getOctave() ) != nullptr );
	bool bFound = false;
	for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
		if ( ppNote != nullptr && ppNote->getUuid() == newNoteUuid ) {
			bFound = true;
			break;
		}
	}
	CPPUNIT_ASSERT( bFound );

	// Remove it again.
	CPPUNIT_ASSERT( pCAC->addOrRemoveNote(
		12, nId, sType, 0, -1, 0.8f, 0.f, 0.f, nKey, nOctave, 1.0f,
		true /*delete*/, false, true, &newNoteUuid ) );
	CPPUNIT_ASSERT_EQUAL( nNotesBefore,
						  static_cast<int>( pPattern->getNotes()->size() ) );
	CPPUNIT_ASSERT( pPattern->findNote(
						12, pInstrument->getId(), sType, pProbe->getKey(),
						pProbe->getOctave() ) == nullptr );
	bFound = false;
	for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
		if ( ppNote != nullptr && ppNote->getUuid() == newNoteUuid ) {
			bFound = true;
			break;
		}
	}
	CPPUNIT_ASSERT( ! bFound );

	// An out-of-range pattern index fails gracefully.
	CPPUNIT_ASSERT( ! pCAC->addOrRemoveNote(
		12, nId, sType, 999, -1, 0.8f, 0.f, 0.f, nKey, nOctave, 1.0f,
		false, false, true, &newNoteUuid ) );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSetPanLaw() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();
	pCAC->setSong( Song::getEmptySong( pHydrogen ) );

	CPPUNIT_ASSERT( pCAC->setPanLaw( Sampler::RATIO_STRAIGHT_POLYGONAL, 1.5f ) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT_EQUAL(
		static_cast<int>( Sampler::RATIO_STRAIGHT_POLYGONAL ),
		pSong->getPanLawType() );
	CPPUNIT_ASSERT_EQUAL( 1.5f, pSong->getPanLawKNorm() );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testPlaybackTrack() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();
	auto pSong = Song::getEmptySong( pHydrogen );
	pCAC->setSong( pSong );

	// With no playback track loaded the setters fail gracefully (no crash).
	CPPUNIT_ASSERT(
		pHydrogen->getSong()->getPlaybackTrackInstrument() == nullptr
	);
	CPPUNIT_ASSERT( !pCAC->setPlaybackTrackMuted( true ) );
	CPPUNIT_ASSERT( !pCAC->setPlaybackTrackVolume( 0.5f ) );

	// A playback track is present: the setters apply.
	pHydrogen->loadPlaybackTrack( H2TEST_FILE( "song/res/playbackTrack.flac" )
	);
	CPPUNIT_ASSERT(
		pHydrogen->getSong()->getPlaybackTrackInstrument() != nullptr
	);
	CPPUNIT_ASSERT( pCAC->setPlaybackTrackVolume( 0.5f ) );
	CPPUNIT_ASSERT_EQUAL(
		0.5f, pHydrogen->getSong()->getPlaybackTrackInstrument()->getVolume()
	);
	CPPUNIT_ASSERT( pCAC->setPlaybackTrackMuted( true ) );
	CPPUNIT_ASSERT( pHydrogen->getSong()->getPlaybackTrackInstrument()->isMuted(
	) );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSessionManagement() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto sFilePath = Filesystem::tmpDir().append( "test1.h2song" );
	auto sFilePath2 = Filesystem::tmpDir().append( "test2.h2song" );

	pHydrogen->setSong( Song::getEmptySong( pTestHydrogen() ) );
	
	QTemporaryFile fileWrong;
	CPPUNIT_ASSERT( fileWrong.open() );
	const auto sFileNameImproper = fileWrong.fileName();

	// Create a new song with a proper file name and existing and
	// writable file.
	sFilePath = QString( "%1.h2song" ).arg( sFileNameImproper );
	QFile fileProper( sFilePath );
	if ( fileProper.open( QIODevice::ReadWrite ) ) {
		// The file only has to exist for the saves below. Windows can
		// neither replace nor remove a file that is still held open —
		// and Song::save() commits via rename (QSaveFile, ADR 0023) —
		// so release our handle right away instead of holding it for
		// the whole block.
		fileProper.close();

		auto pSong = H2Core::Song::getEmptySong( pTestHydrogen() );
		pSong->setPath( fileProper.fileName() );
		CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->setSong( pSong ) );
		ASSERT_PATH( sFilePath, pHydrogen->getSong()->getPath() );
	
		// -----------------------------------------------------------
		// Test pTestHydrogen()->getCoreActionController()->saveSong()
		// -----------------------------------------------------------
		
		CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->saveSong( true ) );

		// -----------------------------------------------------------
	
	}
	
	// Create a new song with proper a file name but no existing file.
	std::shared_ptr<H2Core::Song> pSong;
	sFilePath2 = QString( "%1_new.h2song" ).arg( sFileNameImproper );
	pSong = H2Core::Song::getEmptySong( pTestHydrogen() );
	pSong->setPath( sFilePath2 );
	CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->setSong( pSong ) );
	ASSERT_PATH( sFilePath2, pHydrogen->getSong()->getPath() );

	// ---------------------------------------------------------------
	// Test pTestHydrogen()->getCoreActionController()->loadSong() and ::setSong();
	// ---------------------------------------------------------------

	// Attempt to load a non-existing song.
	pSong = pTestHydrogen()->getCoreActionController()->loadSong( sFileNameImproper );
	CPPUNIT_ASSERT( pSong == nullptr );
	CPPUNIT_ASSERT( ! pTestHydrogen()->getCoreActionController()->setSong( pSong ) );
	
	// The previous action should have not affected the current song.
	ASSERT_PATH( sFilePath2, pHydrogen->getSong()->getPath() );
	CPPUNIT_ASSERT( pSong != pHydrogen->getSong() );
	
	// Load the first song (which was saved).
	pSong = pTestHydrogen()->getCoreActionController()->loadSong( sFilePath );
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->setSong( pSong ) );
	ASSERT_PATH( sFilePath, pHydrogen->getSong()->getPath() );
	CPPUNIT_ASSERT( pSong == pHydrogen->getSong() );

	// Attempt to load the second song. This will fail since it should not be
	// present on disk.
	CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->loadSong( sFilePath2 ) ==
					nullptr );
	
	// ---------------------------------------------------------------
	// Test pTestHydrogen()->getCoreActionController()->saveSongAs()
	// ---------------------------------------------------------------
	
	// But we can, instead, make a copy of the current song by saving
	// it to sFilePath2.
	CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->saveSongAs( sFilePath2, true ) );
	
	// Check if everything worked out.
	pSong = pTestHydrogen()->getCoreActionController()->loadSong( sFilePath );
	CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->setSong( pSong ) );
	ASSERT_PATH( sFilePath, pHydrogen->getSong()->getPath() );
	pSong = pTestHydrogen()->getCoreActionController()->loadSong( sFilePath2 );
	CPPUNIT_ASSERT( pTestHydrogen()->getCoreActionController()->setSong( pSong ) );
	ASSERT_PATH( sFilePath2, pHydrogen->getSong()->getPath() );

	// ---------------------------------------------------------------
	
	CPPUNIT_ASSERT( fileProper.remove() );

	// ---------------------------------------------------------------
	
	pHydrogen->setSong( Song::getEmptySong( pTestHydrogen() ) );

	if ( QFile::exists( sFilePath ) ) {
		QFile::remove( sFilePath );
	}
	if ( QFile::exists( sFilePath2 ) ) {
		QFile::remove( sFilePath2 );
	}

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSaveSongDiscardEvent() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	// A song containing layers with missing samples.
	auto pSong = Song::load(
		H2TEST_FILE( "song/legacy/test_song_invalid_sample_path.h2song" ),
		false, pHydrogen );
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pSong->hasMissingSamples() );

	// Redirect to a scratch path before installing — the fixture in
	// src/tests/data must not be overwritten by the save below.
	const QString sScratchPath = Filesystem::tmpFilePath(
		"save-song-discard-event-XXXX.h2song" );
	pSong->setPath( sScratchPath );
	CPPUNIT_ASSERT( pCAC->setSong( pSong ) );

	auto pQueue = pHydrogen->getEventQueue();
	while ( pQueue->popEvent() != nullptr ) {}

	CPPUNIT_ASSERT( pCAC->saveSong( /* bKeepMissingSamples */ false ) );

	// Discarding the layers only alters the instruments of the current
	// drumkit — both on disk and in the in-memory song — so the event
	// must be scoped accordingly. An UpdateSong(0) would masquerade as
	// a full song replacement and make the GUI reset its undo history
	// for a same-document save (ADR 0026 point 15).
	bool bSawDrumkitLoaded = false;
	bool bSawUpdateSongSaved = false;
	bool bSawUpdateSongLoaded = false;
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pQueue->popEvent() ) != nullptr ) {
		if ( pEvent->getType() == Event::Type::DrumkitLoaded ) {
			bSawDrumkitLoaded = true;
		}
		else if ( pEvent->getType() == Event::Type::UpdateSong ) {
			if ( pEvent->getValue() == 1 ) {
				bSawUpdateSongSaved = true;
			}
			else if ( pEvent->getValue() == 0 ) {
				bSawUpdateSongLoaded = true;
			}
		}
	}
	// DrumkitLoaded refreshes the instrument-facing widgets for the
	// discarded layers and UpdateSong(1) keeps the regular save
	// semantics (window title stand-alone, mirror re-sync via the echo
	// in editor mode — the save is engine-only there). UpdateSong(0)
	// must never fire for a same-document save: the GUI resets the
	// undo stack on it (ADR 0026 point 15).
	CPPUNIT_ASSERT( bSawDrumkitLoaded );
	CPPUNIT_ASSERT( bSawUpdateSongSaved );
	CPPUNIT_ASSERT( ! bSawUpdateSongLoaded );

	// The discard also mutated the in-memory song.
	CPPUNIT_ASSERT( ! pSong->hasMissingSamples() );

	if ( QFile::exists( sScratchPath ) ) {
		QFile::remove( sScratchPath );
	}
	pHydrogen->setSong( Song::getEmptySong( pTestHydrogen() ) );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSetSongPatternSelectionPreservation() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	// Install a song and move the selection off the first pattern.
	CPPUNIT_ASSERT( pCAC->setSong( Song::getEmptySong( pHydrogen ) ) );
	pHydrogen->setSelectedPatternNumber( 3, true, Event::Trigger::Suppress );
	CPPUNIT_ASSERT_EQUAL( 3, pHydrogen->getSelectedPatternNumber() );

	// A re-install of the same document — modeled with the actual
	// mirror-pull mechanism (the IPC XML round-trip preserves both path
	// and uuid) — must keep the selection.
	auto pSongPulled = Song::fromXmlBuffer(
		pHydrogen->getSong()->toXmlBuffer(
			Xml::Flag::KeepMissingSamples | Xml::Flag::Ipc, false ),
		Xml::Flag::Ipc, true, pHydrogen );
	CPPUNIT_ASSERT( pSongPulled != nullptr );
	CPPUNIT_ASSERT( pCAC->setSong( pSongPulled ) );
	CPPUNIT_ASSERT_EQUAL( 3, pHydrogen->getSelectedPatternNumber() );

	// A save-as is the same document at a new location — the engine's
	// saveSongAs() only setPath()s the existing object before the pull —
	// so the re-install must keep the selection too (standalone save-as
	// never re-installs the song at all).
	auto pSongSaveAs = Song::fromXmlBuffer(
		pHydrogen->getSong()->toXmlBuffer(
			Xml::Flag::KeepMissingSamples | Xml::Flag::Ipc, false ),
		Xml::Flag::Ipc, true, pHydrogen );
	CPPUNIT_ASSERT( pSongSaveAs != nullptr );
	pSongSaveAs->setPath( Filesystem::tmpFilePath( "save-as.h2song" ) );
	CPPUNIT_ASSERT( pCAC->setSong( pSongSaveAs ) );
	CPPUNIT_ASSERT_EQUAL( 3, pHydrogen->getSelectedPatternNumber() );

	// A brand-new document must still reset: two fresh empty songs
	// share the sentinel path but never the instance-minted uuid (File
	// > New over an unsaved song).
	CPPUNIT_ASSERT( pCAC->setSong( Song::getEmptySong( pHydrogen ) ) );
	CPPUNIT_ASSERT_EQUAL( 0, pHydrogen->getSelectedPatternNumber() );

	// Installing a different document resets to the beginning.
	auto pSongOther = Song::load(
		QString( H2TEST_FILE( "song/AE_songSizeChanged.h2song" ) ), false,
		pHydrogen );
	CPPUNIT_ASSERT( pSongOther != nullptr );
	CPPUNIT_ASSERT( pCAC->setSong( pSongOther ) );
	CPPUNIT_ASSERT_EQUAL( 0, pHydrogen->getSelectedPatternNumber() );

	// A same-document re-install with fewer patterns must clamp the
	// preserved selection into the new list's range (engine-side
	// deletions before an editor re-pull) — mirroring the instrument
	// clamp in Hydrogen::setSong.
	CPPUNIT_ASSERT( pCAC->setSong( Song::getEmptySong( pHydrogen ) ) );
	pHydrogen->setSelectedPatternNumber( 8, true, Event::Trigger::Suppress );
	auto pSongShrunk = Song::fromXmlBuffer(
		pHydrogen->getSong()->toXmlBuffer(
			Xml::Flag::KeepMissingSamples | Xml::Flag::Ipc, false ),
		Xml::Flag::Ipc, true, pHydrogen );
	CPPUNIT_ASSERT( pSongShrunk != nullptr );
	while ( pSongShrunk->getPatternList()->size() > 3 ) {
		pSongShrunk->getPatternList()->del(
			pSongShrunk->getPatternList()->size() - 1 );
	}
	CPPUNIT_ASSERT( pCAC->setSong( pSongShrunk ) );
	CPPUNIT_ASSERT_EQUAL( 2, pHydrogen->getSelectedPatternNumber() );

	pHydrogen->setSong( Song::getEmptySong( pTestHydrogen() ) );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSetMidiEventMap() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	// Other tests (MidiActionTest) work on the live map — reinstall the
	// original before leaving.
	const auto pOriginalMap = pHydrogen->getPreferences()->getMidiEventMap();

	auto pMap = std::make_shared<MidiEventMap>();
	pMap->registerEvent(
		MidiEvent::Type::MmcStop, Midi::ParameterInvalid,
		MidiAction::fromQStrings( MidiAction::Type::Stop, "", "", "" ),
		Event::Trigger::Suppress, pHydrogen );

	// The base install hands the very object to Preferences — the
	// engine's MIDI dispatch then reads it live.
	CPPUNIT_ASSERT( pCAC->setMidiEventMap( pMap ) );
	CPPUNIT_ASSERT( pHydrogen->getPreferences()->getMidiEventMap() == pMap );
	CPPUNIT_ASSERT(
		pHydrogen->getPreferences()->getMidiEventMap()->getMidiEvents()
			.size() == 1 );

	// A null map is rejected.
	CPPUNIT_ASSERT( ! pCAC->setMidiEventMap( nullptr ) );

	pHydrogen->getPreferences()->setMidiEventMap( pOriginalMap );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSetMidiInstrumentMap() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	// Reinstall the original map before leaving — the live one backs the
	// MIDI I/O of subsequent tests.
	const auto pOriginalMap =
		pHydrogen->getPreferences()->getMidiInstrumentMap();

	auto pMap = std::make_shared<MidiInstrumentMap>();
	pMap->setOutput( MidiInstrumentMap::Output::Constant );

	// The base install hands the very object to Preferences — the
	// engine's MIDI/Sampler dispatch then reads it live.
	CPPUNIT_ASSERT( pCAC->setMidiInstrumentMap( pMap ) );
	CPPUNIT_ASSERT(
		pHydrogen->getPreferences()->getMidiInstrumentMap() == pMap );
	CPPUNIT_ASSERT(
		pHydrogen->getPreferences()->getMidiInstrumentMap()->getOutput() ==
			MidiInstrumentMap::Output::Constant );

	// A null map is rejected.
	CPPUNIT_ASSERT( ! pCAC->setMidiInstrumentMap( nullptr ) );

	pHydrogen->getPreferences()->setMidiInstrumentMap( pOriginalMap );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testSetLastMidiEvent() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	// The pair is one logical value: both members flip together.
	CPPUNIT_ASSERT( pCAC->setLastMidiEvent(
		MidiEvent::Type::CC, Midi::Parameter( 74 ) ) );
	CPPUNIT_ASSERT( pHydrogen->getLastMidiEvent() == MidiEvent::Type::CC );
	CPPUNIT_ASSERT( pHydrogen->getLastMidiEventParameter() ==
					Midi::Parameter( 74 ) );

	// The editor's reset before opening the sense dialog.
	CPPUNIT_ASSERT( pCAC->setLastMidiEvent(
		MidiEvent::Type::Null, Midi::ParameterInvalid ) );
	CPPUNIT_ASSERT( pHydrogen->getLastMidiEvent() == MidiEvent::Type::Null );
	CPPUNIT_ASSERT( pHydrogen->getLastMidiEventParameter() ==
					Midi::ParameterInvalid );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testAddRemoveCustomSoundLibraryDir() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	// Reinstall the original dirs before leaving — the live list
	// backs the sound library of subsequent tests.
	const auto originalDirs =
		pHydrogen->getPreferences()->getCustomSoundLibraryDirs();

	// A real kit in a temporary dir — proves a registered dir reaches
	// the database rescan, not just the Preferences list.
	QTemporaryDir tmpDir( Filesystem::tmpDir() +
						  "custom-lib-dirs-test-XXXXXX" );
	CPPUNIT_ASSERT( tmpDir.isValid() );
	auto pKit = Drumkit::load( H2TEST_FILE( "/drumkits/baseKit/drumkit.xml" ),
							   false, nullptr, false, pHydrogen );
	CPPUNIT_ASSERT( pKit != nullptr );
	CPPUNIT_ASSERT( pKit->save(
		Filesystem::drumkitPathFromDir( tmpDir.path() ), false ) );

	const auto dbHasTmpKit = [&]() {
		for ( const auto& it : pHydrogen->getSoundLibraryDatabase()
					->getDrumkitDatabase() ) {
			if ( it.first.contains( "custom-lib-dirs-test" ) ) {
				return true;
			}
		}
		return false;
	};

	// Add: the dir enters the Preferences list and the database
	// rescans it.
	CPPUNIT_ASSERT( pCAC->addCustomSoundLibraryDir( tmpDir.path() ) );
	CPPUNIT_ASSERT( pHydrogen->getPreferences()->getCustomSoundLibraryDirs()
					.contains( tmpDir.path() ) );
	CPPUNIT_ASSERT( dbHasTmpKit() );

	// Idempotent: a second add changes nothing.
	CPPUNIT_ASSERT( pCAC->addCustomSoundLibraryDir( tmpDir.path() ) );
	CPPUNIT_ASSERT(
		pHydrogen->getPreferences()->getCustomSoundLibraryDirs().size() ==
		originalDirs.size() + 1 );

	// Remove: the dir leaves both the list and the database.
	CPPUNIT_ASSERT( pCAC->removeCustomSoundLibraryDir( tmpDir.path() ) );
	CPPUNIT_ASSERT( ! pHydrogen->getPreferences()->getCustomSoundLibraryDirs()
					.contains( tmpDir.path() ) );
	CPPUNIT_ASSERT( ! dbHasTmpKit() );

	// Idempotent: removing a dir that is not registered is a no-op.
	CPPUNIT_ASSERT( pCAC->removeCustomSoundLibraryDir( tmpDir.path() ) );

	// A dir without any kit registers fine but leaves the database
	// unchanged.
	QTemporaryDir emptyDir( Filesystem::tmpDir() +
							"custom-lib-dirs-empty-XXXXXX" );
	CPPUNIT_ASSERT( emptyDir.isValid() );
	CPPUNIT_ASSERT( pCAC->addCustomSoundLibraryDir( emptyDir.path() ) );
	CPPUNIT_ASSERT( pHydrogen->getPreferences()->getCustomSoundLibraryDirs()
					.contains( emptyDir.path() ) );
	for ( const auto& it : pHydrogen->getSoundLibraryDatabase()
					->getDrumkitDatabase() ) {
		CPPUNIT_ASSERT( ! it.first.contains( "custom-lib-dirs-empty" ) );
	}
	CPPUNIT_ASSERT( pCAC->removeCustomSoundLibraryDir( emptyDir.path() ) );

	// Empty paths are rejected.
	CPPUNIT_ASSERT( ! pCAC->addCustomSoundLibraryDir( "" ) );
	CPPUNIT_ASSERT( ! pCAC->removeCustomSoundLibraryDir( "" ) );

	pHydrogen->getPreferences()->setCustomSoundLibraryDirs( originalDirs );
	pHydrogen->getSoundLibraryDatabase()->update();

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testExportSong() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	auto pSong = Song::load(
		QString( H2TEST_FILE( "functional/test_adsr.h2song" ) ), false,
		pHydrogen );
	ASSERT_SONG( pSong );
	CPPUNIT_ASSERT( pCAC->setSong( pSong ) );

	// Capture the state a session has to restore afterwards.
	const int nOriginalBatchMode =
		pHydrogen->getPreferences()->getRubberBandBatchMode();
	const auto originalInterpolateMode = pHydrogen->getInterpolateMode();
	// Flip both, so the restore is observable.
	const bool bFlippedBatchMode = ! ( nOriginalBatchMode != 0 );
	const auto overriddenInterpolateMode =
		originalInterpolateMode == Interpolation::InterpolateMode::Linear ?
		Interpolation::InterpolateMode::Cosine :
		Interpolation::InterpolateMode::Linear;

	QTemporaryDir tmpDir( Filesystem::tmpDir() +
						  "export-song-test-XXXXXX" );
	CPPUNIT_ASSERT( tmpDir.isValid() );
	const QString sFile1 = tmpDir.path() + "/export-1.wav";
	const QString sFile2 = tmpDir.path() + "/export-2.wav";

	// Empty plan: acknowledged no-op, no session.
	CPPUNIT_ASSERT( pCAC->exportSong(
		48000, 16, 0.0, Interpolation::InterpolateMode::Linear, false,
		{} ) );
	CPPUNIT_ASSERT( ! pHydrogen->getIsExportSessionActive() );

	// Single-render plan: the engine renders and tears the session
	// down itself, restoring everything it parked.
	std::vector<ExportRender> renders;
	renders.push_back( ExportRender{ sFile1, {} } );
	CPPUNIT_ASSERT( pCAC->exportSong(
		48000, 16, 0.0, overriddenInterpolateMode, bFlippedBatchMode,
		renders ) );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pHydrogen->getIsExportSessionActive();
	}, 30000 ) );
	CPPUNIT_ASSERT( QFileInfo( sFile1 ).size() > 0 );
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pHydrogen->getAudioDriver() ) == nullptr );
	CPPUNIT_ASSERT( pHydrogen->getPreferences()->getRubberBandBatchMode() ==
					nOriginalBatchMode );
	CPPUNIT_ASSERT( pHydrogen->getInterpolateMode() ==
					originalInterpolateMode );

	// Two-render plan with a per-instrument exclusion (trackout
	// shape). The exclusion semantics themselves are covered by
	// AudioExportTest; this exercises the plan plumbing.
	auto pExcluded = pSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pExcluded != nullptr );
	renders.clear();
	renders.push_back( ExportRender{ sFile1, {} } );
	renders.push_back( ExportRender{ sFile2, { pExcluded->getUuid() } } );
	CPPUNIT_ASSERT( pCAC->exportSong(
		48000, 16, 0.0, Interpolation::InterpolateMode::Linear, false,
		renders ) );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pHydrogen->getIsExportSessionActive();
	}, 30000 ) );
	CPPUNIT_ASSERT( QFileInfo( sFile1 ).size() > 0 );
	CPPUNIT_ASSERT( QFileInfo( sFile2 ).size() > 0 );
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pHydrogen->getAudioDriver() ) == nullptr );

	// The finished session must leave the transport at rest: neither
	// rolling (state Playing) nor armed to roll (a sticky pending
	// Playing the next process callback would apply) — a wedged
	// engine would ignore later stops, which only queue a pending
	// transition.
	CPPUNIT_ASSERT( pHydrogen->getAudioEngine()->getState() !=
					AudioEngine::State::Playing );
	CPPUNIT_ASSERT( pHydrogen->getAudioEngine()->getNextState() !=
					AudioEngine::State::Playing );

	// Cancel: stopping right after the plan was armed is safe and
	// restores everything.
	renders.clear();
	renders.push_back( ExportRender{ sFile2, {} } );
	CPPUNIT_ASSERT( pCAC->exportSong(
		48000, 16, 0.0, Interpolation::InterpolateMode::Linear, false,
		renders ) );
	pCAC->stopExportSession();
	CPPUNIT_ASSERT( ! pHydrogen->getIsExportSessionActive() );
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pHydrogen->getAudioDriver() ) == nullptr );
	CPPUNIT_ASSERT( pHydrogen->getPreferences()->getRubberBandBatchMode() ==
					nOriginalBatchMode );

	// Idempotent: stopping without an active session is a no-op.
	pCAC->stopExportSession();
	CPPUNIT_ASSERT( ! pHydrogen->getIsExportSessionActive() );

	___INFOLOG( "passed" );
}
