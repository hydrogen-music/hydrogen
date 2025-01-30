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

#ifndef CLI_TEST_H
#define CLI_TEST_H

#include <QString>

#include <cppunit/extensions/HelperMacros.h>

class CliTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(CliTest);
	CPPUNIT_TEST(testKitToDrumkitMap);
	CPPUNIT_TEST_SUITE_END();

	public:
		/** Note that since h2cli is a runtime dependency, we don't have to add
		 * it to CMakeLists.txt of the test folder but just check its present
		 * when running the unit tests.*/
		void setUp();
		void testKitToDrumkitMap();

	private:
		QString m_sCliPath;
};


#endif
