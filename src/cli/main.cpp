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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLibraryInfo>
#include <QStringList>
#include <QThread>
#include <getopt.h>
#include <iostream>
#include <signal.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/config.h>
#include <core/EventQueue.h>
#include <core/Globals.h>
#include <core/H2Exception.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Interpolation.h>
#include <core/Version.h>

using namespace H2Core;

class Sleeper : public QThread
{
public:
	static void usleep(unsigned long usecs){QThread::usleep(usecs);}
	static void msleep(unsigned long msecs){QThread::msleep(msecs);}
	static void sleep(unsigned long secs){QThread::sleep(secs);}
};

volatile bool quit = false;
void signal_handler ( int signum )
{
	if ( signum == SIGINT ) {
		std::cout << "Terminate signal caught" << std::endl;
		quit = true;
	}
}

void show_playlist (uint active )
{
	/* Display playlist members */
	auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	if ( pPlaylist->size() > 0) {
		for ( uint i = 0; i < pPlaylist->size(); ++i ) {
			std::cout << ( i + 1 ) << "." <<
				pPlaylist->get( i )->getSongPath().toLocal8Bit().constData();
			if ( i == active ) {
				std::cout << " *";
			}
			std::cout << std::endl;
		}
	}
	
	std::cout << std::endl;
}

bool convertKitToDrumkitMap( const QString& sKit,
							 const QString& sOutFileName ) {
	bool bCompressed, bLegacyFormatEncountered;
	QString sKitFolder, sTmpFolder;
	const auto pKit = CoreActionController::retrieveDrumkit(
		sKit, &bCompressed, &sKitFolder, &sTmpFolder, &bLegacyFormatEncountered );

	if ( pKit == nullptr ) {
		___ERRORLOG( QString( "Unable to load kit from [%1]" ).arg( sKit ) );
		return false;
	}

	const auto pDrumkitMap = pKit->toDrumkitMap();
	if ( pDrumkitMap == nullptr ) {
		___ERRORLOG( QString( "Unable to create drumkit map from kit [%1]" )
					 .arg( sKit ) );
		return false;
	}

	auto xmlDoc = pDrumkitMap->toXml();
	if ( sOutFileName.isEmpty() ) {
		// Write content to stdout
		std::cout << xmlDoc.toString().toStdString();
	}
	else {
		// Write content to file
		if ( ! Filesystem::dir_readable(
				 QFileInfo( sOutFileName ).dir().absolutePath(), false ) ) {
			___ERRORLOG( QString( "Unable to write output file [%1]. Dir not writable" )
						 .arg( sOutFileName ) );
			return false;
		}
		xmlDoc.write( sOutFileName );
	}

	return true;
}

