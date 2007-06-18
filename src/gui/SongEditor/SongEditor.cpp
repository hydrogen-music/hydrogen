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
 */
#include <qtooltip.h>
#include <qpainter.h>
#include <qwidget.h>
#include <qvbox.h>
#include <qcursor.h>
#include <qglobal.h>
#include <algorithm>


#include "config.h"
#include "SongEditor.h"
#include "SongEditorPanel.h"
#include "gui/PatternEditor/PatternEditorPanel.h"
#include "gui/HydrogenApp.h"
#include "gui/widgets/Button.h"
#include "gui/PatternFillDialog.h"
#include "gui/PatternPropertiesDialog.h"
#include "gui/SongPropertiesDialog.h"
#include "gui/Skin.h"
#include "lib/Song.h"
#include "lib/Hydrogen.h"
#include "lib/Preferences.h"


SongEditor::SongEditor( QWidget *parent )
 : QWidget( parent , "SongEditor", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "SongEditor" )
 , m_bChanged( true )
 , m_bSequenceChanged( true )
 , m_bShowLasso( false )
 , m_bIsMoving( false )
{
	m_nGridHeight = 18;

	int m_nInitialWidth = 10 + m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH;
	int m_nInitialHeight = 10;

	this->resize( QSize(m_nInitialWidth, m_nInitialHeight) );

	createBackground();	// create m_backgroundPixmap pixmap

	update();

	setFocusPolicy (StrongFocus);
}



SongEditor::~SongEditor()
{
}




void SongEditor::keyPressEvent ( QKeyEvent * ev )
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	vector<PatternList*>* pColumns = pEngine->getSong()->getPatternGroupVector();

	if ( ev->key() == Key_Delete ) {
		if ( m_selectedCells.size() != 0 ) {
			pEngine->lockEngine( "SongEditor::mouseReleaseEvent" );
			// delete all selected cells
			for ( int i = 0; i < m_selectedCells.size(); i++ ) {
				QPoint cell = m_selectedCells[ i ];
				PatternList* pColumn = (*pColumns)[ cell.x() ];
				pColumn->del(pPatternList->get( cell.y() ) );
			}
			pEngine->unlockEngine();

			m_selectedCells.clear();
			m_bChanged = true;
			m_bSequenceChanged = true;
			update();
		}
		return;
	}

	ev->ignore();
}



void SongEditor::mousePressEvent( QMouseEvent *ev )
{
	if ( ev->x() < 10 ) {
		return;
	}

	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)SONG_EDITOR_GRID_WIDTH;

	if ( ev->state() == Qt::ControlButton ) {
		m_bIsCtrlPressed = true;
	}
	else {
		m_bIsCtrlPressed = false;
	}

	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->lockEngine( "SongEditor::mousePressEvent" );
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	if ( nRow >= (int)pPatternList->getSize() || nRow < 0 || nColumn < 0 ) {
		return;
	}


	SongEditorActionMode actionMode = HydrogenApp::getInstance()->getSongEditorPanel()->getActionMode();
	if ( actionMode == SELECT_ACTION ) {

		bool bOverExistingPattern = false;
		for ( int i = 0; i < m_selectedCells.size(); i++ ) {
			QPoint cell = m_selectedCells[ i ];
			if ( cell.x() == nColumn && cell.y() == nRow ) {
				bOverExistingPattern = true;
				break;
			}
		}

		if ( bOverExistingPattern ) {
			// MOVE PATTERNS
//			infoLog( "[mousePressEvent] Move patterns" );
			m_bIsMoving = true;
			m_bShowLasso = false;
			m_movingCells = m_selectedCells;

			m_clickPoint.setX( nColumn );
			m_clickPoint.setY( nRow );
		}
		else {
//			infoLog( "[mousePressEvent] Select patterns" );
			// select patterns
			m_bShowLasso = true;
			m_lasso.setCoords( ev->x(), ev->y(), ev->x(), ev->y() );
			setCursor( QCursor( Qt::CrossCursor ) );
			m_selectedCells.clear();
			m_selectedCells.push_back( QPoint( nColumn, nRow ) );
		}
	}
	else if ( actionMode == DRAW_ACTION ) {
		Pattern *pPattern = pPatternList->get( nRow );
		vector<PatternList*> *pColumns = pSong->getPatternGroupVector();	// E' la lista di "colonne" di pattern
		if ( nColumn < pColumns->size() ) {
			PatternList *pColumn = ( *pColumns )[ nColumn ];

			bool bFound = false;
			unsigned nColumnIndex = 0;
			for ( nColumnIndex = 0; nColumnIndex < pColumn->getSize(); nColumnIndex++) {
				if ( pColumn->get( nColumnIndex ) == pPattern ) { // il pattern e' gia presente
					bFound = true;
					break;
				}
			}

			if ( bFound ) {
				// DELETE PATTERN
//				infoLog( "[mousePressEvent] delete pattern" );
				pColumn->del( nColumnIndex );

				// elimino le colonne vuote
				for ( int i = pColumns->size() - 1; i >= 0; i-- ) {
					PatternList *pColumn = ( *pColumns )[ i ];
					if ( pColumn->getSize() == 0 ) {
						pColumns->erase( pColumns->begin() + i );
						delete pColumn;
					}
					else {
						break;
					}
				}
			}
			else {
				if ( nColumn < pColumns->size() ) {
					// ADD PATTERN
//					infoLog( "[mousePressEvent] add pattern" );
					m_selectedCells.clear();
					pColumn->add( pPattern );
				}
			}
		}
		else {
			// ADD PATTERN (with spaces..)
			m_selectedCells.clear();
			int nSpaces = nColumn - pColumns->size();
//			infoLog( "[mousePressEvent] add pattern (with " + toString( nSpaces ) + " spaces)" );

			PatternList *pColumn = new PatternList();
			pColumns->push_back( pColumn );

			for ( int i = 0; i < nSpaces; i++ ) {
				pColumn = new PatternList();
				pColumns->push_back( pColumn );
			}
			pColumn->add( pPattern );
		}
		pSong->m_bIsModified = true;
	}

	pEngine->unlockEngine();

	// update
	m_bChanged = true;
	m_bSequenceChanged = true;
	update();
}



