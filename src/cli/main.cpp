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

#include <iostream>
#include <signal.h>

using namespace H2Core;

void showInfo();
void showUsage();

#define HAS_ARG 1
static struct option long_opts[] = {
	{"driver", required_argument, nullptr, 'd'},
	{"song", required_argument, nullptr, 's'},
	{"playlist", required_argument, nullptr, 'p'},
	{"bits", required_argument, nullptr, 'b'},
	{"rate", required_argument, nullptr, 'r'},
	{"outfile", required_argument, nullptr, 'o'},
	{"interpolation", required_argument, nullptr, 'I'},
	{"version", 0, nullptr, 'v'},
	{"verbose", optional_argument, nullptr, 'V'},
	{"help", 0, nullptr, 'h'},
	{"install", required_argument, nullptr, 'i'},
	{"check", required_argument, nullptr, 'c'},
	{"upgrade", required_argument, nullptr, 'u'},
	{"extract", required_argument, nullptr, 'x'},
	{"target", required_argument, nullptr, 't'},
	{"drumkit", required_argument, nullptr, 'k'},
	{nullptr, 0, nullptr, 0},
};

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
	Playlist* pPlaylist = Playlist::get_instance();
	if ( pPlaylist->size() > 0) {
		for ( uint i = 0; i < pPlaylist->size(); ++i ) {
			std::cout << ( i + 1 ) << "." << pPlaylist->get( i )->filePath.toLocal8Bit().constData();
			if ( i == active ) {
				std::cout << " *";
			}
			std::cout << std::endl;
		}
	}
	
	std::cout << std::endl;
}

QString makePathAbsolute( const char* path ) {

	QString sPath = QString::fromLocal8Bit( path );
	QFileInfo fileInfo( sPath );
	if ( fileInfo.isRelative() ) {
		sPath = fileInfo.absoluteFilePath();
	}

	return sPath;
}

#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )

