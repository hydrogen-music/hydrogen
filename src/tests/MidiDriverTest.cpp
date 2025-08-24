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

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
#include <core/Hydrogen.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Midi/MidiMessage.h>

void MidiDriverTest::setUp() {
	auto pPref = H2Core::Preferences::get_instance();
	m_previousDriver = pPref->m_midiDriver;
	pPref->m_midiDriver = H2Core::Preferences::MidiDriver::LoopBack;

	auto pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	pAudioEngine->stopMidiDriver( H2Core::Event::Trigger::Suppress );
	pAudioEngine->startMidiDriver( H2Core::Event::Trigger::Suppress );
}

void MidiDriverTest::tearDown() {
	auto pPref = H2Core::Preferences::get_instance();
	pPref->m_midiDriver = m_previousDriver;

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
	}

	const auto backlogMessages = pDriver->getBacklogMessages();
	CPPUNIT_ASSERT( backlogMessages.size() == messages.size() );
	for ( int ii = 0; ii < messages.size(); ++ii ) {
		CPPUNIT_ASSERT( messages[ ii ] == backlogMessages[ ii ] );
	}

	___INFOLOG("done");
}