int main(int argc, char *argv[])
{
	// Indicates whether or not h2cli handled the requested action and is done
	// and whether it was successful.
	int nReturnCode = -1;

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

		// Create bootstrap QApplication for command line argument parsing.
		QCoreApplication* pApp = new QCoreApplication( argc, argv );
		pApp->setApplicationVersion( QString::fromStdString( H2Core::get_version() ) );

		QCommandLineParser parser;
		parser.setApplicationDescription(
			H2Core::getAboutText() +
			"\n\nThe CLI of Hydrogen can be used in two different ways. Either for exporting a song into an audio file\n\n" +
			"  h2cli -s /usr/share/hydrogen/data/demo_songs/GM_kit_demo1.h2song \\\n        -d GMRockKit -d auto -o ./example.wav\n\n" +
			"or for checking, extracting, installing, or upgrading an existing drumkit\n\n" +
			"  h2cli -c /usr/share/hydrogen/data/drumkits/GMRockKit" );

		QStringList availableAudioDrivers;
		for ( const auto& ddriver : H2Core::Preferences::getSupportedAudioDrivers() ) {
			availableAudioDrivers << H2Core::Preferences::audioDriverToQString( ddriver );
		}
		availableAudioDrivers << H2Core::Preferences::audioDriverToQString(
			H2Core::Preferences::AudioDriver::Auto );

		QCommandLineOption audioDriverOption(
			QStringList() << "d" << "driver",
			QString( "Use the selected audio driver\n   - " )
			.append( availableAudioDrivers.join( "\n   - " ) )
			.append( " [default]" ), "Audiodriver");
		QCommandLineOption songFileOption(
			QStringList() << "s" << "song",
			"Load a song (*.h2song) at startup", "File" );
		QCommandLineOption installDrumkitOption(
			QStringList() << "i" << "install",
			"Install a drumkit (*.h2drumkit)", "File");
		QCommandLineOption checkDrumkitOption(
			QStringList() << "c" << "check",
			"Validates a drumkit (*.h2drumkit) against current drumkit format",
			"File" );
		QCommandLineOption legacyCheckDrumkitOption(
			QStringList() << "l" << "legacy-check",
			"Validates a drumkit (*.h2drumkit) against current as well as legacy drumkit formats",
			"File" );
		QCommandLineOption upgradeDrumkitOption(
			QStringList() << "u" << "upgrade",
			"Upgrades a drumkit. The provided file can be either an absolute path to a folder containing a drumkit, an absolute path to a drumkit file (drumkit.xml) itself, or an absolute path to a compressed drumkit ( *.h2drumkit). If no target folder was specified using the -t option, a backup of the drumkit is created and the original one is upgraded in place. If a compressed drumkit is provided as first argument, the upgraded drumkit will be compressed as well.",
			"File" );
		QCommandLineOption extractDrumkitOption(
			QStringList() << "x" << "extract",
			"Extracts the content of a drumkit (.h2drumkit). If no target is specified using the -t option, this command behaves like --install.",
			"File" );
		QCommandLineOption targetOption(
			QStringList() << "t" << "target",
			"Target folder the extracted (-x) or upgraded (-u) drumkit will be stored in. The folder is created if it does not exists yet.",
			"Path" );
		QCommandLineOption bitsOption(
			QStringList() << "b" << "bits", "Set bits depth while exporting file",
			"int", "16" );
		QCommandLineOption rateOption(
			QStringList() << "r" << "rate", "Set bitrate while exporting file",
			"int", "44100" );
		QCommandLineOption compressionLevelOption(
			QStringList() << "compression-level", "Trade-off between max. quality (0.0) and max. compression (1.0).",
			"double", "0.0" );
		QCommandLineOption outputFileOption(
			QStringList() << "o" << "outfile", "Output to file (export)", "File" );
		QCommandLineOption interpolationOption(
			QStringList() << "I" << "interpolation",
			"Interpolation:\n   - 0 (linear) [default]\n   - 1 (cosine)\n   - 2 (third)\n   - 3 (cubic)\n   - 4 (hermite)",
			"int", "0" );
		QCommandLineOption playlistFileNameOption(
			QStringList() << "p" << "playlist",
			"Load a playlist (*.h2playlist) at startup", "File" );
		QCommandLineOption systemDataPathOption(
			QStringList() << "P" << "data",
			"Use an alternate system data path", "Path" );
		QCommandLineOption userDataPathOption(
			QStringList() << "user-data", "Use an alternate user data path",
			"Path" );
		QCommandLineOption configFileOption(
			QStringList() << "config", "Use an alternate config file", "Path" );
		QCommandLineOption kitToDrumkitMapOption(
			QStringList() << "kitToDrumkitMap",
			"Create a .h2map from the provided drumkit. To write the output into a file, use it in conjunction with -o.",
			"Path" );
		QCommandLineOption kitOption(
			QStringList() << "k" << "kit",
			"Load a drumkit at startup", "DrumkitName" );
		QCommandLineOption verboseOption(
			QStringList() << "V" << "verbose",
			"Debug level, if present, may be\n   - None\n   - Error [default]\n   - Warning\n   - Info\n   - Debug\n   - Constructors\n   - Locks", "Level" );
		QCommandLineOption logFileOption(
			QStringList() << "L" << "log-file",
			"Alternative log file path", "Path" );
		QCommandLineOption logTimestampsOption(
			QStringList() << "T" << "log-timestamps",
			"Add timestamps to all log messages" );
#ifdef H2CORE_HAVE_OSC
		QCommandLineOption oscPortOption(
			QStringList() << "O" << "osc-port",
			"Custom port for OSC connections", "int" );
#endif

		parser.addOption( audioDriverOption );
		parser.addOption( songFileOption );
		parser.addOption( playlistFileNameOption );
		parser.addOption( outputFileOption );
		parser.addOption( systemDataPathOption );
		parser.addOption( userDataPathOption );
		parser.addOption( configFileOption );
		parser.addOption( rateOption );
		parser.addOption( bitsOption );
		parser.addOption( compressionLevelOption );
		parser.addOption( kitOption );
		parser.addOption( kitToDrumkitMapOption );
		parser.addOption( interpolationOption );
		parser.addOption( installDrumkitOption );
		parser.addOption( checkDrumkitOption );
		parser.addOption( legacyCheckDrumkitOption );
		parser.addOption( upgradeDrumkitOption );
		parser.addOption( extractDrumkitOption );
		parser.addOption( targetOption );
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
		const QString sSongFileName = parser.value( songFileOption );
		const QString sPlaylistFileName = parser.value( playlistFileNameOption );
		const QString sSysDataPath = parser.value( systemDataPathOption );
		const QString sUsrDataPath = parser.value( userDataPathOption );
		const QString sConfigFilePath = parser.value( configFileOption );
		const QString sOutFileName = parser.value( outputFileOption );
		const QString sSelectedDriver = parser.value( audioDriverOption );
		const QString sVerbosityString = parser.value( verboseOption );
		const QString sInstallDrumkitName = parser.value( installDrumkitOption );
		const QString sDrumkitToLoad = parser.value( kitOption );
		const QString sKitToDrumkitMap = parser.value( kitToDrumkitMapOption );
		const QString sDrumkitToValidate = parser.value( checkDrumkitOption );
		const QString sDrumkitToLegacyValidate = parser.value( legacyCheckDrumkitOption );
		const QString sLogFile = parser.value( logFileOption );
		const QString sDrumkitToUpgrade = parser.value( upgradeDrumkitOption );
		const QString sDrumkitToExtract = parser.value( extractDrumkitOption );
		const bool bLogTimestamps = parser.isSet( logTimestampsOption );
		const QString sTarget = parser.value( targetOption );

		bool bOk;
		const short bits = parser.value( bitsOption ).toShort( &bOk );
		if ( ! bOk ) {
			std::cerr << "Unable to parse 'bits' option. Please provide an integer value (short)"
				<< std::endl;
			exit( 1 );
		}
		const int nRate = parser.value( rateOption ).toInt( &bOk );
		if ( ! bOk ) {
			std::cerr << "Unable to parse 'rate' option. Please provide an integer value"
				<< std::endl;
			exit( 1 );
		}
		const double fCompressionLevel =
			parser.value( compressionLevelOption ).toDouble( &bOk );
		if ( ! bOk ) {
			std::cerr << "Unable to parse 'compressionLevel' option. Please provide an double precision value between 0.0 and 1.0"
				<< std::endl;
			exit( 1 );
		}
		const short interpolation =
			parser.value( interpolationOption ).toShort( &bOk );
		if ( ! bOk ) {
			std::cerr << "Unable to parse 'interpolation' option. Please provide an integer value (short)"
				<< std::endl;
			exit( 1 );
		}

		int nOscPort = -1;
#ifdef H2CORE_HAVE_OSC
		const QString sOscPort = parser.value( oscPortOption );
		if ( ! sOscPort.isEmpty() ) {
			nOscPort = parser.value( oscPortOption ).toInt( &bOk );
			if ( ! bOk ) {
				std::cerr << "Unable to parse 'osc-port' option. Please provide an integer value"
						  << std::endl;
				exit( 1 );
			}
		}
#endif

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
											sLogFile, true, bLogTimestamps );
		Base::bootstrap( pLogger, pLogger->should_log( Logger::Debug ) );
		H2Core::Filesystem::bootstrap(
			pLogger, sSysDataPath, sUsrDataPath, sConfigFilePath, sLogFile );
		Preferences::create_instance();
		auto pPref = Preferences::get_instance();
