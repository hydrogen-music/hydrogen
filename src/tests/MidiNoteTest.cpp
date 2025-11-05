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
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

void MidiNoteTest::testDefaultValues() {
	___INFOLOG( "" );
	CPPUNIT_ASSERT( ( OCTAVE_DEFAULT + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE ==
					MidiMessage::nInstrumentOffset );
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

void MidiNoteTest::testMidiInstrumentInputMapping() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	const auto pOldSong = pHydrogen->getSong();
	const auto pOldPreferences = Preferences::get_instance();

	auto pNewPreferences = CoreActionController::loadPreferences(
		H2TEST_FILE( "preferences/midi-instrument-mapping.conf" ) );
	CPPUNIT_ASSERT( pNewPreferences != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setPreferences( pNewPreferences ) );
	const auto pNewSong = Song::getEmptySong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setSong( pNewSong ) );

	const auto pNewDrumkit = Drumkit::load(
		H2TEST_FILE( "drumkits/midi-instrument-mapping" ),
		/* bUpgrade */true, /* pLegacy */nullptr, /*bSilent*/false );
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pNewDrumkit ) );

	auto pPref = Preferences::get_instance();
	auto pMidiInstrumentMap = pPref->getMidiInstrumentMap();
	auto pInstrumentList = pNewDrumkit->getInstruments();

	////////////////////////////////////////////////////////////////////////////
	// Input mapping

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::None );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		CPPUNIT_ASSERT( pMidiInstrumentMap->getInputMapping(
			ppInstrument, pNewDrumkit ).isNull() );
	}
	for ( int nnNote = MidiMessage::nNoteMinimum;
		 nnNote <= MidiMessage::nNoteMaximum; ++nnNote ) {
		for ( int nnChannel = MidiMessage::nChannelOff;
			 nnChannel <= MidiMessage::nChannelMaximum; ++nnChannel ) {
			CPPUNIT_ASSERT( pMidiInstrumentMap->mapInput(
				nnNote, nnChannel, pNewDrumkit ).size() == 0 );
		}
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::AsOutput );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef = pMidiInstrumentMap->getInputMapping(
			ppInstrument, pNewDrumkit );
		CPPUNIT_ASSERT( noteRef.nNote == ppInstrument->getMidiOutNote() );
		CPPUNIT_ASSERT( noteRef.nChannel == ppInstrument->getMidiOutChannel() );
	}
	{
		int nMappedInstruments = 0;
		for ( int nnNote = MidiMessage::nNoteMinimum;
			 nnNote <= MidiMessage::nNoteMaximum; ++nnNote ) {
			for ( int nnChannel = MidiMessage::nChannelOff;
			 	nnChannel <= MidiMessage::nChannelMaximum; ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					nnNote, nnChannel, pNewDrumkit );
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[ 0 ] != nullptr );
					CPPUNIT_ASSERT( mapped[ 0 ]->getMidiOutNote() == nnNote );
					CPPUNIT_ASSERT( mapped[ 0 ]->getMidiOutChannel() == nnChannel );
					++nMappedInstruments;
				}
			}
		}
		CPPUNIT_ASSERT( nMappedInstruments == pInstrumentList->size() );
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::Custom );
	const auto customMappings = pMidiInstrumentMap->getCustomInputMappingsType();
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef = pMidiInstrumentMap->getInputMapping(
			ppInstrument, pNewDrumkit );
		CPPUNIT_ASSERT( customMappings.find( ppInstrument->getType() ) !=
						customMappings.end() );
		const auto customNoteRef = customMappings.at( ppInstrument->getType() );
		CPPUNIT_ASSERT( noteRef.nNote == customNoteRef.nNote );
		CPPUNIT_ASSERT( noteRef.nChannel == customNoteRef.nChannel );
	}
	{
		int nMappedInstruments = 0;
		for ( int nnNote = MidiMessage::nNoteMinimum;
			 nnNote <= MidiMessage::nNoteMaximum; ++nnNote ) {
			for ( int nnChannel = MidiMessage::nChannelOff;
			 	nnChannel <= MidiMessage::nChannelMaximum; ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					nnNote, nnChannel, pNewDrumkit );
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[ 0 ] != nullptr );
					MidiInstrumentMap::NoteRef noteRef;
					noteRef.nNote = nnNote;
					noteRef.nChannel = nnChannel;

					bool bInCustomMap = false;
					for ( const auto [ _, nnoteRef ] : customMappings ) {
						if ( nnoteRef.nNote == noteRef.nNote &&
							 nnoteRef.nChannel == noteRef.nChannel ) {
							bInCustomMap = true;
						}
					}
					CPPUNIT_ASSERT( bInCustomMap );
					++nMappedInstruments;
				}
			}
		}
		CPPUNIT_ASSERT( nMappedInstruments == pInstrumentList->size() );
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::SelectedInstrument );
	pHydrogen->setSelectedInstrumentNumber( 1 );
	const auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	CPPUNIT_ASSERT( pSelectedInstrument != nullptr );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef = pMidiInstrumentMap->getInputMapping(
			ppInstrument, pNewDrumkit );
		if ( ppInstrument == pSelectedInstrument ) {
			CPPUNIT_ASSERT( noteRef.nNote == ppInstrument->getMidiOutNote() );
			CPPUNIT_ASSERT( noteRef.nChannel == ppInstrument->getMidiOutChannel() );
		}
		else {
			CPPUNIT_ASSERT( noteRef.isNull() );
		}
	}
	{
		for ( int nnNote = MidiMessage::nNoteMinimum;
			 nnNote <= MidiMessage::nNoteMaximum; ++nnNote ) {
			for ( int nnChannel = MidiMessage::nChannelOff;
			 	nnChannel <= MidiMessage::nChannelMaximum; ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					nnNote, nnChannel, pNewDrumkit );
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[ 0 ] != nullptr );
					CPPUNIT_ASSERT( mapped[ 0 ] == pSelectedInstrument );
					CPPUNIT_ASSERT( pSelectedInstrument->getMidiOutChannel() ==
									nnChannel );
				}
			}
		}
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::Order );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef = pMidiInstrumentMap->getInputMapping(
			ppInstrument, pNewDrumkit );
		CPPUNIT_ASSERT( noteRef.nNote ==
						pInstrumentList->index( ppInstrument ) +
						MidiMessage::nInstrumentOffset );
		CPPUNIT_ASSERT( noteRef.nChannel == ppInstrument->getMidiOutChannel() );
	}
	{
		int nMappedInstruments = 0;
		for ( int nnNote = MidiMessage::nNoteMinimum;
			 nnNote <= MidiMessage::nNoteMaximum; ++nnNote ) {
			for ( int nnChannel = MidiMessage::nChannelOff;
			 	nnChannel <= MidiMessage::nChannelMaximum; ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					nnNote, nnChannel, pNewDrumkit );
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[ 0 ] != nullptr );
					CPPUNIT_ASSERT(
						mapped[ 0 ] ==
						pInstrumentList->get( nnNote - MidiMessage::nInstrumentOffset ) );
					++nMappedInstruments;
				}
			}
		}
		CPPUNIT_ASSERT( nMappedInstruments == pInstrumentList->size() );
	}

	////////////////////////////////////////////////////////////////////////////

	CoreActionController::setPreferences( pOldPreferences );
	CoreActionController::setSong( pOldSong );

	___INFOLOG( "passed" );
}

