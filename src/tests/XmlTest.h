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

#ifndef XML_TEST_H
#define XML_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class XmlTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(XmlTest);
	CPPUNIT_TEST(testDrumkit);
	CPPUNIT_TEST(testDrumkitLegacy);
	CPPUNIT_TEST(testDrumkit_invalidADSRValues);
	CPPUNIT_TEST(testDrumkitUpgrade);
	CPPUNIT_TEST(testPattern);
	CPPUNIT_TEST(testPlaylist);
	CPPUNIT_TEST(testShippedDrumkits);
	CPPUNIT_TEST(checkTestPatterns);
	CPPUNIT_TEST(testSamplePathPortability);
	CPPUNIT_TEST(testSamplePathsWritten);
	CPPUNIT_TEST(testSongLegacy);
	CPPUNIT_TEST_SUITE_END();

	public:
		// Removes all .bak backup files from the test data folder.
		void tearDown();
		void testDrumkit();
		void testDrumkitLegacy();
		void testDrumkit_invalidADSRValues();
		void testDrumkitUpgrade();
		void testPattern();
		void testPlaylist();
		// Check whether the drumkits provided alongside this repo can
		// be validated against the drumkit XSD.
		void testShippedDrumkits();
		// Check whether the pattern used in the unit test is valid
		// with respect to the shipped XSD file.
		void checkTestPatterns();
		/** In case an absolute path was provided for a sample - e.g. one is
		 * loaded manually using the instrument editor into the song - and the
		 * path is not valid anymore - e.g. the previous session kit was moved
		 * into the user's drumkit folder or the song is loaded on a device with
		 * a different user account /home directoy - Hydrogen should try to load
		 * the <drumkit_name>/<sample_file> combination from its drumkit
		 * folders */
		void testSamplePathPortability();
		/** When saving a drumkit all sample <filename> elements should contain
		 * only the basename + file extention. In songs this holds too for all
		 * samples which are part of a user or system drumkit. Samples manually
		 * loaded by the user, on the other hand, must hold an absolute file
		 * path. */
		void testSamplePathsWritten();
		/** In the beginning of the 1.X.X series we had a lot of changes
		 * regarding how instruments are stored in a song and how the associated
		 * samples are looked up. Unfortunately, shortcomings of the individual
		 * approaches manifested only one at a time.
		 *
		 * This test loads song of various versions and checks whether all
		 * samples could be loaded. */
		void testSongLegacy();

};


#endif
