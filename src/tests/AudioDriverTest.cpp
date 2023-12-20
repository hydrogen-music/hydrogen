/*
 * Hydrogen
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Preferences/Preferences.h>

void AudioDriverTest::setUp() {
	auto pPref = H2Core::Preferences::get_instance();
	m_nPrevBufferSize = pPref->m_nBufferSize;
	m_sPrevAudioDriver = pPref->m_sAudioDriver;
}

void AudioDriverTest::testDriverSwitching() {
	___INFOLOG("");

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	for ( int ii = 0; ii < 10; ++ii ) {
		std::cout << ii << std::endl;
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "ALSA" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "OSS" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "JACK" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "PortAudio" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "CoreAudio" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "PulseAudio" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "DiskWriterDriver" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "NullDriver" );
		pAudioEngine->stopAudioDrivers();
		pAudioEngine->createAudioDriver( "Fake" );
	}

	___INFOLOG("done");
}

void AudioDriverTest::tearDown() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	auto pPref = H2Core::Preferences::get_instance();
	pPref->m_nBufferSize = m_nPrevBufferSize;
	pPref->m_sAudioDriver = m_sPrevAudioDriver;

	pAudioEngine->stopAudioDrivers();
	pAudioEngine->createAudioDriver( "Fake" );
}
