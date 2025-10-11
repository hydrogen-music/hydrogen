/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/Helpers/Filesystem.h>

#include <chrono>
#include <thread>

using namespace H2Core;

void CoreActionControllerTest::testCountIn() {
	___INFOLOG( "" );
	auto pSongSizeChanged = Song::load(
		QString( H2TEST_FILE( "song/AE_songSizeChanged.h2song" ) ) );
	ASSERT_SONG( pSongSizeChanged );
	CoreActionController::setSong( pSongSizeChanged );
	CoreActionController::activateSongMode( true );

	// Move to different columns in song mode and start the count in. Since
	// patterns of different length are present in those columns, we should see
	// different numbers of count in ticks.

	auto countInTicksForColumn = []( int nColumn ) {
		auto pHydrogen = Hydrogen::get_instance();
		auto pAudioEngine = pHydrogen->getAudioEngine();

		CPPUNIT_ASSERT( CoreActionController::locateToColumn( nColumn ) );
		CPPUNIT_ASSERT( CoreActionController::startCountIn() );
		CPPUNIT_ASSERT( CoreActionController::setBpm( MAX_BPM ) );

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

void CoreActionControllerTest::testSessionManagement() {
	___INFOLOG( "" );
	auto pHydrogen = Hydrogen::get_instance();
	auto sFileName = Filesystem::tmp_dir().append( "test1.h2song" );
	auto sFileName2 = Filesystem::tmp_dir().append( "test2.h2song" );

	pHydrogen->setSong( Song::getEmptySong() );
	
	QTemporaryFile fileWrongName;
	CPPUNIT_ASSERT( fileWrongName.open() );
	const auto sFileNameImproper = fileWrongName.fileName();

	// Create a new song with a proper file name and existing and
	// writable file.
	sFileName = QString( "%1.h2song" ).arg( sFileNameImproper );
	QFile fileProperName( sFileName );
	if ( fileProperName.open( QIODevice::ReadWrite ) ) {

		auto pSong = H2Core::Song::getEmptySong();
		pSong->setFileName( fileProperName.fileName() );
		CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
		CPPUNIT_ASSERT( sFileName == pHydrogen->getSong()->getFileName() );
	
		// -----------------------------------------------------------
		// Test CoreActionController::saveSong()
		// -----------------------------------------------------------
		
		CPPUNIT_ASSERT( H2Core::CoreActionController::saveSong( true ) );

		// -----------------------------------------------------------
	
	}
	
	// Create a new song with proper a file name but no existing file.
	std::shared_ptr<H2Core::Song> pSong;
	sFileName2 = QString( "%1_new.h2song" ).arg( sFileNameImproper );
	pSong = H2Core::Song::getEmptySong();
	pSong->setFileName( sFileName2 );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( sFileName2 == pHydrogen->getSong()->getFileName() );

	// ---------------------------------------------------------------
	// Test CoreActionController::loadSong() and ::setSong();
	// ---------------------------------------------------------------

	// Attempt to load a non-existing song.
	pSong = H2Core::CoreActionController::loadSong( sFileNameImproper );
	CPPUNIT_ASSERT( pSong == nullptr );
	CPPUNIT_ASSERT( ! H2Core::CoreActionController::setSong( pSong ) );
	
	// The previous action should have not affected the current song.
	CPPUNIT_ASSERT( sFileName2 == pHydrogen->getSong()->getFileName() );
	CPPUNIT_ASSERT( pSong != pHydrogen->getSong() );
	
	// Load the first song (which was saved).
	pSong = H2Core::CoreActionController::loadSong( sFileName );
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( sFileName == pHydrogen->getSong()->getFileName() );
	CPPUNIT_ASSERT( pSong == pHydrogen->getSong() );

	// Attempt to load the second song. This will fail since it should not be
	// present on disk.
	CPPUNIT_ASSERT( H2Core::CoreActionController::loadSong( sFileName2 ) ==
					nullptr );
	
	// ---------------------------------------------------------------
	// Test CoreActionController::saveSongAs()
	// ---------------------------------------------------------------
	
	// But we can, instead, make a copy of the current song by saving
	// it to sFileName2.
	CPPUNIT_ASSERT( H2Core::CoreActionController::saveSongAs( sFileName2, true ) );
	
	// Check if everything worked out.
	pSong = H2Core::CoreActionController::loadSong( sFileName );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( sFileName == pHydrogen->getSong()->getFileName() );
	pSong = H2Core::CoreActionController::loadSong( sFileName2 );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( sFileName2 == pHydrogen->getSong()->getFileName() );

	// ---------------------------------------------------------------
	
	CPPUNIT_ASSERT( fileProperName.remove() );

	// ---------------------------------------------------------------
	
	pHydrogen->setSong( Song::getEmptySong() );

	if ( QFile::exists( sFileName ) ) {
		QFile::remove( sFileName );
	}
	if ( QFile::exists( sFileName2 ) ) {
		QFile::remove( sFileName2 );
	}

	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testIsPathValid() {
	___INFOLOG( "" );
	
	// Is not absolute.
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Type::Song, "test.h2song" ) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Type::Playlist, "test.h2playlist" ) );

	// Improper suffix.
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Type::Song, "test.test" ) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Type::Playlist, "test.test" ) );
	
	QString sValidSongPath = QString( "%1/test.h2song" ).arg( QDir::tempPath() );
	CPPUNIT_ASSERT( Filesystem::isPathValid(
						Filesystem::Type::Song, sValidSongPath ) );
	QString sValidPlaylistPath = QString( "%1/test.h2playlist" ).arg( QDir::tempPath() );
	CPPUNIT_ASSERT( Filesystem::isPathValid(
						Filesystem::Type::Playlist, sValidPlaylistPath ) );
	
	___INFOLOG( "passed" );
}
