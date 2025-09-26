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

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Song.h>

#include <core/IO/MidiCommon.h>

#include <QFileInfo>

#include "TestHelper.h"

#include <iostream>
#include <stdexcept>

using namespace H2Core;

#define ASSERT_INSTRUMENT_MIDI_NOTE(name, note, instr) checkInstrumentMidiNote(name, note, instr, CPPUNIT_SOURCELINE())

class MidiNoteTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( MidiNoteTest );
	CPPUNIT_TEST( testDefaultValues );
	CPPUNIT_TEST( testLoadLegacySong );
	CPPUNIT_TEST( testLoadNewSong );
	CPPUNIT_TEST_SUITE_END();

	public:

	void setUp() override
	{
		Hydrogen::create_instance();
	}

	void testDefaultValues() {
		___INFOLOG( "" );
		CPPUNIT_ASSERT( ( OCTAVE_DEFAULT + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE ==
						MidiMessage::instrumentOffset );
		___INFOLOG( "passed" );
	}

	void testLoadLegacySong()
	{
		return; // skip this test

		/* Read song created in previous version of Hydrogen.
		 * In that song, all instruments have MIDI note set to 60.
		 * Exporting that song to MIDI results in unusable track
		 * where all instruments play the same note.
		 * That confuses users, so after loading song, assign
		 * instruments sequential numbers starting from 36,
		 * preserving legacy behavior. */

		auto pSong = H2Core::Song::load( H2TEST_FILE( "song/legacy/test_song_0.9.6.h2song" ) );
		CPPUNIT_ASSERT( pSong != nullptr );

		auto instruments = pSong->getInstrumentList();
		CPPUNIT_ASSERT( instruments != nullptr );
		CPPUNIT_ASSERT_EQUAL( 16, instruments->size() );

		ASSERT_INSTRUMENT_MIDI_NOTE( "Kick",       36, instruments->get(0) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Stick",      37, instruments->get(1) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Snare Jazz", 38, instruments->get(2) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Hand Clap",  39, instruments->get(3) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Closed HH",  42, instruments->get(6) );
	}

	void testLoadNewSong()
	{
	___INFOLOG( "" );
		/* Read song with instruments that have assigned distinct
		 * MIDI notes. Check that loading that song does not
		 * change that mapping */

		auto pSong = H2Core::Song::load( H2TEST_FILE( "song/legacy/test_song_0.9.7.h2song" ) );
		CPPUNIT_ASSERT( pSong != nullptr );

		auto instruments = pSong->getInstrumentList();
		CPPUNIT_ASSERT( instruments != nullptr );
		CPPUNIT_ASSERT_EQUAL( 4, instruments->size() );

		ASSERT_INSTRUMENT_MIDI_NOTE( "Kick",       35, instruments->get(0) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Snare Rock", 40, instruments->get(1) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Crash",      49, instruments->get(2) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Ride Rock",  59, instruments->get(3) );
	___INFOLOG( "passed" );
	}

private:
	void checkInstrumentMidiNote(std::string name, int note, std::shared_ptr<Instrument> instr, CppUnit::SourceLine sl) {
		auto instrName = instr->get_name().toStdString();
		auto instrIdx = instr->get_id();
		auto instrNote = instr->get_midi_out_note();

		if (instrName != name) {
			std::string msg = "Bad instrument at index " + std::to_string(instrIdx);
			::CppUnit::Asserter::failNotEqual(name, instrName, sl, CppUnit::AdditionalMessage(), msg);
		}

		if (instrNote != note) {
			std::string msg = "Bad MIDI out note for instrument " + instrName;
			::CppUnit::Asserter::failNotEqual(std::to_string(note), std::to_string(instrNote), sl, CppUnit::AdditionalMessage(), msg);
		}
	}


	/* Find test file
	 *
	 * This function tries to find test files in several directories,
	 * so they can be found whether tests have been run from project
	 * root or build directory.
	 *
	 * Exception of class std::runtime_error is thrown when file
	 * can't be found.
	 */
	static QString get_test_file(const QString &name) {
		std::vector<QString> paths = { "./src", "../src" };
		for (auto const &path : paths) {
			QString fileName = path + "/tests/data/" + name;
			QFileInfo fi(fileName);
			if ( fi.exists() ) {
				return fileName;
			}
		}
		throw std::runtime_error(std::string("Can't find test file ") + name.toStdString());
	}
};

