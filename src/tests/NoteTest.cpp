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
		Note n(nullptr, 0, 1.0f, 0.5f, 0.5f, 1, 1.0f);
		n.set_probability(0.75f);
		CPPUNIT_ASSERT_EQUAL(0.75f, n.get_probability());

		Note other(&n, nullptr);
		CPPUNIT_ASSERT_EQUAL(0.75f, other.get_probability());
	}

	void testSerializeProbability()
	{
		QDomDocument doc;
		QDomElement root = doc.createElement("note");
		XMLNode node(root);

		InstrumentList *instruments = new InstrumentList();
		Instrument *snare = new Instrument( 1, "Snare", nullptr );
		instruments->add( snare );

		Note *in = new Note(snare, 0, 1.0f, 0.5f, 0.5f, 1, 1.0f);
		in->set_probability(0.67f);
		in->save_to(&node);

		Note *out = Note::load_from(&node, instruments);

		CPPUNIT_ASSERT(in->get_instrument() == out->get_instrument());
		CPPUNIT_ASSERT_EQUAL(in->get_position(), out->get_position());
		CPPUNIT_ASSERT_EQUAL(in->get_velocity(), out->get_velocity());
		CPPUNIT_ASSERT_EQUAL(in->get_pan_l(), out->get_pan_l());
		CPPUNIT_ASSERT_EQUAL(in->get_pan_r(), out->get_pan_r());
		CPPUNIT_ASSERT_EQUAL(in->get_length(), out->get_length());
		CPPUNIT_ASSERT_EQUAL(in->get_pitch(), out->get_pitch());
		CPPUNIT_ASSERT_EQUAL(in->get_probability(), out->get_probability());

		/*
		FIXME: this causes double free
		delete in;
		delete out;
		delete instruments;
		delete snare;
		*/
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( NoteTest );
