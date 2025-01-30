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

#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <core/Basics/Song.h>

class CoreActionControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( CoreActionControllerTest );
	CPPUNIT_TEST( testSessionManagement );
	CPPUNIT_TEST( testIsPathValid );
	CPPUNIT_TEST_SUITE_END();
	
private:
	H2Core::Hydrogen* m_pHydrogen;

	QString m_sFileNameImproper;
	QString m_sFileName;
	QString m_sFileName2;
	
public:
	// Initializes the private member variables and loads an empty
	// song into Hydrogen.
	void setUp();
	// Loads an empty song into Hydrogen and deletes all temporary
	// files.
	void tearDown();
	
	// Tests the CoreActionController::loadSong(),
	// CoreActionController::setSong(),
	// CoreActionController::saveSong()
	// CoreActionController::saveSongAs() methods.
	void testSessionManagement();
	
	// Tests Filesystem::isPathValid()
	void testIsPathValid();
};
