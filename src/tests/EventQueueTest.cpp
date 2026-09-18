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

#include "EventQueueTest.h"

#include "TestHelper.h"

#include <core/Hydrogen.h>

#include <pthread.h>

using namespace H2Core;

static void *pushThread(void *p) {
	int *pInt = (int *)p;
	auto pEventQueue = pTestEventQueue();
	for ( int i = 0; i < EventQueueTest::nCountsPerThread; i++) {
		pEventQueue->pushEvent( Event::Type::Metronome, *pInt );
	}
	return nullptr;
}

/** Entries pushed by the concurrent-drain test below. Kept larger than a
 * single serve/timer cycle so the producer is still pushing while the
 * main thread drains — the overlap the lock has to survive. */
static constexpr int nMidiNotesToPush = 2000;

static void *pushMidiNoteThread(void *) {
	auto pEventQueue = pTestEventQueue();
	for ( int i = 0; i < nMidiNotesToPush; i++ ) {
		EventQueue::AddMidiNoteVector noteAction;
		noteAction.nColumn = i;
		noteAction.id = static_cast<Instrument::Id>( 0 );
		noteAction.nPattern = 0;
		noteAction.nLength = -1;
		noteAction.fVelocity = 0.5;
		noteAction.fPan = 0.0;
		noteAction.key = static_cast<Note::Key>( 0 );
		noteAction.octave = static_cast<Note::Octave>( 0 );
		noteAction.bNoteOff = false;
		pEventQueue->pushMidiNoteAction( noteAction );
	}
	return nullptr;
}

void EventQueueTest::setUp() {
	auto pEventQueue = pTestEventQueue();
	pEventQueue->setSilent( false );

	auto pEvent = pEventQueue->popEvent();
	// Clear queue of any events from previous tests.
	do {
		pEvent = pEventQueue->popEvent();
	} while ( pEvent != nullptr );
}

void EventQueueTest::tearDown() {
	pTestEventQueue()->setSilent( true );
}
	
void EventQueueTest::testPushPop() {
	___INFOLOG( "" );
	auto pEventQueue = pTestEventQueue();
	std::unique_ptr<Event> pEvent;

	// Fill the event queue to the maximum permissible size, drain the queue
	// and then do it again.
	for ( int pass = 0; pass < 2; pass++) {
		for ( int i = 0; i < EventQueue::nMaxEvents; i++ ) {
			pEventQueue->pushEvent( Event::Type::AudioExportProgress, i );
		}
		for ( int i = 0; i < EventQueue::nMaxEvents; i++ ) {
			pEvent = pEventQueue->popEvent();
			CPPUNIT_ASSERT( pEvent != nullptr );
			CPPUNIT_ASSERT( pEvent->getType() == Event::Type::AudioExportProgress &&
							pEvent->getValue() == i );
		}

		// Queue should now be empty
		pEvent = pEventQueue->popEvent();
		CPPUNIT_ASSERT( pEvent == nullptr );
	}
	___INFOLOG( "passed" );
}

void EventQueueTest::testOverflow() {
	___INFOLOG( "" );
	auto pEventQueue = pTestEventQueue();
	std::unique_ptr<Event> pEvent;

	// Overfill queue
	for ( int i = 0; i < EventQueue::nMaxEvents + 100; i++) {
		pEventQueue->pushEvent( Event::Type::AudioExportProgress, i );
	}
	// Check that the queue contains the most recent EventQueue::nMaxEvents
	// events
	for ( int i = 0; i < EventQueue::nMaxEvents; i++) {
		pEvent = pEventQueue->popEvent();
		CPPUNIT_ASSERT( pEvent != nullptr );
		CPPUNIT_ASSERT( pEvent->getType() == Event::Type::AudioExportProgress &&
						pEvent->getValue() == i + 100);
	}
	pEvent = pEventQueue->popEvent();
	CPPUNIT_ASSERT( pEvent == nullptr );
	___INFOLOG( "passed" );
}

void EventQueueTest::testThreadedAccess() {
	___INFOLOG( "" );
	auto pEventQueue = pTestEventQueue();

	pthread_t threads[ EventQueueTest::nThreads ];
	int counters[ EventQueueTest::nThreads ];
	int threadIds[ EventQueueTest::nThreads ];

	for ( int i = 0; i < EventQueueTest::nThreads; i++) {
		counters[ i ] = 0;
		threadIds[ i ] = i;
	}

	// Start writer threads
	for ( int i = 0; i < EventQueueTest::nThreads; i++ ) {
		int nRetVal = pthread_create(
			&threads[ i ], nullptr, pushThread, &threadIds[ i ]);
	}

	// Reader counts up the number of events from each thread
	for ( int nTotalEvents = 0; nTotalEvents < EventQueueTest::nCountsPerThread * EventQueueTest::nThreads; ) {
		auto pEvent = pEventQueue->popEvent();
		if ( pEvent != nullptr &&
			 pEvent->getType() == Event::Type::Metronome ) {
			CPPUNIT_ASSERT( pEvent->getValue() < EventQueueTest::nThreads &&
							pEvent->getValue() >= 0 );
			counters[ pEvent->getValue() ]++;
			CPPUNIT_ASSERT( pEvent->getValue() <= EventQueueTest::nCountsPerThread );
			nTotalEvents++;
		}
		else {
			CPPUNIT_ASSERT( pEvent == nullptr );
		}
	}

	for ( int i = 0; i < EventQueueTest::nThreads; i++ ) {
		CPPUNIT_ASSERT( counters[i] == EventQueueTest::nCountsPerThread );
	}
	auto pEvent = pEventQueue->popEvent();
	CPPUNIT_ASSERT( pEvent == nullptr );
	___INFOLOG( "passed" );
}

