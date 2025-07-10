/*
 * Hydrogen
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

#include "AudioDriverTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>

void AudioDriverTest::setUp() {
	auto pPref = H2Core::Preferences::get_instance();
	m_nPrevBufferSize = pPref->m_nBufferSize;
	m_prevAudioDriver = pPref->m_audioDriver;
}

void AudioDriverTest::testDriverSwitching() {
	___INFOLOG("");

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	// Attempting to start up our JACK driver takes some time. In case it fails,
	// it takes much longer. But this only indicates that the system does not
	// offer JACK support. There is no need to test it again (which saves a
	// significant amount of time for the unit tests/pipelines).
	bool bCheckJack = H2Core::Preferences::checkJackSupport();
	H2Core::AudioOutput* pDriver;

	for ( int ii = 0; ii < 10; ++ii ) {
		if ( bCheckJack ) {
			// Apart from the JACK none of our drivers produces stdout/stderr
			// output. No need for a visual separation.
			std::cout << ii << std::endl;
		}
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Alsa );
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Oss );
		pAudioEngine->stopAudioDriver();
		if ( bCheckJack ) {
			pDriver = pAudioEngine->createAudioDriver(
				H2Core::Preferences::AudioDriver::Jack );
			if ( pDriver == nullptr ) {
				bCheckJack = false;
			}
		}
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::PortAudio );
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::CoreAudio );
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::PulseAudio );
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Disk );
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Null );
		pAudioEngine->stopAudioDriver();
		pAudioEngine->createAudioDriver(
			H2Core::Preferences::AudioDriver::Fake );
	}

	___INFOLOG("done");
}

void AudioDriverTest::tearDown() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	auto pPref = H2Core::Preferences::get_instance();
	pPref->m_nBufferSize = m_nPrevBufferSize;
	pPref->m_audioDriver = m_prevAudioDriver;

	pAudioEngine->stopAudioDriver();
	pAudioEngine->createAudioDriver(
		H2Core::Preferences::AudioDriver::Fake );
}
