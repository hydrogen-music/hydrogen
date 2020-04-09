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

void CoreActionControllerTest::testNewSong() {
	
	// ---------------------------------------------------------------
	// Test with improper file name.
	
	QTemporaryFile fileWrongName;
	QString sFileName;
	if ( fileWrongName.open() ) {
		
		CPPUNIT_ASSERT( !m_pController->newSong( fileWrongName.fileName() ) );
		
		sFileName = fileWrongName.fileName();
	}

	// ---------------------------------------------------------------
	// Test with proper file name and existing and writable file.
	
	QFile fileProperName( QString( "%1.h2song" ).arg( sFileName ) );
	if ( fileProperName.open( QIODevice::ReadWrite ) ) {
		
		CPPUNIT_ASSERT( m_pController->newSong( fileProperName.fileName() ) );
		CPPUNIT_ASSERT( QString( "%1.h2song" ).arg( sFileName ) ==
						m_pHydrogen->getSong()->get_filename() );
	}
	
	CPPUNIT_ASSERT( fileProperName.remove() );
	
	// ---------------------------------------------------------------
	// Test with proper file name but no existing file.
	
	CPPUNIT_ASSERT( m_pController->newSong( QString( "%1_new.h2song" ).arg( sFileName ) ) ); 
	CPPUNIT_ASSERT( QString( "%1_new.h2song" ).arg( sFileName ) ==
					m_pHydrogen->getSong()->get_filename() );
}

void CoreActionControllerTest::testOpenSong() {
	std::cout << "[testOpenSong]" << std::endl;
}

void CoreActionControllerTest::testSaveSong() {
	std::cout << "[testSaveSong]" << std::endl;
}

void CoreActionControllerTest::testSaveSongAs() {
	std::cout << "[testSaveSongAs]" << std::endl;
}

void CoreActionControllerTest::testQuit() {
	std::cout << "[testQuit]" << std::endl;
}
