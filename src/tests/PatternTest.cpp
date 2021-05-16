/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "PatternTest.h"

#include <core/AudioEngine.h>
#include <core/Basics/Pattern.h>

CPPUNIT_TEST_SUITE_REGISTRATION( PatternTest );

using namespace H2Core;

void PatternTest::setUp()
{
	AudioEngine::create_instance();
}


void PatternTest::testPurgeInstrument()
{
	Instrument *pInstrument = new Instrument();
	Note *pNote = new Note( pInstrument, 1, 1.0, 1.0, 1.0, 1, 1.0 );

	Pattern *pPattern = new Pattern();
	pPattern->insert_note( pNote );
	CPPUNIT_ASSERT( pPattern->find_note( 1, -1, pInstrument) != nullptr );

	pPattern->purge_instrument( pInstrument );
	CPPUNIT_ASSERT( pPattern->find_note( 1, -1, pInstrument) == nullptr );

	delete pPattern;
}
