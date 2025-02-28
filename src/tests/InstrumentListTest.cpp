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
	___INFOLOG( "" );
		InstrumentList list;
		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote(42);
		list.add(pKick);

		CPPUNIT_ASSERT( !list.has_all_midi_notes_same() );
	___INFOLOG( "passed" );
	}


	void test1()
	{
	___INFOLOG( "" );
		InstrumentList list;

		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote(10);
		list.add(pKick);

		auto pSnare = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Snare" );
		pSnare->setMidiOutNote(10);
		list.add(pSnare);

		auto pHihat = std::make_shared<Instrument>( EMPTY_INSTR_ID, "HiHat" );
		pHihat->setMidiOutNote(10);
		list.add(pHihat);

		CPPUNIT_ASSERT_EQUAL( 3, list.size() );
		CPPUNIT_ASSERT( list.has_all_midi_notes_same() );
	___INFOLOG( "passed" );
	}


	void test2()
	{
	___INFOLOG( "" );
		InstrumentList list;

		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote(36);
		list.add(pKick);

		auto pClap = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Clap" );
		pClap->setMidiOutNote(37);
		list.add(pClap);

		auto pRide = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Ride" );
		pRide->setMidiOutNote(38);
		list.add(pRide);

		auto pDummy = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Dummy Instrument" );
		pDummy->setMidiOutNote(36); // duplicate
		list.add(pDummy);

		CPPUNIT_ASSERT_EQUAL( 4, list.size() );
		CPPUNIT_ASSERT( !list.has_all_midi_notes_same() );
	___INFOLOG( "passed" );
	}


	void test3()
	{
	___INFOLOG( "" );
		InstrumentList list;

		list.add( std::make_shared<Instrument>() );
		list.add( std::make_shared<Instrument>() );
		list.add( std::make_shared<Instrument>() );

		list.set_default_midi_out_notes();

		CPPUNIT_ASSERT_EQUAL( 36, list.get(0)->getMidiOutNote() );
		CPPUNIT_ASSERT_EQUAL( 37, list.get(1)->getMidiOutNote() );
		CPPUNIT_ASSERT_EQUAL( 38, list.get(2)->getMidiOutNote() );
	___INFOLOG( "passed" );
	}
	
	//test is_valid_index
	void test4()
	{
	___INFOLOG( "" );
		InstrumentList list;

		list.add( std::make_shared<Instrument>() );
		
		CPPUNIT_ASSERT( list.is_valid_index(0) );
		CPPUNIT_ASSERT( !list.is_valid_index(1) );
		CPPUNIT_ASSERT( !list.is_valid_index(-42) );
	___INFOLOG( "passed" );
	}
};

