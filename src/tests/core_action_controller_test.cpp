#include "core_action_controller_test.h"

#include <stdio.h>

using namespace H2Core;

void CoreActionControllerTest::setUp() {
	
	m_pHydrogen = Hydrogen::get_instance();
	m_pController = m_pHydrogen->getCoreActionController();
	
	m_pHydrogen->setSong( Song::get_empty_song() );
}

void CoreActionControllerTest::tearDown() {
	
	m_pHydrogen->setSong( Song::get_empty_song() );
}

void CoreActionControllerTest::testSessionManagement() {
	
	// ---------------------------------------------------------------
	// Test CoreActionController::newSong()
	// ---------------------------------------------------------------
	
	// Attempting to create a new song with an improper file name.
	QTemporaryFile fileWrongName;
	QString sFileNameImproper;
	if ( fileWrongName.open() ) {
		
		CPPUNIT_ASSERT( !m_pController->newSong( fileWrongName.fileName() ) );
		
		sFileNameImproper = fileWrongName.fileName();
	}

	// Create a new song with a proper file name and existing and
	// writable file.
	QString sFileName = QString( "%1.h2song" ).arg( sFileNameImproper );
	QFile fileProperName( sFileName );
	if ( fileProperName.open( QIODevice::ReadWrite ) ) {
		
		CPPUNIT_ASSERT( m_pController->newSong( fileProperName.fileName() ) );
		CPPUNIT_ASSERT( sFileName == m_pHydrogen->getSong()->get_filename() );
	
		// -----------------------------------------------------------
		// Test CoreActionController::saveSong()
		// -----------------------------------------------------------
		
		CPPUNIT_ASSERT( m_pController->saveSong() );

		// -----------------------------------------------------------
	
	}
	
	// Create a new song with proper a file name but no existing file.
	QString sFileName2 = QString( "%1_new.h2song" ).arg( sFileNameImproper );
	CPPUNIT_ASSERT( m_pController->newSong( sFileName2 ) ); 
	CPPUNIT_ASSERT( sFileName2 == m_pHydrogen->getSong()->get_filename() );
	
	// ---------------------------------------------------------------
	// Test CoreActionController::openSong()
	// ---------------------------------------------------------------
	
	// Attempt to load a non-existing song.
	CPPUNIT_ASSERT( !m_pController->openSong( sFileNameImproper ) );
	
	// The previous action should have not affected the current song.
	CPPUNIT_ASSERT( sFileName2 == m_pHydrogen->getSong()->get_filename() );
	
	// Load the first song (which was saved).
	CPPUNIT_ASSERT( m_pController->openSong( sFileName ) );
	CPPUNIT_ASSERT( sFileName == m_pHydrogen->getSong()->get_filename() );

	// Attempt to load the second song. This will fail since Hydrogen
	// did not stored the song to disk.
	CPPUNIT_ASSERT( !m_pController->openSong( sFileName2 ) );
	
	// ---------------------------------------------------------------
	// Test CoreActionController::saveSongAs()
	// ---------------------------------------------------------------
	
	// But we can, instead, make a copy of the current song by saving
	// it to sFileName2.
	CPPUNIT_ASSERT( m_pController->saveSongAs( sFileName2 ) );
	
	// Check if everything worked out.
	CPPUNIT_ASSERT( m_pController->openSong( sFileName ) );
	CPPUNIT_ASSERT( sFileName == m_pHydrogen->getSong()->get_filename() );
	CPPUNIT_ASSERT( m_pController->openSong( sFileName2 ) );
	CPPUNIT_ASSERT( sFileName2 == m_pHydrogen->getSong()->get_filename() );

	// ---------------------------------------------------------------
	
	CPPUNIT_ASSERT( fileProperName.remove() );
}

void CoreActionControllerTest::testIsSongPathValid() {
	
	// Is not absolute.
	CPPUNIT_ASSERT( !m_pController->isSongPathValid( "test.h2song" ) );

	// Improper suffix.
	CPPUNIT_ASSERT( !m_pController->isSongPathValid( "test.test" ) );
	
	QString sValidPath = QString( "%1/test.h2song" ).arg( QDir::tempPath() );
	CPPUNIT_ASSERT( m_pController->isSongPathValid( sValidPath ) );
	
}
