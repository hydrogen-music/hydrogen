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
		Note n(nullptr, 0, 1.0f, 0.f, 1, 1.0f);
		n.set_probability(0.75f);
		CPPUNIT_ASSERT_EQUAL(0.75f, n.get_probability());

		Note other(&n, nullptr);
		CPPUNIT_ASSERT_EQUAL(0.75f, other.get_probability());
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

		Note* pIn = new Note( pSnare, 0, 1.0f, 0.5f, 1, 1.0f );
		pIn->set_probability( 0.67f );
		pIn->save_to( node );

		Note* pOut = Note::load_from( node );
		pOut->mapTo( pDrumkit );

		CPPUNIT_ASSERT( pIn->get_instrument() == pOut->get_instrument() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_position(), pOut->get_position() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_velocity(), pOut->get_velocity() );
		CPPUNIT_ASSERT_EQUAL( pIn->getPan(), pOut->getPan() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_length(), pOut->get_length() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_pitch(), pOut->get_pitch() );
		CPPUNIT_ASSERT_EQUAL( pIn->get_probability(), pOut->get_probability() );

		delete pIn;
		delete pOut;
	___INFOLOG( "passed" );
	}
};

