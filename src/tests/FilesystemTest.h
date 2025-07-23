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

#include <core/Helpers/Filesystem.h>

class FilesystemTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( FilesystemTest );
	CPPUNIT_TEST( testPermissions );
	CPPUNIT_TEST( testUniquePrefix );
	CPPUNIT_TEST( testFilePathValidation );
	CPPUNIT_TEST( testSamplePathHandling );
	CPPUNIT_TEST_SUITE_END();
	
	
public:
	void setUp();
	void tearDown();
	
	void testPermissions();
		void testUniquePrefix();
	void testFilePathValidation();
		void testSamplePathHandling();

private:
	QString m_sNotExistingPath;
	QString m_sNoAccessPath;
	QString m_sReadOnlyPath;
	QString m_sFullAccessPath;
	// To ensure Qt does handle the temporary files consistently.
	QString m_sTmpPath;
	QString m_sDemoSongSysPath;
};