void SongEditor::mouseMoveEvent(QMouseEvent *ev)
{
	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)SONG_EDITOR_GRID_WIDTH;
	PatternList *pPatternList = Hydrogen::getInstance()->getSong()->getPatternList();
	vector<PatternList*>* pColumns = Hydrogen::getInstance()->getSong()->getPatternGroupVector();

	if ( m_bIsMoving ) {
//		warningLog( "[mouseMoveEvent] Move patterns not implemented yet" );

		int nRowDiff = nRow  - m_clickPoint.y();
		int nColumnDiff = nColumn - m_clickPoint.x();

//		infoLog( "[mouseMoveEvent] row diff: "+ toString( nRowDiff ) );
//		infoLog( "[mouseMoveEvent] col diff: "+ toString( nColumnDiff ) );

		for ( int i = 0; i < m_movingCells.size(); i++ ) {
			QPoint cell = m_movingCells[ i ];
			m_movingCells[ i ].setX( m_selectedCells[ i ].x() + nColumnDiff );
			m_movingCells[ i ].setY( m_selectedCells[ i ].y() + nRowDiff );
		}

		m_bChanged = true;
		m_bSequenceChanged = true;
		update();
		return;
	}

	if ( m_bShowLasso ) {
		// SELECTION
		setCursor( QCursor( Qt::CrossCursor ) );
		int x = ev->x();
		int y = ev->y();
		if ( x < 0 ) {
			x = 0;
		}
		if ( y < 0 ) {
			y = 0;
		}
		m_lasso.setBottomRight( QPoint( x, y ) );

		// aggiorno la lista di celle selezionate
		m_selectedCells.clear();

		int nStartColumn =  ( m_lasso.left() - 10.0 ) / SONG_EDITOR_GRID_WIDTH;
		int nEndColumn = nColumn;
		if ( nStartColumn > nEndColumn ) {
			int nTemp = nEndColumn;
			nEndColumn = nStartColumn;
			nStartColumn = nTemp;
		}

		int nStartRow = m_lasso.top() / m_nGridHeight;
		int nEndRow = nRow;
		if ( nStartRow > nEndRow ) {
			int nTemp = nEndRow;
			nEndRow = nStartRow;
			nStartRow = nTemp;
		}

		for ( int nRow = nStartRow; nRow <= nEndRow; nRow++ ) {
			for ( int nCol = nStartColumn; nCol <= nEndColumn; nCol++ ) {
				if ( nRow >= (int)pPatternList->getSize() || nRow < 0 || nCol < 0 ) {
					return;
				}
				Pattern *pPattern = pPatternList->get( nRow );

				if ( nCol < pColumns->size() ) {
					PatternList *pColumn = ( *pColumns )[ nCol ];

					for (int i = 0; i < pColumn->getSize(); i++) {
						if ( pColumn->get(i) == pPattern ) { // esiste un pattern in questa posizione
							m_selectedCells.push_back( QPoint( nCol, nRow ) );
						}
					}
				}
			}
		}

		m_bChanged = true;
		m_bSequenceChanged = true;
		update();
	}

}



void SongEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	infoLog( "[mouseReleaseEvent]" );

	Hydrogen *pEngine = Hydrogen::getInstance();

	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	vector<PatternList*>* pColumns = pEngine->getSong()->getPatternGroupVector();

	if ( m_bIsMoving ) {	// fine dello spostamento dei pattern
		pEngine->lockEngine( "SongEditor::mouseReleaseEvent" );
		// create the new patterns
		for ( int i = 0; i < m_movingCells.size(); i++ ) {
			QPoint cell = m_movingCells[ i ];
			if ( cell.x() < 0 || cell.y() < 0 || cell.y() >= pPatternList->getSize() ) {
				// skip
				continue;
			}
			// aggiungo un pattern per volta
			PatternList* pColumn = NULL;
			if ( cell.x() < pColumns->size() ) {
				pColumn = (*pColumns)[ cell.x() ];
			}
			else {
				// creo dei patternlist vuoti
				int nSpaces = cell.x() - pColumns->size();
				for ( int i = 0; i <= nSpaces; i++ ) {
					pColumn = new PatternList();
					pColumns->push_back( pColumn );
				}
			}
			pColumn->add( pPatternList->get( cell.y() ) );
		}

		if ( m_bIsCtrlPressed ) {	// COPY
		}
		else {	// MOVE
			// remove the old patterns
			for ( int i = 0; i < m_selectedCells.size(); i++ ) {
				QPoint cell = m_selectedCells[ i ];
				PatternList* pColumn = NULL;
				if ( cell.x() < pColumns->size() ) {
					pColumn = (*pColumns)[ cell.x() ];
				}
				else {
					pColumn = new PatternList();
					pColumns->push_back( pColumn );
				}
				pColumn->del(pPatternList->get( cell.y() ) );
			}
		}

		// remove the empty patternlist at the end of the song
		for ( int i = pColumns->size() - 1; i != 0 ; i-- ) {
			PatternList *pList = (*pColumns)[ i ];
			int nSize = pList->getSize();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
				delete pList;
			}
			else {
				break;
			}
		}


		pEngine->getSong()->m_bIsModified = true;
		pEngine->unlockEngine();

		m_bIsMoving = false;
		m_movingCells.clear();
		m_selectedCells.clear();
	}

	setCursor( QCursor( Qt::ArrowCursor ) );

	m_bShowLasso = false;
	m_bChanged = true;
	m_bSequenceChanged = true;
	m_bIsCtrlPressed = false;
	update();
}



void SongEditor::paintEvent( QPaintEvent *ev )
{
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;
		// ridisegno tutto solo se sono cambiate le note
		if (m_bSequenceChanged) {
			bitBlt( &m_sequencePixmap, 0, 0, &m_backgroundPixmap, 0, 0, width(), height(), CopyROP );
			drawSequence();
			m_bSequenceChanged = false;
		}

		bitBlt( this, 0, 0, &m_sequencePixmap, 0, 0, width(), height(), CopyROP, true);
		setErasePixmap( m_sequencePixmap );
	}

	if ( m_bShowLasso ) {
		QPainter p( this );
		QPen pen( white );
		pen.setStyle( Qt::DotLine );
		p.setPen( pen );
		p.drawRect( m_lasso );
	}
}



void SongEditor::updateEditor()
{
	if(!isVisible()) {
		return;
	}

//	createBackground();
	update();
}



