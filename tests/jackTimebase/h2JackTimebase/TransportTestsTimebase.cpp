/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/CoreActionController.h>
#include <core/AudioEngine/AudioEngineTests.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Helpers/Filesystem.h>

#include <iostream>

#include "TransportTestsTimebase.h"
#include "TestHelper.h"

#include "assertions/AudioFile.h"

using namespace H2Core;

void TransportTestsTimebase::testTransportProcessingJack() {
	___INFOLOG( "\n\n" );
	auto pHydrogen = Hydrogen::get_instance();

	auto pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" )
								   .arg( Filesystem::demos_dir() ) );
	CPPUNIT_ASSERT( pSongDemo != nullptr );
	pHydrogen->getCoreActionController()->openSong( pSongDemo );

	perform( &AudioEngineTests::testTransportProcessingJack );

	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::testTransportRelocationJack() {
	___INFOLOG( "\n\n" );
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	auto pSongDemo = Song::load( QString( "%1/GM_kit_demo3.h2song" )
								   .arg( Filesystem::demos_dir() ) );
	CPPUNIT_ASSERT( pSongDemo != nullptr );
	pCoreActionController->openSong( pSongDemo );
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 1, 100 );
	pCoreActionController->addTempoMarker( 2, 20 );
	pCoreActionController->addTempoMarker( 3, 13.4 );
	pCoreActionController->addTempoMarker( 4, 383.2 );
	pCoreActionController->addTempoMarker( 5, 64.38372 );
	pCoreActionController->addTempoMarker( 6, 96.3 );
	pCoreActionController->addTempoMarker( 7, 240.46 );
	pCoreActionController->addTempoMarker( 8, 200.1 );
	
	perform( &AudioEngineTests::testTransportRelocationJack );

	pCoreActionController->activateTimeline( false );
	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::perform( std::function<void()> func ) {
	try {
		func();
	} catch ( std::exception& err ) {
		CppUnit::Message msg( err.what() );
		throw CppUnit::Exception( msg );
	}
}
