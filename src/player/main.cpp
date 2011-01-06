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

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <hydrogen/Object.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/audio_engine.h>

using std::cout;
using std::endl;

#include <QApplication>

void usage()
{
	cout << "Usage: hydrogenPlayer song.h2song" << endl;
	exit(0);
}

void quit()
{
	_INFOLOG( "Quitting..." );
	sleep(1);
	cout << "\nBye..." << endl;
	delete Logger::get_instance();
	exit(0);
}

int main(int argc, char** argv)
{
	Object::use_verbose_log( true );
	_INFOLOG( "test" );


	if (argc != 2) {
		usage();
	}
	cout << "Hydrogen player starting..." << endl << endl;

	QApplication a(argc, argv);

	QString filename = argv[1];

	H2Core::Preferences *preferences = H2Core::Preferences::get_instance();
	H2Core::AudioEngine::get_instance();

	H2Core::Song *pSong = H2Core::Song::load( filename );
	if (pSong == NULL) {
		cout << "Error loading song!" << endl;
	}

	H2Core::Hydrogen *hydrogen = H2Core::Hydrogen::get_instance();
	hydrogen->setSong(pSong);


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
				hydrogen->sequencer_stop();

				delete hydrogen;
				delete pSong;
				delete H2Core::EventQueue::get_instance();
				delete H2Core::AudioEngine::get_instance();
				delete preferences;
				delete Logger::get_instance();

				std::cout << std::endl << std::endl << Object::get_objects_number() << " alive objects" << std::endl << std::endl;
				Object::print_object_map();

				exit(0);
				break;

			case 'p':
				hydrogen->sequencer_play();
				break;

			case 's':
				hydrogen->sequencer_stop();
				break;

			case 'b':
				hydrogen->setPatternPos( 0 );
				break;

			case 'f':
				cout << "Frames = " << hydrogen->getTotalFrames() << endl;
				break;

			case 'd':
				cout << "DEBUG" << endl;
				Object::print_object_map();
				int nObj = Object::get_objects_number();
				std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
				break;
		}
	}
}