void SongEditor::createBackground()
{
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );
	QColor linesColor( pStyle->m_songEditor_lineColor.getRed(), pStyle->m_songEditor_lineColor.getGreen(), pStyle->m_songEditor_lineColor.getBlue() );

	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();

	uint nPatterns = pSong->getPatternList()->getSize();

	static int nOldHeight = -1;
	int nNewHeight = m_nGridHeight * nPatterns;

	if (nOldHeight != nNewHeight) {
		// cambiamento di dimensioni...
		if (nNewHeight == 0) {
			nNewHeight = 1;	// the pixmap should not be empty
		}

		m_backgroundPixmap.resize( width(), nNewHeight );	// initialize the pixmap
		m_sequencePixmap.resize( width(), nNewHeight );	// initialize the pixmap
		m_sequencePixmap.fill();
		this->resize( QSize( width(), nNewHeight ) );
	}

	m_backgroundPixmap.fill( alternateRowColor );		// fill the pixmap with white color

	QPainter p( &m_backgroundPixmap );
	p.setPen( linesColor );

	// sfondo per celle scure (alternato)
	for (uint i = 0; i < nPatterns; i++) {
		if ( ( i % 2) == 0) {
			uint y = m_nGridHeight * i;
			p.fillRect ( 0, y, m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH, 2, backgroundColor );
			p.fillRect ( 0, y + 2, m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH, m_nGridHeight - 4, alternateRowColor );
			p.fillRect ( 0, y + m_nGridHeight - 2, m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH, 2, backgroundColor );
		}
	}

	// celle...
	p.setPen( linesColor );

	// vertical lines
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * SONG_EDITOR_GRID_WIDTH;
		int x1 = x;
		int x2 = x + SONG_EDITOR_GRID_WIDTH;

		p.drawLine( x1, 0, x1, m_nGridHeight * nPatterns );
		p.drawLine( x2, 0, x2, m_nGridHeight * nPatterns );
	}

	// horizontal lines
	for (uint i = 0; i < nPatterns; i++) {
		uint y = m_nGridHeight * i;

		int y1 = y + 2;
		int y2 = y + m_nGridHeight - 2;

		p.drawLine( 0, y1, (m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH), y1 );
		p.drawLine( 0, y2, (m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH), y2 );
	}


/*	// vertical lines (erase..)
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = i * SONG_EDITOR_GRID_WIDTH;

		p.fillRect( x, 0, 2, m_nGridHeight * nPatterns, backgroundColor );
		p.fillRect( x + SONG_EDITOR_GRID_WIDTH, 0, -2, m_nGridHeight * nPatterns, backgroundColor );
	}
*/
	// horizontal lines (erase..)
	for (uint i = 0; i < nPatterns + 1; i++) {
		uint y = m_nGridHeight * i;

		p.fillRect( 0, y, m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH, 2, backgroundColor );
		p.fillRect( 0, y + m_nGridHeight, m_nMaxPatternSequence * SONG_EDITOR_GRID_WIDTH, -2, backgroundColor );
	}


	//~ celle


	m_bChanged = true;
	m_bSequenceChanged = true;
}



void SongEditor::drawSequence()
{
	Song* song = (Hydrogen::getInstance())->getSong();

	PatternList *patList = song->getPatternList();
	vector<PatternList*>* pColumns = song->getPatternGroupVector();

	uint listLength = patList->getSize();

	for (uint i = 0; i < pColumns->size(); i++) {
		PatternList* pColumn = (*pColumns)[ i ];

		for (uint nPat = 0; nPat < pColumn->getSize(); nPat++) {
			Pattern *pat = pColumn->get( nPat );

			int position = -1;
			// find the position in pattern list
			for (uint j = 0; j < listLength; j++) {
				Pattern *pat2 = patList->get( j );
				if (pat == pat2) {
					position = j;
					break;
				}
			}
			if (position == -1) {
				warningLog( "[drawSequence] position == -1, group = " + toString( i ) );
			}
			drawPattern( i, position );
		}
	}

	// Moving cells
	QPainter p( &m_sequencePixmap );
	p.setRasterOp( XorROP );
	QPen pen( gray );
	pen.setStyle( Qt::DotLine );
	p.setPen( pen );
	for ( int i = 0; i < m_movingCells.size(); i++ ) {
		int x = 10 + SONG_EDITOR_GRID_WIDTH * ( m_movingCells[ i ] ).x();
		int y = m_nGridHeight * ( m_movingCells[ i ] ).y();

		QColor patternColor;
//		patternColor.setRgb( 100, 100, 100 );
		patternColor.setRgb( 255, 255, 255 );

//		p.fillRect( x + 3, y + 3, SONG_EDITOR_GRID_WIDTH - 5, m_nGridHeight - 5, patternColor );
		p.fillRect( x + 2, y + 4, SONG_EDITOR_GRID_WIDTH - 3, m_nGridHeight - 7, patternColor );
	}

}



void SongEditor::drawPattern( int pos, int number )
{
	Preferences *pref = Preferences::getInstance();
	UIStyle *pStyle = pref->getDefaultUIStyle();
	QPainter p( &m_sequencePixmap );
// 	QColor patternColor;
	QColor patternColor( pStyle->m_songEditor_pattern1Color.getRed(), pStyle->m_songEditor_pattern1Color.getGreen(), pStyle->m_songEditor_pattern1Color.getBlue() );

	bool bIsSelected = false;
	for ( int i = 0; i < m_selectedCells.size(); i++ ) {
		QPoint point = m_selectedCells[ i ];
		if ( point.x() == pos && point.y() == number ) {
			bIsSelected = true;
			break;
		}
	}

	if ( bIsSelected ) {
		patternColor = patternColor.dark( 130 );
	}

	int x = 10 + SONG_EDITOR_GRID_WIDTH * pos;
	int y = m_nGridHeight * number;

// 	p.setPen( patternColor.light( 120 ) );  // willie - For the bevel - haven't yet figured how it's supposed to work...
// 	p.drawLine( x + 1, y + 3, SONG_EDITOR_GRID_WIDTH - 2, y - 3 );
	p.fillRect( x + 1, y + 3, SONG_EDITOR_GRID_WIDTH - 1, m_nGridHeight - 5, patternColor );
}





