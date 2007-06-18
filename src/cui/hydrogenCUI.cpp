#include "CUI.h"
#include <iostream>
using namespace std;

#include <ncurses.h>

#include "lib/Hydrogen.h"
#include "lib/Preferences.h"
#include "lib/EventQueue.h"

int main(int argc, char *argv[])
{
	string sSongFile = "";

	if ( argc == 2 ) {
		sSongFile = argv[1];
	}

	initscr();
	start_color();
	keypad(stdscr, TRUE);
	nonl();
	cbreak();
	noecho();

	CUI *pCUI = new CUI( sSongFile );

	int ch;
	while ( true ) {
		ch = getch();
		if ( ch == 'q' ) {
			break;
		}

		pCUI->handleKey( ch );
	}
	endwin();

	delete Hydrogen::getInstance()->getSong();
	delete pCUI;
	delete Hydrogen::getInstance();
	delete Preferences::getInstance();
	delete EventQueue::getInstance();

	int nObj = Object::getNObjects();
	if (nObj != 0) {
		Object::printObjectMap();
		std::cout << std::endl << std::endl << nObj << " alive objects" << std::endl << std::endl;
	}
	cout << endl << "Bye..." << endl;
	return 0;
}
