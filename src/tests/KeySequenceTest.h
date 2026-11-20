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

#ifndef H2C_KEY_SEQUENCE_TEST_H
#define H2C_KEY_SEQUENCE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * Pins the portable-text format of H2Core::KeySequence: strict
 * parsing, canonical "Meta+Ctrl+Alt+Shift+<Key>" formatting, and
 * byte-identical round-trips for every sequence shipped in the test
 * configuration and for every literal Qt::Key of the Qt version being
 * built against. The QKeySequence conversion parity is load-bearing —
 * the GUI boundary (Skin.h) converts between both types.
 */
class KeySequenceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( KeySequenceTest );
	CPPUNIT_TEST( testValueSemantics );
	CPPUNIT_TEST( testPortableTextRoundTrip );
	CPPUNIT_TEST( testStrictParsing );
	CPPUNIT_TEST( testCorpusRoundTrip );
	CPPUNIT_TEST( testQKeySequenceConversion );
	CPPUNIT_TEST( testQnamespaceCompleteness );
	CPPUNIT_TEST_SUITE_END();

public:
	void testValueSemantics();
	void testPortableTextRoundTrip();
	void testStrictParsing();
	void testCorpusRoundTrip();
	void testQKeySequenceConversion();
	void testQnamespaceCompleteness();
};

#endif
