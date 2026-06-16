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

#include "PluginProcessTest.h"

#include "utils/FakePluginHost.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/IO/AudioDriver.h>
#include <core/Object.h>

#include <cmath>
#include <vector>

using namespace H2Core;

// The host-driven plugin path must survive arbitrary host block sizes: real
// hosts hand us odd, non-power-of-two frame counts. Each must render without
// crashing and produce finite output.
void PluginProcessTest::testOddBlockSizes() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 1024 );

	const unsigned oddSizes[] = { 1, 3, 7, 13, 17, 64, 127, 333, 511, 1023 };
	for ( unsigned nFrames : oddSizes ) {
		int nResult = host.process( nFrames );
		CPPUNIT_ASSERT_EQUAL( 0, nResult );

		const auto& outL = host.getLastOutputL();
		const auto& outR = host.getLastOutputR();
		for ( unsigned i = 0; i < nFrames; ++i ) {
			CPPUNIT_ASSERT( std::isfinite( outL[i] ) );
			CPPUNIT_ASSERT( std::isfinite( outR[i] ) );
		}
	}

	___INFOLOG( "passed" );
}

// An idle engine (empty song, transport stopped) must emit pure silence: a
// plugin that hums while the host is paused is a defect.
void PluginProcessTest::testSilenceInSilenceOut() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 512 );
	CPPUNIT_ASSERT_EQUAL( false, host.isPlaying() );

	for ( int block = 0; block < 8; ++block ) {
		host.process( 512 );
		const auto& outL = host.getLastOutputL();
		const auto& outR = host.getLastOutputR();
		for ( unsigned i = 0; i < 512; ++i ) {
			CPPUNIT_ASSERT_EQUAL( 0.0f, outL[i] );
			CPPUNIT_ASSERT_EQUAL( 0.0f, outR[i] );
		}
	}

	___INFOLOG( "passed" );
}

// The host may change its block size between cycles; the driver must report the
// new size and keep rendering correctly.
void PluginProcessTest::testBlockSizeChange() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	host.process( 256 );

	host.setBlockSize( 1024 );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )1024, host.getBlockSize() );
	int nResult = host.process( 1024 );
	CPPUNIT_ASSERT_EQUAL( 0, nResult );
	// The driver, asked for a 1024-frame block, reports that size to the engine.
	CPPUNIT_ASSERT_EQUAL(
		( unsigned )1024, host.getHydrogen()->getAudioDriver()->getBufferSize() );

	host.setBlockSize( 128 );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )128, host.getBlockSize() );
	nResult = host.process( 128 );
	CPPUNIT_ASSERT_EQUAL( 0, nResult );
	CPPUNIT_ASSERT_EQUAL(
		( unsigned )128, host.getHydrogen()->getAudioDriver()->getBufferSize() );

	___INFOLOG( "passed" );
}

// The host may change its sample rate mid-stream (e.g. session reconfigured).
// The driver must report the new rate so the engine recomputes tick size.
void PluginProcessTest::testSampleRateChange() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 512 );
	CPPUNIT_ASSERT_EQUAL(
		( unsigned )44100, host.getHydrogen()->getAudioDriver()->getSampleRate() );
	host.process( 512 );

	host.setSampleRate( 48000 );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )48000, host.getSampleRate() );
	CPPUNIT_ASSERT_EQUAL(
		( unsigned )48000, host.getHydrogen()->getAudioDriver()->getSampleRate() );
	int nResult = host.process( 512 );
	CPPUNIT_ASSERT_EQUAL( 0, nResult );

	host.setSampleRate( 96000 );
	CPPUNIT_ASSERT_EQUAL(
		( unsigned )96000, host.getHydrogen()->getAudioDriver()->getSampleRate() );
	nResult = host.process( 512 );
	CPPUNIT_ASSERT_EQUAL( 0, nResult );

	___INFOLOG( "passed" );
}

// No sample may ever be NaN or infinite — those poison a host's mix bus. Drive
// the engine through playback and varied block sizes and scan every sample.
void PluginProcessTest::testNoNaN() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 1024 );
	host.setBpm( 140.0f );
	host.setPlaying( true );

	const unsigned sizes[] = { 1024, 512, 333, 1024, 7, 1024 };
	for ( unsigned nFrames : sizes ) {
		host.process( nFrames );
		const auto& outL = host.getLastOutputL();
		const auto& outR = host.getLastOutputR();
		for ( unsigned i = 0; i < nFrames; ++i ) {
			CPPUNIT_ASSERT( ! std::isnan( outL[i] ) );
			CPPUNIT_ASSERT( ! std::isnan( outR[i] ) );
			CPPUNIT_ASSERT( ! std::isinf( outL[i] ) );
			CPPUNIT_ASSERT( ! std::isinf( outR[i] ) );
		}
	}

	host.setPlaying( false );

	___INFOLOG( "passed" );
}

// Two independent host instances given the same configuration and the same
// sequence of blocks must render bit-identical output: the plugin path is
// deterministic, with no hidden shared state between instances (ADR 0015).
void PluginProcessTest::testDeterministicRender() {
	___INFOLOG( "" );

	auto render = []( std::vector<float>& outL, std::vector<float>& outR ) {
		FakePluginHost host( 44100, 256 );
		host.setBpm( 120.0f );
		host.setPlaying( true );
		for ( int block = 0; block < 6; ++block ) {
			host.process( 256 );
		}
		outL = host.getLastOutputL();
		outR = host.getLastOutputR();
		host.setPlaying( false );
	};

	std::vector<float> aL, aR, bL, bR;
	render( aL, aR );
	render( bL, bR );

	CPPUNIT_ASSERT_EQUAL( aL.size(), bL.size() );
	CPPUNIT_ASSERT_EQUAL( aR.size(), bR.size() );
	for ( size_t i = 0; i < aL.size(); ++i ) {
		CPPUNIT_ASSERT_EQUAL( aL[i], bL[i] );
		CPPUNIT_ASSERT_EQUAL( aR[i], bR[i] );
	}

	___INFOLOG( "passed" );
}
