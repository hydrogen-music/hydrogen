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

#include "MidiDriverTest.h"

#include <chrono>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Event.h>
#include <core/Hydrogen.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiMessage.h>

void MidiDriverTest::setUp() {
	auto pPref = H2Core::Preferences::get_instance();
	m_previousDriver = pPref->m_midiDriver;
	pPref->m_midiDriver = H2Core::Preferences::MidiDriver::LoopBack;

	pPref->setMidiClockInputHandling( true );
	// We do not want the AudioEngine itself to adjust the TimingClock based on
	// tempo changes but want to do it ourselves in here.
	pPref->setMidiClockOutputSend( false );

	auto pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	pAudioEngine->stopMidiDriver( H2Core::Event::Trigger::Suppress );
	pAudioEngine->startMidiDriver( H2Core::Event::Trigger::Suppress );
}

void MidiDriverTest::tearDown() {
	auto pPref = H2Core::Preferences::get_instance();
	pPref->m_midiDriver = m_previousDriver;

	pPref->setMidiClockInputHandling( false );
	pPref->setMidiClockOutputSend( false );

	auto pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	pAudioEngine->stopMidiDriver( H2Core::Event::Trigger::Suppress );
	pAudioEngine->startMidiDriver( H2Core::Event::Trigger::Suppress );
}

void MidiDriverTest::testLoopBackMidiDriver() {
	___INFOLOG("");

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	auto pDriver = dynamic_cast<H2Core::LoopBackMidiDriver*>(
		pAudioEngine->getMidiDriver().get() );
	CPPUNIT_ASSERT( pDriver != nullptr );

	// Ensure the MIDI driver is ready. Else, we would loose some initial
	// messages on slow machines/OSs.
	const int nMaxTries = 200;
	int nnTry = 0;
	while ( ! pDriver->isOutputActive() ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );

		++nnTry;
		if ( nnTry > nMaxTries ) {
			break;
		}
	}
	CPPUNIT_ASSERT( nnTry <= nMaxTries );

	// We send more MIDI output messages than the queue can hold. Afterwards, we
	// expect all messages sent to be found in the backlog.
	std::vector<H2Core::MidiMessage> messages;
	messages.reserve( H2Core::LoopBackMidiDriver::nMaxQueueSize * 2 );
	for ( int ii = 0; ii < H2Core::LoopBackMidiDriver::nMaxQueueSize * 2; ++ii ) {
		messages.push_back( H2Core::MidiMessage(
								H2Core::MidiMessage::Type::NoteOff, ii, ii, 0 ) );
	}

	for ( const auto& mmessage : messages ) {
		pDriver->sendMessage( H2Core::MidiMessage( mmessage ) );
		// On Windows we need an additional sleep or the LoopBackMidiDriver has
		// no chance to acquire its mutex and its message queue will run over.
		std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
	}

	// Ensure all messages have been properly handled. Again, this is required
	// for slow machines (Windows ones).
	nnTry = 0;
	while ( pDriver->getMessageQueueSize() > 0 ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

		++nnTry;
		if ( nnTry > nMaxTries ) {
			break;
		}
	}
	CPPUNIT_ASSERT( nnTry <= nMaxTries );

	const auto backlogMessages = pDriver->getBacklogMessages();
	CPPUNIT_ASSERT( backlogMessages.size() == messages.size() );
	for ( int ii = 0; ii < messages.size(); ++ii ) {
		CPPUNIT_ASSERT( messages[ ii ] == backlogMessages[ ii ] );
	}

	___INFOLOG("done");
}

void MidiDriverTest::testMidiClock() {
	___INFOLOG("");

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTransportPosition = pAudioEngine->getTransportPosition();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	auto pMidiDriver = dynamic_cast<H2Core::LoopBackMidiDriver*>(
		pAudioEngine->getMidiDriver().get() );
	CPPUNIT_ASSERT( pMidiDriver != nullptr );

	const std::vector<float> referenceTempos{
		80.0, 123.4, 210.1 };
	const float fTolerance = 8;

	for ( const auto& ffTempo : referenceTempos ) {
		pAudioEngine->lock( RIGHT_HERE );
		const auto fOldBpm = pTransportPosition->getBpm();
		pAudioEngine->unlock();

		pMidiDriver->startMidiClockStream( ffTempo );

		const int nMaxTries = 50;
		int nnTry = 0;
		float fCurrentBpm;
		// Wait till we received enough ticks to synchronize.
		while ( nnTry < nMaxTries ) {
			pAudioEngine->lock( RIGHT_HERE );
			fCurrentBpm = pTransportPosition->getBpm();
			pAudioEngine->unlock();

			if ( std::abs( fCurrentBpm - fOldBpm ) > fTolerance ) {
				break;
			}

			std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
			++nnTry;
		}
		CPPUNIT_ASSERT( nnTry < nMaxTries );

		if ( std::abs( fCurrentBpm - ffTempo ) >= fTolerance ) {
			___ERRORLOG( QString( "Current tempo [%1] exceeds references [%2] by [%3 (%4 tolerance)]" )
						 .arg( fCurrentBpm ).arg( ffTempo )
						 .arg( std::abs( fCurrentBpm - ffTempo ) )
						 .arg( fTolerance ) );
		}
		CPPUNIT_ASSERT( std::abs( fCurrentBpm - ffTempo ) < fTolerance );

		pMidiDriver->stopMidiClockStream();

		pAudioEngine->lock( RIGHT_HERE );
		pMidiActionManager->resetTimingClockTicks();
		// Flush any BPM changes that are still transient.
		pAudioEngine->setNextBpm( fCurrentBpm );
		pAudioEngine->unlock();
	}

	___INFOLOG("done");
}
