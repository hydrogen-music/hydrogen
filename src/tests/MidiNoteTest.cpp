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

#include <chrono>

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Helpers/Time.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>

using namespace H2Core;

void MidiNoteTest::testDefaultValues()
{
	___INFOLOG( "" );
	CPPUNIT_ASSERT(
		( static_cast<int>( Note::OctaveDefault ) + OCTAVE_OFFSET ) *
			KEYS_PER_OCTAVE ==
		static_cast<int>( Midi::NoteOffset )
	);
	___INFOLOG( "passed" );
}

void MidiNoteTest::testLoadNewSong()
{
	___INFOLOG( "" );
	/* Read song with drumkit that have assigned distinct MIDI notes. Check
	 * that loading that song does not change that mapping */

	auto pSong =
		H2Core::Song::load( H2TEST_FILE( "song/legacy/test_song_0.9.7.h2song" )
		);
	CPPUNIT_ASSERT( pSong != nullptr );

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT( pInstrumentList != nullptr );
	CPPUNIT_ASSERT_EQUAL( 4, pInstrumentList->size() );

	checkInstrumentMidiNote(
		"Kick", Midi::noteFromInt( 35 ), pInstrumentList->get( 0 )
	);
	checkInstrumentMidiNote(
		"Snare Rock", Midi::noteFromInt( 40 ), pInstrumentList->get( 1 )
	);
	checkInstrumentMidiNote(
		"Crash", Midi::noteFromInt( 49 ), pInstrumentList->get( 2 )
	);
	checkInstrumentMidiNote(
		"Ride Rock", Midi::noteFromInt( 59 ), pInstrumentList->get( 3 )
	);
	___INFOLOG( "passed" );
}

