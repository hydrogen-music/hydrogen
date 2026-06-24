/*
 * Hydrogen
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

#ifndef H2PROJECT_TEST_H
#define H2PROJECT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class H2ProjectTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( H2ProjectTest );
	CPPUNIT_TEST( testBufferRoundTrip );
	CPPUNIT_TEST( testBasicsBufferRoundTrip );
	CPPUNIT_TEST( testFileRoundTrip );
	CPPUNIT_TEST( testContainerDetection );
	CPPUNIT_TEST( testUnifiedOpen );
	CPPUNIT_TEST_SUITE_END();

public:
	void testBufferRoundTrip();
	void testBasicsBufferRoundTrip();
	void testFileRoundTrip();
	void testContainerDetection();
	void testUnifiedOpen();
};

#endif
