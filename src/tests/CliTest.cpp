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

#include "CliTest.h"

#include <QFileInfo>

#include "TestHelper.h"
#include "assertions/File.h"

#include <core/config.h>
#include <core/Basics/Drumkit.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>

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
	const auto pDrumkitRef = H2Core::Drumkit::load(
		sRefFolder, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitRef != nullptr );
	const auto pDrumkitNoTypes = H2Core::Drumkit::load(
		sNoTypesFolder, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitNoTypes != nullptr );

	// Now, we also write the output and compare it with reference files.
	const QString sTmpRefFile =
		H2Core::Filesystem::tmp_dir() + "sample-cli.h2map";
	QStringList argsRefFile;
	argsRefFile << "--kitToDrumkitMap" << sRefFolder
		<< "-o" << sTmpRefFile;
	auto pProcessRefFile = new QProcess();
	pProcessRefFile->start( m_sCliPath, argsRefFile );
	CPPUNIT_ASSERT( pProcessRefFile->waitForFinished() );
	CPPUNIT_ASSERT( pProcessRefFile->exitCode() == 0 );

	H2TEST_ASSERT_XML_FILES_EQUAL(
		sTmpRefFile, H2TEST_FILE( "drumkit_map/sample.h2map" ) );

	H2Core::Filesystem::rm( sTmpRefFile, true );

	// And now for the empty one.

	const QString sTmpNoTypesFile =
		H2Core::Filesystem::tmp_dir() + "empty-cli.h2map";
	QStringList argsNoTypesFile;
	argsNoTypesFile << "--kitToDrumkitMap" << sNoTypesFolder
		<< "-o" << sTmpNoTypesFile;
	auto pProcessNoTypesFile = new QProcess();
	pProcessNoTypesFile->start( m_sCliPath, argsNoTypesFile );
	CPPUNIT_ASSERT( pProcessNoTypesFile->waitForFinished() );
	CPPUNIT_ASSERT( pProcessNoTypesFile->exitCode() == 0 );

	H2TEST_ASSERT_XML_FILES_EQUAL(
		sTmpNoTypesFile, H2TEST_FILE( "drumkit_map/empty.h2map" ) );

	H2Core::XMLDoc docNoTypes;
	CPPUNIT_ASSERT( docNoTypes.read( sTmpNoTypesFile ) );

	H2Core::Filesystem::rm( sTmpNoTypesFile, true );

	___INFOLOG( "passed" );
}
