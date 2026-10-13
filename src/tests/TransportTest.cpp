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

#include "TransportTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/AudioEngineTests.h>
#include <core/Basics/Drumkit.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/AudioEngine/Transport.h>
#include <core/Hydrogen.h>
#include <core/IO/SoftwareDriver.h>
#include <core/Preferences/Preferences.h>

#include <cstdlib>
#include <iostream>

#include "TestHelper.h"
#include "utils/FakePluginHost.h"

#include "assertions/AudioFile.h"

using namespace H2Core;

void TransportTest::setUp(){
	pTestPreferences()->m_bUseMetronome = false;
	// AudioEngineTests lives in libcore and cannot reach pTestHydrogen(); inject
	// the instance under test here (ADR 0015).
	AudioEngineTests::setHydrogen( pTestHydrogen() );
}

void TransportTest::tearDown() {
	// The tests in here tend to produce a very large number of log
	// messages and a couple of them may tend to be printed _after_
	// the results of the overall test runnner. This is quite
	// unpleasant as the overall result is only shown after
	// scrolling. As the TestRunner itself does not seem to support
	// fixtures, we flush the logger in here.
	H2Core::Logger::get_instance()->flush();

	// Reset to default audio driver config
	auto pPref = pTestPreferences();
	pPref->m_nBufferSize = 1024;
	pPref->m_nSampleRate = 44100;
	pTestHydrogen()->restartAudioDriver();
}

void TransportTest::testFrameToTickConversion() {
	___INFOLOG( "" );
	auto pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" )
								   .arg( Filesystem::demosDir() ), false, pTestHydrogen() );
	ASSERT_SONG( pSongDemo );
	pTestHydrogen()->getCoreActionController()->setSong( pSongDemo );

	const std::vector<int> indices{ 0, 5, 7, 12 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testFrameToTickConversion );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testTransportProcessing() {
	___INFOLOG( "" );
	auto pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" )
								   .arg( Filesystem::demosDir() ), false, pTestHydrogen() );
	ASSERT_SONG( pSongDemo );
	pTestHydrogen()->getCoreActionController()->setSong( pSongDemo );

	const std::vector<int> indices{ 1, 9, 14 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testTransportProcessing );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testTransportProcessingTimeline() {
	___INFOLOG( "" );
	auto pSongTransportProcessingTimeline =
		Song::load( QString( H2TEST_FILE( "song/AE_transportProcessingTimeline.h2song" ) ), false, pTestHydrogen() );
	ASSERT_SONG( pSongTransportProcessingTimeline );
	pTestHydrogen()->getCoreActionController()->
		setSong( pSongTransportProcessingTimeline );

	const std::vector<int> indices{ 2, 9, 10 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testTransportProcessingTimeline );
	}
	___INFOLOG( "passed" );
}		
 
void TransportTest::testTransportRelocation() {
	___INFOLOG( "" );
	auto pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" )
								   .arg( Filesystem::demosDir() ), false, pTestHydrogen() );
	ASSERT_SONG( pSongDemo );
	pTestHydrogen()->getCoreActionController()->setSong( pSongDemo );
	
	pTestHydrogen()->getCoreActionController()->activateTimeline( true );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 0, 120 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 1, 100 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 2, 20 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 3, 13.4 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 4, 383.2 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 5, 64.38372 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 6, 96.3 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 7, 240.46 );
	pTestHydrogen()->getCoreActionController()->addTempoMarker( 8, 200.1 );
	
	const std::vector<int> indices{ 0, 5, 6 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testTransportRelocation );
	}

	pTestHydrogen()->getCoreActionController()->activateTimeline( false );
	___INFOLOG( "passed" );
}

