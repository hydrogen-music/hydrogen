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

#include "CliTest.h"

#include <QFileInfo>

#include "TestHelper.h"
#include "assertions/File.h"

#include <core/config.h>
#include <core/Basics/Drumkit.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <qprocess.h>

using namespace H2Core;

void CliTest::setUp() {
	___INFOLOG( "" );
	m_sCliPath = QString( "%1/%2" ).arg( CMAKE_BINARY_DIR )
		.arg( "/src/cli/h2cli" );

	CPPUNIT_ASSERT( QFileInfo::exists( m_sCliPath ) );
	___INFOLOG( "passed" );
}

void CliTest::testKitToDrumkitMap() {
	___INFOLOG( "" );

	const QString sRefFolder = H2TEST_FILE( "drumkits/sampleKit" );
	// Check whether things work for a kit without any kits too.
	const QString sNoTypesFolder = H2TEST_FILE( "drumkits/baseKit" );
	// We load the kits to ensure they are clean and can be loaded.
	const auto pDrumkitRef = Drumkit::load(
		Filesystem::drumkitPathFromDir( sRefFolder ), false, nullptr,
		true, pTestHydrogen()
	);
	CPPUNIT_ASSERT( pDrumkitRef != nullptr );
	const auto pDrumkitNoTypes = Drumkit::load(
		Filesystem::drumkitPathFromDir( sNoTypesFolder ), false,
		nullptr, true, pTestHydrogen()
	);
	CPPUNIT_ASSERT( pDrumkitNoTypes != nullptr );

	// Now, we also write the output and compare it with reference files.
	const QString sTmpRefFile =
		Filesystem::tmpDir() + "sample-cli.h2map";
	QStringList argsRefFile;
	argsRefFile << "--kitToDrumkitMap" << sRefFolder
		<< "-o" << sTmpRefFile;
	auto pProcessRefFile = new QProcess();
	pProcessRefFile->start( m_sCliPath, argsRefFile );
	CPPUNIT_ASSERT( pProcessRefFile->waitForFinished() );
	CPPUNIT_ASSERT( pProcessRefFile->exitCode() == 0 );

	H2TEST_ASSERT_XML_FILES_EQUAL(
		sTmpRefFile, H2TEST_FILE( "drumkit_map/sample.h2map" ) );

	Filesystem::rm( sTmpRefFile, true );

	// And now for the empty one.

	const QString sTmpNoTypesFile =
		Filesystem::tmpDir() + "empty-cli.h2map";
	QStringList argsNoTypesFile;
	argsNoTypesFile << "--kitToDrumkitMap" << sNoTypesFolder
		<< "-o" << sTmpNoTypesFile;
	auto pProcessNoTypesFile = new QProcess();
	pProcessNoTypesFile->start( m_sCliPath, argsNoTypesFile );
	CPPUNIT_ASSERT( pProcessNoTypesFile->waitForFinished() );
	CPPUNIT_ASSERT( pProcessNoTypesFile->exitCode() == 0 );

	H2TEST_ASSERT_XML_FILES_EQUAL(
		sTmpNoTypesFile, H2TEST_FILE( "drumkit_map/empty.h2map" ) );

	XMLDoc docNoTypes;
	CPPUNIT_ASSERT( docNoTypes.read( sTmpNoTypesFile ) );

	Filesystem::rm( sTmpNoTypesFile, true );

	___INFOLOG( "passed" );
}

