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

#ifndef H2C_SHORTCUTS_TEST_H
#define H2C_SHORTCUTS_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * Pins the headless contract of Shortcuts: default creation is driven
 * by injected Undo/Redo sequences (the GUI resolves them via
 * QKeySequence::keyBindings(), which requires a QGuiApplication),
 * malformed rows are skipped without suppressing the remaining ones,
 * and a configuration without usable shortcuts defers to the
 * defaults instead of silently leaving the user without any.
 */
class ShortcutsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( ShortcutsTest );
	CPPUNIT_TEST( testCreateDefaultShortcutsUsesInjectedSequences );
	CPPUNIT_TEST( testLoadFromSkipsMalformedRows );
	CPPUNIT_TEST( testLoadFromGarbageOnlyFallsBackToDefaults );
	CPPUNIT_TEST( testLoadFromDefersDefaultsWhenNodeMissing );
	CPPUNIT_TEST( testSaveToLoadFromRoundTrip );
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateDefaultShortcutsUsesInjectedSequences();
	void testLoadFromSkipsMalformedRows();
	void testLoadFromGarbageOnlyFallsBackToDefaults();
	void testLoadFromDefersDefaultsWhenNodeMissing();
	void testSaveToLoadFromRoundTrip();
};

#endif
