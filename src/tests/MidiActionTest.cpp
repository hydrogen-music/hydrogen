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

#include "TestHelper.h"
#include "core/Midi/Midi.h"

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
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Helpers/TimeHelper.h>
#include <core/Hydrogen.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiMessage.h>
#include <core/Midi/MidiEventMap.h>

using namespace H2Core;

void MidiActionTest::setUp()
{
	auto pPref = Preferences::get_instance();
	pPref->m_midiActionChannel = Midi::ChannelAll;

	CoreActionController::activateTimeline( false );
}

void MidiActionTest::tearDown()
{
	Preferences::get_instance()->getMidiEventMap()->reset();
}

void MidiActionTest::testBeatCounterAction()
{
	___INFOLOG( "" );

	auto pTestHelper = TestHelper::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pTimeHelper = pHydrogen->getTimeHelper();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pPref = Preferences::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto beatCounterPara = Midi::parameterFromInt( 0 );
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, beatCounterPara,
		std::make_shared<MidiAction>( MidiAction::Type::BeatCounter )
	);
	pPref->m_bpmTap = Preferences::BpmTap::BeatCounter;
	pPref->m_beatCounter = Preferences::BeatCounter::Tap;
	pPref->m_nBeatCounterDriftCompensation = 0;
	pHydrogen->setBeatCounterTotalBeats( 8 );
	pHydrogen->setBeatCounterBeatLength( 1 );
	pHydrogen->updateBeatCounterSettings();

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	const float fTargetBpm = 338.4;
	const float fIntervalMs = 60000.0 / fTargetBpm;
	const auto interval =
		std::chrono::duration<float, std::milli>{ fIntervalMs };
	CPPUNIT_ASSERT( fTargetBpm < MAX_BPM );

	___DEBUGLOG( QString( "interval: %1" ).arg( fIntervalMs ) );

	const auto beatCounterMessage = MidiMessage(
		MidiMessage::Type::ControlChange, beatCounterPara,
		Midi::ParameterMinimum, Midi::ChannelDefault
	);

	auto intervalCompensation = std::chrono::duration<float, std::milli>( 0 );
	for ( int nnMessage = 0; nnMessage < 8; ++nnMessage ) {
		const auto preSleep = Clock::now();
		pTimeHelper->highResolutionSleep( interval - intervalCompensation );
		const auto postSleep = Clock::now();

		const auto preSend = Clock::now();
		sendMessage( beatCounterMessage );
		const auto postSend = Clock::now();

		// Compensate the additional time spent during sleep as well as while
		// sending the MIDI message.
		intervalCompensation =
			( postSleep - preSleep - ( interval - intervalCompensation ) ) +
			( postSend - preSend );
	}

	// Wait for the audio engine to adopt the new tempo.
	TestHelper::waitForAudioDriver();

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	const float fTolerance = 1;
	___INFOLOG( QString( "[%1] -> [%2] target [%3 +/- %4]" )
					.arg( fOldBpm )
					.arg( fNewBpm )
					.arg( fTargetBpm )
					.arg( fTolerance ) );
	CPPUNIT_ASSERT( fNewBpm != fOldBpm );
	if ( !pTestHelper->isAppveyor() ) {
		CPPUNIT_ASSERT( std::abs( fTargetBpm - fNewBpm ) < fTolerance );
	}

	___INFOLOG( "done" );
}

void MidiActionTest::testBpmCcRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nDiff = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::BpmCcRelative );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 120;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault
	) );

	TestHelper::waitForAudioDriver();

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT( fNewBpm == fOldBpm - nDiff );

	___INFOLOG( "done" );
}

void MidiActionTest::testBpmDecreaseAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nValue = 1;
	const int nDiff = 5;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmDecr );
	pAction->setValue( QString::number( nValue ) );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 130;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault
	) );

	TestHelper::waitForAudioDriver();

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT( fNewBpm == fOldBpm - nDiff );

	___INFOLOG( "done" );
}

void MidiActionTest::testBpmFineCcRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nDiff = 2;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::BpmFineCcRelative );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 140;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault
	) );

	TestHelper::waitForAudioDriver();

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT(
		std::abs( fNewBpm - ( fOldBpm - 0.01 * static_cast<float>( nDiff ) ) ) <
		0.01
	);

	___INFOLOG( "done" );
}

