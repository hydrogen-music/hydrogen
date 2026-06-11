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

#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

void MultiInstanceTest::testInstanceOwnsContext() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	CPPUNIT_ASSERT( pHydrogen != nullptr );

	// T1.3: a Hydrogen instance owns its Preferences and EventQueue and exposes
	// them through the new per-instance API (ADR 0015).
	CPPUNIT_ASSERT( pHydrogen->getPreferences() != nullptr );
	CPPUNIT_ASSERT( pHydrogen->getEventQueue() != nullptr );
	CPPUNIT_ASSERT( pHydrogen->getAudioEngine() != nullptr );

	// The transitional process-current shims resolve to the instance-owned
	// objects, so the ~2,000 unconverted get_instance() call sites see this
	// instance's context until the de-singletoning sweep (T1.4/T1.5) converts
	// them.
	CPPUNIT_ASSERT( pHydrogen->getPreferences() == Preferences::get_instance() );
	CPPUNIT_ASSERT( pHydrogen->getEventQueue() == EventQueue::get_instance() );

	// NOTE: full two-live-instance isolation (independent song/tempo/transport
	// and EventQueue, mutating one never affecting the other) is the Phase 1
	// done-gate. It is added to this suite once later T1.x work lands the
	// supporting lifecycle changes — ~Hydrogen currently resets the global
	// process-current pointer to null rather than to a previous instance, and
	// the audio/OSC layers are still process-global.

	___INFOLOG( "passed" );
}
