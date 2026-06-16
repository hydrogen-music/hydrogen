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

#include "OutputBusTest.h"

#include "utils/FakePluginHost.h"

#include <core/Hydrogen.h>
#include <core/IO/PluginAudioDriver.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <vector>

using namespace H2Core;

namespace {

constexpr unsigned nBlock = 256;

// Which outputs carried any signal across a run of process blocks.
struct Signals {
	std::vector<bool> bus;
	bool master = false;
};

// Configure a host so MIDI note (NoteOffset + i) maps to instrument i (Order
// mode), pinned to the default input channel for determinism.
void configureOrderMapping( FakePluginHost& host ) {
	auto pMap = host.getHydrogen()->getPreferences()->getMidiInstrumentMap();
	pMap->setInput( MidiInstrumentMap::Input::Order );
	pMap->setUseGlobalInputChannel( true );
	pMap->setGlobalInputChannel( Midi::ChannelDefault );
}

bool bufferHasSignal( float* pBuffer ) {
	if ( pBuffer == nullptr ) {
		return false;
	}
	for ( unsigned i = 0; i < nBlock; ++i ) {
		if ( pBuffer[i] != 0.0f ) {
			return true;
		}
	}
	return false;
}

// Process nBlocks blocks, recording which buses and the master ever carried a
// non-zero sample (the sample may finish rendering before the last block, so we
// OR across all blocks).
Signals run( FakePluginHost& host, int nBlocks ) {
	Signals s;
	s.bus.assign( host.getBusCount(), false );
	for ( int b = 0; b < nBlocks; ++b ) {
		host.process( nBlock );
		for ( unsigned bus = 0; bus < host.getBusCount(); ++bus ) {
			if ( bufferHasSignal( host.getBusOutputL( bus ) ) ||
				 bufferHasSignal( host.getBusOutputR( bus ) ) ) {
				s.bus[ bus ] = true;
			}
		}
		if ( bufferHasSignal( host.getOutputL() ) ||
			 bufferHasSignal( host.getOutputR() ) ) {
			s.master = true;
		}
	}
	return s;
}

} // namespace

// The host exposes exactly N buses and the engine's driver agrees once the host
// has published them.
void OutputBusTest::testActiveBusCount() {
	___INFOLOG( "" );

	const unsigned nBuses = 4;
	FakePluginHost host( 44100, nBlock, nBuses );
	CPPUNIT_ASSERT_EQUAL( nBuses, host.getBusCount() );

	host.process( nBlock );

	auto pDriver = std::dynamic_pointer_cast<PluginAudioDriver>(
		host.getHydrogen()->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );
	CPPUNIT_ASSERT_EQUAL( static_cast<int>( nBuses ), pDriver->getBusCount() );

	___INFOLOG( "passed" );
}

// The first instrument is routed to bus 0 only (1-to-1 mapping), nothing leaks
// onto the other buses, and the master carries the sum.
void OutputBusTest::testInstrumentRoutedToOwnBus() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, nBlock, 4 );
	configureOrderMapping( host );
	host.setPlaying( true );

	host.addNoteOn( 0, static_cast<int>( Midi::NoteOffset ) + 0, 100 );
	const auto s = run( host, 8 );

	CPPUNIT_ASSERT( s.bus[0] );      // instrument 0 -> bus 0
	CPPUNIT_ASSERT( ! s.bus[1] );    // no auto-sharing onto other buses
	CPPUNIT_ASSERT( ! s.bus[2] );
	CPPUNIT_ASSERT( ! s.bus[3] );
	CPPUNIT_ASSERT( s.master );      // master carries the full sum

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

// The second instrument lands on bus 1 (by kit order), leaving bus 0 silent.
void OutputBusTest::testSecondInstrumentToSecondBus() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, nBlock, 4 );
	configureOrderMapping( host );
	host.setPlaying( true );

	host.addNoteOn( 0, static_cast<int>( Midi::NoteOffset ) + 1, 100 );
	const auto s = run( host, 8 );

	CPPUNIT_ASSERT( ! s.bus[0] );
	CPPUNIT_ASSERT( s.bus[1] );      // instrument 1 -> bus 1
	CPPUNIT_ASSERT( ! s.bus[2] );
	CPPUNIT_ASSERT( ! s.bus[3] );
	CPPUNIT_ASSERT( s.master );

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

// An instrument whose kit index exceeds the bus count routes to the master only
// (kits with more instruments than buses): no bus carries it.
void OutputBusTest::testSurplusInstrumentMasterOnly() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, nBlock, 4 );
	configureOrderMapping( host );
	host.setPlaying( true );

	// Instrument 5 is beyond the 4 buses -> master only.
	host.addNoteOn( 0, static_cast<int>( Midi::NoteOffset ) + 5, 100 );
	const auto s = run( host, 8 );

	for ( unsigned bus = 0; bus < host.getBusCount(); ++bus ) {
		CPPUNIT_ASSERT( ! s.bus[ bus ] );
	}
	CPPUNIT_ASSERT( s.master );

	host.setPlaying( false );

	___INFOLOG( "passed" );
}
