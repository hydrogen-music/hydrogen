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

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
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

	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	auto pDriver = dynamic_cast<LoopBackMidiDriver*>(
		pAudioEngine->getMidiDriver().get() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	////////////////////////////////////////////////////////////////////////////
	// Setting up MIDI action mappings. We use CC event with various values.
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
	pDriver->sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	pDriver->sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	pDriver->sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	pDriver->sendMessage( beatCounterMessage );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	pAudioEngine->lock( RIGHT_HERE );
	const auto fNewTempoBC = pAudioEngine->getNextBpm();
	pAudioEngine->unlock();

	___INFOLOG( QString( "[%1] -> [%2]" ).arg( fOldTempoBC ).arg( fNewTempoBC ) );
	CPPUNIT_ASSERT( fNewTempoBC != fOldTempoBC );

	___INFOLOG("done");
}
