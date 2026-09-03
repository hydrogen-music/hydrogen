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

#include "SampleTest.h"

#include "TestHelper.h"

#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <cppunit/TestAssert.h>

#include <QtCore/QFile>

#include <vector>

using namespace H2Core;

void SampleTest::testLoadInvalidSample()
{
	___INFOLOG( "" );
	std::shared_ptr<Sample> pSample;

	// TC1: Sample does not exist
	QString SamplePath( "PathDoesNotExist" );
	pSample = Sample::load( SamplePath );

	CPPUNIT_ASSERT( pSample == nullptr );

	// TC2: Sample does exist, but is not a valid sample
	pSample = Sample::load( H2TEST_FILE( "drumkits/baseKit/drumkit.xml" ) );
	CPPUNIT_ASSERT( pSample == nullptr );
	___INFOLOG( "passed" );
}

void SampleTest::testStoringSamplesInCurrentDrumkit()
{
	___INFOLOG( "" );

	auto pSong = Song::getEmptySong( pTestHydrogen() );
	CPPUNIT_ASSERT( pSong != nullptr );

	auto pDrumkit = std::make_shared<Drumkit>(pSong->getDrumkit());
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getInstruments()->size() > 0 );
	CPPUNIT_ASSERT( pDrumkit->getName() == "GMRockKit" );
    // Reassign copy to ensure it is properly saved to disk later on.
    pSong->setDrumkit( pDrumkit );

	// Import an instrument from another drumkit (installed)
	const QString sAnotherDrumkitPath(
		Filesystem::systemDrumkitsDir() + "TR808EmulationKit/drumkit.xml"
	);
	auto pAnotherDrumkit =
		std::make_shared<Drumkit>(pTestHydrogen()->getSoundLibraryDatabase()->getDrumkit(
			sAnotherDrumkitPath, false
		));
	CPPUNIT_ASSERT( pAnotherDrumkit != nullptr );
	CPPUNIT_ASSERT( pAnotherDrumkit->getInstruments()->size() > 0 );

	auto pAnotherInstrument = pAnotherDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pAnotherInstrument != nullptr );
	CPPUNIT_ASSERT( pAnotherInstrument->getComponent( 0 ) != nullptr );
	CPPUNIT_ASSERT(
		pAnotherInstrument->getComponent( 0 )->getLayer( 0 ) != nullptr
	);

	pDrumkit->addInstrument( pAnotherInstrument, -1 );
	CPPUNIT_ASSERT( !pDrumkit->hasMissingSamples() );

	// Emulate a downloaded kit
	const QString sDownloadedDrumkitPath(
		Filesystem::userDrumkitsDir() + "url/downloadedKit/drumkit.xml"
	);
	const QString sDownloadedDrumkitName( "downloadedKit" );
	const QString sDownloadedDrumkitDir(
		Filesystem::drumkitDirFromPath( sDownloadedDrumkitPath ) );
	auto pDownloadedDrumkit = std::make_shared<Drumkit>( pAnotherDrumkit );
	CPPUNIT_ASSERT( pDownloadedDrumkit != nullptr );
	CPPUNIT_ASSERT( pDownloadedDrumkit->getInstruments()->size() > 0 );
	pDownloadedDrumkit->setName( sDownloadedDrumkitName );
	pDownloadedDrumkit->setPath( sDownloadedDrumkitPath );
	CPPUNIT_ASSERT( pDownloadedDrumkit->save( sDownloadedDrumkitPath ) );

	auto pDownloadedInstrument = pDownloadedDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pDownloadedInstrument != nullptr );
	CPPUNIT_ASSERT( pDownloadedInstrument->getComponent( 0 ) != nullptr );
	CPPUNIT_ASSERT(
		pDownloadedInstrument->getComponent( 0 )->getLayer( 0 ) != nullptr
	);
	pDownloadedInstrument->setName( "downloadedInstrument" );

	pDrumkit->addInstrument( pDownloadedInstrument, -1 );
	CPPUNIT_ASSERT( !pDrumkit->hasMissingSamples() );

	// Import an arbitrary sample.
	{
		auto pInstrument = std::make_shared<Instrument>();
		pInstrument->setName( "free sample" );
		auto pComponent = pInstrument->getComponent( 0 );
		auto pSample = std::make_shared<Sample>(
			H2TEST_FILE( "/functional/mutegroups.ref.flac" )
		);
		pInstrument->addLayer(
			pComponent, std::make_shared<InstrumentLayer>( pSample ), -1,
			Event::Trigger::Suppress, nullptr );
		pDrumkit->addInstrument( pInstrument );
		CPPUNIT_ASSERT( !pDrumkit->hasMissingSamples() );
	}

	// Import a sample from another registered drumkit (which is not associated
	// with the drumkit via #Instrument::m_sDrumkitPath).
	{
		auto pInstrument = std::make_shared<Instrument>();
		pInstrument->setName( "registered sample" );
		auto pComponent = pInstrument->getComponent( 0 );
		pInstrument->addLayer(
			pComponent, pAnotherInstrument->getComponent( 0 )->getLayer( 0 ), -1,
			Event::Trigger::Suppress, nullptr );
		pDrumkit->addInstrument( pInstrument );
		CPPUNIT_ASSERT( !pDrumkit->hasMissingSamples() );
	}

	// Import a sample from another unregistered drumkit (within a drumkit
	// folder but not within a folder known to the SoundLibraryDatabase).
	{
		auto pInstrument = std::make_shared<Instrument>();
		pInstrument->setName( "unregistered sample" );
		auto pComponent = pInstrument->getComponent( 0 );
		auto pSample =
			std::make_shared<Sample>( H2TEST_FILE( "/drumkits/baseKit/crash.wav" ) );
		pInstrument->addLayer(
			pComponent, std::make_shared<InstrumentLayer>( pSample ), -1,
			Event::Trigger::Suppress, nullptr );
		pDrumkit->addInstrument( pInstrument );
		CPPUNIT_ASSERT( !pDrumkit->hasMissingSamples() );
	}

	const int nSampleNumber = pDrumkit->summarizeContent().size();

	// Save a copy of the drumkit to disk and reload it. If everything worked,
	// all samples can be reloaded. Be sure to do this using a _copy_.
	QTemporaryDir tmpDir( "storing-sample-test-kit" );
	const QString sTmpDrumkitPath =
		Filesystem::drumkitPathFromDir( tmpDir.path() );
	auto pDrumkitCopy = std::make_shared<Drumkit>(pDrumkit);
	pDrumkitCopy->save( sTmpDrumkitPath, false );

	auto pDrumkitReloaded =
		Drumkit::load( sTmpDrumkitPath, false, nullptr, false , pTestHydrogen() );
	CPPUNIT_ASSERT( pDrumkitReloaded != nullptr );
	CPPUNIT_ASSERT( !pDrumkitReloaded->hasMissingSamples() );
	CPPUNIT_ASSERT(
		pDrumkitReloaded->summarizeContent().size() == nSampleNumber
	);

	// Now let's do the same for the song itself.
	const QString sTmpSongPath =
		Filesystem::tmpFilePath( "storing-samples-test-song" );
	CPPUNIT_ASSERT( pSong->save( sTmpSongPath, false, false ) );
	auto pSongReloaded = Song::load( sTmpSongPath, false, pTestHydrogen() );
	CPPUNIT_ASSERT( pSongReloaded != nullptr );
	auto pSongKitReloaded = pSongReloaded->getDrumkit();
	CPPUNIT_ASSERT( pSongKitReloaded != nullptr );
	CPPUNIT_ASSERT( !pSongKitReloaded->hasMissingSamples() );
	CPPUNIT_ASSERT(
		pSongKitReloaded->summarizeContent().size() == nSampleNumber
	);
	QStringList drumkitPaths;
	for ( const auto& ppInstrument : *pSongKitReloaded->getInstruments() ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		if ( ! drumkitPaths.contains( ppInstrument->getDrumkitPath() ) ) {
			drumkitPaths << ppInstrument->getDrumkitPath();
		}
	}
	// 4? Because there is also the empty path for the "unregistered sample"
	CPPUNIT_ASSERT( drumkitPaths.size() == 4 );

	Filesystem::rm( sTmpSongPath, false, true );
	Filesystem::rm(
		Filesystem::drumkitDirFromPath( sDownloadedDrumkitPath ), true, true
	);

	___INFOLOG( "passed" );
}
