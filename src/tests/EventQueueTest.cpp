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
			pEventQueue->pushEvent( Event::Type::Progress, i );
		}
		for ( int i = 0; i < EventQueue::nMaxEvents; i++ ) {
			pEvent = pEventQueue->popEvent();
			CPPUNIT_ASSERT( pEvent != nullptr );
			CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Progress &&
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
		pEventQueue->pushEvent( Event::Type::Progress, i );
	}
	// Check that the queue contains the most recent EventQueue::nMaxEvents
	// events
	for ( int i = 0; i < EventQueue::nMaxEvents; i++) {
		pEvent = pEventQueue->popEvent();
		CPPUNIT_ASSERT( pEvent != nullptr );
		CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Progress &&
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
