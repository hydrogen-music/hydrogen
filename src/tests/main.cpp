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

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>

#include <core/Helpers/Filesystem.h>
#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include <core/config.h>

#include <QCoreApplication>
#include <QTemporaryDir>

#include "registeredTests.h"
#include "TestHelper.h"
#include "utils/AppveyorTestListener.h"
#include "utils/AppveyorRestClient.h"
#include "AudioBenchmark.h"
#include <chrono>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#include <signal.h>
#include <string.h>
#endif

void setupEnvironment(unsigned log_level, const QString& sLogFilePath,
					  const QString& sUserDataFolder )
{
	/* Logger */
	H2Core::Logger* pLogger = nullptr;
	if ( ! sLogFilePath.isEmpty() ) {
		pLogger = H2Core::Logger::bootstrap( log_level, sLogFilePath, false, true );
	}
	else {
		pLogger = H2Core::Logger::bootstrap( log_level, "", true, true );
	}
	/* Test helper */
	auto pTestHelper = TestHelper::get_instance();
	/* Base */
	H2Core::Base::bootstrap( pLogger, true );
	/* Filesystem */
	H2Core::Filesystem::bootstrap(
		pLogger, pTestHelper->getDataDir(), sUserDataFolder,
		pTestHelper->getTestDataDir().append( "/preferences/current.conf" ),
		sLogFilePath );
	H2Core::Filesystem::info();
	
	/* Use fake audio driver */
	H2Core::Preferences::create_instance();
	auto pPref = H2Core::Preferences::get_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::LoopBack;
	pPref->m_nBufferSize = 1024;
	pPref->setUseRelativeFileNamesForPlaylists( true );

	// Use a dedicated OSC port to not cause conflicts with (JACK) integration
	// tests running in a different shell.
	H2Core::Hydrogen::create_instance( 4563 );
	H2Core::Hydrogen::get_instance()->setGUIState(
		H2Core::Hydrogen::GUIState::headless );
	// Prevent the EventQueue from flooding the log since we will push
	// more events in a short period of time than it is able to handle.
	EventQueue::get_instance()->setSilent( true );
}

#ifdef HAVE_EXECINFO_H
void fatal_signal( int sig )
{
	void *frames[ BUFSIZ ];
	signal( sig, SIG_DFL );

	fprintf( stderr, "Caught fatal signal (%s)\n", strsignal( sig ) );
	int nFrames = backtrace( frames, BUFSIZ );
	backtrace_symbols_fd( frames, nFrames, fileno( stderr ) );

	exit(1);
}
#endif

int main( int argc, char **argv)
{
	auto start = std::chrono::high_resolution_clock::now();
	
	QCoreApplication app(argc, argv);

	QCommandLineParser parser;
	QCommandLineOption verboseOption( QStringList() << "V" << "verbose", "Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH","Level");
	QCommandLineOption appveyorOption( QStringList() << "appveyor", "Report test progress to AppVeyor build" );
	QCommandLineOption childOption( QStringList() << "child", "Child process (no backtrace)" );
	QCommandLineOption benchmarkOption( QStringList() << "b" << "benchmark", "Run audio system benchmark" );
	QCommandLineOption outputFileOption( QStringList() << "o" << "output-file", "If specified the output of the logger will not be directed to stdout but instead stored in a file (either plain file name or with relative of absolute path)",
										 "Output File", "");
	parser.addHelpOption();
	parser.addOption( verboseOption );
	parser.addOption( appveyorOption );
	parser.addOption( childOption );
	parser.addOption( benchmarkOption );
	parser.addOption( outputFileOption );
	parser.process(app);
	QString sVerbosityString = parser.value( verboseOption );

	const QString sLogFile = parser.value( outputFileOption );
	QString sLogFilePath = "";
	if ( parser.isSet( outputFileOption ) ) {
		if ( ! sLogFile.contains( QDir::separator() ) ) {
			// A plain filename was provided. It will be placed in the
			// current working directory.
			sLogFilePath = QDir::currentPath() + QDir::separator() + sLogFile;
		} else {
			QFileInfo fi( sLogFile );
			sLogFilePath = fi.absoluteFilePath();
		}
	}
	else {
		sLogFilePath = QString( "%1%2test.log" )
			.arg( QDir::currentPath() ).arg( QDir::separator() );
	}
	
	auto logLevelOpt = H2Core::Logger::Error | H2Core::Logger::Warning |
		H2Core::Logger::Info | H2Core::Logger::Debug ;
	if ( ! sVerbosityString.isEmpty() ) {
		logLevelOpt =  H2Core::Logger::parse_log_level(
			sVerbosityString.toLocal8Bit() );
	}

	// Transient user-level data to ensure no data of the system the unit tests
	// are run on does leak into the test setup.
	QTemporaryDir userDataDir( H2Core::Filesystem::tmp_dir() + "-user-data-XXXXX" );
	userDataDir.setAutoRemove( false );

	qDebug() << "Using transient data dir: [" << userDataDir.path() << "]";

	TestHelper::createInstance( parser.isSet( appveyorOption ) );
	setupEnvironment( logLevelOpt, sLogFilePath, userDataDir.path() );

#ifdef HAVE_EXECINFO_H
	if ( ! parser.isSet( childOption ) ) {
		signal( SIGSEGV, fatal_signal );
		signal( SIGILL, fatal_signal );
		signal( SIGABRT, fatal_signal );
		signal( SIGFPE, fatal_signal );
		signal( SIGBUS, fatal_signal );
	}
#endif

	// Enable the audio benchmark
	if ( parser.isSet( benchmarkOption ) ) {
		AudioBenchmark::enable();
	}
	
	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	
	std::unique_ptr<AppVeyor::BuildWorkerApiClient> appveyorApiClient;
	std::unique_ptr<AppVeyorTestListener> avtl;
	if( parser.isSet( appveyorOption )) {
		appveyorApiClient.reset( new AppVeyor::BuildWorkerApiClient() );
		avtl.reset( new AppVeyorTestListener( *appveyorApiClient ));
		runner.eventManager().addListener( avtl.get() );
	}
	bool wasSuccessful = runner.run( "", false );
	auto stop = std::chrono::high_resolution_clock::now();

	// Ensure the log is written properly
	auto pLogger = H2Core::Logger::get_instance();
	pLogger->flush();

	H2Core::Filesystem::rm( userDataDir.path(), true, true );

	auto durationSeconds = std::chrono::duration_cast<std::chrono::seconds>( stop - start );
	auto durationMilliSeconds =
		std::chrono::duration_cast<std::chrono::milliseconds>( stop - start ) -
		std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::seconds( durationSeconds.count() ) );

	qDebug().noquote() << QString( "Tests required %1.%2s to complete\n\n" )
		.arg( durationSeconds.count() ).arg( durationMilliSeconds.count() );

	return wasSuccessful ? 0 : 1;
}
