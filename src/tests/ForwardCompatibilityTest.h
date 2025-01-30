/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef FORWARD_COMPATIBILITY_TEST_H
#define FORWARD_COMPATIBILITY_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class ForwardCompatibilityTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(ForwardCompatibilityTest);
	CPPUNIT_TEST(testDrumkits);
	CPPUNIT_TEST(testPattern);
	CPPUNIT_TEST(testPlaylist);
	CPPUNIT_TEST(testSong);
	CPPUNIT_TEST_SUITE_END();

	public:
		// Removes all .bak backup files from the test data folder.
		void tearDown();
		void testDrumkits();
		void testPattern();
		void testPlaylist();
		void testSong();
	
};


#endif
