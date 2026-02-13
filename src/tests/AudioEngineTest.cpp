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

#include "AudioEngineTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/IO/FakeAudioDriver.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>

#include "TestHelper.h"

using namespace H2Core;

void AudioEngineTest::testMidiNoteOrdering()
{
	___INFOLOG( "" );

	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pAudioDriver = std::dynamic_pointer_cast<FakeAudioDriver>(
		pAudioEngine->getAudioDriver()
	);
	auto pLoopBackDriver = std::dynamic_pointer_cast<LoopBackMidiDriver>(
		pAudioEngine->getMidiDriver()
	);

	CPPUNIT_ASSERT( pAudioDriver != nullptr );
	CPPUNIT_ASSERT( pLoopBackDriver != nullptr );

	auto pSong = Song::load( H2TEST_FILE( "song/midi-note-ordering.h2song" ) );
	CPPUNIT_ASSERT( pSong != nullptr || pSong->getDrumkit() != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setSong( pSong ) );

    // Ensure the playback stops after playing a single pattern.
    CPPUNIT_ASSERT( pSong->getPatternGroupVector()->size() == 1 );

    // Ensure we do not build a MIDI feedback loop
	const auto oldInputMapping = pPref->getMidiInstrumentMap()->getInput();
	const auto oldOutputMapping = pPref->getMidiInstrumentMap()->getOutput();
	const auto bOldUseGlobalOutputChannel =
		pPref->getMidiInstrumentMap()->getUseGlobalOutputChannel();
	const auto oldGlobalOutputChannel =
		pPref->getMidiInstrumentMap()->getGlobalOutputChannel();
	const auto oldActionChannel = pPref->m_midiActionChannel;

	pPref->getMidiInstrumentMap()->setInput(
		MidiInstrumentMap::Input::None
	);
	pPref->getMidiInstrumentMap()->setOutput(
		MidiInstrumentMap::Output::Offset
	);
	pPref->getMidiInstrumentMap()->setUseGlobalOutputChannel( true );
	pPref->getMidiInstrumentMap()->setGlobalOutputChannel( Midi::ChannelDefault
	);
    // We must disable action event handling. Otherwise, Note-On events set in
    // the config files might cause weird side effects in this test.
    pPref->m_midiActionChannel = Midi::ChannelOff;

	const auto bOldSongMode = pSong->getMode();
	const auto bOldLoopMode = pSong->getLoopMode();
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( true ) );
	CPPUNIT_ASSERT( CoreActionController::activateLoopMode( false ) );

	pLoopBackDriver->clearBacklogMessages();

	pHydrogen->sequencerPlay();
	CPPUNIT_ASSERT(
		pAudioEngine->getState() == AudioEngine::State::Playing ||
		pAudioEngine->getNextState() == AudioEngine::State::Playing
	);

	// Wait till playback is done.
	// Since incoming MIDI events are handled asynchronously, we pause execution
	// till all are handled.
	const int nMaxTries = 100;
	int nnTry = 0;
	// The audio engine start playback within the next processing cycle. Wait
	// for it.
	bool bIsPlaying = false;
	while ( nnTry < nMaxTries ) {
		if ( bIsPlaying &&
			 pAudioEngine->getState() != AudioEngine::State::Playing ) {
			break;
		}
		else if ( !bIsPlaying &&
				  pAudioEngine->getState() == AudioEngine::State::Playing ) {
			bIsPlaying = true;
		}

		++nnTry;
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}
	CPPUNIT_ASSERT( nnTry < nMaxTries );

	// Compare the number of encountered Note-On messages with the total number
	// of notes in the current song.
	const auto backlogMessage = pLoopBackDriver->getBacklogMessages();

	// We expect both a Note-On and Note-Off message for each note
	CPPUNIT_ASSERT( pSong->getAllNotes().size() * 2 == backlogMessage.size() );

	// After each Note-On there should be the corresponding Note-Off. (Since
	// they all have custom lengths with tails touching the next note head).
	for ( int ii = 0; ii < backlogMessage.size() / 2; ++ii ) {
		CPPUNIT_ASSERT(
			backlogMessage[ii * 2].getType() == MidiMessage::Type::NoteOn
		);
		CPPUNIT_ASSERT(
			backlogMessage[ii * 2 + 1].getType() == MidiMessage::Type::NoteOff
		);
		// Each note was shifted to an unique pitch within the pattern
		CPPUNIT_ASSERT(
			backlogMessage[ii * 2].getData1() ==
			backlogMessage[ii * 2 + 1].getData1()
		);

		// Since note tails are touching note heads, we expect the corresponding
		// MIDI messages to be send at the same time.
		if ( ii > 0 ) {
			CPPUNIT_ASSERT(
				backlogMessage[ii * 2 - 1].getFrameOffset() ==
				backlogMessage[ii * 2].getFrameOffset()
			);
		}
	}
	for ( const auto mmessage : backlogMessage ) {
		CPPUNIT_ASSERT( mmessage.getFrameOffset() >= 0 );
		CPPUNIT_ASSERT(
			mmessage.getFrameOffset() <= pAudioDriver->getBufferSize()
		);
	}

	// Clean up
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( bOldSongMode == Song::Mode::Song ) );
	CPPUNIT_ASSERT( CoreActionController::activateLoopMode( bOldLoopMode == Song::LoopMode::Enabled ) );

	pPref->getMidiInstrumentMap()->setInput( oldInputMapping );
	pPref->getMidiInstrumentMap()->setOutput( oldOutputMapping );
	pPref->getMidiInstrumentMap()->setUseGlobalOutputChannel(
		bOldUseGlobalOutputChannel
	);
	pPref->getMidiInstrumentMap()->setGlobalOutputChannel(
		oldGlobalOutputChannel
	);
    pPref->m_midiActionChannel = oldActionChannel;

	___INFOLOG( "passed" );
}

