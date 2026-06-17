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

#include "PluginStateTest.h"

#include "TestHelper.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Helpers/H2Project.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

namespace {

std::shared_ptr<Song> makeLoadedSong() {
	auto* pHydrogen = pTestHydrogen();
	auto pSong = Song::getEmptySong( pHydrogen );
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		pSong->getDrumkit()->loadSamples(
			120, pHydrogen->getPreferences().get() );
	}
	return pSong;
}

// Does the song's kit have at least one sample loaded into memory (non-zero
// frames)?
bool hasLoadedSample( std::shared_ptr<Song> pSong ) {
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	for ( int ii = 0; ii < pInstruments->size(); ++ii ) {
		auto pInstrument = pInstruments->get( ii );
		if ( pInstrument == nullptr ) {
			continue;
		}
		for ( auto pComponent : *pInstrument->getComponents() ) {
			if ( pComponent == nullptr ) {
				continue;
			}
			for ( auto pLayer : pComponent->getLayers() ) {
				if ( pLayer != nullptr && pLayer->getSample() != nullptr &&
					 pLayer->getSample()->isLoaded() &&
					 pLayer->getSample()->getFrames() > 0 ) {
					return true;
				}
			}
		}
	}
	return false;
}

} // namespace

// Embed ON → the plugin state is a portable `.h2project` bundle that reloads
// with its sample audio decoded from the state itself (ADR 0017/0020/T4b.5).
void PluginStateTest::testEmbedOn() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const auto state = H2Project::toState( pSong, /*bEmbedSamples=*/true, true );
	CPPUNIT_ASSERT( ! state.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( state ) );

	auto pBack = H2Project::fromState( state, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pBack != nullptr );
	CPPUNIT_ASSERT_EQUAL(
		pSong->getDrumkit()->getInstruments()->size(),
		pBack->getDrumkit()->getInstruments()->size() );
	// Samples came back decoded straight from the embedded bundle.
	CPPUNIT_ASSERT( hasLoadedSample( pBack ) );

	___INFOLOG( "passed" );
}

// Embed OFF → the plugin state is song-only (the `.h2song` XML), much smaller
// than the embedded bundle and not an archive; it still reconstructs the song.
void PluginStateTest::testEmbedOff() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const auto songOnly = H2Project::toState( pSong, /*bEmbedSamples=*/false, true );
	CPPUNIT_ASSERT( ! songOnly.empty() );
	CPPUNIT_ASSERT( ! H2Project::looksLikeArchive( songOnly ) );

	auto pBack = H2Project::fromState( songOnly, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pBack != nullptr );
	CPPUNIT_ASSERT_EQUAL(
		pSong->getDrumkit()->getInstruments()->size(),
		pBack->getDrumkit()->getInstruments()->size() );

	// Song-only state carries no sample audio, so it is far smaller than the
	// embedded bundle.
	const auto embedded = H2Project::toState( pSong, /*bEmbedSamples=*/true, true );
	CPPUNIT_ASSERT( ! embedded.empty() );
	CPPUNIT_ASSERT( songOnly.size() < embedded.size() );

	___INFOLOG( "passed" );
}