void MidiActionTest::testBpmIncreaseAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nValue = 1;
	const int nDiff = 7;
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmIncr );
	pAction->setValue( QString::number( nValue ) );
	pAction->setParameter1( QString::number( nDiff ) );

	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = 150;
	pAudioEngine->setNextBpm( fOldBpm );
	pAudioEngine->unlock();

	// Wait for the audio engine to pick up the new tempo.
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pTransportPosition->getBpm() == fOldBpm );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);

	TestHelper::waitForAudioDriver();

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldBpm ).arg( fNewBpm ) );
	CPPUNIT_ASSERT( fNewBpm == fOldBpm + nDiff );

	___INFOLOG( "done" );
}

void MidiActionTest::testClearPatternAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, parameter,
		std::make_shared<MidiAction>( MidiAction::Type::ClearPattern )
	);

	const int nPatternNumber = pHydrogen->getSelectedPatternNumber();
	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);

	// We first add note and then ensure it is gone after triggering the action.
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pPattern = pSong->getPatternList()->get( nPatternNumber );
	CPPUNIT_ASSERT( pPattern != nullptr );

	pPattern->insertNote( std::make_shared<Note>( nullptr, 0 ) );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 1 );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 0 );

	// Check robustness against having no pattern selected (should not happen).
	pHydrogen->setSelectedPatternNumber(
		-1, true /* bNeedsLock */, Event::Trigger::Suppress
	);
	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	pHydrogen->setSelectedPatternNumber(
		nPatternNumber, true /* bNeedsLock */, Event::Trigger::Suppress
	);

	___INFOLOG( "done" );
}

void MidiActionTest::testClearSelectedInstrumentAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const auto parameterClearPattern = Midi::parameterFromInt( 2 );
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, parameter,
		std::make_shared<MidiAction>( MidiAction::Type::ClearSelectedInstrument
		)
	);
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, parameterClearPattern,
		std::make_shared<MidiAction>( MidiAction::Type::ClearPattern )
	);

	// We reset the whole pattern to have a clean canvas.
	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameterClearPattern,  Midi::ParameterMinimum,
		Midi::ChannelDefault
	) );

	// We first add note and then ensure it is gone after triggering the action.
	const int nPatternNumber = pHydrogen->getSelectedPatternNumber();
	const int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pAnotherInstrument =
		pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument + 1 );
	CPPUNIT_ASSERT( pAnotherInstrument != nullptr );
	CPPUNIT_ASSERT( pAnotherInstrument != pInstrument );

	auto pPattern = pSong->getPatternList()->get( nPatternNumber );
	CPPUNIT_ASSERT( pPattern != nullptr );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 0 );

	pPattern->insertNote( std::make_shared<Note>( pInstrument, 0 ) );
	pPattern->insertNote( std::make_shared<Note>( pAnotherInstrument, 0 ) );
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 2 );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pPattern->getNotes()->size() == 1 );

	// Check robustness against having no pattern selected (should not happen).
	pHydrogen->setSelectedInstrumentNumber( -1, Event::Trigger::Suppress );
	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	pHydrogen->setSelectedInstrumentNumber(
		nSelectedInstrument, Event::Trigger::Suppress
	);

	___INFOLOG( "done" );
}

void MidiActionTest::testEffectLevelAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nFxValue = 25;
	const int nFxId = 1;
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::EffectLevelAbsolute );
	pAction->setValue( QString::number( nFxValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nFxId ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setFxLevel( fOldValue, nFxId );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nFxValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getFxLevel( nFxId ) != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getFxLevel( nFxId ) )
					.arg( static_cast<float>( nFxValue / 127.0 ) ) );
	CPPUNIT_ASSERT(
		std::abs(
			pInstrument->getFxLevel( nFxId ) -
			( static_cast<float>( nFxValue ) / 127.0 )
		) < 0.01
	);
	pInstrument->setFxLevel( fOldValue, nFxId );

	___INFOLOG( "done" );
}

void MidiActionTest::testEffectLevelRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nFxValue = 1;
	const int nFxId = 1;
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::EffectLevelRelative );
	pAction->setValue( QString::number( nFxValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nFxId ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setFxLevel( fOldValue, nFxId );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nFxValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getFxLevel( nFxId ) != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getFxLevel( nFxId ) )
					.arg( fOldValue + 0.05 ) );
	CPPUNIT_ASSERT(
		std::abs( pInstrument->getFxLevel( nFxId ) - ( fOldValue + 0.05 ) ) <=
		0.01
	);
	pInstrument->setFxLevel( fOldValue, nFxId );

	___INFOLOG( "done" );
}

