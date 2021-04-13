/*
 * Hydrogen
 * Copyright(c) 2002-2018 by the Hydrogen Team
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <core/Smf/SMF.h>
#include "TestHelper.h"
#include "assertions/File.h"
#include "assertions/AudioFile.h"

#include <chrono>
#include <memory>

using namespace H2Core;

/**
 * \brief Export Hydrogon song to audio file
 * \param songFile Path to Hydrogen file
 * \param fileName Output file name
 **/
void exportSong( const QString &songFile, const QString &fileName )
{
	auto t0 = std::chrono::high_resolution_clock::now();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	EventQueue *pQueue = EventQueue::get_instance();

	Song *pSong = Song::load( songFile );
	CPPUNIT_ASSERT( pSong != nullptr );
	
	if( !pSong ) {
		return;
	}
	
	pHydrogen->setSong( pSong );

	InstrumentList *pInstrumentList = pSong->getInstrumentList();
	for (auto i = 0; i < pInstrumentList->size(); i++) {
		pInstrumentList->get(i)->set_currently_exported( true );
	}

	pHydrogen->startExportSession( 44100, 16 );
	pHydrogen->startExportSong( fileName );

	bool done = false;
	while ( ! done ) {
		Event event = pQueue->pop_event();

		if (event.type == EVENT_PROGRESS && event.value == 100) {
			done = true;
		}
		else {
			usleep(100 * 1000);
		}
	}
	pHydrogen->stopExportSession();

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("Audio export took %1 seconds").arg(t) );
}

/**
 * \brief Export Hydrogon song to MIDI file
 * \param songFile Path to Hydrogen file
 * \param fileName Output file name
 **/
void exportMIDI( const QString &songFile, const QString &fileName, SMFWriter& writer )
{
	auto t0 = std::chrono::high_resolution_clock::now();

	std::unique_ptr<Song> pSong { Song::load( songFile ) };
	CPPUNIT_ASSERT( pSong != nullptr );

	writer.save( fileName, pSong.get() );

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("MIDI track export took %1 seconds").arg(t) );
}


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
		auto sSongFile = H2TEST_FILE( "functional/test.h2song" );
		auto sDrumkitFile = H2TEST_FILE( "/drumkits/baseKit" );

		auto pSong = Song::load( sSongFile );
		auto pVelocityAutomationPath = pSong->getVelocityAutomationPath();
		auto pInstrumentList = pSong->getInstrumentList();
		auto pInstrument = pInstrumentList->get( 0 );
		auto pADSR = pInstrument->get_adsr();
		auto pInstrumentComponent = pInstrument->get_component( 0 );
		auto pInstrumentLayer = pInstrumentComponent->get_layer( 0 );
		auto pSample = pInstrumentLayer->get_sample();
		auto pPatternList = pSong->getPatternList();
		auto pPattern = pPatternList->get( 0 );
		auto pNote = pPattern->find_note( 0, -1, pInstrument, false );
		auto pDrumkit = Drumkit::load( sDrumkitFile, true );
		auto pDrumkitComponent = (*pDrumkit->get_components())[ 0 ];

		std::cout << std::endl;
		std::cout << pVelocityAutomationPath << std::endl;
		std::cout << pInstrumentList << std::endl;
		std::cout << pInstrument << std::endl;
		std::cout << pADSR << std::endl;
		std::cout << pInstrumentComponent << std::endl;
		std::cout << pInstrumentLayer << std::endl;
		std::cout << pSample << std::endl;
		std::cout << pPatternList << std::endl;
		std::cout << pPattern << std::endl;
		std::cout << pNote << std::endl;
		std::cout << pDrumkitComponent << std::endl;
		std::cout << pDrumkit << std::endl;
		std::cout << pSong << std::endl;
		std::cout << Hydrogen::get_instance() << std::endl;
 
		qDebug() << pVelocityAutomationPath;
		qDebug() << pInstrumentList;
		qDebug() << pInstrument;
		qDebug() << pADSR;
		qDebug() << pInstrumentComponent;
		qDebug() << pInstrumentLayer;
		qDebug() << pSample;
		qDebug() << pPatternList;
		qDebug() << pPattern;
		qDebug() << pNote;
		qDebug() << pDrumkitComponent;
		qDebug() << pDrumkit;
		qDebug() << pSong;
		qDebug() << Hydrogen::get_instance();
 
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

	}

	void testExportAudio()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("test.wav");
		auto refFile = H2TEST_FILE("functional/test.ref.flac");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_AUDIO_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportMIDISMF1Single()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("smf1single.test.mid");
		auto refFile = H2TEST_FILE("functional/smf1single.test.ref.mid");

		SMF1WriterSingle writer;
		exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}
	
	void testExportMIDISMF1Multi()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("smf1multi.test.mid");
		auto refFile = H2TEST_FILE("functional/smf1multi.test.ref.mid");

		SMF1WriterMulti writer;
		exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}
	
	void testExportMIDISMF0()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file_path("smf0.test.mid");
		auto refFile = H2TEST_FILE("functional/smf0.test.ref.mid");

		SMF0Writer writer;
		exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}
	
/* SKIP
	void testExportMuteGroupsAudio()
	{
		auto songFile = H2TEST_FILE("functional/mutegroups.h2song");
		auto outFile = Filesystem::tmp_file_path("mutegroups.wav");
		auto refFile = H2TEST_FILE("functional/mutegroups.ref.flac");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_AUDIO_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}
*/
	void testExportVelocityAutomationAudio()
	{
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file_path("velocityautomation.wav");
		auto refFile = H2TEST_FILE("functional/velocityautomation.ref.flac");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_AUDIO_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportVelocityAutomationMIDISMF1()
	{
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file_path("smf1.velocityautomation.mid");
		auto refFile = H2TEST_FILE("functional/smf1.velocityautomation.ref.mid");

		SMF1WriterSingle writer;
		exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		
		Filesystem::rm( outFile );
	}
	
	void testExportVelocityAutomationMIDISMF0()
	{
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file_path("smf0.velocityautomation.mid");
		auto refFile = H2TEST_FILE("functional/smf0.velocityautomation.ref.mid");

		SMF0Writer writer;
		exportMIDI( songFile, outFile, writer );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		
		Filesystem::rm( outFile );
	}

};
CPPUNIT_TEST_SUITE_REGISTRATION( FunctionalTest );
