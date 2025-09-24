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
#include <core/Basics/DrumkitComponent.h>
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
		auto ADSR = std::make_shared<H2Core::ADSR>();
		auto ADSR2 = new H2Core::ADSR( ADSR );
		ADSR = nullptr;
		delete ADSR2;
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
		auto pDrumkitComponent = std::make_shared<H2Core::DrumkitComponent>( 0, "ladida" );
		auto pDrumkitComponent2 = std::make_shared<H2Core::DrumkitComponent>( pDrumkitComponent );
		pDrumkitComponent = nullptr;
		pDrumkitComponent2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pADSR = std::make_shared<H2Core::ADSR>();
		auto Instrument = std::make_shared<H2Core::Instrument>( 0, "ladida", pADSR );
		auto Instrument2 = new H2Core::Instrument( Instrument );
		delete Instrument2;
		Instrument = nullptr;
		pADSR = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSample = std::make_shared<H2Core::Sample>( H2TEST_FILE( "/drumkits/baseKit/kick.wav" ) );
		auto InstrumentLayer = std::make_shared<H2Core::InstrumentLayer>( pSample );
		auto InstrumentLayer2 = new H2Core::InstrumentLayer( InstrumentLayer );
		InstrumentLayer = nullptr;
		pSample = nullptr;
		delete InstrumentLayer2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentList = std::make_shared<H2Core::InstrumentList>();
		auto pInstrumentList2 = std::shared_ptr<H2Core::InstrumentList>( pInstrumentList );
		pInstrumentList = nullptr;
		pInstrumentList2 = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto InstrumentComponent = std::make_shared<H2Core::InstrumentComponent>( 0 );
		auto InstrumentComponent2 = new H2Core::InstrumentComponent( InstrumentComponent );
		InstrumentComponent = nullptr;
		delete InstrumentComponent2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pADSR = std::make_shared<H2Core::ADSR>();
		auto pInstrument = std::make_shared<H2Core::Instrument>( 0, "ladida", pADSR );
		auto Note = new H2Core::Note( pInstrument, 0, 0.f, 0.f, 1, 1.f );
		auto Note2 = new H2Core::Note( Note );
		delete Note;
		delete Note2;
		pInstrument = nullptr;
		pADSR = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto Pattern = new H2Core::Pattern( "ladida", "ladida", "ladida" );
		auto Pattern2 = new H2Core::Pattern( Pattern );
		delete Pattern;
		delete Pattern2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto PatternList = new H2Core::PatternList();
		auto PatternList2 = new H2Core::PatternList( PatternList );
		delete PatternList;
		delete PatternList2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto Sample = std::make_shared<H2Core::Sample>( H2TEST_FILE( "/drumkits/baseKit/kick.wav" ) );
		auto Sample2 = new H2Core::Sample( Sample );
		Sample = nullptr;
		delete Sample2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto Song = new H2Core::Song( "ladida", "ladida", 120, 1 );
		delete Song;
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
												 true ) );
	CPPUNIT_ASSERT( pDrumkitProper != nullptr );
	
	pDrumkitProper->load_samples();
	
	CPPUNIT_ASSERT( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 )->get_layer( 0 )->get_sample() != nullptr );
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
		auto pInstrumentList = std::make_shared<H2Core::InstrumentList>( pSongProper->getInstrumentList() );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pPatternList = new H2Core::PatternList( pSongProper->getPatternList() );
		CPPUNIT_ASSERT( pPatternList != nullptr );
		delete pPatternList;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}
	
	{
		auto pPattern = new H2Core::Pattern( pSongProper->getPatternList()->get( 0 ) );
		CPPUNIT_ASSERT( pPattern != nullptr );
		delete pPattern;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto notes = pSongProper->getPatternList()->get( 0 )->get_notes();
		auto pNote = new H2Core::Note( notes->begin()->second );
		CPPUNIT_ASSERT( pNote != nullptr );
		delete pNote;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pDrumkitComponents = pSongProper->getComponents();
		auto pDrumkitComponent = std::make_shared<H2Core::DrumkitComponent>( (*pDrumkitComponents)[0] );
		CPPUNIT_ASSERT( pDrumkitComponent != nullptr );
		pDrumkitComponent = nullptr;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrument = new H2Core::Instrument( pDrumkitProper->get_instruments()->get( 0 ) );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		delete pInstrument;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pADSR = new H2Core::ADSR( pDrumkitProper->get_instruments()->get( 0 )->get_adsr() );
		CPPUNIT_ASSERT( pADSR != nullptr );
		delete pADSR;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentComponent = new H2Core::InstrumentComponent( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 ) );
		CPPUNIT_ASSERT( pInstrumentComponent != nullptr );
		delete pInstrumentComponent;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrumentLayer = new H2Core::InstrumentLayer( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 )->get_layer( 0 ) );
		CPPUNIT_ASSERT( pInstrumentLayer != nullptr );
		delete pInstrumentLayer;
		CPPUNIT_ASSERT( nNewCount == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pSample = new H2Core::Sample( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 )->get_layer( 0 )->get_sample() );
		CPPUNIT_ASSERT( pSample != nullptr );
		delete pSample;
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
	auto pCoreActionController = pHydrogen->getCoreActionController();

	QString sDrumkitPath =
		H2Core::Filesystem::drumkit_path_search( "GMRockKit",
												 H2Core::Filesystem::Lookup::system );

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/drumkitComponent.xml" ) ) );
		node = doc.firstChildElement( "drumkitComponent" );
		auto pDrumkitComponent = H2Core::DrumkitComponent::load_from( &node );
		CPPUNIT_ASSERT( pDrumkitComponent != nullptr );
		pDrumkitComponent = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/baseKit/" ) );
		CPPUNIT_ASSERT( pDrumkit != nullptr );

		pDrumkit->load_samples();
		
		pDrumkit = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentComponent.xml" ) ) );
		node = doc.firstChildElement( "instrumentComponent" );
		auto pInstrumentComponent = H2Core::InstrumentComponent::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "" );
		CPPUNIT_ASSERT( pInstrumentComponent != nullptr );
		pInstrumentComponent = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pInstrument = H2Core::Instrument::load_instrument( sDrumkitPath, "Kick" );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		pInstrument = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}
	
	{
		auto pInstrument = H2Core::Instrument::load_instrument( sDrumkitPath, "Kick" );
		pInstrument->load_from( sDrumkitPath, "Snare" );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		pInstrument = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrument.xml" ) ) );
		node = doc.firstChildElement( "instrument" );
		auto pInstrument = H2Core::Instrument::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "", "" );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		pInstrument = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentLayer.xml" ) ) );
		node = doc.firstChildElement( "instrumentLayer" );
		auto pInstrumentLayer = H2Core::InstrumentLayer::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "" );
		CPPUNIT_ASSERT( pInstrumentLayer != nullptr );
		pInstrumentLayer = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit", "" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit", "" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/note.xml" ) ) );
		node = doc.firstChildElement( "note" );
		auto pNote = H2Core::Note::load_from( &node, pInstrumentList );
		CPPUNIT_ASSERT( pNote != nullptr );
		delete pNote;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentListV2.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit", "" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/note.xml" ) ) );
		node = doc.firstChildElement( "note" );
		auto pNote = H2Core::Note::load_from( &node, pInstrumentList );
		CPPUNIT_ASSERT( pNote != nullptr );
		delete pNote;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "song" );
		auto pInstrumentList = H2Core::InstrumentList::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit", "" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		auto pPattern = H2Core::Pattern::load_file( H2TEST_FILE( "pattern/pat.h2pattern" ), pInstrumentList );
		CPPUNIT_ASSERT( pPattern != nullptr );
		delete pPattern;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pPlaylist = H2Core::Playlist::load_file( H2TEST_FILE( "playlist/test.h2playlist" ), true );
		CPPUNIT_ASSERT( pPlaylist != nullptr );
		delete pPlaylist;
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
		auto pInstrumentList = H2Core::InstrumentList::load_from(
			&node, H2TEST_FILE( "/drumkits/baseKit" ), "baseKit", "" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		auto pPattern = H2Core::Legacy::load_drumkit_pattern( H2TEST_FILE( "pattern/legacy_pattern.h2pattern" ), pInstrumentList );
		CPPUNIT_ASSERT( pPattern != nullptr );
		delete pPattern;
		pInstrumentList = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Base::getAliveObjectCount() );
	}

	{
		auto pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/baseKit" ) );
		pDrumkit->load_samples();
		auto pDrumkit2 = H2Core::Drumkit::load(
			H2Core::Filesystem::drumkit_path_search( "GMRockKit",
													 H2Core::Filesystem::Lookup::system,
													 true ) );
		pDrumkit2->load_samples();
	
		pCoreActionController->setDrumkit( pDrumkit );
		int nLoaded = H2Core::Base::getAliveObjectCount();
		pCoreActionController->setDrumkit( pDrumkit );
		CPPUNIT_ASSERT( nLoaded == H2Core::Base::getAliveObjectCount() );
		pCoreActionController->setDrumkit( pDrumkit2 );
		pCoreActionController->setDrumkit( pDrumkit );
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