void AudioEngineTest::testNotePickup()
{
	___INFOLOG( "" );

	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pAudioDriver = std::dynamic_pointer_cast<FakeAudioDriver>(
		pAudioEngine->getAudioDriver()
	);
	auto pMidiDriver = std::dynamic_pointer_cast<LoopBackMidiDriver>(
		pAudioEngine->getMidiDriver()
	);

	CPPUNIT_ASSERT( pAudioDriver != nullptr );
	CPPUNIT_ASSERT( pMidiDriver != nullptr );

	auto pSong = Song::load( H2TEST_FILE( "song/AE_loopMode.h2song" ) );
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setSong( pSong ) );

    // Ensure the playback stops after playing a single pattern and that MIDI
    // Note-On events are sent for each encountered note.
    CPPUNIT_ASSERT( pSong->getPatternGroupVector()->size() == 1 );

    // Ensure we do not build a MIDI feedback loop
	const auto oldInputMapping = pPref->getMidiInstrumentMap()->getInput();
	const auto oldOutputMapping = pPref->getMidiInstrumentMap()->getOutput();
	const auto bOldUseGlobalOutputChannel =
		pPref->getMidiInstrumentMap()->getUseGlobalOutputChannel();
	const auto oldGlobalOutputChannel =
		pPref->getMidiInstrumentMap()->getGlobalOutputChannel();
	const auto oldActionChannel = pPref->m_midiActionChannel;

	pPref->getMidiInstrumentMap()->setInput(
		MidiInstrumentMap::Input::None
	);
	pPref->getMidiInstrumentMap()->setOutput(
		MidiInstrumentMap::Output::Constant
	);
	pPref->getMidiInstrumentMap()->setUseGlobalOutputChannel( true );
	pPref->getMidiInstrumentMap()->setGlobalOutputChannel( Midi::ChannelDefault
	);
    // We must disable action event handling. Otherwise, Note-On events set in
    // the config files might cause weird side effects in this test.
    pPref->m_midiActionChannel = Midi::ChannelOff;

	const auto bOldSongMode = pSong->getMode();
	const auto bOldLoopMode = pSong->getLoopMode();
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( true ) );
	CPPUNIT_ASSERT( CoreActionController::activateLoopMode( false ) );

	pMidiDriver->clearBacklogMessages();

	pHydrogen->sequencerPlay();
	CPPUNIT_ASSERT(
		pAudioEngine->getState() == AudioEngine::State::Playing ||
		pAudioEngine->getNextState() == AudioEngine::State::Playing
	);

	// Wait till playback is done.
	// Since incoming MIDI events are handled asynchronously, we pause execution
	// till all are handled.
	const int nMaxTries = 100;
	int nnTry = 0;
	// The audio engine start playback within the next processing cycle. Wait
	// for it.
	bool bIsPlaying = false;
	while ( nnTry < nMaxTries ) {
		if ( bIsPlaying &&
			 pAudioEngine->getState() != AudioEngine::State::Playing ) {
			break;
		}
		else if ( !bIsPlaying &&
				  pAudioEngine->getState() == AudioEngine::State::Playing ) {
			bIsPlaying = true;
		}

		++nnTry;
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}
	CPPUNIT_ASSERT( nnTry < nMaxTries );

	// Compare the number of encountered Note-On messages with the total number
	// of notes in the current song.
	const auto backlogMessage = pMidiDriver->getBacklogMessages();
	int nNoteOnMessage = 0;
	for ( const auto& mmidiMessage : backlogMessage ) {
		if ( mmidiMessage.getType() == MidiMessage::Type::NoteOn ) {
            ++nNoteOnMessage;
		}
	}

    CPPUNIT_ASSERT( nNoteOnMessage != 0 );
    CPPUNIT_ASSERT( nNoteOnMessage == pSong->getAllNotes().size() );

	// Clean up
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( bOldSongMode == Song::Mode::Song ) );
	CPPUNIT_ASSERT( CoreActionController::activateLoopMode( bOldLoopMode == Song::LoopMode::Enabled ) );

	pPref->getMidiInstrumentMap()->setInput( oldInputMapping );
	pPref->getMidiInstrumentMap()->setOutput( oldOutputMapping );
	pPref->getMidiInstrumentMap()->setUseGlobalOutputChannel(
		bOldUseGlobalOutputChannel
	);
	pPref->getMidiInstrumentMap()->setGlobalOutputChannel(
		oldGlobalOutputChannel
	);
    pPref->m_midiActionChannel = oldActionChannel;

	___INFOLOG( "passed" );
}
