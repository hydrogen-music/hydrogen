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
#include <core/Preferences/Preferences.h>

#include <QTest>

using namespace H2Core;

void FilesystemTest::setUp() {
#if !defined(WIN32) 
	m_sNotExistingPath = QDir::homePath().append( "aFunnyNameYouWouldNotExpectInYourHomeFolder.h2song" );
#ifdef Q_OS_MACX
	m_sNoAccessPath = "/etc/master.passwd";
#else
	m_sNoAccessPath = "/etc/shadow";
#endif
	m_sReadOnlyPath = "/etc/profile";
	m_sFullAccessPath = Filesystem::usr_data_path().append( "test.h2song"  );
	m_sTmpPath = Filesystem::tmp_file_path( "test.h2song" );
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
	CPPUNIT_ASSERT( ! Filesystem::file_exists( m_sNotExistingPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_exists( m_sNoAccessPath, true ) );
	CPPUNIT_ASSERT( ! Filesystem::file_readable( m_sNoAccessPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_readable( m_sReadOnlyPath, true ) );
	CPPUNIT_ASSERT( ! Filesystem::file_writable( m_sReadOnlyPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_writable( m_sFullAccessPath, true ) );

	CPPUNIT_ASSERT( Filesystem::file_exists( m_sTmpPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_readable( m_sTmpPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_writable( m_sTmpPath, true ) );
	___INFOLOG( "passed" );
#endif
}

void FilesystemTest::testFilePathValidation() {
	___INFOLOG( "" );
	QStringList invalidFilenames, validFilenames;

	validFilenames << "test.h2song" << "123-te-s_t.h2drumkit"
		<< "ếИ£TestKit越.h2pattern";
	invalidFilenames << "te/s/t/.h2song" << "test@h2song" << "t\\e\\s\\t.h2song"
		<< "?!h2song" << "test*.%h2drumkit";

	for ( const auto& ssName : validFilenames ) {
		const auto ssValidated = Filesystem::validateFilePath( ssName );
		CPPUNIT_ASSERT( ssName == ssValidated );
	}
	for ( const auto& ssName : invalidFilenames ) {
		const auto ssValidated = Filesystem::validateFilePath( ssName );
		const auto ssValidatedTwice = Filesystem::validateFilePath( ssValidated );
		CPPUNIT_ASSERT( ssName != ssValidated );
		CPPUNIT_ASSERT( ssValidatedTwice == ssValidated );
	}
	___INFOLOG( "passed" );
}

void FilesystemTest::testSamplePathHandling() {
	___INFOLOG( "" );
	const auto sPathInSystemKit = QString( "%1/sampleKit/sample.wav" )
		.arg( Filesystem::sys_drumkits_dir() );
	const auto sPathInUserKit = QString( "%1/sampleKit/sample.wav" )
		.arg( Filesystem::usr_drumkits_dir() );
	const auto sAbsolutePath( "/path/to/sample.wav" );
	const auto sRelativePath( "../../sample.wav" );
	const QString sFileName( "sample.wav" );

	const auto sPathInSystemKitPrepared = Filesystem::prepare_sample_path(
		sPathInSystemKit );
	___INFOLOG( QString( "sPathInSystemKitPrepared: [%1]" )
				.arg( sPathInSystemKitPrepared ) );
	CPPUNIT_ASSERT( sPathInSystemKit != sPathInSystemKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInSystemKitPrepared );

	const auto sPathInUserKitPrepared = Filesystem::prepare_sample_path(
		sPathInUserKit );
	___INFOLOG( QString( "sPathInUserKitPrepared: [%1]" )
				.arg( sPathInUserKitPrepared ) );
	CPPUNIT_ASSERT( sPathInUserKit != sPathInUserKitPrepared );
	CPPUNIT_ASSERT( sFileName == sPathInUserKitPrepared );

	const auto sAbsolutePathPrepared = Filesystem::prepare_sample_path(
		sAbsolutePath );
	___INFOLOG( QString( "sAbsolutePathPrepared: [%1]" )
				.arg( sAbsolutePathPrepared ) );
	CPPUNIT_ASSERT( sAbsolutePath == sAbsolutePathPrepared );
	CPPUNIT_ASSERT( sFileName != sAbsolutePathPrepared );

	const auto sRelativePathPrepared = Filesystem::prepare_sample_path(
		sRelativePath );
	___INFOLOG( QString( "sRelativePathPrepared: [%1]" )
				.arg( sRelativePathPrepared ) );
	CPPUNIT_ASSERT( sRelativePath == sRelativePathPrepared );
	CPPUNIT_ASSERT( sFileName != sRelativePathPrepared );

	___INFOLOG( "passed" );
}