void MidiNoteTest::testMidiInstrumentInputMapping()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	auto pNewPreferences = CoreActionController::loadPreferences(
		H2TEST_FILE( "preferences/midi-instrument-mapping.conf" )
	);
	CPPUNIT_ASSERT( pNewPreferences != nullptr );

	const auto pNewDrumkit = Drumkit::load(
		H2TEST_FILE( "drumkits/midi-instrument-mapping" ),
		/* bUpgrade */ true, /* pLegacy */ nullptr, /*bSilent*/ false
	);
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pNewDrumkit ) );

	auto pMidiInstrumentMap = pNewPreferences->getMidiInstrumentMap();
	auto pInstrumentList = pNewDrumkit->getInstruments();

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::None );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		CPPUNIT_ASSERT( pMidiInstrumentMap
							->getInputMapping( ppInstrument, pNewDrumkit )
							.isNull() );
	}
	for ( int nnNote = static_cast<int>( Midi::NoteMinimum );
		  nnNote <= static_cast<int>( Midi::NoteMaximum ); ++nnNote ) {
		for ( int nnChannel = static_cast<int>( Midi::ChannelOff );
			  nnChannel <= static_cast<int>( Midi::ChannelMaximum );
			  ++nnChannel ) {
			CPPUNIT_ASSERT(
				pMidiInstrumentMap
					->mapInput(
						Midi::noteFromInt( nnNote ),
						Midi::channelFromInt( nnChannel ), pNewDrumkit
					)
					.size() == 0
			);
		}
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::AsOutput );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef =
			pMidiInstrumentMap->getInputMapping( ppInstrument, pNewDrumkit );
		CPPUNIT_ASSERT( noteRef.note == ppInstrument->getMidiOutNote() );
		CPPUNIT_ASSERT( noteRef.channel == ppInstrument->getMidiOutChannel() );
	}
	{
		int nMappedInstruments = 0;
		for ( int nnNote = static_cast<int>( Midi::NoteMinimum );
			  nnNote <= static_cast<int>( Midi::NoteMaximum ); ++nnNote ) {
			for ( int nnChannel = static_cast<int>( Midi::ChannelOff );
				  nnChannel <= static_cast<int>( Midi::ChannelMaximum );
				  ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					Midi::noteFromInt( nnNote ),
					Midi::channelFromInt( nnChannel ), pNewDrumkit
				);
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[0] != nullptr );
					CPPUNIT_ASSERT(
						mapped[0]->getMidiOutNote() ==
						Midi::noteFromInt( nnNote )
					);
					CPPUNIT_ASSERT(
						mapped[0]->getMidiOutChannel() ==
						Midi::channelFromInt( nnChannel )
					);
					++nMappedInstruments;
				}
			}
		}
		CPPUNIT_ASSERT( nMappedInstruments == pInstrumentList->size() );
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::Custom );
	const auto customMappings =
		pMidiInstrumentMap->getCustomInputMappingsType();
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef =
			pMidiInstrumentMap->getInputMapping( ppInstrument, pNewDrumkit );
		CPPUNIT_ASSERT(
			customMappings.find( ppInstrument->getType() ) !=
			customMappings.end()
		);
		const auto customNoteRef = customMappings.at( ppInstrument->getType() );
		CPPUNIT_ASSERT( noteRef.note == customNoteRef.note );
		CPPUNIT_ASSERT( noteRef.channel == customNoteRef.channel );
	}
	{
		int nMappedInstruments = 0;
		for ( int nnNote = static_cast<int>( Midi::NoteMinimum );
			  nnNote <= static_cast<int>( Midi::NoteMaximum ); ++nnNote ) {
			for ( int nnChannel = static_cast<int>( Midi::ChannelOff );
				  nnChannel <= static_cast<int>( Midi::ChannelMaximum );
				  ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					Midi::noteFromInt( nnNote ),
					Midi::channelFromInt( nnChannel ), pNewDrumkit
				);
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[0] != nullptr );
					MidiInstrumentMap::NoteRef noteRef;
					noteRef.note = Midi::noteFromInt( nnNote );
					noteRef.channel = Midi::channelFromInt( nnChannel );

					bool bInCustomMap = false;
					for ( const auto [_, nnoteRef] : customMappings ) {
						if ( nnoteRef.note == noteRef.note &&
							 nnoteRef.channel == noteRef.channel ) {
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

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::SelectedInstrument
	);
	pHydrogen->setSelectedInstrumentNumber( 1 );
	const auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	CPPUNIT_ASSERT( pSelectedInstrument != nullptr );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef =
			pMidiInstrumentMap->getInputMapping( ppInstrument, pNewDrumkit );
		if ( ppInstrument == pSelectedInstrument ) {
			CPPUNIT_ASSERT( noteRef.note == ppInstrument->getMidiOutNote() );
			CPPUNIT_ASSERT(
				noteRef.channel == ppInstrument->getMidiOutChannel()
			);
		}
		else {
			CPPUNIT_ASSERT( noteRef.isNull() );
		}
	}
	{
		for ( int nnNote = static_cast<int>( Midi::NoteMinimum );
			  nnNote <= static_cast<int>( Midi::NoteMaximum ); ++nnNote ) {
			for ( int nnChannel = static_cast<int>( Midi::ChannelOff );
				  nnChannel <= static_cast<int>( Midi::ChannelMaximum );
				  ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					Midi::noteFromInt( nnNote ),
					Midi::channelFromInt( nnChannel ), pNewDrumkit
				);
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[0] != nullptr );
					CPPUNIT_ASSERT( mapped[0] == pSelectedInstrument );
					CPPUNIT_ASSERT(
						pSelectedInstrument->getMidiOutChannel() ==
						Midi::channelFromInt( nnChannel )
					);
				}
			}
		}
	}

	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::Order );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef =
			pMidiInstrumentMap->getInputMapping( ppInstrument, pNewDrumkit );
		CPPUNIT_ASSERT(
			noteRef.note == Midi::noteFromInt(
								pInstrumentList->index( ppInstrument ) +
								static_cast<int>( Midi::NoteOffset )
							)
		);
		CPPUNIT_ASSERT( noteRef.channel == ppInstrument->getMidiOutChannel() );
	}
	{
		int nMappedInstruments = 0;
		for ( int nnNote = static_cast<int>( Midi::NoteMinimum );
			  nnNote <= static_cast<int>( Midi::NoteMaximum ); ++nnNote ) {
			for ( int nnChannel = static_cast<int>( Midi::ChannelOff );
				  nnChannel <= static_cast<int>( Midi::ChannelMaximum );
				  ++nnChannel ) {
				const auto mapped = pMidiInstrumentMap->mapInput(
					Midi::noteFromInt( nnNote ),
					Midi::channelFromInt( nnChannel ), pNewDrumkit
				);
				if ( mapped.size() > 0 ) {
					CPPUNIT_ASSERT( mapped.size() == 1 );
					CPPUNIT_ASSERT( mapped[0] != nullptr );
					CPPUNIT_ASSERT(
						mapped[0] ==
						pInstrumentList->get(
							nnNote - static_cast<int>( Midi::NoteOffset )
						)
					);
					++nMappedInstruments;
				}
			}
		}
		CPPUNIT_ASSERT( nMappedInstruments == pInstrumentList->size() );
	}

	////////////////////////////////////////////////////////////////////////////
	// Test mapping override using special channel values.

	{
		pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::AsOutput );
		const auto pInstrument = pNewDrumkit->getInstruments()->get( 0 );
		CPPUNIT_ASSERT( pInstrument != nullptr );
		CPPUNIT_ASSERT(
			pInstrument->getMidiOutChannel() >= Midi::ChannelMinimum
		);
		const auto note = pInstrument->getMidiOutNote();
		const auto channel = pInstrument->getMidiOutChannel();

		// Sanity tests
		{
			const auto instrumentsMapped =
				pMidiInstrumentMap->mapInput( note, channel, pNewDrumkit );
			CPPUNIT_ASSERT( instrumentsMapped.size() == 1 );
			CPPUNIT_ASSERT( instrumentsMapped[0] == pInstrument );
		}
		{
			const auto instrumentsMapped = pMidiInstrumentMap->mapInput(
				note,
				Midi::channelFromIntClamp( static_cast<int>( channel ) + 1 ),
				pNewDrumkit
			);
			CPPUNIT_ASSERT( instrumentsMapped.size() == 0 );
		}
		// Disable overwrite
		{
			const auto instrumentsMapped = pMidiInstrumentMap->mapInput(
				note, Midi::ChannelOff, pNewDrumkit
			);
			CPPUNIT_ASSERT( instrumentsMapped.size() == 0 );
		}
		// Enable overwrite
		{
			const auto instrumentsMapped = pMidiInstrumentMap->mapInput(
				note, Midi::ChannelAll, pNewDrumkit
			);
			CPPUNIT_ASSERT( instrumentsMapped.size() == 1 );
			CPPUNIT_ASSERT( instrumentsMapped[0] == pInstrument );
		}
	}

	___INFOLOG( "passed" );
}

