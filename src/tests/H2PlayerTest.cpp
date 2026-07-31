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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
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

// =========================================================================
// Helper methods
// =========================================================================

QString H2PlayerTest::runPlayerAndReadLog( const QStringList& args,
										  unsigned nTimeoutMs )
{
	const QString sLogFile = Filesystem::tmpDir() + "h2player_test_" +
		QString::number( QCoreApplication::applicationPid() ) + "_" +
		QString::number( reinterpret_cast<quintptr>( this ) ) + ".log";

	QStringList allArgs = args;
	allArgs << "--no-ipc" << "-L" << sLogFile;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, allArgs );
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	QThread::msleep( nTimeoutMs );

	if ( pProcess->state() != QProcess::NotRunning ) {
		pProcess->kill();
		pProcess->waitForFinished( 3000 );
	}
	else {
		pProcess->waitForFinished( 3000 );
	}

	delete pProcess;

	QString sLogContent;
	QFile logFile( sLogFile );
	if ( logFile.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		sLogContent = QString::fromUtf8( logFile.readAll() );
		logFile.close();
	}

	___DEBUGLOG( sLogFile );

	//Filesystem::rm( sLogFile );

	return sLogContent;
}

QString H2PlayerTest::prepareCustomConfig( const QString& sDestDir, int nNewPort )
{
	// Clean up any leftovers from a previous (possibly failed) run, then
	// (re)create the destination folder.
	Filesystem::rm( sDestDir, true, true );
	QDir dir;
	CPPUNIT_ASSERT( dir.mkpath( sDestDir ) );

	const QString sSourceConfig = Filesystem::systemConfigPath();
	const QString sDestConfig = sDestDir + "/hydrogen.default.conf";

	CPPUNIT_ASSERT( QFile::copy( sSourceConfig, sDestConfig ) );

	// Alter the oscServerPort value in the copied config.
	QFile configFile( sDestConfig );
	if ( !configFile.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		CPPUNIT_FAIL( "Could not open copied config for reading" );
	}
	QString sContent = QString::fromUtf8( configFile.readAll() );
	configFile.close();

	sContent.replace(
		QRegularExpression( "<oscServerPort>\\s*\\d+\\s*</oscServerPort>" ),
		QString( "<oscServerPort>%1</oscServerPort>" ).arg( nNewPort )
	);

	if ( !configFile.open( QIODevice::WriteOnly | QIODevice::Text |
						   QIODevice::Truncate ) ) {
		CPPUNIT_FAIL( "Could not open copied config for writing" );
	}
	configFile.write( sContent.toUtf8() );
	configFile.close();

	return sDestConfig;
}

// =========================================================================
// Tests
// =========================================================================

void H2PlayerTest::testLogFileOption() {
	___INFOLOG( "" );

	const QString sLogFile = Filesystem::tmpDir() + "h2player_logfile_test.log";

	QStringList args;
	args << "-V" << "Info" << "-L" << sLogFile << "--no-ipc"
		<< m_sTestSongPath;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	QThread::msleep( 2000 );

	pProcess->kill();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 3000 ) );

	delete pProcess;

	CPPUNIT_ASSERT( QFileInfo::exists( sLogFile ) );

	QString sLogContent;
	QFile logFile( sLogFile );
	CPPUNIT_ASSERT( logFile.open( QIODevice::ReadOnly | QIODevice::Text ) );
	sLogContent = QString::fromUtf8( logFile.readAll() );
	logFile.close();

	CPPUNIT_ASSERT( !sLogContent.isEmpty() );
	CPPUNIT_ASSERT( sLogContent.contains( "Using log file" ) );

	Filesystem::rm( sLogFile );

	___INFOLOG( "passed" );
}

void H2PlayerTest::testLogTimestampsOption() {
	___INFOLOG( "" );

	QStringList args;
	args << "-V" << "Info" << "-T" << "--no-ipc" << m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

	// With timestamps, log lines should contain "[hh:mm:ss.zzz]".
	QRegularExpression re( "\\[\\d{2}:\\d{2}:\\d{2}\\.\\d{3}\\]" );
	CPPUNIT_ASSERT( sLogContent.contains( re ) );

	___INFOLOG( "passed" );
}

