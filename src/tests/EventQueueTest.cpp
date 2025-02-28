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

#include <cppunit/extensions/HelperMacros.h>
#include <core/EventQueue.h>
#include <pthread.h>

using namespace H2Core;

const int nThreads = 16;
const int nCountsPerThread = EventQueue::nMaxEvents / nThreads;

static void *pushThread(void *p) {
	int *pInt = (int *)p;
	EventQueue *pQ = EventQueue::get_instance();
	for ( int i = 0; i < nCountsPerThread; i++) {
		pQ->pushEvent( Event::Type::Metronome, *pInt );
	}
	return nullptr;
}

class EventQueueTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( EventQueueTest );
	CPPUNIT_TEST( testPushPop );
	CPPUNIT_TEST( testOverflow );
	CPPUNIT_TEST( testThreadedAccess );
	CPPUNIT_TEST_SUITE_END();

	EventQueue *m_pQ;

public:

	void setUp() override {
		EventQueue::create_instance();
		m_pQ = EventQueue::get_instance();
		m_pQ->setSilent( false );

		auto pEvent = m_pQ->popEvent();
		// Clear queue of any events from previous tests.
		do {
			pEvent = m_pQ->popEvent();
		} while ( pEvent != nullptr );
	}

	void tearDown() override {
		EventQueue::get_instance()->setSilent( true );
	}
	
	void testPushPop() {
	___INFOLOG( "" );
		std::unique_ptr<Event> pEvent;

		// Fill the event queue to the maximum permissible size, drain the queue
		// and then do it again.
		for ( int pass = 0; pass < 2; pass++) {
			for ( int i = 0; i < EventQueue::nMaxEvents; i++ ) {
				m_pQ->pushEvent( Event::Type::Progress, i );
			}
			for ( int i = 0; i < EventQueue::nMaxEvents; i++ ) {
				pEvent = m_pQ->popEvent();
				CPPUNIT_ASSERT( pEvent != nullptr );
				CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Progress &&
								pEvent->getValue() == i );
			}

			// Queue should now be empty
			pEvent = m_pQ->popEvent();
			CPPUNIT_ASSERT( pEvent == nullptr );
		}
	___INFOLOG( "passed" );
	}

	void testOverflow() {
	___INFOLOG( "" );
		std::unique_ptr<Event> pEvent;

		// Overfill queue
		for ( int i = 0; i < EventQueue::nMaxEvents + 100; i++) {
			m_pQ->pushEvent( Event::Type::Progress, i );
		}
		// Check that the queue contains the most recent EventQueue::nMaxEvents
		// events
		for ( int i = 0; i < EventQueue::nMaxEvents; i++) {
			pEvent = m_pQ->popEvent();
			CPPUNIT_ASSERT( pEvent != nullptr );
			CPPUNIT_ASSERT( pEvent->getType() == Event::Type::Progress &&
							pEvent->getValue() == i + 100);
		}
		pEvent = m_pQ->popEvent();
		CPPUNIT_ASSERT( pEvent == nullptr );
	___INFOLOG( "passed" );
	}

	void testThreadedAccess() {
	___INFOLOG( "" );
		pthread_t threads[ nThreads ];
		int counters[ nThreads ];
		int threadIds[ nThreads ];

		for ( int i = 0; i < nThreads; i++) {
			counters[ i ] = 0;
			threadIds[ i ] = i;
		}

		// Start writer threads
		for ( int i = 0; i < nThreads; i++ ) {
			int nRetVal = pthread_create(
				&threads[ i ], nullptr, pushThread, &threadIds[ i ]);
		}

		// Reader counts up the number of events from each thread
		for ( int nTotalEvents = 0; nTotalEvents < nCountsPerThread * nThreads; ) {
			auto pEvent = m_pQ->popEvent();
			if ( pEvent != nullptr &&
				 pEvent->getType() == Event::Type::Metronome ) {
				CPPUNIT_ASSERT( pEvent->getValue() < nThreads &&
								pEvent->getValue() >= 0 );
				counters[ pEvent->getValue() ]++;
				CPPUNIT_ASSERT( pEvent->getValue() <= nCountsPerThread );
				nTotalEvents++;
			}
			else {
				CPPUNIT_ASSERT( pEvent == nullptr );
			}
		}

		for ( int i = 0; i < nThreads; i++ ) {
			CPPUNIT_ASSERT( counters[i] == nCountsPerThread );
		}
		auto pEvent = m_pQ->popEvent();
		CPPUNIT_ASSERT( pEvent == nullptr );
	___INFOLOG( "passed" );
	}

};

