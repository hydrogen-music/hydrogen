/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <cppunit/extensions/HelperMacros.h>

#include <QString>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
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
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Playlist.h>
#include <core/SMF/SMF.h>
#include "TestHelper.h"
#include "assertions/File.h"
#include "assertions/AudioFile.h"

#include <chrono>
#include <memory>

using namespace H2Core;

class FunctionalTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( FunctionalTest );
	CPPUNIT_TEST( testExportAudio );
	CPPUNIT_TEST( testExportMIDISMF0 );
	CPPUNIT_TEST( testExportMIDISMF1Single );
	CPPUNIT_TEST( testExportMIDISMF1Multi );
//	CPPUNIT_TEST( testExportMuteGroupsAudio ); // SKIP
	CPPUNIT_TEST( testExportVelocityAutomationAudio );
	CPPUNIT_TEST( testExportVelocityAutomationMIDISMF0 );
	CPPUNIT_TEST( testExportVelocityAutomationMIDISMF1 );
	// CPPUNIT_TEST( testPrintMessages ); // MANUAL
	CPPUNIT_TEST_SUITE_END();
	
public:

	/** Intended for manual verification of the Print() methods of
		some basic core classes.*/
	void testPrintMessages()
	{
	___INFOLOG( "" );
		auto sSongFile = H2TEST_FILE( "functional/test.h2song" );
		auto sDrumkitFile = H2TEST_FILE( "/drumkits/baseKit" );

		// auto pSong = Song::load( sSongFile );
		// auto pVelocityAutomationPath = pSong->getVelocityAutomationPath();
		// auto pInstrumentList = pSong->getInstrumentList();
		// auto pInstrument = pInstrumentList->get( 0 );
		// auto pADSR = pInstrument->get_adsr();
		// auto pInstrumentComponent = pInstrument->get_component( 0 );
		// auto pInstrumentLayer = pInstrumentComponent->get_layer( 0 );
		// auto pSample = pInstrumentLayer->get_sample();
		// auto pPatternList = pSong->getPatternList();
		// auto pPattern = pPatternList->get( 0 );
		// auto pNote = pPattern->find_note( 0, -1, pInstrument, false );
		// auto pDrumkit = Drumkit::load( sDrumkitFile );
		// pDrumkit->load_samples();
		// auto pDrumkitComponent = (*pDrumkit->get_components())[ 0 ];
		// auto pPlaylist = Playlist::get_instance();
		// auto entry = Playlist::Entry{ "/tmp", true, "/usr/", false };
		// pPlaylist->add( &entry );
		// pPlaylist->add( &entry );

		auto pHydrogen = Hydrogen::get_instance();
		pHydrogen->getTimeline()->addTag( 0, "ladida" );
		pHydrogen->getTimeline()->addTag( 4, "test" );
		pHydrogen->getTimeline()->addTempoMarker( 2, 120 );
		pHydrogen->getTimeline()->addTempoMarker( 3, 220 );
		
		// std::cout << std::endl;
		// std::cout << pVelocityAutomationPath << std::endl;
		// std::cout << pInstrumentList << std::endl;
		// std::cout << pInstrument << std::endl;
		// std::cout << pADSR << std::endl;
		// std::cout << pInstrumentComponent << std::endl;
		// std::cout << pInstrumentLayer << std::endl;
		// std::cout << pSample << std::endl;
		// std::cout << pPatternList << std::endl;
		// std::cout << pPattern << std::endl;
		// std::cout << pNote << std::endl;
		// std::cout << pDrumkitComponent << std::endl;
		// std::cout << pDrumkit << std::endl;
		// std::cout << pSong << std::endl;
		// std::cout << pPlaylist << std::endl;
		// std::cout << Hydrogen::get_instance() << std::endl;
 
		// qDebug() << pVelocityAutomationPath;
		// qDebug() << pInstrumentList;
		// qDebug() << pInstrument;
		// qDebug() << pADSR;
		// qDebug() << pInstrumentComponent;
		// qDebug() << pInstrumentLayer;
		// qDebug() << pSample;
		// qDebug() << pPatternList;
		// qDebug() << pPattern;
		// qDebug() << pNote;
		// qDebug() << pDrumkitComponent;
		// qDebug() << pDrumkit;
		// qDebug() << pSong;
		// qDebug() << pPlaylist;
		// qDebug() << pHydrogen;
		std::cout << pHydrogen->toQString( "", false ).toLocal8Bit().data() << std::endl;;
 
		// 	std::cout << std::endl;
		// 	std::cout << pVelocityAutomationPath->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pInstrumentList->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pInstrument->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pADSR->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pInstrumentComponent->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pInstrumentLayer->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pSample->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pPatternList->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pPattern->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pNote->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pDrumkitComponent->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pDrumkit->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// 	std::cout << pSong->toQString( "", false ).toLocal8Bit().data() << std::endl;
		// std::cout << pPlaylist->toQString( "", false ).toLocal8Bit().data();

	___INFOLOG( "passed" );
	}

	void testExportAudio()
	{
	___INFOLOG( "" );
		const auto sSongFile = H2TEST_FILE("functional/test_adsr.h2song");

		// Test exporting using a couple of different sample rate (to test both
		// resampling and no resampling) and sample depths.
		struct Setup {
			QString sTempFile;
			QString sReferenceFile;
			int nSampleRate;
			int nSampleDepth;
		};

		std::vector<Setup> setups;
		setups.push_back( { "test-44100-16.wav",
				"functional/test-44100-16.ref.flac", 44100, 16} );
		setups.push_back( { "test-48000-16.wav",
				"functional/test-48000-16.ref.flac", 48000, 16} );
		setups.push_back( { "test-48000-32.wav",
				"functional/test-48000-32.ref.flac", 48000, 32} );

		for ( int ii = 0; ii < setups.size(); ++ii ) {
			___INFOLOG( QString( "Testing sample rate: [%1] and depth: [%2]" )
						.arg( setups[ii].nSampleRate )
						.arg( setups[ii].nSampleDepth ) );
			const auto sOutFile = Filesystem::tmp_file_path( setups[ii].sTempFile );
			const auto sRefFile = H2TEST_FILE( setups[ii].sReferenceFile );

			TestHelper::exportSong( sSongFile, sOutFile, setups[ii].nSampleRate,
									setups[ii].nSampleDepth );
			H2TEST_ASSERT_AUDIO_FILES_EQUAL( sRefFile, sOutFile );
			Filesystem::rm( sOutFile );
		}
	___INFOLOG( "passed" );
	}

	void testExportMIDISMF1Single()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("smf1single.test.mid");
		auto refFile = H2TEST_FILE("functional/smf1single.test.ref.mid");

		SMF1WriterSingle writer;
		TestHelper::exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}
	
	void testExportMIDISMF1Multi()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("smf1multi.test.mid");
		auto refFile = H2TEST_FILE("functional/smf1multi.test.ref.mid");

		SMF1WriterMulti writer;
		TestHelper::exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}
	
	void testExportMIDISMF0()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("smf0.test.mid");
		auto refFile = H2TEST_FILE("functional/smf0.test.ref.mid");

		SMF0Writer writer;
		TestHelper::exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}
	