void TransportTest::testLoopMode() {
	___INFOLOG( "" );

	const QString sSongFile = H2TEST_FILE( "song/AE_loopMode.h2song" );

	auto pSong = H2Core::Song::load( sSongFile, false, pTestHydrogen() );
	ASSERT_SONG( pSong );

	pTestHydrogen()->getCoreActionController()->setSong( pSong );
	
	const std::vector<int> indices{ 0, 1, 12 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testLoopMode );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testSongSizeChange() {
	___INFOLOG( "" );
	auto pSongSizeChanged =
		Song::load( QString( H2TEST_FILE( "song/AE_songSizeChanged.h2song" ) ), false, pTestHydrogen() );
	ASSERT_SONG( pSongSizeChanged );
	pTestHydrogen()->getCoreActionController()->setSong( pSongSizeChanged );

	// Depending on buffer size and sample rate transport might be
	// loop when toggling a pattern at the end of the song. If there
	// were tempo markers present, the chunk of the interval covered
	// by AudioEngine::computeTickInterval being looped would have a
	// different tickSize than its first part. This is itself no
	// problem but it would make the test much more complex as we test
	// against those calculated intervals to remain constant.
	pTestHydrogen()->getCoreActionController()->activateTimeline( false );

	const std::vector<int> indices{ 0, 1, 2, 3 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		
		// For larger sample rates no notes will remain in the
		// AudioEngine::m_songNoteQueue after one process step.
		if ( pTestPreferences()->m_nSampleRate <= 48000 ) {
			perform( &AudioEngineTests::testSongSizeChange );
		}
	}
	
	pTestHydrogen()->getCoreActionController()->activateLoopMode( false );
	___INFOLOG( "passed" );
}		

void TransportTest::testSongSizeChangeInLoopMode() {
	___INFOLOG( "" );
	auto pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" )
								   .arg( Filesystem::demosDir() ), false, pTestHydrogen() );
	ASSERT_SONG( pSongDemo );
	pTestHydrogen()->getCoreActionController()->setSong( pSongDemo );

	const std::vector<int> indices{ 0, 5, 7, 13 };
	for ( const int ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testSongSizeChangeInLoopMode );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testPlaybackTrack() {
	___INFOLOG( "" );

	QString sSongFile = H2TEST_FILE( "song/AE_playbackTrack.h2song" );
	QString sOutFile = Filesystem::tmpFilePath("testPlaybackTrack.wav");
	QString sRefFile = H2TEST_FILE("song/res/playbackTrack.flac");

	TestHelper::exportSong( sSongFile, sOutFile );
	H2TEST_ASSERT_AUDIO_FILES_EQUAL( sRefFile, sOutFile );
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}

void TransportTest::testSampleConsistency() {
	___INFOLOG( "" );

	const QString sSongFile = H2TEST_FILE( "song/AE_sampleConsistency.h2song" );
	const QString sDrumkitPath = H2TEST_FILE( "drumkits/sampleKit/drumkit.xml" );
	const QString sOutFile = Filesystem::tmpFilePath("testsampleConsistency.wav");
	const QString sRefFile = H2TEST_FILE("drumkits/sampleKit/longSample.flac");

	auto pHydrogen = pTestHydrogen();

	auto pSong = H2Core::Song::load( sSongFile, false, pTestHydrogen() );
	ASSERT_SONG( pSong );
		
	pHydrogen->setSong( pSong );

	// Apply drumkit containing the long sample to be tested.
	const auto pDrumkit = H2Core::Drumkit::load(
		sDrumkitPath, false, nullptr, true , pTestHydrogen() );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	pTestHydrogen()->getCoreActionController()->setDrumkit( pDrumkit );

	TestHelper::exportSong( sOutFile );
	
	H2TEST_ASSERT_AUDIO_FILES_DATA_EQUAL( sRefFile, sOutFile );
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}

void TransportTest::testNoteEnqueuing() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();

	auto pSongNoteEnqueuing =
		Song::load( QString( H2TEST_FILE( "song/AE_noteEnqueuing.h2song" ) ), false, pTestHydrogen() );
	ASSERT_SONG( pSongNoteEnqueuing );

	pTestHydrogen()->getCoreActionController()->setSong( pSongNoteEnqueuing );

	// This test is quite time consuming.
	std::vector<int> indices{ 1, 9, 12 };
	for ( auto ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testNoteEnqueuing );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testNoteEnqueuingTimeline() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();
	auto pSong = Song::load( QString( H2TEST_FILE( "song/AE_noteEnqueuingTimeline.h2song" ) ), false, pTestHydrogen() );
	ASSERT_SONG( pSong );

	pTestHydrogen()->getCoreActionController()->setSong( pSong );

	// This test is quite time consuming.
	std::vector<int> indices{ 0, 5, 7 };

	for ( auto ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testNoteEnqueuingTimeline );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testHumanization() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();

	auto pSongHumanization =
		Song::load( QString( H2TEST_FILE( "song/AE_humanization.h2song" ) ), false, pTestHydrogen() );
	ASSERT_SONG( pSongHumanization );
	pTestHydrogen()->getCoreActionController()->setSong( pSongHumanization );

	// This test is quite time consuming.
	std::vector<int> indices{ 1, 10 };
	for ( auto ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testHumanization );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testMuteGroups() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();

	auto pSongMuteGroups =
		Song::load( QString( H2TEST_FILE( "song/AE_muteGroups.h2song" ) ), false, pTestHydrogen() );
	CPPUNIT_ASSERT( pSongMuteGroups != nullptr );
	pTestHydrogen()->getCoreActionController()->setSong( pSongMuteGroups );

	std::vector<int> indices{ 1, 3, 10 };
	for ( auto ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testMuteGroups );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testNoteOff() {
	___INFOLOG( "" );
	auto pHydrogen = pTestHydrogen();

	auto pSongNoteOff =
		Song::load( QString( H2TEST_FILE( "song/AE_noteOff.h2song" ) ), false, pTestHydrogen() );
	CPPUNIT_ASSERT( pSongNoteOff != nullptr );
	pTestHydrogen()->getCoreActionController()->setSong( pSongNoteOff );

	std::vector<int> indices{ 1, 3, 10 };
	for ( auto ii : indices ) {
		TestHelper::varyAudioDriverConfig( ii );
		perform( &AudioEngineTests::testNoteOff );
	}
	___INFOLOG( "passed" );
}

void TransportTest::testUpdateTransport() {
	___INFOLOG( "" );

	perform( &AudioEngineTests::testUpdateTransport );

	___INFOLOG( "passed" );
}

void TransportTest::perform( std::function<void()> func ) {
	try {
		// Stop the processing of the callback of the AudioEngine. TransportTest
		// was written before `FakeAudioDriver` (now `SoftwareDriver`) was a
		// proper driver.
		auto pDriver = std::dynamic_pointer_cast<SoftwareDriver>(
			pTestHydrogen()->getAudioEngine()->getAudioDriver()
		);
		if ( pDriver != nullptr ) {
			pDriver->deactivate();
		}

		func();
	} catch ( std::exception& err ) {
		CppUnit::Message msg( err.what() );
		throw CppUnit::Exception( msg );
	}
}

// ── T3.3: host-transport follower (ADR 0013) ──────────────────────────────
// These run against a FakePluginHost, which owns its own headless engine driven
// by the host-driven Plugin audio/MIDI drivers, independent of pTestHydrogen().

void TransportTest::testPluginHostTempo() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto pPlayhead = host.getHydrogen()->getAudioEngine()->getPlayhead();

	host.setBpm( 156.0f );
	host.setPlaying( true );
	host.process( 256 );
	host.process( 256 );

	// The playhead adopted the host tempo.
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 156.0, pPlayhead->getBpm(), 0.001 );

	// A subsequent host tempo change is followed too.
	host.setBpm( 92.5f );
	host.process( 256 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 92.5, pPlayhead->getBpm(), 0.001 );

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

