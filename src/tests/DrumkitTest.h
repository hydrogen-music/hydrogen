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

#ifndef H2TEST_INSTRUMENT_LIST_TEST_H
#define H2TEST_INSTRUMENT_LIST_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class DrumkitTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( DrumkitTest );
	CPPUNIT_TEST( testDefaultMidiOutNotes );
	CPPUNIT_TEST( testHasAllMidiNotesSame );
	CPPUNIT_TEST( testIsValidIndex );
	CPPUNIT_TEST( testLayerHandling );
	CPPUNIT_TEST( testInstrumentMove );
	CPPUNIT_TEST_SUITE_END();

   public:
	void testDefaultMidiOutNotes();
	void testHasAllMidiNotesSame();
	void testIsValidIndex();
	void testLayerHandling();
	void testInstrumentMove();
};

#endif
