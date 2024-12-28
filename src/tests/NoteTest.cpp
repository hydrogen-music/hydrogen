/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/IO/MidiCommon.h>
#include <core/Preferences/Shortcuts.h>
#include <core/Helpers/Xml.h>
#include <QDomDocument>

using namespace H2Core;

class NoteTest : public CppUnit::TestCase {
		CPPUNIT_TEST_SUITE( NoteTest );
		CPPUNIT_TEST( testMidiDefaultOffset );
		CPPUNIT_TEST( testPitchConversions );
		CPPUNIT_TEST( testVirtualKeyboard );
		CPPUNIT_TEST( testProbability );
		CPPUNIT_TEST( testSerializeProbability );
		CPPUNIT_TEST_SUITE_END();

	void testMidiDefaultOffset() {
		___INFOLOG( "" );
		CPPUNIT_ASSERT_EQUAL( MidiMessage::instrumentOffset, KEYS_PER_OCTAVE *
							  ( OCTAVE_DEFAULT + OCTAVE_OFFSET ) );
		___INFOLOG( "passed" );
	}

		void testPitchConversions() {
			___INFOLOG( "" );

			CPPUNIT_ASSERT( H2Core::Note::C == KEY_MIN );
			CPPUNIT_ASSERT( H2Core::Note::B == KEY_MAX );
			CPPUNIT_ASSERT( KEYS_PER_OCTAVE == KEY_MAX - KEY_MIN + 1 );
			CPPUNIT_ASSERT( H2Core::Note::P8Z == OCTAVE_MIN );
			CPPUNIT_ASSERT( H2Core::Note::P8C == OCTAVE_MAX );
			CPPUNIT_ASSERT( OCTAVE_NUMBER == OCTAVE_MAX - OCTAVE_MIN + 1 );
			CPPUNIT_ASSERT( H2Core::Note::P8 == OCTAVE_DEFAULT );

			std::vector<int> octaves = { H2Core::Note::P8Z, H2Core::Note::P8Y,
				H2Core::Note::P8X, H2Core::Note::P8, H2Core::Note::P8A,
				H2Core::Note::P8B, H2Core::Note::P8C, OCTAVE_MIN, OCTAVE_MAX,
				OCTAVE_DEFAULT };

			std::vector<int> keys = { H2Core::Note::C, H2Core::Note::Cs,
				H2Core::Note::D, H2Core::Note::Ef, H2Core::Note::E,
				H2Core::Note::Fs, H2Core::Note::G, H2Core::Note::Af,
				H2Core::Note::A, H2Core::Note::Bf, H2Core::Note::B,
				KEY_MIN, KEY_MAX };

			auto pInstrument = std::make_shared<H2Core::Instrument>();
			for ( const auto ooctave : octaves ) {
				for ( const auto kkey : keys ) {
					auto pNote = std::make_shared<H2Core::Note>( pInstrument );
					pNote->set_key_octave(
						static_cast<H2Core::Note::Key>(kkey),
						static_cast<H2Core::Note::Octave>(ooctave) );

					const float fPitch = pNote->get_pitch_from_key_octave();
					const int nLine = Note::pitchToLine( fPitch );
					const int nPitch = Note::lineToPitch( nLine );
					CPPUNIT_ASSERT( static_cast<int>(fPitch) == nPitch );

					const auto key = Note::pitchToKey( nPitch );
					const auto octave = Note::pitchToOctave( nPitch );
					CPPUNIT_ASSERT( key == kkey );
					CPPUNIT_ASSERT( octave == ooctave );
				}
			}

			___INFOLOG( "passed" );
		}

	/** Check whether notes entered via the virtual keyboard can be
	 * handled properly */
	void testVirtualKeyboard() {
		___INFOLOG( "" );
		CPPUNIT_ASSERT_EQUAL( static_cast<int>(Shortcuts::Action::VK_36_C2), 400 );
		CPPUNIT_ASSERT_EQUAL( MidiMessage::instrumentOffset, 36 ); // MIDI note C2
		___INFOLOG( "passed" );
	}

	void testProbability()
	{
		___INFOLOG( "" );

		auto pNote = std::make_shared<Note>( nullptr, 0, 1.0f, 0.f, 1, 1.0f );
		pNote->set_probability(0.75f);
		CPPUNIT_ASSERT_EQUAL(0.75f, pNote->get_probability());

		auto pOther = std::make_shared<Note>( pNote, nullptr );
		CPPUNIT_ASSERT_EQUAL(0.75f, pOther->get_probability());

		___INFOLOG( "passed" );
	}

	void testSerializeProbability()
	{
	___INFOLOG( "" );
		QDomDocument doc;
		QDomElement root = doc.createElement("note");
		XMLNode node( root );

		auto pDrumkit = std::make_shared<Drumkit>();
		auto pInstruments = std::make_shared<InstrumentList>();
		auto pSnare = std::make_shared<Instrument>( 1, "Snare", nullptr );
		pInstruments->add( pSnare );
		pDrumkit->setInstruments( pInstruments );

		auto pIn = std::make_shared<Note>( pSnare, 0, 1.0f, 0.5f, 1, 1.0f );
		pIn->set_probability( 0.67f );
		pIn->save_to( node );

		auto pOut = Note::load_from( node );
		pOut->mapTo( pDrumkit );

		CPPUNIT_ASSERT( pIn->get_instrument() == pOut->get_instrument() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_position(), pOut->get_position() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_velocity(), pOut->get_velocity() );
		CPPUNIT_ASSERT_EQUAL( pIn->getPan(), pOut->getPan() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_length(), pOut->get_length() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_pitch(), pOut->get_pitch() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_probability(), pOut->get_probability() );

		___INFOLOG( "passed" );
	}
};

