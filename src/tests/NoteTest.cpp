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

#include "NoteTest.h"

#include "TestHelper.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/IO/MidiCommon.h>
#include <core/Preferences/Shortcuts.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <QDomDocument>

#include <algorithm>

using namespace H2Core;

void NoteTest::testComparison() {
	___INFOLOG( "" );

	// Setup example vectors.
	std::vector< std::shared_ptr<H2Core::Note> > notes1, notes2, notes3;
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 283 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 0 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 22 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 284 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 283 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 282 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 130 ) );
	notes1.push_back( std::make_shared<H2Core::Note>( nullptr, 803 ) );

	for ( const auto& ppNote : notes1 ) {
		notes2.push_back( std::make_shared<H2Core::Note>( ppNote ) );

		auto pNoteStart = std::make_shared<H2Core::Note>( ppNote );
		pNoteStart->computeNoteStart();
		notes3.push_back( pNoteStart );
	}

	// Descending order based on position
	std::sort( notes1.begin(), notes1.end(), Note::compare );
	for ( int ii = 1; ii < notes1.size(); ++ii ) {
		CPPUNIT_ASSERT( notes1[ ii - 1 ]->getPosition() >=
						notes1[ ii ]->getPosition() );
	}

	// Ascending order based on position
	std::sort( notes2.begin(), notes2.end(), Note::compareAscending );
	for ( int ii = 1; ii < notes2.size(); ++ii ) {
		CPPUNIT_ASSERT( notes2[ ii - 1 ]->getPosition() <=
						notes2[ ii ]->getPosition() );
	}

	// Descending based on note start
	std::sort( notes3.begin(), notes3.end(), Note::compareStart );
	for ( int ii = 1; ii < notes3.size(); ++ii ) {
		CPPUNIT_ASSERT( notes3[ ii - 1 ]->getPosition() >=
						notes3[ ii ]->getPosition() );
		CPPUNIT_ASSERT( notes3[ ii - 1 ]->getNoteStart() >=
						notes3[ ii ]->getNoteStart() );
	}

	// Full size example
	auto pSong = Song::load(
		H2TEST_FILE( "song/AE_noteEnqueuingTimeline.h2song" ) );
	CPPUNIT_ASSERT( pSong != nullptr );

	const auto songNotes = pSong->getAllNotes();
	for ( int ii = 1; ii < songNotes.size(); ++ii ) {
		CPPUNIT_ASSERT( songNotes[ ii - 1 ]->getPosition() <=
						songNotes[ ii ]->getPosition() );
	}

	___INFOLOG( "passed" );
}

