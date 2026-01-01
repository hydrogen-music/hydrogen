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

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

void PatternTest::testCustomLegacyImport()
{
	___INFOLOG( "" );
	auto pDB = Hydrogen::get_instance()->getSoundLibraryDatabase();

	// Try to import the legcay pattern without loading the kit into the DB
	// first.
	auto pPattern = Pattern::load(
		H2TEST_FILE( "pattern/legacyImport.h2pattern" ) );
	CPPUNIT_ASSERT( pPattern != nullptr );
	CPPUNIT_ASSERT( pPattern->getAllTypes().size() == 0 );

	// Now we load our custom kit into the db and try again
	auto pSampleKit = pDB->getDrumkit(
		H2TEST_FILE( "drumkits/sampleKit" ), false );
	CPPUNIT_ASSERT( pSampleKit != nullptr );
	CPPUNIT_ASSERT( pSampleKit->toDrumkitMap()->getAllTypes().size() > 0 );

	auto pPatternReload = Pattern::load(
		H2TEST_FILE( "pattern/legacyImport.h2pattern" ) );
	CPPUNIT_ASSERT( pPatternReload != nullptr );
	CPPUNIT_ASSERT( pPatternReload->getAllTypes().size() > 0 );

	___INFOLOG( "passed" );
}

void PatternTest::testPurgeInstrument()
{
	___INFOLOG( "" );
	auto pInstrument = std::make_shared<Instrument>();
	auto pNote = std::make_shared<Note>( pInstrument, 1, 1.0, 0.f, 1, 1.0 );

	auto pPattern = std::make_shared<Pattern>();
	pPattern->insertNote( pNote );
	CPPUNIT_ASSERT( pPattern->findNote(
						1, pInstrument->getId(),
						pInstrument->getType(),
						Note::KeyMin,
						static_cast<Note::Octave>(OCTAVE_DEFAULT) ) != nullptr );
	auto notes = pPattern->findNotes( 1, pInstrument->getId(),
									  pInstrument->getType() );
	CPPUNIT_ASSERT( notes.size() == 1 );
	notes.clear();

	pPattern->purgeInstrument( pInstrument );
	CPPUNIT_ASSERT( pPattern->findNote(
						1, pInstrument->getId(),
						pInstrument->getType(),
						Note::KeyMin,
						static_cast<Note::Octave>(OCTAVE_DEFAULT) ) == nullptr );
	notes = pPattern->findNotes( 1, pInstrument->getId(),
								 pInstrument->getType() );
	CPPUNIT_ASSERT( notes.size() == 0 );

	___INFOLOG( "passed" );
}
