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
#include <core/Basics/Note.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Helpers/Xml.h>
#include <QDomDocument>

using namespace H2Core;

class NoteTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( NoteTest );
	CPPUNIT_TEST( testProbability );
	CPPUNIT_TEST( testSerializeProbability );
	CPPUNIT_TEST_SUITE_END();

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
		XMLNode node(root);

		auto instruments = std::make_shared<InstrumentList>();
		auto snare = std::make_shared<Instrument>( 1, "Snare", nullptr );
		instruments->add( snare );

		Note *in = new Note(snare, 0, 1.0f, 0.5f, 1, 1.0f);
		in->set_probability(0.67f);
		in->save_to(&node);

		Note *out = Note::load_from(&node, instruments);

		CPPUNIT_ASSERT(in->get_instrument() == out->get_instrument());
		CPPUNIT_ASSERT_EQUAL(in->get_position(), out->get_position());
		CPPUNIT_ASSERT_EQUAL(in->get_velocity(), out->get_velocity());
		CPPUNIT_ASSERT_EQUAL( in->getPan(), out->getPan() );
		CPPUNIT_ASSERT_EQUAL(in->get_length(), out->get_length());
		CPPUNIT_ASSERT_EQUAL(in->get_pitch(), out->get_pitch());
		CPPUNIT_ASSERT_EQUAL(in->get_probability(), out->get_probability());

		delete in;
		delete out;
	___INFOLOG( "passed" );
	}
};

