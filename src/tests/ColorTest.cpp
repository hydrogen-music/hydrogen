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

#include "ColorTest.h"

#include <core/Preferences/Color.h>
#include <core/Object.h>

void ColorTest::testColor()
{
	___INFOLOG( "" );

	// Default construction matches QColor(): invalid, reads as black.
	const H2Core::Color invalid;
	CPPUNIT_ASSERT( ! invalid.isValid() );
	CPPUNIT_ASSERT( invalid.red() == 0 );
	CPPUNIT_ASSERT( invalid.green() == 0 );
	CPPUNIT_ASSERT( invalid.blue() == 0 );
	CPPUNIT_ASSERT( invalid.name() == "#000000" );

	// Out-of-range components invalidate instead of clamping — the
	// XML reader relies on this to reject malformed "r,g,b" values.
	CPPUNIT_ASSERT( ! H2Core::Color( 256, 0, 0 ).isValid() );
	CPPUNIT_ASSERT( ! H2Core::Color( -1, 0, 0 ).isValid() );
	CPPUNIT_ASSERT( ! H2Core::Color( 0, 999, 0 ).isValid() );
	// An invalidated color reads back as black (QColor parity).
	CPPUNIT_ASSERT( H2Core::Color( 256, 0, 0 ).red() == 0 );

	const H2Core::Color color( 1, 2, 3 );
	CPPUNIT_ASSERT( color.isValid() );
	CPPUNIT_ASSERT( color.red() == 1 );
	CPPUNIT_ASSERT( color.green() == 2 );
	CPPUNIT_ASSERT( color.blue() == 3 );

	// QColor::name() parity: lowercase, zero-padded "#rrggbb".
	CPPUNIT_ASSERT( color.name() == "#010203" );
	CPPUNIT_ASSERT( H2Core::Color( 255, 255, 255 ).name() == "#ffffff" );

	// Equality: invalid colors compare equal (both read as black), but
	// never equal a valid black.
	CPPUNIT_ASSERT( invalid == H2Core::Color() );
	CPPUNIT_ASSERT( invalid != H2Core::Color( 0, 0, 0 ) );
	CPPUNIT_ASSERT( color == H2Core::Color( 1, 2, 3 ) );
	CPPUNIT_ASSERT( color != H2Core::Color( 3, 2, 1 ) );

	___INFOLOG( "passed" );
}
