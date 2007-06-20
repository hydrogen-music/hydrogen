/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <getopt.h>

#include "SplashScreen.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/data_path.h>
#include <hydrogen/H2Exception.h>

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

		string songFilename = "";
		bool bNoSplash = false;
		string sSelectedDriver = "";

#ifdef CONFIG_DEBUG
		Object::useVerboseLog( true );
#endif

		// Options...
		char *cp;
		struct option *op;
		char opts[NELEM(long_opts) * 2 + 1];

		// Build up the short option string
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
					Object::useVerboseLog( true );
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

		_INFOLOG( string("Using QT version ") + string( qVersion() ) );
		_INFOLOG( "Using data path: " + H2Core::DataPath::getDataPath() );

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

		QString family = ( pPref->getApplicationFontFamily() ).c_str();
		pQApp->setFont( QFont( family, pPref->getApplicationFontPointSize() ) );

		QTranslator tor( 0 );
		QString sTranslationFile = QString("hydrogen.") + QLocale::system().name();
		QString sLocale = QLocale::system().name();
		if ( sLocale != "C") {
			QString sTranslationPath = "data/i18n";
			QString total = sTranslationPath + "/" + sTranslationFile + ".qm";

			bool bTransOk = tor.load( total, "." );
			if ( bTransOk ) {
				_INFOLOG( "Using locale: " + sTranslationPath.toStdString() + "/" + sTranslationFile.toStdString() );
			}
			else {
				sTranslationPath = QString( H2Core::DataPath::getDataPath().c_str() ) + "/i18n";
				total = sTranslationPath + "/" + sTranslationFile + ".qm";
				bTransOk = tor.load( total, "." );
				if (bTransOk) {
					_INFOLOG( "Using locale: " + sTranslationPath.toStdString() + "/" + sTranslationFile.toStdString() );
				}
				else {
					_INFOLOG( "Warning: no locale found: " + sTranslationPath.toStdString() + "/" + sTranslationFile.toStdString() );
				}
			}
			if (tor.isEmpty()) {
				_INFOLOG( "Warning: error loading locale: " +  total.toStdString() );
			}
		}
		pQApp->installTranslator( &tor );

		string sStyle = pPref->getQTStyle();
		if (sStyle != "" ) {
			pQApp->setStyle( QString( sStyle.c_str() ) );
		}

		setPalette( pQApp );

		SplashScreen *pSplash = new SplashScreen();

		if (bNoSplash) {
			pSplash->hide();
		}
		else {
			pSplash->show();
		}

		MainForm *pMainForm = new MainForm( pQApp, songFilename );
		pMainForm->show();
		pSplash->finish( pMainForm );

		pQApp->exec();

		delete pSplash;
		delete pMainForm;
		delete pQApp;
		delete pPref;
		delete H2Core::EventQueue::getInstance();
		delete H2Core::AudioEngine::getInstance();

		_INFOLOG( "Quitting..." );
		cout << "\nBye..." << endl;
		delete Logger::getInstance();

		int nObj = Object::getNObjects();
		if (nObj != 0) {
			std::cerr << "\n\n\n " << toString( nObj ) << " alive objects\n\n" << std::endl << std::endl;
			Object::printObjectMap();
		}

		//	pQApp->dumpObjectTree();

	}
	catch ( const H2Core::H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.message << std::endl;
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
	cout << "\nHydrogen " + string(VERSION) + " [" + string(__DATE__) + "]  [http://www.hydrogen-music.org]" << endl;
	cout << "Copyright 2002-2007 Alessandro Cominu" << endl;
//	_INFOLOG( "Compiled modules: " + string(COMPILED_FEATURES) << endl;

	if ( Object::isUsingVerboseLog() ) {
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
