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

#include <pthread.h>

using namespace H2Core;

static void *pushThread(void *p) {
	int *pInt = (int *)p;
	auto pEventQueue = EventQueue::get_instance();
	for ( int i = 0; i < EventQueueTest::nCountsPerThread; i++) {
		pEventQueue->pushEvent( Event::Type::Metronome, *pInt );
	}
	return nullptr;
}

void EventQueueTest::setUp() {
	auto pEventQueue = EventQueue::get_instance();
	pEventQueue->setSilent( false );

	auto pEvent = pEventQueue->popEvent();
	// Clear queue of any events from previous tests.
	do {
		pEvent = pEventQueue->popEvent();
	} while ( pEvent != nullptr );
}

void EventQueueTest::tearDown() {
	EventQueue::get_instance()->setSilent( true );
}
	
void EventQueueTest::testPushPop() {
	___INFOLOG( "" );
	auto pEventQueue = EventQueue::get_instance();
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
	auto pEventQueue = EventQueue::get_instance();
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
	auto pEventQueue = EventQueue::get_instance();

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
	auto pEventQueue = EventQueue::get_instance();
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
	auto pA = std::make_unique<EventQueue>();
	auto pB = std::make_unique<EventQueue>();
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

void EventQueueTest::testProcessCurrent() {
	___INFOLOG( "" );

	// Remember the harness's process-current EventQueue; the rest of the suite
	// relies on it, so it must be restored before returning.
	EventQueue* pPrevious = EventQueue::get_instance();

	auto pA = std::make_unique<EventQueue>();
	auto pB = std::make_unique<EventQueue>();

	// get_instance() returns whichever instance was registered as
	// process-current — the transitional shim unconverted call sites use.
	EventQueue::setInstance( pA.get() );
	CPPUNIT_ASSERT( EventQueue::get_instance() == pA.get() );

	EventQueue::setInstance( pB.get() );
	CPPUNIT_ASSERT( EventQueue::get_instance() == pB.get() );

	// Registering does not own: the previous instance is untouched and still
	// usable once restored.
	EventQueue::setInstance( pPrevious );
	CPPUNIT_ASSERT( EventQueue::get_instance() == pPrevious );

	___INFOLOG( "passed" );
}
