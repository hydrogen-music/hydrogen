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

#ifndef PLUGIN_PROCESS_TEST_H
#define PLUGIN_PROCESS_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class PluginProcessTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PluginProcessTest );
	CPPUNIT_TEST( testOddBlockSizes );
	CPPUNIT_TEST( testSilenceInSilenceOut );
	CPPUNIT_TEST( testBlockSizeChange );
	CPPUNIT_TEST( testSampleRateChange );
	CPPUNIT_TEST( testNoNaN );
	CPPUNIT_TEST( testDeterministicRender );
	CPPUNIT_TEST_SUITE_END();

public:
	void testOddBlockSizes();
	void testSilenceInSilenceOut();
	void testBlockSizeChange();
	void testSampleRateChange();
	void testNoNaN();
	void testDeterministicRender();
};

#endif
