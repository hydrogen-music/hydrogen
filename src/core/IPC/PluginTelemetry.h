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

#ifndef H2C_PLUGIN_TELEMETRY_H
#define H2C_PLUGIN_TELEMETRY_H

#include <atomic>
#include <cstdint>
#include <cstring>

namespace H2Core {

/** Bumped whenever the PluginTelemetry layout changes; validated at attach. A
 * mismatch disables telemetry (editor falls back to events-only). */
constexpr uint32_t PLUGIN_TELEMETRY_VERSION = 1;

/** Fixed per-instrument meter cap (ADR 0018). Independent of the output-bus
 * count; instruments beyond this get no meter. */
constexpr int PLUGIN_TELEMETRY_MAX_INSTRUMENTS = 256;

/** Plain, copyable snapshot of the telemetry payload (no seqlock field). The
 * audio thread fills one of these and stores it; the editor loads one. */
struct PluginTelemetrySnapshot {
	int64_t  frame = 0;            ///< playhead frame
	int32_t  bar = 0;
	int32_t  beat = 0;
	int32_t  tick = 0;
	float    bpm = 0.0f;
	uint8_t  playing = 0;
	float    masterPeakL = 0.0f;
	float    masterPeakR = 0.0f;
	float    procTimeCur = 0.0f;
	float    procTimeMax = 0.0f;
	/// Playback-track meter peaks, filled by Sampler::processPlaybackTrack()
	/// so the editor can render the playback-track waveform without accessing
	/// the engine's instrument layer directly.
	float    playbackTrackPeakL = 0.0f;
	float    playbackTrackPeakR = 0.0f;
	uint16_t instPeakCount = 0;    ///< number of valid entries in peakL/peakR
	float    peakL[ PLUGIN_TELEMETRY_MAX_INSTRUMENTS ] = { 0.0f };
	float    peakR[ PLUGIN_TELEMETRY_MAX_INSTRUMENTS ] = { 0.0f };
};

/**
 * Shared-memory telemetry block (ADR 0018). Single-writer (engine audio thread)
 * / single-reader (editor) with a **seqlock** so reads are tear-free without a
 * mutex on the audio thread: the writer bumps @a seq before and after each write
 * (odd while writing); the reader samples @a seq, copies the payload, and
 * retries if @a seq changed or is odd.
 *
 * Use the three functions below rather than touching @a seq directly.
 */
struct PluginTelemetry {
	uint32_t formatVersion;
	std::atomic<uint32_t> seq;
	PluginTelemetrySnapshot data;
};

/** Prepare a freshly-mapped block: stamp the version and clear the seqlock. */
inline void telemetryInit( PluginTelemetry& block ) {
	block.formatVersion = PLUGIN_TELEMETRY_VERSION;
	block.seq.store( 0, std::memory_order_relaxed );
	block.data = PluginTelemetrySnapshot();
}

/** Publish a snapshot (engine side). Wait-free; safe on the audio thread. */
inline void telemetryStore( PluginTelemetry& block,
							const PluginTelemetrySnapshot& snapshot ) {
	const uint32_t s = block.seq.load( std::memory_order_relaxed );
	block.seq.store( s + 1, std::memory_order_release );   // begin (odd)
	std::atomic_thread_fence( std::memory_order_release );
	block.data = snapshot;
	std::atomic_thread_fence( std::memory_order_release );
	block.seq.store( s + 2, std::memory_order_release );   // end (even)
}

/**
 * Read a tear-free snapshot (editor side). Returns false if the block's layout
 * version does not match (telemetry must then be disabled). On success @a out
 * holds a consistent copy from a single writer generation.
 */
inline bool telemetryLoad( const PluginTelemetry& block,
						   PluginTelemetrySnapshot& out ) {
	if ( block.formatVersion != PLUGIN_TELEMETRY_VERSION ) {
		return false;
	}
	for ( ;; ) {
		const uint32_t s1 = block.seq.load( std::memory_order_acquire );
		if ( ( s1 & 1u ) != 0u ) {
			continue; // a write is in progress
		}
		out = block.data;
		std::atomic_thread_fence( std::memory_order_acquire );
		const uint32_t s2 = block.seq.load( std::memory_order_acquire );
		if ( s1 == s2 ) {
			return true; // no write straddled the copy
		}
	}
}

};

#endif
