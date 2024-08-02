/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
	CPPUNIT_TEST(testDrumkit_UpgradeInvalidADSRValues);
	CPPUNIT_TEST(testDrumkitUpgrade);
	CPPUNIT_TEST(testDrumkitInstrumentTypeUniqueness);
	CPPUNIT_TEST(testDrumkitMap);
	CPPUNIT_TEST(testPattern);
	CPPUNIT_TEST(testPatternLegacy);
	CPPUNIT_TEST(testPatternInstrumentTypes);
	CPPUNIT_TEST(testPlaylist);
	CPPUNIT_TEST(testShippedDrumkits);
	CPPUNIT_TEST(checkTestPatterns);
	CPPUNIT_TEST(testSong);
	CPPUNIT_TEST(testSongLegacy);
	CPPUNIT_TEST_SUITE_END();

	public:
		// Removes all .bak backup files from the test data folder.
		void tearDown();
		void testDrumkit();
		void testDrumkit_UpgradeInvalidADSRValues();
		void testDrumkitUpgrade();
		void testDrumkitInstrumentTypeUniqueness();
		void testDrumkitMap();
		void testPattern();
		void testPatternLegacy();
		void testPatternInstrumentTypes();
		void testPlaylist();
		// Check whether the drumkits provided alongside this repo can
		// be validated against the drumkit XSD.
		void testShippedDrumkits();
		// Check whether the pattern used in the unit test is valid
		// with respect to the shipped XSD file.
		void checkTestPatterns();

		void testSong();
		// In the beginning of the 1.X.X series we had a lot of changes
		// regarding how instruments are stored in a song and how the associated
		// samples are looked up. Unfortunately, shortcomings of the individual
		// approaches manifested only one at a time.
		//
		// This test loads song of various versions and checks whether all
		// samples could be loaded.
		void testSongLegacy();
};


#endif