// :::::::::::::::::::





SongEditorPatternList::SongEditorPatternList( QWidget *parent )
 : QWidget( parent , "SongEditorPatternList", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "SongEditPatList" )
{
	m_nWidth = 200;
	m_nGridHeight = 18;

	m_bChanged = true;
	this->resize( m_nWidth, m_nInitialHeight );

	m_labelBackgroundLight.load( QString( Skin::getImagePath().append( "/songEditor/songEditorLabelBG.png" ).c_str() ) );
	m_labelBackgroundDark.load( QString( Skin::getImagePath().append( "/songEditor/songEditorLabelABG.png" ).c_str() ) );
	m_labelBackgroundSelected.load( QString( Skin::getImagePath().append( "/songEditor/songEditorLabelSBG.png" ).c_str() ) );
	m_playingPattern_on_Pixmap.load( QString( Skin::getImagePath().append( "/songEditor/playingPattern_on.png" ).c_str() ) );
	m_playingPattern_off_Pixmap.load( QString( Skin::getImagePath().append( "/songEditor/playingPattern_off.png" ).c_str() ) );

	QPixmap patternIcon;
	patternIcon.load( QString( Skin::getImagePath().append("/songEditor/patternIcon.png").c_str() ) );

	m_pPatternPopup = new QPopupMenu( this, "patternPopupMenu" );
	m_pPatternPopup->insertItem( patternIcon, trUtf8("Edit"),  this, SLOT( patternPopup_edit() ) );
	m_pPatternPopup->insertItem( patternIcon, trUtf8("Copy"),  this, SLOT( patternPopup_copy() ) );
	m_pPatternPopup->insertItem( patternIcon, trUtf8("Delete"),  this, SLOT( patternPopup_delete() ) );
	m_pPatternPopup->insertItem( patternIcon, trUtf8("Fill/Clear ..."),  this, SLOT( patternPopup_fill() ) );
	m_pPatternPopup->insertItem( patternIcon, trUtf8("Properties"),  this, SLOT( patternPopup_properties() ) );

	createBackground();
	update();
}



SongEditorPatternList::~SongEditorPatternList()
{
}



/// Single click, select the next pattern
void SongEditorPatternList::mousePressEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	PatternList *patternList = song->getPatternList();

	if ( row >= (int)patternList->getSize() ) {
		return;
	}
	engine->setSelectedPatternNumber(row);

	if (ev->button() == RightButton) {
		m_pPatternPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
	}
	else if ( ev->button() == LeftButton ) {
		if ( song->getMode() == Song::PATTERN_MODE ) {

			PatternList *pCurrentPatternList = engine->getCurrentPatternList();
			if ( pCurrentPatternList->getSize() == 0 ) {
				// nessun pattern e' attivo. seleziono subito questo.
				pCurrentPatternList->add( patternList->get( row ) );
			}
			else {
				engine->setNextPattern( row );
			}

		}
	}

	createBackground();
	update();
}



void SongEditorPatternList::mouseDoubleClickEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);

	Hydrogen *engine = Hydrogen::getInstance();

	Song *song = engine->getSong();
	PatternList *patternList = song->getPatternList();
	if ( row >= (int)patternList->getSize() ) {
		return;
	}

	engine->setSelectedPatternNumber(row);

	patternPopup_edit();
}



void SongEditorPatternList::paintEvent( QPaintEvent *ev ) {
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;
		// no op
	}
	bitBlt( this, 0, 0, &m_backgroundPixmap, 0, 0, m_nWidth, height(), CopyROP );
}



void SongEditorPatternList::updateEditor()
{
	if(!isVisible()) {
		return;
	}

	update();
}



