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


#include "TimeTest.h"
#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Song.h>
#include <core/config.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Time.h>
#include <core/Helpers/TimeHelper.h>
#include <core/Hydrogen.h>

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>
#include <QTest>

using namespace H2Core;

void TimeTest::setUp(){
	
	m_sValidPath = QString( "%1/hydrogen_time_test.h2song" )
		.arg( QDir::tempPath() );

	// We need a song that has at least the maximum pattern group
	// number provided in testElapsedTime(). An empty one won't do it.
	auto pSong = pTestHydrogen()->getCoreActionController()->loadSong(
		QString( "%1/GM_kit_demo3.h2song" ).arg( Filesystem::demosDir() ) );
	pTestHydrogen()->getCoreActionController()->setSong( pSong );
	pTestHydrogen()->getCoreActionController()->saveSongAs( m_sValidPath, true );
	
	pTestHydrogen()->getCoreActionController()->activateTimeline( true );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 0, 120 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 3, 100 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 5, 40 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 7, 200 );
}

void TimeTest::tearDown(){

	pTestHydrogen()->getCoreActionController()->activateTimeline( false );
	
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

namespace {
/** Maximum time a sleep may last longer than requested. */
constexpr double fSleepToleranceMs = 0.5;

std::shared_ptr<TimeHelper> burnedInTimeHelper()
{
	auto pTimeHelper = std::make_shared<TimeHelper>();
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( ! pTimeHelper->isBurnedIn() && nnTry < nMaxTries ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		++nnTry;
	}
	CPPUNIT_ASSERT( pTimeHelper->isBurnedIn() );
	return pTimeHelper;
}

double toMs( const std::chrono::duration<double> duration )
{
	return std::chrono::duration<double, std::milli>( duration ).count();
}
}  // namespace

void TimeTest::testHighResolutionSleep(){
	___INFOLOG( "" );

	auto pTestHelper = TestHelper::get_instance();

	// A fresh helper: no sleep may serve as warm-up for the next one.
	auto pTimeHelper = burnedInTimeHelper();

	const std::vector<double> durationsMs{ 2, 10, 65, 158.5, 234, 500, 21.3 };
	for ( const auto ffDurationMs : durationsMs ) {
		const auto start = Clock::now();
		pTimeHelper->highResolutionSleep(
			std::chrono::duration<double, std::milli>( ffDurationMs ) );
		const double fPassedMs = toMs( Clock::now() - start );

		___INFOLOG( QString( "Interval: [%1], Milliseconds passed: [%2], tolerance: [%3]" )
					.arg( ffDurationMs ).arg( fPassedMs ).arg( fSleepToleranceMs ) );
		// We have to wait at least the requested amount. A little bit more is
		// ok. But less is not.
		CPPUNIT_ASSERT( fPassedMs >= ffDurationMs );

		if ( ! pTestHelper->isAppveyor() ) {
			CPPUNIT_ASSERT( fPassedMs - ffDurationMs <= fSleepToleranceMs );
		}
	}

	___INFOLOG( "passed" );
}

void TimeTest::testSleepUntil(){
	___INFOLOG( "" );

	auto pTestHelper = TestHelper::get_instance();
	auto pTimeHelper = burnedInTimeHelper();

	// Tap tempo at 378.4 bpm.
	const auto interval = std::chrono::duration<double, std::milli>( 158.5624 );
	const auto start = Clock::now();
	for ( int nnTick = 1; nnTick <= 10; ++nnTick ) {
		const auto deadline = start +
			std::chrono::duration_cast<Clock::duration>( nnTick * interval );
		pTimeHelper->sleepUntil( deadline );
		const double fLateMs = toMs( Clock::now() - deadline );

		___INFOLOG( QString( "Tick: [%1], late by [%2]ms, tolerance: [%3]" )
					.arg( nnTick ).arg( fLateMs ).arg( fSleepToleranceMs ) );
		CPPUNIT_ASSERT( fLateMs >= 0 );
		if ( ! pTestHelper->isAppveyor() ) {
			CPPUNIT_ASSERT( fLateMs <= fSleepToleranceMs );
		}
	}

	// A deadline in the past returns right away.
	const auto pastStart = Clock::now();
	pTimeHelper->sleepUntil( pastStart - std::chrono::milliseconds( 5 ) );
	CPPUNIT_ASSERT( toMs( Clock::now() - pastStart ) <= fSleepToleranceMs );

	___INFOLOG( "passed" );
}

void TimeTest::testClockIsMonotonic(){
	___INFOLOG( "" );

	CPPUNIT_ASSERT( Clock::is_steady );

	___INFOLOG( "passed" );
}

void TimeTest::testTimePointToQString(){
	___INFOLOG( "" );

	const auto wallBefore = std::chrono::system_clock::now();
	const QString sNow = H2Core::timePointToQString( Clock::now() );
	const auto wallAfter = std::chrono::system_clock::now();

	const QRegularExpression format( "^(\\d\\d):(\\d\\d):(\\d\\d)\\.(\\d\\d\\d)$" );
	const auto match = format.match( sNow );
	___INFOLOG( QString( "now: [%1]" ).arg( sNow ) );
	CPPUNIT_ASSERT( match.hasMatch() );

	// Milliseconds since midnight UTC, compared modulo a day.
	const auto msOfDay = []( std::chrono::system_clock::time_point timePoint ) {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
				   timePoint.time_since_epoch() ).count() % 86400000;
	};
	const long long nShownMs = ( match.captured( 1 ).toLongLong() * 3600 +
								 match.captured( 2 ).toLongLong() * 60 +
								 match.captured( 3 ).toLongLong() ) * 1000 +
		match.captured( 4 ).toLongLong();
	const long long nBeforeMs = msOfDay( wallBefore );
	const long long nAfterMs = msOfDay( wallAfter );
	// Converting between clocks costs a few microseconds of accuracy.
	const long long nSlackMs = 2;
	if ( nBeforeMs <= nAfterMs ) {
		CPPUNIT_ASSERT( nShownMs >= nBeforeMs - nSlackMs );
		CPPUNIT_ASSERT( nShownMs <= nAfterMs + nSlackMs );
	}

	___INFOLOG( "passed" );
}
float TimeTest::locateAndLookupTime( int nPatternPos ){
	pTestHydrogen()->getCoreActionController()->locateToColumn( nPatternPos );
	return pTestHydrogen()->getAudioEngine()->getElapsedTime();
}