int main(int argc, char *argv[])
{
	int nReturnCode = 0;
	
	try {
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
		QString songFilename;
		QString playlistFilename;
		QString outFilename = nullptr;
		QString sSelectedDriver;
		bool showVersionOpt = false;
		const char* logLevelOpt = "Error";
		bool showHelpOpt = false;
		QString drumkitName;
		QString drumkitToLoad;
		QString sDrumkitToValidate;
		bool bValidateDrumkit = false;
		QString sDrumkitToUpgrade;
		bool bUpgradeDrumkit = false;
		QString sDrumkitToExtract;
		bool bExtractDrumkit = false;
		QString sTarget = "";
		short bits = 16;
		int rate = 44100;
		short interpolation = 0;
		int c;
		while ( 1 ) {
			c = getopt_long(argc, argv, opts, long_opts, nullptr);
			if ( c == -1 ) break;

			switch(c) {
			case 'd':
				sSelectedDriver = QString::fromLocal8Bit(optarg);
				break;
			case 's':
				songFilename = QString::fromLocal8Bit(optarg);
				break;
			case 'p':
				playlistFilename = QString::fromLocal8Bit(optarg);
				break;
			case 'o':
				outFilename = QString::fromLocal8Bit(optarg);
				break;
			case 'i':
				//install h2drumkit
				drumkitName = makePathAbsolute( optarg );
				break;
			case 'c':
				//validate h2drumkit
				sDrumkitToValidate = makePathAbsolute( optarg );
				bValidateDrumkit = true;
				break;
			case 'u':
				//upgrade h2drumkit
				sDrumkitToUpgrade = makePathAbsolute( optarg );
				bUpgradeDrumkit = true;
				break;
			case 'x':
				//extract h2drumkit
				sDrumkitToExtract = makePathAbsolute( optarg );
				bExtractDrumkit = true;
				break;
			case 't':
				sTarget = makePathAbsolute( optarg );
				break;
			case 'k':
				//load Drumkit
				drumkitToLoad = QString::fromLocal8Bit(optarg);
				break;
			case 'r':
				rate = strtol(optarg, nullptr, 10);
				break;
			case 'b':
				bits = strtol(optarg, nullptr, 10);
				break;
			case 'v':
				showVersionOpt = true;
				break;
			case 'V':
				logLevelOpt = (optarg) ? optarg : "Warning";
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
		Logger* logger = Logger::bootstrap( Logger::parse_log_level( logLevelOpt ) );
		Base::bootstrap( logger, logger->should_log( Logger::Debug ) );
		Filesystem::bootstrap( logger );
		MidiMap::create_instance();
		Preferences::create_instance();
		Preferences* preferences = Preferences::get_instance();
#ifdef H2CORE_HAVE_OSC
		preferences->setOscServerEnabled( true );
#endif
		// See below for Hydrogen.

		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		___INFOLOG( "Using data path: " + Filesystem::sys_data_path() );

#ifdef H2CORE_HAVE_LASH
		LashClient::create_instance("hydrogen", "Hydrogen", &argc, &argv);
		LashClient* lashClient = LashClient::get_instance();
#endif

		if ( ! drumkitName.isEmpty() ){
			Drumkit::install( drumkitName );
			exit(0);
		}

		if (sSelectedDriver == "auto") {
			preferences->m_sAudioDriver = "Auto";
		}
		else if (sSelectedDriver == "jack") {
			preferences->m_sAudioDriver = "JACK";
		}
		else if ( sSelectedDriver == "oss" ) {
			preferences->m_sAudioDriver = "OSS";
		}
		else if ( sSelectedDriver == "alsa" ) {
			preferences->m_sAudioDriver = "ALSA";
		}
		else if (sSelectedDriver == "CoreAudio") {
			preferences->m_sAudioDriver = "CoreAudio";
		}
		else if (sSelectedDriver == "PulseAudio") {
			preferences->m_sAudioDriver = "PulseAudio";
		}

#ifdef H2CORE_HAVE_LASH
		if ( preferences->useLash() && lashClient->isConnected() ) {
			lash_event_t* lash_event = lashClient->getNextEvent();
			if (lash_event && lash_event_get_type(lash_event) == LASH_Restore_File) {
				// notify client that this project was not a new one
				lashClient->setNewProject(false);

				songFilename = "";
				songFilename.append( QString::fromLocal8Bit(lash_event_get_string(lash_event)) );
				songFilename.append("/hydrogen.h2song");

				//Logger::get_instance()->log("[LASH] Restore file: " + songFilename);

				lash_event_destroy(lash_event);
			} else if (lash_event) {
				//Logger::get_instance()->log("[LASH] ERROR: Instead of restore file got event: " + lash_event_get_type(lash_event));
				lash_event_destroy(lash_event);
			}
		}
#endif
		Hydrogen::create_instance();
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		std::shared_ptr<Song> pSong = nullptr;
		Playlist *pPlaylist = nullptr;

		// Load playlist
		if ( ! playlistFilename.isEmpty() ) {
			pPlaylist = Playlist::load ( playlistFilename, preferences->isPlaylistUsingRelativeFilenames() );
			if ( ! pPlaylist ) {
				___ERRORLOG( "Error loading the playlist" );
				return 0;
			}

			/* Load first song */
			preferences->setLastPlaylistFilename( playlistFilename );
			
			QString FirstSongFilename;
			pPlaylist->getSongFilenameByNumber( 0, FirstSongFilename );
			pSong = Song::load( FirstSongFilename );
			
			if( pSong ){
				pHydrogen->setSong( pSong );
				preferences->setLastSongFilename( songFilename );
				
				pPlaylist->activateSong( 0 );
			}
			
			show_playlist( pPlaylist->getActiveSongNumber() );
		}

		// Load song - if wasn't already loaded with playlist
		if ( ! pSong ) {
			if ( !songFilename.isEmpty() ) {
				pSong = Song::load( songFilename );
			} else {
				/* Try load last song */
				bool restoreLastSong = preferences->isRestoreLastSongEnabled();
				QString filename = preferences->getLastSongFilename();
				if ( restoreLastSong && ( !filename.isEmpty() )) {
					pSong = Song::load( filename );
				}
			}

			/* Still not loaded */
			if (! pSong) {
				___INFOLOG("Starting with empty song");
				pSong = Song::getEmptySong();
			} else {
				preferences->setLastSongFilename( songFilename );
			}

			pHydrogen->setSong( pSong );
		}

		if ( ! drumkitToLoad.isEmpty() ){
			pHydrogen->getCoreActionController()->setDrumkit( drumkitToLoad, true );
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

		
		bool ExportMode = false;
		if ( ! outFilename.isEmpty() ) {
			auto pInstrumentList = pSong->getInstrumentList();
			for (auto i = 0; i < pInstrumentList->size(); i++) {
				pInstrumentList->get(i)->set_currently_exported( true );
			}
			pHydrogen->startExportSession(rate, bits);
			pHydrogen->startExportSong( outFilename );
			std::cout << "Export Progress ... ";
			ExportMode = true;
		}

		auto pCoreActionController = pHydrogen->getCoreActionController();

		if ( bValidateDrumkit ) {
			if ( ! pCoreActionController->validateDrumkit( sDrumkitToValidate ) ) {
				nReturnCode = -1;

				std::cout << "Provided drumkit [" <<
					sDrumkitToValidate.toLocal8Bit().data() << "] is INVALID!" << std::endl;
				
			} else {
				std::cout << "Provided drumkit [" <<
					sDrumkitToValidate.toLocal8Bit().data() << "] is valid" << std::endl;
			}

		} else if ( bExtractDrumkit ) {
			if ( ! pCoreActionController->extractDrumkit( sDrumkitToExtract,
														  sTarget ) ) {
				nReturnCode = -1;

				if ( sTarget.isEmpty() ) {
					std::cout << "Unable to install drumkit [" <<
						sDrumkitToExtract.toLocal8Bit().data() << "]" << std::endl;
				} else  {
					std::cout << "Unable to extract drumkit [" <<
						sDrumkitToExtract.toLocal8Bit().data() << "] to [" <<
						sTarget.toLocal8Bit().data() << "]" << std::endl;
				}
			} else {
				
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

		} else if ( bUpgradeDrumkit ) {
			if ( ! pCoreActionController->upgradeDrumkit( sDrumkitToUpgrade,
														  sTarget ) ) {
				nReturnCode = -1;

				std::cout << "Unable to upgrade provided drumkit [" <<
					sDrumkitToUpgrade.toLocal8Bit().data() << "]!" << std::endl;
				
			} else {
				std::cout << "Provided drumkit [" <<
					sDrumkitToUpgrade.toLocal8Bit().data() << "] upgraded";

				if ( ! sTarget.isEmpty() ) {
					std::cout << " into [" << 
						sTarget.toLocal8Bit().data() << "]";
				}
				std::cout << std::endl;
			}
		} else {

			// Interactive mode
			while ( ! quit ) {
				/* FIXME: Someday here will be The Real CLI ;-) */
				Event event = pQueue->pop_event();
				// if ( event.type > 0) std::cout << "EVENT TYPE: " << event.type << std::endl;

				/* Event handler */
				switch ( event.type ) {
				case EVENT_PROGRESS: /* event used only in export mode */
					if ( ! ExportMode ) break;
	
					if ( event.value < 100 ) {
						std::cout << "\rExport Progress ... " << event.value << "%";
					} else {
						pHydrogen->stopExportSession();
						std::cout << "\rExport Progress ... DONE" << std::endl;
						quit = true;
					}
					break;
				case EVENT_PLAYLIST_LOADSONG: /* Load new song on MIDI event */
					if( pPlaylist ){
						QString FirstSongFilename;
						pPlaylist->getSongFilenameByNumber( event.value, FirstSongFilename );
						pSong = Song::load( FirstSongFilename );
					
						if( pSong ) {
							pHydrogen->setSong( pSong );
							preferences->setLastSongFilename( songFilename );
						
							pPlaylist->activateSong( event.value );
						}
					}
					break;
				case EVENT_NONE: /* Sleep if there is no more events */
					Sleeper::msleep ( 100 );
					break;
				
				case EVENT_QUIT: // Shutdown if indicated by a
					// corresponding OSC message.
					quit = true;
					break;
				default:
					// EVENT_STATE, EVENT_PATTERN_CHANGED, etc are ignored
					break;
				}
			}
		}

		if ( pHydrogen->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
			pHydrogen->sequencer_stop();
		}

		pSong = nullptr;
		delete Playlist::get_instance();

		preferences->savePreferences();
		delete pHydrogen;
		delete pQueue;
		delete preferences;

		delete MidiMap::get_instance();
		delete MidiActionManager::get_instance();

		___INFOLOG( "Quitting..." );
		delete Logger::get_instance();

		if (H2Core::Base::count_active()) {
			H2Core::Base::write_objects_map_to_cerr();
		}
	}
	catch ( const H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[main] Unknown exception X-(" << std::endl;
	}

	return nReturnCode;
}

/* Show some information */
void showInfo()
{
	std::cout << "\nHydrogen " + get_version() + " [" + __DATE__ + "]  [http://www.hydrogen-music.org]" << std::endl;
	std::cout << "\nCopyright 2002-2008 Alessandro Cominu\nCopyright 2008-2022 The hydrogen development team" << std::endl;

	if ( Base::count_active() ) {
		std::cout << "\nObject counting active" << std::endl;
	}

	std::cout << "\nHydrogen comes with ABSOLUTELY NO WARRANTY" << std::endl;
	std::cout << "This is free software, and you are welcome to redistribute it" << std::endl;
	std::cout << "under certain conditions. See the file COPYING for details\n" << std::endl;
}

/**
 * Show the correct usage
 */
void showUsage()
{
	QStringList availableAudioDrivers;
#ifdef H2CORE_HAVE_JACK
	availableAudioDrivers << "jack";
#endif
#ifdef H2CORE_HAVE_ALSA
	availableAudioDrivers << "alsa";
#endif
#ifdef H2CORE_HAVE_OSS
	availableAudioDrivers << "oss";
#endif
#ifdef H2CORE_HAVE_PULSEAUDIO
	availableAudioDrivers << "pulseaudio";
#endif
#ifdef H2CORE_HAVE_PORTAUDIO
	availableAudioDrivers << "portaudio";
#endif
#ifdef H2CORE_HAVE_COREAUDIO
	availableAudioDrivers << "coreaudio";
#endif
	availableAudioDrivers << "auto";

		
	std::cout << "Usage: h2cli OPTION [ARGS]" << std::endl;
	std::cout << std::endl;
	std::cout << "The CLI of Hydrogen can be used in two different ways. Either" << std::endl;
	std::cout << "for exporting a song into an audio file or for checking and" << std::endl;
	std::cout << "an existing drumkit." << std::endl;
	std::cout << std::endl;
	std::cout << "Exporting:" << std::endl;
	std::cout << "   -d, --driver AUDIODRIVER - Use the selected audio driver" << std::endl;
	std::cout << QString( "       [%1]" ).arg( availableAudioDrivers.join( ", " ) )
		.toLocal8Bit().data() << std::endl;
	std::cout << "   -s, --song FILE - Load a song (*.h2song) at startup" << std::endl;
	std::cout << "   -p, --playlist FILE - Load a playlist (*.h2playlist) at startup" << std::endl;
	std::cout << "   -o, --outfile FILE - Output to file (export)" << std::endl;
	std::cout << "   -r, --rate RATE - Set bitrate while exporting file" << std::endl;
	std::cout << "   -b, --bits BITS - Set bits depth while exporting file" << std::endl;
	std::cout << "   -k, --kit drumkit_name - Load a drumkit at startup" << std::endl;
	std::cout << "   -I, --interpolate INT - Interpolation" << std::endl;
	std::cout << "       [0:linear (default), 1:cosine, 2:third, 3:cubic, 4:hermite]" << std::endl;

#ifdef H2CORE_HAVE_LASH
	std::cout << "   --lash-no-start-server - If LASH server not running, don't start" << std::endl
			  << "                            it (LASH 0.5.3 and later)." << std::endl;
	std::cout << "   --lash-no-autoresume - Tell LASH server not to assume I'm returning" << std::endl
			  << "                          from a crash." << std::endl;
#endif
	std::cout << std::endl;
	std::cout << "Example: h2cli -s /usr/share/hydrogen/data/demo_songs/GM_kit_demo1.h2song \\" << std::endl;
	std::cout << "               -d GMRockKit -d auto -o ./example.wav" << std::endl;

	std::cout << std::endl;
	std::cout << "Drumkit handling:" << std::endl;
	std::cout << "   -i, --install FILE - install a drumkit (*.h2drumkit)" << std::endl;
	std::cout << "   -c, --check FILE - validates a drumkit (*.h2drumkit)" << std::endl;
	std::cout << "   -u, --upgrade FILE - upgrades a drumkit. FILE can be either" << std::endl;
	std::cout << "                        an absolute path to a folder containing a" << std::endl;
	std::cout << "                        drumkit, an absolute path to a drumkit file" << std::endl;
	std::cout << "                        (drumkit.xml) itself, or an absolute path to" << std::endl;
	std::cout << "                        a compressed drumkit ( *.h2drumkit). If no" << std::endl;
	std::cout << "                        target folder was specified using the -t option" << std::endl;
	std::cout << "                        a backup of the drumkit created and the original" << std::endl;
	std::cout << "                        one is upgraded in place. If a compressed drumkit" << std::endl;
	std::cout << "                        is provided as first argument, the upgraded" << std::endl;
	std::cout << "                        drumkit will be compressed as well." << std::endl;
	std::cout << "   -x, --extract FILE - extracts the content of a drumkit (.h2drumkit)" << std::endl;
	std::cout << "                        If no target is specified using the -t option" << std::endl;
	std::cout << "                        this command behaves like --install." << std::endl;
	std::cout << "   -t, --target FOLDER - target folder the extracted (-x) or upgraded (-u)" << std::endl;
	std::cout << "                         drumkit will be stored in. The folder is created" << std::endl;
	std::cout << "                         if it not exists yet." << std::endl;
	std::cout << std::endl;
	std::cout << "Example: h2cli -c /usr/share/hydrogen/data/drumkits/GMRockKit" << std::endl;

	std::cout << std::endl;
	std::cout << "Miscellaneous:" << std::endl;
	std::cout << "   -V[Level], --verbose[=Level] - Set verbosity level" << std::endl;
	std::cout << "       [None, Error, Warning, Info, Debug, Constructor, Locks, 0xHHHH]" << std::endl;
	std::cout << "   -v, --version - Show version info" << std::endl;
	std::cout << "   -h, --help - Show this help message" << std::endl;
}