void MidiNoteTest::testMidiInstrumentOutputMapping()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	auto pNewPreferences = CoreActionController::loadPreferences(
		H2TEST_FILE( "preferences/midi-instrument-mapping.conf" )
	);
	CPPUNIT_ASSERT( pNewPreferences != nullptr );

	const auto pNewDrumkit = Drumkit::load(
		H2TEST_FILE( "drumkits/midi-instrument-mapping" ),
		/* bUpgrade */ true, /* pLegacy */ nullptr, /*bSilent*/ false
	);
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pNewDrumkit ) );

	auto pMidiInstrumentMap = pNewPreferences->getMidiInstrumentMap();
	auto pInstrumentList = pNewDrumkit->getInstruments();

	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::None );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		CPPUNIT_ASSERT( pMidiInstrumentMap
							->getOutputMapping( nullptr, ppInstrument )
							.isNull() );
	}

	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::Constant );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		const auto noteRef =
			pMidiInstrumentMap->getOutputMapping( nullptr, ppInstrument );
		CPPUNIT_ASSERT( noteRef.note == ppInstrument->getMidiOutNote() );
		CPPUNIT_ASSERT( noteRef.channel == ppInstrument->getMidiOutChannel() );
	}

	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::Offset );
	std::vector<std::shared_ptr<Note> > notes;
	notes.push_back( std::make_shared<Note>() );
	auto pNoteLow = std::make_shared<Note>();
	pNoteLow->setKey( Note::Key::D );
	pNoteLow->setOctave( Note::Octave::P8Y );
	notes.push_back( pNoteLow );
	auto pNoteHigh = std::make_shared<Note>();
	pNoteHigh->setKey( Note::Key::F );
	pNoteHigh->setOctave( Note::Octave::P8A );
	notes.push_back( pNoteHigh );
	for ( const auto& ppInstrument : *pInstrumentList ) {
		CPPUNIT_ASSERT( ppInstrument != nullptr );
		for ( auto& ppNote : notes ) {
			ppNote->mapToInstrument( ppInstrument );
			const auto noteRef = pMidiInstrumentMap->getOutputMapping( ppNote );
			CPPUNIT_ASSERT(
				noteRef.note ==
				Midi::noteFromInt(
					static_cast<int>( ppInstrument->getMidiOutNote() ) +
					static_cast<int>(
						std::round( static_cast<float>( ppNote->toPitch() ) )
					)
				)
			);
			CPPUNIT_ASSERT(
				noteRef.channel == ppInstrument->getMidiOutChannel()
			);
		}
	}

	___INFOLOG( "passed" );
}

