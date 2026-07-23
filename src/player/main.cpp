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

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <memory>
#include <atomic>

#include <core/Object.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/EventQueue.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Helpers/Filesystem.h>
#include <core/IPC/HeadlessEngineLauncher.h>
#include <core/IPC/EngineSession.h>
#include <core/Basics/Song.h>

using std::cout;
using std::endl;

#include <QCoreApplication>
#include <QThread>

void usage()
{
	cout << "Usage: h2player [OPTIONS] song.h2song" << endl;
	cout << "Options:" << endl;
	cout << "  -i, --interactive  Keyboard-interactive mode (legacy)" << endl;
	cout << "  -n, --no-ipc       Disable IPC server (pure headless mode)" << endl;
	cout << "  -h, --help         Show this help message" << endl;
	cout << endl;
	cout << "By default, h2player starts an IPC server and prints connection info." << endl;
	cout << "Connect the GUI with: hydrogen -c <endpoint>" << endl;
	exit(1);
}

void runHeadlessMode( H2Core::Hydrogen* pHydrogen )
{
	cout << "Headless mode: running event loop..." << endl;
	cout << "Press Ctrl+C to exit." << endl;

	// Run Qt event loop to handle IPC events
	QCoreApplication* pApp = QCoreApplication::instance();
	while ( true ) {
		pApp->processEvents();
		QThread::msleep( 50 );
	}
}

void runInteractiveMode( H2Core::Hydrogen* pHydrogen )
{
	cout << "Interactive mode. Commands:" << endl;
	cout << "  b - rewind to beginning" << endl;
	cout << "  p - play" << endl;
	cout << "  s - stop" << endl;
	cout << "  f - show current frame" << endl;
	cout << "  d - debug (show object count)" << endl;
	cout << "  q - quit" << endl;

	// Set stdin to non-blocking mode for keyboard input + event loop
	// Note: This is a simplified implementation. For production use,
	// platform-specific non-blocking I/O should be implemented.

	char c;
	while ( true ) {
		// Process Qt events for IPC communication
		QCoreApplication::processEvents();

		// Check for keyboard input (non-blocking check)
		// For now, we'll use a simple approach that works on most platforms
		fd_set fds;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		FD_ZERO( &fds );
		FD_SET( STDIN_FILENO, &fds );

		int retval = select( STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv );
		if ( retval > 0 && FD_ISSET( STDIN_FILENO, &fds ) ) {
			c = getchar();
			switch ( c ) {
				case 'q':
					cout << "Shutting down..." << endl;
					pHydrogen->sequencerStop();
					return;
				case 'p':
					pHydrogen->sequencerPlay();
					break;
				case 's':
					pHydrogen->sequencerStop();
					break;
				case 'b':
					pHydrogen->getCoreActionController()->locateToColumn( 0 );
					break;
				case 'f':
					cout << "Frame = "
					     << pHydrogen->getAudioEngine()->getPlayhead()->getFrame()
					     << endl;
					break;
				case 'd':
					cout << "DEBUG" << endl;
					H2Core::Base::write_objects_map_to_cerr();
					int nObj = H2Core::Base::objects_count();
					cout << endl << endl << nObj << " alive objects" << endl << endl;
					break;
			}
		}

		QThread::msleep( 50 );
	}
}

void cleanup( H2Core::Hydrogen* pHydrogen )
{
	if ( pHydrogen != nullptr ) {
		pHydrogen->sequencerStop();

		auto pPref = pHydrogen->getPreferences();
		if ( pPref != nullptr ) {
			pPref->save();
		}

		// Hydrogen owns its Preferences and EventQueue and frees them in
		// ~Hydrogen (ADR 0015).
		delete pHydrogen;
	}

	delete H2Core::Logger::get_instance();

	cout << endl << endl << H2Core::Base::objects_count() << " alive objects" << endl << endl;
	H2Core::Base::write_objects_map_to_cerr();
}

int main( int argc, char** argv )
{
#ifdef WIN32
	// In case Hydrogen was started using a CLI attach its output to
	// the latter.
	if ( AttachConsole( ATTACH_PARENT_PROCESS ) ) {
		freopen( "CONOUT$", "w", stdout );
		freopen( "CONOUT$", "w", stderr );
		freopen( "CONIN$", "w", stdin );
	}
#endif

	bool bInteractive = false;
	bool bNoIpc = false;

	static struct option long_options[] = {
		{ "interactive", no_argument, 0, 'i' },
		{ "no-ipc", no_argument, 0, 'n' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	int opt;
	while ( ( opt = getopt_long( argc, argv, "inh", long_options, nullptr ) ) != -1 ) {
		switch ( opt ) {
			case 'i':
				bInteractive = true;
				break;
			case 'n':
				bNoIpc = true;
				break;
			case 'h':
				usage();
				break;
			default:
				usage();
		}
	}

	if ( optind >= argc ) {
		cout << "Error: No song file specified" << endl;
		usage();
	}

	const QString sFileName = argv[optind];

	unsigned logLevelOpt = H2Core::Logger::Error;
	H2Core::Logger::create_instance();
	H2Core::Logger::set_bit_mask( logLevelOpt );
	H2Core::Logger* logger = H2Core::Logger::get_instance();
	H2Core::Base::bootstrap( logger, logger->should_log( H2Core::Logger::Debug ) );

	QCoreApplication a( argc, argv );

	H2Core::Filesystem::bootstrap( logger );

	cout << "Hydrogen player starting..." << endl << endl;

	// Create headless engine using shared infrastructure
	auto pHydrogen = H2Core::HeadlessEngineLauncher::createHeadlessEngine();

	// Load song
	auto pSong = H2Core::Song::load( sFileName, false, pHydrogen );
	if ( pSong == nullptr ) {
		cout << "Error loading song!" << endl;
		cleanup( pHydrogen );
		exit( 2 );
	}
	pHydrogen->setSong( pSong );

	// Start IPC server by default (unless --no-ipc)
	std::unique_ptr<H2Core::EngineSession> pEngineSession;
	const bool bEnableIpc = !bNoIpc;

	if ( bEnableIpc ) {
		const QString sEndpoint = H2Core::HeadlessEngineLauncher::makeEndpoint();
		pEngineSession = H2Core::EngineSession::start( pHydrogen, sEndpoint );

		if ( pEngineSession != nullptr ) {
			// Print connection info for user
			cout << H2Core::HeadlessEngineLauncher::formatConnectionInfo( sEndpoint ).toStdString()
			     << endl;
		} else {
			cout << "Warning: Failed to start IPC server" << endl;
		}
	}

	if ( bInteractive ) {
		// Keyboard-interactive mode (with or without IPC)
		runInteractiveMode( pHydrogen );
	} else {
		// Headless mode (with or without IPC)
		runHeadlessMode( pHydrogen );
	}

	// Cleanup
	cleanup( pHydrogen );

	return 0;
}