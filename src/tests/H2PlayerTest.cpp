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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "H2PlayerTest.h"

#include <QFileInfo>
#include <QProcess>
#include <QTimer>

#include "TestHelper.h"

#include <core/config.h>
#include <core/Helpers/Filesystem.h>

using namespace H2Core;

void H2PlayerTest::setUp() {
	___INFOLOG( "" );
	m_sH2PlayerPath = QString( "%1/%2" ).arg( CMAKE_BINARY_DIR )
		.arg( "/src/player/h2player" );

	// Use a test song file that should exist
	m_sTestSongPath = H2TEST_FILE( "song/sample-path-portability.h2song" );

	CPPUNIT_ASSERT( QFileInfo::exists( m_sH2PlayerPath ) );
	CPPUNIT_ASSERT( QFileInfo::exists( m_sTestSongPath ) );
	___INFOLOG( "passed" );
}

void H2PlayerTest::testHelpOption() {
	___INFOLOG( "" );

	QStringList args;
	args << "--help";

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForFinished( 5000 ) );

	QString sOutput = QString::fromUtf8( pProcess->readAllStandardOutput() );
	QString sError = QString::fromUtf8( pProcess->readAllStandardError() );

	// Check that help message is displayed
	CPPUNIT_ASSERT( sOutput.contains( "Usage:" ) ||
	                sError.contains( "Usage:" ) );
	CPPUNIT_ASSERT( sOutput.contains( "--interactive" ) ||
	                sError.contains( "--interactive" ) );
	CPPUNIT_ASSERT( sOutput.contains( "--no-ipc" ) ||
	                sError.contains( "--no-ipc" ) );
	CPPUNIT_ASSERT( sOutput.contains( "--help" ) ||
	                sError.contains( "--help" ) );

	// Help should exit with success
	CPPUNIT_ASSERT( pProcess->exitCode() == 0 || pProcess->exitCode() == 1 );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testDefaultIpcMode() {
	___INFOLOG( "" );

	QStringList args;
	args << m_sTestSongPath;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );

	// Wait a bit for the process to start and print output
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	// Give it time to start IPC server and print connection info
	QThread::msleep( 2000 );

	// Kill the process more forcefully
	pProcess->kill();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 3000 ) );

	QString sOutput = QString::fromUtf8( pProcess->readAllStandardOutput() );
	QString sError = QString::fromUtf8( pProcess->readAllStandardError() );

	// Check that IPC server started and connection info was printed
	QString sCombinedOutput = sOutput + sError;
	CPPUNIT_ASSERT( sCombinedOutput.contains( "IPC Server Started" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "Endpoint:" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "hydrogen-headless-" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "hydrogen -c" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "--connect-via-ipc" ) );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testNoIpcMode() {
	___INFOLOG( "" );

	QStringList args;
	args << "--no-ipc" << m_sTestSongPath;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );

	// Wait a bit for the process to start
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	// Give it time to start without IPC server
	QThread::msleep( 2000 );

	// Kill the process
	pProcess->kill();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 3000 ) );

	QString sOutput = QString::fromUtf8( pProcess->readAllStandardOutput() );
	QString sError = QString::fromUtf8( pProcess->readAllStandardError() );

	QString sCombinedOutput = sOutput + sError;

	// Check that IPC server was NOT started
	CPPUNIT_ASSERT( !sCombinedOutput.contains( "IPC Server Started" ) );
	CPPUNIT_ASSERT( !sCombinedOutput.contains( "Endpoint:" ) );
	CPPUNIT_ASSERT( !sCombinedOutput.contains( "hydrogen -c" ) );

	// But it should still run in headless mode
	CPPUNIT_ASSERT( sCombinedOutput.contains( "Headless mode" ) ||
	                sCombinedOutput.contains( "Hydrogen player starting" ) );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testInteractiveMode() {
	___INFOLOG( "" );

	QStringList args;
	args << "--interactive" << m_sTestSongPath;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );

	// Wait a bit for the process to start
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	// Give it time to start in interactive mode
	QThread::msleep( 2000 );

	// Kill the process
	pProcess->kill();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 3000 ) );

	QString sOutput = QString::fromUtf8( pProcess->readAllStandardOutput() );
	QString sError = QString::fromUtf8( pProcess->readAllStandardError() );

	QString sCombinedOutput = sOutput + sError;

	// Check that interactive mode was started
	CPPUNIT_ASSERT( sCombinedOutput.contains( "Interactive mode" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "Commands:" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "b - rewind" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "p - play" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "s - stop" ) );
	CPPUNIT_ASSERT( sCombinedOutput.contains( "q - quit" ) );

	// IPC server should still be available in interactive mode
	CPPUNIT_ASSERT( sCombinedOutput.contains( "IPC Server Started" ) );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testMissingSongFile() {
	___INFOLOG( "" );

	QStringList args;
	args << "nonexistent_song.h2song";

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForFinished( 5000 ) );

	QString sOutput = QString::fromUtf8( pProcess->readAllStandardOutput() );
	QString sError = QString::fromUtf8( pProcess->readAllStandardError() );

	QString sCombinedOutput = sOutput + sError;

	// Check that error message is displayed
	CPPUNIT_ASSERT( sCombinedOutput.contains( "Error" ) ||
	                sCombinedOutput.contains( "error" ) );

	// Should exit with error code
	CPPUNIT_ASSERT( pProcess->exitCode() != 0 );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testInvalidSongFile() {
	___INFOLOG( "" );

	// Create a temporary invalid file
	QString sInvalidFile = Filesystem::tmpDir() + "invalid_test.h2song";
	QFile file( sInvalidFile );
	if ( file.open( QIODevice::WriteOnly ) ) {
		file.write( "This is not a valid song file" );
		file.close();
	}

	QStringList args;
	args << sInvalidFile;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForFinished( 5000 ) );

	QString sOutput = QString::fromUtf8( pProcess->readAllStandardOutput() );
	QString sError = QString::fromUtf8( pProcess->readAllStandardError() );

	QString sCombinedOutput = sOutput + sError;

	// Check that error message is displayed
	CPPUNIT_ASSERT( sCombinedOutput.contains( "Error" ) ||
	                sCombinedOutput.contains( "error" ) ||
	                sCombinedOutput.contains( "loading" ) );

	// Should exit with error code
	CPPUNIT_ASSERT( pProcess->exitCode() != 0 );

	// Clean up
	Filesystem::rm( sInvalidFile );

	delete pProcess;
	___INFOLOG( "passed" );
}
