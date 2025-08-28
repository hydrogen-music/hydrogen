/*
 * Hydrogen
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

#include "MidiActionTest.h"

#include <chrono>
#include <thread>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiMessage.h>
#include <core/Midi/MidiMap.h>

using namespace H2Core;

void MidiActionTest::setUp() {
	auto pPref = Preferences::get_instance();
	pPref->m_nMidiChannelFilter = -1;
}

void MidiActionTest::tearDown() {
	Preferences::get_instance()->getMidiMap()->reset();
}

void MidiActionTest::testBeatCounterAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pPref = Preferences::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nBeatCounterPara = 0;
	pMidiMap->registerCCEvent( nBeatCounterPara, std::make_shared<MidiAction>(
								   MidiAction::Type::BeatCounter ) );
	pPref->m_bpmTap = Preferences::BpmTap::BeatCounter;
	pPref->m_beatCounter = Preferences::BeatCounter::Tap;
	pHydrogen->setBeatCounterTotalBeats( 4 );
	pHydrogen->setBeatCounterBeatLength( 1 );

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	const auto beatCounterMessage = MidiMessage(
		MidiMessage::Type::ControlChange, nBeatCounterPara, 0, 0 );
	sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldTempoBC ).arg( fNewTempoBC ) );
	CPPUNIT_ASSERT( fNewTempoBC != fOldTempoBC );

	___INFOLOG("done");
}

void MidiActionTest::testBpmCcRelativeAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nDiff = 3;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmCcRelative );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldTempoBC = 120;
	pAudioEngine->setNextBpm( fOldTempoBC );
	pAudioEngine->unlock();

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldTempoBC ).arg( fNewTempoBC ) );
	CPPUNIT_ASSERT( fNewTempoBC == fOldTempoBC - nDiff );

	___INFOLOG("done");
}

void MidiActionTest::testBpmDecreaseAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nValue = 1;
	const int nDiff = 5;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmDecr );
	pAction->setValue( QString::number( nValue ) );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldTempoBC = 120;
	pAudioEngine->setNextBpm( fOldTempoBC );
	pAudioEngine->unlock();

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldTempoBC ).arg( fNewTempoBC ) );
	CPPUNIT_ASSERT( fNewTempoBC == fOldTempoBC - nDiff );

	___INFOLOG("done");
}

void MidiActionTest::testBpmFineCcRelativeAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nDiff = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::BpmFineCcRelative );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldTempoBC = 120;
	pAudioEngine->setNextBpm( fOldTempoBC );
	pAudioEngine->unlock();

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldTempoBC ).arg( fNewTempoBC ) );
	CPPUNIT_ASSERT(
		std::abs( fNewTempoBC - (
					  fOldTempoBC - 0.01 * static_cast<float>(nDiff) ) ) < 0.01 );

	___INFOLOG("done");
}

void MidiActionTest::testBpmIncreaseAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nValue = 1;
	const int nDiff = 7;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmIncr );
	pAction->setValue( QString::number( nValue ) );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldTempoBC = 120;
	pAudioEngine->setNextBpm( fOldTempoBC );
	pAudioEngine->unlock();

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldTempoBC ).arg( fNewTempoBC ) );
	CPPUNIT_ASSERT( fNewTempoBC == fOldTempoBC + nDiff );

	___INFOLOG("done");
}

void MidiActionTest::testClearPatternAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	pMidiMap->registerCCEvent(
		nParameter, std::make_shared<MidiAction>( MidiAction::Type::ClearPattern ) );

	const int nPatternNumber = pHydrogen->getSelectedPatternNumber();
	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	// We first add note and then ensure it is gone after triggering the action.
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pPattern = pSong->getPatternList()->get( nPatternNumber );
	CPPUNIT_ASSERT( pPattern != nullptr );

	pPattern->insertNote( std::make_shared<Note>( nullptr, 0 ) );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 1 );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 0 );

	// Check robustness against having no pattern selected (should not happen).
	pHydrogen->setSelectedPatternNumber( -1, true /* bNeedsLock */,
										 Event::Trigger::Suppress );
	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	pHydrogen->setSelectedPatternNumber( nPatternNumber, true /* bNeedsLock */,
										 Event::Trigger::Suppress );

	___INFOLOG("done");
}

void MidiActionTest::testClearSelectedInstrumentAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nParameterClearPattern = 2;
	pMidiMap->registerCCEvent(
		nParameter, std::make_shared<MidiAction>(
			MidiAction::Type::ClearSelectedInstrument ) );
	pMidiMap->registerCCEvent(
		nParameterClearPattern, std::make_shared<MidiAction>(
			MidiAction::Type::ClearPattern ) );

	// We reset the whole pattern to have a clean canvas.
	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameterClearPattern, 0, 0 ) );

	// We first add note and then ensure it is gone after triggering the action.
	const int nPatternNumber = pHydrogen->getSelectedPatternNumber();
	const int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nSelectedInstrument );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pAnotherInstrument = pSong->getDrumkit()->getInstruments()
		->get( nSelectedInstrument + 1 );
	CPPUNIT_ASSERT( pAnotherInstrument != nullptr );
	CPPUNIT_ASSERT( pAnotherInstrument != pInstrument );

	auto pPattern = pSong->getPatternList()->get( nPatternNumber );
	CPPUNIT_ASSERT( pPattern != nullptr );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 0 );

	pPattern->insertNote( std::make_shared<Note>( pInstrument, 0 ) );
	pPattern->insertNote( std::make_shared<Note>( pAnotherInstrument, 0 ) );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 2 );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 1 );

	// Check robustness against having no pattern selected (should not happen).
	pHydrogen->setSelectedInstrumentNumber( -1, Event::Trigger::Suppress );
	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	pHydrogen->setSelectedInstrumentNumber(
		nSelectedInstrument, Event::Trigger::Suppress );

	___INFOLOG("done");
}

void MidiActionTest::sendMessage( const MidiMessage& msg ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	auto pDriver = dynamic_cast<LoopBackMidiDriver*>(
		pAudioEngine->getMidiDriver().get() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	pDriver->clearBacklogMessages();
	pDriver->sendMessage( msg );

	// Wait till the LoopBackMidiDriver did send, receive, and handle the
	// message.
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( pDriver->getBacklogMessages().size() == 0 ) {
		CPPUNIT_ASSERT( nnTry < nMaxTries );

		++nnTry;
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
	}
}
