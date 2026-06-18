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

#ifndef PREFERENCES_PERSIST_TEST_H
#define PREFERENCES_PERSIST_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <QtCore/QString>

/**
 * T5.6 wiring (ADR 0023): Preferences retains a load baseline and routes
 * shared-config saves through the concurrency-safe PluginConfig::persist merge.
 */
class PreferencesPersistTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PreferencesPersistTest );
	CPPUNIT_TEST( testBaselineRetainedOnLoad );
	CPPUNIT_TEST( testConcurrentBaseEditSurvives );
	CPPUNIT_TEST( testOverrideFieldNotWritten );
	CPPUNIT_TEST_SUITE_END();

public:
	void tearDown() override;

	void testBaselineRetainedOnLoad();
	void testConcurrentBaseEditSurvives();
	void testOverrideFieldNotWritten();
};

#endif