void SongEditorPatternList::createBackground()
{
	Preferences *pref = Preferences::getInstance();
	UIStyle *pStyle = pref->getDefaultUIStyle();
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );

	QString family = pref->getApplicationFontFamily().c_str();
	int size = pref->getApplicationFontPointSize();
	QFont textFont( family, size );

	QFont boldTextFont( textFont);
	boldTextFont.setBold( true );

	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();
	int nPatterns = pSong->getPatternList()->getSize();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();

	static int oldHeight = -1;
	int newHeight = m_nGridHeight * nPatterns;

	if (oldHeight != newHeight) {
		if (newHeight == 0) {
			newHeight = 1;	// the pixmap should not be empty
		}
		m_backgroundPixmap.resize( m_nWidth, newHeight );	// initialize the pixmap
		this->resize( m_nWidth, newHeight );
	}
	m_backgroundPixmap.fill( Qt::black );

	QPainter p( &m_backgroundPixmap );
	p.setFont( boldTextFont );

	for ( int i = 0; i < nPatterns; i++ ) {
		uint y = m_nGridHeight * i;
		if ( (int)i == nSelectedPattern ) {
			p.drawPixmap( QPoint( 0, y ), m_labelBackgroundSelected );
		}
		else {
			if ( ( i % 2) == 0 ) {
				p.drawPixmap( QPoint( 0, y ), m_labelBackgroundDark );
			}
			else {
				p.drawPixmap( QPoint( 0, y ), m_labelBackgroundLight );
			}
		}
	}

	PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();

	// horizontal lines
	for ( int i = 0; i < nPatterns; i++ ) {
		Pattern *pPattern = pSong->getPatternList()->get(i);
		uint y = m_nGridHeight * i;

		// Text
		bool bActive = false;
		for (uint j = 0; j < pCurrentPatternList->getSize(); j++) {
			if ( pPattern == pCurrentPatternList->get(j) ) {
				bActive = true;
				break;
			}
		}

		if ( i == nSelectedPattern ) {
			p.setPen( QColor( 0,0,0 ) );
		}
		else {
			p.setPen( textColor );
		}

		uint text_y = i * m_nGridHeight;
		if (bActive) {
//			p.drawText( 5, text_y - 1 - m_nGridHeight, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, ">" );
			p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_on_Pixmap );
		}
		else {
//			p.drawPixmap( QPoint( 5, text_y ), m_playingPattern_off_Pixmap );
		}


		p.drawText( 25, text_y - 1, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, ( pPattern->m_sName ).c_str() );
	}

	m_bChanged = true;
}



void SongEditorPatternList::patternPopup_edit()
{
	HydrogenApp::getInstance()->getPatternEditorPanel()->show();
	HydrogenApp::getInstance()->getPatternEditorPanel()->setFocus();
}



void SongEditorPatternList::patternPopup_properties()
{
	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	PatternList *patternList = song->getPatternList();

	int nSelectedPattern = engine->getSelectedPatternNumber();
	Pattern *pattern = patternList->get( nSelectedPattern );

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog(this, pattern);
	if (dialog->exec() == QDialog::Accepted) {
		Hydrogen *engine = Hydrogen::getInstance();
		Song *song = engine->getSong();
		song->m_bIsModified = true;
		createBackground();
		update();
	}
	delete dialog;
	dialog = NULL;
}



void SongEditorPatternList::patternPopup_delete()
{
	Hydrogen *pEngine = Hydrogen::getInstance();

//	int state = engine->getState();
// 	// per ora non lascio possibile la cancellazione del pattern durante l'esecuzione
// 	// da togliere quando correggo il bug
//         if (state == PLAYING) {
//                 QMessageBox::information( this, "Hydrogen", trUtf8("Can't delete the pattern while the audio engine is playing"));
//                 return;
//         }

	if ( pEngine->getSong()->getMode() == Song::PATTERN_MODE ) {
		pEngine->setNextPattern( -1 );	// reimposto il prossimo pattern a NULL, altrimenti viene scelto quello che sto distruggendo ora...
	}

	pEngine->lockEngine( "SongEditorPatternList::patternPopup_delete" );

	Song *song = pEngine->getSong();
	PatternList *pSongPatternList = song->getPatternList();

	Pattern *pattern = pSongPatternList->get( pEngine->getSelectedPatternNumber() );
	infoLog( "[patternPopup_delete] Delete pattern: " + pattern->m_sName + " @" + toString( (long)pattern ) );
	pSongPatternList->del(pattern);

	vector<PatternList*> *patternGroupVect = song->getPatternGroupVector();

	uint i = 0;
	while (i < patternGroupVect->size() ) {
		PatternList *list = (*patternGroupVect)[i];

		uint j = 0;
		while ( j < list->getSize() ) {
			Pattern *pOldPattern = list->get( j );
			if (pOldPattern == pattern ) {
				list->del( j );
				continue;
			}
			j++;
		}
// 		for (uint j = 0; j < list->getSize(); j++) {
// 			Pattern *pOldPattern = list->get( j );
// 			if (pOldPattern == pattern ) {
// 				list->del( j );
// 			}
// 		}

/*		if (list->getSize() == 0 ) {
			patternGroupVect->erase( patternGroupVect->begin() + i );
			delete list;
			list = NULL;
		}
		else {
*/			i++;
//		}
	}


	PatternList *list = pEngine->getCurrentPatternList();
	list->del( pattern );
	// se esiste, seleziono il primo pattern
	if ( pSongPatternList->getSize() > 0 ) {
		Pattern *pFirstPattern = pSongPatternList->get( 0 );
		list->add( pFirstPattern );
		// Cambio due volte...cosi' il pattern editor viene costretto ad aggiornarsi
		pEngine->setSelectedPatternNumber( -1 );
		pEngine->setSelectedPatternNumber( 0 );
	}
	else {
		// there's no patterns..
		pEngine->setSelectedPatternNumber( -1 );	// cosi' il pattern editor viene costretto ad aggiornarsi
	}

	delete pattern;

	song->m_bIsModified = true;

	pEngine->unlockEngine();

	( HydrogenApp::getInstance() )->getSongEditorPanel()->updateAll();
}



