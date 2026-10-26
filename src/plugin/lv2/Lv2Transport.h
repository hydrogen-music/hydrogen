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

#ifndef H2C_PLUGIN_LV2_TRANSPORT_H
#define H2C_PLUGIN_LV2_TRANSPORT_H

// LV2 transport latch (time:Position, ADR 0013). Header-only on purpose: the
// LV2 module (HydrogenLv2.cpp) and the LV2 smoke (lv2_smoke.cpp) share this
// source, so the smoke can unit-test the parsing against forged host atom
// sequences - the engine's following of the published values is covered by
// the FakePluginHost transport tests.

#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>
#include <lv2/time/time.h>

#include <cstdint>

namespace H2Core {

/** Latch over the host's LV2 transport (ADR 0013).
 *
 * Hosts publish their transport as a time:Position object on the control
 * atom port - when it changes, or every block, depending on the host. The
 * latch keeps the last known state between updates and self-advances the
 * frame while rolling, so the engine sees a contiguous transport clock
 * either way. */
struct Lv2TransportLatch {
	// ── URIDs, mapped once per instance from the host's urid:map ──────────
	LV2_URID atomObject = 0;
	LV2_URID atomFloat = 0;
	LV2_URID atomLong = 0;
	LV2_URID timePosition = 0;
	LV2_URID timeSpeed = 0;
	LV2_URID timeFrame = 0;
	LV2_URID timeBeatsPerMinute = 0;

	// ── Latched transport state ──────────────────────────────────────────
	/** Host transport rolling (time:speed > 0). Reverse (speed < 0) cannot
	 *  be rendered and latches as stopped. */
	bool bRolling = false;
	/** Host tempo (time:beatsPerMinute). 0 = the host never provided one;
	 *  the engine then keeps its own tempo. */
	double fBpm = 0.0;
	/** Transport frame (time:frame) at the start of the current block. */
	long long nFrame = 0;

	void mapUrids( LV2_URID_Map* map ) {
		atomObject = map->map( map->handle, LV2_ATOM__Object );
		atomFloat = map->map( map->handle, LV2_ATOM__Float );
		atomLong = map->map( map->handle, LV2_ATOM__Long );
		timePosition = map->map( map->handle, LV2_TIME__Position );
		timeSpeed = map->map( map->handle, LV2_TIME__speed );
		timeFrame = map->map( map->handle, LV2_TIME__frame );
		timeBeatsPerMinute = map->map( map->handle, LV2_TIME__beatsPerMinute );
	}

	/** Scan a control-port atom sequence for time:Position objects and
	 *  latch their speed / frame / beatsPerMinute. Absent or wrong-typed
	 *  properties are skipped - the latched value survives. */
	void read( const LV2_Atom_Sequence* control ) {
		if ( control == nullptr ) {
			return;
		}
		LV2_ATOM_SEQUENCE_FOREACH( control, ev ) {
			if ( ev->body.type != atomObject ) {
				continue;
			}
			const auto* obj =
				reinterpret_cast<const LV2_Atom_Object*>( &ev->body );
			if ( obj->body.otype != timePosition ) {
				continue;
			}
			LV2_ATOM_OBJECT_FOREACH( obj, prop ) {
				if ( prop->key == timeSpeed && prop->value.type == atomFloat ) {
					const float fSpeed =
						reinterpret_cast<const LV2_Atom_Float*>(
							&prop->value )->body;
					// 1 = rolling forward, 0 = stopped, -1 = reverse (not
					// renderable - latches as stopped).
					bRolling = fSpeed > 0.0f;
				}
				else if ( prop->key == timeFrame &&
						  prop->value.type == atomLong ) {
					nFrame = reinterpret_cast<const LV2_Atom_Long*>(
						&prop->value )->body;
				}
				else if ( prop->key == timeBeatsPerMinute &&
						  prop->value.type == atomFloat ) {
					fBpm = static_cast<double>(
						reinterpret_cast<const LV2_Atom_Float*>(
							&prop->value )->body );
				}
			}
		}
	}

	/** The transport's frame at the start of the next block: while rolling
	 *  it advances with the audio; while stopped it stands. */
	void advance( uint32_t nFrames ) {
		if ( bRolling ) {
			nFrame += static_cast<long long>( nFrames );
		}
	}
};

} // namespace H2Core

#endif // H2C_PLUGIN_LV2_TRANSPORT_H
