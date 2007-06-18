/*
 * Hydrogen * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: main.cpp,v 1.18 2005/06/23 10:38:17 comix Exp $
 *
 */

#include <qapplication.h>
#include <qfont.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qtranslator.h>

#ifndef WIN32
	#include <getopt.h>
#endif

#include "SplashScreen.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include "lib/Hydrogen.h"
#include "lib/Globals.h"
#include "lib/EventQueue.h"
#include "lib/Preferences.h"
#include "lib/DataPath.h"
#include "lib/Exception.h"

#include <iostream>
using namespace std;

void showInfo();
void showUsage();


#ifndef WIN32
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
#endif

//
// Set the palette used in the application
//
void setPalette( QApplication *pQApp )
{
	// create the default palette
	QPalette defaultPalette;

	// A general background color.
	defaultPalette.setColor( QColorGroup::Background, QColor( 58, 62, 72 ) );

	// A general foreground color.
	defaultPalette.setColor( QColorGroup::Foreground, QColor( 255, 255, 255 ) );

	// Used as the background color for text entry widgets; usually white or another light color.
	defaultPalette.setColor( QColorGroup::Base, QColor( 88, 94, 112 ) );

	// Used as the alternate background color in views with alternating row colors
//	defaultPalette.setColor( QColorGroup::AlternateBase, QColor( 138, 144, 162 ) );

	// The foreground color used with Base. This is usually the same as the Foreground, in which case it must provide good contrast with Background and Base.
	defaultPalette.setColor( QColorGroup::Text, QColor( 255, 255, 255 ) );

	// The general button background color. This background can be different from Background as some styles require a different background color for buttons.
	defaultPalette.setColor( QColorGroup::Button, QColor( 88, 94, 112 ) );

	// A foreground color used with the Button color.
	defaultPalette.setColor( QColorGroup::ButtonText, QColor( 255, 255, 255 ) );


	// Lighter than Button color.
	defaultPalette.setColor( QColorGroup::Light, QColor( 138, 144, 162 ) );

	// Between Button and Light.
	defaultPalette.setColor( QColorGroup::Midlight, QColor( 128, 134, 152 ) );

	// Darker than Button.
	defaultPalette.setColor( QColorGroup::Dark, QColor( 58, 62, 72 ) );

	// Between Button and Dark.
	defaultPalette.setColor( QColorGroup::Mid, QColor( 81, 86, 99 ) );

	// A very dark color. By default, the shadow color is Qt::black.
	defaultPalette.setColor( QColorGroup::Shadow, QColor( 255, 255, 255 ) );


	// A color to indicate a selected item or the current item.
	defaultPalette.setColor( QColorGroup::Highlight, QColor( 116, 124, 149 ) );

	// A text color that contrasts with Highlight.
	defaultPalette.setColor( QColorGroup::HighlightedText, QColor( 255, 255, 255 ) );

	pQApp->setPalette( defaultPalette );
}


int main(int argc, char *argv[]) {

	try {

		string songFilename = "";
		bool bNoSplash = false;
		string sSelectedDriver = "";

#ifdef CONFIG_DEBUG
		Object::useVerboseLog( true );
#endif

#ifndef WIN32
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

		QApplication a(argc, argv);
		setPalette( &a );

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
					cout << VERSION << endl;
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
#endif

		showInfo();

		Logger::getInstance()->log( "Using data path: " + DataPath::getDataPath() );

		Preferences *pPref = Preferences::getInstance();
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
		a.setFont( QFont( family, pPref->getApplicationFontPointSize() ) );

		QTranslator tor( 0 );
		QString sTranslationFile = QString("hydrogen.") + QTextCodec::locale();
		QString sLocale = QTextCodec::locale();
		if ( sLocale != "C") {
			QString sTranslationPath = "data/i18n";
			QString total = sTranslationPath + "/" + sTranslationFile + ".qm";

			bool bTransOk = tor.load( total, "." );
			if ( bTransOk ) {
				cout << "Using locale: " << sTranslationPath << "/" << sTranslationFile << endl;
			}
			else {
				sTranslationPath = QString( DataPath::getDataPath().c_str() ) + "/i18n";
				total = sTranslationPath + "/" + sTranslationFile + ".qm";
				bTransOk = tor.load( total, "." );
				if (bTransOk) {
					cout << "Using locale: " << sTranslationPath << "/" << sTranslationFile << endl;
				}
				else {
					cerr << "Warning: no locale found: " << sTranslationPath << "/" << sTranslationFile << endl;
				}
			}
			//Introduced with QT3.1 :(
			if (tor.isEmpty()) {
				cerr << "Warning: error loading locale: " << total << endl;
			}
		}
		a.installTranslator( &tor );

		string sStyle = pPref->getQTStyle();
		if (sStyle != "" ) {
			a.setStyle( QString( sStyle.c_str() ) );
		}


		SplashScreen *pSplash = new SplashScreen();

		if (bNoSplash) {
			pSplash->hide();
		}
		else {
			pSplash->show();
		}

		a.setMainWidget( pSplash );

		MainForm *pMainForm = new MainForm( &a, songFilename );
		pMainForm->show();

	//    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
		a.exec();

		// destroy splash screen
		delete pSplash;
		pSplash = NULL;

		delete pMainForm;
		pMainForm = NULL;

		delete pPref;
		pPref = NULL;

		delete EventQueue::getInstance();

	//	if ( Object::isUsingVerboseLog() ) {
			int nObj = Object::getNObjects();
			if (nObj != 0) {
				Object::printObjectMap();
				std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
			}
	//	}
		Logger::getInstance()->log( "\nBye..." );
		delete Logger::getInstance();

	}
	catch ( const H2Exception& ex ) {
		cerr << "Exception: " << ex.message << endl;
	}
	catch ( ... ) {
		cerr << "Unknown exception.." << endl;
	}

	return 0;
}



/**
 * Show some information
 */
void showInfo() {
	Logger::getInstance()->log( "\nHydrogen " + string(VERSION) + " [" + string(__DATE__) + "]  [http://www.hydrogen-music.org]" );
	Logger::getInstance()->log( "Copyright 2002-2005 Alessandro Cominu\n\n" );
	Logger::getInstance()->log( "Compiled modules: " + string(COMPILED_FEATURES) );

	if ( Object::isUsingVerboseLog() ) {
		Logger::getInstance()->log( "Verbose log mode = active" );
	}

	Logger::getInstance()->log( "\nHydrogen comes with ABSOLUTELY NO WARRANTY" );
	Logger::getInstance()->log( "This is free software, and you are welcome to redistribute it" );
	Logger::getInstance()->log( "under certain conditions. See the file COPYING for details\n" );
}



/**
 * Show the correct usage
 */
void showUsage() {
	cout << "Usage: hydrogen [-v] [-h] -s file" << endl;
	cout << "   -d, --driver AUDIODRIVER - Use the selected audio driver (jack, alsa, oss)" << endl;
	cout << "   -s, --song FILE - Load a song (*.h2song) at startup" << endl;
	cout << "   -n, --nosplash - Hide splash screen" << endl;
	cout << "   -V, --verbose - Print a lot of debugging info" << endl;
	cout << "   -v, --version - Show version info" << endl;
	cout << "   -h, --help - Show this help message" << endl;
}



