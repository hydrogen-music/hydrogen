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

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/HeadlessEngineLauncher.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Version.h>

using std::cout;
using std::endl;
using namespace H2Core;

#include <QCoreApplication>
#include <QThread>

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

	QCoreApplication* pApp = new QCoreApplication( argc, argv );
	pApp->setApplicationVersion( QString::fromStdString( H2Core::get_version() ) );

	QCommandLineParser parser;
	parser.setApplicationDescription(
		H2Core::getAboutText() +
		"\nHeadless version of Hydrogen. By default, it starts an IPC server that "
		"allows the Hydrogen GUI to connect and control playback remotely. The "
		"GUI can be attached manually using the `hydrogen -c <ENDPOINT>` command "
		"using the endpoint printed on startup."
		"\n\nAdditionally, it can be started in interactive mode and be "
		"keyboard-controlled mode with simple commands: play (p), stop (s), "
		"rewind (b), show frame (f), debug (d), quit (q)."
	);

	QStringList availableAudioDrivers;
	for ( const auto& ddriver :
		  H2Core::Preferences::getSupportedAudioDrivers() ) {
		availableAudioDrivers
			<< H2Core::Preferences::audioDriverToQString( ddriver );
	}
	availableAudioDrivers << H2Core::Preferences::audioDriverToQString(
		H2Core::Preferences::AudioDriver::Auto
	);

	QCommandLineOption interactiveOption(
		QStringList() << "i"
					  << "interactive",
		"Basic transport control using keyboard"
	);
	QCommandLineOption noIpcOption(
		QStringList() << "n" << "no-ipc",
		"Disable IPC server (pure headless mode)"
	);
	QCommandLineOption audioDriverOption(
		QStringList() << "d"
					  << "driver",
		QString( "Use the selected audio driver\n   - " )
			.append( availableAudioDrivers.join( "\n   - " ) )
			.append( " [default]" ),
		"Audiodriver"
	);
	QCommandLineOption playlistFileNameOption(
		QStringList() << "p" << "playlist",
		"Load a playlist (*.h2playlist) at startup", "File" );
	QCommandLineOption systemDataPathOption(
		QStringList() << "P"
					  << "data",
		"Use an alternate system data path", "Path"
	);
	QCommandLineOption userDataPathOption(
		QStringList() << "user-data", "Use an alternate user data path", "Path"
	);
	QCommandLineOption configFileOption(
		QStringList() << "config", "Use an alternate config file", "Path"
	);
	QCommandLineOption kitOption(
		QStringList() << "k"
					  << "kit",
		"Load a drumkit at startup", "DrumkitName"
	);
	QCommandLineOption verboseOption(
		QStringList() << "V"
					  << "verbose",
		"Debug level, if present, may be\n   - None\n   - Error [default]\n   "
		"- Warning\n   - Info\n   - Debug\n   - Constructors\n   - Locks",
		"Level"
	);
	QCommandLineOption logFileOption(
		QStringList() << "L"
					  << "log-file",
		"Alternative log file path", "Path"
	);
	QCommandLineOption logTimestampsOption(
		QStringList() << "T"
					  << "log-timestamps",
		"Add timestamps to all log messages"
	);
#ifdef H2CORE_HAVE_OSC
	QCommandLineOption oscPortOption(
		QStringList() << "O"
					  << "osc-port",
		"Custom port for OSC connections", "int"
	);
#endif

	parser.addPositionalArgument( "song", "Load a song (*.h2song) at startup" );
	parser.addOption( interactiveOption );
	parser.addOption( noIpcOption );
	parser.addOption( audioDriverOption );
	parser.addOption( playlistFileNameOption );
	parser.addOption( systemDataPathOption );
	parser.addOption( userDataPathOption );
	parser.addOption( configFileOption );
	parser.addOption( kitOption );
#ifdef H2CORE_HAVE_OSC
	parser.addOption( oscPortOption );
#endif
	parser.addOption( verboseOption );
	parser.addOption( logFileOption );
	parser.addOption( logTimestampsOption );
	parser.addHelpOption();
	parser.addVersionOption();
	// Evaluate the options
	parser.process( *pApp );

	// Deal with the options
	const auto positionalArgs = parser.positionalArguments();
	if ( positionalArgs.isEmpty() ) {
		std::cerr << "Error: missing song file!" << endl;
		cout << parser.helpText().toUtf8().data() << endl;
		return 1;
	}
	const QString sSongFileName = positionalArgs.first();
	const bool bInteractive = parser.isSet( interactiveOption );
	const bool bNoIpc = parser.isSet( noIpcOption );
	const QString sSysDataPath = parser.value( systemDataPathOption );
	const QString sPlaylistFileName = parser.value( playlistFileNameOption );
	const QString sUsrDataPath = parser.value( userDataPathOption );
	const QString sConfigFilePath = parser.value( configFileOption );
	const QString sSelectedDriver = parser.value( audioDriverOption );
	const QString sVerbosityString = parser.value( verboseOption );
	const QString sDrumkitNameToLoad = parser.value( kitOption );
	const QString sLogFile = parser.value( logFileOption );
	const bool bLogTimestamps = parser.isSet( logTimestampsOption );

	int nOscPort = -1;
