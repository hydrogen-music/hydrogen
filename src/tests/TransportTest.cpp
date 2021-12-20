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
	
	bool bNoMismatch = pAudioEngine->testFrameToTickConversion();
	CPPUNIT_ASSERT( bNoMismatch );
}

void TransportTest::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
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
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	bool bNoMismatch = pAudioEngine->testComputeTickInterval();
	CPPUNIT_ASSERT( bNoMismatch );
}		