void NoteTest::testMappingValidDrumkits() {
	___INFOLOG( "" );

	const auto pDB = Hydrogen::get_instance()->getSoundLibraryDatabase();
	auto pDrumkit = pDB->getDrumkit( "GMRockKit" );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( ! pDrumkit->hasMissingTypes() );
	auto pDrumkitOther = pDB->getDrumkit( "TR808EmulationKit" );
	CPPUNIT_ASSERT( pDrumkitOther != nullptr );
	CPPUNIT_ASSERT( ! pDrumkitOther->hasMissingTypes() );

	auto pPatternMatchingTypes = Pattern::load(
		H2TEST_FILE( "pattern/noteTestMatchingTypes.h2pattern" ) );
	CPPUNIT_ASSERT( pPatternMatchingTypes != nullptr );
	auto pPatternTypeMisses = Pattern::load(
		H2TEST_FILE( "pattern/noteTestTypeMisses.h2pattern" ) );
	CPPUNIT_ASSERT( pPatternTypeMisses != nullptr );

	// Verify the patterns were created for the primary drumkit.
	auto pDrumkitMap = pDrumkit->toDrumkitMap();
	for ( const auto& [ _, ppNote ] : *pPatternMatchingTypes->getNotes() ) {
		CPPUNIT_ASSERT( ppNote != nullptr );
		CPPUNIT_ASSERT( ! ppNote->getType().isEmpty() );
		CPPUNIT_ASSERT( ppNote->getInstrumentId() != EMPTY_INSTR_ID );
		CPPUNIT_ASSERT( ppNote->getType() ==
						pDrumkitMap->getType( ppNote->getInstrumentId() ) );
	}
	for ( const auto& [ _, ppNote ] : *pPatternTypeMisses->getNotes() ) {
		CPPUNIT_ASSERT( ppNote != nullptr );
		CPPUNIT_ASSERT( ! ppNote->getType().isEmpty() );
		CPPUNIT_ASSERT( ppNote->getInstrumentId() != EMPTY_INSTR_ID );
		CPPUNIT_ASSERT( ppNote->getType() ==
						pDrumkitMap->getType( ppNote->getInstrumentId() ) );
	}

	// Using a pattern containing only notes holding instrument types present in
	// both kits.
	//
	// We expect all notes to be mapped to valid IDs.
	auto pPatternDeepCopy = std::make_shared<Pattern>(pPatternMatchingTypes);
	pPatternDeepCopy->mapTo( pDrumkitOther, pDrumkit );

	// We store the notes in a vector for a better comparison.
	std::vector< std::shared_ptr<Note> > notesOrig, notesMapped;
	for ( const auto& [ _, ppNote ] : *pPatternMatchingTypes->getNotes() ) {
		notesOrig.push_back( ppNote );
	}
	for ( const auto& [ _, ppNote ] : *pPatternDeepCopy->getNotes() ) {
		notesMapped.push_back( ppNote );
	}

	bool bIdMismatch = false;
	for ( int ii = 0; ii < notesMapped.size(); ++ii ) {
		CPPUNIT_ASSERT( notesOrig[ ii ]->getType() ==
						notesMapped[ ii ]->getType() );
		CPPUNIT_ASSERT( notesMapped[ ii ]->getInstrumentId() != EMPTY_INSTR_ID );
		if ( notesOrig[ ii ]->getInstrumentId() !=
			 notesMapped[ ii ]->getInstrumentId() ) {
			bIdMismatch = true;
		}
	}
	CPPUNIT_ASSERT( bIdMismatch );

	// Map them back must yield the same notes we started from.
	pPatternDeepCopy->mapTo( pDrumkit, pDrumkitOther );

	notesMapped.clear();
	for ( const auto& [ _, ppNote ] : *pPatternDeepCopy->getNotes() ) {
		notesMapped.push_back( ppNote );
	}

	for ( int ii = 0; ii < notesMapped.size(); ++ii ) {
		CPPUNIT_ASSERT( notesOrig[ ii ]->getType() ==
						notesMapped[ ii ]->getType() );
		CPPUNIT_ASSERT( notesOrig[ ii ]->getInstrumentId() ==
						notesMapped[ ii ]->getInstrumentId() );
	}

	// Next we will use a pattern with notes holding types only present in the
	// primary drumkit.
	//
	// We expect some notes to be mapped to an  invalid ID (-1 / EMPTY).
	pPatternDeepCopy = std::make_shared<Pattern>(pPatternTypeMisses);
	pPatternDeepCopy->mapTo( pDrumkitOther, pDrumkit );

	// We store the notes in a vector for a better comparison.
	notesOrig.clear();
	notesMapped.clear();
	for ( const auto& [ _, ppNote ] : *pPatternTypeMisses->getNotes() ) {
		notesOrig.push_back( ppNote );
	}
	for ( const auto& [ _, ppNote ] : *pPatternDeepCopy->getNotes() ) {
		notesMapped.push_back( ppNote );
	}

	bool bInvalidId = false;
	for ( int ii = 0; ii < notesMapped.size(); ++ii ) {
		CPPUNIT_ASSERT( notesOrig[ ii ]->getType() ==
						notesMapped[ ii ]->getType() );
		if ( notesMapped[ ii ]->getInstrumentId() == EMPTY_INSTR_ID ) {
			bInvalidId = true;
		}
	}
	CPPUNIT_ASSERT( bInvalidId );

	// Map them back must yield the same notes we started from.
	pPatternDeepCopy->mapTo( pDrumkit, pDrumkitOther );

	notesMapped.clear();
	for ( const auto& [ _, ppNote ] : *pPatternDeepCopy->getNotes() ) {
		notesMapped.push_back( ppNote );
	}

	for ( int ii = 0; ii < notesMapped.size(); ++ii ) {
		CPPUNIT_ASSERT( notesOrig[ ii ]->getType() ==
						notesMapped[ ii ]->getType() );
		CPPUNIT_ASSERT( notesOrig[ ii ]->getInstrumentId() ==
						notesMapped[ ii ]->getInstrumentId() );
	}

	___INFOLOG( "passed" );
}

void NoteTest::testMidiDefaultOffset() {
	___INFOLOG( "" );
	CPPUNIT_ASSERT_EQUAL( MidiMessage::instrumentOffset, KEYS_PER_OCTAVE *
						  ( OCTAVE_DEFAULT + OCTAVE_OFFSET ) );
	___INFOLOG( "passed" );
}

void NoteTest::testPitchConversions() {
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
			pNote->setKeyOctave( static_cast<H2Core::Note::Key>(kkey),
								 static_cast<H2Core::Note::Octave>(ooctave) );

			const float fPitch = pNote->getPitchFromKeyOctave();
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

void NoteTest::testProbability() {
	___INFOLOG( "" );

	auto pNote = std::make_shared<Note>( nullptr, 0, 1.0f, 0.f, 1, 1.0f );
	pNote->setProbability(0.75f);
	CPPUNIT_ASSERT_EQUAL(0.75f, pNote->getProbability());

	auto pOther = std::make_shared<Note>( pNote, nullptr );
	CPPUNIT_ASSERT_EQUAL(0.75f, pOther->getProbability());

	___INFOLOG( "passed" );
}

void NoteTest::testSerializeProbability() {
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
	pIn->setProbability( 0.67f );
	pIn->saveTo( node );

	auto pOut = Note::loadFrom( node );
	pOut->mapTo( pDrumkit );

	CPPUNIT_ASSERT( pIn->getInstrument() == pOut->getInstrument() );
	CPPUNIT_ASSERT_EQUAL( pIn->getPosition(), pOut->getPosition() );
	CPPUNIT_ASSERT_EQUAL( pIn->getVelocity(), pOut->getVelocity() );
	CPPUNIT_ASSERT_EQUAL( pIn->getPan(), pOut->getPan() );
	CPPUNIT_ASSERT_EQUAL( pIn->getLength(), pOut->getLength() );
	CPPUNIT_ASSERT_EQUAL( pIn->getPitch(), pOut->getPitch() );
	CPPUNIT_ASSERT_EQUAL( pIn->getProbability(), pOut->getProbability() );

	___INFOLOG( "passed" );
}

void NoteTest::testVirtualKeyboard() {
	___INFOLOG( "" );
	CPPUNIT_ASSERT_EQUAL( static_cast<int>(Shortcuts::Action::VK_36_C2), 400 );
	CPPUNIT_ASSERT_EQUAL( MidiMessage::instrumentOffset, 36 ); // MIDI note C2
	___INFOLOG( "passed" );
}
