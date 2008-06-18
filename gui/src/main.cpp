/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <QtGui>
#include "config.h"
#include <getopt.h>


#ifdef LASH_SUPPORT
#include <hydrogen/LashClient.h>
#endif

#include "SplashScreen.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include <hydrogen/midiMap.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>

#include <iostream>
using namespace std;

void showInfo();
void showUsage();


#define HAS_ARG 1
static struct option long_opts[] = {
        {"driver", HAS_ARG, NULL, 'd'},
        {"song", HAS_ARG, NULL, 's'},
        {"version", 0, NULL, 'v'},
        {"nosplash", 0, NULL, 'n'},
	{"verbose", 0, NULL, 'V'},
	{"help", 0, NULL, 'h'},
        {0, 0, 0, 0},
};

#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )


//
// Set the palette used in the application
//
void setPalette( QApplication *pQApp )
{
	// create the default palette
	QPalette defaultPalette;

	// A general background color.
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );

	// A general foreground color.
	defaultPalette.setColor( QPalette::Foreground, QColor( 255, 255, 255 ) );

	// Used as the background color for text entry widgets; usually white or another light color.
	defaultPalette.setColor( QPalette::Base, QColor( 88, 94, 112 ) );

	// Used as the alternate background color in views with alternating row colors
	defaultPalette.setColor( QPalette::AlternateBase, QColor( 138, 144, 162 ) );

	// The foreground color used with Base. This is usually the same as the Foreground, in which case it must provide good contrast with Background and Base.
	defaultPalette.setColor( QPalette::Text, QColor( 255, 255, 255 ) );

	// The general button background color. This background can be different from Background as some styles require a different background color for buttons.
	defaultPalette.setColor( QPalette::Button, QColor( 88, 94, 112 ) );

	// A foreground color used with the Button color.
	defaultPalette.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );


	// Lighter than Button color.
	defaultPalette.setColor( QPalette::Light, QColor( 138, 144, 162 ) );

	// Between Button and Light.
	defaultPalette.setColor( QPalette::Midlight, QColor( 128, 134, 152 ) );

	// Darker than Button.
	defaultPalette.setColor( QPalette::Dark, QColor( 58, 62, 72 ) );

	// Between Button and Dark.
	defaultPalette.setColor( QPalette::Mid, QColor( 81, 86, 99 ) );

	// A very dark color. By default, the shadow color is Qt::black.
	defaultPalette.setColor( QPalette::Shadow, QColor( 255, 255, 255 ) );


	// A color to indicate a selected item or the current item.
	defaultPalette.setColor( QPalette::Highlight, QColor( 116, 124, 149 ) );

	// A text color that contrasts with Highlight.
	defaultPalette.setColor( QPalette::HighlightedText, QColor( 255, 255, 255 ) );

	pQApp->setPalette( defaultPalette );
}




