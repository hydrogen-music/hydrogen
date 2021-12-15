/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "TransportTest.h"
#include <core/CoreActionController.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>

using namespace H2Core;

void TransportTest::setUp(){

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine m_randomEngine( randomSeed() );
    std::uniform_int_distribution<int> m_randomDist(1, 4096);
	
	m_sValidPath = QString( "%1/hydrogen_time_test.h2song" )
		.arg( QDir::tempPath() );

	// We need a song that has at least the maximum pattern group
	// number provided in testElapsedTime(). An empty one won't do it.
	auto pCoreActionController = Hydrogen::get_instance()->getCoreActionController();
	pCoreActionController->openSong( QString( "%1/GM_kit_demo3.h2song" ).arg( Filesystem::demos_dir() ) );
	pCoreActionController->saveSongAs( m_sValidPath );
}

void TransportTest::tearDown(){
	
	// Delete all temporary files
	if ( QFile::exists( m_sValidPath ) ) {
		QFile::remove( m_sValidPath );
	}
}

void TransportTest::testFrameToTickConversion() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 3, 100 );
	pCoreActionController->addTempoMarker( 5, 40 );
	pCoreActionController->addTempoMarker( 7, 200 );

	int nFrameOffset1, nFrameOffset2, nFrameOffset3;

	long aux;
	double unused;
	float unused2;
	
	long long nFrame1 = 342732;
	long long nFrame2 = 1037223;
	long long nFrame3 = 453610333722;
	long nTick1 = pAudioEngine->computeTickFromFrame( nFrame1, &nFrameOffset1, &unused2 );
	long nTick2 = pAudioEngine->computeTickFromFrame( nFrame2, &nFrameOffset2, &unused2 );
	long nTick3 = pAudioEngine->computeTickFromFrame( nFrame3, &nFrameOffset3, &unused2 );
	long long nFrame1Computed = pAudioEngine->computeFrameFromTick( nTick1, &unused ) +
			static_cast<long long>(nFrameOffset1);
	long long nFrame2Computed = pAudioEngine->computeFrameFromTick( nTick2, &unused ) +
			static_cast<long long>(nFrameOffset2);
	long long nFrame3Computed = pAudioEngine->computeFrameFromTick( nTick3, &unused ) +
			static_cast<long long>(nFrameOffset3);

	long nTick4 = 552;
	long nTick5 = 1939;
	long nTick6 = 534623409;
	long long nFrame4 = pAudioEngine->computeFrameFromTick( nTick4, &unused );
	long long nFrame5 = pAudioEngine->computeFrameFromTick( nTick5, &unused );
	long long nFrame6 = pAudioEngine->computeFrameFromTick( nTick6, &unused );
	long nTick4Computed = pAudioEngine->computeTickFromFrame( nFrame4, &nFrameOffset1, &unused2 );
	long nTick5Computed = pAudioEngine->computeTickFromFrame( nFrame5, &nFrameOffset2, &unused2 );
	long nTick6Computed = pAudioEngine->computeTickFromFrame( nFrame6, &nFrameOffset3, &unused2 );
	
	// Due to the rounding error in AudioEngine::computeTick() and
	// AudioEngine::computeFrame() a small mismatch is allowed.
	CPPUNIT_ASSERT( abs( nFrame1Computed - nFrame1 ) <= 1 );
	CPPUNIT_ASSERT( abs( nFrame2Computed - nFrame2 ) <= 1 );
	CPPUNIT_ASSERT( abs( nFrame3Computed - nFrame3 ) <= 1 );
	CPPUNIT_ASSERT( abs( nTick4Computed - nTick4 ) <= 1 );
	CPPUNIT_ASSERT( nFrameOffset1 == 0 );
	CPPUNIT_ASSERT( abs( nTick5Computed - nTick5 ) <= 1 );
	CPPUNIT_ASSERT( nFrameOffset2 == 0 );
	CPPUNIT_ASSERT( abs( nTick6Computed - nTick6 ) <= 1 );
	CPPUNIT_ASSERT( nFrameOffset3 == 0 );
}

void TransportTest::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pCoreActionController->activateTimeline( false );
	
	bool bNoMismatch = pAudioEngine->testTransportProcessing();
	CPPUNIT_ASSERT( bNoMismatch );
}		
 
void TransportTest::testTransportRelocation() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 1, 100 );
	pCoreActionController->addTempoMarker( 2, 20 );
	pCoreActionController->addTempoMarker( 3, 13.4 );
	pCoreActionController->addTempoMarker( 4, 383.2 );
	pCoreActionController->addTempoMarker( 5, 64.38372 );
	pCoreActionController->addTempoMarker( 6, 96.3 );
	pCoreActionController->addTempoMarker( 7, 240.46 );
	pCoreActionController->addTempoMarker( 8, 200.1 );
	
	bool bNoMismatch = pAudioEngine->testTransportRelocation();
	CPPUNIT_ASSERT( bNoMismatch );
}		

void TransportTest::testComputeTickInterval() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	pCoreActionController->activateTimeline( false );

	bool bNoMismatch = pAudioEngine->testComputeTickInterval();
	CPPUNIT_ASSERT( bNoMismatch );
}		

void TransportTest::testUpdateNoteQueue() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	pCoreActionController->activateTimeline( false );

	bool bNoMismatch = pAudioEngine->testUpdateNoteQueue();
	CPPUNIT_ASSERT( bNoMismatch );
}		