void TransportTest::testPluginHostTransportState() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto pPlayhead = host.getHydrogen()->getAudioEngine()->getPlayhead();
	host.setBpm( 120.0f );

	// Stopped: processing must not advance the playhead.
	host.process( 256 );
	CPPUNIT_ASSERT_EQUAL( (long long)0, pPlayhead->getFrame() );

	// Rolling: the playhead advances by exactly one block per cycle.
	host.setPlaying( true );
	host.process( 256 );
	host.process( 256 );
	CPPUNIT_ASSERT_EQUAL( (long long)512, pPlayhead->getFrame() );

	// Stopped again: the playhead freezes at its last position.
	host.setPlaying( false );
	const long long nFrozen = pPlayhead->getFrame();
	host.process( 256 );
	host.process( 256 );
	CPPUNIT_ASSERT_EQUAL( nFrozen, pPlayhead->getFrame() );

	___INFOLOG( "passed" );
}

void TransportTest::testPluginHostPositionTracking() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto pPlayhead = host.getHydrogen()->getAudioEngine()->getPlayhead();
	host.setBpm( 120.0f );
	host.setPlaying( true );

	const int nBlocks = 16;
	for ( int ii = 0; ii < nBlocks; ++ii ) {
		host.process( 256 );
	}

	// The engine advanced by exactly nframes per block, matching the host clock.
	CPPUNIT_ASSERT_EQUAL( (long long)( nBlocks * 256 ), pPlayhead->getFrame() );
	CPPUNIT_ASSERT_EQUAL( (long long)( nBlocks * 256 ), host.getFramePosition() );

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

