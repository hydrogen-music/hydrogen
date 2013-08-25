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

void showInfo();
void showUsage();

#define HAS_ARG 1
static struct option long_opts[] = {
	{"driver", required_argument, NULL, 'd'},
	{"song", required_argument, NULL, 's'},
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

volatile bool quit = false;
void signal_handler ( int signum ) {
	if ( signum == SIGINT ) {
		std::cout << "Terminate signal caught" << endl;
		quit = true;
	}
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
			case 'h':
			case '?':
				showHelpOpt = true;
				break;
			}
		}

		if( showVersionOpt ) {
			std::cout << H2Core::get_version() << std::endl;
			exit(0);
		}

		showInfo();
		if( showHelpOpt ) {
			showUsage();
			exit(0);
		}

		// Man your battle stations... this is not a drill.
		H2Core::Logger* logger = H2Core::Logger::bootstrap( H2Core::Logger::parse_log_level( logLevelOpt ) );
		H2Core::Object::bootstrap( logger, logger->should_log( H2Core::Logger::Debug ) );
		H2Core::Filesystem::bootstrap( logger );
		MidiMap::create_instance();
		H2Core::Preferences::create_instance();
		H2Core::Preferences* preferences = H2Core::Preferences::get_instance();
		// See below for H2Core::Hydrogen.

		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		___INFOLOG( "Using data path: " + H2Core::Filesystem::sys_data_path() );

#ifdef H2CORE_HAVE_LASH

		LashClient::create_instance("hydrogen", "Hydrogen", &argc, &argv);
		LashClient* lashClient = LashClient::get_instance();
#endif

		if( ! drumkitName.isEmpty() ){
			H2Core::Drumkit::install( drumkitName );
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

				//H2Core::Logger::get_instance()->log("[LASH] Restore file: " + songFilename);

				lash_event_destroy(lash_event);
			} else if (lash_event) {
				//H2Core::Logger::get_instance()->log("[LASH] ERROR: Instead of restore file got event: " + lash_event_get_type(lash_event));
				lash_event_destroy(lash_event);
			}
		}
#endif
		H2Core::Hydrogen::create_instance();
		H2Core::Hydrogen *hydrogen = H2Core::Hydrogen::get_instance();

		// Load default song
		H2Core::Song *song = NULL;
		if ( !songFilename.isEmpty() ) {
			song = H2Core::Song::load( songFilename );
		} else {
			/* Try load last song */
			bool restoreLastSong = preferences->isRestoreLastSongEnabled();
			QString filename = preferences->getLastSongFilename();
			if ( restoreLastSong && ( !filename.isEmpty() )) {
				song = H2Core::Song::load( filename );
			}
		}

		// Load song - if wasn't already loaded with playlist
		if ( ! song ) {
			if ( !songFilename.isEmpty() ) {
				song = H2Core::Song::load( songFilename );
			} else {
				/* Try load last song */
				bool restoreLastSong = preferences->isRestoreLastSongEnabled();
				QString filename = preferences->getLastSongFilename();
				if ( restoreLastSong && ( !filename.isEmpty() ))
					song = H2Core::Song::load( filename );
			}

			/* Still not loaded */
			if (! song) {
				___INFOLOG("Starting with empty song");
				song = H2Core::Song::get_empty_song();
				song->set_filename( "" );
			}

			hydrogen->setSong( song );
			preferences->setLastSongFilename( songFilename );
		}

		if ( ! drumkitToLoad.isEmpty() ){
			H2Core::Drumkit* drumkitInfo = H2Core::Drumkit::load( H2Core::Filesystem::drumkit_path_search( drumkitToLoad ), true );
			hydrogen->loadDrumkit( drumkitInfo );
		}

		H2Core::AudioEngine* AudioEngine = H2Core::AudioEngine::get_instance();
		H2Core::Sampler* sampler = AudioEngine->get_sampler();
		switch ( interpolation ) {
			case 1:
					sampler->setInterpolateMode( H2Core::Sampler::COSINE );
					break;
			case 2:
					sampler->setInterpolateMode( H2Core::Sampler::THIRD );
					break;
			case 3:
					sampler->setInterpolateMode( H2Core::Sampler::CUBIC );
					break;
			case 4:
					sampler->setInterpolateMode( H2Core::Sampler::HERMITE );
					break;
			case 0:
			default:
					sampler->setInterpolateMode( H2Core::Sampler::LINEAR );
		}

		// use the timer to do schedule instrument slaughter;
		H2Core::EventQueue *eQueue = H2Core::EventQueue::get_instance();

		if ( !outFilename.isEmpty() ) {
			// Export mode
			hydrogen->startExportSong ( outFilename, rate, bits );
			// Actuall export song
			int progress = 0;
			std::cout << "\rExporting progress ...";
			while ( progress < 100 ) {
				H2Core::Event event = eQueue->pop_event();
				if ( event.type == H2Core::EVENT_PROGRESS ) {
					progress = event.value;
					std::cout << "\rExporting progress ... " << progress << "%";
				}
			}
			std::cout << "\rExporting progress ... DONE" << std::endl;
		} else {
			signal(SIGINT, signal_handler);

			// Interactive mode
			while ( ! quit ) {
				/* Someday here will be The Real CLI ;-) */
			}
		}

		if ( hydrogen->getState() == STATE_PLAYING )
			hydrogen->sequencer_stop();

		delete song;
		delete Playlist::get_instance();

		delete eQueue;
		delete hydrogen;
		delete preferences;
		delete AudioEngine;

		delete MidiMap::get_instance();
		delete MidiActionManager::get_instance();

		___INFOLOG( "Quitting..." );
		delete H2Core::Logger::get_instance();

		int nObj = H2Core::Object::objects_count();
		if (nObj != 0) {
			std::cerr << "\n\n\n " << nObj << " alive objects\n\n" << std::endl << std::endl;
			H2Core::Object::write_objects_map_to_cerr();
		}


	}
	catch ( const H2Core::H2Exception& ex ) {
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
	cout << "\nHydrogen " + H2Core::get_version() + " [" + __DATE__ + "]  [http://www.hydrogen-music.org]" << endl;
	cout << "Copyright 2002-2008 Alessandro Cominu" << endl;

	if ( H2Core::Object::count_active() ) {
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
