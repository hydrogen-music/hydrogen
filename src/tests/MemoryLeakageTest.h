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

#ifndef MEMORY_LEAKAGE_TEST_H
#define MEMORY_LEAKAGE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class MemoryLeakageTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(MemoryLeakageTest);
	CPPUNIT_TEST(testConstructors);
	CPPUNIT_TEST(testLoading);
	CPPUNIT_TEST_SUITE_END();

public:
	void tearDown();
	/** Creates and destroys all basic classes using their
	 * destructors and checks whether there are some objects alive
	 * afterwards.
	 */
	void testConstructors();
	
	/** Creates and destroys all basic classes using their various
	 * loading routines and checks whether there are some objects
	 * alive afterwards.
	 */
	void testLoading();

};


#endif
