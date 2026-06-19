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

/** Specifies and pins the Object-level instance identity (ADR 0028 / T4.4a). */
class ObjectUuidTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( ObjectUuidTest );
	CPPUNIT_TEST( testUniqueness );
	CPPUNIT_TEST( testFreshOnCopy );
	CPPUNIT_TEST( testStableUnderAssignment );
	CPPUNIT_TEST( testValueSemantics );
	CPPUNIT_TEST( testCrossProcessEpoch );
	CPPUNIT_TEST( testInstrumentIdentityVsId );
	CPPUNIT_TEST( testLockFreeMinting );
	CPPUNIT_TEST_SUITE_END();

   public:
	void testUniqueness();
	void testFreshOnCopy();
	void testStableUnderAssignment();
	void testValueSemantics();
	void testCrossProcessEpoch();
	void testInstrumentIdentityVsId();
	void testLockFreeMinting();
};
