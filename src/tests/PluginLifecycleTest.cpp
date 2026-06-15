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

#include "PluginLifecycleTest.h"

#include <memory>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

// A Preferences configured for a headless, host-less secondary instance: fake
// audio, no MIDI, no OSC. create_instance() returns a freshly-owned object
// (ADR 0015), so each instance gets its own.
static std::shared_ptr<Preferences> makePluginPreferences() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->m_nBufferSize = 1024;
	pPref->setOscServerEnabled( false );
	return pPref;
}

void PluginLifecycleTest::testRepeatedLifecycle() {
	___INFOLOG( "" );

	// A plugin host instantiates and destroys engine instances repeatedly within
	// one process; doing so must leave no residual global state (ADR 0015). The
	// harness's own Hydrogen is alive throughout, so the alive-object count after
	// each destroyed instance must return to this baseline.
	const int nBaseline = Base::getAliveObjectCount();
	const int nIterations = 5;

	for ( int ii = 0; ii < nIterations; ++ii ) {
		// Each instance owns its own OSC server / NSM client (per-instance, not a
		// singleton — ADR 0015); with OSC disabled they never touch the network.
		auto* pHydrogen = new Hydrogen( makePluginPreferences(), -1 );

		CPPUNIT_ASSERT( pHydrogen->getAudioEngine() != nullptr );
		CPPUNIT_ASSERT( pHydrogen->getPreferences() != nullptr );
		CPPUNIT_ASSERT( pHydrogen->getEventQueue() != nullptr );
		CPPUNIT_ASSERT( pHydrogen->getSong() != nullptr );

		// Use it.
		const float fBpm = 123.0f + static_cast<float>( ii );
		pHydrogen->getSong()->setBpm( fBpm );
		CPPUNIT_ASSERT_DOUBLES_EQUAL(
			fBpm, pHydrogen->getSong()->getBpm(), 0.001 );

		delete pHydrogen;

		// No residual global state: every object the instance constructed has
		// been destructed, so we are back to the baseline.
		CPPUNIT_ASSERT_EQUAL( nBaseline, Base::getAliveObjectCount() );
	}

	___INFOLOG( "passed" );
}
