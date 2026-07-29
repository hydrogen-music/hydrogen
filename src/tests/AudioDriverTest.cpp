/*
 * Hydrogen
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

#include "AudioDriverTest.h"
#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Event.h>
#include <core/Hydrogen.h>
#include <core/IO/SoftwareDriver.h>

#include <chrono>
#include <thread>

void AudioDriverTest::setUp() {
	auto pPref = pTestPreferences();
	m_nPrevBufferSize = pPref->m_nBufferSize;
	m_prevAudioDriver = pPref->m_audioDriver;
}

void AudioDriverTest::testDriverSwitching() {
	___INFOLOG("");

	auto pHydrogen = pTestHydrogen();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	// Attempting to start up our JACK driver takes some time. In case it fails,
	// it takes much longer. But this only indicates that the system does not
	// offer JACK support. There is no need to test it again (which saves a
	// significant amount of time for the unit tests/pipelines).
	bool bCheckJack = H2Core::Preferences::checkJackSupport();
	std::shared_ptr<H2Core::AudioDriver> pDriver;

	for ( int ii = 0; ii < 10; ++ii ) {
		if ( bCheckJack ) {
			// Apart from the JACK none of our drivers produces stdout/stderr
			// output. No need for a visual separation.
			std::cout << ii << std::endl;
		}
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Alsa,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Oss,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
#ifndef Q_OS_MACX
		// JACK support in our macOS pipeline is present but severly flawed. As
		// of 2025-09-23 the JACK server routinely dies during switching causing
		// the test to freeze.
		if ( bCheckJack ) {
			pDriver = pAudioEngine->createAudioDriver(
				H2Core::Preferences::AudioDriver::Jack,
				H2Core::Event::Trigger::Default );
			if ( pDriver == nullptr ) {
				bCheckJack = false;
			}
		}
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
#endif
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::PortAudio,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::CoreAudio,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::PulseAudio,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Disk,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Null,
			H2Core::Event::Trigger::Default );
		pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Fake,
			H2Core::Event::Trigger::Default );
	}

	___INFOLOG("done");
}

void AudioDriverTest::testMidiOnlyMode() {
	___INFOLOG("");

	// A standalone MIDI-only configuration (ADR 0031): no audio output device
	// (the headless software driver, selected as `Null`) paired with a real MIDI
	// driver (LoopBack — software, needs no hardware). This is its own isolated
	// instance so it does not disturb the shared test engine.
	auto pPref = H2Core::Preferences::create_instance();
	pPref->m_audioDriver = H2Core::Preferences::AudioDriver::Null;
	pPref->m_midiDriver = H2Core::Preferences::MidiDriver::LoopBack;
	pPref->setOscServerEnabled( false );

	auto* pHydrogen = new H2Core::Hydrogen( pPref, -1 );
	pHydrogen->setProcessMode( H2Core::Hydrogen::ProcessMode::Headless );
	auto pAudioEngine = pHydrogen->getAudioEngine();

	// Audio side: a headless software driver — it clocks the engine but feeds no
	// real audio sink.
	auto pDriver = std::dynamic_pointer_cast<H2Core::SoftwareDriver>(
		pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );
	CPPUNIT_ASSERT( ! pDriver->getProducesAudio() );
	CPPUNIT_ASSERT( pDriver->getSampleRate() > 0 );
	CPPUNIT_ASSERT( pDriver->getBufferSize() > 0 );

	// MIDI side: a real driver is up alongside the silent audio driver — the whole
	// point of MIDI-only mode (previously impossible: the inert Null driver left
	// the engine frozen, so a MIDI driver had no clock to run against).
	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	// The headless clock actually advances the transport: start playback and watch
	// the playhead move. With the old inert Null driver this stayed at 0 forever.
	const long long nStartFrame = pAudioEngine->getPlayhead()->getFrame();
	pHydrogen->sequencerPlay();
	bool bAdvanced = false;
	for ( int ii = 0; ii < 200 && ! bAdvanced; ++ii ) { // poll up to ~2 s
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		if ( pAudioEngine->getPlayhead()->getFrame() > nStartFrame ) {
			bAdvanced = true;
		}
	}
	pHydrogen->sequencerStop();
	CPPUNIT_ASSERT( bAdvanced );

	delete pHydrogen;

	___INFOLOG("passed");
}

void AudioDriverTest::tearDown() {
	auto pHydrogen = pTestHydrogen();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	auto pPref = pTestPreferences();
	pPref->m_nBufferSize = m_nPrevBufferSize;
	pPref->m_audioDriver = m_prevAudioDriver;

	pAudioEngine->stopAudioDriver( H2Core::Event::Trigger::Default );
	pAudioEngine->createAudioDriver(
		H2Core::Preferences::AudioDriver::Fake,
		H2Core::Event::Trigger::Default );
}