void TransportTest::testPluginHostRelocate() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto pPlayhead = host.getHydrogen()->getAudioEngine()->getPlayhead();
	host.setBpm( 120.0f );

	// A relocate while stopped jumps the playhead to the host frame.
	host.setFramePosition( 88200 ); // 2 s at 44.1 kHz
	host.process( 256 );
	CPPUNIT_ASSERT( std::llabs( pPlayhead->getFrame() - 88200 ) <= 1 );

	// And while rolling: jump, then keep rolling contiguously from there.
	host.setPlaying( true );
	host.setFramePosition( 44100 );
	host.process( 256 );
	CPPUNIT_ASSERT( std::llabs( pPlayhead->getFrame() - ( 44100 + 256 ) ) <= 1 );
	host.process( 256 );
	CPPUNIT_ASSERT( std::llabs( pPlayhead->getFrame() - ( 44100 + 512 ) ) <= 1 );

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

void TransportTest::testPluginHostLoop() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto pPlayhead = host.getHydrogen()->getAudioEngine()->getPlayhead();
	host.setBpm( 120.0f );
	host.setPlaying( true );

	for ( int ii = 0; ii < 4; ++ii ) {
		host.process( 256 );
	}
	CPPUNIT_ASSERT_EQUAL( (long long)1024, pPlayhead->getFrame() );

	// The host loops back to the start of its region (frame jumps backwards).
	host.setFramePosition( 0 );
	host.process( 256 );
	// Followed back and rolling again, no hang, no stop.
	CPPUNIT_ASSERT_EQUAL( (long long)256, pPlayhead->getFrame() );
	host.process( 256 );
	CPPUNIT_ASSERT_EQUAL( (long long)512, pPlayhead->getFrame() );

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

void TransportTest::testPluginHostTempoWinsOverTimeline() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto* pHydrogen = host.getHydrogen();
	auto pCAC = pHydrogen->getCoreActionController();
	auto pPlayhead = pHydrogen->getAudioEngine()->getPlayhead();

	// Song mode + an active Timeline with a tempo marker that disagrees with the
	// host. Under a plugin host the host tempo must still win (ADR 0013).
	pCAC->activateSongMode( true );
	pCAC->activateTimeline( true );
	pCAC->addTempoMarker( 0, 90.0f );

	host.setBpm( 150.0f );
	host.setPlaying( true );
	host.process( 256 );
	host.process( 256 );

	CPPUNIT_ASSERT_DOUBLES_EQUAL( 150.0, pPlayhead->getBpm(), 0.001 );

	host.setPlaying( false );
	pCAC->activateTimeline( false );

	___INFOLOG( "passed" );
}
