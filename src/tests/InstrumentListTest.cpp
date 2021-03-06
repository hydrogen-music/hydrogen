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
		Instrument *pKick = new Instrument(EMPTY_INSTR_ID, "Kick");
		pKick->set_midi_out_note(42);
		list.add(pKick);

		CPPUNIT_ASSERT( !list.has_all_midi_notes_same() );
	}


	void test1()
	{
		InstrumentList list;

		Instrument *pKick = new Instrument(EMPTY_INSTR_ID, "Kick");
		pKick->set_midi_out_note(10);
		list.add(pKick);

		Instrument *pSnare = new Instrument(EMPTY_INSTR_ID, "Snare");
		pSnare->set_midi_out_note(10);
		list.add(pSnare);

		Instrument *pHihat = new Instrument(EMPTY_INSTR_ID, "HiHat");
		pHihat->set_midi_out_note(10);
		list.add(pHihat);

		CPPUNIT_ASSERT_EQUAL( 3, list.size() );
		CPPUNIT_ASSERT( list.has_all_midi_notes_same() );
	}


	void test2()
	{
		InstrumentList list;

		Instrument *pKick = new Instrument(EMPTY_INSTR_ID, "Kick");
		pKick->set_midi_out_note(36);
		list.add(pKick);

		Instrument *pClap = new Instrument(EMPTY_INSTR_ID, "Clap");
		pClap->set_midi_out_note(37);
		list.add(pClap);

		Instrument *pRide = new Instrument(EMPTY_INSTR_ID, "Ride");
		pRide->set_midi_out_note(38);
		list.add(pRide);

		Instrument *pDummy = new Instrument(EMPTY_INSTR_ID, "Dummy Instrument");
		pDummy->set_midi_out_note(36); // duplicate
		list.add(pDummy);

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
