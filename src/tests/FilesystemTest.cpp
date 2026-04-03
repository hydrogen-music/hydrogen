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

#include "FilesystemTest.h"

#include <QTest>

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
		CPPUNIT_ASSERT( sBaseUniquePath != sBasePath );

		sBasePathAgain = Filesystem::removeUniquePrefix( sBaseUniquePath );
		CPPUNIT_ASSERT( sBasePathAgain == sBasePath );
	}

	// Almost our prefix, but not exactly.
	const QString sNoPrefixPath( QDir::temp().absoluteFilePath(
									 "tmp-AEWF-test.h2song" ) );
	const QString sNoPrefixPath2( QDir::temp().absoluteFilePath(
									 "tmp-AEWFDSD4-test.h2song" ) );

	CPPUNIT_ASSERT( sNoPrefixPath ==
					Filesystem::removeUniquePrefix( sNoPrefixPath ) );
	CPPUNIT_ASSERT( sNoPrefixPath2 ==
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
	const QString& sSystemKitPath(
		QString( "%1/sampleKit" ).arg( Filesystem::systemDrumkitsDir() )
	);
	const QString& sUserKitPath(
		QString( "%1/sampleKit" ).arg( Filesystem::userDrumkitsDir() )
	);
	const auto sPathInSystemKit = QString( "%1/sample.wav" )
		.arg( sSystemKitPath );
	const auto sPathInUserKit = QString( "%1/sample.wav" )
		.arg( sUserKitPath );
	const auto sAbsolutePath( "/path/to/sample.wav" );
	const auto sRelativePath( "../../sample.wav" );
	const QString sFileName( "sample.wav" );

	const auto sPathInSystemKitPrepared = Filesystem::prepareSamplePath(
		sPathInSystemKit, sSystemKitPath );
	___INFOLOG( QString( "sPathInSystemKitPrepared: [%1]" )
				.arg( sPathInSystemKitPrepared ) );
	CPPUNIT_ASSERT( sPathInSystemKit != sPathInSystemKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInSystemKitPrepared );

	const auto sPathInUserKitPrepared = Filesystem::prepareSamplePath(
		sPathInUserKit, sUserKitPath );
	___INFOLOG( QString( "sPathInUserKitPrepared: [%1]" )
				.arg( sPathInUserKitPrepared ) );
	CPPUNIT_ASSERT( sPathInUserKit != sPathInUserKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInUserKitPrepared );

	const auto sAbsolutePathPrepared = Filesystem::prepareSamplePath(
		sAbsolutePath, "" );
	___INFOLOG( QString( "sAbsolutePathPrepared: [%1]" )
				.arg( sAbsolutePathPrepared ) );
	CPPUNIT_ASSERT( sAbsolutePath == sAbsolutePathPrepared );
	CPPUNIT_ASSERT( sFileName != sAbsolutePathPrepared );

	const auto sRelativePathPrepared = Filesystem::prepareSamplePath(
		sRelativePath, "" );
	___INFOLOG( QString( "sRelativePathPrepared: [%1]" )
				.arg( sRelativePathPrepared ) );
	CPPUNIT_ASSERT( sRelativePath == sRelativePathPrepared );
	CPPUNIT_ASSERT( sFileName != sRelativePathPrepared );

	___INFOLOG( "passed" );
}
