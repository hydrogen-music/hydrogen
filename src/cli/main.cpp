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
#include <hydrogen/version.h>
#include <getopt.h>

#ifdef H2CORE_HAVE_LASH
#include <hydrogen/LashClient.h>
#endif

#include <hydrogen/basics/song.h>
#include <hydrogen/midi_map.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/playlist.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/LocalFileMng.h>

#include <iostream>
#include <signal.h>

using namespace std;
using namespace H2Core;

void showInfo();
void showUsage();

#define HAS_ARG 1
static struct option long_opts[] = {
	{"driver", required_argument, NULL, 'd'},
	{"song", required_argument, NULL, 's'},
#ifdef H2CORE_HAVE_JACKSESSION
	{"jacksessionid", required_argument, NULL, 'S'},
#endif
	{"playlist", required_argument, NULL, 'p'},
	{"bits", required_argument, NULL, 'b'},
	{"rate", required_argument, NULL, 'r'},
	{"outfile", required_argument, NULL, 'o'},
	{"interpolation", required_argument, NULL, 'I'},
	{"version", 0, NULL, 'v'},
	{"verbose", optional_argument, NULL, 'V'},
	{"help", 0, NULL, 'h'},
	{"install", required_argument, NULL, 'i'},
	{"drumkit", required_argument, NULL, 'k'},
	{0, 0, 0, 0},
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
		cout << "Terminate signal caught" << endl;
		quit = true;
	}
}

void show_playlist (Hydrogen *pHydrogen, uint active )
{
	/* Display playlist members */
	if ( pHydrogen->m_PlayList.size() > 0) {
		for ( uint i = 0; i < pHydrogen->m_PlayList.size(); ++i ) {
			cout << ( i + 1 ) << "." << pHydrogen->m_PlayList[i].m_hFile.toLocal8Bit().constData();
			if ( i == active ) cout << " *";
			cout << endl;
		}
	}
	cout << endl;
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
			if (op->has_arg)
				*cp++ = ':';
			if (op->has_arg == optional_argument )
				*cp++ = ':';  // gets another one
		}

		// Deal with the options
		QString songFilename;
		QString playlistFilename;
		QString outFilename = NULL;
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
			c = getopt_long(argc, argv, opts, long_opts, NULL);
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
				rate = strtol(optarg, NULL, 10);
				break;
			case 'b':
				bits = strtol(optarg, NULL, 10);
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
			cout << get_version() << endl;
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
        Preferences::create_instance();
        Filesystem::bootstrap( logger );
		MidiMap::create_instance();

		Preferences* preferences = Preferences::get_instance();
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
		Song *pSong = NULL;
		Playlist *pPlaylist = NULL;

		// Load playlist
		if ( ! playlistFilename.isEmpty() ) {
			pPlaylist = Playlist::load ( playlistFilename );
			if ( ! pPlaylist ) {
				___ERRORLOG( "Error loading the playlist" );
				return 0;
			}

			/* Load first song */
			preferences->setLastPlaylistFilename( playlistFilename );
			pPlaylist->loadSong( 0 );
			pSong = pHydrogen->getSong();
			show_playlist ( pHydrogen, pPlaylist->getActiveSongNumber() );
		}

		// Load song - if wasn't already loaded with playlist
		if ( ! pSong ) {
			if ( !songFilename.isEmpty() ) {
				pSong = Song::load( songFilename );
			} else {
				/* Try load last song */
				bool restoreLastSong = preferences->isRestoreLastSongEnabled();
				QString filename = preferences->getLastSongFilename();
				if ( restoreLastSong && ( !filename.isEmpty() ))
					pSong = Song::load( filename );
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
			pHydrogen->startExportSong ( outFilename, rate, bits );
			cout << "Export Progress ... ";
			ExportMode = true;
		}

		// Interactive mode
		while ( ! quit ) {
			/* FIXME: Someday here will be The Real CLI ;-) */
			Event event = pQueue->pop_event();
			// if ( event.type > 0) cout << "EVENT TYPE: " << event.type << endl;

			/* Event handler */
			switch ( event.type ) {
			case EVENT_PROGRESS: /* event used only in export mode */
				if ( ! ExportMode ) break;
	
				if ( event.value < 100 ) {
					cout << "\rExport Progress ... " << event.value << "%";
				} else {
					cout << "\rExport Progress ... DONE" << endl;
					quit = true;
				}
				break;
			case EVENT_PLAYLIST_LOADSONG: /* Load new song on MIDI event */
				if( pPlaylist ){
					if ( pPlaylist->loadSong ( event.value ) ) {
						pSong = pHydrogen->getSong();
						show_playlist ( pHydrogen, pPlaylist->getActiveSongNumber() );
					}
				}
				break;
			case EVENT_NONE: /* Sleep if there is no more events */
				Sleeper::msleep ( 100 );
				break;
			}
		}

		if ( pHydrogen->getState() == STATE_PLAYING )
			pHydrogen->sequencer_stop();

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
			cerr << "\n\n\n " << nObj << " alive objects\n\n" << endl << endl;
			Object::write_objects_map_to_cerr();
		}
	}
	catch ( const H2Exception& ex ) {
		cerr << "[main] Exception: " << ex.what() << endl;
	}
	catch (...) {
		cerr << "[main] Unknown exception X-(" << endl;
	}

	return 0;
}

