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

#ifndef H2C_HOST_TRANSPORT_LATCH_H
#define H2C_HOST_TRANSPORT_LATCH_H

// Host transport latch (ADR 0013). Header-only and free of any plugin SDK
// on purpose: the format shims (CLAP and VST3 via HydrogenClap.cpp, LV2 via
// HydrogenLv2.cpp), HydrogenPlugin::process() and the unit tests
// (PluginTransportTest) all share this source, so the transport policy is
// specified and tested without a host.

#include <cstdint>
#include <cstdlib>

namespace H2Core {

/** One host transport reading for an upcoming process block, as parsed by
 *  a format shim. */
struct HostTransportReading {
	/** Whether the host supplied a usable transport for this block. A
	 *  shim only sets this when it has a position to offer: rolling
	 *  state or tempo alone would start the engine from an arbitrary
	 *  position, so they do not make a transport usable. */
	bool bValid = false;
	/** Host transport rolling this block. */
	bool bRolling = false;
	/** Host tempo in BPM. <= 0 means "no tempo this block"; the engine
	 *  then keeps its own. */
	double fBpm = 0.0;
	/** Host transport frame at the start of the block. Negative
	 *  positions (host pre-roll) are clamped to 0 by the latch. */
	long long nFrame = 0;
};

/** Sanitizes host transport readings into the contiguous clock the audio
 *  engine contract requires (ADR 0013): the frame published for the start
 *  of block N+1 must equal the frame published for block N plus block N's
 *  frame count, unless the host genuinely relocated.
 *
 *  Raw readings violate that contract routinely: CLAP positions arrive as
 *  clap_sectime whose round-trip through double seconds is lossy at the
 *  frame level (the VST3 build inherits the same path through
 *  clap-wrapper), so consecutive readings jitter by a frame around the
 *  true position. Fed to the engine as-is, every other block looks like a
 *  relocate: the tick lookahead is reset and re-consumed, and notes are
 *  enqueued a second time on top of the still-sounding first ones.
 *
 *  Policy, per block:
 *  - Contiguous transport (same rolling state on both sides, frame within
 *    #knJitterToleranceFrames of the expected position): present the
 *    expected position, absorbing the jitter. Both clocks advance by the
 *    block's frame count, so absorption cannot accumulate drift.
 *  - Any discontinuity beyond the tolerance - a seek, a loop wrap, a
 *    stop/resume transition, the first transport ever: adopt the host
 *    reading as-is.
 *  - No transport this block: hold the latched state (self-advancing
 *    while rolling), like the LV2 latch.
 *  - No transport ever: present nothing (bValid false). The engine is
 *    left in its own state: the playhead stays parked - it only moves
 *    on a rolling host - while realtime notes keep triggering on
 *    AudioEngine::m_nRealtimeFrame, the engine-internal clock that is
 *    independent of transport and audio driver alike. */
struct HostTransportLatch {
	/** How far a reading may sit from the expected position and still
	 *  count as the same position. The sectime round-trip error stays
	 *  below half a frame once rounded to nearest; two frames also cover
	 *  small host quirks without masking real seeks (which move by at
	 *  least a buffer). */
	static constexpr long long knJitterToleranceFrames = 2;

	/** The transport to publish for the block of @a nFrames frames that
	 *  is about to render. */
	HostTransportReading next( const HostTransportReading& host,
							   uint32_t nFrames ) {
		HostTransportReading presented;

		if ( ! host.bValid ) {
			if ( ! m_bHaveTransport ) {
				// Never seen transport: nothing to present. The engine
				// stays parked on its own clock; realtime notes run on
				// m_nRealtimeFrame regardless.
				return presented;
			}
			// Hold the latched state: self-advance while rolling, freeze
			// while parked.
			presented.bValid = true;
			presented.bRolling = m_bRolling;
			presented.fBpm = m_fBpm;
			if ( m_bRolling ) {
				m_nFrame = m_nExpectedNext;
				m_nExpectedNext += static_cast<long long>( nFrames );
			}
			presented.nFrame = m_nFrame;
			return presented;
		}

		// A usable tempo updates the latch; an absent or nonsensical one
		// (<= 0) keeps the latched value.
		if ( host.fBpm > 0.0 ) {
			m_fBpm = host.fBpm;
		}

		// Host pre-roll reports negative song positions; the engine
		// contract has no negative frames.
		const long long nHostFrame = host.nFrame < 0 ? 0 : host.nFrame;

		if ( ! m_bHaveTransport ) {
			// First transport ever: adopt.
			m_bHaveTransport = true;
			m_bRolling = host.bRolling;
			m_nFrame = nHostFrame;
			m_nExpectedNext = nHostFrame + static_cast<long long>( nFrames );
		}
		else {
			// While parked the expected position is the latched one;
			// while rolling it is where the contiguous roll got us.
			const long long nExpected =
				m_bRolling ? m_nExpectedNext : m_nFrame;
			if ( host.bRolling == m_bRolling &&
				 std::llabs( nHostFrame - nExpected ) <=
					 knJitterToleranceFrames ) {
				// Contiguous transport: present the expected position,
				// absorbing the jitter. Both clocks advance by the
				// block's frame count, so absorption cannot accumulate
				// drift.
				m_nFrame = nExpected;
				m_nExpectedNext =
					nExpected + static_cast<long long>( nFrames );
			}
			else {
				// A discontinuity: a seek, a loop wrap, a stop/resume
				// transition, or drift beyond the tolerance. Adopt the
				// host reading as-is.
				m_bRolling = host.bRolling;
				m_nFrame = nHostFrame;
				m_nExpectedNext =
					nHostFrame + static_cast<long long>( nFrames );
			}
		}

		presented.bValid = true;
		presented.bRolling = m_bRolling;
		presented.fBpm = m_fBpm;
		presented.nFrame = m_nFrame;
		return presented;
	}

	/** Drop all latched state (called on activation: a new sample rate
	 *  rescales the timeline, so old frame expectations are void). */
	void reset() {
		m_bHaveTransport = false;
		m_bRolling = false;
		m_fBpm = 0.0;
		m_nFrame = 0;
		m_nExpectedNext = 0;
	}

	/** Whether any host transport was ever seen. */
	bool m_bHaveTransport = false;
	/** Latched rolling state. */
	bool m_bRolling = false;
	/** Latched tempo (0 = none seen). */
	double m_fBpm = 0.0;
	/** Frame at the start of the block presented last. */
	long long m_nFrame = 0;
	/** Frame the contiguous roll expects the next block to start at. */
	long long m_nExpectedNext = 0;
};

} // namespace H2Core

#endif // H2C_HOST_TRANSPORT_LATCH_H
