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

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLibraryInfo>
#include <QStringList>
#include <QThread>
#include <core/config.h>
#include <core/Version.h>
#include <getopt.h>

#ifdef H2CORE_HAVE_LASH
#include <core/Lash/LashClient.h>
#endif

#include <core/Basics/Song.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/AudioEngineTests.h>
#include <core/Hydrogen.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include <core/Globals.h>
#include <core/EventQueue.h>
#include <core/Preferences/Preferences.h>
#include <core/H2Exception.h>
#include <core/Basics/Playlist.h>
#include <core/Sampler/Interpolation.h>
#include <core/Helpers/Filesystem.h>
#include <core/OscServer.h>

#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include "TransportTestsTimebase.h"
#include "TestHelper.h"

#include <iostream>
#include <signal.h>

using namespace H2Core;

class Sleeper : public QThread
{
public:
	static void usleep(unsigned long usecs){QThread::usleep(usecs);}
	static void msleep(unsigned long msecs){QThread::msleep(msecs);}
	static void sleep(unsigned long secs){QThread::sleep(secs);}
};

volatile bool bQuit = false;
void signal_handler ( int signum )
{
	if ( signum == SIGINT ) {
		std::cout << "Terminate signal caught" << std::endl;
		bQuit = true;
	}
}

void tearDown() {
	___INFOLOG( "Shutting down" );
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getAudioEngine()->getState() ==
			 H2Core::AudioEngine::State::Playing ) {
			pHydrogen->sequencerStop();
	}

	delete TestHelper::get_instance();

	delete pHydrogen;
	delete EventQueue::get_instance();

	delete MidiActionManager::get_instance();

	___INFOLOG( "Quitting..." );
	delete Logger::get_instance();
}

void startTestJackDriver( lo_arg **argv, int argc ) {
	___INFOLOG("");

	CoreActionController::activateLoopMode( false );
	CoreActionController::locateToTick( 0 );

	AudioEngineTests::startJackAudioDriver();

	// This binary runs indefinitely. It is either stopped by an assertion or by
	// the teardown of the parent process.
}

void runTransportTests( lo_arg **argv, int argc ) {
	___INFOLOG( "\n\n\nstart tests\n\n\n" );

	CPPUNIT_TEST_SUITE_REGISTRATION( TransportTestsTimebase );

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry =
		CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	const bool bSuccessful = runner.run( "", false );

	if ( bSuccessful ) {
		___INFOLOG( "\n\n\nDONE\n\n\n" );
	} else {
		___ERRORLOG( "\n\n\nFAILED\n\n\n" );
	}

	tearDown();

	if ( bSuccessful ) {
		exit( 0 );
	} else {
		exit( 1 );
	}
}

