#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>

using namespace H2Core;

class InstrumentListTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( InstrumentListTest );
	CPPUNIT_TEST( test_one_instrument );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST( test3 );
	CPPUNIT_TEST( test4 );
	CPPUNIT_TEST_SUITE_END();
	
	public:
	void test_one_instrument()
	{
		InstrumentList list;
		Instrument *kick = new Instrument(EMPTY_INSTR_ID, "Kick");
		kick->set_midi_out_note(42);
		list.add(kick);

		CPPUNIT_ASSERT( !list.has_all_midi_notes_same() );
	}


	void test1()
	{
		InstrumentList list;

		Instrument *kick = new Instrument(EMPTY_INSTR_ID, "Kick");
		kick->set_midi_out_note(10);
		list.add(kick);

		Instrument *snare = new Instrument(EMPTY_INSTR_ID, "Snare");
		snare->set_midi_out_note(10);
		list.add(snare);

		Instrument *hh = new Instrument(EMPTY_INSTR_ID, "HiHat");
		hh->set_midi_out_note(10);
		list.add(hh);

		CPPUNIT_ASSERT_EQUAL( 3, list.size() );
		CPPUNIT_ASSERT( list.has_all_midi_notes_same() );
	}


	void test2()
	{
		InstrumentList list;

		Instrument *kick = new Instrument(EMPTY_INSTR_ID, "Kick");
		kick->set_midi_out_note(36);
		list.add(kick);

		Instrument *clap = new Instrument(EMPTY_INSTR_ID, "Clap");
		clap->set_midi_out_note(37);
		list.add(clap);

		Instrument *ride = new Instrument(EMPTY_INSTR_ID, "Ride");
		ride->set_midi_out_note(38);
		list.add(ride);

		Instrument *dummy = new Instrument(EMPTY_INSTR_ID, "Dummy Instrument");
		dummy->set_midi_out_note(36); // duplicate
		list.add(dummy);

		CPPUNIT_ASSERT_EQUAL( 4, list.size() );
		CPPUNIT_ASSERT( !list.has_all_midi_notes_same() );
	}


	void test3()
	{
		InstrumentList list;

		list.add( new Instrument() );
		list.add( new Instrument() );
		list.add( new Instrument() );

		list.set_default_midi_out_notes();

		CPPUNIT_ASSERT_EQUAL( 36, list.get(0)->get_midi_out_note() );
		CPPUNIT_ASSERT_EQUAL( 37, list.get(1)->get_midi_out_note() );
		CPPUNIT_ASSERT_EQUAL( 38, list.get(2)->get_midi_out_note() );
	}
	
	//test is_valid_index
	void test4()
	{
		InstrumentList list;

		list.add( new Instrument() );
		
		CPPUNIT_ASSERT( list.is_valid_index(0) );
		CPPUNIT_ASSERT( !list.is_valid_index(1) );
		CPPUNIT_ASSERT( !list.is_valid_index(-42) );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( InstrumentListTest );