void H2PlayerTest::testLogColorsOption() {
	___INFOLOG( "" );

	QStringList args;
	args << "-V" << "Info" << "--log-colors" << "--no-ipc"
		<< m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

	// With colors, log lines should contain ANSI escape sequences (CSI).
	CPPUNIT_ASSERT( sLogContent.contains( "\033[" ) );

	___INFOLOG( "passed" );
}

void H2PlayerTest::testNoLogColorsOption() {
	___INFOLOG( "" );

	QStringList args;
	args << "-V" << "Info" << "--no-log-colors" << "--no-ipc"
		<< m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

	// Without colors, log lines should NOT contain ANSI escape sequences.
	CPPUNIT_ASSERT( !sLogContent.contains( "\033[" ) );

	___INFOLOG( "passed" );
}

void H2PlayerTest::testVerboseOption() {
	___INFOLOG( "" );

	// With -V Info, Info-level log lines (prefix "(I) ") should be present.
	{
		QStringList args;
		args << "-V" << "Info" << "--no-ipc" << m_sTestSongPath;

		QString sLogContent = runPlayerAndReadLog( args );

		CPPUNIT_ASSERT( sLogContent.contains( "(I) " ) );
	}

	// With -V Warning, Info-level log lines should be absent.
	{
		QStringList args;
		args << "-V" << "Warning" << "--no-ipc" << m_sTestSongPath;

		QString sLogContent = runPlayerAndReadLog( args );

		CPPUNIT_ASSERT( !sLogContent.contains( "(I) " ) );
	}

	___INFOLOG( "passed" );
}

void H2PlayerTest::testConfigOption() {
	___INFOLOG( "" );

	const int nCustomPort = 9991;
	const QString sTempDir = Filesystem::tmpDir() + "h2player_config_test/";
	const QString sConfigPath = prepareCustomConfig( sTempDir, nCustomPort );

	QStringList args;
	args << "-V" << "Info" << "--config" << sConfigPath << m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

#ifdef H2CORE_HAVE_OSC
	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Osc server started. Listening on port %1" ).arg( nCustomPort )
	) );
#endif

	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Using custom user-level config file [%1]" ).arg( sConfigPath )
	) );

	Filesystem::rm( sTempDir, true );

	___INFOLOG( "passed" );
}

void H2PlayerTest::testUserDataOption() {
	___INFOLOG( "" );

	const int nCustomPort = 9992;
	const QString sTempDir = Filesystem::tmpDir() + "h2player_userdata_test/";
	const QString sConfigPath = prepareCustomConfig( sTempDir, nCustomPort );

	QStringList args;
	args << "-V" << "Info" << "--user-data" << sTempDir
		<< "--config" << sConfigPath << m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Using custom user data folder [%1]" ).arg( sTempDir )
	) );

#ifdef H2CORE_HAVE_OSC
	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Osc server started. Listening on port %1" ).arg( nCustomPort )
	) );
#endif

	Filesystem::rm( sTempDir, true );

	___INFOLOG( "passed" );
}

void H2PlayerTest::testSystemDataOption() {
	___INFOLOG( "" );

	const int nCustomPort = 9993;
	const QString sTempDir = Filesystem::tmpDir() + "h2player_sysdata_test/";
	const QString sConfigPath = prepareCustomConfig( sTempDir, nCustomPort );

	QStringList args;
	args << "-V" << "Info" << "-P" << sTempDir
		<< "--config" << sConfigPath << m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Using custom system data folder [%1]" ).arg( sTempDir )
	) );

#ifdef H2CORE_HAVE_OSC
	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Osc server started. Listening on port %1" ).arg( nCustomPort )
	) );
#endif

	Filesystem::rm( sTempDir, true );

	___INFOLOG( "passed" );
}

#ifdef H2CORE_HAVE_OSC
void H2PlayerTest::testOscPortOption() {
	___INFOLOG( "" );

	const int nCustomPort = 9994;

	QStringList args;
	args << "-V" << "Info" << "-O" << QString::number( nCustomPort )
		<< "--no-ipc" << m_sTestSongPath;

	QString sLogContent = runPlayerAndReadLog( args );

	CPPUNIT_ASSERT( sLogContent.contains(
		QString( "Osc server started. Listening on port %1" ).arg( nCustomPort )
	) );

	___INFOLOG( "passed" );
}
#endif // H2CORE_HAVE_OSC
