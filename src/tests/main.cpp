/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Preferences.h>
#include <core/Hydrogen.h>
#include <core/config.h>

#include <QCoreApplication>

#include "TestHelper.h"
#include "utils/AppveyorTestListener.h"
#include "utils/AppveyorRestClient.h"

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#include <signal.h>
#include <string.h>
#endif


void setupEnvironment(unsigned log_level)
{
	/* Logger */
	H2Core::Logger* logger = H2Core::Logger::bootstrap( log_level );
	/* Test helper */
	TestHelper::createInstance();
	TestHelper* test_helper = TestHelper::get_instance();
	/* Object */
	H2Core::Object::bootstrap( logger, logger->should_log( H2Core::Logger::Debug ) );
	/* Filesystem */
	H2Core::Filesystem::bootstrap( logger, test_helper->getDataDir() );
	H2Core::Filesystem::info();
	
	/* Use fake audio driver */
	H2Core::Preferences::create_instance();
	H2Core::Preferences* preferences = H2Core::Preferences::get_instance();
	preferences->m_sAudioDriver = "Fake";
	
	H2Core::Hydrogen::create_instance();
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
	QCoreApplication app(argc, argv);

	QCommandLineParser parser;
	QCommandLineOption verboseOption( QStringList() << "V" << "verbose", "Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH","Level");
	QCommandLineOption appveyorOption( QStringList() << "appveyor", "Report test progress to AppVeyor build worker" );
	parser.addHelpOption();
	parser.addOption( verboseOption );
	parser.addOption( appveyorOption );
	parser.process(app);
	QString sVerbosityString = parser.value( verboseOption );
	unsigned logLevelOpt = H2Core::Logger::None;
	if( parser.isSet(verboseOption) ){
		if( !sVerbosityString.isEmpty() )
		{
			logLevelOpt =  H2Core::Logger::parse_log_level( sVerbosityString.toLocal8Bit() );
		} else {
			logLevelOpt = H2Core::Logger::Error|H2Core::Logger::Warning;
		}
	}

	setupEnvironment(logLevelOpt);

#ifdef HAVE_EXECINFO_H
	signal(SIGSEGV, fatal_signal);
	signal(SIGILL, fatal_signal);
	signal(SIGABRT, fatal_signal);
	signal(SIGFPE, fatal_signal);
	signal(SIGBUS, fatal_signal);
#endif

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	
	std::unique_ptr<AppVeyor::BuildWorkerApiClient> appveyorApiClient;
	std::unique_ptr<AppVeyorTestListener> avtl;
	if( parser.isSet( appveyorOption )) {
		qDebug() << "Enabled AppVeyor reporting";
		appveyorApiClient.reset( new AppVeyor::BuildWorkerApiClient() );
		avtl.reset( new AppVeyorTestListener( *appveyorApiClient ));
		runner.eventManager().addListener( avtl.get() );
	}
	bool wasSuccessful = runner.run( "", false );

	return wasSuccessful ? 0 : 1;
}