#ifdef H2CORE_HAVE_OSC
		pPref->setOscServerEnabled( true );
#endif
		// See below for Hydrogen.

		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		___INFOLOG( "Using data path: " + Filesystem::sys_data_path() );

		if ( ! sInstallDrumkitName.isEmpty() ){
			if ( ! Drumkit::install( sInstallDrumkitName ) ) {
				___ERRORLOG( QString( "Unable to install drumkit [%1]" )
							 .arg( sInstallDrumkitName ) );
				exit( 1  );
			}

			exit( 0 );
		}

		if ( ! sSelectedDriver.isEmpty() ) {
			pPref->m_audioDriver =
				Preferences::parseAudioDriver( sSelectedDriver );
		}

		Hydrogen::create_instance( nOscPort );
		Hydrogen *pHydrogen = Hydrogen::get_instance();

		// Tell the core that we are done initializing the most basic parts.
		pHydrogen->setGUIState( H2Core::Hydrogen::GUIState::headless );

		std::shared_ptr<Song> pSong = nullptr;
		std::shared_ptr<Playlist> pPlaylist = nullptr;

		// Load playlist
		if ( ! sPlaylistFileName.isEmpty() ) {
			pPlaylist = Playlist::load( sPlaylistFileName );
			if ( pPlaylist == nullptr ) {
				___ERRORLOG( "Error loading the playlist" );
				return 1;
			}

			if ( ! CoreActionController::setPlaylist( pPlaylist ) ) {
				___ERRORLOG( QString( "Unable to set playlist loaded from [%1]" )
							 .arg( sPlaylistFileName ) );
				return 1;
			}

			/* Load first song */
			auto sSongPath = pPlaylist->getSongFileNameByNumber( 0 );
			pSong = CoreActionController::loadSong( sSongPath );

			if ( pSong != nullptr && CoreActionController::setSong( pSong ) ) {
				CoreActionController::activatePlaylistSong( 0 );
			}

			show_playlist( pPlaylist->getActiveSongNumber() );
		}

		// Load song - if wasn't already loaded with playlist
		if ( pSong == nullptr ) {
			if ( ! sSongFileName.isEmpty() ) {
				pSong = CoreActionController::loadSong( sSongFileName, "" );
			}
			else {
				/* Try load last song */
				const QString sSongPath = pPref->getLastSongFileName();
				if ( ! sSongPath.isEmpty() ) {
					pSong = CoreActionController::loadSong( sSongPath, "" );
				}
			}

			/* Still not loaded */
			if ( pSong == nullptr ) {
				___INFOLOG( "Starting with empty song" );
				pSong = Song::getEmptySong();

				// We avoid setting LastSongFileName in the PPref
				pHydrogen->setSong( pSong );
			}
			else {
				CoreActionController::setSong( pSong );
			}
		}

		if ( ! sDrumkitToLoad.isEmpty() ){
			CoreActionController::setDrumkit( sDrumkitToLoad );
		}

		AudioEngine* pAudioEngine = pHydrogen->getAudioEngine();
		Sampler* pSampler = pAudioEngine->getSampler();
		switch ( interpolation ) {
			case 1:
					pSampler->setInterpolateMode( Interpolation::InterpolateMode::Cosine );
					break;
			case 2:
					pSampler->setInterpolateMode( Interpolation::InterpolateMode::Third );
					break;
			case 3:
					pSampler->setInterpolateMode( Interpolation::InterpolateMode::Cubic );
					break;
			case 4:
					pSampler->setInterpolateMode( Interpolation::InterpolateMode::Hermite );
					break;
			case 0:
			default:
					pSampler->setInterpolateMode( Interpolation::InterpolateMode::Linear );
		}

		EventQueue *pQueue = EventQueue::get_instance();

		signal(SIGINT, signal_handler);

		// Hydrogen is up and running. Let's handle the requested user action.
		//
		// Note that handling of the output file option is kinda evolving
		// naturally. It is (still) possible to trigger all sorts of actions via
		// the CLI at the same time. But on the other hand we do not want to
		// introduce output file arguments for each and every action. At some
		// point the CLI has to be properly reworked. But as it seems not to be
		// in common usage only support audio export or .h2map for now.
		bool bExportMode = false;
		if ( ! sOutFileName.isEmpty() && sKitToDrumkitMap.isEmpty() ) {
			auto pInstrumentList = pSong->getDrumkit()->getInstruments();
			for (auto i = 0; i < pInstrumentList->size(); i++) {
				pInstrumentList->get(i)->setCurrentlyExported( true );
			}
			pHydrogen->startExportSession(nRate, bits, fCompressionLevel);
			pHydrogen->startExportSong( sOutFileName );
			std::cout << "Export Progress ... ";
			bExportMode = true;
		}

		if ( ! sDrumkitToValidate.isEmpty() ) {
			if ( ! H2Core::CoreActionController::validateDrumkit(
					 sDrumkitToValidate, false ) ) {
				nReturnCode = 1;

				std::cout << "Provided drumkit [" <<
					sDrumkitToValidate.toLocal8Bit().data() << "] is INVALID!" << std::endl;

			} else {
				nReturnCode = 0;
				std::cout << "Provided drumkit [" <<
					sDrumkitToValidate.toLocal8Bit().data() << "] is valid" << std::endl;
			}

		}

		if ( ! sDrumkitToLegacyValidate.isEmpty() ) {
			if ( ! H2Core::CoreActionController::validateDrumkit(
					 sDrumkitToLegacyValidate, true ) ) {
				nReturnCode = 1;

				std::cout << "Provided legacy drumkit [" <<
					sDrumkitToLegacyValidate.toLocal8Bit().data() << "] is INVALID!" << std::endl;
				
			} else {
				nReturnCode = 0;
				std::cout << "Provided legacy drumkit [" <<
					sDrumkitToLegacyValidate.toLocal8Bit().data() << "] is valid" << std::endl;
			}

		}

		if ( ! sDrumkitToExtract.isEmpty() ) {
			if ( ! H2Core::CoreActionController::extractDrumkit(
					 sDrumkitToExtract, sTarget ) ) {
				nReturnCode = 1;

				if ( sTarget.isEmpty() ) {
					std::cout << "Unable to install drumkit [" <<
						sDrumkitToExtract.toLocal8Bit().data() << "]" << std::endl;
				} else  {
					std::cout << "Unable to extract drumkit [" <<
						sDrumkitToExtract.toLocal8Bit().data() << "] to [" <<
						sTarget.toLocal8Bit().data() << "]" << std::endl;
				}
			}
			else {
				nReturnCode = 0;
				if ( sTarget.isEmpty() ) {
					std::cout << "Drumkit [" <<
						sDrumkitToExtract.toLocal8Bit().data() <<
						"] successfully installed!" << std::endl;
				} else  {
					std::cout << "Drumkit [" <<
						sDrumkitToExtract.toLocal8Bit().data() <<
						"] successfully extracted to [" <<
						sTarget.toLocal8Bit().data() << "]!" << std::endl;
				}
			}
		}

		if ( ! sDrumkitToUpgrade.isEmpty() ) {
			if ( ! H2Core::CoreActionController::upgradeDrumkit(
					 sDrumkitToUpgrade, sTarget ) ) {
				nReturnCode = 1;

				std::cout << "Unable to upgrade provided drumkit [" <<
					sDrumkitToUpgrade.toLocal8Bit().data() << "]!" << std::endl;
				
			}
			else {
				nReturnCode = 0;
				std::cout << "Provided drumkit [" <<
					sDrumkitToUpgrade.toLocal8Bit().data() << "] upgraded";

				if ( ! sTarget.isEmpty() ) {
					std::cout << " into [" << 
						sTarget.toLocal8Bit().data() << "]";
				}
				std::cout << std::endl;
			}
		}

		if ( ! sKitToDrumkitMap.isEmpty() ) {
			if ( ! convertKitToDrumkitMap( sKitToDrumkitMap, sOutFileName ) ) {
				nReturnCode = 1;
			} else {
				nReturnCode = 0;
			}
		}

		// The Preferences is provided as a shared pointer. We discard our local
		// reference in order to not prevent cleanup to the old instance in case
		// it is replaced while Hydrogen is running.
		pPref = nullptr;

		if ( nReturnCode == -1 || bExportMode ) {
			// Interactive mode - h2cli is not done yet.
			while ( ! quit ) {
				/* FIXME: Someday here will be The Real CLI ;-) */
				auto pEvent = pQueue->popEvent();
				if ( pEvent == nullptr ) {
					/* Sleep if there is no more events */
					Sleeper::msleep ( 100 );
					continue;
				}

				/* Event handler */
				switch ( pEvent->getType() ) {
				case Event::Type::Progress: /* event used only in export mode */
					if ( ! bExportMode ) {
						break;
					}
	
					if ( pEvent->getValue() < 100 ) {
						std::cout << "\rExport Progress ... " <<
							pEvent->getValue() << "%";
					}
					else {
						const auto pDriver = std::dynamic_pointer_cast<DiskWriterDriver>(
							pHydrogen->getAudioEngine()->getAudioDriver()
						);
						if ( pDriver != nullptr && pDriver->m_bWritingFailed ) {
							std::cerr << "\rExport FAILED" << std::endl;
							nReturnCode = 1;
						}
						else {
							std::cout << "\rExport Progress ... DONE" << std::endl;
						}
						pHydrogen->stopExportSession();
						quit = true;
					}
					break;

				case Event::Type::Quit: // Shutdown if indicated by a
					// corresponding OSC message.
					quit = true;
					break;
				default:
					break;
				}
			}
		}

		if ( pHydrogen->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
			pHydrogen->sequencerStop();
		}

		pSong = nullptr;

		pPref = H2Core::Preferences::get_instance();

		pPref->save();
		delete pHydrogen;
		delete pQueue;
		delete pApp;

		// There is no particular need to clean up the Preferences outselves.
		// This is just done in order for it to not appear in the objects map
		// printed below.
		pPref->replaceInstance( nullptr );
		pPref = nullptr;

		___INFOLOG( "Quitting..." );
		delete Logger::get_instance();

		if (H2Core::Base::count_active()) {
			H2Core::Base::write_objects_map_to_cerr();
		}
	}
	catch ( const H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.what() << std::endl;
		nReturnCode = 1;
	}
	catch (...) {
		std::cerr << "[main] Unknown exception X-(" << std::endl;
		nReturnCode = 1;
	}

	return std::max( nReturnCode, 0 );
}
