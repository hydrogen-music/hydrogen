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

#ifndef EVENT_QUEUE_TEST_H
#define EVENT_QUEUE_TEST_H

#include <cppunit/extensions/HelperMacros.h>
#include <core/EventQueue.h>

class EventQueueTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( EventQueueTest );
	CPPUNIT_TEST( testPushPop );
	CPPUNIT_TEST( testOverflow );
	CPPUNIT_TEST( testThreadedAccess );
	CPPUNIT_TEST_SUITE_END();

public:
	static constexpr int nThreads = 16;
	static constexpr int nCountsPerThread = H2Core::EventQueue::nMaxEvents /
											nThreads;

	void setUp() override;
	void tearDown() override;
	
	void testPushPop();
	void testOverflow();
	void testThreadedAccess();
};

#endif
