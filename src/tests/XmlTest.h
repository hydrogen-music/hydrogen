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

#include <QString>

#include <cppunit/extensions/HelperMacros.h>

namespace H2Core {
	class Drumkit;
}

class XmlTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(XmlTest);
	CPPUNIT_TEST(testDrumkitFormatIntegrity);
	CPPUNIT_TEST(testDrumkit);
	CPPUNIT_TEST(testDrumkitLegacy);
	CPPUNIT_TEST(testDrumkit_invalidADSRValues);
	CPPUNIT_TEST(testDrumkitUpgrade);
	CPPUNIT_TEST(testDrumkitInstrumentTypeUniqueness);
	CPPUNIT_TEST(testShippedDrumkits);
	CPPUNIT_TEST(testDrumkitMapFormatIntegrity);
	CPPUNIT_TEST(testDrumkitMap);
	CPPUNIT_TEST(testShippedDrumkitMaps);
	CPPUNIT_TEST(testPatternFormatIntegrity);
	CPPUNIT_TEST(testPattern);
	CPPUNIT_TEST(testPatternLegacy);
	CPPUNIT_TEST(testPatternInstrumentTypes);
	CPPUNIT_TEST(testPlaylistFormatIntegrity);
	CPPUNIT_TEST(testPlaylist);
	CPPUNIT_TEST(checkTestPatterns);
	CPPUNIT_TEST(testSongFormatIntegrity);
	CPPUNIT_TEST(testSong);
	CPPUNIT_TEST(testSongLegacy);
	CPPUNIT_TEST(testPreferencesFormatIntegrity);
	CPPUNIT_TEST(testShippedPreferences);
	CPPUNIT_TEST(testShippedThemes);
	CPPUNIT_TEST(testSamplePathsWritten);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp();
		// Removes all .bak backup files from the test data folder.
		void tearDown();
		/** Checks whether the format of `drumkit.xml` files did change. */
		void testDrumkitFormatIntegrity();
		void testDrumkit();
		void testDrumkitLegacy();
		void testDrumkit_invalidADSRValues();
		void testDrumkitUpgrade();
		void testDrumkitInstrumentTypeUniqueness();
		// Check whether the drumkits provided alongside this repo can
		// be validated against the drumkit XSD.
		void testShippedDrumkits();

		/** Checks whether the format of `.h2map` files did change. */
		void testDrumkitMapFormatIntegrity();
		void testDrumkitMap();
		// Check whether the drumkit maps provided alongside this repo can be
		// validated against their XSD.
		void testShippedDrumkitMaps();

		/** Checks whether the format of `.h2pattern` files did change. */
		void testPatternFormatIntegrity();
		void testPattern();
		void testPatternLegacy();
		void testPatternInstrumentTypes();
		// Check whether the pattern used in the unit test is valid
		// with respect to the shipped XSD file.
		void checkTestPatterns();

		/** Checks whether the format of `.h2playlist` files did change. */
		void testPlaylistFormatIntegrity();
		void testPlaylist();

		/** Checks whether the format of `.h2song` files did change. */
		void testSongFormatIntegrity();
		void testSong();
		// In the beginning of the 1.X.X series we had a lot of changes
		// regarding how instruments are stored in a song and how the associated
		// samples are looked up. Unfortunately, shortcomings of the individual
		// approaches manifested only one at a time.
		//
		// This test loads song of various versions and checks whether all
		// samples could be loaded.
		void testSongLegacy();

		/** Checks whether the format of our preferences file `hydrogen.conf`
		 * did change. */
		void testPreferencesFormatIntegrity();
		// Check whether the shipped default/fallback config file is up-to-date.
		void testShippedPreferences();
		// Check whether the shipped default/fallback .h2theme files are
		// up-to-date.
		void testShippedThemes();

		/** When saving a drumkit all sample <filename> elements should contain
		 * only the basename + file extention. In songs this holds too for all
		 * samples which are part of a user or system drumkit. Samples manually
		 * loaded by the user, on the other hand, must hold an absolute file
		 * path. */
		void testSamplePathsWritten();

	private:
		static bool checkSampleData( std::shared_ptr<H2Core::Drumkit> pKit,
									 bool bLoaded );
		QString m_sPrefPre;
		QString m_sHydrogenPre;
};


#endif
