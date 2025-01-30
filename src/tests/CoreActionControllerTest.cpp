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
#include <core/Helpers/Filesystem.h>

#include <stdio.h>

using namespace H2Core;

void CoreActionControllerTest::setUp() {
	
	m_pHydrogen = Hydrogen::get_instance();
	m_pController = m_pHydrogen->getCoreActionController();
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
	
	// ---------------------------------------------------------------
	// Test CoreActionController::newSong()
	// ---------------------------------------------------------------
	
	// Attempting to create a new song with an improper file name.
	QTemporaryFile fileWrongName;
	if ( fileWrongName.open() ) {
		
		CPPUNIT_ASSERT( !m_pController->newSong( fileWrongName.fileName() ) );
		
		m_sFileNameImproper = fileWrongName.fileName();
	}

	// Create a new song with a proper file name and existing and
	// writable file.
	m_sFileName = QString( "%1.h2song" ).arg( m_sFileNameImproper );
	QFile fileProperName( m_sFileName );
	if ( fileProperName.open( QIODevice::ReadWrite ) ) {
		
		CPPUNIT_ASSERT( m_pController->newSong( fileProperName.fileName() ) );
		CPPUNIT_ASSERT( m_sFileName == m_pHydrogen->getSong()->getFilename() );
	
		// -----------------------------------------------------------
		// Test CoreActionController::saveSong()
		// -----------------------------------------------------------
		
		CPPUNIT_ASSERT( m_pController->saveSong() );

		// -----------------------------------------------------------
	
	}
	
	// Create a new song with proper a file name but no existing file.
	m_sFileName2 = QString( "%1_new.h2song" ).arg( m_sFileNameImproper );
	CPPUNIT_ASSERT( m_pController->newSong( m_sFileName2 ) ); 
	CPPUNIT_ASSERT( m_sFileName2 == m_pHydrogen->getSong()->getFilename() );
	
	// ---------------------------------------------------------------
	// Test CoreActionController::openSong()
	// ---------------------------------------------------------------
	
	// Attempt to load a non-existing song.
	CPPUNIT_ASSERT( !m_pController->openSong( m_sFileNameImproper ) );
	
	// The previous action should have not affected the current song.
	CPPUNIT_ASSERT( m_sFileName2 == m_pHydrogen->getSong()->getFilename() );
	
	// Load the first song (which was saved).
	CPPUNIT_ASSERT( m_pController->openSong( m_sFileName ) );
	CPPUNIT_ASSERT( m_sFileName == m_pHydrogen->getSong()->getFilename() );

	// Attempt to load the second song. This will fail since Hydrogen
	// did not stored the song to disk.
	CPPUNIT_ASSERT( !m_pController->openSong( m_sFileName2 ) );
	
	// ---------------------------------------------------------------
	// Test CoreActionController::saveSongAs()
	// ---------------------------------------------------------------
	
	// But we can, instead, make a copy of the current song by saving
	// it to m_sFileName2.
	CPPUNIT_ASSERT( m_pController->saveSongAs( m_sFileName2 ) );
	
	// Check if everything worked out.
	CPPUNIT_ASSERT( m_pController->openSong( m_sFileName ) );
	CPPUNIT_ASSERT( m_sFileName == m_pHydrogen->getSong()->getFilename() );
	CPPUNIT_ASSERT( m_pController->openSong( m_sFileName2 ) );
	CPPUNIT_ASSERT( m_sFileName2 == m_pHydrogen->getSong()->getFilename() );

	// ---------------------------------------------------------------
	
	CPPUNIT_ASSERT( fileProperName.remove() );
	___INFOLOG( "passed" );
}

void CoreActionControllerTest::testIsSongPathValid() {
	___INFOLOG( "" );
	
	// Is not absolute.
	CPPUNIT_ASSERT( !Filesystem::isSongPathValid( "test.h2song" ) );

	// Improper suffix.
	CPPUNIT_ASSERT( !Filesystem::isSongPathValid( "test.test" ) );
	
	QString sValidPath = QString( "%1/test.h2song" ).arg( QDir::tempPath() );
	CPPUNIT_ASSERT( Filesystem::isSongPathValid( sValidPath ) );
	
	___INFOLOG( "passed" );
}
