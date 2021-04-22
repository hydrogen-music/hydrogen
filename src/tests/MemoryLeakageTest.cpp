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

#include <core/Sampler/Sampler.h>
#include <core/Sampler/Sampler.cpp>
#include <core/Helpers/Xml.h>
#include <core/Helpers/Legacy.h>

#include "TestHelper.h"

CPPUNIT_TEST_SUITE_REGISTRATION( MemoryLeakageTest );


void MemoryLeakageTest::testConstructors() {
	int nAliveReference = H2Core::Object::getAliveObjectCount();

	{
		auto ADSR = new H2Core::ADSR();
		auto ADSR2 = new H2Core::ADSR( ADSR );
		delete ADSR;
		delete ADSR2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto AutomationPath = new H2Core::AutomationPath( 0, 1, 0.2 );
		delete AutomationPath;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto Drumkit = new H2Core::Drumkit();
		auto Drumkit2 = new H2Core::Drumkit( Drumkit );
		delete Drumkit;
		delete Drumkit2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto DrumkitComponent = new H2Core::DrumkitComponent( 0, "ladida" );
		auto DrumkitComponent2 = new H2Core::DrumkitComponent( DrumkitComponent );
		delete DrumkitComponent;
		delete DrumkitComponent2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pADSR = new H2Core::ADSR();
		auto Instrument = new H2Core::Instrument( 0, "ladida", pADSR );
		auto Instrument2 = new H2Core::Instrument( Instrument );
		delete Instrument;
		delete Instrument2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pSample = std::make_shared<H2Core::Sample>( H2TEST_FILE( "/drumkits/baseKit/kick.wav" ) );
		auto InstrumentLayer = new H2Core::InstrumentLayer( pSample );
		auto InstrumentLayer2 = new H2Core::InstrumentLayer( InstrumentLayer );
		delete InstrumentLayer;
		pSample = nullptr;
		delete InstrumentLayer2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto InstrumentList = new H2Core::InstrumentList();
		auto InstrumentList2 = new H2Core::InstrumentList( InstrumentList );
		delete InstrumentList;
		delete InstrumentList2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto InstrumentComponent = new H2Core::InstrumentComponent( 0 );
		auto InstrumentComponent2 = new H2Core::InstrumentComponent( InstrumentComponent );
		delete InstrumentComponent;
		delete InstrumentComponent2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );	
	}

	{
		auto pADSR = new H2Core::ADSR();
		auto pInstrument = new H2Core::Instrument( 0, "ladida", pADSR );
		auto Note = new H2Core::Note( pInstrument, 0, 0, 0, 0, 1, 1 );
		auto Note2 = new H2Core::Note( Note );
		delete Note;
		delete Note2;
		delete pInstrument;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto Pattern = new H2Core::Pattern( "ladida", "ladida", "ladida" );
		auto Pattern2 = new H2Core::Pattern( Pattern );
		delete Pattern;
		delete Pattern2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto PatternList = new H2Core::PatternList();
		auto PatternList2 = new H2Core::PatternList( PatternList );
		delete PatternList;
		delete PatternList2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto Sample = std::make_shared<H2Core::Sample>( H2TEST_FILE( "/drumkits/baseKit/kick.wav" ) );
		auto Sample2 = new H2Core::Sample( Sample );
		Sample = nullptr;
		delete Sample2;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto Song = new H2Core::Song( "ladida", "ladida", 120, 1 );
		delete Song;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}
	
	{
		auto pSampler = new H2Core::Sampler();
		delete pSampler;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	// Test copy constructors using real-live instead of new objects.
	auto pDrumkitProper = H2Core::Drumkit::load_by_name( "GMRockKit", true, H2Core::Filesystem::Lookup::system );
	CPPUNIT_ASSERT( pDrumkitProper != nullptr );
	auto pSongProper = H2Core::Song::load( H2Core::Filesystem::demos_dir() + "GM_kit_Diddley.h2song" );
	CPPUNIT_ASSERT( pSongProper != nullptr );

	int nNewCount = H2Core::Object::getAliveObjectCount();

	{
		auto pDrumkit = new H2Core::Drumkit( pDrumkitProper );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		delete pDrumkit;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pInstrumentList = new H2Core::InstrumentList( pSongProper->getInstrumentList() );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		delete pInstrumentList;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pPatternList = new H2Core::PatternList( pSongProper->getPatternList() );
		CPPUNIT_ASSERT( pPatternList != nullptr );
		delete pPatternList;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}
	
	{
		auto pPattern = new H2Core::Pattern( pSongProper->getPatternList()->get( 0 ) );
		CPPUNIT_ASSERT( pPattern != nullptr );
		delete pPattern;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto notes = pSongProper->getPatternList()->get( 0 )->get_notes();
		auto pNote = new H2Core::Note( notes->begin()->second );
		CPPUNIT_ASSERT( pNote != nullptr );
		delete pNote;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pDrumkitComponents = pSongProper->getComponents();
		auto pDrumkitComponent = new H2Core::DrumkitComponent( (*pDrumkitComponents)[0] );
		CPPUNIT_ASSERT( pDrumkitComponent != nullptr );
		delete pDrumkitComponent;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pInstrument = new H2Core::Instrument( pDrumkitProper->get_instruments()->get( 0 ) );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		delete pInstrument;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pADSR = new H2Core::ADSR( pDrumkitProper->get_instruments()->get( 0 )->get_adsr() );
		CPPUNIT_ASSERT( pADSR != nullptr );
		delete pADSR;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pInstrumentComponent = new H2Core::InstrumentComponent( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 ) );
		CPPUNIT_ASSERT( pInstrumentComponent != nullptr );
		delete pInstrumentComponent;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pInstrumentLayer = new H2Core::InstrumentLayer( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 )->get_layer( 0 ) );
		CPPUNIT_ASSERT( pInstrumentLayer != nullptr );
		delete pInstrumentLayer;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pSample = new H2Core::Sample( pDrumkitProper->get_instruments()->get( 0 )->get_component( 0 )->get_layer( 0 )->get_sample() );
		CPPUNIT_ASSERT( pSample != nullptr );
		delete pSample;
		CPPUNIT_ASSERT( nNewCount == H2Core::Object::getAliveObjectCount() );
	}
	
	delete pDrumkitProper;
	delete pSongProper;
	CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
}

