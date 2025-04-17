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

#include "SoundLibraryTest.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

void SoundLibraryTest::testContextValidity() {
	___INFOLOG( "" );

	auto pDB = H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase();
	for ( const auto& [ _, ppDrumkit ]: pDB->getDrumkitDatabase() ) {
		CPPUNIT_ASSERT( ppDrumkit != nullptr );
		CPPUNIT_ASSERT( ppDrumkit->getContext() != H2Core::Drumkit::Context::Song );
	}

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testKitRetrievalCopy() {
	___INFOLOG( "" );

	auto pDB = H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase();

	const float fNewGainValue = 1.23456;

	auto pKit = pDB->getDrumkit( "GMRockKit" );
	CPPUNIT_ASSERT( pKit != nullptr );

	auto pKitCopy = std::make_shared<H2Core::Drumkit>( pKit );

	pKitCopy->getInstruments()->get( 0 )->setGain( fNewGainValue );

	auto pKitAgain = pDB->getDrumkit( "GMRockKit" );
	CPPUNIT_ASSERT( pKitAgain != nullptr );
	CPPUNIT_ASSERT( pKitAgain->getInstruments()->get( 0 )->getGain() !=
					fNewGainValue );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testKitRetrievalDirect() {
	___INFOLOG( "" );

	auto pDB = H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase();

	const float fNewGainValue = 1.23456;

	auto pKit = pDB->getDrumkit( "GMRockKit" );
	CPPUNIT_ASSERT( pKit != nullptr );

	const float fOldValue = pKit->getInstruments()->get( 0 )->getGain();
	pKit->getInstruments()->get( 0 )->setGain( fNewGainValue );

	auto pKitAgain = pDB->getDrumkit( "GMRockKit" );
	CPPUNIT_ASSERT( pKitAgain != nullptr );
	CPPUNIT_ASSERT( pKitAgain->getInstruments()->get( 0 )->getGain() ==
					fNewGainValue );

	pKit->getInstruments()->get( 0 )->setGain( fOldValue );

	___INFOLOG( "passed" );
}
