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

#include "FilesystemTest.h"

#include "TestHelper.h"

#include <cppunit/extensions/HelperMacros.h>

#include <QTest>

#include <core/Helpers/Filesystem.h>

using namespace H2Core;

void FilesystemTest::setUp() {
#if !defined(WIN32) 
	m_sNotExistingPath = QDir::homePath().append( "aFunnyNameYouWouldNotExpectInYourHomeFolder.h2song" );
#ifdef Q_OS_BSD4
	m_sNoAccessPath = "/etc/master.passwd";
#else
	m_sNoAccessPath = "/etc/shadow";
#endif
	m_sReadOnlyPath = "/etc/hosts";
	m_sFullAccessPath = QString( Filesystem::userDataPath() )
		.append( "test.h2song"  );
	m_sTmpPath = Filesystem::tmpFilePath( "test.h2song" );
#endif
}

void FilesystemTest::tearDown() {
#ifndef WIN32
	CPPUNIT_ASSERT( Filesystem::rm( m_sTmpPath, false ) );
#endif
}

void FilesystemTest::testPermissions(){
#ifndef WIN32
	___INFOLOG( "" );
	CPPUNIT_ASSERT( ! Filesystem::fileExists( m_sNotExistingPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileExists( m_sNoAccessPath, true ) );
	CPPUNIT_ASSERT( ! Filesystem::fileReadable( m_sNoAccessPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileReadable( m_sReadOnlyPath, true ) );
	CPPUNIT_ASSERT( ! Filesystem::fileWritable( m_sReadOnlyPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileWritable( m_sFullAccessPath, true ) );

	CPPUNIT_ASSERT( Filesystem::fileExists( m_sTmpPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileReadable( m_sTmpPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileWritable( m_sTmpPath, true ) );
	___INFOLOG( "passed" );
#endif
}

void FilesystemTest::testUniquePrefix() {
	const QString sBasePath( QDir::temp().absoluteFilePath( "base" ) );

	QString sBaseUniquePath, sBasePathAgain;
	for ( int ii = 0; ii < 10; ++ii ) {
		sBaseUniquePath = Filesystem::addUniquePrefix( sBasePath );
		ASSERT_PATH_UNEQUAL( sBaseUniquePath, sBasePath );

		sBasePathAgain = Filesystem::removeUniquePrefix( sBaseUniquePath );
		ASSERT_PATH( sBasePathAgain, sBasePath );
	}

	// Almost our prefix, but not exactly.
	const QString sNoPrefixPath( QDir::temp().absoluteFilePath(
									 "tmp-AEWF-test.h2song" ) );
	const QString sNoPrefixPath2( QDir::temp().absoluteFilePath(
									 "tmp-AEWFDSD4-test.h2song" ) );

	ASSERT_PATH( sNoPrefixPath,
					Filesystem::removeUniquePrefix( sNoPrefixPath ) );
	ASSERT_PATH( sNoPrefixPath2,
					Filesystem::removeUniquePrefix( sNoPrefixPath2 ) );
}

void FilesystemTest::testFilePathValidation() {
	___INFOLOG( "" );
	QStringList invalidFileNames, validFileNames;

	validFileNames << "test.h2song" << "123-te-s_t.h2drumkit"
		<< "ếИ£TestKit越.h2pattern";
	invalidFileNames << "te/s/t/.h2song" << "test@h2song" << "t\\e\\s\\t.h2song"
		<< "?!h2song" << "test*.%h2drumkit";

	for ( const auto& ssName : validFileNames ) {
		const auto ssValidated = Filesystem::validateFilePath( ssName );
		CPPUNIT_ASSERT( ssName == ssValidated );
	}
	for ( const auto& ssName : invalidFileNames ) {
		const auto ssValidated = Filesystem::validateFilePath( ssName );
		const auto ssValidatedTwice = Filesystem::validateFilePath( ssValidated );
		CPPUNIT_ASSERT( ssName != ssValidated );
		CPPUNIT_ASSERT( ssValidatedTwice == ssValidated );
	}
	___INFOLOG( "passed" );
}

void FilesystemTest::testListContent() {
	___INFOLOG( "" );

	// We don't want this test to fail every time we add a new pattern, drumkit,
	// or song. That's why we just check for the present of some content on
	// system-level (the data folder of this repo).
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::DrumkitBundled,
						Filesystem::Context::System
	)
						.isEmpty() );
	CPPUNIT_ASSERT( !Filesystem::listContent(
						 Filesystem::Artifact::DrumkitExtracted,
						 Filesystem::Context::System
	)
						 .isEmpty() );
	CPPUNIT_ASSERT( !Filesystem::listContent(
						 Filesystem::Artifact::Pattern,
						 Filesystem::Context::System
	)
						 .isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Playlist,
						Filesystem::Context::System
	)
						.isEmpty() );
	CPPUNIT_ASSERT( !Filesystem::listContent(
						Filesystem::Artifact::Song, Filesystem::Context::System
	)
						.isEmpty() );

	// There won't be any session artifacts during unit testing.
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::DrumkitBundled,
						Filesystem::Context::SessionReadOnly
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::DrumkitExtracted,
						Filesystem::Context::SessionReadOnly
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Pattern,
						Filesystem::Context::SessionReadOnly
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Playlist,
						Filesystem::Context::SessionReadOnly
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Song,
						Filesystem::Context::SessionReadOnly
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::DrumkitBundled,
						Filesystem::Context::SessionReadWrite
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::DrumkitExtracted,
						Filesystem::Context::SessionReadWrite
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Pattern,
						Filesystem::Context::SessionReadWrite
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Playlist,
						Filesystem::Context::SessionReadWrite
	)
						.isEmpty() );
	CPPUNIT_ASSERT( Filesystem::listContent(
						Filesystem::Artifact::Song,
						Filesystem::Context::SessionReadWrite
	)
						.isEmpty() );

	// For the user folder we build up a dummy one containing various empty
	// files pretending to be Hydrogen artifacts.
	const QString sDummyDir(
		H2Core::Filesystem::tmpDir() + "/testListArtifacts"
	);
	if ( Filesystem::dirExists( sDummyDir ) ) {
		Filesystem::rm( sDummyDir, true, true );
	}

	CPPUNIT_ASSERT( Filesystem::mkdir( sDummyDir ) );
	CPPUNIT_ASSERT( Filesystem::mkdir( sDummyDir + "/first" ) );
	CPPUNIT_ASSERT( Filesystem::mkdir( sDummyDir + "/first/second" ) );
	QFile firstLevelDrumkit( sDummyDir + "/first/drumkit.xml" );
	CPPUNIT_ASSERT( firstLevelDrumkit.open( QIODevice::WriteOnly ) );
	QFile firstLevelDrumkitBundled( sDummyDir + "/first/test.h2drumkit" );
	CPPUNIT_ASSERT( firstLevelDrumkitBundled.open( QIODevice::WriteOnly ) );
	QFile secondLevelDrumkit( sDummyDir + "/first/second/drumkit.xml" );
	CPPUNIT_ASSERT( secondLevelDrumkit.open( QIODevice::WriteOnly ) );
	QFile topLevelPattern( sDummyDir + "/top.h2pattern" );
	CPPUNIT_ASSERT( topLevelPattern.open( QIODevice::WriteOnly ) );
	QFile topLevelPattern2( sDummyDir + "/another.h2pattern" );
	CPPUNIT_ASSERT( topLevelPattern2.open( QIODevice::WriteOnly ) );
	QFile firstLevelPattern( sDummyDir + "/first/first.h2pattern" );
	CPPUNIT_ASSERT( firstLevelPattern.open( QIODevice::WriteOnly ) );
	QFile secondLevelPattern( sDummyDir + "/first/second/second.h2pattern" );
	CPPUNIT_ASSERT( secondLevelPattern.open( QIODevice::WriteOnly ) );
	QFile topLevelPlaylist( sDummyDir + "/top.h2playlist" );
	CPPUNIT_ASSERT( topLevelPlaylist.open( QIODevice::WriteOnly ) );
	QFile firstLevelPlaylist( sDummyDir + "/first/first.h2playlist" );
	CPPUNIT_ASSERT( firstLevelPlaylist.open( QIODevice::WriteOnly ) );
	QFile secondLevelPlaylist( sDummyDir + "/first/second/second.h2playlist" );
	CPPUNIT_ASSERT( secondLevelPlaylist.open( QIODevice::WriteOnly ) );
	QFile topLevelSong( sDummyDir + "/top.h2song" );
	CPPUNIT_ASSERT( topLevelSong.open( QIODevice::WriteOnly ) );
	QFile firstLevelSong( sDummyDir + "/first/first.h2song" );
	CPPUNIT_ASSERT( firstLevelSong.open( QIODevice::WriteOnly ) );
	QFile secondLevelSong( sDummyDir + "/first/second/second.h2song" );
	CPPUNIT_ASSERT( secondLevelSong.open( QIODevice::WriteOnly ) );
	QFile secondLevelSong2( sDummyDir + "/first/second/another.h2song" );
	CPPUNIT_ASSERT( secondLevelSong2.open( QIODevice::WriteOnly ) );

	// There is no user-level folder associated with bundled drumkits. They are
	// meant to moving transport to other devices only.
	CPPUNIT_ASSERT(
		Filesystem::listContent(
			Filesystem::Artifact::DrumkitBundled, Filesystem::Context::User,
			sDummyDir
		)
			.size() == 0
	);
	CPPUNIT_ASSERT(
		Filesystem::listContent(
			Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::User,
			sDummyDir

		)
			.size() == 2
	);
	CPPUNIT_ASSERT(
		Filesystem::listContent(
			Filesystem::Artifact::Pattern, Filesystem::Context::User, sDummyDir

		)
			.size() == 4
	);
	CPPUNIT_ASSERT(
		Filesystem::listContent(
			Filesystem::Artifact::Playlist, Filesystem::Context::User, sDummyDir

		)
			.size() == 3
	);
	CPPUNIT_ASSERT(
		Filesystem::listContent(
			Filesystem::Artifact::Song, Filesystem::Context::User, sDummyDir

		)
			.size() == 4
	);

	// Cleanup
	Filesystem::rm( sDummyDir, true, true );

	___INFOLOG( "passed" );
}