void MidiActionTest::testFilterCutoffLevelAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nCutoffValue = 101;
	const int nInstrumentNumber = 3;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::FilterCutoffLevelAbsolute
	);
	pAction->setValue( QString::number( nCutoffValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setFilterCutoff( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nCutoffValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getFilterCutoff() != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getFilterCutoff() )
					.arg( static_cast<float>( nCutoffValue ) / 127.0 ) );
	CPPUNIT_ASSERT(
		std::abs(
			pInstrument->getFilterCutoff() -
			( static_cast<float>( nCutoffValue ) / 127.0 )
		) <= 0.01
	);
	pInstrument->setFilterCutoff( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testGainLevelAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nGainValue = 101;
	const int nInstrumentNumber = 3;
	const int nComponentId = 0;
	const int nLayerId = 0;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::GainLevelAbsolute );
	pAction->setValue( QString::number( nGainValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nComponentId ) );
	pAction->setParameter3( QString::number( nLayerId ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pComponent = pInstrument->getComponent( nComponentId );
	CPPUNIT_ASSERT( pComponent != nullptr );
	auto pLayer = pComponent->getLayer( nLayerId );
	CPPUNIT_ASSERT( pLayer != nullptr );

	const float fOldValue = 0.92;
	pLayer->setGain( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nGainValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pLayer->getGain() != fOldValue );

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pLayer->getGain() )
					.arg( 5.0 * static_cast<float>( nGainValue ) / 127.0 ) );
	CPPUNIT_ASSERT(
		std::abs(
			pLayer->getGain() -
			5.0 * ( static_cast<float>( nGainValue ) / 127.0 )
		) <= 0.01
	);
	pLayer->setGain( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testInstrumentPitchAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPitchValue = 101;
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::InstrumentPitch );
	pAction->setValue( QString::number( nPitchValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setPitchOffset( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPitchValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getPitchOffset() != fOldValue );
	const float fRef = ( Instrument::fPitchMax - Instrument::fPitchMin ) *
						   ( static_cast<float>( nPitchValue ) / 127.0 ) +
					   Instrument::fPitchMin;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getPitchOffset() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPitchOffset() - fRef ) <= 0.01 );
	pInstrument->setPitchOffset( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testLoadNextDrumkitAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::LoadNextDrumkit );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pOldDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pOldDrumkit != nullptr );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	auto pNewDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( pNewDrumkit != pOldDrumkit );

	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pOldDrumkit ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testLoadPrevDrumkitAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::LoadPrevDrumkit );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pOldDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pOldDrumkit != nullptr );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	auto pNewDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pNewDrumkit != nullptr );
	CPPUNIT_ASSERT( pNewDrumkit != pOldDrumkit );

	CPPUNIT_ASSERT( CoreActionController::setDrumkit( pOldDrumkit ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testMasterVolumeAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const auto volumeValue = Midi::parameterFromInt( 102 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::MasterVolumeAbsolute );
	pAction->setValue( QString::number( static_cast<int>( volumeValue ) ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const float fOldValue = 0.92;
	pSong->setVolume( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, volumeValue,
		Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pSong->getVolume() != fOldValue );

	const float fRef = 1.5 * static_cast<float>( volumeValue ) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pSong->getVolume() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pSong->getVolume() - fRef ) <= 0.01 );
	pSong->setVolume( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testMasterVolumeRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const auto volumeValue = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::MasterVolumeRelative );
	pAction->setValue( QString::number( static_cast<int>( volumeValue ) ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const float fOldValue = 0.92;
	pSong->setVolume( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, volumeValue,
		Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pSong->getVolume() != fOldValue );

	const float fRef = fOldValue + 0.05;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pSong->getVolume() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pSong->getVolume() - fRef ) <= 0.01 );
	pSong->setVolume( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testMuteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Mute );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const bool bOldValue = false;
	pSong->setIsMuted( bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pSong->getIsMuted() != bOldValue );
	pSong->setIsMuted( bOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testMuteToggleAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::MuteToggle );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const bool bOldValue = false;
	pSong->setIsMuted( bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pSong->getIsMuted() != bOldValue );
	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pSong->getIsMuted() == bOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testNextBarAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::NextBar );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	const auto pPreviousSong = pHydrogen->getSong();
	const auto pNewSong = Song::getEmptySong();
	CPPUNIT_ASSERT( CoreActionController::setSong( pNewSong ) );

	const int nPatternNumber = 0;
	const int nColumn = 5;
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( true ) );
	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
		GridPoint( nColumn, nPatternNumber )
	) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pPatternGroupVector = pSong->getPatternGroupVector();
	___INFOLOG(
		QString( "number of columns: [%1]" ).arg( pPatternGroupVector->size() )
	);
	CPPUNIT_ASSERT( pPatternGroupVector->size() == nColumn + 1 );

	const int nOldValue = 2;
	CPPUNIT_ASSERT( CoreActionController::locateToColumn( nOldValue ) );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);

	pAudioEngine->lock( RIGHT_HERE );
	const int nNewValue = pAudioEngine->getTransportPosition()->getColumn();
	pAudioEngine->unlock();

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( nNewValue )
					.arg( nOldValue ) );
	CPPUNIT_ASSERT( nNewValue == nOldValue + 1 );

	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
		GridPoint( nColumn, nPatternNumber )
	) );
	CPPUNIT_ASSERT( CoreActionController::setSong( pPreviousSong ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testPanAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPanValue = 101;
	const int nInstrumentNumber = 2;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PanAbsolute );
	pAction->setValue( QString::number( nPanValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setPanWithRangeFrom0To1( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPanValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getPanWithRangeFrom0To1() != fOldValue );
	const float fRef = static_cast<float>( nPanValue ) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getPanWithRangeFrom0To1() )
					.arg( fRef ) );
	CPPUNIT_ASSERT(
		std::abs( pInstrument->getPanWithRangeFrom0To1() - fRef ) <= 0.01
	);
	pInstrument->setPanWithRangeFrom0To1( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testPanAbsoluteSymAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPanValue = 101;
	const int nInstrumentNumber = 2;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PanAbsoluteSym );
	pAction->setValue( QString::number( nPanValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setPan( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPanValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getPan() != fOldValue );
	const float fRef = static_cast<float>( nPanValue ) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getPan() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPan() - fRef ) <= 0.01 );
	pInstrument->setPan( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testPanRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPanValue = 1;
	const int nInstrumentNumber = 2;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PanRelative );
	pAction->setValue( QString::number( nPanValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.82;
	pInstrument->setPan( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPanValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getPan() != fOldValue );
	const float fRef = fOldValue + 0.1;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getPan() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getPan() - fRef ) <= 0.01 );
	pInstrument->setPan( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testPauseAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Pause );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->play();
	TestHelper::waitForAudioDriver();

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() != 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testPitchLevelAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPitchValue = 101;
	const int nInstrumentNumber = 3;
	const int nComponentId = 0;
	const int nLayerId = 0;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PitchLevelAbsolute );
	pAction->setValue( QString::number( nPitchValue ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pAction->setParameter2( QString::number( nComponentId ) );
	pAction->setParameter3( QString::number( nLayerId ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pComponent = pInstrument->getComponent( nComponentId );
	CPPUNIT_ASSERT( pComponent != nullptr );
	auto pLayer = pComponent->getLayer( nLayerId );
	CPPUNIT_ASSERT( pLayer != nullptr );

	const float fOldValue = 0.92;
	pLayer->setPitchOffset( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPitchValue ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pLayer->getPitchOffset() != fOldValue );

	const float fRef = ( Instrument::fPitchMax - Instrument::fPitchMin ) *
						   ( static_cast<float>( nPitchValue ) / 127.0 ) +
					   Instrument::fPitchMin;

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pLayer->getPitchOffset() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pLayer->getPitchOffset() - fRef ) <= 0.01 );
	pLayer->setPitchOffset( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testPlayAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Play );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->stop();
	TestHelper::waitForAudioDriver();

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	pAudioEngine->stop();
	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testPlaylistNextSongAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pPreviousSong = pHydrogen->getSong();
	auto pPreviousPlaylist = pHydrogen->getPlaylist();

	auto pPlaylist = CoreActionController::loadPlaylist(
		H2TEST_FILE( "/playlist/test.h2playlist" )
	);
	CPPUNIT_ASSERT( pPlaylist != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setPlaylist( pPlaylist ) );
	const int nOldSongNumber = 0;
	CPPUNIT_ASSERT( CoreActionController::loadSong(
		pPlaylist->getSongFileNameByNumber( nOldSongNumber )
	) );
	CPPUNIT_ASSERT( CoreActionController::activatePlaylistSong( nOldSongNumber )
	);

	auto pOldSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pOldSong != nullptr );
	const auto sOldSongName = pOldSong->getName();

	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistNextSong );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);

	auto pNewSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	CPPUNIT_ASSERT( pNewSong != pOldSong );
	const auto sNewSongName = pNewSong->getName();
	CPPUNIT_ASSERT( sNewSongName != sOldSongName );

	CPPUNIT_ASSERT( CoreActionController::setPlaylist( pPreviousPlaylist ) );
	CPPUNIT_ASSERT( CoreActionController::setSong( pPreviousSong ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testPlaylistPrevSongAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pPreviousSong = pHydrogen->getSong();
	auto pPreviousPlaylist = pHydrogen->getPlaylist();

	auto pPlaylist = CoreActionController::loadPlaylist(
		H2TEST_FILE( "/playlist/test.h2playlist" )
	);
	CPPUNIT_ASSERT( pPlaylist != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setPlaylist( pPlaylist ) );
	const int nOldSongNumber = 1;
	CPPUNIT_ASSERT( CoreActionController::loadSong(
		pPlaylist->getSongFileNameByNumber( nOldSongNumber )
	) );
	CPPUNIT_ASSERT( CoreActionController::activatePlaylistSong( nOldSongNumber )
	);

	auto pOldSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pOldSong != nullptr );
	const auto sOldSongName = pOldSong->getName();

	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistPrevSong );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);

	auto pNewSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	CPPUNIT_ASSERT( pNewSong != pOldSong );
	const auto sNewSongName = pNewSong->getName();
	CPPUNIT_ASSERT( sNewSongName != sOldSongName );

	CPPUNIT_ASSERT( CoreActionController::setPlaylist( pPreviousPlaylist ) );
	CPPUNIT_ASSERT( CoreActionController::setSong( pPreviousSong ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testPlaylistSongAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pPreviousSong = pHydrogen->getSong();
	auto pPreviousPlaylist = pHydrogen->getPlaylist();

	auto pPlaylist = CoreActionController::loadPlaylist(
		H2TEST_FILE( "/playlist/test.h2playlist" )
	);
	CPPUNIT_ASSERT( pPlaylist != nullptr );
	CPPUNIT_ASSERT( CoreActionController::setPlaylist( pPlaylist ) );
	const int nOldSongNumber = 0;
	const int nNewSongNumber = 1;
	CPPUNIT_ASSERT( CoreActionController::loadSong(
		pPlaylist->getSongFileNameByNumber( nOldSongNumber )
	) );
	CPPUNIT_ASSERT( CoreActionController::activatePlaylistSong( nOldSongNumber )
	);

	auto pOldSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pOldSong != nullptr );
	const auto sOldSongName = pOldSong->getName();

	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameterOldSong = Midi::parameterFromInt( 1 );
	auto pActionOldSong =
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistSong );
	pActionOldSong->setParameter1( QString::number( nOldSongNumber ) );
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, parameterOldSong, pActionOldSong
	);

	const auto parameterNewSong = Midi::parameterFromInt( 2 );
	auto pActionNewSong =
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistSong );
	pActionNewSong->setParameter1( QString::number( nNewSongNumber ) );
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, parameterNewSong, pActionNewSong
	);

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameterNewSong,
		Midi::ParameterMinimum, Midi::ChannelDefault
	) );

	auto pNewSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pNewSong != nullptr );
	CPPUNIT_ASSERT( pNewSong != pOldSong );
	const auto sNewSongName = pNewSong->getName();
	___INFOLOG(
		QString( "[%1] -> [%2]" ).arg( sOldSongName ).arg( sNewSongName )
	);
	CPPUNIT_ASSERT( sNewSongName != sOldSongName );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameterOldSong,
		Midi::ParameterMinimum, Midi::ChannelDefault
	) );

	auto pNewSongRevert = pHydrogen->getSong();
	CPPUNIT_ASSERT( pNewSongRevert != nullptr );
	CPPUNIT_ASSERT( pNewSongRevert != pNewSong );
	CPPUNIT_ASSERT( pNewSongRevert != pOldSong );
	const auto sNewSongNameRevert = pNewSongRevert->getName();
	___INFOLOG(
		QString( "[%1] -> [%2]" ).arg( sNewSongName ).arg( sNewSongNameRevert )
	);
	CPPUNIT_ASSERT( sNewSongNameRevert != sNewSongName );
	CPPUNIT_ASSERT( sNewSongNameRevert != sOldSongName );

	CPPUNIT_ASSERT( CoreActionController::setPlaylist( pPreviousPlaylist ) );
	CPPUNIT_ASSERT( CoreActionController::setSong( pPreviousSong ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testPlayPauseToggleAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PlayPauseToggle );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->stop();
	TestHelper::waitForAudioDriver();

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() != 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testPlayStopToggleAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PlayStopToggle );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->stop();
	TestHelper::waitForAudioDriver();

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() == 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testPreviousBarAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::PreviousBar );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	const auto pPreviousSong = pHydrogen->getSong();
	const auto pNewSong = Song::getEmptySong();
	CPPUNIT_ASSERT( CoreActionController::setSong( pNewSong ) );

	const int nPatternNumber = 0;
	const int nColumn = 4;
	CPPUNIT_ASSERT( CoreActionController::activateSongMode( true ) );
	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
		GridPoint( nColumn, nPatternNumber )
	) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pPatternGroupVector = pSong->getPatternGroupVector();
	___INFOLOG(
		QString( "number of columns: [%1]" ).arg( pPatternGroupVector->size() )
	);
	CPPUNIT_ASSERT( pPatternGroupVector->size() == ( nColumn + 1 ) );

	const int nOldValue = 2;
	CPPUNIT_ASSERT( CoreActionController::locateToColumn( nOldValue ) );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);

	pAudioEngine->lock( RIGHT_HERE );
	const int nNewValue = pAudioEngine->getTransportPosition()->getColumn();
	pAudioEngine->unlock();

	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( nNewValue )
					.arg( nOldValue ) );
	CPPUNIT_ASSERT( nNewValue == nOldValue - 1 );

	CPPUNIT_ASSERT( CoreActionController::toggleGridCell(
		GridPoint( nColumn, nPatternNumber )
	) );
	CPPUNIT_ASSERT( CoreActionController::setSong( pPreviousSong ) );

	___INFOLOG( "done" );
}