#ifdef H2CORE_HAVE_OSC
	const QString sOscPort = parser.value( oscPortOption );
	bool bOk;
	if ( !sOscPort.isEmpty() ) {
		nOscPort = parser.value( oscPortOption ).toInt( &bOk );
		if ( !bOk ) {
			std::cerr << "Unable to parse 'osc-port' option. Please provide an "
						 "integer value"
					  << std::endl;
			exit( 1 );
		}
	}
#endif

	unsigned logLevelOpt = H2Core::Logger::Error;
	if ( parser.isSet( verboseOption ) ) {
		if ( !sVerbosityString.isEmpty() ) {
			logLevelOpt =
				H2Core::Logger::parse_log_level( sVerbosityString.toLocal8Bit()
				);
		}
		else {
			logLevelOpt = H2Core::Logger::Error | H2Core::Logger::Warning;
		}
	}

	Logger* pLogger =
		Logger::bootstrap( logLevelOpt, sLogFile, true, bLogTimestamps );
	Base::bootstrap( pLogger, pLogger->should_log( Logger::Debug ) );
	H2Core::Filesystem::bootstrap(
		pLogger, sSysDataPath, sUsrDataPath, sConfigFilePath, sLogFile
	);
	auto pPref = Preferences::create_instance();
#ifdef H2CORE_HAVE_OSC
	pPref->setOscServerEnabled( true );
#endif
	// See below for Hydrogen.

	___INFOLOG( QString( "Using QT version " ) + QString( qVersion() ) );
	___INFOLOG( "Using data path: " + Filesystem::systemDataPath() );

	cout << "Hydrogen player starting..." << endl << endl;

	if ( !sSelectedDriver.isEmpty() ) {
		pPref->m_audioDriver = Preferences::parseAudioDriver( sSelectedDriver );
	}

	Hydrogen* pHydrogen = Hydrogen::create_instance( nOscPort, pPref );

	// Create headless engine using shared infrastructure
	pHydrogen->setGUIState( H2Core::Hydrogen::GUIState::headless );

	std::shared_ptr<Song> pSong = nullptr;
	std::shared_ptr<Playlist> pPlaylist = nullptr;

	// Load playlist
	if ( !sPlaylistFileName.isEmpty() ) {
		pPlaylist =
			Playlist::load( sPlaylistFileName, pHydrogen->getPreferences() );
		if ( pPlaylist == nullptr ) {
			std::cerr << "Error loading playlist" << endl;
			___ERRORLOG( "Error loading playlist" );
			delete pHydrogen;
			delete pLogger;
			return 1;
		}

		if ( !pHydrogen->getCoreActionController()->setPlaylist( pPlaylist ) ) {
			std::cerr << "Error loading playlist" << endl;
			___ERRORLOG( QString( "Unable to set playlist loaded from [%1]" )
							 .arg( sPlaylistFileName ) );
			delete pHydrogen;
			delete pLogger;
			return 1;
		}

		/* Load first song */
		auto sSongPath = pPlaylist->getSongFileNameByNumber( 0 );
		pSong = pHydrogen->getCoreActionController()->loadSong( sSongPath );

		if ( pSong != nullptr &&
			 pHydrogen->getCoreActionController()->setSong( pSong ) ) {
			pHydrogen->getCoreActionController()->activatePlaylistSong( 0 );
		}
	}

	// Load song - if wasn't already loaded with playlist
	if ( pSong == nullptr ) {
		if ( !sSongFileName.isEmpty() ) {
			pSong = pHydrogen->getCoreActionController()->loadSong(
				sSongFileName, ""
			);
		}
		else {
			/* Try load last song */
			const QString sSongPath = pPref->getLastSongPath();
			if ( !sSongPath.isEmpty() ) {
				pSong = pHydrogen->getCoreActionController()->loadSong(
					sSongPath, ""
				);
			}
		}

		/* Still not loaded */
		if ( pSong == nullptr ) {
			std::cerr << "Error loading song" << endl;
			delete pHydrogen;
			delete pLogger;
			return 1;
		}
		else {
			pHydrogen->getCoreActionController()->setSong( pSong );
		}
	}

	if ( !sDrumkitNameToLoad.isEmpty() ) {
		auto pDB = pHydrogen->getSoundLibraryDatabase();
		auto pDrumkit = pDB->getDrumkit( pDB->findArtifact(
			Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::User,
			sDrumkitNameToLoad, true
		) );
		if ( pDrumkit != nullptr ) {
			pHydrogen->getCoreActionController()->setDrumkit( pDrumkit );
		}
		else {
			std::cerr << "Error loading drumkit" << endl;
			___ERRORLOG( QString( "Unable to retrieve drumkit called [%1]" )
							 .arg( sDrumkitNameToLoad ) );
			delete pHydrogen;
			delete pLogger;
			return 1;
		}
	}

	// Start IPC server by default (unless --no-ipc)
	std::unique_ptr<H2Core::EngineSession> pEngineSession;

	if ( !bNoIpc ) {
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
