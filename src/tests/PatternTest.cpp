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

#include "PatternTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Pattern.h>

using namespace H2Core;

void PatternTest::testPurgeInstrument()
{
	___INFOLOG( "" );
	auto pInstrument = std::make_shared<Instrument>();
	Note *pNote = new Note( pInstrument, 1, 1.0, 0.f, 1, 1.0 );

	Pattern *pPattern = new Pattern();
	pPattern->insert_note( pNote );
	CPPUNIT_ASSERT( pPattern->find_note( 1, -1, pInstrument) != nullptr );

	pPattern->purge_instrument( pInstrument );
	CPPUNIT_ASSERT( pPattern->find_note( 1, -1, pInstrument) == nullptr );

	delete pPattern;
	___INFOLOG( "passed" );
}
