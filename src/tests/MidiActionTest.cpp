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
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
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
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	const int nParameter = 1;
	const int nDiff = 3;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmCcRelative );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 120;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT( fNewBpm == fOldBpm - nDiff );

	___INFOLOG("done");
}

void MidiActionTest::testBpmDecreaseAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	const int nParameter = 1;
	const int nValue = 1;
	const int nDiff = 5;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmDecr );
	pAction->setValue( QString::number( nValue ) );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 130;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT( fNewBpm == fOldBpm - nDiff );

	___INFOLOG("done");
}

void MidiActionTest::testBpmFineCcRelativeAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	const int nParameter = 1;
	const int nDiff = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::BpmFineCcRelative );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 140;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT(
		std::abs( fNewBpm - (
					  fOldBpm - 0.01 * static_cast<float>(nDiff) ) ) < 0.01 );

	___INFOLOG("done");
}

void MidiActionTest::testBpmIncreaseAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	const int nParameter = 1;
	const int nValue = 1;
	const int nDiff = 7;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmIncr );
	pAction->setValue( QString::number( nValue ) );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 150;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT( fNewBpm == fOldBpm + nDiff );

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

void MidiActionTest::testEffectLevelAbsoluteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nFxValue = 25;
	const int nFxId = 1;
	const int nInstrumentNumber = 3;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::EffectLevelAbsolute );
	pAction->setValue( QString::number( nFxValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nFxId ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setFxLevel( fOldValue, nFxId );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nFxValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getFxLevel( nFxId ) != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getFxLevel( nFxId ) )
				.arg( static_cast<float>(nFxValue / 127.0) ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getFxLevel( nFxId ) -
							  ( static_cast<float>(nFxValue) / 127.0 ) ) < 0.01 );
	pInstrument->setFxLevel( fOldValue, nFxId );

	___INFOLOG("done");
}

void MidiActionTest::testEffectLevelRelativeAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nFxValue = 1;
	const int nFxId = 1;
	const int nInstrumentNumber = 3;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::EffectLevelRelative );
	pAction->setValue( QString::number( nFxValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nFxId ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setFxLevel( fOldValue, nFxId );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nFxValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getFxLevel( nFxId ) != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getFxLevel( nFxId ) )
				.arg( fOldValue + 0.05 ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getFxLevel( nFxId ) -
							  ( fOldValue + 0.05 ) ) <= 0.01 );
	pInstrument->setFxLevel( fOldValue, nFxId );

	___INFOLOG("done");
}

void MidiActionTest::testFilterCutoffLevelAbsoluteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nCutoffValue = 101;
	const int nInstrumentNumber = 3;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::FilterCutoffLevelAbsolute );
	pAction->setValue( QString::number( nCutoffValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setFilterCutoff( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nCutoffValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getFilterCutoff() != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getFilterCutoff() )
				.arg( static_cast<float>(nCutoffValue) / 127.0 ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getFilterCutoff() -
							  ( static_cast<float>(nCutoffValue) / 127.0 ) ) <=
					0.01 );
	pInstrument->setFilterCutoff( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testGainLevelAbsoluteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nGainValue = 101;
	const int nInstrumentNumber = 3;
	const int nComponentId = 0;
	const int nLayerId = 0;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::GainLevelAbsolute );
	pAction->setValue( QString::number( nGainValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nComponentId ) );
	pAction->setParameter3( QString::number( nLayerId ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pComponent = pInstrument->getComponent( nComponentId );
	CPPUNIT_ASSERT( pComponent != nullptr );
	auto pLayer = pComponent->getLayer( nLayerId );
	CPPUNIT_ASSERT( pLayer != nullptr );

	const float fOldValue = 0.92;
	pLayer->setGain( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nGainValue, 0 ) );
	CPPUNIT_ASSERT( pLayer->getGain() != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pLayer->getGain() )
				.arg( 5.0 * static_cast<float>(nGainValue) / 127.0 ) );
	CPPUNIT_ASSERT( std::abs( pLayer->getGain() -
							  5.0 * ( static_cast<float>(nGainValue) / 127.0 ) ) <=
					0.01 );
	pLayer->setGain( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testInstrumentPitchAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nPitchValue = 101;
	const int nInstrumentNumber = 3;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::InstrumentPitch );
	pAction->setValue( QString::number( nPitchValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setPitchOffset( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nPitchValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getPitchOffset() != fOldValue );
	const float fRef = ( Instrument::fPitchMax - Instrument::fPitchMin ) *
		( static_cast<float>(nPitchValue) / 127.0 ) + Instrument::fPitchMin;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getPitchOffset() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPitchOffset() - fRef ) <= 0.01 );
	pInstrument->setPitchOffset( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testLoadNextDrumkitAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::LoadNextDrumkit );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pOldDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pOldDrumkit != nullptr );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	auto pNewDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( pNewDrumkit != pOldDrumkit );

	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pOldDrumkit ) );

	___INFOLOG("done");
}

