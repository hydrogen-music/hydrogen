/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <core/Basics/Song.h>

class CoreActionControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( CoreActionControllerTest );
	CPPUNIT_TEST( testCountIn );
	CPPUNIT_TEST( testSessionManagement );
	CPPUNIT_TEST( testSetPatternSize );
	CPPUNIT_TEST( testEditNoteProperty );
	CPPUNIT_TEST( testAddOrRemoveNote );
	CPPUNIT_TEST( testOverwriteNotes );
	CPPUNIT_TEST( testSetPanLaw );
	CPPUNIT_TEST( testPlaybackTrack );
	CPPUNIT_TEST_SUITE_END();

public:

		void testCountIn();
		/** Bucket-B write-surface entry points (ADR 0027 / T4.5). */
		void testSetPatternSize();
		void testEditNoteProperty();
		void testAddOrRemoveNote();
		void testOverwriteNotes();
		void testSetPanLaw();
		void testPlaybackTrack();

	// Tests the H2Core::Hydrogen::get_instance()->getCoreActionController()->loadSong(),
	// H2Core::Hydrogen::get_instance()->getCoreActionController()->setSong(),
	// H2Core::Hydrogen::get_instance()->getCoreActionController()->saveSong()
	// H2Core::Hydrogen::get_instance()->getCoreActionController()->saveSongAs() methods.
	void testSessionManagement();
};