void MidiActionTest::testRecordExitAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	pHydrogen->setRecordEnabled( true );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::RecordExit );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( !pHydrogen->getRecordEnabled() );

	___INFOLOG( "done" );
}

void MidiActionTest::testRecordReadyAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	pHydrogen->setRecordEnabled( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::RecordReady );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pHydrogen->getRecordEnabled() );
	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( !pHydrogen->getRecordEnabled() );

	pHydrogen->sequencerPlay();
	TestHelper::waitForAudioDriver();

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( !pHydrogen->getRecordEnabled() );
	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( !pHydrogen->getRecordEnabled() );

	pHydrogen->sequencerStop();

	___INFOLOG( "done" );
}

void MidiActionTest::testRecordStrobeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	pHydrogen->setRecordEnabled( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::RecordStrobe );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pHydrogen->getRecordEnabled() );

	pHydrogen->setRecordEnabled( false );

	___INFOLOG( "done" );
}

void MidiActionTest::testRecordStrobeToggleAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	pHydrogen->setRecordEnabled( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::RecordStrobeToggle );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pHydrogen->getRecordEnabled() );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( !pHydrogen->getRecordEnabled() );

	___INFOLOG( "done" );
}

void MidiActionTest::testRedoAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pEventQueue = EventQueue::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	pHydrogen->setRecordEnabled( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::RedoAction );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( nnTry <= nMaxTries ) {
		const auto pEvent = pEventQueue->popEvent();
		if ( pEvent != nullptr ) {
			if ( pEvent->getType() == Event::Type::UndoRedo ) {
				break;
			}
		}
		else {
			++nnTry;
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		}
	}
	CPPUNIT_ASSERT( nnTry <= nMaxTries );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectAndPlayPatternAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedPatternNumber = 2;
	pHydrogen->setSelectedPatternNumber( nOldSelectedPatternNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPatternNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::SelectAndPlayPattern );
	pAction->setParameter1( QString::number( nPatternNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pHydrogen->sequencerStop();
	TestHelper::waitForAudioDriver();

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() != 0 );
	CPPUNIT_ASSERT( pHydrogen->getSelectedPatternNumber() == nPatternNumber );

	pHydrogen->sequencerStop();

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectInstrumentAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedInstrumentNumber = 2;
	pHydrogen->setSelectedInstrumentNumber( nOldSelectedInstrumentNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::SelectInstrument );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nInstrumentNumber ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT(
		pHydrogen->getSelectedInstrumentNumber() == nInstrumentNumber
	);

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectNextPatternAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedPatternNumber = 2;
	pHydrogen->setSelectedPatternNumber( nOldSelectedPatternNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPatternNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::SelectNextPattern );
	pAction->setParameter1( QString::number( nPatternNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pHydrogen->getSelectedPatternNumber() == nPatternNumber );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectNextPatternCcAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedPatternNumber = 2;
	pHydrogen->setSelectedPatternNumber( nOldSelectedPatternNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPatternNumber = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectNextPatternCcAbsolute
	);
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPatternNumber ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pHydrogen->getSelectedPatternNumber() == nPatternNumber );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectNextPatternRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedPatternNumber = 2;
	pHydrogen->setSelectedPatternNumber( nOldSelectedPatternNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPatternNumber = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectNextPatternRelative
	);
	pAction->setParameter1( QString::number( nPatternNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT(
		pHydrogen->getSelectedPatternNumber() ==
		nOldSelectedPatternNumber + nPatternNumber
	);

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectOnlyNextPatternAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedPatternNumber = 2;
	pHydrogen->setSelectedPatternNumber( nOldSelectedPatternNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPatternNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::SelectOnlyNextPattern );
	pAction->setParameter1( QString::number( nPatternNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pHydrogen->getSelectedPatternNumber() == nPatternNumber );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testSelectOnlyNextPatternCcAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );

	const int nOldSelectedPatternNumber = 2;
	pHydrogen->setSelectedPatternNumber( nOldSelectedPatternNumber );

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nPatternNumber = 2;
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectOnlyNextPatternCcAbsolute
	);
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter,
		Midi::parameterFromInt( nPatternNumber ), Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pHydrogen->getSelectedPatternNumber() == nPatternNumber );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testStopAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	auto pDriver =
		dynamic_cast<FakeAudioDriver*>( pAudioEngine->getAudioDriver() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	CoreActionController::activateSongMode( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Stop );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	pAudioEngine->play();
	TestHelper::waitForAudioDriver();

	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Playing );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	TestHelper::waitForAudioDriver();
	CPPUNIT_ASSERT( pAudioEngine->getState() == AudioEngine::State::Ready );
	CPPUNIT_ASSERT( pTransportPosition->getFrame() == 0 );

	CoreActionController::activateSongMode( true );

	___INFOLOG( "done" );
}

