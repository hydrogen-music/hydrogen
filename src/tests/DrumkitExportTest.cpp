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
	m_sTestKitName = "testKit";
	m_sTestKitNameSampleFormats = "sampleFormatKit";
	m_sTestKitNameUtf8 = "ếИ£TestKit越";

	// We do not check return value as the folder should not exist in the first
	// place.
	if ( Filesystem::dirExists(
			 Filesystem::userDrumkitsDir() + m_sTestKitName, true
		 ) ) {
		Filesystem::rm(
			Filesystem::userDrumkitsDir() + m_sTestKitName, true, true
		);
	}
	if ( Filesystem::dirExists(
			 Filesystem::userDrumkitsDir() + m_sTestKitNameSampleFormats, true
		 ) ) {
		Filesystem::rm(
			Filesystem::userDrumkitsDir() + m_sTestKitNameSampleFormats, true,
			true
		);
	}
	if ( Filesystem::dirExists(
			 Filesystem::userDrumkitsDir() + m_sTestKitNameUtf8, true
		 ) ) {
		Filesystem::rm(
			Filesystem::userDrumkitsDir() + m_sTestKitNameUtf8, true, true
		);
	}

	auto pSong =
		H2Core::Hydrogen::get_instance()->getCoreActionController()->loadSong( H2TEST_FILE( "functional/test.h2song" )
		);
	H2Core::Hydrogen::get_instance()->getCoreActionController()->setSong( pSong );
}

void DrumkitExportTest::tearDown()
{
	// Remove the test kit from the system.
	if ( Filesystem::dirExists(
			 Filesystem::userDrumkitsDir() + m_sTestKitName, true
		 ) ) {
		Filesystem::rm(
			Filesystem::userDrumkitsDir() + m_sTestKitName, true, true
		);
	}
	if ( Filesystem::dirExists(
			 Filesystem::userDrumkitsDir() + m_sTestKitNameSampleFormats, true
		 ) ) {
		Filesystem::rm(
			Filesystem::userDrumkitsDir() + m_sTestKitNameSampleFormats, true,
			true
		);
	}
	if ( Filesystem::dirExists(
			 Filesystem::userDrumkitsDir() + m_sTestKitNameUtf8, true
		 ) ) {
		Filesystem::rm(
			Filesystem::userDrumkitsDir() + m_sTestKitNameUtf8, true, true
		);
	}

	// Discard all changes to the test song.
	auto pSong =
		H2Core::Hydrogen::get_instance()->getCoreActionController()->loadSong( H2TEST_FILE( "functional/test.h2song" )
		);
	H2Core::Hydrogen::get_instance()->getCoreActionController()->setSong( pSong );
}

void DrumkitExportTest::testDrumkitExportAndImport() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	const QString sTestKitPath =
		H2TEST_FILE( QString( "drumkits/%1%2" ).arg( m_sTestKitName )
					 .arg( Filesystem::sDrumkitSuffix ) );

	// Check validity of test kit
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->validateDrumkit(
						sTestKitPath, false ) );

	// Import test kit into Hydrogen.
	QString sInstalledDir;
	CPPUNIT_ASSERT(
		H2Core::Hydrogen::get_instance()->getCoreActionController()->extractDrumkit( sTestKitPath, "", &sInstalledDir )
	);

	// Check whether import worked, the UTF-8 path and name was read properly,
	// and all samples are present.
	const auto pDB = pHydrogen->getSoundLibraryDatabase();
	const QString sExtractedKitPath =
		Filesystem::userDrumkitsDir() + m_sTestKitName + "/drumkit.xml";
	const auto pDrumkit = pDB->getDrumkit( sExtractedKitPath );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	ASSERT_PATH(
		Filesystem::drumkitPathFromDir( sInstalledDir ), sExtractedKitPath
	);
	CPPUNIT_ASSERT( pDrumkit->getName() == m_sTestKitName );
	for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
		CPPUNIT_ASSERT( ! ppInstrument->hasMissingSamples() );
	}

	// Load the kit and export it.
	CPPUNIT_ASSERT( pDrumkit->exportTo( Filesystem::tmpDir() ) );

	// Bitwise comparison of the (!extracted!) original drumkit and the one we
	// just exported.
	const QString sExportPath = QString( "%1%2%3" )
		.arg( Filesystem::tmpDir() ).arg( m_sTestKitName )
		.arg( Filesystem::sDrumkitSuffix );
	QTemporaryDir exportValidation( H2Core::Filesystem::tmpDir() + "-XXXXXX" );
	exportValidation.setAutoRemove( false );
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->extractDrumkit(
						sExportPath, exportValidation.path() ) );

	H2TEST_ASSERT_DIRS_EQUAL(
		exportValidation.path() + QDir::separator() + m_sTestKitName +
			QDir::separator() + Filesystem::drumkitXml(),
		sExtractedKitPath
	);

	// Cleanup
	H2Core::Filesystem::rm( exportValidation.path(), true, true );
	H2Core::Filesystem::rm( sExportPath, false, true );

	___INFOLOG( "passed" );
}