void EventQueueTest::testEventDrop() {
	___INFOLOG( "" );
	auto pEventQueue = pTestEventQueue();
	std::unique_ptr<Event> pEvent;
	// Clear queue of any events from previous tests.
	do {
		pEvent = pEventQueue->popEvent();
	} while ( pEvent != nullptr );
	pEventQueue->dropEvents( Event::Type::TempoChanged );

	// Fill queue with different events
	const int nEvents = 20;
	for ( int ii = 0; ii < nEvents; ii++ ) {
		if ( ii % 2 == 0 ) {
			pEventQueue->pushEvent( Event::Type::AudioExportProgress, ii );
		}
		else {
			pEventQueue->pushEvent( Event::Type::TempoChanged, ii );
		}
	}

	pEventQueue->dropEvents( Event::Type::TempoChanged );

	for ( int ii = 0; ii < nEvents / 2; ii++) {
		pEvent = pEventQueue->popEvent();
		CPPUNIT_ASSERT( pEvent->getType() == Event::Type::AudioExportProgress );
	}

	pEvent = pEventQueue->popEvent();
	CPPUNIT_ASSERT( pEvent == nullptr );

	___INFOLOG( "passed" );
}

void EventQueueTest::testIndependentInstances() {
	___INFOLOG( "" );

	// EventQueue is instance-ownable: several may coexist, fully independent of
	// the process-current one and of each other. Constructing one no longer
	// hijacks the process-current pointer (ADR 0015).
	auto pA = std::make_unique<EventQueue>( pTestHydrogen() );
	auto pB = std::make_unique<EventQueue>( pTestHydrogen() );
	CPPUNIT_ASSERT( pA != pB );

	// An event pushed onto one queue is invisible to the other.
	pA->pushEvent( Event::Type::Metronome, 1 );
	CPPUNIT_ASSERT( pB->popEvent() == nullptr );

	auto pEvent = pA->popEvent();
	CPPUNIT_ASSERT( pEvent != nullptr );
	CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Metronome &&
					pEvent->getValue() == 1 );
	CPPUNIT_ASSERT( pA->popEvent() == nullptr );

	___INFOLOG( "passed" );
}

void EventQueueTest::testMidiNoteVectorThreadedAccess() {
	___INFOLOG( "" );
	auto pEventQueue = pTestEventQueue();

	// Clear any residue earlier tests may have left on the same queue.
	pEventQueue->drainMidiNoteActions();
	CPPUNIT_ASSERT( pEventQueue->getMidiNoteActions().empty() );

	// Semantics: FIFO drain and a non-consuming snapshot.
	EventQueue::AddMidiNoteVector noteAction;
	noteAction.id = static_cast<Instrument::Id>( 0 );
	noteAction.nPattern = 3;
	noteAction.nLength = -1;
	noteAction.fVelocity = 0.5;
	noteAction.fPan = 0.0;
	noteAction.key = static_cast<Note::Key>( 0 );
	noteAction.octave = static_cast<Note::Octave>( 0 );
	noteAction.bNoteOff = false;

	noteAction.nColumn = 23;
	pEventQueue->pushMidiNoteAction( noteAction );
	noteAction.nColumn = 42;
	pEventQueue->pushMidiNoteAction( noteAction );

	const auto snapshot = pEventQueue->getMidiNoteActions();
	CPPUNIT_ASSERT( snapshot.size() == 2 );
	CPPUNIT_ASSERT( snapshot[ 0 ].nColumn == 23 );
	CPPUNIT_ASSERT( snapshot[ 1 ].nColumn == 42 );
	// A snapshot must not consume the queued entries.
	CPPUNIT_ASSERT( pEventQueue->getMidiNoteActions().size() == 2 );

	const auto batch = pEventQueue->drainMidiNoteActions();
	CPPUNIT_ASSERT( batch.size() == 2 );
	CPPUNIT_ASSERT( batch[ 0 ].nColumn == 23 );
	CPPUNIT_ASSERT( batch[ 1 ].nColumn == 42 );
	// Draining consumed the entries — exactly once.
	CPPUNIT_ASSERT( pEventQueue->drainMidiNoteActions().empty() );
	CPPUNIT_ASSERT( pEventQueue->getMidiNoteActions().empty() );

	// Concurrency: a producer thread pushes while this thread drains —
	// the shape of the GUI timer racing the audio thread (standalone) or
	// the IPC reader thread (split). Every entry must survive exactly
	// once and in order.
	pthread_t thread;
	CPPUNIT_ASSERT( pthread_create( &thread, nullptr, pushMidiNoteThread,
									nullptr ) == 0 );

	int nSeen = 0;
	int nIdleSpins = 0;
	while ( nSeen < nMidiNotesToPush && nIdleSpins < 10000000 ) {
		const auto drained = pEventQueue->drainMidiNoteActions();
		if ( drained.empty() ) {
			++nIdleSpins;
			continue;
		}
		for ( const auto& drainedNote : drained ) {
			CPPUNIT_ASSERT( drainedNote.nColumn == nSeen );
			++nSeen;
		}
	}
	pthread_join( thread, nullptr );
	// Whatever the producer pushed after our last drain.
	for ( const auto& drainedNote : pEventQueue->drainMidiNoteActions() ) {
		CPPUNIT_ASSERT( drainedNote.nColumn == nSeen );
		++nSeen;
	}

	CPPUNIT_ASSERT( nSeen == nMidiNotesToPush );
	___INFOLOG( "passed" );
}
