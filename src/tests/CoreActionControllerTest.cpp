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
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>

#include <stdio.h>

using namespace H2Core;

void CoreActionControllerTest::setUp() {
	
	m_pHydrogen = Hydrogen::get_instance();
	m_sFileName = Filesystem::tmp_dir().append( "test1.h2song" );
	m_sFileName2 = Filesystem::tmp_dir().append( "test2.h2song" );
	m_sFileNameImproper = Filesystem::tmp_dir().append( "test3.h2song" );
	
	m_pHydrogen->setSong( Song::getEmptySong() );
}

void CoreActionControllerTest::tearDown() {
	
	m_pHydrogen->setSong( Song::getEmptySong() );

	if ( QFile::exists( m_sFileName ) ) {
		QFile::remove( m_sFileName );
	}
	if ( QFile::exists( m_sFileName2 ) ) {
		QFile::remove( m_sFileName2 );
	}
	// The improper file name must not be used to create a file.
	CPPUNIT_ASSERT( !QFile::exists( m_sFileNameImproper ) );
}

void CoreActionControllerTest::testSessionManagement() {
	___INFOLOG( "" );
	
	QTemporaryFile fileWrongName;
	if ( fileWrongName.open() ) {
		m_sFileNameImproper = fileWrongName.fileName();
	}

	// Create a new song with a proper file name and existing and
	// writable file.
	m_sFileName = QString( "%1.h2song" ).arg( m_sFileNameImproper );
	QFile fileProperName( m_sFileName );
	if ( fileProperName.open( QIODevice::ReadWrite ) ) {

		auto pSong = H2Core::Song::getEmptySong();
		pSong->setFilename( fileProperName.fileName() );
		CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
		CPPUNIT_ASSERT( m_sFileName == m_pHydrogen->getSong()->getFilename() );
	
		// -----------------------------------------------------------
		// Test CoreActionController::saveSong()
		// -----------------------------------------------------------
		
		CPPUNIT_ASSERT( H2Core::CoreActionController::saveSong() );

		// -----------------------------------------------------------
	
	}
	
	// Create a new song with proper a file name but no existing file.
	std::shared_ptr<H2Core::Song> pSong;
	m_sFileName2 = QString( "%1_new.h2song" ).arg( m_sFileNameImproper );
	pSong = H2Core::Song::getEmptySong();
	pSong->setFilename( m_sFileName2 );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( m_sFileName2 == m_pHydrogen->getSong()->getFilename() );

	// ---------------------------------------------------------------
	// Test CoreActionController::loadSong() and ::setSong();
	// ---------------------------------------------------------------

	// Attempt to load a non-existing song.
	pSong = H2Core::CoreActionController::loadSong( m_sFileNameImproper );
	CPPUNIT_ASSERT( pSong == nullptr );
	CPPUNIT_ASSERT( ! H2Core::CoreActionController::setSong( pSong ) );
	
	// The previous action should have not affected the current song.
	CPPUNIT_ASSERT( m_sFileName2 == m_pHydrogen->getSong()->getFilename() );
	CPPUNIT_ASSERT( pSong != m_pHydrogen->getSong() );
	
	// Load the first song (which was saved).
	pSong = H2Core::CoreActionController::loadSong( m_sFileName );
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( m_sFileName == m_pHydrogen->getSong()->getFilename() );
	CPPUNIT_ASSERT( pSong == m_pHydrogen->getSong() );

	// Attempt to load the second song. This will fail since it should not be
	// present on disk.
	CPPUNIT_ASSERT( H2Core::CoreActionController::loadSong( m_sFileName2 ) ==
					nullptr );
	
	// ---------------------------------------------------------------
	// Test CoreActionController::saveSongAs()
	// ---------------------------------------------------------------
	
	// But we can, instead, make a copy of the current song by saving
	// it to m_sFileName2.
	CPPUNIT_ASSERT( H2Core::CoreActionController::saveSongAs( m_sFileName2 ) );
	
	// Check if everything worked out.
	pSong = H2Core::CoreActionController::loadSong( m_sFileName );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( m_sFileName == m_pHydrogen->getSong()->getFilename() );
	pSong = H2Core::CoreActionController::loadSong( m_sFileName2 );
	CPPUNIT_ASSERT( H2Core::CoreActionController::setSong( pSong ) );
	CPPUNIT_ASSERT( m_sFileName2 == m_pHydrogen->getSong()->getFilename() );

	// ---------------------------------------------------------------
	
	CPPUNIT_ASSERT( fileProperName.remove() );
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
