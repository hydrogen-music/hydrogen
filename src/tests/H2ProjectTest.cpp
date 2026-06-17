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

#include "H2ProjectTest.h"

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

#include <map>

using namespace H2Core;

namespace {

// Collect, in stable (inst/comp/layer) order, the loaded frame count of every
// sample of a song's kit. Empty/missing samples report 0.
std::map<QString, long long> sampleFrames( std::shared_ptr<Song> pSong ) {
	std::map<QString, long long> result;
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	for ( int ii = 0; ii < pInstruments->size(); ++ii ) {
		auto pInstrument = pInstruments->get( ii );
		if ( pInstrument == nullptr ) {
			continue;
		}
		auto pComponents = pInstrument->getComponents();
		for ( int cc = 0; cc < static_cast<int>( pComponents->size() ); ++cc ) {
			auto pComponent = ( *pComponents )[ cc ];
			if ( pComponent == nullptr ) {
				continue;
			}
			const auto layers = pComponent->getLayers();
			for ( int ll = 0; ll < static_cast<int>( layers.size() ); ++ll ) {
				auto pLayer = layers[ ll ];
				if ( pLayer == nullptr || pLayer->getSample() == nullptr ) {
					continue;
				}
				result[ QString( "%1/%2/%3" ).arg( ii ).arg( cc ).arg( ll ) ] =
					pLayer->getSample()->isLoaded()
						? pLayer->getSample()->getFrames()
						: 0;
			}
		}
	}
	return result;
}

std::shared_ptr<Song> makeLoadedSong() {
	auto* pHydrogen = pTestHydrogen();
	auto pSong = Song::getEmptySong( pHydrogen );
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		pSong->getDrumkit()->loadSamples(
			120, pHydrogen->getPreferences().get() );
	}
	return pSong;
}

} // namespace

// song + kit -> .h2project bundle (in memory) -> reconstructs identically,
// including the per-instrument order that drives the bus mapping (ADR 0019/0025).
void H2ProjectTest::testBufferRoundTrip() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pSong->getDrumkit() != nullptr );
	const int nInstruments = pSong->getDrumkit()->getInstruments()->size();
	CPPUNIT_ASSERT( nInstruments > 0 );

	const auto framesBefore = sampleFrames( pSong );
	CPPUNIT_ASSERT( ! framesBefore.empty() );

	const auto bundle = H2Project::toBuffer( pSong, true );
	CPPUNIT_ASSERT( ! bundle.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( bundle ) );

	auto pReconstructed = H2Project::fromBuffer( bundle, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pReconstructed != nullptr );
	CPPUNIT_ASSERT( pReconstructed->getDrumkit() != nullptr );

	// Same instrument count and order (the 1-to-1 bus mapping rides on this).
	auto pInstr1 = pSong->getDrumkit()->getInstruments();
	auto pInstr2 = pReconstructed->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT_EQUAL( pInstr1->size(), pInstr2->size() );
	for ( int ii = 0; ii < pInstr1->size(); ++ii ) {
		CPPUNIT_ASSERT_EQUAL( pInstr1->get( ii )->getName().toStdString(),
							  pInstr2->get( ii )->getName().toStdString() );
	}

	// Every sample decoded from the bundle and matches the original frame count.
	const auto framesAfter = sampleFrames( pReconstructed );
	CPPUNIT_ASSERT_EQUAL( framesBefore.size(), framesAfter.size() );
	for ( const auto& [ sKey, nFrames ] : framesBefore ) {
		CPPUNIT_ASSERT( framesAfter.find( sKey ) != framesAfter.end() );
		CPPUNIT_ASSERT( framesAfter.at( sKey ) > 0 ); // decoded from memory
		CPPUNIT_ASSERT_EQUAL( nFrames, framesAfter.at( sKey ) );
	}

	___INFOLOG( "passed" );
}

// The same round-trip via a file on disk.
void H2ProjectTest::testFileRoundTrip() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	const auto framesBefore = sampleFrames( pSong );

	const QString sPath = Filesystem::tmpFilePath( "roundtrip.h2project" );
	CPPUNIT_ASSERT( H2Project::save( pSong, sPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileExists( sPath, true ) );

	auto pReconstructed = H2Project::load( sPath, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pReconstructed != nullptr );
	CPPUNIT_ASSERT_EQUAL( sPath.toStdString(),
						  pReconstructed->getPath().toStdString() );

	const auto framesAfter = sampleFrames( pReconstructed );
	CPPUNIT_ASSERT_EQUAL( framesBefore.size(), framesAfter.size() );
	for ( const auto& [ sKey, nFrames ] : framesBefore ) {
		CPPUNIT_ASSERT( framesAfter.find( sKey ) != framesAfter.end() );
		CPPUNIT_ASSERT_EQUAL( nFrames, framesAfter.at( sKey ) );
	}

	Filesystem::rm( sPath, false, true );

	___INFOLOG( "passed" );
}

// The container detector tells `.h2project` archives from `.h2song` XML so the
// unified open path can route correctly (T4b.3).
void H2ProjectTest::testContainerDetection() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	const auto bundle = H2Project::toBuffer( pSong, true );
	CPPUNIT_ASSERT( ! bundle.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( bundle ) );

	// An .h2song XML document must NOT look like an archive.
	const QByteArray songXml = pSong->toXmlBuffer( true, true );
	std::vector<unsigned char> xml(
		reinterpret_cast<const unsigned char*>( songXml.constData() ),
		reinterpret_cast<const unsigned char*>( songXml.constData() ) +
			songXml.size() );
	CPPUNIT_ASSERT( ! H2Project::looksLikeArchive( xml ) );

	___INFOLOG( "passed" );
}

// One open endpoint loads both a `.h2song` and a `.h2project` (T4b.3).
void H2ProjectTest::testUnifiedOpen() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	const int nInstruments = pSong->getDrumkit()->getInstruments()->size();

	const QString sSongPath = Filesystem::tmpFilePath( "unified.h2song" );
	const QString sProjPath = Filesystem::tmpFilePath( "unified.h2project" );
	CPPUNIT_ASSERT( pSong->save( sSongPath, true, true ) );
	CPPUNIT_ASSERT( H2Project::save( pSong, sProjPath, true ) );

	// The same endpoint loads either container.
	auto pFromSong = H2Project::openSong( sSongPath, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pFromSong != nullptr );
	CPPUNIT_ASSERT_EQUAL( nInstruments,
						  pFromSong->getDrumkit()->getInstruments()->size() );

	auto pFromProject = H2Project::openSong( sProjPath, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pFromProject != nullptr );
	CPPUNIT_ASSERT_EQUAL( nInstruments,
						  pFromProject->getDrumkit()->getInstruments()->size() );

	Filesystem::rm( sSongPath, false, true );
	Filesystem::rm( sProjPath, false, true );

	___INFOLOG( "passed" );
}
