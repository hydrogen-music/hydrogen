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

#include "DrumkitExportTest.h"

#include "assertions/File.h"
#include "TestHelper.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <memory>

using namespace H2Core;

void DrumkitExportTest::setUp() {
	// Ensure the test kit is not installed on the system.
	m_sTestKitName = "ếИ£TestKit越";

	// We do not check return value as the folder should not exist in the first
	// place.
	Filesystem::rm( Filesystem::drumkit_usr_path( m_sTestKitName ), true, true );

	Hydrogen::get_instance()->getCoreActionController()->openSong(
		H2TEST_FILE( "functional/test.h2song" ) );
}

void DrumkitExportTest::tearDown() {
	// Remove the test kit from the system.
	Filesystem::rm( Filesystem::drumkit_usr_path( m_sTestKitName ), true, true );

	// Discard all changes to the test song.
	Hydrogen::get_instance()->getCoreActionController()->openSong(
		H2TEST_FILE( "functional/test.h2song" ) );
}

void DrumkitExportTest::testDrumkitExportAndImport() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	const QString sTestKitPath =
		H2TEST_FILE( QString( "drumkits/%1%2" ).arg( m_sTestKitName )
					 .arg( Filesystem::drumkit_ext ) );

	// Check validity of test kit
	CPPUNIT_ASSERT( pCoreActionController->validateDrumkit(
						sTestKitPath, false ) );

	// Import test kit into Hydrogen.
	CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sTestKitPath ) );

	// Check whether import worked, the UTF-8 path and name was read properly,
	// and all samples are present.
	const auto pDB = pHydrogen->getSoundLibraryDatabase();
	const QString sExtractedKit = Filesystem::drumkit_usr_path( m_sTestKitName );
	const auto pDrumkit = pDB->getDrumkit( sExtractedKit, true );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->get_name() == m_sTestKitName );
	for ( const auto& ppInstrument : *pDrumkit->get_instruments() ) {
		CPPUNIT_ASSERT( ! ppInstrument->has_missing_samples() );
	}

	// Load the kit and export it.
	CPPUNIT_ASSERT( pDrumkit->exportTo( Filesystem::tmp_dir() ) );

	// Bitwise comparison of the (!extracted!) original drumkit and the one we
	// just exported.
	const QString sExportPath = QString( "%1%2%3" )
		.arg( Filesystem::tmp_dir() ).arg( m_sTestKitName )
		.arg( Filesystem::drumkit_ext );
	QTemporaryDir exportValidation( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
	exportValidation.setAutoRemove( false );
	CPPUNIT_ASSERT( pCoreActionController->extractDrumkit(
						sExportPath, exportValidation.path() ) );

	H2TEST_ASSERT_DIRS_EQUAL( exportValidation.path(), sExtractedKit );

	// Cleanup
	H2Core::Filesystem::rm( exportValidation.path(), true, true );

	___INFOLOG( "passed" );
}