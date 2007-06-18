/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: HydrogenPlayer.cpp,v 1.7 2005/06/17 15:03:50 comix Exp $
 *
 */
#include "config.h"

#include <qapplication.h>

#include <iostream>
#include <stdio.h>
#include <string>

#include "lib/Hydrogen.h"
#include "lib/LocalFileMng.h"
#include "lib/Preferences.h"
#include "lib/fx/LadspaFX.h"
#include "lib/EventQueue.h"

using std::string;
using std::cout;
using std::endl;

void usage() {
	cout << "Usage: hydrogenPlayer song.h2song" << endl;
}


int main(int argc, char** argv) {
	if (argc != 2) {
		usage();
		exit(0);
	}
	cout << "Hydrogen player starting..." << endl << endl;
	QApplication a(argc, argv);

	Object::useVerboseLog( true );

	string filename = argv[1];

	Preferences *pPref = Preferences::getInstance();


	Song *pSong = Song::load(filename);
	if (pSong == NULL) {
		cout << "Error loading song!" << endl;
		exit(0);
	}

	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->setSong(pSong);
	
/*
	cout << "\n\n\nTrying to load a new LADSPA plugin" << endl;
	
	LadspaFX *pFX1 = pSong->getLadspaFX( 0 );
	if (pFX1) {
		cout << "FX1: " << pFX1->getPluginName() << endl;
	}
	
	string sLibraryPath = "/usr/lib/ladspa/phasers_1217.so";
	string sPluginLabel = "autoPhaser";
	long nSampleRate = 44100;
	LadspaFX *pNewFX = LadspaFX::load( sLibraryPath, sPluginLabel, nSampleRate );
	if ( pNewFX ) {
		pEngine->lockEngine( "main" );
		pSong->setLadspaFX( 0, pNewFX );
		Instrument *pKickDrum = pSong->getInstrumentList()->get( 0 );
		pKickDrum->setFXLevel( 0, 2.0 );
		
		pNewFX->setEnabled( true );
		pEngine->unlockEngine();
		pEngine->restartLadspaFX();
	}
	else {
		cout << "Error loading ladspa plugin" << endl;
	}
	cout << "\n\n\n" << endl;
*/	
	
	cout << "Press b for rewind from beginning" << endl;
	cout << "Press p for play" << endl;
	cout << "Press s for stop" << endl;
	cout << "Press q for quit" << endl;
	cout << "f = show frames" << endl;

	char pippo;

	while (true) {
		pippo = getchar();
		switch( pippo ) {
			case 'q':
				cout << endl << "HydrogenPlayer shutdown..." << endl;
				pEngine->stop();

				delete pEngine;
				delete pSong;
				delete pPref;
				delete Logger::getInstance();
				delete EventQueue::getInstance();

				std::cout << std::endl << std::endl << Object::getNObjects() << " alive objects" << std::endl << std::endl;
				Object::printObjectMap();

				exit( 0 );
				break;

			case 'p':
				pEngine->start();
				break;

			case 's':
				pEngine->stop();
				break;

			case 'b':
				pEngine->setPatternPos( 0 );
				break;

			case 'f':
				cout << "Frames = " << pEngine->getTotalFrames() << endl;
				break;

			case 'd':
				cout << "DEBUG" << endl;
				Object::printObjectMap();
				int nObj = Object::getNObjects();
				std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
				break;
		}
	}

}



