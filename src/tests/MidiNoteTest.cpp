#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>

#include <QFileInfo>

#include <iostream>
#include <stdexcept>

using namespace H2Core;

#define ASSERT_INSTRUMENT_MIDI_NOTE(name, note, instr) checkInstrumentMidiNote(name, note, instr, CPPUNIT_SOURCELINE())
void
checkInstrumentMidiNote(std::string name, int note, Instrument *instr, CppUnit::SourceLine sl)
{
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
static QString
get_test_file(const QString &name)
{
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


class MidiNoteTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( MidiNoteTest );
	CPPUNIT_TEST( testLoadLegacySong );
	CPPUNIT_TEST( testLoadNewSong );
	CPPUNIT_TEST_SUITE_END();

	public:

	void setUp() override
	{
		Hydrogen::create_instance();
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

		SongReader reader;
		auto song = reader.readSong( get_test_file("song/test_song_0.9.6.h2song") );
		CPPUNIT_ASSERT( song != nullptr );

		auto instruments = song->get_instrument_list();
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
		/* Read song with instruments that have assigned distinct
		 * MIDI notes. Check that loading that song does not
		 * change that mapping */

		SongReader reader;
		auto song = reader.readSong( get_test_file("song/test_song_0.9.7.h2song") );
		CPPUNIT_ASSERT( song != nullptr );

		auto instruments = song->get_instrument_list();
		CPPUNIT_ASSERT( instruments != nullptr );
		CPPUNIT_ASSERT_EQUAL( 4, instruments->size() );

		ASSERT_INSTRUMENT_MIDI_NOTE( "Kick",       35, instruments->get(0) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Snare Rock", 40, instruments->get(1) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Crash",      49, instruments->get(2) );
		ASSERT_INSTRUMENT_MIDI_NOTE( "Ride Rock",  59, instruments->get(3) );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( MidiNoteTest );