void FilesystemTest::testIsPathValid() {
	___INFOLOG( "" );

	// Is not absolute.
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Artifact::DrumkitBundled, "test.h2drumkit" ) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Artifact::DrumkitExtracted, "drumkit.xml" ) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Artifact::Pattern, "test.h2pattern" ) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Artifact::Playlist, "test.h2playlist" ) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
						Filesystem::Artifact::Song, "test.h2song" ) );

	const auto sTmp = QDir::tempPath() + QDir::separator();

	// Improper suffix.
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
		Filesystem::Artifact::DrumkitBundled, sTmp + "test.test"
	) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
		Filesystem::Artifact::DrumkitExtracted, sTmp + "test.test"
	) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
		Filesystem::Artifact::Pattern, sTmp + "test.test"
	) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
		Filesystem::Artifact::Playlist, sTmp + "test.test"
	) );
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
		Filesystem::Artifact::Song, sTmp + "test.test"
	) );

	// Improper name.
	CPPUNIT_ASSERT( !Filesystem::isPathValid(
		Filesystem::Artifact::DrumkitExtracted, sTmp + "test.xml"
	) );

	// Valid ones
	CPPUNIT_ASSERT( Filesystem::isPathValid(
		Filesystem::Artifact::DrumkitBundled, sTmp + "test.h2drumkit"
	) );
	CPPUNIT_ASSERT( Filesystem::isPathValid(
		Filesystem::Artifact::DrumkitExtracted, sTmp + "drumkit.xml"
	) );
	CPPUNIT_ASSERT( Filesystem::isPathValid(
		Filesystem::Artifact::Pattern, sTmp + "test.h2pattern"
	) );
	CPPUNIT_ASSERT( Filesystem::isPathValid(
		Filesystem::Artifact::Playlist, sTmp + "test.h2playlist"
	) );
	CPPUNIT_ASSERT( Filesystem::isPathValid(
		Filesystem::Artifact::Song, sTmp + "test.h2song"
	) );

	___INFOLOG( "passed" );
}

