#include <cppunit/TestCase.h>

#include "hydrogen/note.h"
#include "hydrogen/instrument.h"

using namespace H2Core;

class NoteTest : public CppUnit::TestCase
{
public:
	NoteTest()
			: CppUnit::TestCase( "NoteTest" )
	{}

	void runTest()
	{
		printf("Inizio test per NoteTest\n");

		Instrument* instr = new Instrument(
				"dummy instrument",
				"dummy instrument",
				new ADSR()
		);

		uint position = 0;
		float vel = 1.0;
		float pan_l = 0.3;
		float pan_r = 0.4;
		int lenght = 1;
		float pitch = 2;

		Note* n = new Note(
				instr,
				position,
				vel,
				pan_l,
				pan_r,
				lenght,
				pitch
		);

		CPPUNIT_ASSERT(n->get_instrument() == instr);
		CPPUNIT_ASSERT(n->get_position() == position);
		CPPUNIT_ASSERT(n->get_velocity() == vel);

		CPPUNIT_ASSERT(n->get_pan_L() == pan_l);
		CPPUNIT_ASSERT(n->get_pan_R() == pan_r);
		CPPUNIT_ASSERT(n->get_lenght() == lenght);
		CPPUNIT_ASSERT(n->get_pitch() == pitch);

		delete n;

		printf("Test completato\n");
	}
};
