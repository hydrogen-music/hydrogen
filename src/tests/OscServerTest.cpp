#include <core/config.h>

#ifdef H2CORE_HAVE_OSC

#include "OscServerTest.h"
#include <core/Preferences.h>
#include <core/OscServer.h>

#include <QTest>

using namespace H2Core;


// Wait 10, 30, 70, 150, 310, or 630ms for Hydrogen to process the
// received messages.
#define WAIT(X) {													\
		int timeBase = 10;											\
		for ( int ii = 0; ii < 6; ii++ ) {							\
			QTest::qSleep( timeBase );								\
			if ( X ) {												\
				break;												\
			}														\
			timeBase *= 2;											\
		}															\
	}

void OscServerTest::setUp(){
	m_pHydrogen = Hydrogen::get_instance();
	
	// Create, set up, and start a dummy OSC server.
	m_pServerThread = new lo::ServerThread( 7362 );
	CPPUNIT_ASSERT( m_pServerThread->is_valid() );
	
	m_pServerThread->add_method( nullptr, nullptr, 
								 OscServer::generic_handler, nullptr);
	m_pServerThread->add_method("/Hydrogen/NEW_SONG", "s", 
								OscServer::NEW_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/OPEN_SONG", "s", 
								OscServer::OPEN_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_SONG", "", 
								OscServer::SAVE_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_SONG_AS", "s", 
								OscServer::SAVE_SONG_AS_Handler);
	m_pServerThread->add_method("/Hydrogen/QUIT", "", 
								OscServer::QUIT_Handler);
	
	m_pServerThread->start();

	// Proper a temporary file names.
	m_sValidPath = QString( "%1/hydrogen_osc_server_test.h2song" )
		.arg( QDir::tempPath() );
	m_sValidPath2 = QString( "%1/hydrogen_osc_server_test2.h2song" )
		.arg( QDir::tempPath() );
}

void OscServerTest::tearDown(){
	
	delete m_pServerThread;
	
	// Delete all temporary files
	if ( QFile::exists( m_sValidPath ) ) {
		QFile::remove( m_sValidPath );
	}
	if ( QFile::exists( m_sValidPath2 ) ) {
		QFile::remove( m_sValidPath2 );
	}
}

void OscServerTest::testSessionManagement(){

	// Create an object with which we will send messages to the custom
	// OSC server.
	lo::Address hydrogenOSC("localhost", "7362" );
	
	// Create the new song.
	hydrogenOSC.send("/Hydrogen/NEW_SONG", "s", 
					 m_sValidPath.toLocal8Bit().data());
	
	// Store it to disk so we can retrieve it later on.
	hydrogenOSC.send("/Hydrogen/SAVE_SONG");
	WAIT(m_sValidPath == m_pHydrogen->getSong()->get_filename());
	CPPUNIT_ASSERT( m_sValidPath == m_pHydrogen->getSong()->get_filename() );

	// Store a copy in another file.
	hydrogenOSC.send("/Hydrogen/SAVE_SONG_AS", "s",
					 m_sValidPath2.toLocal8Bit().data());	
	WAIT(m_sValidPath2 == m_pHydrogen->getSong()->get_filename());
	CPPUNIT_ASSERT( m_sValidPath2 == m_pHydrogen->getSong()->get_filename() );
	
	// Load the first song. This will only be successful if the
	// SAVE_SONG did work.
	hydrogenOSC.send("/Hydrogen/OPEN_SONG", "s",
					 m_sValidPath.toLocal8Bit().data());
	WAIT(m_sValidPath == m_pHydrogen->getSong()->get_filename());
	CPPUNIT_ASSERT( m_sValidPath == m_pHydrogen->getSong()->get_filename() );
}

#endif