void MidiActionTest::testLoadPrevDrumkitAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::LoadPrevDrumkit );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pOldDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pOldDrumkit != nullptr );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	auto pNewDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( pNewDrumkit != pOldDrumkit );

	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pOldDrumkit ) );

	___INFOLOG("done");
}

void MidiActionTest::testMasterVolumeAbsoluteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nVolumeValue = 102;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::MasterVolumeAbsolute );
	pAction->setValue( QString::number( nVolumeValue ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const float fOldValue = 0.92;
	pSong->setVolume( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nVolumeValue, 0 ) );
	CPPUNIT_ASSERT( pSong->getVolume() != fOldValue );

	const float fRef = 1.5 * static_cast<float>(nVolumeValue) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pSong->getVolume() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pSong->getVolume() - fRef ) <= 0.01 );
	pSong->setVolume( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testMasterVolumeRelativeAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nVolumeValue = 1;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::MasterVolumeRelative );
	pAction->setValue( QString::number( nVolumeValue ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const float fOldValue = 0.92;
	pSong->setVolume( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nVolumeValue, 0 ) );
	CPPUNIT_ASSERT( pSong->getVolume() != fOldValue );

	const float fRef = fOldValue + 0.05;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pSong->getVolume() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pSong->getVolume() - fRef ) <= 0.01 );
	pSong->setVolume( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testMuteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Mute );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const bool bOldValue = false;
	pSong->setIsMuted( bOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	CPPUNIT_ASSERT( pSong->getIsMuted() != bOldValue );
	pSong->setIsMuted( bOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testMuteToggleAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::MuteToggle );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const bool bOldValue = false;
	pSong->setIsMuted( bOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	CPPUNIT_ASSERT( pSong->getIsMuted() != bOldValue );
	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	CPPUNIT_ASSERT( pSong->getIsMuted() == bOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testNextBarAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::NextBar );
	pMidiMap->registerCCEvent( nParameter, pAction );

	const int nPatternNumber = 0;
	const int nColumn = 5;
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( true ) );
	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
						GridPoint( nColumn, nPatternNumber ) ) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pPatternGroupVector = pSong->getPatternGroupVector();
	___INFOLOG( QString( "number of columns: [%1]" )
				.arg( pPatternGroupVector->size() ) );
	CPPUNIT_ASSERT( pPatternGroupVector->size() == nColumn + 1 );

	const int nOldValue = 2;
	CPPUNIT_ASSERT( CoreActionController::locateToColumn( nOldValue ) );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const int nNewValue = pAudioEngine->getTransportPosition()->getColumn();
	pAudioEngine->unlock();

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( nNewValue ).arg( nOldValue ) );
	CPPUNIT_ASSERT( nNewValue == nOldValue + 1 );

	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
						GridPoint( nColumn, nPatternNumber ) ) );

	___INFOLOG("done");
}

void MidiActionTest::testPanAbsoluteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nPanValue = 101;
	const int nInstrumentNumber = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PanAbsolute );
	pAction->setValue( QString::number( nPanValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setPanWithRangeFrom0To1( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nPanValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getPanWithRangeFrom0To1() != fOldValue );
	const float fRef = static_cast<float>(nPanValue) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getPanWithRangeFrom0To1() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPanWithRangeFrom0To1() - fRef ) <=
					0.01 );
	pInstrument->setPanWithRangeFrom0To1( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testPanAbsoluteSymAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nPanValue = 101;
	const int nInstrumentNumber = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PanAbsoluteSym );
	pAction->setValue( QString::number( nPanValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setPan( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nPanValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getPan() != fOldValue );
	const float fRef = static_cast<float>(nPanValue) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getPan() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPan() - fRef ) <=
					0.01 );
	pInstrument->setPan( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testPanRelativeAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nPanValue = 1;
	const int nInstrumentNumber = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PanRelative );
	pAction->setValue( QString::number( nPanValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.82;
	pInstrument->setPan( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nPanValue, 0 ) );
	CPPUNIT_ASSERT( pInstrument->getPan() != fOldValue );
	const float fRef = fOldValue + 0.1;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pInstrument->getPan() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPan() - fRef ) <=
					0.01 );
	pInstrument->setPan( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testPauseAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Pause );
	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->play();
	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() != 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG("done");
}

