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

#ifndef NOTE_TEST_H
#define NOTE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class NoteTest : public CppUnit::TestCase {
		CPPUNIT_TEST_SUITE( NoteTest );
		CPPUNIT_TEST( testComparison );
		CPPUNIT_TEST( testMappingLegacyDrumkit );
		CPPUNIT_TEST( testMappingValidDrumkits );
		CPPUNIT_TEST( testMidiDefaultOffset );
		CPPUNIT_TEST( testPitchConversions );
		CPPUNIT_TEST( testProbability );
		CPPUNIT_TEST( testSerializeProbability );
		CPPUNIT_TEST( testStrongTypedPitch );
		CPPUNIT_TEST( testVirtualKeyboard );
		CPPUNIT_TEST_SUITE_END();

	public:
		void testComparison();
		/** Notes will be mapped back and forth between a valid/new drumkit and
		 * one created prior to version 2.0. */
		void testMappingLegacyDrumkit();
		/** Notes will be mapped back and forth between two valid drumkits. */
		void testMappingValidDrumkits();
		void testMidiDefaultOffset();
		void testPitchConversions();
		void testProbability();
		void testSerializeProbability();
        void testStrongTypedPitch();
		/** Check whether notes entered via the virtual keyboard can be handled
		 * properly */
		void testVirtualKeyboard();
};

#endif