int main(int argc, char *argv[])
{
	try {

		QString songFilename = "";
		bool bNoSplash = false;
		QString sSelectedDriver = "";

#ifdef CONFIG_DEBUG
		Object::use_verbose_log( true );
#endif

#ifdef LASH_SUPPORT
		LashClient *lashClient = new LashClient("hydrogen", "Hydrogen", &argc, &argv);
#endif

		// Options...
		char *cp;
		struct option *op;
		char opts[NELEM(long_opts) * 2 + 1];

		// Build up the short option QString
		cp = opts;
		for (op = long_opts; op < &long_opts[NELEM(long_opts)]; op++) {
			*cp++ = op->val;
			if (op->has_arg)
				*cp++ = ':';
		}

		QApplication* pQApp = new QApplication(argc, argv);

		// Deal with the options
		int c;
		for (;;) {
			c = getopt_long(argc, argv, opts, long_opts, NULL);
			if (c == -1)
				break;

			switch(c) {
				case 'd':
					sSelectedDriver = optarg;
					break;

				case 's':
					songFilename = optarg;
					break;

				case 'v':
					std::cout << VERSION << std::endl;
					exit(0);
					break;

				case 'V':
					Object::use_verbose_log( true );
					break;

				case 'n':
					bNoSplash = true;
					break;

				case 'h':
				case '?':
					showInfo();
					showUsage();
					exit(0);
					break;
			}
		}

		showInfo();

		_INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		_INFOLOG( "Using data path: " + H2Core::DataPath::get_data_path() );

		H2Core::Preferences *pPref = H2Core::Preferences::getInstance();
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

		QString family = pPref->getApplicationFontFamily();
		pQApp->setFont( QFont( family, pPref->getApplicationFontPointSize() ) );

		QTranslator tor( 0 );
		QString sTranslationFile = QString("hydrogen.") + QLocale::system().name();
		QString sLocale = QLocale::system().name();
		if ( sLocale != "C") {
			QString sTranslationPath = "data/i18n";
			QString total = sTranslationPath + "/" + sTranslationFile + ".qm";

			bool bTransOk = tor.load( total, "." );
			if ( bTransOk ) {
				_INFOLOG( QString( "Using locale: %1/%2" ).arg( sTranslationPath ).arg( sTranslationFile ) );
			}
			else {
				sTranslationPath = H2Core::DataPath::get_data_path() + "/i18n";
				total = sTranslationPath + "/" + sTranslationFile + ".qm";
				bTransOk = tor.load( total, "." );
				if (bTransOk) {
					_INFOLOG( "Using locale: " + sTranslationPath + "/" + sTranslationFile );
				}
				else {
					_INFOLOG( "Warning: no locale found: " + sTranslationPath + "/" + sTranslationFile );
				}
			}
			if (tor.isEmpty()) {
				_INFOLOG( "Warning: error loading locale: " +  total );
			}
		}
		pQApp->installTranslator( &tor );

		QString sStyle = pPref->getQTStyle();
		if (sStyle != "" ) {
			pQApp->setStyle( sStyle );
		}

		setPalette( pQApp );

		SplashScreen *pSplash = new SplashScreen();

		if (bNoSplash) {
			pSplash->hide();
		}
		else {
			pSplash->show();
		}

#ifdef LASH_SUPPORT
		
		if (lashClient->isConnected())
		{
			lash_event_t* lash_event = lashClient->getNextEvent();
			if (lash_event && lash_event_get_type(lash_event) == LASH_Restore_File)
			{
				// notify client that this project was not a new one
				lashClient->setNewProject(false);
				
				songFilename = "";
				songFilename.append(lash_event_get_string(lash_event));
				songFilename.append("/hydrogen.h2song"); 
				
//				Logger::getInstance()->log("[LASH] Restore file: " + songFilename);
	
				lash_event_destroy(lash_event);
			}
			else if (lash_event)
			{
//				Logger::getInstance()->log("[LASH] ERROR: Instead of restore file got event: " + lash_event_get_type(lash_event));
				lash_event_destroy(lash_event);
			}
		}
		
#endif

		
		MainForm *pMainForm = new MainForm( pQApp, songFilename );
		pMainForm->show();
		pSplash->finish( pMainForm );

		pQApp->exec();

		delete pSplash;
		delete pMainForm;
		delete pQApp;
		delete pPref;
		delete H2Core::EventQueue::get_instance();
		delete H2Core::AudioEngine::get_instance();

		delete midiMap::getInstance();
		delete actionManager::getInstance();

		_INFOLOG( "Quitting..." );
		cout << "\nBye..." << endl;
		delete Logger::get_instance();

		int nObj = Object::get_objects_number();
		if (nObj != 0) {
			std::cerr << "\n\n\n " << to_string( nObj ).toStdString() << " alive objects\n\n" << std::endl << std::endl;
			Object::print_object_map();
		}

		//	pQApp->dumpObjectTree();

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
	cout << "\nHydrogen " + VERSION + " [" + __DATE__ + "]  [http://www.hydrogen-music.org]" << endl;
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
	std::cout << "   -n, --nosplash - Hide splash screen" << std::endl;
	std::cout << "   -V, --verbose - Print a lot of debugging info" << std::endl;
	std::cout << "   -v, --version - Show version info" << std::endl;
	std::cout << "   -h, --help - Show this help message" << std::endl;
}
