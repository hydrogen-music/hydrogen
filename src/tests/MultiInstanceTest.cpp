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

#include "MultiInstanceTest.h"

#include <memory>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include "TestHelper.h"

using namespace H2Core;

// A Preferences for a headless, host-less secondary instance: fake audio, no
// MIDI, no OSC. create_instance() returns a freshly-owned object (ADR 0015).
static std::shared_ptr<Preferences> makePluginPreferences() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->m_nBufferSize = 1024;
	pPref->setOscServerEnabled( false );
	return pPref;
}

void MultiInstanceTest::testInstanceOwnsContext() {
	___INFOLOG( "" );

	auto pHydrogen = pTestHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );

	// A Hydrogen instance owns its Preferences and EventQueue and exposes them
	// through the per-instance API; no process-wide singleton is involved
	// (ADR 0015).
	CPPUNIT_ASSERT( pHydrogen->getPreferences() != nullptr );
	CPPUNIT_ASSERT( pHydrogen->getEventQueue() != nullptr );
	CPPUNIT_ASSERT( pHydrogen->getAudioEngine() != nullptr );

	___INFOLOG( "passed" );
}

void MultiInstanceTest::testTwoIndependentInstances() {
	___INFOLOG( "" );

	// Two live engine instances coexist in one process (alongside the harness's
	// own), each owning its Preferences/EventQueue/AudioEngine/Song *and* its own
	// OSC server / NSM client (per-instance, not singletons — ADR 0015). OSC is
	// disabled here so neither binds a port.
	auto* pA = new Hydrogen(
		makePluginPreferences(), Hydrogen::ProcessMode::Headless, -1
	);
	auto* pB = new Hydrogen(
		makePluginPreferences(), Hydrogen::ProcessMode::Headless, -1
	);

	// EventQueue::pushEvent() drops events while an instance is still in the
	// startup ProcessMode; a headless/plugin instance runs in 'headless'.
	pA->setFullyOperational( true );
	pB->setFullyOperational( true );

	// Distinct instances own distinct context objects.
	CPPUNIT_ASSERT( pA != pB );
	CPPUNIT_ASSERT( pA->getAudioEngine() != pB->getAudioEngine() );
	CPPUNIT_ASSERT( pA->getPreferences() != pB->getPreferences() );
	CPPUNIT_ASSERT( pA->getEventQueue() != pB->getEventQueue() );
	CPPUNIT_ASSERT( pA->getSong() != pB->getSong() );

	// Independent tempo: mutating one song never affects the other.
	pA->getSong()->setBpm( 140.0f );
	pB->getSong()->setBpm( 90.0f );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 140.0, pA->getSong()->getBpm(), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 90.0, pB->getSong()->getBpm(), 0.001 );

	// Independent Preferences.
	pA->getPreferences()->m_nBufferSize = 256;
	pB->getPreferences()->m_nBufferSize = 512;
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pA->getPreferences()->m_nBufferSize );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )512, pB->getPreferences()->m_nBufferSize );

	// Independent EventQueue: an event pushed onto one is never seen on the
	// other. Drain any startup/driver events first, then push a sentinel onto A
	// and confirm it never surfaces on B.
	while ( pA->getEventQueue()->popEvent() != nullptr ) {}
	while ( pB->getEventQueue()->popEvent() != nullptr ) {}

	const int nSentinel = 4242;
	pA->getEventQueue()->pushEvent( Event::Type::Metronome, nSentinel );

	bool bSentinelOnB = false;
	std::unique_ptr<Event> pEvent;
	while ( ( pEvent = pB->getEventQueue()->popEvent() ) != nullptr ) {
		if ( pEvent->getType() == Event::Type::Metronome &&
			 pEvent->getValue() == nSentinel ) {
			bSentinelOnB = true;
		}
	}
	CPPUNIT_ASSERT( ! bSentinelOnB );

	bool bSentinelOnA = false;
	while ( ( pEvent = pA->getEventQueue()->popEvent() ) != nullptr ) {
		if ( pEvent->getType() == Event::Type::Metronome &&
			 pEvent->getValue() == nSentinel ) {
			bSentinelOnA = true;
		}
	}
	CPPUNIT_ASSERT( bSentinelOnA );

	delete pA;
	delete pB;

	___INFOLOG( "passed" );
}
