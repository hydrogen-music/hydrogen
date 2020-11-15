/*
 * A headless attempt for hydrogen
 * Copyright(c) 2013 by Sebastian Moors, Pawel Piatek
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QLibraryInfo>
#include <QThread>
#include <hydrogen/config.h>
#include <core/Version.h>
#include <getopt.h>

#ifdef H2CORE_HAVE_LASH
#include <core/Lash/LashClient.h>
#endif

#include <core/Basics/Song.h>
#include <core/MidiMap.h>
#include <core/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include <core/Globals.h>
#include <core/EventQueue.h>
#include <core/Preferences.h>
#include <core/H2Exception.h>
#include <core/Basics/Playlist.h>
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
#ifdef H2CORE_HAVE_JACKSESSION
	{"jacksessionid", required_argument, nullptr, 'S'},
#endif
	{"playlist", required_argument, nullptr, 'p'},
	{"bits", required_argument, nullptr, 'b'},
	{"rate", required_argument, nullptr, 'r'},
	{"outfile", required_argument, nullptr, 'o'},
	{"interpolation", required_argument, nullptr, 'I'},
	{"version", 0, nullptr, 'v'},
	{"verbose", optional_argument, nullptr, 'V'},
	{"help", 0, nullptr, 'h'},
	{"install", required_argument, nullptr, 'i'},
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

#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )

int main(int argc, char *argv[])
{
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
		short bits = 16;
		int rate = 44100;
		short interpolation = 0;
#ifdef H2CORE_HAVE_JACKSESSION
		QString sessionId;
#endif
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
				drumkitName = QString::fromLocal8Bit(optarg);
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
#ifdef H2CORE_HAVE_JACKSESSION
			case 'S':
				sessionId = QString::fromLocal8Bit(optarg);
				break;
#endif
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
		Object::bootstrap( logger, logger->should_log( Logger::Debug ) );
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
			preferences->m_sAudioDriver = "Jack";
		}
		else if ( sSelectedDriver == "oss" ) {
			preferences->m_sAudioDriver = "Oss";
		}
		else if ( sSelectedDriver == "alsa" ) {
			preferences->m_sAudioDriver = "Alsa";
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
#ifdef H2CORE_HAVE_JACKSESSION
		if (!sessionId.isEmpty()) {
			preferences->setJackSessionUUID ( sessionId );
			/* imo, jack sessions use jack as default audio driver.
			 * hydrogen remember last used audiodriver.
			 * here we make it save that hydrogen start in a jacksession case
			 * every time with jack as audio driver
			 */
			preferences->m_sAudioDriver = "Jack";

		}
		/* the use of applicationFilePath() make it
		 * possible to use different executables.
		 * for example if you start hydrogen from a local
		 * build directory.
		 */
//		QString path = pQApp->applicationFilePath();
//		preferences->setJackSessionApplicationPath ( path );
#endif
		Hydrogen::create_instance();
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		Song *pSong = nullptr;
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
				pSong = Song::get_empty_song();
				pSong->set_filename( "" );
			}

			pHydrogen->setSong( pSong );
			preferences->setLastSongFilename( songFilename );
		}

		if ( ! drumkitToLoad.isEmpty() ){
			Drumkit* drumkitInfo = Drumkit::load_by_name( drumkitToLoad, true );
			if ( drumkitInfo ) {
				pHydrogen->loadDrumkit( drumkitInfo );
			} else {
				___ERRORLOG ( "Error loading the drumkit" );
			}
		}

		AudioEngine* AudioEngine = AudioEngine::get_instance();
		Sampler* sampler = AudioEngine->get_sampler();
		switch ( interpolation ) {
			case 1:
					sampler->setInterpolateMode( Sampler::COSINE );
					break;
			case 2:
					sampler->setInterpolateMode( Sampler::THIRD );
					break;
			case 3:
					sampler->setInterpolateMode( Sampler::CUBIC );
					break;
			case 4:
					sampler->setInterpolateMode( Sampler::HERMITE );
					break;
			case 0:
			default:
					sampler->setInterpolateMode( Sampler::LINEAR );
		}

		EventQueue *pQueue = EventQueue::get_instance();

		signal(SIGINT, signal_handler);

		
		bool ExportMode = false;
		if ( ! outFilename.isEmpty() ) {
			InstrumentList *pInstrumentList = pSong->get_instrument_list();
			for (auto i = 0; i < pInstrumentList->size(); i++) {
				pInstrumentList->get(i)->set_currently_exported( true );
			}
			pHydrogen->startExportSession(rate, bits);
			pHydrogen->startExportSong( outFilename );
			std::cout << "Export Progress ... ";
			ExportMode = true;
		}

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
			}
		}

		if ( pHydrogen->getState() == STATE_PLAYING ) {
			pHydrogen->sequencer_stop();
		}

		delete pSong;
		delete pPlaylist;

		delete pQueue;
		delete pHydrogen;
		delete preferences;
		delete AudioEngine;

		delete MidiMap::get_instance();
		delete MidiActionManager::get_instance();

		___INFOLOG( "Quitting..." );
		delete Logger::get_instance();

		int nObj = Object::objects_count();
		if (nObj != 0) {
			std::cerr << "\n\n\n " << nObj << " alive objects\n\n" << std::endl << std::endl;
			Object::write_objects_map_to_cerr();
		}
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
	std::cout << "Copyright 2002-2008 Alessandro Cominu" << std::endl;

	if ( Object::count_active() ) {
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
	std::cout << "Usage: hydrogen [-v] [-h] -s file" << std::endl;
	std::cout << "   -d, --driver AUDIODRIVER - Use the selected audio driver (jack, alsa, oss)" << std::endl;
	std::cout << "   -s, --song FILE - Load a song (*.h2song) at startup" << std::endl;
	std::cout << "   -p, --playlist FILE - Load a playlist (*.h2playlist) at startup" << std::endl;
	std::cout << "   -o, --outfile FILE - Output to file (export)" << std::endl;
	std::cout << "   -r, --rate RATE - Set bitrate while exporting file" << std::endl;
	std::cout << "   -b, --bits BITS - Set bits depth while exporting file" << std::endl;
	std::cout << "   -k, --kit drumkit_name - Load a drumkit at startup" << std::endl;
	std::cout << "   -i, --install FILE - install a drumkit (*.h2drumkit)" << std::endl;
	std::cout << "   -I, --interpolate INT - Interpolation" << std::endl;
	std::cout << "       (0:linear [default],1:cosine,2:third,3:cubic,4:hermite)" << std::endl;

#ifdef H2CORE_HAVE_JACKSESSION
	std::cout << "   -S, --jacksessionid ID - Start a JackSessionHandler session" << std::endl;
#endif

#ifdef H2CORE_HAVE_LASH
	std::cout << "   --lash-no-start-server - If LASH server not running, don't start" << std::endl
			  << "                            it (LASH 0.5.3 and later)." << std::endl;
	std::cout << "   --lash-no-autoresume - Tell LASH server not to assume I'm returning" << std::endl
			  << "                          from a crash." << std::endl;
#endif
	std::cout << "   -V[Level], --verbose[=Level] - Print a lot of debugging info" << std::endl;
	std::cout << "                 Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH" << std::endl;
	std::cout << "   -v, --version - Show version info" << std::endl;
	std::cout << "   -h, --help - Show this help message" << std::endl;
}