void MidiNoteTest::testMidiInstrumentOutputMapping() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	const auto pOldSong = pHydrogen->getSong();
	const auto pOldPreferences = Preferences::get_instance();

	auto pNewPreferences = CoreActionController::loadPreferences(
		H2TEST_FILE( "preferences/midi-instrument-mapping.conf" ) );
	CPPUNIT_ASSERT( pNewPreferences != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setPreferences( pNewPreferences ) );
	const auto pNewSong = Song::getEmptySong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setSong( pNewSong ) );

	const auto pNewDrumkit = Drumkit::load(
		H2TEST_FILE( "drumkits/midi-instrument-mapping" ),
		/* bUpgrade */true, /* pLegacy */nullptr, /*bSilent*/false );
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pNewDrumkit ) );

	auto pPref = Preferences::get_instance();
	auto pMidiInstrumentMap = pPref->getMidiInstrumentMap();
	auto pInstrumentList = pNewDrumkit->getInstruments();

	////////////////////////////////////////////////////////////////////////////
	// Output mapping

	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::None );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		CPPUNIT_ASSERT( pMidiInstrumentMap->getOutputMapping(
			nullptr, ppInstrument ).isNull() );
	}

	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::Constant );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef = pMidiInstrumentMap->getOutputMapping(
			nullptr, ppInstrument );
		CPPUNIT_ASSERT( noteRef.nNote == ppInstrument->getMidiOutNote() );
		CPPUNIT_ASSERT( noteRef.nChannel == ppInstrument->getMidiOutChannel() );
	}

	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::Offset );
	std::vector< std::shared_ptr<Note> > notes;
	notes.push_back( std::make_shared<Note>() );
	auto pNoteLow = std::make_shared<Note>();
	pNoteLow->setKeyOctave( Note::Key::D, Note::Octave::P8Y );
	notes.push_back( pNoteLow );
	auto pNoteHigh = std::make_shared<Note>();
	pNoteHigh->setKeyOctave( Note::Key::F, Note::Octave::P8A );
	notes.push_back( pNoteHigh );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		for ( auto& ppNote : notes ) {
			ppNote->mapToInstrument( ppInstrument );
			const auto noteRef = pMidiInstrumentMap->getOutputMapping(
				ppNote );
			CPPUNIT_ASSERT( noteRef.nNote ==
							ppInstrument->getMidiOutNote() +
							ppNote->getPitchFromKeyOctave() );
			CPPUNIT_ASSERT( noteRef.nChannel == ppInstrument->getMidiOutChannel() );
		}
	}

	////////////////////////////////////////////////////////////////////////////

	CoreActionController::setPreferences( pOldPreferences );
	CoreActionController::setSong( pOldSong );

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