/* Show some information */
void showInfo()
{
	cout << "\nHydrogen " + get_version() + " [" + __DATE__ + "]  [http://www.hydrogen-music.org]" << endl;
	cout << "Copyright 2002-2008 Alessandro Cominu" << endl;

	if ( Object::count_active() ) {
		cout << "\nObject counting active" << endl;
	}

	cout << "\nHydrogen comes with ABSOLUTELY NO WARRANTY" << endl;
	cout << "This is free software, and you are welcome to redistribute it" << endl;
	cout << "under certain conditions. See the file COPYING for details\n" << endl;
}

/**
 * Show the correct usage
 */
void showUsage()
{
	cout << "Usage: hydrogen [-v] [-h] -s file" << endl;
	cout << "   -d, --driver AUDIODRIVER - Use the selected audio driver (jack, alsa, oss)" << endl;
	cout << "   -s, --song FILE - Load a song (*.h2song) at startup" << endl;
	cout << "   -p, --playlist FILE - Load a playlist (*.h2playlist) at startup" << endl;
	cout << "   -o, --outfile FILE - Output to file (export)" << endl;
	cout << "   -r, --rate RATE - Set bitrate while exporting file" << endl;
	cout << "   -b, --bits BITS - Set bits depth while exporting file" << endl;
	cout << "   -k, --kit drumkit_name - Load a drumkit at startup" << endl;
	cout << "   -i, --install FILE - install a drumkit (*.h2drumkit)" << endl;
	cout << "   -I, --interpolate INT - Interpolation" << endl;
	cout << "       (0:linear [default],1:cosine,2:third,3:cubic,4:hermite)" << endl;

#ifdef H2CORE_HAVE_JACKSESSION
	cout << "   -S, --jacksessionid ID - Start a JackSessionHandler session" << endl;
#endif

#ifdef H2CORE_HAVE_LASH
	cout << "   --lash-no-start-server - If LASH server not running, don't start" << endl
			  << "                            it (LASH 0.5.3 and later)." << endl;
	cout << "   --lash-no-autoresume - Tell LASH server not to assume I'm returning" << endl
			  << "                          from a crash." << endl;
#endif
	cout << "   -V[Level], --verbose[=Level] - Print a lot of debugging info" << endl;
	cout << "                 Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH" << endl;
	cout << "   -v, --version - Show version info" << endl;
	cout << "   -h, --help - Show this help message" << endl;
}