void FilesystemTest::testSamplePathHandling() {
	___INFOLOG( "" );
	const QString sSystemKitDir(
		QString( "%1sampleKit" ).arg( Filesystem::systemDrumkitsDir() )
	);
	const QString sUserKitDir(
		QString( "%1/sampleKit" ).arg( Filesystem::userDrumkitsDir() )
	);
	const QString sCustomKitDir( "/home/user/folder/sampleKit" );
	const QString sSystemKitPath = Filesystem::drumkitPathFromDir( sSystemKitDir );
	const QString sUserKitPath = Filesystem::drumkitPathFromDir( sUserKitDir );
	const QString sCustomKitPath = Filesystem::drumkitPathFromDir( sCustomKitDir );
	const auto sPathInSystemKit = QString( "%1/sample.wav" )
		.arg( sSystemKitDir );
	const auto sPathInUserKit = QString( "%1/sample.wav" )
		.arg( sUserKitDir );
	const auto sPathInCustomKit = QString( "%1/sample.wav" )
		.arg( sCustomKitDir );
	const auto sAbsolutePath( "/path/to/sample.wav" );
	const auto sRelativePath( "../../sample.wav" );
	const QString sFileName( "sample.wav" );

	const auto sPathInSystemKitPrepared = Filesystem::prepareSamplePath(
		sPathInSystemKit, sSystemKitPath );
	___INFOLOG( QString( "sPathInSystemKitPrepared: [%1]" )
				.arg( sPathInSystemKitPrepared ) );
	ASSERT_PATH_UNEQUAL( sPathInSystemKit, sPathInSystemKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInSystemKitPrepared );

	const auto sPathInUserKitPrepared = Filesystem::prepareSamplePath(
		sPathInUserKit, sUserKitPath );
	___INFOLOG( QString( "sPathInUserKitPrepared: [%1]" )
				.arg( sPathInUserKitPrepared ) );
	ASSERT_PATH_UNEQUAL( sPathInUserKit, sPathInUserKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInUserKitPrepared );

	const auto sPathInCustomKitPrepared = Filesystem::prepareSamplePath(
		sPathInCustomKit, sCustomKitPath );
	___INFOLOG( QString( "sPathInCustomKitPrepared: [%1]" )
				.arg( sPathInCustomKitPrepared ) );
	ASSERT_PATH_UNEQUAL( sPathInCustomKit, sPathInCustomKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInCustomKitPrepared );

	const auto sAbsolutePathPrepared = Filesystem::prepareSamplePath(
		sAbsolutePath, "" );
	___INFOLOG( QString( "sAbsolutePathPrepared: [%1]" )
				.arg( sAbsolutePathPrepared ) );
	ASSERT_PATH( sAbsolutePath, sAbsolutePathPrepared );
	CPPUNIT_ASSERT( sFileName != sAbsolutePathPrepared );

	const auto sRelativePathPrepared = Filesystem::prepareSamplePath(
		sRelativePath, "" );
	___INFOLOG( QString( "sRelativePathPrepared: [%1]" )
				.arg( sRelativePathPrepared ) );
	ASSERT_PATH( sRelativePath, sRelativePathPrepared );
	CPPUNIT_ASSERT( sFileName != sRelativePathPrepared );

	___INFOLOG( "passed" );
}

