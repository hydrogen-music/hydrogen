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

#include "TimeTest.h"
#include <core/CoreActionController.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>

#include <cmath>
#include <QTest>

using namespace H2Core;

void TimeTest::setUp(){
	
	m_sValidPath = QString( "%1/hydrogen_time_test.h2song" )
		.arg( QDir::tempPath() );

	// We need a song that has at least the maximum pattern group
	// number provided in testElapsedTime(). An empty one won't do it.
	auto pCoreActionController = Hydrogen::get_instance()->getCoreActionController();
	pCoreActionController->openSong( QString( "%1/GM_kit_demo3.h2song" ).arg( Filesystem::demos_dir() ) );
	pCoreActionController->saveSongAs( m_sValidPath );
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 3, 100 );
	pCoreActionController->addTempoMarker( 5, 40 );
	pCoreActionController->addTempoMarker( 7, 200 );
}

void TimeTest::tearDown(){

	Hydrogen::get_instance()->getCoreActionController()->activateTimeline( false );
	
	// Delete all temporary files
	if ( QFile::exists( m_sValidPath ) ) {
		QFile::remove( m_sValidPath );
	}
}

float TimeTest::locateAndLookupTime( int nPatternPos ){
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	pCoreActionController->locateToColumn( nPatternPos );
	return pAudioEngine->getElapsedTime();
}

void TimeTest::testElapsedTime(){
	___INFOLOG( "" );

	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 0 ) - 0 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 1 ) - 2 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 2 ) - 4 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 3 ) - 6 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 4 ) - 8.4 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 5 ) - 10.8 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 6 ) - 16.8 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 7 ) - 22.8 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 8 ) - 24 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 1 ) - 2 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 5 ) - 10.8 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 2 ) - 4 ) < 0.0001 );
	___INFOLOG( "passed" );
}