/* SKIP
	void testExportMuteGroupsAudio()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/mutegroups.h2song");
		auto outFile = Filesystem::tmp_file_path("mutegroups.wav");
		auto refFile = H2TEST_FILE("functional/mutegroups.ref.flac");

		TestHelper::exportSong( songFile, outFile );
		H2TEST_ASSERT_AUDIO_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}
*/
	void testExportVelocityAutomationAudio()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file_path("velocityautomation.wav");
		auto refFile = H2TEST_FILE("functional/velocityautomation.ref.flac");

		TestHelper::exportSong( songFile, outFile );
		H2TEST_ASSERT_AUDIO_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}

	void testExportVelocityAutomationMIDISMF1()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file_path("smf1.velocityautomation.mid");
		auto refFile = H2TEST_FILE("functional/smf1.velocityautomation.ref.mid");

		SMF1WriterSingle writer;
		TestHelper::exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}
	
	void testExportVelocityAutomationMIDISMF0()
	{
	___INFOLOG( "" );
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file_path("smf0.velocityautomation.mid");
		auto refFile = H2TEST_FILE("functional/smf0.velocityautomation.ref.mid");

		SMF0Writer writer;
		TestHelper::exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		
		Filesystem::rm( outFile );
	___INFOLOG( "passed" );
	}


};