void MidiNoteTest::testMidiInstrumentGlobalMapping()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	auto pNewPreferences = CoreActionController::loadPreferences(
		H2TEST_FILE( "preferences/midi-instrument-mapping.conf" )
	);
	CPPUNIT_ASSERT( pNewPreferences != nullptr );

	const auto pNewDrumkit = Drumkit::load(
		H2TEST_FILE( "drumkits/midi-instrument-mapping" ),
		/* bUpgrade */ true, /* pLegacy */ nullptr, /*bSilent*/ false
	);
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pNewDrumkit ) );

	auto pMidiInstrumentMap = pNewPreferences->getMidiInstrumentMap();
	auto pInstrumentList = pNewDrumkit->getInstruments();

	auto testOutput = [&]( bool bGlobal, Midi::Channel channel ) {
		for ( const auto& ppInstrument : *pInstrumentList ) {
			CPPUNIT_ASSERT( ppInstrument != nullptr );
			const auto noteRef =
				pMidiInstrumentMap->getOutputMapping( nullptr, ppInstrument );
			CPPUNIT_ASSERT( noteRef.note == ppInstrument->getMidiOutNote() );
			if ( bGlobal ) {
				CPPUNIT_ASSERT( noteRef.channel == channel );
			}
			else {
				CPPUNIT_ASSERT(
					noteRef.channel == ppInstrument->getMidiOutChannel()
				);
			}
		}
	};

	pMidiInstrumentMap->setUseGlobalOutputChannel( false );
	pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::Constant );
	testOutput( false, Midi::ChannelDefault );

	const auto globalOutputChannel = Midi::channelFromInt( 11 );
	pMidiInstrumentMap->setUseGlobalOutputChannel( true );
	pMidiInstrumentMap->setGlobalOutputChannel( globalOutputChannel );
	testOutput( true, globalOutputChannel );
	pMidiInstrumentMap->setUseGlobalOutputChannel( false );

	auto testInput = [&]( bool bGlobal, Midi::Channel channel ) {
		for ( const auto& ppInstrument : *pInstrumentList ) {
			CPPUNIT_ASSERT( ppInstrument != nullptr );
			const auto noteRef = pMidiInstrumentMap->getInputMapping(
				ppInstrument, pNewDrumkit
			);
			CPPUNIT_ASSERT( noteRef.note == ppInstrument->getMidiOutNote() );
			if ( bGlobal ) {
				CPPUNIT_ASSERT( noteRef.channel == channel );
			}
			else {
				CPPUNIT_ASSERT(
					noteRef.channel == ppInstrument->getMidiOutChannel()
				);
			}
		}
		{
			int nMappedInstruments = 0;
			for ( int nnNote = static_cast<int>( Midi::NoteMinimum );
				  nnNote <= static_cast<int>( Midi::NoteMaximum ); ++nnNote ) {
				for ( int nnChannel = static_cast<int>( Midi::ChannelOff );
					  nnChannel <= static_cast<int>( Midi::ChannelMaximum );
					  ++nnChannel ) {
					const auto mapped = pMidiInstrumentMap->mapInput(
						Midi::noteFromInt( nnNote ),
						Midi::channelFromInt( nnChannel ), pNewDrumkit
					);
					if ( mapped.size() > 0 ) {
						CPPUNIT_ASSERT( mapped.size() == 1 );
						CPPUNIT_ASSERT( mapped[0] != nullptr );
						CPPUNIT_ASSERT(
							mapped[0]->getMidiOutNote() ==
							Midi::noteFromInt( nnNote )
						);
						if ( bGlobal ) {
							CPPUNIT_ASSERT(
								Midi::channelFromInt( nnChannel ) == channel
							);
						}
						else {
							CPPUNIT_ASSERT(
								mapped[0]->getMidiOutChannel() ==
								Midi::channelFromInt( nnChannel )
							);
						}
						++nMappedInstruments;
					}
				}
			}
			CPPUNIT_ASSERT( nMappedInstruments == pInstrumentList->size() );
		}
	};
	pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::AsOutput );
	pMidiInstrumentMap->setUseGlobalInputChannel( false );
	testInput( false, Midi::ChannelDefault );

	pMidiInstrumentMap->setUseGlobalInputChannel( true );
	const auto globalInputChannel = Midi::channelFromInt( 8 );
	pMidiInstrumentMap->setGlobalInputChannel( globalInputChannel );
	testInput( true, globalInputChannel );
	pMidiInstrumentMap->setUseGlobalOutputChannel( true );
	testInput( true, globalInputChannel );
	pMidiInstrumentMap->setUseGlobalInputChannel( false );
	testInput( true, globalOutputChannel );
	pMidiInstrumentMap->setUseGlobalOutputChannel( false );
	testInput( false, globalOutputChannel );

	___INFOLOG( "passed" );
}