void SongEditorPatternList::patternPopup_copy()
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	Pattern *pPattern = pPatternList->get( nSelectedPattern );

	Pattern *pNewPattern = pPattern->copy();
	pPatternList->add( pNewPattern );

	// rename the copied pattern
	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, pNewPattern );
	if ( dialog->exec() == QDialog::Accepted ) {
		pSong->m_bIsModified = true;
		pEngine->setSelectedPatternNumber(pPatternList->getSize() - 1);	// select the last pattern (the copied one)
		if (pSong->getMode() == Song::PATTERN_MODE) {
			pEngine->setNextPattern( pPatternList->getSize() - 1 );	// select the last pattern (the new copied pattern)
		}
	}
	else {
		pPatternList->del( pNewPattern );
		delete pNewPattern;
	}
	delete dialog;

	HydrogenApp::getInstance()->getSongEditorPanel()->updateAll();
}

void SongEditorPatternList::patternPopup_fill()
{

	Hydrogen *pEngine = Hydrogen::getInstance();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	FillRange range;
	PatternFillDialog *dialog = new PatternFillDialog( this, &range );
	SongEditorPanel *pSEPanel = HydrogenApp::getInstance()->getSongEditorPanel();


	// use a PatternFillDialog to get the range and mode data
	if ( dialog->exec() == QDialog::Accepted ) {
		fillRangeWithPattern(&range, nSelectedPattern);
		pSEPanel->updateAll();
	}

	delete dialog;

}


void SongEditorPatternList::fillRangeWithPattern(FillRange* pRange, int nPattern)
{

	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->lockEngine( "SongEditorPatternList::fillRangeWithPattern" );


	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	Pattern *pPattern = pPatternList->get( nPattern );
	vector<PatternList*> *pColumns = pSong->getPatternGroupVector();	// E' la lista di "colonne" di pattern
	PatternList *pColumn;

	int nColumn, nColumnIndex;
	bool bHasPattern = false;
	int fromVal = pRange->fromVal - 1;
	int toVal   = pRange->toVal;



	// Add patternlists to PatternGroupVector as necessary

	int nDelta = toVal - pColumns->size() + 1;

	for ( int i = 0; i < nDelta; i++ ) {
		pColumn = new PatternList();
		pColumns->push_back( pColumn );
	}


	// Fill or Clear each cell in range

	for ( nColumn = fromVal; nColumn < toVal; nColumn++ ) {

		// expand Pattern
		pColumn = ( *pColumns )[ nColumn ];

		bHasPattern = false;

		// check whether the pattern (and column) already exists
		for ( nColumnIndex = 0; pColumn && nColumnIndex < pColumn->getSize(); nColumnIndex++) {

			if ( pColumn->get( nColumnIndex ) == pPattern ) {
				bHasPattern = true;
				break;
			}
		}

		if ( pRange->bInsert && !bHasPattern ) {       //fill
			pColumn->add( pPattern);
		}
		else if ( !pRange->bInsert && bHasPattern ) {  // clear
			pColumn->del( pPattern);
		}
	}

		// remove all the empty patternlists at the end of the song
		for ( int i = pColumns->size() - 1; i != 0 ; i-- ) {
			PatternList *pList = (*pColumns)[ i ];
			int nSize = pList->getSize();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
				delete pList;
			}
			else {
				break;
			}
		}
	pEngine->unlockEngine();


	// Update
	pSong->m_bIsModified = true;
	m_bChanged = true;
}


