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


#include "TimeTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Song.h>
#include <core/config.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Time.h>
#include <core/Helpers/TimeHelper.h>
#include <core/Hydrogen.h>

#include <cmath>
#include <QTest>

using namespace H2Core;

void TimeTest::setUp(){
	
	m_sValidPath = QString( "%1/hydrogen_time_test.h2song" )
		.arg( QDir::tempPath() );

	// We need a song that has at least the maximum pattern group
	// number provided in testElapsedTime(). An empty one won't do it.
	auto pSong = H2Core::CoreActionController::loadSong(
		QString( "%1/GM_kit_demo3.h2song" ).arg( Filesystem::demos_dir() ) );
	H2Core::CoreActionController::setSong( pSong );
	H2Core::CoreActionController::saveSongAs( m_sValidPath );
	
	H2Core::CoreActionController::activateTimeline( true );
	H2Core::CoreActionController::addTempoMarker( 0, 120 );
	H2Core::CoreActionController::addTempoMarker( 3, 100 );
	H2Core::CoreActionController::addTempoMarker( 5, 40 );
	H2Core::CoreActionController::addTempoMarker( 7, 200 );
}

void TimeTest::tearDown(){

	H2Core::CoreActionController::activateTimeline( false );
	
	// Delete all temporary files
	if ( QFile::exists( m_sValidPath ) ) {
		QFile::remove( m_sValidPath );
	}
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

void TimeTest::testHighResolutionSleep(){
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	const float fTolerance = 1;
	std::vector<int> durationsMs{ 2, 10, 65, 234 };

	for ( const auto ffDurationMs : durationsMs ) {
		// Perform one sleep without checking time. This allows for `TimeHelper`
		// to adopt to the new system surplus (which might depend on the sleep
		// duration) and produces results closer to those encountered in real
		// life.
		pHydrogen->getTimeHelper()->highResolutionSleep(
			std::chrono::duration<float, std::milli>( ffDurationMs ) );

		const auto start = Clock::now();
		pHydrogen->getTimeHelper()->highResolutionSleep(
			std::chrono::duration<float, std::milli>( ffDurationMs ) );
		const auto end = Clock::now();

		const auto fPassedMs = std::chrono::duration_cast<
			std::chrono::milliseconds >( end - start ).count();

		___INFOLOG( QString( "Interval: [%1], Milliseconds passed: [%2], tolerance: [%3]" )
					.arg( ffDurationMs ).arg( fPassedMs ).arg( fTolerance ) );
		// We have to wait at least the requested amount. A little bit more is
		// ok. But less is not.
		CPPUNIT_ASSERT( fPassedMs >= ffDurationMs );
		CPPUNIT_ASSERT( std::abs( fPassedMs - ffDurationMs ) <= fTolerance );
	}

	___INFOLOG( "passed" );
}

float TimeTest::locateAndLookupTime( int nPatternPos ){
	H2Core::CoreActionController::locateToColumn( nPatternPos );
	return Hydrogen::get_instance()->getAudioEngine()->getElapsedTime();
}
