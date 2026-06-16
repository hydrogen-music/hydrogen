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

#ifndef OUTPUT_BUS_TEST_H
#define OUTPUT_BUS_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class OutputBusTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( OutputBusTest );
	CPPUNIT_TEST( testActiveBusCount );
	CPPUNIT_TEST( testInstrumentRoutedToOwnBus );
	CPPUNIT_TEST( testSecondInstrumentToSecondBus );
	CPPUNIT_TEST( testSurplusInstrumentMasterOnly );
	CPPUNIT_TEST_SUITE_END();

public:
	void testActiveBusCount();
	void testInstrumentRoutedToOwnBus();
	void testSecondInstrumentToSecondBus();
	void testSurplusInstrumentMasterOnly();
};

#endif
