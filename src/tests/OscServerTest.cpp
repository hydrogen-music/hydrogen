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

#include <core/config.h>

#ifdef H2CORE_HAVE_OSC

#include "OscServerTest.h"
#include <core/Preferences/Preferences.h>
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
	___INFOLOG( "" );

	// Create an object with which we will send messages to the custom
	// OSC server.
	lo::Address hydrogenOSC("localhost", "7362" );
	
	// Create the new song.
	const auto sValidPathLocal8Bit = m_sValidPath.toLocal8Bit();
	hydrogenOSC.send("/Hydrogen/NEW_SONG", "s", 
					 sValidPathLocal8Bit.data());
	
	// Store it to disk so we can retrieve it later on.
	hydrogenOSC.send("/Hydrogen/SAVE_SONG");
	WAIT(m_sValidPath == m_pHydrogen->getSong()->getFilename());
	CPPUNIT_ASSERT( m_sValidPath == m_pHydrogen->getSong()->getFilename() );

	// Store a copy in another file.
	const auto sValidPath2Local8Bit = m_sValidPath2.toLocal8Bit();
	hydrogenOSC.send("/Hydrogen/SAVE_SONG_AS", "s",
					 sValidPath2Local8Bit.data());
	WAIT(m_sValidPath2 == m_pHydrogen->getSong()->getFilename());
	CPPUNIT_ASSERT( m_sValidPath2 == m_pHydrogen->getSong()->getFilename() );
	
	// Load the first song. This will only be successful if the
	// SAVE_SONG did work.
	hydrogenOSC.send("/Hydrogen/OPEN_SONG", "s",
					 sValidPathLocal8Bit.data());
	WAIT(m_sValidPath == m_pHydrogen->getSong()->getFilename());
	CPPUNIT_ASSERT( m_sValidPath == m_pHydrogen->getSong()->getFilename() );
	___INFOLOG( "passed" );
}

#endif
