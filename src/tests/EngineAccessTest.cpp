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

#include "EngineAccessTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/LocalEngineAccess.h>

#include "TestHelper.h"

using namespace H2Core;

void EngineAccessTest::testLocalEngineAccess() {
	___INFOLOG( "" );

	auto* pHydrogen = pTestHydrogen();
	LocalEngineAccess access( pHydrogen );
	IEngineAccess& engine = access; // the GUI sees only this interface

	// The access surface resolves to the wrapped instance's own services — the
	// GUI fans out from these the same way it did from the engine (ADR 0016).
	CPPUNIT_ASSERT( engine.getCoreActionController() ==
					pHydrogen->getCoreActionController() );
	CPPUNIT_ASSERT( engine.getEventQueue() == pHydrogen->getEventQueue() );
	CPPUNIT_ASSERT( engine.getAudioEngine() == pHydrogen->getAudioEngine() );
	CPPUNIT_ASSERT( engine.getPreferences() == pHydrogen->getPreferences() );
	CPPUNIT_ASSERT( engine.getSong() == pHydrogen->getSong() );
	CPPUNIT_ASSERT( engine.getPlaylist() == pHydrogen->getPlaylist() );
	CPPUNIT_ASSERT( engine.getSoundLibraryDatabase() ==
					pHydrogen->getSoundLibraryDatabase() );

	// State reads track the live engine.
	CPPUNIT_ASSERT( engine.getSong() != nullptr );
	CPPUNIT_ASSERT_EQUAL( pHydrogen->getMode(), engine.getMode() );

	// A command issued through the interface reaches the engine.
	engine.setSongModified( true );
	CPPUNIT_ASSERT( pHydrogen->getSong()->getIsModified() );
	engine.setSongModified( false );
	CPPUNIT_ASSERT( ! pHydrogen->getSong()->getIsModified() );

	// Events flow through the same queue the engine emits on.
	auto* pQueue = engine.getEventQueue();
	while ( pQueue->popEvent() != nullptr ) {}
	pQueue->pushEvent( Event::Type::Metronome, 7 );
	auto pEvent = engine.getEventQueue()->popEvent();
	CPPUNIT_ASSERT( pEvent != nullptr &&
					pEvent->getType() == Event::Type::Metronome &&
					pEvent->getValue() == 7 );

	___INFOLOG( "passed" );
}
