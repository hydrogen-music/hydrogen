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

#include "Skin.h"

#include <QColor>

#include <vector>

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

void ColorTest::testQColorConversion()
{
	___INFOLOG( "" );

	// The GUI boundary converts in both directions; parity must hold
	// across the full component range.
	const std::vector<H2Core::Color> pins = {
		H2Core::Color( 0, 0, 0 ),
		H2Core::Color( 255, 255, 255 ),
		H2Core::Color( 1, 2, 3 ),
		H2Core::Color( 128, 64, 200 ),
		H2Core::Color( 255, 0, 128 ),
	};
	for ( const auto& core : pins ) {
		const QColor qt = Skin::toQColor( core );
		CPPUNIT_ASSERT( qt.isValid() );
		CPPUNIT_ASSERT( qt.red() == core.red() );
		CPPUNIT_ASSERT( qt.green() == core.green() );
		CPPUNIT_ASSERT( qt.blue() == core.blue() );
		// "#rrggbb" parity holds across the boundary.
		CPPUNIT_ASSERT( qt.name() == core.name() );
		CPPUNIT_ASSERT( Skin::fromQColor( qt ) == core );
	}

	// An invalid core color reads as black — the boundary hands the
	// GUI a valid black QColor, never an invalid one.
	const QColor invalidCore = Skin::toQColor( H2Core::Color() );
	CPPUNIT_ASSERT( invalidCore.isValid() );
	CPPUNIT_ASSERT( invalidCore == QColor( 0, 0, 0 ) );

	// Alpha does not survive the boundary: core colors have no alpha.
	CPPUNIT_ASSERT( Skin::fromQColor( QColor( 1, 2, 3, 128 ) )
					== H2Core::Color( 1, 2, 3 ) );

	// An invalid QColor reads as black too (its components read back
	// as 0), so the boundary yields a valid black core color.
	const H2Core::Color invalidQt = Skin::fromQColor( QColor() );
	CPPUNIT_ASSERT( invalidQt.isValid() );
	CPPUNIT_ASSERT( invalidQt == H2Core::Color( 0, 0, 0 ) );

	___INFOLOG( "passed" );
}