void MidiActionTest::testStripMuteToggleAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::StripMuteToggle );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const bool bOldValue = false;
	pInstrument->setMuted( bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pInstrument->isMuted() != bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pInstrument->isMuted() == bOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testStripSoloToggleAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::StripSoloToggle );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const bool bOldValue = false;
	pInstrument->setSoloed( bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pInstrument->isSoloed() != bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pInstrument->isSoloed() == bOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testStripVolumeAbsoluteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const auto volumeValue = Midi::parameterFromInt( 101 );
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::StripVolumeAbsolute );
	pAction->setValue( QString::number( static_cast<int>( volumeValue ) ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setVolume( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, volumeValue,
		Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getVolume() != fOldValue );
	const float fRef = 1.5 * static_cast<float>( volumeValue ) / 127.0;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getVolume() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getVolume() - fRef ) <= 0.01 );
	pInstrument->setVolume( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testStripVolumeRelativeAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	const auto volumeValue = Midi::parameterFromInt( 1 );
	const int nInstrumentNumber = 3;
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::StripVolumeRelative );
	pAction->setValue( QString::number( static_cast<int>( volumeValue ) ) );
	pAction->setParameter1( QString::number( nInstrumentNumber ) );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	const float fOldValue = 0.92;
	pInstrument->setVolume( fOldValue );

	sendMessage( MidiMessage(
		MidiMessage::Type::ControlChange, parameter, volumeValue,
		Midi::ChannelDefault
	) );
	CPPUNIT_ASSERT( pInstrument->getVolume() != fOldValue );
	const float fRef = fOldValue + 0.1;
	___INFOLOG( QString( "new value: [%1], ref: [%2]" )
					.arg( pInstrument->getVolume() )
					.arg( fRef ) );
	CPPUNIT_ASSERT( std::abs( pInstrument->getVolume() - fRef ) <= 0.01 );
	pInstrument->setVolume( fOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testTapTempoAction()
{
	___INFOLOG( "" );

	auto pTestHelper = TestHelper::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pPref = Preferences::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::ParameterMinimum;
	pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, parameter,
		std::make_shared<MidiAction>( MidiAction::Type::TapTempo )
	);
	pPref->m_bpmTap = Preferences::BpmTap::TapTempo;
	pPref->m_beatCounter = Preferences::BeatCounter::Tap;

	// Since we do not have a proper audio driver here picking up the new BPM
	// during the next process cycle, we just check whether the next value did
	// change.
	pAudioEngine->lock( RIGHT_HERE );
	const auto fOldBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	const float fTargetBpm = 378.4;
	const float fIntervalMs = 60000.0 / fTargetBpm;
	const auto interval =
		std::chrono::duration<float, std::milli>{ fIntervalMs };
	CPPUNIT_ASSERT( fTargetBpm < MAX_BPM );

	___DEBUGLOG( QString( "interval: %1" ).arg( fIntervalMs ) );

	const auto tapTempoMessage =
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault );

	auto intervalCompensation = std::chrono::duration<float, std::milli>( 0 );
	for ( int nnMessage = 0; nnMessage < 8; ++nnMessage ) {
		const auto preSleep = Clock::now();
		pHydrogen->getTimeHelper()->highResolutionSleep(
			interval - intervalCompensation
		);
		const auto postSleep = Clock::now();

		const auto preSend = Clock::now();
		sendMessage( tapTempoMessage );
		const auto postSend = Clock::now();

		// Compensate the additional time spent during sleep as well as while
		// sending the MIDI message.
		intervalCompensation =
			( postSleep - preSleep - ( interval - intervalCompensation ) ) +
			( postSend - preSend );
	}

	// Wait for the audio engine to adopt the new tempo.
	TestHelper::waitForAudioDriver();

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewBpm = pTransportPosition->getBpm();
	pAudioEngine->unlock();

	const float fTolerance = 1;
	___INFOLOG( QString( "[%1] -> [%2] target [%3 +/- %4]" )
					.arg( fOldBpm )
					.arg( fNewBpm )
					.arg( fTargetBpm )
					.arg( fTolerance ) );
	CPPUNIT_ASSERT( fNewBpm != fOldBpm );
	if ( !pTestHelper->isAppveyor() ) {
		CPPUNIT_ASSERT( std::abs( fTargetBpm - fNewBpm ) < fTolerance );
	}

	___INFOLOG( "done" );
}

