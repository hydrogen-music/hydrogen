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
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocalServer>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include "TestHelper.h"

#include <core/config.h>
#include <core/Helpers/Filesystem.h>
#include <core/IPC/EngineTelemetryShm.h>

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

	stopPlayerGracefully( pProcess );

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

	stopPlayerGracefully( pProcess );

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

	// Closing our end of the player's stdin reports EOF, which the player
	// treats as "no further commands can arrive" and answers with a
	// graceful shutdown.
	pProcess->closeWriteChannel();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 5000 ) );
	CPPUNIT_ASSERT( pProcess->exitStatus() == QProcess::NormalExit );

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

void H2PlayerTest::testGracefulShutdownCleansIpcResources() {
	___INFOLOG( "" );

	// Qt derives the SysV key of the telemetry shared memory from a key file
	// in the temp directory. A player dying without running its shutdown
	// path (e.g. to a SIGKILL) orphans both the segment and the key file, so
	// a graceful shutdown must not leave any new key file behind.
	const QStringList beforeFiles = QDir( QStandardPaths::writableLocation(
		QStandardPaths::TempLocation ) )
		.entryList( QStringList() << "qipc_sharedmemory_*",
					QDir::Files, QDir::Name );

	QStringList args;
	args << m_sTestSongPath;

	auto pProcess = new QProcess();
	// A single merged channel keeps the incremental output polling below
	// simple.
	pProcess->setProcessChannelMode( QProcess::MergedChannels );
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	// The connection info is printed once the IPC session - and with it the
	// telemetry shared memory - is up. Waiting for it (instead of a fixed
	// sleep) guarantees the key file exists before the shutdown.
	QString sOutput;
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();
	while ( ! sOutput.contains( "Endpoint:" ) ) {
		if ( pProcess->state() != QProcess::Running ||
			 elapsedTimer.elapsed() > 10000 ) {
			CPPUNIT_FAIL( QString( "h2player did not report its IPC endpoint "
								   "within 10 s. Output so far: [%1]" )
							  .arg( sOutput )
							  .toStdString() );
		}
		pProcess->waitForReadyRead( 100 );
		sOutput += QString::fromUtf8( pProcess->readAllStandardOutput() );
	}

	stopPlayerGracefully( pProcess );

	const QStringList afterFiles = QDir( QStandardPaths::writableLocation(
		QStandardPaths::TempLocation ) )
		.entryList( QStringList() << "qipc_sharedmemory_*",
					QDir::Files, QDir::Name );

	CPPUNIT_ASSERT_MESSAGE(
		QString( "A graceful shutdown must not orphan IPC key files. "
				 "Before: [%1] After: [%2]" )
			.arg( beforeFiles.join( ", " ) )
			.arg( afterFiles.join( ", " ) )
			.toStdString(),
		beforeFiles == afterFiles );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testQuitLeavesNoAliveObjects() {
	___INFOLOG( "" );

	QStringList args;
	// Debug verbosity also switches on the object counting the assertion
	// below relies on (Base::bootstrap( pLogger, should_log( Debug ) )).
	args << "-V" << "Debug" << "-d" << "Fake" << "--interactive"
		 << m_sTestSongPath;

	auto pProcess = new QProcess();
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	// Play, stop, quit: the interactive lifecycle from the keyboard. The
	// pipe buffers the commands until the player's stdin reader is up.
	const QByteArray sCommands = "p\ns\nq\n";
	CPPUNIT_ASSERT( pProcess->write( sCommands ) == sCommands.size() );
	CPPUNIT_ASSERT( pProcess->waitForFinished( 10000 ) );
	CPPUNIT_ASSERT( pProcess->exitStatus() == QProcess::NormalExit );

	// The player prints an objects map on exit if - and only if - objects
	// are still alive at that point.
	const QString sError =
		QString::fromUtf8( pProcess->readAllStandardError() );
	CPPUNIT_ASSERT_MESSAGE(
		QString( "h2player reported alive objects at exit:\n%1" )
			.arg( sError.left( 2000 ) )
			.toStdString(),
		! sError.contains( "Objects map" ) );

	delete pProcess;
	___INFOLOG( "passed" );
}

void H2PlayerTest::testStaleTelemetryOfKilledPlayerIsReleased() {
	___INFOLOG( "" );

	QStringList args;
	args << m_sTestSongPath;

	auto pProcess = new QProcess();
	// A single merged channel keeps the incremental output polling below
	// simple.
	pProcess->setProcessChannelMode( QProcess::MergedChannels );
	pProcess->start( m_sH2PlayerPath, args );
	CPPUNIT_ASSERT( pProcess->waitForStarted( 5000 ) );

	// The connection info is printed once the IPC session - and with it the
	// telemetry shared memory - is up; its endpoint names the key the
	// player derived the telemetry block from.
	QString sOutput;
	QString sEndpoint;
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();
	const QRegularExpression reEndpoint( "Endpoint: (\\S+)" );
	while ( true ) {
		sOutput += QString::fromUtf8( pProcess->readAllStandardOutput() );
		const auto match = reEndpoint.match( sOutput );
		if ( match.hasMatch() ) {
			sEndpoint = match.captured( 1 );
			break;
		}
		if ( pProcess->state() != QProcess::Running ||
			 elapsedTimer.elapsed() > 10000 ) {
			CPPUNIT_FAIL( QString( "h2player did not report its IPC endpoint "
								   "within 10 s. Output so far: [%1]" )
							  .arg( sOutput )
							  .toStdString() );
		}
		pProcess->waitForReadyRead( 100 );
	}

	// Kill the player without letting it run its shutdown path: the
	// telemetry segment and its key file are orphaned (stale).
	pProcess->kill();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 3000 ) );

	// The killed player's local server socket file lingers as well; remove
	// it through the same API a listener uses to clear its own name.
	QLocalServer::removeServer( sEndpoint );

	// A subsequent create() under the very same key must release the stale
	// segment instead of failing with AlreadyExists.
	EngineTelemetryShm writer;
	CPPUNIT_ASSERT_MESSAGE(
		QString( "create() must release the stale telemetry segment left "
				 "behind by the killed player [%1]" )
			.arg( sEndpoint )
			.toStdString(),
		writer.create( EngineTelemetryShm::keyForEndpoint( sEndpoint ) ) );

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

	// The player logs to stdout as well as to the `-L` file. As nobody
	// consumes its stdout, the OS pipe (64 KiB) would fill up and stall the
	// player mid-write - and, thus, stall the log file - if we were just
	// sleeping here. `waitForFinished()` drains the pipes into the process'
	// internal buffers instead and returns as soon as the player exits by
	// itself (which most invocations do, e.g. on the song-load error). Only
	// players still running at the deadline are killed.
	if ( !pProcess->waitForFinished( nTimeoutMs ) ) {
		pProcess->kill();
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

void H2PlayerTest::stopPlayerGracefully( QProcess* pProcess )
{
	// SIGTERM routes the player through its regular shutdown path: the IPC
	// session is torn down and its SysV shared memory segment and key file
	// are removed. Dying to the default disposition instead would orphan
	// both (they are only cleaned up by a later attach+detach on the very
	// same key, which never recurs for per-session endpoints).
	pProcess->terminate();
	CPPUNIT_ASSERT( pProcess->waitForFinished( 3000 ) );
	CPPUNIT_ASSERT( pProcess->exitStatus() == QProcess::NormalExit );
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

	stopPlayerGracefully( pProcess );

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
