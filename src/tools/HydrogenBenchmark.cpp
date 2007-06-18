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
 * $Id: HydrogenBenchmark.cpp,v 1.10 2005/05/01 19:51:41 comix Exp $
 *
 */
#include "config.h"

#include <iostream>
#include <stdio.h>
#include <string>
using std::string;
#include <unistd.h>
#include "lib/Hydrogen.h"
#include "lib/LocalFileMng.h"
#include "lib/Preferences.h"
#include "lib/EventQueue.h"

using std::cout;
using std::endl;

int main(int argc, char** argv) {
	if (argc != 2) {
		cout << "Usage: hydrogenBenchmark numberOfNotes" << endl;
		exit(0);
	}
	cout << "Hydrogen benchmark is starting..." << endl << endl;

	Object::useVerboseLog( true );

	Preferences::getInstance()->m_nMaxNotes = 10000; // NO LIMITS ;)
	Preferences::getInstance()->m_sAudioDriver = "Fake";

	Song *pSong = Song::load( argv[1] );
	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->setSong( pSong );

	for (int i = 0; i < 10; i++) {
		pEngine->getAudioDriver()->locate( 0 );
		pEngine->start();
	}

	pEngine->stop();

	delete pEngine;
	delete pSong;
	delete Preferences::getInstance();
	delete EventQueue::getInstance();
	delete Logger::getInstance();

	sleep( 1 );

	int nObj = Object::getNObjects();
	if (nObj != 0 ) {
		std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
		Object::printObjectMap();
	}

	return 0;
}



