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

#include "InstrumentListTest.h"

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>

using namespace H2Core;

void InstrumentListTest::testDefaultMidiOutNotes()
{
	___INFOLOG( "" );
	InstrumentList list;

	list.add( std::make_shared<Instrument>() );
	list.add( std::make_shared<Instrument>() );
	list.add( std::make_shared<Instrument>() );

	list.setDefaultMidiOutNotes();

	CPPUNIT_ASSERT_EQUAL( 36, list.get( 0 )->getMidiOutNote() );
	CPPUNIT_ASSERT_EQUAL( 37, list.get( 1 )->getMidiOutNote() );
	CPPUNIT_ASSERT_EQUAL( 38, list.get( 2 )->getMidiOutNote() );
	___INFOLOG( "passed" );
}

void InstrumentListTest::testHasAllMidiNotesSame()
{
	___INFOLOG( "" );
	// One instrument
	{
		InstrumentList list;
		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote( 42 );
		list.add( pKick );

		CPPUNIT_ASSERT( !list.hasAllMidiNotesSame() );
	}

	// All the same
	{
		InstrumentList list;

		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote( 10 );
		list.add( pKick );

		auto pSnare = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Snare" );
		pSnare->setMidiOutNote( 10 );
		list.add( pSnare );

		auto pHihat = std::make_shared<Instrument>( EMPTY_INSTR_ID, "HiHat" );
		pHihat->setMidiOutNote( 10 );
		list.add( pHihat );

		CPPUNIT_ASSERT_EQUAL( 3, list.size() );
		CPPUNIT_ASSERT( list.hasAllMidiNotesSame() );
	}

	// All different
	{
		InstrumentList list;

		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote( 36 );
		list.add( pKick );

		auto pClap = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Clap" );
		pClap->setMidiOutNote( 37 );
		list.add( pClap );

		auto pRide = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Ride" );
		pRide->setMidiOutNote( 38 );
		list.add( pRide );

		auto pDummy =
			std::make_shared<Instrument>( EMPTY_INSTR_ID, "Dummy Instrument" );
		pDummy->setMidiOutNote( 36 );  // duplicate
		list.add( pDummy );

		CPPUNIT_ASSERT_EQUAL( 4, list.size() );
		CPPUNIT_ASSERT( !list.hasAllMidiNotesSame() );
	}
	___INFOLOG( "passed" );
}

void InstrumentListTest::testIsValidIndex()
{
	___INFOLOG( "" );
	InstrumentList list;

	list.add( std::make_shared<Instrument>() );

	CPPUNIT_ASSERT( list.isValidIndex( 0 ) );
	CPPUNIT_ASSERT( !list.isValidIndex( 1 ) );
	CPPUNIT_ASSERT( !list.isValidIndex( -42 ) );
	___INFOLOG( "passed" );
}
