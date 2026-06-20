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
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>
#include <core/Helpers/Filesystem.h>

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
	CPPUNIT_ASSERT( pCAC->addOrRemoveNote(
		12 /*position*/, nId, sType, 0 /*pattern*/, -1 /*length*/,
		0.8f, 0.f, 0.f, nKey, nOctave, 1.0f,
		false /*delete*/, false /*noteOff*/, true /*mapped*/ ) );
	CPPUNIT_ASSERT_EQUAL( nNotesBefore + 1,
						  static_cast<int>( pPattern->getNotes()->size() ) );
	CPPUNIT_ASSERT( pPattern->findNote(
						12, pInstrument->getId(), sType, pProbe->getKey(),
						pProbe->getOctave() ) != nullptr );

	// Remove it again.
	CPPUNIT_ASSERT( pCAC->addOrRemoveNote(
		12, nId, sType, 0, -1, 0.8f, 0.f, 0.f, nKey, nOctave, 1.0f,
		true /*delete*/, false, true ) );
	CPPUNIT_ASSERT_EQUAL( nNotesBefore,
						  static_cast<int>( pPattern->getNotes()->size() ) );
	CPPUNIT_ASSERT( pPattern->findNote(
						12, pInstrument->getId(), sType, pProbe->getKey(),
						pProbe->getOctave() ) == nullptr );

	// An out-of-range pattern index fails gracefully.
	CPPUNIT_ASSERT( ! pCAC->addOrRemoveNote(
		12, nId, sType, 999, -1, 0.8f, 0.f, 0.f, nKey, nOctave, 1.0f,
		false, false, true ) );

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testOverwriteNotes() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();

	pCAC->setSong( Song::getEmptySong( pHydrogen ) );
	auto pSong = pHydrogen->getSong();
	auto pInstrument = pSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pPattern = pSong->getPatternList()->get( 0 );
	CPPUNIT_ASSERT( pPattern != nullptr );

	// Two notes sharing one slot (same instrument/key/octave at position 5).
	auto pKept = std::make_shared<Note>( pInstrument, 5, 0.8f, 0.f, -1 );
	auto pOther = std::make_shared<Note>( pInstrument, 5, 0.4f, 0.f, -1 );
	pPattern->insertNote( pKept );
	pPattern->insertNote( pOther );
	const int nBaseline = static_cast<int>( pPattern->getNotes()->size() );

	// Overwrite keeps one note at the slot and erases the duplicate.
	std::vector<std::shared_ptr<Note>> selected{ pKept };
	CPPUNIT_ASSERT( pCAC->overwriteNotes( 0, selected ) );
	CPPUNIT_ASSERT_EQUAL( nBaseline - 1,
						  static_cast<int>( pPattern->getNotes()->size() ) );

	// Restore re-inserts the overwritten note.
	std::vector<std::shared_ptr<Note>> overwritten{ pOther };
	CPPUNIT_ASSERT( pCAC->restoreOverwrittenNotes( 0, overwritten ) );
	CPPUNIT_ASSERT_EQUAL( nBaseline,
						  static_cast<int>( pPattern->getNotes()->size() ) );

	// Out-of-range pattern index fails gracefully.
	CPPUNIT_ASSERT( ! pCAC->overwriteNotes( 999, selected ) );
	CPPUNIT_ASSERT( ! pCAC->restoreOverwrittenNotes( 999, overwritten ) );

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
