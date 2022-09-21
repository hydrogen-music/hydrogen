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
#include <core/AudioEngine/AudioEngineTests.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
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
	m_pSongNoteEnqueuing = Song::load( QString( H2TEST_FILE( "song/AE_noteEnqueuing.h2song" ) ) );

	CPPUNIT_ASSERT( m_pSongDemo != nullptr );
	CPPUNIT_ASSERT( m_pSongSizeChanged != nullptr );
	CPPUNIT_ASSERT( m_pSongNoteEnqueuing != nullptr );

	Preferences::get_instance()->m_bUseMetronome = false;
}

void TransportTest::tearDown() {
	// The tests in here tend to produce a very large number of log
	// messages and a couple of them may tend to be printed _after_
	// the results of the overall test runnner. This is quite
	// unpleasant as the overall result is only shown after
	// scrolling. As the TestRunner itself does not seem to support
	// fixtures, we flush the logger in here.
	H2Core::Logger::get_instance()->flush();
}

void TransportTest::testFrameToTickConversion() {
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );
	
	for ( int ii = 0; ii < 15; ++ii ) {
		TestHelper::varyAudioDriverConfig( ii );
		bool bNoMismatch = AudioEngineTests::testFrameToTickConversion();
		CPPUNIT_ASSERT( bNoMismatch );
	}
}

void TransportTest::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );

	for ( int ii = 0; ii < 15; ++ii ) {
		TestHelper::varyAudioDriverConfig( ii );
		bool bNoMismatch = AudioEngineTests::testTransportProcessing();
		CPPUNIT_ASSERT( bNoMismatch );
	}
}		
 
void TransportTest::testTransportRelocation() {
	auto pHydrogen = Hydrogen::get_instance();
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
	
	for ( int ii = 0; ii < 15; ++ii ) {
		TestHelper::varyAudioDriverConfig( ii );
		bool bNoMismatch = AudioEngineTests::testTransportRelocation();
		CPPUNIT_ASSERT( bNoMismatch );
	}

	pCoreActionController->activateTimeline( false );
}		

void TransportTest::testComputeTickInterval() {
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );

	for ( int ii = 0; ii < 15; ++ii ) {
		TestHelper::varyAudioDriverConfig( ii );
		bool bNoMismatch = AudioEngineTests::testComputeTickInterval();
		CPPUNIT_ASSERT( bNoMismatch );
	}
}		

void TransportTest::testSongSizeChange() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	pCoreActionController->openSong( m_pSongSizeChanged );

	// Depending on buffer size and sample rate transport might be
	// loop when toggling a pattern at the end of the song. If there
	// were tempo markers present, the chunk of the interval covered
	// by AudioEngine::computeTickInterval being looped would have a
	// different tickSize than its first part. This is itself no
	// problem but it would make the test much more complex as we test
	// against those calculated intervals to remain constant.
	pCoreActionController->activateTimeline( false );

	for ( int ii = 0; ii < 15; ++ii ) {
		TestHelper::varyAudioDriverConfig( ii );
		
		// For larger sample rates no notes will remain in the
		// AudioEngine::m_songNoteQueue after one process step.
		if ( H2Core::Preferences::get_instance()->m_nSampleRate <= 48000 ) {
			bool bNoMismatch = AudioEngineTests::testSongSizeChange();
			CPPUNIT_ASSERT( bNoMismatch );
		}
	}
	
	pCoreActionController->activateLoopMode( false );
}		

void TransportTest::testSongSizeChangeInLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->getCoreActionController()->openSong( m_pSongDemo );

	for ( int ii = 0; ii < 15; ++ii ) {
		TestHelper::varyAudioDriverConfig( ii );
		bool bNoMismatch = AudioEngineTests::testSongSizeChangeInLoopMode();
		CPPUNIT_ASSERT( bNoMismatch );
	}
}		

void TransportTest::testNoteEnqueuing() {
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->getCoreActionController()->openSong( m_pSongNoteEnqueuing );

	// This test is quite time consuming.
	std::vector<int> indices{ 0, 1, 2, 5, 7, 9, 12, 15 };

	for ( auto ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		bool bNoMismatch = AudioEngineTests::testNoteEnqueuing();
		CPPUNIT_ASSERT( bNoMismatch );
	}
}		
