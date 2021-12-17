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

#include <iostream>

#include "TransportTest.h"
#include <core/CoreActionController.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>

using namespace H2Core;

void TransportTest::setUp(){
	
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

	double fFrameOffset1, fFrameOffset2, fFrameOffset3,
		fFrameOffset4, fFrameOffset5, fFrameOffset6;
	
	long long nFrame1 = 342732;
	long long nFrame2 = 1037223;
	long long nFrame3 = 453610333722;
	double fTick1 = pAudioEngine->computeTickFromFrame( nFrame1 );
	double fTick2 = pAudioEngine->computeTickFromFrame( nFrame2 );
	double fTick3 = pAudioEngine->computeTickFromFrame( nFrame3 );
	long long nFrame1Computed = pAudioEngine->computeFrameFromTick( fTick1, &fFrameOffset1 );
	long long nFrame2Computed = pAudioEngine->computeFrameFromTick( fTick2, &fFrameOffset2 );
	long long nFrame3Computed = pAudioEngine->computeFrameFromTick( fTick3, &fFrameOffset3 );
	
	if ( nFrame1Computed != nFrame1 ) {
		std::cout << QString( "[1] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4" )
			.arg( nFrame1 ).arg( fTick1, 0, 'f' ).arg( nFrame1Computed )
			.arg( fFrameOffset1, 0, 'f' ).toLocal8Bit().data() << std::endl;
	}
	CPPUNIT_ASSERT( nFrame1Computed == nFrame1 );
	CPPUNIT_ASSERT( fFrameOffset1 == 0 );

	if ( nFrame2Computed != nFrame2 ) {
		std::cout << QString( "[2] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4" )
			.arg( nFrame2 ).arg( fTick2, 0, 'f' ).arg( nFrame2Computed )
			.arg( fFrameOffset2, 0, 'f' ).toLocal8Bit().data() << std::endl;
	}
	CPPUNIT_ASSERT( nFrame2Computed == nFrame2 );
	CPPUNIT_ASSERT( fFrameOffset2 == 0 );
	
	if ( nFrame3Computed != nFrame3 ) {
		std::cout << QString( "[3] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4" )
			.arg( nFrame3 ).arg( fTick3, 0, 'f' ).arg( nFrame3Computed )
			.arg( fFrameOffset3, 0, 'f' ).toLocal8Bit().data() << std::endl;
	}
	CPPUNIT_ASSERT( nFrame3Computed == nFrame3 );
	CPPUNIT_ASSERT( fFrameOffset3 == 0 );

	long aux;
	double fTick4 = 552;
	double fTick5 = 1939;
	double fTick6 = 534623409;
	long long nFrame4 = pAudioEngine->computeFrameFromTick( fTick4, &fFrameOffset4 );
	long long nFrame5 = pAudioEngine->computeFrameFromTick( fTick5, &fFrameOffset5 );
	long long nFrame6 = pAudioEngine->computeFrameFromTick( fTick6, &fFrameOffset6 );
	double fTick4Computed = pAudioEngine->computeTickFromFrame( nFrame4 ) +
		fFrameOffset4;
	double fTick5Computed = pAudioEngine->computeTickFromFrame( nFrame5 ) +
		fFrameOffset5;
	double fTick6Computed = pAudioEngine->computeTickFromFrame( nFrame6 ) +
		fFrameOffset6;
	
	
	if ( abs( fTick4Computed - fTick4 ) > 1e-9 ) {
		std::cout << QString( "[4] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4" )
			.arg( nFrame4 ).arg( fTick4, 0, 'f' ).arg( fTick4Computed, 0, 'f' )
			.arg( fFrameOffset4, 0, 'f' ).toLocal8Bit().data() << std::endl;
	}
	CPPUNIT_ASSERT( abs( fTick4Computed - fTick4 ) < 1e-9 );

	if ( abs( fTick5Computed - fTick5 ) > 1e-9 ) {
		std::cout << QString( "[5] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4" )
			.arg( nFrame5 ).arg( fTick5, 0, 'f' ).arg( fTick5Computed, 0, 'f' )
			.arg( fFrameOffset5, 0, 'f' ).toLocal8Bit().data() << std::endl;
	}
	CPPUNIT_ASSERT( abs( fTick5Computed - fTick5 ) < 1e-9 );

	if ( abs( fTick6Computed - fTick6 ) > 1e-9 ) {
		std::cout << QString( "[6] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4" )
			.arg( nFrame6 ).arg( fTick6, 0, 'f' ).arg( fTick6Computed, 0, 'f' )
			.arg( fFrameOffset6, 0, 'f' ).toLocal8Bit().data() << std::endl;
	}
	CPPUNIT_ASSERT( abs( fTick6Computed - fTick6 ) < 1e-9 );
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