void FilesystemTest::testDrumkitPathConversion()
{
	___INFOLOG( "" );

	// These routines must not care whether there are files and folders backing
	// the provided paths. So, we use non-existing ones right away during test.
	const QString sDrumkitDir( "/non/existing/path/to/folder" );
	const QString sDrumkitPath( "/non/existing/path/to/folder/drumkit.xml" );

	ASSERT_PATH(
		Filesystem::drumkitDirFromPath( sDrumkitPath ), sDrumkitDir
	);
	ASSERT_PATH(
		Filesystem::drumkitPathFromDir( sDrumkitDir ), sDrumkitPath
	);

	___INFOLOG( "passed" );
}

void FilesystemTest::testSanitizeDrumkitPath()
{
	___INFOLOG( "" );

	// This is how we store drumkit paths since 2.0.
	const QString sPathValid( "/tmp/folder/drumkit.xml" );
	// This is how we stored them prior to 2.0.
	const QString sPathFolder( "/tmp/folder" );
	// This folder doesn't contain a drumkit.xml and is not a valid drumkit.
	const QString sPathSubfolder( "/tmp/folder/subfolder" );
	const QString sPathInvalid( "/tmp/this/path/is/probably/invalid" );

	CPPUNIT_ASSERT( Filesystem::mkdir( sPathFolder ) );
	CPPUNIT_ASSERT( Filesystem::mkdir( sPathSubfolder ) );
	QFile fileValid( sPathValid );
	CPPUNIT_ASSERT( fileValid.open( QIODevice::WriteOnly ) );

	ASSERT_PATH(
		Filesystem::sanitizeDrumkitPath( sPathValid ), sPathValid
	);
	CPPUNIT_ASSERT( Filesystem::sanitizeDrumkitPath( sPathInvalid ).isEmpty() );
	ASSERT_PATH(
		Filesystem::sanitizeDrumkitPath( sPathFolder ), sPathValid
	);
	CPPUNIT_ASSERT( Filesystem::sanitizeDrumkitPath( sPathSubfolder ).isEmpty()
	);

	___INFOLOG( "passed" );
}
