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

#ifndef MIDI_EXPORT_TEST_H
#define MIDI_EXPORT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class MidiExportTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( MidiExportTest );
	CPPUNIT_TEST( testExportMIDISMF0 );
	CPPUNIT_TEST( testExportMIDISMF1Single );
	CPPUNIT_TEST( testExportMIDISMF1Multi );
	CPPUNIT_TEST( testExportVelocityAutomationMIDISMF0 );
	CPPUNIT_TEST( testExportVelocityAutomationMIDISMF1 );
	CPPUNIT_TEST( testTimeSignatureCalculation );
	CPPUNIT_TEST_SUITE_END();
	
	public:
		void testExportMIDISMF1Single();
		void testExportMIDISMF1Multi();
		void testExportMIDISMF0();
		void testExportVelocityAutomationMIDISMF1();
		void testExportVelocityAutomationMIDISMF0();
		void testTimeSignatureCalculation();
};

#endif
