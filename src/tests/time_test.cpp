#include <core/config.h>

#include "time_test.h"
#include <core/CoreActionController.h>
#include <core/AudioEngine.h>
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
}

void TimeTest::tearDown(){
	
	// Delete all temporary files
	if ( QFile::exists( m_sValidPath ) ) {
		QFile::remove( m_sValidPath );
	}
}

float TimeTest::locateAndLookupTime( int nPatternPos ){
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioDriver = pHydrogen->getAudioOutput();
	auto pSong = pHydrogen->getSong();
	auto pAudioEngine = AudioEngine::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	pCoreActionController->relocate( nPatternPos );

	pAudioEngine->lock( RIGHT_HERE );
	long nTick = pHydrogen->getTickForPosition( nPatternPos );
	pAudioEngine->unlock();
	
	pAudioEngine->calculateElapsedTime( pAudioDriver->getSampleRate(),
										nTick * pAudioDriver->m_transport.m_fTickSize,
										pSong->__resolution );
	return pAudioEngine->getElapsedTime();
}

void TimeTest::testElapsedTime(){
	
	auto pCoreActionController = Hydrogen::get_instance()->getCoreActionController();
		
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 1, 120 );
	pCoreActionController->addTempoMarker( 3, 100 );
	pCoreActionController->addTempoMarker( 5, 40 );
	pCoreActionController->addTempoMarker( 7, 200 );

	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 0 ) - 0 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 1 ) - 1.98958 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 2 ) - 3.98958 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 3 ) - 5.98958 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 4 ) - 8.3875 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 5 ) - 10.7875 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 6 ) - 16.7687 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 7 ) - 22.7687 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 8 ) - 23.9937 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 1 ) - 1.98958 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 5 ) - 10.7875 ) < 0.0001 );
	CPPUNIT_ASSERT( std::abs( locateAndLookupTime( 2 ) - 3.98958 ) < 0.0001 );
}
