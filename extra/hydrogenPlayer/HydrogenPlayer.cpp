/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <QApplication>

#include <iostream>
#include <cstdio>
#include <string>

#include <hydrogen/Object.h>
#include <hydrogen/Hydrogen.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/EventQueue.h>
#include <hydrogen/AudioEngine.h>

using std::string;
using std::cout;
using std::endl;

void usage() 
{
	cout << "Usage: hydrogenPlayer song.h2song" << endl;
	exit(0);
}


int main(int argc, char** argv)
{
	QApplication a(argc, argv);

	_INFOLOG( "test" );

	if (argc != 2) {
		usage();
	}
	cout << "Hydrogen player starting..." << endl << endl;

	Object::useVerboseLog( true );

	string filename = argv[1];

	cout << 1 << endl;
	H2Core::Preferences *pPref = H2Core::Preferences::getInstance();

	cout << 2 << endl;
	H2Core::AudioEngine::getInstance();

	H2Core::Song *pSong = H2Core::Song::load(filename);
	if (pSong == NULL) {
		cout << "Error loading song!" << endl;
		exit(0);
	}

	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::getInstance();
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
				delete H2Core::EventQueue::getInstance();
				delete H2Core::AudioEngine::getInstance();
				delete pPref;
				delete Logger::getInstance();

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