void DrumkitExportTest::testDrumkitExportAndImportSampleFormats() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	const QString sTestKitPath =
		H2TEST_FILE( QString( "drumkits/%1%2" )
					 .arg( m_sTestKitNameSampleFormats )
					 .arg( Filesystem::sDrumkitSuffix ) );

	// Check validity of test kit
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->validateDrumkit(
						sTestKitPath, false ) );

	// Import test kit into Hydrogen.
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->extractDrumkit( sTestKitPath ) );

	// Check whether import worked, the UTF-8 path and name was read properly,
	// and all samples are present.
	const auto pDB = pHydrogen->getSoundLibraryDatabase();
	const QString sExtractedKitPath = Filesystem::userDrumkitsDir() +
									  m_sTestKitNameSampleFormats +
									  "/drumkit.xml";
	const auto pDrumkit = pDB->getDrumkit( sExtractedKitPath );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getName() == m_sTestKitNameSampleFormats );
	for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
		CPPUNIT_ASSERT( ! ppInstrument->hasMissingSamples() );
	}

	// Load the kit and export it.
	CPPUNIT_ASSERT( pDrumkit->exportTo( Filesystem::tmpDir() ) );

	// Bitwise comparison of the (!extracted!) original drumkit and the one we
	// just exported.
	const QString sExportPath = QString( "%1%2%3" )
		.arg( Filesystem::tmpDir() ).arg( m_sTestKitNameSampleFormats )
		.arg( Filesystem::sDrumkitSuffix );
	QTemporaryDir exportValidation( H2Core::Filesystem::tmpDir() + "-XXXXXX" );
	exportValidation.setAutoRemove( false );
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->extractDrumkit(
						sExportPath, exportValidation.path() ) );

	H2TEST_ASSERT_DIRS_EQUAL(
		exportValidation.path() + QDir::separator() +
			m_sTestKitNameSampleFormats + QDir::separator() +
			Filesystem::drumkitXml(),
		sExtractedKitPath
	);

	// Cleanup
	H2Core::Filesystem::rm( exportValidation.path(), true, true );
	H2Core::Filesystem::rm( sExportPath, false, true );

	___INFOLOG( "passed" );
}

void DrumkitExportTest::testDrumkitExportAndImportUtf8() {
	___INFOLOG( "" );

	auto pHydrogen = Hydrogen::get_instance();

	const QString sTestKitPath =
		H2TEST_FILE( QString( "drumkits/%1%2" ).arg( m_sTestKitNameUtf8 )
					 .arg( Filesystem::sDrumkitSuffix ) );

	// Check validity of test kit
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->validateDrumkit(
						sTestKitPath, false ) );

	// Import test kit into Hydrogen.
	QString sInstalledDir;
	bool bEncodingIssuesDetected;
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->extractDrumkit(
						sTestKitPath, "", &sInstalledDir,
						&bEncodingIssuesDetected ) );

	// Check whether import worked, the UTF-8 path and name was read properly,
	// and all samples are present.
	const auto pDB = pHydrogen->getSoundLibraryDatabase();
	const auto pDrumkit =
		pDB->getDrumkit( Filesystem::drumkitPathFromDir( sInstalledDir ) );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getName() == m_sTestKitNameUtf8 );
	if ( ! bEncodingIssuesDetected ) {
		// This can cause file names of sample files to extracted from the tar
		// ball to get altered. But as we do not touch the drumkit definition
		// itself, they will be considered a missing sample.
		for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
			CPPUNIT_ASSERT( ! ppInstrument->hasMissingSamples() );
		}
	}

	// Load the kit and export it.
	bool bUtf8SupportOnSystem;
	const auto bDrumkitExportSuccessful = pDrumkit->exportTo(
		Filesystem::tmpDir(), &bUtf8SupportOnSystem );
	if ( ! bUtf8SupportOnSystem ) {
		___WARNINGLOG( "UTF-8 support couldn't be enforced. Unit test not applicable." )
		___INFOLOG( "skipped" );
		return;
	}

	CPPUNIT_ASSERT( bDrumkitExportSuccessful );

	// Bitwise comparison of the (!extracted!) original drumkit and the one we
	// just exported.
	const QString sExportPath = QString( "%1%2%3" )
		.arg( Filesystem::tmpDir() ).arg( m_sTestKitNameUtf8 )
		.arg( Filesystem::sDrumkitSuffix );

	QTemporaryDir exportValidation( H2Core::Filesystem::tmpDir() + "-XXXXXX" );
	exportValidation.setAutoRemove( false );
	CPPUNIT_ASSERT( H2Core::Hydrogen::get_instance()->getCoreActionController()->extractDrumkit(
						sExportPath, exportValidation.path() ) );

	QDir exportDir( exportValidation.path() );
	// Should contain just the drumkit folder.
	auto exportDirContent =
		exportDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
	CPPUNIT_ASSERT( exportDirContent.size() == 1 );
	H2TEST_ASSERT_DIRS_EQUAL( exportDir.filePath( exportDirContent[ 0 ] ),
							  sInstalledDir );

	// Cleanup
	H2Core::Filesystem::rm( exportValidation.path(), true, true );
	H2Core::Filesystem::rm( sInstalledDir, true, true );

	___INFOLOG( "passed" );
}
