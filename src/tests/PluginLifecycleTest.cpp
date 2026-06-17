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

#include <cmath>
#include <memory>
#include <vector>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/H2Project.h>
#include <core/Hydrogen.h>
#include <core/Midi/Midi.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <plugin/HydrogenPlugin.h>

using namespace H2Core;

namespace {
// Allocate L/R + nBuses stereo buffers of nFrames each, zeroed.
struct PluginBuffers {
	std::vector<float> masterL, masterR;
	std::vector<std::vector<float>> busL, busR;
	std::vector<float*> busLPtr, busRPtr;

	PluginBuffers( unsigned nFrames, int nBuses )
		: masterL( nFrames, 0.0f ), masterR( nFrames, 0.0f ) {
		for ( int ii = 0; ii < nBuses; ++ii ) {
			busL.emplace_back( nFrames, 0.0f );
			busR.emplace_back( nFrames, 0.0f );
		}
		for ( int ii = 0; ii < nBuses; ++ii ) {
			busLPtr.push_back( busL[ ii ].data() );
			busRPtr.push_back( busR[ ii ].data() );
		}
	}
};

bool allFinite( const std::vector<float>& buf, unsigned n ) {
	for ( unsigned i = 0; i < n; ++i ) {
		if ( ! std::isfinite( buf[i] ) ) {
			return false;
		}
	}
	return true;
}
} // namespace

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

void PluginLifecycleTest::testPluginProcess() {
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	const int nBuses = 4;

	HydrogenPlugin plugin( nSampleRate, nBlock, nBuses );
	plugin.activate( nSampleRate, nBlock );
	CPPUNIT_ASSERT_EQUAL( nBuses, plugin.getBusCount() );

	PluginBuffers buf( nBlock, nBuses );

	// Idle (not rolling): output stays finite and silent.
	for ( int b = 0; b < 4; ++b ) {
		plugin.process( nBlock, buf.masterL.data(), buf.masterR.data(),
						buf.busLPtr, buf.busRPtr,
						/*bRolling=*/false, 120.0, 0 );
		CPPUNIT_ASSERT( allFinite( buf.masterL, nBlock ) );
		CPPUNIT_ASSERT( allFinite( buf.masterR, nBlock ) );
	}

	// Rolling + a note: still finite, no NaN/denormals leaking to the host.
	plugin.noteOn( 36, 100, static_cast<int>( Midi::ChannelDefault ) );
	long long nFrame = 0;
	for ( int b = 0; b < 8; ++b ) {
		plugin.process( nBlock, buf.masterL.data(), buf.masterR.data(),
						buf.busLPtr, buf.busRPtr,
						/*bRolling=*/true, 130.0, nFrame );
		CPPUNIT_ASSERT( allFinite( buf.masterL, nBlock ) );
		CPPUNIT_ASSERT( allFinite( buf.masterR, nBlock ) );
		for ( int bus = 0; bus < nBuses; ++bus ) {
			CPPUNIT_ASSERT( allFinite( buf.busL[ bus ], nBlock ) );
			CPPUNIT_ASSERT( allFinite( buf.busR[ bus ], nBlock ) );
		}
		nFrame += nBlock;
	}

	plugin.deactivate();

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testPluginStateRoundTrip() {
	___INFOLOG( "" );

	HydrogenPlugin plugin( 44100, 512, 4 );

	// Embedded state is a portable .h2project bundle.
	const auto state = plugin.saveState( /*bEmbedSamples=*/true );
	CPPUNIT_ASSERT( ! state.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( state ) );

	// A fresh plugin instance restores it.
	HydrogenPlugin other( 48000, 256, 2 );
	CPPUNIT_ASSERT( other.loadState( state ) );
	CPPUNIT_ASSERT( other.getHydrogen()->getSong() != nullptr );

	// Song-only state round-trips too.
	const auto songOnly = plugin.saveState( /*bEmbedSamples=*/false );
	CPPUNIT_ASSERT( ! songOnly.empty() );
	CPPUNIT_ASSERT( ! H2Project::looksLikeArchive( songOnly ) );
	CPPUNIT_ASSERT( other.loadState( songOnly ) );

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testPluginRepeatedLifecycle() {
	___INFOLOG( "" );

	// A host instantiates and destroys plugin instances repeatedly within one
	// process; doing so must leave no residual global state (ADR 0015).
	const int nBaseline = Base::getAliveObjectCount();

	for ( int ii = 0; ii < 4; ++ii ) {
		auto* pPlugin = new HydrogenPlugin( 44100, 256, 2 );
		PluginBuffers buf( 256, 2 );
		pPlugin->process( 256, buf.masterL.data(), buf.masterR.data(),
						  buf.busLPtr, buf.busRPtr, false, 120.0, 0 );
		delete pPlugin;
		CPPUNIT_ASSERT_EQUAL( nBaseline, Base::getAliveObjectCount() );
	}

	___INFOLOG( "passed" );
}
