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
#include <core/Helpers/Filesystem.h>
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

	// Without a preferences argument the archive carries no preferences
	// entry: the out-param stays empty and the caller keeps its own.
	QByteArray prefsXmlAbsent;
	auto pBackNoPrefs = H2Project::fromState(
		state, pTestHydrogen(), true, &prefsXmlAbsent );
	CPPUNIT_ASSERT( pBackNoPrefs != nullptr );
	CPPUNIT_ASSERT( prefsXmlAbsent.isEmpty() );

	// With one, the entry round-trips byte-identically: fromState hands
	// back exactly the blob toState embedded (ADR 0022/0025).
	auto pPreferences = std::make_shared<Preferences>();
	pPreferences->setFieldOwnership( Preferences::FieldOwnership::BaseLayer );
	pPreferences->setLastOpenSongDirectory( Filesystem::userPatternsDir() );
	const auto withPrefs = H2Project::toState(
		pSong, /*bEmbedSamples=*/true, true, pPreferences );
	CPPUNIT_ASSERT( ! withPrefs.empty() );

	QByteArray prefsXml;
	auto pBackPrefs = H2Project::fromState(
		withPrefs, pTestHydrogen(), true, &prefsXml );
	CPPUNIT_ASSERT( pBackPrefs != nullptr );
	CPPUNIT_ASSERT( ! prefsXml.isEmpty() );
	CPPUNIT_ASSERT( prefsXml == pPreferences->toXmlBuffer() );

	___INFOLOG( "passed" );
}

// Embed OFF → the plugin state is still the `.h2project` archive container,
// just without the `samples/*` entries: much smaller than the embedded
// bundle, and it still reconstructs the song (ADR 0025).
void PluginStateTest::testEmbedOff() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const auto sampleFree =
		H2Project::toState( pSong, /*bEmbedSamples=*/false, true );
	CPPUNIT_ASSERT( ! sampleFree.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( sampleFree ) );

	QByteArray prefsXml;
	auto pBack = H2Project::fromState(
		sampleFree, pTestHydrogen(), true, &prefsXml );
	CPPUNIT_ASSERT( pBack != nullptr );
	// Created without a preferences argument: no entry, empty out-param.
	CPPUNIT_ASSERT( prefsXml.isEmpty() );
	CPPUNIT_ASSERT_EQUAL(
		pSong->getDrumkit()->getInstruments()->size(),
		pBack->getDrumkit()->getInstruments()->size() );

	// The embed-off state carries no sample audio, so it is far smaller
	// than the embedded bundle.
	const auto embedded = H2Project::toState( pSong, /*bEmbedSamples=*/true, true );
	CPPUNIT_ASSERT( ! embedded.empty() );
	CPPUNIT_ASSERT( sampleFree.size() < embedded.size() );

	___INFOLOG( "passed" );
}
