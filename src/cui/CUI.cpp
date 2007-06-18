#include "CUI.h"

#include <ncurses.h>
#include <cstdlib>
#include "config.h"

#include "lib/Hydrogen.h"
#include "lib/Preferences.h"

CUI::CUI(string sSongFilename)
 : m_mode( HELP_MODE )
{
 	m_nCursor_x = 0;
 	m_nCursor_y = 0;

	Song *pSong = NULL;
	if (sSongFilename == "") {
		Preferences *pref = Preferences::getInstance();
		string filename = pref->getLastSongFilename();
		if ( pref->isRestoreLastSongEnabled() && (filename != "" )) {
			pSong = Song::load( filename );
		}
		else {
			pSong = Song::getEmptySong();
		}
	}
	else {
		pSong = Song::load( sSongFilename );
	}

	Hydrogen::getInstance()->setSong( pSong );



	showHelp();
}



CUI::~CUI()
{
}




void CUI::printInfo()
{
	clear();
	mvprintw( 0, 0, "----------------------------------------" );
	mvprintw( 1, 0, "Hydrogen %s", VERSION );
	mvprintw( 2, 0, "x:%d\ty:%d", m_nCursor_x, m_nCursor_y );

	int height, width;
	getmaxyx(stdscr, height, width);
	mvprintw( 3, 0, "Cols:%d\tRows:%d", width, height );


	mvprintw( 24, 0, "----------------------------------------" );
	move( m_nCursor_y, m_nCursor_x );
	refresh();
}




void CUI::showHelp()
{
	m_nCursor_x = 0;
	m_nCursor_y = 0;

	clear();
	mvprintw( 0, 0, "Hydrogen - Help" );
	mvprintw( 1, 0, "q - Quit" );
	mvprintw( 2, 0, "h - Show help page" );
	mvprintw( 3, 0, "p - Show pattern editor" );
	mvprintw( 4, 0, "Pag down - next pattern" );
	mvprintw( 5, 0, "Pag up - previous pattern" );

	move( m_nCursor_y, m_nCursor_x );
	refresh();
}



void CUI::handleKey(char ch)
{
	Hydrogen *pEngine = Hydrogen::getInstance();

	switch( ch ) {
		case 'h':	// HELP
			showHelp();
			break;

		case 'p':	// show pattern editor
			showPatternEditor();
			break;

		case ' ':	// play/stop
			if ( pEngine->getState() == STATE_READY ) {
				pEngine->start();
			}
			else if ( pEngine->getState() == STATE_PLAYING ) {
				pEngine->stop();
			}
			break;

		case KEY_DOWN:
			++m_nCursor_y;
			moveCursor();
			break;

		case KEY_UP:
			--m_nCursor_y;
			moveCursor();
			break;

		case KEY_LEFT:
			--m_nCursor_x;
			moveCursor();
			break;

		case KEY_RIGHT:
			++m_nCursor_x;
			moveCursor();
			break;

		case KEY_NPAGE:
			{
				int nPat = Hydrogen::getInstance()->getSelectedPatternNumber();
				if (nPat + 1 < Hydrogen::getInstance()->getSong()->getPatternList()->getSize() ) {
					Hydrogen::getInstance()->setSelectedPatternNumber( ++nPat );
					showPatternEditor();
				}
			}
			break;

		case KEY_PPAGE:
			{
				int nPat = Hydrogen::getInstance()->getSelectedPatternNumber();
				if ( nPat -1 >= 0 ) {
					Hydrogen::getInstance()->setSelectedPatternNumber( --nPat );
					showPatternEditor();
				}
			}
			break;
	}
}



void CUI::moveCursor()
{
	int height, width;
	getmaxyx(stdscr, height, width);
	width = 40; // braille interface limit
	height = 25;

	if ( m_nCursor_x < 0 ) {
		m_nCursor_x = 0;
	}
	if ( m_nCursor_x >= width ) {
		m_nCursor_x = width - 1;
	}
	if ( m_nCursor_y < 0 ) {
		m_nCursor_y = 0;
	}
	if ( m_nCursor_y >= height ) {
		m_nCursor_y = height - 1;
	}

//	printInfo();
	move( m_nCursor_y, m_nCursor_x );
	refresh();
}



void CUI::showPatternEditor()
{
	m_nCursor_x = 0;
	m_nCursor_y = 0;

	Song *pSong = Hydrogen::getInstance()->getSong();

	PatternList *pPatternList = pSong->getPatternList();
	Pattern *pPattern = pPatternList->get( Hydrogen::getInstance()->getSelectedPatternNumber() );


	clear();
	mvprintw( 0, 0, "----------------------------------------" );
	mvprintw( 1, 0, "Hydrogen - Pattern editor" );
	mvprintw( 2, 0, "Pattern [%d] - %s", Hydrogen::getInstance()->getSelectedPatternNumber(), pPattern->m_sName.c_str() );

	// lista strumenti
	for (int nInstr = 0; nInstr < pPattern->m_pSequenceList->getSize(); nInstr++ ) {
		Instrument *pInstr = pSong->getInstrumentList()->get( nInstr );
		mvprintw( 5 + nInstr, 0, "%s", pInstr->m_sName.c_str() );

		Sequence *pSeq = pPattern->m_pSequenceList->get( nInstr );
		// 1 16esimo = 12 tick
		for ( int i = 0; i < 16; i++ ) {
			Note *pNote = pSeq->m_noteList[ i * 12 ];
			if (pNote) {
				mvprintw( 5 + nInstr, i + 10, "X" );
			}
			else {
				mvprintw( 5 + nInstr, i + 10, "." );
			}
		}
	}


	mvprintw( 24, 0, "----------------------------------------" );

	move( m_nCursor_y, m_nCursor_x );

	refresh();
}


