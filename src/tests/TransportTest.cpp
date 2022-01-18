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

#include <core/CoreActionController.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Helpers/Filesystem.h>

#include <iostream>

#include "TransportTest.h"
#include "TestHelper.h"

using namespace H2Core;

void TransportTest::setUp(){

	// We need a song that has at least the maximum pattern group
	// number provided in testElapsedTime(). An empty one won't do it.
	m_pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" ).arg( Filesystem::demos_dir() ) );

	m_pSongSizeChanged = Song::load( QString( H2TEST_FILE( "song/AE_songSizeChanged.h2song" ) ) );

	CPPUNIT_ASSERT( m_pSongDemo != nullptr );
	CPPUNIT_ASSERT( m_pSongSizeChanged != nullptr );
}

void TransportTest::testFrameToTickConversion() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );
	
	bool bNoMismatch = pAudioEngine->testFrameToTickConversion();
	CPPUNIT_ASSERT( bNoMismatch );
}

void TransportTest::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );
	
	bool bNoMismatch = pAudioEngine->testTransportProcessing();
	CPPUNIT_ASSERT( bNoMismatch );
}		
 
void TransportTest::testTransportRelocation() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	pCoreActionController->openSong( m_pSongDemo );
	
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

	pCoreActionController->activateTimeline( false );
}		

void TransportTest::testComputeTickInterval() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );

	bool bNoMismatch = pAudioEngine->testComputeTickInterval();
	CPPUNIT_ASSERT( bNoMismatch );
}		

void TransportTest::testSongSizeChange() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pHydrogen->getCoreActionController()->openSong( m_pSongSizeChanged );

	bool bNoMismatch = pAudioEngine->testSongSizeChange();
	CPPUNIT_ASSERT( bNoMismatch );
}		

void TransportTest::testSongSizeChangeInLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );

	bool bNoMismatch = pAudioEngine->testSongSizeChangeInLoopMode();
	CPPUNIT_ASSERT( bNoMismatch );
}		