void MemoryLeakageTest::testLoading() {
	H2Core::XMLDoc doc;
	H2Core::XMLNode node;

	auto mapSnapshot = H2Core::Object::getObjectMap();
	int nAliveReference = H2Core::Object::getAliveObjectCount();

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/drumkitComponent.xml" ) ) );
		node = doc.firstChildElement( "drumkitComponent" );
		auto pDrumkitComponent = H2Core::DrumkitComponent::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ) );
		CPPUNIT_ASSERT( pDrumkitComponent != nullptr );
		delete pDrumkitComponent;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pDrumkit = H2Core::Drumkit::load_file( H2TEST_FILE( "drumkits/baseKit/drumkit.xml" ), true );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		delete pDrumkit;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentComponent.xml" ) ) );
		node = doc.firstChildElement( "instrumentComponent" );
		auto pInstrumentComponent = H2Core::InstrumentComponent::load_from( &node, H2TEST_FILE( "drumkits/baseKit" ) );
		CPPUNIT_ASSERT( pInstrumentComponent != nullptr );
		delete pInstrumentComponent;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pInstrument = H2Core::Instrument::load_instrument( "GMRockKit", "Kick", H2Core::Filesystem::Lookup::system );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		delete pInstrument;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}
	
	{
		auto pInstrument = H2Core::Instrument::load_instrument( "GMRockKit", "Kick", H2Core::Filesystem::Lookup::system );
		pInstrument->load_from( "GMRockKit", "Snare", false, H2Core::Filesystem::Lookup::system );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		delete pInstrument;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrument.xml" ) ) );
		node = doc.firstChildElement( "instrument" );
		auto pInstrument = H2Core::Instrument::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		delete pInstrument;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentLayer.xml" ) ) );
		node = doc.firstChildElement( "instrumentLayer" );
		auto pInstrumentLayer = H2Core::InstrumentLayer::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ) );
		CPPUNIT_ASSERT( pInstrumentLayer != nullptr );
		delete pInstrumentLayer;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "instrumentList" );
		auto pInstrumentList = H2Core::InstrumentList::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		delete pInstrumentList;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		// Yes, saving creates a new Drumkit as well.
		doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) );
		node = doc.firstChildElement( "instrumentList" );
		auto pInstrumentList = H2Core::InstrumentList::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
		doc.read( H2TEST_FILE( "/memoryLeakage/drumkitComponent.xml" ) );
		node = doc.firstChildElement( "drumkitComponent" );
		auto pDrumkitComponent = H2Core::DrumkitComponent::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ) );
		std::vector<H2Core::DrumkitComponent*> pDrumkitComponents { pDrumkitComponent };
		CPPUNIT_ASSERT( H2Core::Drumkit::save( "testKitLadida", "ladida", "ladida", "ladida", "ladida", "ladida", pInstrumentList, &pDrumkitComponents, true ) );
		delete pInstrumentList;
		delete pDrumkitComponent;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "instrumentList" );
		auto pInstrumentList = H2Core::InstrumentList::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/note.xml" ) ) );
		node = doc.firstChildElement( "note" );
		auto pNote = H2Core::Note::load_from( &node, pInstrumentList );
		CPPUNIT_ASSERT( pNote != nullptr );
		delete pNote;
		delete pInstrumentList;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	// {
	// 	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentListV2.xml" ) ) );
	// 	node = doc.firstChildElement( "instrumentList" );
	// 	auto pInstrumentList = H2Core::InstrumentList::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
	// 	CPPUNIT_ASSERT( pInstrumentList != nullptr );
	// 	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/note.xml" ) ) );
	// 	node = doc.firstChildElement( "note" );
	// 	auto pNote = H2Core::Note::load_from( &node, pInstrumentList );
	// 	CPPUNIT_ASSERT( pNote != nullptr );
	// 	delete pNote;
	// 	delete pInstrumentList;
	// 	H2Core::Object::printObjectMapDiff( mapSnapshot );
	// 	CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	// }

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "instrumentList" );
		auto pInstrumentList = H2Core::InstrumentList::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		auto pPattern = H2Core::Pattern::load_file( H2TEST_FILE( "pattern/pat.h2pattern" ), pInstrumentList );
		CPPUNIT_ASSERT( pPattern != nullptr );
		delete pPattern;
		delete pInstrumentList;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pPlaylist = H2Core::Playlist::load_file( H2TEST_FILE( "playlist/test.h2playlist" ), true );
		CPPUNIT_ASSERT( pPlaylist != nullptr );
		delete pPlaylist;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pSample = H2Core::Sample::load( H2TEST_FILE( "drumkits/baseKit/snare.wav" ) );
		CPPUNIT_ASSERT( pSample != nullptr );
		pSample = nullptr;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pSong = H2Core::Song::getDefaultSong();
		CPPUNIT_ASSERT( pSong != nullptr );
		delete pSong;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	// {
	// 	auto pSong = H2Core::Song::getEmptySong();
	// 	pSong->readTempPatternList( H2TEST_FILE( "memoryLeakage/tempPatternList.xml" ) );
	// 	CPPUNIT_ASSERT( pSong != nullptr );
	// 	delete pSong;
	// 	CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	// }

	{
		auto pReader = new H2Core::SongReader();
		auto pSong = pReader->readSong( H2TEST_FILE( "functional/test.h2song" ) );
		CPPUNIT_ASSERT( pSong != nullptr );
		delete pSong;
		delete pReader;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pSampler = new H2Core::Sampler();
		pSampler->reinitializePlaybackTrack();
		delete pSampler;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		H2Core::Hydrogen::get_instance()->getSong()->setPlaybackTrackFilename( H2TEST_FILE( "drumkits/baseKit/kick.wav" ) );
		auto pSampler = new H2Core::Sampler();
		pSampler->reinitializePlaybackTrack();
		delete pSampler;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pInstrument = H2Core::createInstrument( 0, H2TEST_FILE( "drumkits/baseKit/kick.wav" ), 0.7 );
		delete pInstrument;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pDrumkit = H2Core::Legacy::load_drumkit( H2TEST_FILE( "drumkits/legacy_GMkit/drumkit.xml" ) );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		delete pDrumkit;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/memoryLeakage/instrumentList.xml" ) ) );
		node = doc.firstChildElement( "instrumentList" );
		auto pInstrumentList = H2Core::InstrumentList::load_from( &node, H2TEST_FILE( "/drumkits/baseKit" ), "H2 test DK" );
		CPPUNIT_ASSERT( pInstrumentList != nullptr );
		auto pPattern = H2Core::Legacy::load_drumkit_pattern( H2TEST_FILE( "pattern/legacy_pattern.h2pattern" ), pInstrumentList );
		CPPUNIT_ASSERT( pPattern != nullptr );
		delete pPattern;
		delete pInstrumentList;
		CPPUNIT_ASSERT( nAliveReference == H2Core::Object::getAliveObjectCount() );
	}

	{
		auto pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/baseKit" ), true );
		auto pDrumkit2 = H2Core::Drumkit::load_by_name( "GMRockKit", true, H2Core::Filesystem::Lookup::system );
	
		H2Core::Hydrogen::get_instance()->loadDrumkit( pDrumkit );
		int nLoaded = H2Core::Object::getAliveObjectCount();
		H2Core::Hydrogen::get_instance()->loadDrumkit( pDrumkit );
		CPPUNIT_ASSERT( nLoaded == H2Core::Object::getAliveObjectCount() );
		H2Core::Hydrogen::get_instance()->loadDrumkit( pDrumkit2 );
		H2Core::Hydrogen::get_instance()->loadDrumkit( pDrumkit );
		CPPUNIT_ASSERT( nLoaded == H2Core::Object::getAliveObjectCount() );
	}	
}

void MemoryLeakageTest::tearDown() {
	if ( H2Core::Filesystem::drumkit_exists( "testKitLadida" ) ) {
		QString sPath = H2Core::Filesystem::drumkit_usr_path( "testKitLadida" );
		CPPUNIT_ASSERT( H2Core::Filesystem::rm( sPath, true ) );
	}
}
