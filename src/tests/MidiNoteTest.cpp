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

#include "MidiNoteTest.h"

#include "TestHelper.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiMessage.h>

using namespace H2Core;

void MidiNoteTest::testDefaultValues() {
	___INFOLOG( "" );
	CPPUNIT_ASSERT( ( OCTAVE_DEFAULT + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE ==
					MidiMessage::instrumentOffset );
	___INFOLOG( "passed" );
}

void MidiNoteTest::testLoadNewSong() {
	___INFOLOG( "" );
	/* Read song with drumkit that have assigned distinct MIDI notes. Check
	 * that loading that song does not change that mapping */

	auto pSong = H2Core::Song::load( H2TEST_FILE( "song/legacy/test_song_0.9.7.h2song" ) );
	CPPUNIT_ASSERT( pSong != nullptr );

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstrumentList != nullptr );
	CPPUNIT_ASSERT_EQUAL( 4, pInstrumentList->size() );

	checkInstrumentMidiNote( "Kick",       35, pInstrumentList->get(0) );
	checkInstrumentMidiNote( "Snare Rock", 40, pInstrumentList->get(1) );
	checkInstrumentMidiNote( "Crash",      49, pInstrumentList->get(2) );
	checkInstrumentMidiNote( "Ride Rock",  59, pInstrumentList->get(3) );
	___INFOLOG( "passed" );
}

void MidiNoteTest::checkInstrumentMidiNote( const QString& sName, int nNote,
											std::shared_ptr<Instrument> pInstrument ) {
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const auto sInstrumentName = pInstrument->getName();
	const auto nInstrumentNote = pInstrument->getMidiOutNote();

	// Bad index / setup.
	CPPUNIT_ASSERT( pInstrument->getName() == sName );
	CPPUNIT_ASSERT( pInstrument->getMidiOutNote() == nNote );
}
