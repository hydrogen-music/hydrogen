/*
 * A headless attempt for hydrogen
 * Copyright(c) 2009 by Sebastian Moors
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
#include "config.h"
#include "version.h"
#include <getopt.h>



#ifdef LASH_SUPPORT
#include <hydrogen/LashClient.h>
#endif

#include <hydrogen/Song.h>
#include <hydrogen/midiMap.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/SoundLibrary.h>
#include <hydrogen/LocalFileMng.h>

#include <iostream>
using namespace std;

void showInfo();
void showUsage();


#define HAS_ARG 1
static struct option long_opts[] = {
        {"driver", required_argument, NULL, 'd'},
        {"song", required_argument, NULL, 's'},
        {"version", 0, NULL, 'v'},
        {"nosplash", 0, NULL, 'n'},
        {"verbose", optional_argument, NULL, 'V'},
        {"help", 0, NULL, 'h'},
	{"install", required_argument, NULL, 'i'},
	{"drumkit", required_argument, NULL, 'k'},
        {0, 0, 0, 0},
};

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
                bool bNoSplash = false;
                QString sSelectedDriver;
                bool showVersionOpt = false;
                const char* logLevelOpt = "Error";
                bool showHelpOpt = false;
		QString drumkitName;
		QString drumkitToLoad;

                int c;
                for (;;) {
                        c = getopt_long(argc, argv, opts, long_opts, NULL);
                        if (c == -1)
                                break;

                        switch(c) {
                                case 'd':
                                        sSelectedDriver = QString::fromLocal8Bit(optarg);
                                        break;

                                case 's':
                                        songFilename = QString::fromLocal8Bit(optarg);
                                        break;

				case 'i':
					//install h2drumkit
					drumkitName = QString::fromLocal8Bit(optarg);
					break;

				case 'k':
					//load Drumkit
					drumkitToLoad = QString::fromLocal8Bit(optarg);
					break;

                                case 'v':
                                        showVersionOpt = true;
                                        break;

                                case 'V':
                                        if( optarg ) {
                                                logLevelOpt = optarg;
                                        } else {
                                                logLevelOpt = "Warning";
                                        }
                                        break;
                                case 'n':
                                        bNoSplash = true;
                                        break;

                                case 'h':
                                case '?':
                                        showHelpOpt = true;
                                        break;
                        }
                }

                if( showVersionOpt ) {
                        std::cout << get_version() << std::endl;
                        exit(0);
                }
                showInfo();
                if( showHelpOpt ) {
                        showUsage();
                        exit(0);
                }

                // Man your battle stations... this is not a drill.
                Logger::create_instance();
                MidiMap::create_instance();
                H2Core::Preferences::create_instance();
                Object::set_logging_level( logLevelOpt );
                // See below for H2Core::Hydrogen.


                _INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
                _INFOLOG( "Using data path: " + H2Core::DataPath::get_data_path() );

                H2Core::Preferences *pPref = H2Core::Preferences::get_instance();

#ifdef LASH_SUPPORT

                LashClient::create_instance("hydrogen", "Hydrogen", &argc, &argv);
                LashClient* lashClient = LashClient::get_instance();

#endif
		if( ! drumkitName.isEmpty() ){
		    H2Core::Drumkit::install( drumkitName );
		    exit(0);
		}

                if (sSelectedDriver == "auto") {
                        pPref->m_sAudioDriver = "Auto";
                }
                else if (sSelectedDriver == "jack") {
                        pPref->m_sAudioDriver = "Jack";
                }
                else if ( sSelectedDriver == "oss" ) {
                        pPref->m_sAudioDriver = "Oss";
                }
                else if ( sSelectedDriver == "alsa" ) {
                        pPref->m_sAudioDriver = "Alsa";
                }
		if (sSelectedDriver == "CoreAudio") {
			pPref->m_sAudioDriver = "CoreAudio";
		}




#ifdef LASH_SUPPORT
        if ( H2Core::Preferences::get_instance()->useLash() ){
                if (lashClient->isConnected())
                {
                        lash_event_t* lash_event = lashClient->getNextEvent();
                        if (lash_event && lash_event_get_type(lash_event) == LASH_Restore_File)
                        {
                                // notify client that this project was not a new one
                                lashClient->setNewProject(false);

                                songFilename = "";
                                songFilename.append( QString::fromLocal8Bit(lash_event_get_string(lash_event)) );
                                songFilename.append("/hydrogen.h2song");

//				Logger::get_instance()->log("[LASH] Restore file: " + songFilename);

                                lash_event_destroy(lash_event);
                        }
                        else if (lash_event)
                        {
//				Logger::get_instance()->log("[LASH] ERROR: Instead of restore file got event: " + lash_event_get_type(lash_event));
                                lash_event_destroy(lash_event);
                        }
                }
        }
#endif
                H2Core::Hydrogen::create_instance();



                // Load default song
                H2Core::Song *song = NULL;
                if ( !songFilename.isEmpty() ) {
                        song = H2Core::Song::load( songFilename );
                        if (song == NULL) {
                                song = H2Core::Song::get_empty_song();
                                song->set_filename( "" );
                        }
                }
                else {
                        H2Core::Preferences *pref = H2Core::Preferences::get_instance();
                        bool restoreLastSong = pref->isRestoreLastSongEnabled();
                        QString filename = pref->getLastSongFilename();
                        if ( restoreLastSong && ( !filename.isEmpty() )) {
                                song = H2Core::Song::load( filename );
                                if (song == NULL) {
                                        _INFOLOG("Starting with empty song");
                                        song = H2Core::Song::get_empty_song();
                                        song->set_filename( "" );
                                }
                        }
                        else {
                                song = H2Core::Song::get_empty_song();
                                song->set_filename( "" );
                        }
                }


                H2Core::Hydrogen::get_instance()->setSong( song );
                H2Core::Preferences::get_instance()->setLastSongFilename(  songFilename);



		if( ! drumkitToLoad.isEmpty() ){
                    H2Core::LocalFileMng* mng;
                    H2Core::Drumkit* drumkitInfo = mng->loadDrumkit( drumkitToLoad );
                    H2Core::Hydrogen::get_instance()->loadDrumkit( drumkitInfo );
		}

                while( true ){

                }


                delete pPref;
                delete H2Core::EventQueue::get_instance();
                delete H2Core::AudioEngine::get_instance();

                delete MidiMap::get_instance();
                delete ActionManager::get_instance();

                _INFOLOG( "Quitting..." );
                cout << "\nBye..." << endl;
                delete Logger::get_instance();

                int nObj = Object::get_objects_number();
                if (nObj != 0) {
                        std::cerr << "\n\n\n " << nObj << " alive objects\n\n" << std::endl << std::endl;
                        Object::print_object_map();
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



/**
 * Show some information
 */