#if defined(Q_OS_MACX) || defined(WIN32)
#else
void CliTest::testXdgPaths() {
	___INFOLOG( "" );

	auto rm = []( const QString& sDir ) {
		if ( Filesystem::dirExists( sDir, true ) ) {
			return Filesystem::rm( sDir, true, true );
		}
		return true;
	};

	const QString sKitPath = H2TEST_FILE( "drumkits/testKit.h2drumkit" );
	CPPUNIT_ASSERT( Filesystem::fileExists( sKitPath ) );

	const QString sOldUserDir = QDir::homePath().append( "/" H2_USR_PATH "/" );
	const bool bOldUserDirPresent = Filesystem::dirExists( sOldUserDir, true );
	bool bTestKitOldBackedUp = false;
	const QString sTestKitOldPath = sOldUserDir + "data/drumkits/testKit";
	const QString sTestKitOldBackupPath = Filesystem::tmpDir() + "/oldTestKit";
	rm( sTestKitOldBackupPath );
	if ( bOldUserDirPresent && Filesystem::dirExists( sTestKitOldPath, true ) ) {
		QDir dir;
		// Move the whole folder
		CPPUNIT_ASSERT( dir.rename( sTestKitOldPath, sTestKitOldBackupPath ) );
		bTestKitOldBackedUp = true;
	}

	// We back up the user-level data folder as well.
	bool bOldUserDirBackedUp = false;
	const QString sOldUserDirBackupPath = Filesystem::tmpDir() + "/oldUserData";
	rm( sOldUserDirBackupPath );
	if ( bOldUserDirPresent ) {
		QDir dir;
		// Move the whole folder
		if ( ! dir.rename( sOldUserDir, sOldUserDirBackupPath ) ) {
			// Cleanup
			if ( bTestKitOldBackedUp ) {
				dir.rename( sTestKitOldBackupPath, sTestKitOldPath );
			}
			CPPUNIT_FAIL( "Unable to back up old user home folder" );
		}
		bOldUserDirBackedUp = true;
	}

	auto tearDown = [&]( const QString& sMsg ) {
		if ( bOldUserDirBackedUp ) {
			QDir dir;
			// Move the whole folder
			dir.rename( sOldUserDirBackupPath, sOldUserDir );
		}
		if ( bTestKitOldBackedUp ) {
			QDir dir;
			dir.rename( sTestKitOldBackupPath, sTestKitOldPath );
		}
		CPPUNIT_FAIL( sMsg.toStdString() );
	};

	// First, we check whether the old data folder is honored.
	if ( Filesystem::dirExists( sOldUserDir, true ) ) {
		tearDown( "Old user dir must not be present prior to test" );
	}
	if ( ! Filesystem::mkdir( sOldUserDir ) ) {
		tearDown( "Unable to create old user dir" );
	}

	QStringList argsRefFile;
	argsRefFile << "-i" << sKitPath << "-VDebug";

	const QString sXdgTmpData = Filesystem::tmpDir() + "/tmpXdgData";
	const QString sXdgTmpCache = Filesystem::tmpDir() + "/tmpXdgCache";
	const QString sXdgTmpConfig = Filesystem::tmpDir() + "/tmpXdgConfig";
	rm( sXdgTmpCache );
	rm( sXdgTmpConfig );
	rm( sXdgTmpData );

	auto startProcess = [&]( bool bUseXdg ) {
		const QString sContext = bUseXdg ? "XDG dir" : "old user dir";

		___INFOLOG( QString( "Starting process [%1]" ).arg( sContext ) );
		auto pProcessRefFile = new QProcess();
		// Connect signals to slots or lambdas to read output as it arrives
		QObject::connect(
			pProcessRefFile, &QProcess::readyReadStandardOutput,
			[pProcessRefFile]() {
				QString output =
					QString::fromUtf8( pProcessRefFile->readAllStandardOutput()
					);
				___DEBUGLOG( QString( "Stdout: %1" ).arg( output ) );
			}
		);

		QObject::connect(
			pProcessRefFile, &QProcess::readyReadStandardError,
			[pProcessRefFile]() {
				QString error =
					QString::fromUtf8( pProcessRefFile->readAllStandardError()
					);
				___DEBUGLOG( QString( "Stderr: %1" ).arg( error ) );
			}
		);
		if ( bUseXdg ) {
			auto env = QProcessEnvironment::systemEnvironment();
			env.insert( "XDG_CONFIG_HOME", sXdgTmpConfig );
			env.insert( "XDG_CACHE_HOME", sXdgTmpCache );
			env.insert( "XDG_DATA_HOME", sXdgTmpData );
			pProcessRefFile->setProcessEnvironment( env );
		}
		pProcessRefFile->start( m_sCliPath, argsRefFile );
		if ( !pProcessRefFile->waitForFinished() ) {
			tearDown( QString( "h2cli on [%1] did not return" ).arg( sContext )
			);
		}
		if ( pProcessRefFile->exitCode() != 0 ) {
			tearDown( QString( "h2cli on [%1] existed with %1" )
						  .arg( sContext )
						  .arg( pProcessRefFile->exitCode() ) );
		}
	};

	startProcess( false );

	if ( ! Filesystem::dirExists( sTestKitOldPath, true ) ) {
		tearDown( "Test kit was not installed to old data dir as expected." );
	}

	if ( ! Filesystem::rm( sOldUserDir, true ) ) {
		tearDown( "Unable to remove old user dir." );
	}

	// Now we test the same thing with the XDG counter part.
	const QString sTestKitXdgPath = sXdgTmpData + "/hydrogen/drumkits/testKit";
	startProcess( true );

	if ( ! Filesystem::dirExists( sTestKitXdgPath, true ) ) {
		tearDown( "Test kit was not installed to XDG data dir as expected." );
	}

	// Cleanup
		if ( bOldUserDirBackedUp ) {
			QDir dir;
			// Move the whole folder
			rm( sOldUserDir );
			dir.rename( sOldUserDirBackupPath, sOldUserDir );
		}
		if ( bTestKitOldBackedUp ) {
			QDir dir;
			rm( sTestKitOldPath );
			dir.rename( sTestKitOldBackupPath, sTestKitOldPath );
		}

	Filesystem::rm( sXdgTmpData, true );
	Filesystem::rm( sXdgTmpConfig, true );
	Filesystem::rm( sXdgTmpCache, true );
	Filesystem::rm( sTestKitOldBackupPath, true );

	___INFOLOG( "passed" );
}
#endif
