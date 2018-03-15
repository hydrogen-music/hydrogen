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
#include <hydrogen/basics/song.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/smf/SMF.h>
#include "test_helper.h"
#include "assertions/file.h"

#include <chrono>

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
	CPPUNIT_ASSERT( pSong != NULL );
	pHydrogen->setSong( pSong );

	InstrumentList *pInstrumentList = pSong->get_instrument_list();
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
void exportMIDI( const QString &songFile, const QString &fileName )
{
	auto t0 = std::chrono::high_resolution_clock::now();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = Song::load( songFile );
	CPPUNIT_ASSERT( pSong != NULL );

	SMFWriter writer;
	writer.save( fileName, pSong );

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("MIDI export took %1 seconds").arg(t) );
}


class FunctionalTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( FunctionalTest );
	CPPUNIT_TEST( testExportAudio );
	CPPUNIT_TEST( testExportMIDI );
	CPPUNIT_TEST( testExportMuteGroupsAudio );
	CPPUNIT_TEST( testExportVelocityAutomationAudio );
	CPPUNIT_TEST( testExportVelocityAutomationMIDI );
	CPPUNIT_TEST_SUITE_END();

	public:

	void testExportAudio()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file("test.wav");
		auto refFile = H2TEST_FILE("functional/test.ref.wav");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportMIDI()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file("test.mid");
		auto refFile = H2TEST_FILE("functional/test.ref.mid");

		exportMIDI( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportMuteGroupsAudio()
	{
		auto songFile = H2TEST_FILE("functional/mutegroups.h2song");
		auto outFile = Filesystem::tmp_file("mutegroups.wav");
		auto refFile = H2TEST_FILE("functional/mutegroups.ref.wav");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportVelocityAutomationAudio()
	{
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file("velocityautomation.wav");
		auto refFile = H2TEST_FILE("functional/velocityautomation.ref.wav");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportVelocityAutomationMIDI()
	{
		auto songFile = H2TEST_FILE("functional/velocityautomation.h2song");
		auto outFile = Filesystem::tmp_file("velocityautomation.mid");
		auto refFile = H2TEST_FILE("functional/velocityautomation.ref.mid");

		exportMIDI( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

};
CPPUNIT_TEST_SUITE_REGISTRATION( FunctionalTest );