int main(int argc, char *argv[])
{
	try {
#ifdef WIN32
		// In case Hydrogen was started using a CLI attach its output to
		// the latter. 
		if ( AttachConsole(ATTACH_PARENT_PROCESS)) {
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
			freopen("CONIN$", "w", stdin);
		}
#endif

#ifndef H2CORE_HAVE_OSC
		std::cout << "Error: Hydrogen has to be build with OSC support in order to run this test"
				  << std::endl;
		exit( 1 );
#endif
		// Create bootstrap QApplication for command line argument parsing.
		QCoreApplication* pApp = new QCoreApplication( argc, argv );
		pApp->setApplicationVersion( QString::fromStdString( H2Core::get_version() ) );

		QCommandLineParser parser;
		parser.setApplicationDescription( H2Core::getAboutText() );

		QCommandLineOption songFileOption(
			QStringList() << "s" << "song",
			"Load a song (*.h2song) at startup", "File" );
		QCommandLineOption verboseOption(
			QStringList() << "V" << "verbose",
			"Debug level, if present, may be\n   - None\n   - Error [default]\n   - Warning\n   - Info\n   - Debug\n   - Constructors\n   - Locks", "Level" );
		QCommandLineOption logFileOption(
			QStringList() << "L" << "log-file",
			"Alternative log file path", "Path" );
		QCommandLineOption configFileOption(
			QStringList() << "config", "Use an alternate config file", "Path" );
		QCommandLineOption timebaseStateOption(
			QStringList() << "timebase-state",
			"Initial JACK timebase base (1 - controller, 0 - listener, -1 - none)",
			"int", "-1" );
#ifdef H2CORE_HAVE_OSC
		QCommandLineOption oscPortOption(
			QStringList() << "O" << "osc-port",
			"Custom port for OSC connections", "int" );
#endif

		parser.addOption( songFileOption );
#ifdef H2CORE_HAVE_OSC
		parser.addOption( oscPortOption );
#endif
		parser.addOption( timebaseStateOption );
		parser.addOption( verboseOption );
		parser.addOption( logFileOption );
		parser.addOption( configFileOption );
		parser.addHelpOption();
		parser.addVersionOption();
		// Evaluate the options
		parser.process( *pApp );

		// Deal with the options
		const QString sSongFilename = parser.value( songFileOption );
		const QString sVerbosityString = parser.value( verboseOption );
		const QString sLogFile = parser.value( logFileOption );
		const QString sConfigFilePath = parser.value( configFileOption );

		int nOscPort = -1;
#ifdef H2CORE_HAVE_OSC
		const QString sOscPort = parser.value( oscPortOption );
		if ( ! sOscPort.isEmpty() ) {
			bool bOk;
			nOscPort = parser.value( oscPortOption ).toInt( &bOk );
			if ( ! bOk ) {
				std::cerr << "Unable to parse 'osc-port' option. Please provide an integer value"
						  << std::endl;
				exit( 1 );
			}
		}
#endif
		bool bOk;
		const int nTimebaseStateOption =
			parser.value( timebaseStateOption ).toInt( &bOk );
		if ( ! bOk || ( nTimebaseStateOption != 0 && nTimebaseStateOption != 1 &&
			 nTimebaseStateOption != -1 ) ) {
			std::cerr << "Unable to parse 'timebase-state' option. Please provide an integer value between [-1,1]"
						  << std::endl;
				exit( 1 );
		}
		const auto timebaseState =
			static_cast<JackAudioDriver::Timebase>(nTimebaseStateOption);
		AudioEngineTests::m_referenceTimebase = timebaseState;

		unsigned logLevelOpt = H2Core::Logger::Error;
		if ( parser.isSet( verboseOption ) ){
			if( !sVerbosityString.isEmpty() )
			{
				logLevelOpt =  H2Core::Logger::parse_log_level( sVerbosityString.toLocal8Bit() );
			} else {
				logLevelOpt = H2Core::Logger::Error|H2Core::Logger::Warning;
			}
		}

		// Man your battle stations... this is not a drill.
		Logger* pLogger = Logger::bootstrap( logLevelOpt,
											sLogFile, true, true );
		Base::bootstrap( pLogger, pLogger->should_log( Logger::Debug ) );
		Filesystem::bootstrap( pLogger, "", sConfigFilePath, sLogFile );
		Preferences::create_instance();
		auto pPref = Preferences::get_instance();
		pPref->setOscServerEnabled( true );
		if ( nOscPort != -1 ) {
			pPref->m_nOscTemporaryPort = nOscPort;
		}
		if ( timebaseState == JackAudioDriver::Timebase::Controller ) {
			pPref->m_bJackTimebaseMode = Preferences::USE_JACK_TIMEBASE_CONTROL;
		} else {
			pPref->m_bJackTimebaseMode = Preferences::NO_JACK_TIMEBASE_CONTROL;
		}

		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		___INFOLOG( "Using data path: " + Filesystem::sys_data_path() );

		pPref->m_bUseMetronome = false;
		pPref->m_audioDriver = H2Core::Preferences::AudioDriver::Jack;
		pPref->m_nJackTransportMode =
			H2Core::Preferences::USE_JACK_TRANSPORT;
		pPref->m_nBufferSize = 1024;

		Hydrogen::create_instance();
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		std::shared_ptr<Song> pSong = nullptr;

		if ( ! sSongFilename.isEmpty() ) {
			pSong = Song::load( sSongFilename );
		}

		/* Still not loaded */
		if ( pSong == nullptr ) {
			___INFOLOG("Starting with empty song");
			pSong = Song::getEmptySong();
		}
		pHydrogen->setSong( pSong );

        if ( ! pHydrogen->hasJackAudioDriver() ) {
			___ERRORLOG( "Unable to start JACK driver" );
			bQuit = true;
		}
		AudioEngine* pAudioEngine = pHydrogen->getAudioEngine();

		EventQueue *pQueue = EventQueue::get_instance();
		// Surpress errors in case the queue is full
		pQueue->setSilent( true );
		TestHelper::createInstance();

		signal(SIGINT, signal_handler);

		auto pOscServer = OscServer::get_instance();
		pOscServer->getServerThread()->add_method(
			"/h2JackTimebase/StartTestJackDriver", "", startTestJackDriver );
		pOscServer->getServerThread()->add_method(
			"/h2JackTimebase/TransportTests", "", runTransportTests );
		pOscServer->getServerThread()->add_method(
			nullptr, nullptr, OscServer::generic_handler, nullptr );

		while ( ! bQuit ) {
			Event event = pQueue->pop_event();

			/* Event handler */
			switch ( event.type ) {
			case EVENT_NONE: /* Sleep if there is no more events */
				Sleeper::msleep ( 100 );
				break;
				
			case EVENT_QUIT: // Shutdown if indicated by a
				// corresponding OSC message.
				bQuit = true;
				break;
			default:
				break;
			}
		}
		pSong = nullptr;
		tearDown();
	}
	catch ( const H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[main] Unknown exception X-(" << std::endl;
	}

	return 0;
}
