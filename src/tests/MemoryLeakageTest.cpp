/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "MemoryLeakageTest.h"

#include <memory>

#include <core/Basics/Adsr.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/License.h>

#include <core/Sampler/Sampler.h>
#include <core/Sampler/Sampler.cpp>
#include <core/Helpers/Xml.h>
#include <core/Helpers/Legacy.h>

#include "TestHelper.h"


void MemoryLeakageTest::testConstructors() {
	___INFOLOG( "" );
	auto mapSnapshot = H2Core::Base::getObjectMap();
	int nAliveReference = H2Core::Base::getAliveObjectCount();

	{
		auto pADSR = std::make_shared<H2Core::ADSR>();
		auto pADSR2 = std::make_shared<H2Core::ADSR>( pADSR );
		pADSR = nullptr;
		pADSR2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto AutomationPath = new H2Core::AutomationPath( 0, 1, 0.2 );
		delete AutomationPath;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pDrumkit = std::make_shared<H2Core::Drumkit>();
		auto pDrumkit2 = std::make_shared<H2Core::Drumkit>( pDrumkit );
		pDrumkit = nullptr;
		pDrumkit2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pADSR = std::make_shared<H2Core::ADSR>();
		auto pInstrument = std::make_shared<H2Core::Instrument>( 0, "ladida", pADSR );
		auto pInstrument2 = std::make_shared<H2Core::Instrument>( pInstrument );
		pInstrument = nullptr;
		pInstrument2 = nullptr;
		pADSR = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSample = std::make_shared<H2Core::Sample>( H2TEST_FILE( "/drumkits/baseKit/kick.wav" ) );
		auto pInstrumentLayer = std::make_shared<H2Core::InstrumentLayer>( pSample );
		auto pInstrumentLayer2 = std::make_shared<H2Core::InstrumentLayer>( pInstrumentLayer );
		pInstrumentLayer = nullptr;
		pSample = nullptr;
		pInstrumentLayer2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentList = std::make_shared<H2Core::InstrumentList>();
		auto pInstrumentList2 = std::make_shared<H2Core::InstrumentList>( pInstrumentList );
		pInstrumentList = nullptr;
		pInstrumentList2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentComponent = std::make_shared<H2Core::InstrumentComponent>();
		auto pInstrumentComponent2 = std::make_shared<H2Core::InstrumentComponent>( pInstrumentComponent );
		pInstrumentComponent = nullptr;
		pInstrumentComponent2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pADSR = std::make_shared<H2Core::ADSR>();
		auto pInstrument = std::make_shared<H2Core::Instrument>( 0, "ladida", pADSR );
		auto pNote = std::make_shared<H2Core::Note>( pInstrument, 0, 0.f, 0.f, 1, 1.f );
		auto pNote2 = std::make_shared<H2Core::Note>( pNote );
		pNote = nullptr;
		pNote2 = nullptr;
		pInstrument = nullptr;
		pADSR = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pPattern = std::make_shared<H2Core::Pattern>( "ladida", "ladida", "ladida" );
		auto pPattern2 = std::make_shared<H2Core::Pattern>( pPattern );
		pPattern = nullptr;
		pPattern2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pPatternList = std::make_shared<H2Core::PatternList>();
		auto pPatternList2 = std::make_shared<H2Core::PatternList>( pPatternList );
		pPatternList = nullptr;
		pPatternList2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSample = std::make_shared<H2Core::Sample>( H2TEST_FILE( "/drumkits/baseKit/kick.wav" ) );
		auto pSample2 = std::make_shared<H2Core::Sample>( pSample );
		pSample = nullptr;
		pSample2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSong = std::make_shared<H2Core::Song>( "ladida", "ladida", 120, 1 );
		pSong = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}
	
	{
		auto pSampler = new H2Core::Sampler();
		delete pSampler;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	// Test copy constructors using real-live instead of new objects.
	auto pDrumkitProper = H2Core::Drumkit::load(
		H2Core::Filesystem::drumkit_path_search( "GMRockKit",
												 H2Core::Filesystem::Lookup::system,
												 true ), false, true );
	CPPUNIT_ASSERT( pDrumkitProper != nullptr );
	
	pDrumkitProper->loadSamples();
	
	CPPUNIT_ASSERT( pDrumkitProper->getInstruments()->get( 0 )->getComponent( 0 )->getLayer( 0 )->getSample() != nullptr );
	auto pSongProper = H2Core::Song::load( H2Core::Filesystem::demos_dir() + "GM_kit_Diddley.h2song" );
	CPPUNIT_ASSERT( pSongProper != nullptr );

	int nNewCount = H2Core::Base::getAliveObjectCount();

	{
		auto pDrumkit = std::make_shared<H2Core::Drumkit>( pDrumkitProper );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		pDrumkit = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentList = std::make_shared<H2Core::InstrumentList>( pSongProper->getDrumkit()->getInstruments() );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pPatternList = std::make_shared<H2Core::PatternList>(
			pSongProper->getPatternList() );
		CPPUNIT_ASSERT( pPatternList != nullptr );
		pPatternList = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}
	
	{
		auto pPattern = std::make_shared<H2Core::Pattern>( pSongProper->getPatternList()->get( 0 ) );
		CPPUNIT_ASSERT( pPattern != nullptr );
		pPattern = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto notes = pSongProper->getPatternList()->get( 0 )->getNotes();
		auto pNote = std::make_shared<H2Core::Note>( notes->begin()->second );
		CPPUNIT_ASSERT( pNote != nullptr );
		pNote = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrument = std::make_shared<H2Core::Instrument>( pDrumkitProper->getInstruments()->get( 0 ) );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		pInstrument = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pADSR = std::make_shared<H2Core::ADSR>( pDrumkitProper->getInstruments()->get( 0 )->getAdsr() );
		CPPUNIT_ASSERT( pADSR != nullptr );
		pADSR = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentComponent = std::make_shared<H2Core::InstrumentComponent>( pDrumkitProper->getInstruments()->get( 0 )->getComponent( 0 ) );
		CPPUNIT_ASSERT( pInstrumentComponent != nullptr );
		pInstrumentComponent = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentLayer = std::make_shared<H2Core::InstrumentLayer>( pDrumkitProper->getInstruments()->get( 0 )->getComponent( 0 )->getLayer( 0 ) );
		CPPUNIT_ASSERT( pInstrumentLayer != nullptr );
		pInstrumentLayer = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSample = std::make_shared<H2Core::Sample>( pDrumkitProper->getInstruments()->get( 0 )->getComponent( 0 )->getLayer( 0 )->getSample() );
		CPPUNIT_ASSERT( pSample != nullptr );
		pSample = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}
	
	pDrumkitProper = nullptr;
	pSongProper = nullptr;
	CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	___INFOLOG( "passed" );
}

void MemoryLeakageTest::testLoading() {
	___INFOLOG( "" );
	H2Core::XMLDoc doc;
	H2Core::XMLNode node;

	auto mapSnapshot = H2Core::Base::getObjectMap();
	int nAliveReference = H2Core::Base::getAliveObjectCount();
	
	auto pHydrogen = H2Core::Hydrogen::get_instance();

	QString sDrumkitPath =
		H2Core::Filesystem::drumkit_path_search( "GMRockKit",
												 H2Core::Filesystem::Lookup::system );

	{
		auto pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/baseKit/" ),
											   false, true );
		CPPUNIT_ASSERT( pDrumkit != nullptr );

		pDrumkit->loadSamples();
		
		pDrumkit = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentComponent.xml" ) ) );
		node = doc.firstChildElement( "instrumentComponent" );
		auto pInstrumentComponent = H2Core::InstrumentComponent::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ) );
		CPPUNIT_ASSERT( pInstrumentComponent != nullptr );
		pInstrumentComponent = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrument.xml" ) ) );
		node = doc.firstChildElement( "instrument" );
		auto pInstrument = H2Core::Instrument::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ) );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		pInstrument = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentLayer.xml" ) ) );
		node = doc.firstChildElement( "instrumentLayer" );
		auto pInstrumentLayer = H2Core::InstrumentLayer::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ) );
		CPPUNIT_ASSERT( pInstrumentLayer != nullptr );
		pInstrumentLayer = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/note.xml" ) ) );
		node = doc.firstChildElement( "note" );
		auto pNote = H2Core::Note::loadFrom( node );
		CPPUNIT_ASSERT( pNote != nullptr );
		pNote = nullptr;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentListV2.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/note.xml" ) ) );
		node = doc.firstChildElement( "note" );
		auto pNote = H2Core::Note::loadFrom( node );
		CPPUNIT_ASSERT( pNote != nullptr );
		pNote = nullptr;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		auto pPattern = H2Core::Pattern::load(
			H2TEST_FILE( "pattern/pattern.h2pattern" ) );
		CPPUNIT_ASSERT( pPattern != nullptr );
		pPattern = nullptr;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pPlaylist = H2Core::Playlist::load( H2TEST_FILE( "playlist/test.h2playlist" ) );
		CPPUNIT_ASSERT( pPlaylist != nullptr );
		pPlaylist = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSample = H2Core::Sample::load( H2TEST_FILE( "drumkits/baseKit/snare.wav" ) );
		CPPUNIT_ASSERT( pSample != nullptr );
		pSample = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSong = H2Core::Song::getEmptySong();
		CPPUNIT_ASSERT( pSong != nullptr );
		pSong = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSong = H2Core::Song::load( H2TEST_FILE( "functional/test.h2song" ) );
		CPPUNIT_ASSERT( pSong != nullptr );
		pSong = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSampler = new H2Core::Sampler();
		pSampler->reinitializePlaybackTrack();
		delete pSampler;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		pHydrogen->getSong()->setPlaybackTrackFilename( H2TEST_FILE( "drumkits/baseKit/kick.wav" ) );
		auto pSampler = new H2Core::Sampler();
		pSampler->reinitializePlaybackTrack();
		delete pSampler;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrument = H2Core::createInstrument( 0, H2TEST_FILE( "drumkits/baseKit/kick.wav" ), 0.7 );
		pInstrument = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::loadFrom( node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		auto pPattern = H2Core::Legacy::loadPattern(
			H2TEST_FILE( "pattern/legacy/legacy_pattern.h2pattern" ) );
		CPPUNIT_ASSERT( pPattern != nullptr );
		pPattern = nullptr;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/baseKit" ),
											   false, true );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		pDrumkit->loadSamples();
		auto pDrumkit2 = H2Core::Drumkit::load(
			H2Core::Filesystem::drumkit_path_search( "GMRockKit",
													 H2Core::Filesystem::Lookup::system,
													 true ), false, true );
		CPPUNIT_ASSERT( pDrumkit2 != nullptr );
		pDrumkit2->loadSamples();
	
		H2Core::CoreActionController::setDrumkit( pDrumkit );
		int nLoaded = H2Core::Base::getAliveObjectCount();
		H2Core::CoreActionController::setDrumkit( pDrumkit );
		CPPUNIT_ASSERT( nLoaded == H2Core::Base::getAliveObjectCount() );
		H2Core::CoreActionController::setDrumkit( pDrumkit2 );
		H2Core::CoreActionController::setDrumkit( pDrumkit );
		CPPUNIT_ASSERT( nLoaded == H2Core::Base::getAliveObjectCount() );
	}
	___INFOLOG( "passed" );
}

void MemoryLeakageTest::tearDown() {
	if ( H2Core::Filesystem::drumkit_exists( "testKitLadida" ) ) {
		QString sPath = H2Core::Filesystem::drumkit_usr_path( "testKitLadida" );
		CPPUNIT_ASSERT( H2Core::Filesystem::rm( sPath, true ) );
	}
}
