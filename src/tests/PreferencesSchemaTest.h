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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef PREFERENCES_SCHEMA_TEST_H
#define PREFERENCES_SCHEMA_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * Round-trip oracle for the Preferences schema table (ADR 0023 amendment):
 * every member tied by PreferencesSchema::tieAllMembers() must survive a
 * writeRows()/readRows() cycle, and elements no schema row covers must be
 * reported.
 */
class PreferencesSchemaTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PreferencesSchemaTest );
	CPPUNIT_TEST( testSchemaRoundTrip );
	CPPUNIT_TEST( testUnknownElementsReported );
	CPPUNIT_TEST( testLegacyElementsTolerated );
	CPPUNIT_TEST( testLegacyDefaultConfigClean );
	CPPUNIT_TEST( testOverrideLayerMembership );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() override;
	void tearDown() override;

	void testSchemaRoundTrip();
	void testUnknownElementsReported();
	void testLegacyElementsTolerated();
	void testLegacyDefaultConfigClean();
	void testOverrideLayerMembership();

private:
	unsigned m_nPreviousBitMask = 0;
};

#endif
