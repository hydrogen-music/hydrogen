/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "PluginMidiTest.h"

#include "utils/FakePluginHost.h"

#include <core/Hydrogen.h>
#include <core/IO/MidiInput.h>
#include <core/IO/PluginMidiDriver.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <vector>

using namespace H2Core;

// A host Note-On injected for this block must run the full input pipeline and
// map to an instrument of the loaded kit. With the instrument map in "Order"
// mode, note NoteOffset (36) maps to the first instrument.
void PluginMidiTest::testNoteOnMapsToInstrument() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto* pDriver = host.getMidiDriver();
	CPPUNIT_ASSERT( pDriver != nullptr );

	// "Order" mode maps incoming notes to instruments by their position in the
	// kit (note NoteOffset → instrument 0). Pin the input channel so the test is
	// independent of the loaded preferences' default channel.
	auto pMap = host.getHydrogen()->getPreferences()->getMidiInstrumentMap();
	pMap->setInput( MidiInstrumentMap::Input::Order );
	pMap->setUseGlobalInputChannel( true );
	pMap->setGlobalInputChannel( Midi::ChannelDefault );
	pDriver->clearHandledInput();

	const int nKey = static_cast<int>( Midi::NoteOffset );
	host.addNoteOn( 0, nKey, 100 );
	host.process( 256 );

	const auto handled = pDriver->getHandledInputs();
	bool bFound = false;
	for ( const auto& pInput : handled ) {
		if ( pInput != nullptr &&
			 pInput->type == MidiMessage::Type::NoteOn &&
			 static_cast<int>( pInput->data1 ) == nKey ) {
			bFound = true;
			// The note reached the engine and triggered an instrument.
			CPPUNIT_ASSERT( ! pInput->mappedInstruments.isEmpty() );
		}
	}
	CPPUNIT_ASSERT( bFound );

	___INFOLOG( "passed" );
}

// The velocity carried by a host Note-On must survive into the handled input.
void PluginMidiTest::testNoteVelocity() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto* pDriver = host.getMidiDriver();
	pDriver->clearHandledInput();

	const int nKey = static_cast<int>( Midi::NoteOffset );
	const int nVelocity = 77;
	host.addNoteOn( 0, nKey, nVelocity );
	host.process( 256 );

	bool bFound = false;
	for ( const auto& pInput : pDriver->getHandledInputs() ) {
		if ( pInput != nullptr &&
			 pInput->type == MidiMessage::Type::NoteOn &&
			 static_cast<int>( pInput->data1 ) == nKey ) {
			CPPUNIT_ASSERT_EQUAL( nVelocity, static_cast<int>( pInput->data2 ) );
			bFound = true;
		}
	}
	CPPUNIT_ASSERT( bFound );

	___INFOLOG( "passed" );
}

// A host Control-Change mapped to a MIDI action must trigger that action.
void PluginMidiTest::testControlChangeAction() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto* pDriver = host.getMidiDriver();

	auto pEventMap = host.getHydrogen()->getPreferences()->getMidiEventMap();
	pEventMap->reset();

	const auto parameter = Midi::parameterFromInt( 1 );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmIncr );
	pEventMap->registerEvent( MidiEvent::Type::CC, parameter, pAction,
							  Event::Trigger::Suppress, host.getHydrogen() );

	pDriver->clearHandledInput();
	host.addControlChange( 0, 1, 64 );
	host.process( 256 );

	bool bActionSeen = false;
	for ( const auto& pInput : pDriver->getHandledInputs() ) {
		if ( pInput != nullptr &&
			 pInput->type == MidiMessage::Type::ControlChange ) {
			for ( const auto& actionType : pInput->actionTypes ) {
				if ( actionType == MidiAction::Type::BpmIncr ) {
					bActionSeen = true;
				}
			}
		}
	}
	CPPUNIT_ASSERT( bActionSeen );

	pEventMap->reset();

	___INFOLOG( "passed" );
}

// Events queued in arbitrary order must be dispatched in ascending sample-offset
// order within the block.
void PluginMidiTest::testEventsDispatchInOffsetOrder() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto* pDriver = host.getMidiDriver();

	host.getHydrogen()->getPreferences()->getMidiInstrumentMap()->setInput(
		MidiInstrumentMap::Input::Order );
	pDriver->clearHandledInput();

	// Enqueue out of order; expected dispatch order by offset is 36, 37, 38.
	host.addNoteOn( 200, 38, 100 );
	host.addNoteOn( 10, 36, 100 );
	host.addNoteOn( 100, 37, 100 );
	host.process( 256 );

	std::vector<int> noteOrder;
	for ( const auto& pInput : pDriver->getHandledInputs() ) {
		if ( pInput != nullptr && pInput->type == MidiMessage::Type::NoteOn ) {
			noteOrder.push_back( static_cast<int>( pInput->data1 ) );
		}
	}

	CPPUNIT_ASSERT_EQUAL( (size_t)3, noteOrder.size() );
	CPPUNIT_ASSERT_EQUAL( 36, noteOrder[0] );
	CPPUNIT_ASSERT_EQUAL( 37, noteOrder[1] );
	CPPUNIT_ASSERT_EQUAL( 38, noteOrder[2] );

	___INFOLOG( "passed" );
}

// process() must drain the host event queue: events apply to exactly one block.
void PluginMidiTest::testQueueDrainedAfterProcess() {
	___INFOLOG( "" );

	FakePluginHost host( 44100, 256 );
	auto* pDriver = host.getMidiDriver();

	host.addNoteOn( 0, 36, 100 );
	host.addNoteOn( 0, 38, 100 );
	CPPUNIT_ASSERT_EQUAL( (size_t)2, pDriver->getHostEventCount() );
	CPPUNIT_ASSERT_EQUAL( (size_t)2, host.getMidiEvents().size() );

	host.process( 256 );

	CPPUNIT_ASSERT_EQUAL( (size_t)0, pDriver->getHostEventCount() );
	CPPUNIT_ASSERT( host.getMidiEvents().empty() );

	___INFOLOG( "passed" );
}
