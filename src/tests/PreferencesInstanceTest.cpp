/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "PreferencesInstanceTest.h"

using namespace H2Core;

void PreferencesInstanceTest::testIndependentInstances() {
	___INFOLOG( "" );

	// Preferences is instance-ownable: several may coexist, fully independent
	// of each other (ADR 0015).
	auto pA = std::make_shared<Preferences>();
	auto pB = std::make_shared<Preferences>();
	CPPUNIT_ASSERT( pA != pB );

	pA->m_nBufferSize = 256;
	pB->m_nBufferSize = 2048;

	// Mutating one instance never affects the other.
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pA->m_nBufferSize );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )2048, pB->m_nBufferSize );

	// A copy is an independent object, not an alias.
	auto pCopy = std::make_shared<Preferences>( pA );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pCopy->m_nBufferSize );
	pCopy->m_nBufferSize = 512;
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pA->m_nBufferSize );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )512, pCopy->m_nBufferSize );

	___INFOLOG( "passed" );
}
