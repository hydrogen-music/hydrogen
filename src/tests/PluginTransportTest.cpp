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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "PluginTransportTest.h"

#include <vector>

#include <core/AudioEngine/AudioEngine.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Logger.h>

#include <plugin/HostTransportLatch.h>
#include <plugin/HydrogenPlugin.h>

using namespace H2Core;

namespace {

constexpr uint32_t knFrames = 256;

/** A valid rolling reading. */
HostTransportReading rolling( long long nFrame, double fBpm = 120.0 ) {
	return { true, true, fBpm, nFrame };
}
/** A valid parked reading. */
HostTransportReading parked( long long nFrame, double fBpm = 120.0 ) {
	return { true, false, fBpm, nFrame };
}
/** A block without host transport. */
const HostTransportReading kInvalid{};

/** Master-only output buffers for one block (bus-less instances take an
 *  empty bus vector). */
struct TransportBuffers {
	std::vector<float> masterL, masterR;
	std::vector<float*> busLPtr, busRPtr;

	explicit TransportBuffers( unsigned nFrames )
		: masterL( nFrames, 0.0f ), masterR( nFrames, 0.0f ) {}
};

void processBlock( HydrogenPlugin& plugin, TransportBuffers& buf,
				   const HostTransportReading& host ) {
	plugin.process( knFrames, buf.masterL.data(), buf.masterR.data(),
					buf.busLPtr, buf.busRPtr, host );
}

/** Drain the instance's event queue and count Relocation events - the
 *  engine's only "the transport jumped" signal in the plugin path
 *  (AudioEngine::locateToFrame). */
int countRelocations( Hydrogen* pHydrogen ) {
	int nCount = 0;
	while ( auto pEvent = pHydrogen->getEventQueue()->popEvent() ) {
		if ( pEvent->getType() == Event::Type::Relocation ) {
			++nCount;
		}
	}
	return nCount;
}

} // namespace

