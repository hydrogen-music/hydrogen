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
* $Id: Hydrogen2Midi.cpp,v 1.6 2005/05/01 19:51:41 comix Exp $
*
*/

#include <iostream>
using namespace std;

#include "../lib/Song.h"
#include "../lib/smf/SMF.h"


void usage() {
	cout << endl << "Usage: hydrogen2midi input.h2song output.mid" << endl;
}


int main(int argc, char** argv) {
	cout << "hydrogen2midi (c) Alessandro Cominu" << endl;

	if (argc != 3) {
		usage();
		exit(0);
	}
	Object::useVerboseLog( true );


	string sInputFile = argv[1];
	string sOutputFile = argv[2];

	cout << "Input file: " << sInputFile << endl;
	cout << "Output file: " << sOutputFile << endl;

	// load hydrogen song
	Song *pSong = Song::load( sInputFile );
	if ( pSong == NULL) {
		cout << "Error loading song " << sInputFile << endl;
		exit(0);
	}

	// create the Standard Midi File object
	SMFWriter *pSmfWriter = new SMFWriter();
	pSmfWriter->save( sOutputFile, pSong );

	delete pSmfWriter;
	delete pSong;
	delete Logger::getInstance();


	int nObj = Object::getNObjects();
	if (nObj != 0) {
		std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
		Object::printObjectMap();
	}

	cout << "Bye.." << endl;

	return 0;
}

