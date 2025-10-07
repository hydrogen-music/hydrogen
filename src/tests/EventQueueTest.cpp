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
	auto pQ = EventQueue::get_instance();
	for ( int i = 0; i < EventQueueTest::nCountsPerThread; i++) {
		pQ->push_event( EVENT_METRONOME, *pInt );
	}
	return nullptr;
}

void EventQueueTest::setUp() {
	auto pEventQueue = EventQueue::get_instance();
	pEventQueue->setSilent( false );

	// Clear queue of any events from previous tests.
	Event ev;
	do {
		ev = pEventQueue->pop_event();
	} while ( ev.type != EVENT_NONE );
}

void EventQueueTest::tearDown() {
	EventQueue::get_instance()->setSilent( true );
}
	
void EventQueueTest::testPushPop() {
	___INFOLOG( "" );
	auto pEventQueue = EventQueue::get_instance();
	Event ev;

	// Fill the event queue to the maximum permissible size, drain the queue and
	// then do it again.
	for ( int pass = 0; pass < 2; pass++) {
		for ( int i = 0; i < MAX_EVENTS; i++ ) {
			pEventQueue->push_event( EVENT_PROGRESS, i );
		}
		for ( int i = 0; i < MAX_EVENTS; i++ ) {
			ev = pEventQueue->pop_event();
			CPPUNIT_ASSERT( ev.type == EVENT_PROGRESS && ev.value == i );
		}

		// Queue should now be empty
		ev = pEventQueue->pop_event();
		CPPUNIT_ASSERT( ev.type == EVENT_NONE );
	}
	___INFOLOG( "passed" );
}

void EventQueueTest::testOverflow() {
	___INFOLOG( "" );
	auto pEventQueue = EventQueue::get_instance();
	Event ev;

	// Overfill queue
	for ( int i = 0; i < MAX_EVENTS + 100; i++) {
		pEventQueue->push_event( EVENT_PROGRESS, i );
	}
	// Check that the queue contains the most recent MAX_EVENTS events
	for ( int i = 0; i < MAX_EVENTS; i++) {
		ev = pEventQueue->pop_event();
		CPPUNIT_ASSERT( ev.type == EVENT_PROGRESS && ev.value == i + 100);
	}
	ev = pEventQueue->pop_event();
	CPPUNIT_ASSERT( ev.type == EVENT_NONE );
	___INFOLOG( "passed" );
}

void EventQueueTest::testThreadedAccess() {
	___INFOLOG( "" );
	pthread_t threads[ EventQueueTest::nThreads ];
	int counters[ EventQueueTest::nThreads ];
	int threadIds[ EventQueueTest::nThreads ];

	auto pEventQueue = EventQueue::get_instance();

	for ( int i = 0; i < EventQueueTest::nThreads; i++) {
		counters[ i ] = 0;
		threadIds[ i ] = i;
	}

	// Start writer threads
	for ( int i = 0; i < EventQueueTest::nThreads; i++ ) {
		int nRetVal = pthread_create(
			&threads[ i ], nullptr, pushThread, &threadIds[ i ] );
	}

	// Reader counts up the number of events from each thread
	for ( int nTotalEvents = 0;
		 nTotalEvents < EventQueueTest::nCountsPerThread *
						EventQueueTest::nThreads; ) {
		Event ev = pEventQueue->pop_event();
		if ( ev.type == EVENT_METRONOME ) {
			CPPUNIT_ASSERT( ev.value < EventQueueTest::nThreads &&
							ev.value >= 0 );
			counters[ ev.value ]++;
			CPPUNIT_ASSERT( ev.value <= EventQueueTest::nCountsPerThread );
			nTotalEvents++;
		} else {
			CPPUNIT_ASSERT( ev.type == EVENT_NONE );
		}
	}

	for ( int i = 0; i < EventQueueTest::nThreads; i++ ) {
		CPPUNIT_ASSERT( counters[i] == EventQueueTest::nCountsPerThread );
	}
	Event ev = pEventQueue->pop_event();
	CPPUNIT_ASSERT( ev.type == EVENT_NONE );
	___INFOLOG( "passed" );
}