void MidiActionTest::testToggleMetronomeAction()
{
	___INFOLOG( "" );

	auto pPref = Preferences::get_instance();
	auto pMidiEventMap = pPref->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction =
		std::make_shared<MidiAction>( MidiAction::Type::ToggleMetronome );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	const bool bOldValue = false;
	pPref->m_bUseMetronome = bOldValue;

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pPref->m_bUseMetronome != bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pPref->m_bUseMetronome == bOldValue );

	___INFOLOG( "done" );
}

void MidiActionTest::testUndoAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pEventQueue = EventQueue::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	pHydrogen->setRecordEnabled( false );

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::UndoAction );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( nnTry <= nMaxTries ) {
		const auto pEvent = pEventQueue->popEvent();
		if ( pEvent != nullptr ) {
			if ( pEvent->getType() == Event::Type::UndoRedo ) {
				break;
			}
		}
		else {
			++nnTry;
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		}
	}
	CPPUNIT_ASSERT( nnTry <= nMaxTries );

	___INFOLOG( "done" );
}

void MidiActionTest::testUnmuteAction()
{
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	pMidiEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::Unmute );
	pMidiEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction );

	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );

	const bool bOldValue = true;
	pSong->setIsMuted( bOldValue );

	sendMessage(
		MidiMessage( MidiMessage::Type::ControlChange, parameter, Midi::ParameterMinimum, Midi::ChannelDefault )
	);
	CPPUNIT_ASSERT( pSong->getIsMuted() != bOldValue );
	pSong->setIsMuted( false );

	___INFOLOG( "done" );
}

void MidiActionTest::sendMessage( const MidiMessage& msg )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	auto pDriver =
		dynamic_cast<LoopBackMidiDriver*>( pAudioEngine->getMidiDriver().get()
		);
	CPPUNIT_ASSERT( pDriver != nullptr );

	pDriver->clearBacklogMessages();
	pDriver->sendMessage( msg );

	// Wait till the LoopBackMidiDriver did send, receive, and handle the
	// message.
	TestHelper::waitForMidiDriver();
	TestHelper::waitForMidiActionManagerWorkerThread();
}