void PluginTransportTest::testLatchAbsorbsRollingJitter() {
	___INFOLOG( "" );

	// The host clock is exact, but the position it reports jitters: the
	// CLAP sectime round-trip is lossy at the frame level, so every other
	// block reports one frame less than the true position. The latch must
	// present the true contiguous clock regardless.
	HostTransportLatch latch;
	long long nTrue = 0;
	for ( int ii = 0; ii < 32; ++ii ) {
		const long long nReported = nTrue - ( ii % 2 == 1 ? 1 : 0 );
		const HostTransportReading presented =
			latch.next( rolling( nReported ), knFrames );
		CPPUNIT_ASSERT( presented.bValid );
		CPPUNIT_ASSERT( presented.bRolling );
		CPPUNIT_ASSERT_EQUAL( nTrue, presented.nFrame );
		nTrue += knFrames;
	}

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchAbsorbsParkedJitter() {
	___INFOLOG( "" );

	// A parked host keeps re-reporting its position with the same jitter;
	// the presented position stays pinned at the adopted one.
	HostTransportLatch latch;
	CPPUNIT_ASSERT_EQUAL( (long long)1000,
						  latch.next( parked( 1000 ), knFrames ).nFrame );
	CPPUNIT_ASSERT_EQUAL( (long long)1000,
						  latch.next( parked( 999 ), knFrames ).nFrame );
	CPPUNIT_ASSERT_EQUAL( (long long)1000,
						  latch.next( parked( 1001 ), knFrames ).nFrame );
	CPPUNIT_ASSERT_EQUAL( (long long)1000,
						  latch.next( parked( 1000 ), knFrames ).nFrame );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchHoldsOverAbsentTransport() {
	___INFOLOG( "" );

	// A write-on-change host stops sending transport objects; while the
	// latched state rolls, the latch holds it and self-advances.
	HostTransportLatch latch;
	latch.next( rolling( 0 ), knFrames );
	for ( int ii = 1; ii <= 4; ++ii ) {
		const HostTransportReading presented =
			latch.next( kInvalid, knFrames );
		CPPUNIT_ASSERT( presented.bValid );
		CPPUNIT_ASSERT( presented.bRolling );
		CPPUNIT_ASSERT_EQUAL( static_cast<long long>( ii * knFrames ),
							  presented.nFrame );
	}

	// While parked it holds the position without advancing.
	latch.next( parked( 2000 ), knFrames );
	for ( int ii = 0; ii < 4; ++ii ) {
		const HostTransportReading presented =
			latch.next( kInvalid, knFrames );
		CPPUNIT_ASSERT( presented.bValid );
		CPPUNIT_ASSERT( ! presented.bRolling );
		CPPUNIT_ASSERT_EQUAL( (long long)2000, presented.nFrame );
	}

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchClampsNegativeFrames() {
	___INFOLOG( "" );

	// Host pre-roll reports negative song positions; the engine contract
	// has no negative frames, so the latch clamps to 0.
	HostTransportLatch latch;
	const HostTransportReading presented =
		latch.next( rolling( -3000 ), knFrames );
	CPPUNIT_ASSERT( presented.bValid );
	CPPUNIT_ASSERT_EQUAL( (long long)0, presented.nFrame );

	// The roll continues contiguously from the clamped position.
	const HostTransportReading after =
		latch.next( rolling( knFrames ), knFrames );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( knFrames ),
						  after.nFrame );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchTempoGating() {
	___INFOLOG( "" );

	// A real tempo passes through and is remembered.
	HostTransportLatch latch;
	HostTransportReading presented =
		latch.next( rolling( 0, 120.0 ), knFrames );
	CPPUNIT_ASSERT( presented.bValid );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 120.0, presented.fBpm, 0.001 );

	// A block without tempo (fBpm <= 0) keeps the latched tempo.
	presented = latch.next( rolling( knFrames, 0.0 ), knFrames );
	CPPUNIT_ASSERT( presented.bValid );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 120.0, presented.fBpm, 0.001 );

	// Tempo changes pass through.
	presented = latch.next( rolling( 2 * knFrames, 96.0 ), knFrames );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 96.0, presented.fBpm, 0.001 );

	// Without any tempo ever seen there is nothing to hold: 0 is
	// presented and the engine keeps its own tempo.
	HostTransportLatch fresh;
	presented = fresh.next( rolling( 0, 0.0 ), knFrames );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, presented.fBpm, 0.001 );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchAdoptsSeek() {
	___INFOLOG( "" );

	HostTransportLatch latch;
	latch.next( rolling( 0 ), knFrames );
	latch.next( rolling( knFrames ), knFrames );

	// The host seeks far beyond the jitter tolerance: adopted as-is, not
	// absorbed into the expected position.
	const HostTransportReading presented =
		latch.next( rolling( 441000 ), knFrames );
	CPPUNIT_ASSERT( presented.bValid );
	CPPUNIT_ASSERT_EQUAL( (long long)441000, presented.nFrame );

	// The roll continues contiguously from the seek point.
	const HostTransportReading after =
		latch.next( rolling( 441000 + knFrames ), knFrames );
	CPPUNIT_ASSERT_EQUAL( (long long)( 441000 + knFrames ), after.nFrame );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchAdoptsLoopWrap() {
	___INFOLOG( "" );

	HostTransportLatch latch;
	latch.next( rolling( 1764000 ), knFrames );
	latch.next( rolling( 1764000 + knFrames ), knFrames );

	// The host loops back to its region start: a backwards jump is a
	// discontinuity like a seek, adopted as-is.
	const HostTransportReading presented = latch.next( rolling( 0 ), knFrames );
	CPPUNIT_ASSERT_EQUAL( (long long)0, presented.nFrame );

	const HostTransportReading after =
		latch.next( rolling( knFrames ), knFrames );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( knFrames ), after.nFrame );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchStopParkAndResume() {
	___INFOLOG( "" );

	HostTransportLatch latch;
	latch.next( rolling( 0 ), knFrames );
	latch.next( rolling( knFrames ), knFrames );

	// Stop: the host parks at the position its clock reached.
	HostTransportReading presented =
		latch.next( parked( 2 * knFrames ), knFrames );
	CPPUNIT_ASSERT( presented.bValid );
	CPPUNIT_ASSERT( ! presented.bRolling );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 2 * knFrames ),
						  presented.nFrame );

	// Further parked blocks hold the position.
	presented = latch.next( parked( 2 * knFrames ), knFrames );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 2 * knFrames ),
						  presented.nFrame );

	// Resume from the parked position: adopted, then contiguous again.
	presented = latch.next( rolling( 2 * knFrames ), knFrames );
	CPPUNIT_ASSERT( presented.bRolling );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 2 * knFrames ),
						  presented.nFrame );
	presented = latch.next( rolling( 3 * knFrames ), knFrames );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 3 * knFrames ),
						  presented.nFrame );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testLatchInvalidUntilTransportSeen() {
	___INFOLOG( "" );

	// Before any transport was seen, invalid readings present nothing:
	// the engine is left in its own state instead of following a
	// phantom position.
	HostTransportLatch latch;
	for ( int ii = 0; ii < 4; ++ii ) {
		const HostTransportReading presented =
			latch.next( kInvalid, knFrames );
		CPPUNIT_ASSERT( ! presented.bValid );
	}

	// The first real transport is adopted as-is.
	const HostTransportReading presented =
		latch.next( parked( 7 * knFrames ), knFrames );
	CPPUNIT_ASSERT( presented.bValid );
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 7 * knFrames ),
						  presented.nFrame );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testHostJitterDoesNotRelocate() {
	___INFOLOG( "" );

	// The regression this latch exists for (ADR 0013): a host whose
	// reported position jitters by a frame around the true position must
	// not be treated as a relocate by the engine. A relocate resets the
	// tick lookahead and enqueues notes a second time on top of the
	// still-playing first ones - the chorus glitch.
	HydrogenPlugin plugin( 44100, 256, 0 );
	TransportBuffers buf( 256 );
	auto* pHydrogen = plugin.getHydrogen();
	const auto pPlayhead = pHydrogen->getAudioEngine()->getPlayhead();

	long long nTrue = 0;
	for ( int ii = 0; ii < 32; ++ii ) {
		const long long nReported = nTrue - ( ii % 2 == 1 ? 1 : 0 );
		processBlock( plugin, buf, rolling( nReported ) );
		nTrue += knFrames;
		// The engine advanced by exactly one block, matching the true
		// host clock - not the jittered reading.
		CPPUNIT_ASSERT_EQUAL( nTrue, pPlayhead->getFrame() );
	}

	// No block was treated as a relocate.
	CPPUNIT_ASSERT_EQUAL( 0, countRelocations( pHydrogen ) );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testHostSeekRelocatesOnce() {
	___INFOLOG( "" );

	HydrogenPlugin plugin( 44100, 256, 0 );
	TransportBuffers buf( 256 );
	auto* pHydrogen = plugin.getHydrogen();
	const auto pPlayhead = pHydrogen->getAudioEngine()->getPlayhead();

	// Four contiguous rolling blocks: no relocate.
	for ( int ii = 0; ii < 4; ++ii ) {
		processBlock( plugin, buf, rolling( ii * knFrames ) );
	}
	CPPUNIT_ASSERT_EQUAL( static_cast<long long>( 4 * knFrames ),
						  pPlayhead->getFrame() );

	// The host seeks 1 s into the song: exactly one relocate, and the
	// engine keeps rolling contiguously from the new position.
	processBlock( plugin, buf, rolling( 441000 ) );
	CPPUNIT_ASSERT_EQUAL( (long long)( 441000 + knFrames ),
						  pPlayhead->getFrame() );

	for ( int ii = 1; ii <= 8; ++ii ) {
		processBlock( plugin, buf, rolling( 441000 + ii * knFrames ) );
	}
	CPPUNIT_ASSERT_EQUAL( (long long)( 441000 + 9 * knFrames ),
						  pPlayhead->getFrame() );

	CPPUNIT_ASSERT_EQUAL( 1, countRelocations( pHydrogen ) );

	___INFOLOG( "passed" );
}

void PluginTransportTest::testTransportlessHostLeavesEngineAlone() {
	___INFOLOG( "" );

	// A host that never publishes transport (a bare note generator): no
	// synthesized position may reach the engine - the playhead stays
	// parked (realtime notes run on m_nRealtimeFrame either way) and
	// nothing relocates.
	HydrogenPlugin plugin( 44100, 256, 0 );
	TransportBuffers buf( 256 );
	auto* pHydrogen = plugin.getHydrogen();
	auto* pAudioEngine = pHydrogen->getAudioEngine();

	for ( int ii = 0; ii < 32; ++ii ) {
		processBlock( plugin, buf, kInvalid );
	}

	CPPUNIT_ASSERT_EQUAL( (long long)0,
						  pAudioEngine->getPlayhead()->getFrame() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT_EQUAL( 0, countRelocations( pHydrogen ) );

	___INFOLOG( "passed" );
}