void showInfo()
{
        cout << "\nHydrogen " + get_version() + " [" + __DATE__ + "]  [http://www.hydrogen-music.org]" << endl;
        cout << "Copyright 2002-2008 Alessandro Cominu" << endl;
//	_INFOLOG( "Compiled modules: " + QString(COMPILED_FEATURES) << endl;

        if ( Object::is_using_verbose_log() ) {
                cout << "\nVerbose log mode = active" << endl;
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
	std::cout << "   -k, --kit drumkit_name - Load a drumkit at startup" << std::endl;
	std::cout << "   -i, --install FILE - install a drumkit (*.h2drumkit)" << std::endl;
#ifdef LASH_SUPPORT
        std::cout << "   --lash-no-start-server - If LASH server not running, don't start" << endl
                  << "                            it (LASH 0.5.3 and later)." << std::endl;
        std::cout << "   --lash-no-autoresume - Tell LASH server not to assume I'm returning" << std::endl
                  << "                          from a crash." << std::endl;
#endif
        std::cout << "   -n, --nosplash - Hide splash screen" << std::endl;
        std::cout << "   -V[Level], --verbose[=Level] - Print a lot of debugging info" << std::endl;
        std::cout << "                 Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH" << std::endl;
        std::cout << "   -v, --version - Show version info" << std::endl;
        std::cout << "   -h, --help - Show this help message" << std::endl;
}
