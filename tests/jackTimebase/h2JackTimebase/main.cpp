/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/MidiMap.h>
#include <core/AudioEngine/AudioEngine.h>
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

void showInfo();
void showUsage();

#define HAS_ARG 1
static struct option long_opts[] = {
	{"osc-port", required_argument, nullptr, 'O'},
	{"song", required_argument, nullptr, 's'},
	{"version", 0, nullptr, 'v'},
	{"verbose", optional_argument, nullptr, 'V'},
	{"log-file", required_argument, nullptr, 'L'},
	{"help", 0, nullptr, 'h'},
	{nullptr, 0, nullptr, 0},
};

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
	delete Preferences::get_instance();

	delete MidiMap::get_instance();
	delete MidiActionManager::get_instance();

	___INFOLOG( "Quitting..." );
	delete Logger::get_instance();
}

#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )

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

		// Options...
		char *cp;
		struct option *op;
		char opts[NELEM(long_opts) * 3 + 1];

		// Build up the short option QString
		cp = opts;
		for (op = long_opts; op < &long_opts[NELEM(long_opts)]; op++) {
			*cp++ = op->val;
			if (op->has_arg) {
				*cp++ = ':';
			}
			if (op->has_arg == optional_argument ) {
				*cp++ = ':';  // gets another one
			}
		}

		// Deal with the options
		QString songFilename, sLogFile;
		int nOscPort = -1;
		bool showVersionOpt = false;
		const char* logLevelOpt = "Error";
		bool showHelpOpt = false;
		int c;
		while ( 1 ) {
			c = getopt_long(argc, argv, opts, long_opts, nullptr);
			if ( c == -1 ) break;

			switch(c) {
			case 's':
				songFilename = QString::fromLocal8Bit(optarg);
				break;
			case 'O':
				nOscPort = strtol( optarg, nullptr, 10 );
				break;
			case 'v':
				showVersionOpt = true;
				break;
			case 'V':
				logLevelOpt = (optarg) ? optarg : "Warning";
				break;
			case 'L':
				sLogFile = QString::fromLocal8Bit( optarg );
				break;
			case 'h':
			case '?':
				showHelpOpt = true;
				break;
			}
		}

		if ( showVersionOpt ) {
			std::cout << get_version() << std::endl;
			exit(0);
		}

		showInfo();
		if ( showHelpOpt ) {
			showUsage();
			exit(0);
		}

		// Man your battle stations... this is not a drill.
		Logger* logger = Logger::bootstrap( Logger::parse_log_level( logLevelOpt ),
											sLogFile, true, true );
		Base::bootstrap( logger, logger->should_log( Logger::Debug ) );
		Filesystem::bootstrap( logger );
		MidiMap::create_instance();
		Preferences::create_instance();
		Preferences* preferences = Preferences::get_instance();
		preferences->setOscServerEnabled( true );
		if ( nOscPort != -1 ) {
			preferences->m_nOscTemporaryPort = nOscPort;
		}

		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		___INFOLOG( "Using data path: " + Filesystem::sys_data_path() );

		preferences->m_audioDriver = Preferences::AudioDriver::Jack;

		Hydrogen::create_instance();
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		std::shared_ptr<Song> pSong = nullptr;

		if ( ! songFilename.isEmpty() ) {
			pSong = Song::load( songFilename );
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

		preferences->m_bUseMetronome = false;
		preferences->m_audioDriver = H2Core::Preferences::AudioDriver::Fake;
		preferences->m_nBufferSize = 1024;

		EventQueue *pQueue = EventQueue::get_instance();
		TestHelper::createInstance();

		signal(SIGINT, signal_handler);

		auto pOscServer = OscServer::get_instance();
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

/* Show some information */
void showInfo()
{
	std::cout << "\nHydrogen " + get_version() + " [" + __DATE__ + "]  [http://www.hydrogen-music.org]" << std::endl;
	std::cout << "\nCopyright 2002-2008 Alessandro Cominu\nCopyright 2008-2024 The hydrogen development team" << std::endl;
	std::cout << "\nHydrogen comes with ABSOLUTELY NO WARRANTY" << std::endl;
	std::cout << "This is free software, and you are welcome to redistribute it" << std::endl;
	std::cout << "under certain conditions. See the file COPYING for details\n" << std::endl;
}

/**
 * Show the correct usage
 */
void showUsage() {
	std::cout << "Usage: h2JackTimebase OPTION [ARGS]" << std::endl;
	std::cout << std::endl;
	std::cout << "This CLI version of Hydrogen is build to the sole purpose of" << std::endl;
	std::cout << "performing an intergration test of the JACK Timebase transport" << std::endl;
	std::cout << std::endl;
	std::cout << "   -s, --song FILE - Load a song (*.h2song) at startup" << std::endl;

	std::cout << std::endl;
	std::cout << "Example: h2JackTimebase -s /usr/share/hydrogen/data/demo_songs/GM_kit_demo1.h2song \\" << std::endl;
	std::cout << "               -d GMRockKit -d auto -o ./example.wav" << std::endl;

	std::cout << std::endl;
	std::cout << "Miscellaneous:" << std::endl;
	std::cout << "   -V[Level], --verbose[=Level] - Set verbosity level" << std::endl;
	std::cout << "       [None, Error, Warning, Info, Debug, Constructor, Locks, 0xHHHH]" << std::endl;
	std::cout << "   -v, --version - Show version info" << std::endl;
	std::cout << "   -h, --help - Show this help message" << std::endl;
}
