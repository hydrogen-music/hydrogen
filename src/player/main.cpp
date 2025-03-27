/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <core/Object.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/FX/Effects.h>
#include <core/EventQueue.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Helpers/Filesystem.h>

using std::cout;
using std::endl;

#include <QCoreApplication>

void usage()
{
	cout << "Usage: hydrogenPlayer song.h2song" << endl;
	exit(1);
}

int main(int argc, char** argv){

#ifdef WIN32
	// In case Hydrogen was started using a CLI attach its output to
	// the latter. 
	if ( AttachConsole(ATTACH_PARENT_PROCESS)) {
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "w", stdin);
	}
#endif
	
	unsigned logLevelOpt = H2Core::Logger::Error;
	H2Core::Logger::create_instance();
	H2Core::Logger::set_bit_mask( logLevelOpt );
	H2Core::Logger* logger = H2Core::Logger::get_instance();
	H2Core::Base::bootstrap( logger, logger->should_log(H2Core::Logger::Debug) );

	QCoreApplication a(argc, argv);

	H2Core::Filesystem::bootstrap( logger );

	if (argc != 2) {
		usage();
	}
	cout << "Hydrogen player starting..." << endl << endl;

	QString filename = argv[1];

	H2Core::Preferences::create_instance();
	H2Core::Hydrogen::create_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();

	// Tell the core that we are done initializing the most basic parts.
	pHydrogen->setGUIState( H2Core::Hydrogen::GUIState::headless );

	auto pPref = H2Core::Preferences::get_instance();
	// The Preferences is provided as a shared pointer. We discard our local
	// reference in order to not prevent cleanup to the old instance in case
	// it is replaced while Hydrogen is running.
	pPref = nullptr;

	std::shared_ptr<H2Core::Song>pSong = H2Core::Song::load( filename );
	if (pSong == nullptr) {
		cout << "Error loading song!" << endl;
		exit(2);
	}

	pHydrogen->setSong(pSong);

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
				pHydrogen->sequencerStop();

				pPref = H2Core::Preferences::get_instance();
				pPref->save();
				pSong = nullptr;
				delete pHydrogen;
				delete H2Core::EventQueue::get_instance();
				delete H2Core::Logger::get_instance();

				// There is no particular need to clean up the Preferences
				// outselves. This is just done in order for it to not appear in
				// the objects map printed below.
				pPref->replaceInstance( nullptr );
				pPref = nullptr;

				std::cout << std::endl << std::endl << H2Core::Base::objects_count() << " alive objects" << std::endl << std::endl;
				H2Core::Base::write_objects_map_to_cerr();

				exit(0);
				break;

			case 'p':
				pHydrogen->sequencerPlay();
				break;

			case 's':
				pHydrogen->sequencerStop();
				break;

			case 'b':
				H2Core::CoreActionController::locateToColumn( 0 );
				break;

			case 'f':
				cout << "Frame = " << pHydrogen->getAudioEngine()->
					getTransportPosition()->getFrame() << endl;
				break;

			case 'd':
				cout << "DEBUG" << endl;
				H2Core::Base::write_objects_map_to_cerr();
				int nObj = H2Core::Base::objects_count();
				std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
				break;
		}
	}
}



