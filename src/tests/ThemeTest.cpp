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

#include "ThemeTest.h"

#include <QStringList>

#include <core/Preferences/Theme.h>

void ThemeTest::testSanitizeFamilies()
{
	___INFOLOG( "" );

	auto pFont = std::make_shared<H2Core::FontTheme>();
	pFont->m_sApplicationFontFamily = "Kept";
	pFont->m_sLevel2FontFamily = "Missing";
	pFont->m_sLevel3FontFamily = "";

	// Qt resolves font families case-insensitively ("kept" is installed);
	// the check must not fall back on a mere case mismatch.
	pFont->sanitizeFamilies( QStringList{ "kept", "Other" }, "Fallback" );

	CPPUNIT_ASSERT( pFont->m_sApplicationFontFamily == "Kept" );
	CPPUNIT_ASSERT( pFont->m_sLevel2FontFamily == "Fallback" );
	CPPUNIT_ASSERT( pFont->m_sLevel3FontFamily == "Fallback" );

	// Sanitization is stable: a second pass over the already sanitized
	// values changes nothing.
	pFont->sanitizeFamilies( QStringList{ "kept", "Fallback" }, "Other" );
	CPPUNIT_ASSERT( pFont->m_sApplicationFontFamily == "Kept" );
	CPPUNIT_ASSERT( pFont->m_sLevel2FontFamily == "Fallback" );
	CPPUNIT_ASSERT( pFont->m_sLevel3FontFamily == "Fallback" );

	___INFOLOG( "passed" );
}