void MidiActionTest::testPitchLevelAbsoluteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	const int nPitchValue = 101;
	const int nInstrumentNumber = 3;
	const int nComponentId = 0;
	const int nLayerId = 0;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PitchLevelAbsolute );
	pAction->setValue( QString::number( nPitchValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nComponentId ) );
	pAction->setParameter3( QString::number( nLayerId ) );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument = pSong->getDrumkit()->getInstruments()
		->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pComponent = pInstrument->getComponent( nComponentId );
	CPPUNIT_ASSERT( pComponent != nullptr );
	auto pLayer = pComponent->getLayer( nLayerId );
	CPPUNIT_ASSERT( pLayer != nullptr );

	const float fOldValue = 0.92;
	pLayer->setPitch( fOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, nPitchValue, 0 ) );
	CPPUNIT_ASSERT( pLayer->getPitch() != fOldValue );

	const float fRef = ( Instrument::fPitchMax - Instrument::fPitchMin ) *
		( static_cast<float>(nPitchValue) / 127.0 ) + Instrument::fPitchMin;

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( pLayer->getPitch() ).arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pLayer->getPitch() - fRef ) <= 0.01 );
	pLayer->setPitch( fOldValue );

	___INFOLOG("done");
}

void MidiActionTest::testPlayAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Play );
	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->stop();
	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	pAudioEngine->stop();
	CoreActionController::activateSongMode( true );

	___INFOLOG("done");
}

void MidiActionTest::testPlayPauseToggleAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PlayPauseToggle );
	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->stop();
	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() != 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG("done");
}

void MidiActionTest::testPlayStopToggleAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PlayStopToggle );
	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->stop();
	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() == 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG("done");
}

void MidiActionTest::testPreviousBarAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::PreviousBar );
	pMidiMap->registerCCEvent( nParameter, pAction );

	const int nPatternNumber = 0;
	const int nColumn = 4;
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( true ) );
	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
						GridPoint( nColumn, nPatternNumber ) ) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pPatternGroupVector = pSong->getPatternGroupVector();
	___INFOLOG( QString( "number of columns: [%1]" )
				.arg( pPatternGroupVector->size() ) );
	CPPUNIT_ASSERT( pPatternGroupVector->size() == ( nColumn + 1 ) );

	const int nOldValue = 2;
	CPPUNIT_ASSERT( CoreActionController::locateToColumn( nOldValue ) );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const int nNewValue = pAudioEngine->getTransportPosition()->getColumn();
	pAudioEngine->unlock();

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
				.arg( nNewValue ).arg( nOldValue ) );
	CPPUNIT_ASSERT( nNewValue == nOldValue - 1 );

	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
						GridPoint( nColumn, nPatternNumber ) ) );

	___INFOLOG("done");
}

void MidiActionTest::testStopAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	auto pDriver = dynamic_cast<FakeAudioDriver*>(pAudioEngine->getAudioDriver());
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Stop );
	pMidiMap->registerCCEvent( nParameter, pAction );

	pAudioEngine->play();
	std::this_thread::sleep_for( pDriver->getProcessInterval() );

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	std::this_thread::sleep_for( pDriver->getProcessInterval() );
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() == 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG("done");
}

void MidiActionTest::testUnmuteAction() {
	___INFOLOG("");

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiMap = Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();

	const int nParameter = 1;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Unmute );
	pMidiMap->registerCCEvent( nParameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const bool bOldValue = true;
	pSong->setIsMuted( bOldValue );

	sendMessage( MidiMessage( MidiMessage::Type::ControlChange,
							  nParameter, 0, 0 ) );
	CPPUNIT_ASSERT( pSong->getIsMuted() != bOldValue );
	pSong->setIsMuted( false );

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