// ::::::::::::::::::::::::::



SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent , "SongEditorPositionRuler", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "SongEditPosRuler" )
{
	m_bChanged = true;
	this->resize( m_nInitialWidth, m_nHeight );

	m_tempPixmap.resize( m_nInitialWidth, m_nHeight );	// initialize the pixmap
	m_tempPixmap.fill();

	m_backgroundPixmap.resize( m_nInitialWidth, m_nHeight );	// initialize the pixmap

	createBackground();	// create m_backgroundPixmap pixmap

	// create tick position pixmap
	string tickPosition_path = Skin::getImagePath() + string( "/patternEditor/tickPosition.png" );
	bool ok = m_tickPositionPixmap.load(tickPosition_path.c_str());
	if( ok == false ){
		errorLog( "Error loading pixmap " + tickPosition_path );
	}

	update();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updatePosition()));
	m_pTimer->start(200);
}



SongEditorPositionRuler::~SongEditorPositionRuler() {
	m_pTimer->stop();
}



void SongEditorPositionRuler::createBackground()
{
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );

	m_backgroundPixmap.fill( backgroundColor );

	Preferences *pref = Preferences::getInstance();
	QString family = pref->getApplicationFontFamily().c_str();
	int size = pref->getApplicationFontPointSize();
	QFont font( family, size );

	QPainter p( &m_backgroundPixmap );
	p.setFont( font );

	char tmp[10];
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * SONG_EDITOR_GRID_WIDTH;
		if ( (i % 4) == 0 ) {
			p.setPen( textColor );
			sprintf( tmp, "%d", i + 1 );
			p.drawText( x - SONG_EDITOR_GRID_WIDTH, 0, SONG_EDITOR_GRID_WIDTH * 2, height(), Qt::AlignCenter, tmp );
		}
		else {
			p.setPen( textColor );
			p.drawLine( x, 10, x, m_nHeight - 10 );
		}
	}

	p.setPen( QColor(35, 39, 51) );
	p.drawLine( 0, 0, width(), 0 );

	p.fillRect ( 0, height() - 3, width(), 2, alternateRowColor );

}



void SongEditorPositionRuler::mouseMoveEvent(QMouseEvent *ev)
{
	mousePressEvent( ev );
}



void SongEditorPositionRuler::mousePressEvent( QMouseEvent *ev )
{
	int column = (ev->x() / SONG_EDITOR_GRID_WIDTH);

	if ( column >= Hydrogen::getInstance()->getSong()->getPatternGroupVector()->size() ) {
		return;
	}

	int nPatternPos = Hydrogen::getInstance()->getPatternPos();
	if ( nPatternPos != column ) {
		Hydrogen::getInstance()->setPatternPos( column );
		update();
	}
}



void SongEditorPositionRuler::paintEvent( QPaintEvent *ev )
{
	if (!isVisible()) {
		return;
	}

	static float fOldPosition = -1000;
	float fPos = ( Hydrogen::getInstance() )->getPatternPos();

	if ( Hydrogen::getInstance()->getCurrentPatternList()->getSize() != 0 ) {
		Pattern *pPattern = Hydrogen::getInstance()->getCurrentPatternList()->get( 0 );
		fPos += (float)Hydrogen::getInstance()->getTickPosition() / (float)pPattern->m_nSize;
	}
	else {
		// nessun pattern, uso la grandezza di default
		fPos += (float)Hydrogen::getInstance()->getTickPosition() / (float)MAX_NOTES;
	}

	if ( fOldPosition != fPos ) {
		m_bChanged = true;
		fOldPosition = fPos;
	}

	if ( Hydrogen::getInstance()->getSong()->getMode() == Song::PATTERN_MODE ) {
		fPos = -1;
	}

//	if (m_bChanged) {
		m_bChanged = false;
		//bitBlt( this, 0, 0, &m_backgroundPixmap, 0, 0, m_nInitialWidth, m_nHeight, CopyROP );
		bitBlt( this, QPoint( ev->rect().x(), ev->rect().y() ), &m_backgroundPixmap, ev->rect(), CopyROP );


		if (fPos != -1) {
			uint x = 10 + fPos * SONG_EDITOR_GRID_WIDTH - 11 / 2;
			bitBlt( this, x, height() / 2, &m_tickPositionPixmap, 0, 0, 11, 8, CopyROP );
		}

		//setErasePixmap( m_tempPixmap );
//	}
}



void SongEditorPositionRuler::updatePosition()
{
	update();
}