void MidiNoteTest::testSendNoteOff()
{
	___INFOLOG( "" );

	auto pPref = Preferences::get_instance();

	// Since we rely on the Sampler to properly set the end of notes with custom
	// length, we have to ensure the audio engine is in the right state.
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pAudioEngine->getAudioDriver() != nullptr );

	auto pSampler = pAudioEngine->getSampler();
	auto renderNote = [&]( std::shared_ptr<Note> pNote ) {
        pAudioEngine->lock( RIGHT_HERE );
		pNote->setPosition( TransportPosition::computeTickFromFrame(
			pAudioEngine->getRealtimeFrame() +
			pAudioEngine->getAudioDriver()->getBufferSize()
		) );
		pNote->computeNoteStart();
		const bool bReturn = pSampler->noteOn( pNote );
		pAudioEngine->unlock();

        return bReturn;
	};

	const auto pSong =
		Song::load( H2TEST_FILE( "song/midi-send-note-off.h2song" ), false );
	CPPUNIT_ASSERT( pSong != nullptr && pSong->getDrumkit() != nullptr );

	// Ensure no other notes are playing/ringing out.
	auto clearSampler = [&]() {
		pSampler->releasePlayingNotes();
		bool bStillPlayingNotes = true;
		for ( int ii = 0; ii < 30; ++ii ) {
			if ( !pSampler->isRenderingNotes() ) {
				bStillPlayingNotes = false;
				break;
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		}
	};
	clearSampler();
	CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

	auto pLoopBackMidiDriver = dynamic_cast<LoopBackMidiDriver*>(
		pAudioEngine->getMidiDriver().get()
	);
	CPPUNIT_ASSERT( pLoopBackMidiDriver != nullptr );

    // Maximum temporal distance between a NOTE_OFF preceding a NOTE_ON in the
    // auto-stop feature. This is expected to be significantly shorter than the
    // (custom) note length. Given in milliseconds.
	const int nMaxDelayAutoStopNoteMs = 1;
    // We do not care about the exact length of the note but just check that the
    // corresponding NOTE_OFF is not send directly after NOTE_ON.
	const int nMinimalNoteDurationMs = 10;

    const int nCustomLengthInTicks = 108;
	const int nCustomLengthDurationMs =
		pAudioEngine->getTransportPosition()->getTickSize() * 1000 *
		nCustomLengthInTicks /
		pAudioEngine->getAudioDriver()->getSampleRate();
    const int nDurationTolerance = nCustomLengthDurationMs * 0.05;

	////////////////////////////////////////////////////////////////////////////
	// Tests with sample

	const auto pInstrumentWithSample =
		pSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrumentWithSample != nullptr );
	CPPUNIT_ASSERT( pInstrumentWithSample->hasSamples() );
	pInstrumentWithSample->loadSamples( Hydrogen::get_instance()
											->getAudioEngine()
											->getTransportPosition()
											->getBpm() );

	auto pNoteWithSample = std::make_shared<Note>( pInstrumentWithSample );
	auto pNoteWithSampleCustomLength =
		std::make_shared<Note>( pInstrumentWithSample );
	pNoteWithSampleCustomLength->setLength( nCustomLengthInTicks );

	{
		pPref->setMidiSendNoteOff( Preferences::MidiSendNoteOff::Always );
		pLoopBackMidiDriver->clearBacklogMessages();
		CPPUNIT_ASSERT(
			renderNote( std::make_shared<Note>( pNoteWithSample ) )
		);
		clearSampler();
		CPPUNIT_ASSERT( renderNote(
			std::make_shared<Note>( pNoteWithSampleCustomLength )
		) );
		clearSampler();
		CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

		const auto messageBacklog = pLoopBackMidiDriver->getBacklogMessages();
		CPPUNIT_ASSERT( messageBacklog.size() == 6 );
		CPPUNIT_ASSERT(
			messageBacklog[0].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[1].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[2].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[3].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[4].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[5].getType() == MidiMessage::Type::NoteOff
		);

		// Check the temporal distances between sent notes.
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[1].getTimePoint() -
				messageBacklog[0].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[2].getTimePoint() -
				messageBacklog[1].getTimePoint()
			)
				.count() >= nMinimalNoteDurationMs
		);
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[4].getTimePoint() -
				messageBacklog[3].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		CPPUNIT_ASSERT(
			std::abs(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					messageBacklog[5].getTimePoint() -
					messageBacklog[4].getTimePoint()
				)
					.count() -
				nCustomLengthDurationMs
			) < nDurationTolerance
		);
	}

	{
		pPref->setMidiSendNoteOff( Preferences::MidiSendNoteOff::Never );
		pLoopBackMidiDriver->clearBacklogMessages();
		CPPUNIT_ASSERT(
			renderNote( std::make_shared<Note>( pNoteWithSample ) )
		);
		clearSampler();
		CPPUNIT_ASSERT( renderNote(
			std::make_shared<Note>( pNoteWithSampleCustomLength )
		) );
		clearSampler();
		CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

		const auto messageBacklog = pLoopBackMidiDriver->getBacklogMessages();
		CPPUNIT_ASSERT( messageBacklog.size() == 2 );
		CPPUNIT_ASSERT(
			messageBacklog[0].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[1].getType() == MidiMessage::Type::NoteOn
		);
	}

	{
		pPref->setMidiSendNoteOff( Preferences::MidiSendNoteOff::OnCustomLengths
		);
		pLoopBackMidiDriver->clearBacklogMessages();
		CPPUNIT_ASSERT(
			renderNote( std::make_shared<Note>( pNoteWithSample ) )
		);
		clearSampler();
		CPPUNIT_ASSERT( renderNote(
			std::make_shared<Note>( pNoteWithSampleCustomLength )
		) );
		clearSampler();
		CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

		const auto messageBacklog = pLoopBackMidiDriver->getBacklogMessages();
		CPPUNIT_ASSERT( messageBacklog.size() == 4 );
		CPPUNIT_ASSERT(
			messageBacklog[0].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[1].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[2].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[3].getType() == MidiMessage::Type::NoteOff
		);

		// Check the temporal distances between sent notes.
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[2].getTimePoint() -
				messageBacklog[1].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		CPPUNIT_ASSERT(
			std::abs(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					messageBacklog[3].getTimePoint() -
					messageBacklog[2].getTimePoint()
				)
					.count() -
				nCustomLengthDurationMs
			) < nDurationTolerance
		);
	}

	////////////////////////////////////////////////////////////////////////////
	// Tests without sample

	auto pInstrumentWithoutSample =
		pSong->getDrumkit()->getInstruments()->get( 1 );
	CPPUNIT_ASSERT( pInstrumentWithoutSample != nullptr );
	CPPUNIT_ASSERT( !pInstrumentWithoutSample->hasSamples() );

	auto pNoteWithoutSample =
		std::make_shared<Note>( pInstrumentWithoutSample );
	auto pNoteWithoutSampleCustomLength =
		std::make_shared<Note>( pInstrumentWithoutSample );
	pNoteWithoutSampleCustomLength->setLength( nCustomLengthInTicks );

	{
		pPref->setMidiSendNoteOff( Preferences::MidiSendNoteOff::Always );
		pLoopBackMidiDriver->clearBacklogMessages();
		CPPUNIT_ASSERT( renderNote( pNoteWithoutSample ) );
		clearSampler();
		CPPUNIT_ASSERT( renderNote( pNoteWithoutSampleCustomLength ) );
		clearSampler();
		CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

		const auto messageBacklog = pLoopBackMidiDriver->getBacklogMessages();
		CPPUNIT_ASSERT( messageBacklog.size() == 6 );
		CPPUNIT_ASSERT(
			messageBacklog[0].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[1].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[2].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[3].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[4].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[5].getType() == MidiMessage::Type::NoteOff
		);

		// Check the temporal distances between sent notes.
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[1].getTimePoint() -
				messageBacklog[0].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		// Since there is no sample, the note off will be sent right away.
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[2].getTimePoint() -
				messageBacklog[1].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[4].getTimePoint() -
				messageBacklog[3].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		CPPUNIT_ASSERT(
			std::abs(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					messageBacklog[5].getTimePoint() -
					messageBacklog[4].getTimePoint()
				)
					.count() -
				nCustomLengthDurationMs
			) < nDurationTolerance
		);
	}

	{
		pPref->setMidiSendNoteOff( Preferences::MidiSendNoteOff::Never );
		pLoopBackMidiDriver->clearBacklogMessages();
		CPPUNIT_ASSERT(
			renderNote( std::make_shared<Note>( pNoteWithoutSample ) )
		);
		clearSampler();
		CPPUNIT_ASSERT( renderNote(
			std::make_shared<Note>( pNoteWithoutSampleCustomLength )
		) );
		clearSampler();
		CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

		const auto messageBacklog = pLoopBackMidiDriver->getBacklogMessages();
		CPPUNIT_ASSERT( messageBacklog.size() == 2 );
		CPPUNIT_ASSERT(
			messageBacklog[0].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[1].getType() == MidiMessage::Type::NoteOn
		);
	}

	{
		pPref->setMidiSendNoteOff( Preferences::MidiSendNoteOff::OnCustomLengths
		);
		pLoopBackMidiDriver->clearBacklogMessages();
		CPPUNIT_ASSERT(
			renderNote( std::make_shared<Note>( pNoteWithoutSample ) )
		);
		clearSampler();
		CPPUNIT_ASSERT( renderNote(
			std::make_shared<Note>( pNoteWithoutSampleCustomLength )
		) );
		clearSampler();
		CPPUNIT_ASSERT( !pSampler->isRenderingNotes() );

		const auto messageBacklog = pLoopBackMidiDriver->getBacklogMessages();
		CPPUNIT_ASSERT( messageBacklog.size() == 4 );
		CPPUNIT_ASSERT(
			messageBacklog[0].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[1].getType() == MidiMessage::Type::NoteOff
		);
		CPPUNIT_ASSERT(
			messageBacklog[2].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			messageBacklog[3].getType() == MidiMessage::Type::NoteOff
		);

		// Check the temporal distances between sent notes.
		CPPUNIT_ASSERT(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				messageBacklog[2].getTimePoint() -
				messageBacklog[1].getTimePoint()
			)
				.count() <= nMaxDelayAutoStopNoteMs
		);
		CPPUNIT_ASSERT(
			std::abs(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					messageBacklog[3].getTimePoint() -
					messageBacklog[2].getTimePoint()
				)
					.count() -
				nCustomLengthDurationMs
			) < nDurationTolerance
		);
	}

	___INFOLOG( "passed" );
}

void MidiNoteTest::checkInstrumentMidiNote(
	const QString& sName,
	Midi::Note note,
	std::shared_ptr<Instrument> pInstrument
)
{
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const auto sInstrumentName = pInstrument->getName();
	const auto nInstrumentNote = pInstrument->getMidiOutNote();

	// Bad index / setup.
	CPPUNIT_ASSERT( pInstrument->getName() == sName );
	CPPUNIT_ASSERT( pInstrument->getMidiOutNote() == note );
}
