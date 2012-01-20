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

#include <assert.h>
#include <algorithm>
#include <memory>
#include <unistd.h>

#include <hydrogen/Song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/instrument.h>
using namespace H2Core;


#include "SongEditor.h"
#include "SongEditorPanel.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanelTagWidget.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../widgets/Button.h"
#include "../PatternFillDialog.h"
#include "../PatternPropertiesDialog.h"
#include "../SongPropertiesDialog.h"
#include "../Skin.h"
#include "../VirtualPatternDialog.h"
#include <hydrogen/LocalFileMng.h>


using namespace std;

SongEditor::SongEditor( QWidget *parent )
 : QWidget( parent )
 , Object( "SongEditor" )
 , m_bSequenceChanged( true )
 , m_bIsMoving( false )
 , m_bShowLasso( false )
{
	setAttribute(Qt::WA_NoBackground);
	setFocusPolicy (Qt::StrongFocus);

	m_nGridWidth = 16;
	m_nGridHeight = 18;

	Preferences *pref = Preferences::get_instance();
	m_nMaxPatternSequence = pref->getMaxBars();


	int m_nInitialWidth = 10 + m_nMaxPatternSequence * m_nGridWidth;
	int m_nInitialHeight = 10;

	this->resize( QSize(m_nInitialWidth, m_nInitialHeight) );

	createBackground();	// create m_backgroundPixmap pixmap

	update();
}



SongEditor::~SongEditor()
{
}



int SongEditor::getGridWidth ()
{
	return m_nGridWidth;
}



void SongEditor::setGridWidth( uint width )
{
	if ( ( SONG_EDITOR_MIN_GRID_WIDTH <= width ) && ( SONG_EDITOR_MAX_GRID_WIDTH >= width ) ) {
		m_nGridWidth = width;
		this->resize ( 10 + m_nMaxPatternSequence * m_nGridWidth, height() );
	}
}



void SongEditor::keyPressEvent ( QKeyEvent * ev )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	vector<PatternList*>* pColumns = pEngine->getSong()->get_pattern_group_vector();

	if ( ev->key() == Qt::Key_Delete ) {
		if ( m_selectedCells.size() != 0 ) {
			AudioEngine::get_instance()->lock( RIGHT_HERE );
			// delete all selected cells
			for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
				QPoint cell = m_selectedCells[ i ];
				PatternList* pColumn = (*pColumns)[ cell.x() ];
				pColumn->del(pPatternList->get( cell.y() ) );
			}
			AudioEngine::get_instance()->unlock();

			m_selectedCells.clear();
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
	//WARNINGLOG( "editor-pressed" );

	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)m_nGridWidth;

	if ( ev->modifiers() == Qt::ControlModifier ) {
		INFOLOG( "[mousePressEvent] CTRL pressed!" );
		m_bIsCtrlPressed = true;
	}
	else {
		m_bIsCtrlPressed = false;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	// don't lock the audio driver before checking that...
	if ( nRow >= (int)pPatternList->get_size() || nRow < 0 || nColumn < 0 ) { return; }
	AudioEngine::get_instance()->lock( RIGHT_HERE );


	SongEditorActionMode actionMode = HydrogenApp::get_instance()->getSongEditorPanel()->getActionMode();
	if ( actionMode == SELECT_ACTION ) {

		bool bOverExistingPattern = false;
		for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
			QPoint cell = m_selectedCells[ i ];
			if ( cell.x() == nColumn && cell.y() == nRow ) {
				bOverExistingPattern = true;
				break;
			}
		}

		if ( bOverExistingPattern ) {
			// MOVE PATTERNS
//			INFOLOG( "[mousePressEvent] Move patterns" );
			m_bIsMoving = true;
			m_bShowLasso = false;
			m_movingCells = m_selectedCells;

			m_clickPoint.setX( nColumn );
			m_clickPoint.setY( nRow );
		}
		else {
//			INFOLOG( "[mousePressEvent] Select patterns" );
			// select patterns
			m_bShowLasso = true;
			m_lasso.setCoords( ev->x(), ev->y(), ev->x(), ev->y() );
			setCursor( QCursor( Qt::CrossCursor ) );
			m_selectedCells.clear();
			m_selectedCells.push_back( QPoint( nColumn, nRow ) );
		}
	}
	else if ( actionMode == DRAW_ACTION ) {
		H2Core::Pattern *pPattern = pPatternList->get( nRow );
		vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();	// E' la lista di "colonne" di pattern
		if ( nColumn < (int)pColumns->size() ) {
			PatternList *pColumn = ( *pColumns )[ nColumn ];

			bool bFound = false;
			unsigned nColumnIndex = 0;
			for ( nColumnIndex = 0; nColumnIndex < pColumn->get_size(); nColumnIndex++) {
				if ( pColumn->get( nColumnIndex ) == pPattern ) { // il pattern e' gia presente
					bFound = true;
					break;
				}
			}

			if ( bFound ) {
				// DELETE PATTERN
//				INFOLOG( "[mousePressEvent] delete pattern" );
				pColumn->del( nColumnIndex );

				// elimino le colonne vuote
				for ( int i = pColumns->size() - 1; i >= 0; i-- ) {
					PatternList *pColumn = ( *pColumns )[ i ];
					if ( pColumn->get_size() == 0 ) {
						pColumns->erase( pColumns->begin() + i );
						delete pColumn;
					}
					else {
						break;
					}
				}
			}
			else {
				if ( nColumn < (int)pColumns->size() ) {
					// ADD PATTERN
//					INFOLOG( "[mousePressEvent] add pattern" );
					m_selectedCells.clear();
					pColumn->add( pPattern );
				}
			}
		}
		else {
			// ADD PATTERN (with spaces..)
			m_selectedCells.clear();
			int nSpaces = nColumn - pColumns->size();
//			INFOLOG( "[mousePressEvent] add pattern (with " + to_string( nSpaces ) + " spaces)" );

			PatternList *pColumn = new PatternList();
			pColumns->push_back( pColumn );

			for ( int i = 0; i < nSpaces; i++ ) {
				pColumn = new PatternList();
				pColumns->push_back( pColumn );
			}
			pColumn->add( pPattern );
		}
		pSong->__is_modified = true;
	}

	AudioEngine::get_instance()->unlock();

	// update
	m_bSequenceChanged = true;
	update();
}



void SongEditor::mouseMoveEvent(QMouseEvent *ev)
{
	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)m_nGridWidth;
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->get_pattern_list();
	vector<PatternList*>* pColumns = Hydrogen::get_instance()->getSong()->get_pattern_group_vector();

	if ( m_bIsMoving ) {
//		WARNINGLOG( "[mouseMoveEvent] Move patterns not implemented yet" );

		int nRowDiff = nRow  - m_clickPoint.y();
		int nColumnDiff = nColumn - m_clickPoint.x();

//		INFOLOG( "[mouseMoveEvent] row diff: "+ to_string( nRowDiff ) );
//		INFOLOG( "[mouseMoveEvent] col diff: "+ to_string( nColumnDiff ) );

		for ( int i = 0; i < (int)m_movingCells.size(); i++ ) {
			QPoint cell = m_movingCells[ i ];
			m_movingCells[ i ].setX( m_selectedCells[ i ].x() + nColumnDiff );
			m_movingCells[ i ].setY( m_selectedCells[ i ].y() + nRowDiff );
		}

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

		int nStartColumn = (int)( ( m_lasso.left() - 10.0 ) / m_nGridWidth );
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
				if ( nRow >= (int)pPatternList->get_size() || nRow < 0 || nCol < 0 ) {
					return;
				}
				H2Core::Pattern *pPattern = pPatternList->get( nRow );

				if ( nCol < (int)pColumns->size() ) {
					PatternList *pColumn = ( *pColumns )[ nCol ];

					for ( uint i = 0; i < pColumn->get_size(); i++) {
						if ( pColumn->get(i) == pPattern ) { // esiste un pattern in questa posizione
							m_selectedCells.push_back( QPoint( nCol, nRow ) );
						}
					}
				}
			}
		}

		m_bSequenceChanged = true;
		update();
	}

}



void SongEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	UNUSED( ev );
//	INFOLOG( "[mouseReleaseEvent]" );

	Hydrogen *pEngine = Hydrogen::get_instance();

	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	vector<PatternList*>* pColumns = pEngine->getSong()->get_pattern_group_vector();

	if ( m_bIsMoving ) {	// fine dello spostamento dei pattern
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		// create the new patterns
		for ( uint i = 0; i < m_movingCells.size(); i++ ) {
			QPoint cell = m_movingCells[ i ];
			if ( cell.x() < 0 || cell.y() < 0 || cell.y() >= (int)pPatternList->get_size() ) {
				// skip
				continue;
			}
			// aggiungo un pattern per volta
			PatternList* pColumn = NULL;
			if ( cell.x() < (int)pColumns->size() ) {
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
			for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
				QPoint cell = m_selectedCells[ i ];
				PatternList* pColumn = NULL;
				if ( cell.x() < (int)pColumns->size() ) {
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
			int nSize = pList->get_size();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
				delete pList;
			}
			else {
				break;
			}
		}


		pEngine->getSong()->__is_modified = true;
		AudioEngine::get_instance()->unlock();

		m_bIsMoving = false;
		m_movingCells.clear();
		m_selectedCells.clear();
	}

	setCursor( QCursor( Qt::ArrowCursor ) );

	m_bShowLasso = false;
	m_bSequenceChanged = true;
	m_bIsCtrlPressed = false;
	update();
}



void SongEditor::paintEvent( QPaintEvent *ev )
{
/*	INFOLOG(
			"[paintEvent] x: " + to_string( ev->rect().x() ) +
			" y: " + to_string( ev->rect().y() ) +
			" w: " + to_string( ev->rect().width() ) +
			" h: " + to_string( ev->rect().height() )
	);
*/

	// ridisegno tutto solo se sono cambiate le note
	if (m_bSequenceChanged) {
		m_bSequenceChanged = false;
		drawSequence();
	}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pSequencePixmap, ev->rect() );

	if ( m_bShowLasso ) {
		QPen pen( Qt::white );
		pen.setStyle( Qt::DotLine );
		painter.setPen( pen );
		painter.drawRect( m_lasso );
	}
}



void SongEditor::createBackground()
{
	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );
	QColor linesColor( pStyle->m_songEditor_lineColor.getRed(), pStyle->m_songEditor_lineColor.getGreen(), pStyle->m_songEditor_lineColor.getBlue() );

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();

	uint nPatterns = pSong->get_pattern_list()->get_size();

	static int nOldHeight = -1;
	int nNewHeight = m_nGridHeight * nPatterns;

	if (nOldHeight != nNewHeight) {	
		// cambiamento di dimensioni...
		if (nNewHeight == 0) {
			nNewHeight = 1;	// the pixmap should not be empty
		}

		m_pBackgroundPixmap = new QPixmap( width(), nNewHeight );	// initialize the pixmap
		m_pSequencePixmap = new QPixmap( width(), nNewHeight );	// initialize the pixmap
		this->resize( QSize( width(), nNewHeight ) );
	}

	m_pBackgroundPixmap->fill( alternateRowColor );

	QPainter p( m_pBackgroundPixmap );
	p.setPen( linesColor );

/*	// sfondo per celle scure (alternato)
	for (uint i = 0; i < nPatterns; i++) {
		if ( ( i % 2) != 0) {
			uint y = m_nGridHeight * i;
			p.fillRect ( 0, y, m_nMaxPatternSequence * m_nGridWidth, 2, backgroundColor );
			p.fillRect ( 0, y + 2, m_nMaxPatternSequence * m_nGridWidth, m_nGridHeight - 4, alternateRowColor );
			p.fillRect ( 0, y + m_nGridHeight - 2, m_nMaxPatternSequence * m_nGridWidth, 2, backgroundColor );
		}
	}
*/
	// celle...
	p.setPen( linesColor );

	// vertical lines
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * m_nGridWidth;
		int x1 = x;
		int x2 = x + m_nGridWidth;

		p.drawLine( x1, 0, x1, m_nGridHeight * nPatterns );
		p.drawLine( x2, 0, x2, m_nGridHeight * nPatterns );
	}

	p.setPen( linesColor );
	// horizontal lines
	for (uint i = 0; i < nPatterns; i++) {
		uint y = m_nGridHeight * i;

		int y1 = y + 2;
		int y2 = y + m_nGridHeight - 2;

		p.drawLine( 0, y1, (m_nMaxPatternSequence * m_nGridWidth), y1 );
		p.drawLine( 0, y2, (m_nMaxPatternSequence * m_nGridWidth), y2 );
	}


	p.setPen( backgroundColor );
	// horizontal lines (erase..)
	for (uint i = 0; i < nPatterns + 1; i++) {
		uint y = m_nGridHeight * i;

		p.fillRect( 0, y, m_nMaxPatternSequence * m_nGridWidth, 2, backgroundColor );
		p.drawLine( 0, y + m_nGridHeight - 1, m_nMaxPatternSequence * m_nGridWidth, y + m_nGridHeight - 1 );
	}

	//~ celle
	m_bSequenceChanged = true;
}

void SongEditor::cleanUp(){

	delete m_pBackgroundPixmap;
	delete m_pSequencePixmap;
}

void SongEditor::drawSequence()
{
	QPainter p;
	p.begin( m_pSequencePixmap );
	p.drawPixmap( rect(), *m_pBackgroundPixmap, rect() );
	p.end();

	Song* song = Hydrogen::get_instance()->getSong();
	PatternList *patList = song->get_pattern_list();
	vector<PatternList*>* pColumns = song->get_pattern_group_vector();
	uint listLength = patList->get_size();
	for (uint i = 0; i < pColumns->size(); i++) {
		PatternList* pColumn = (*pColumns)[ i ];
		
		std::set<Pattern*> drawnAsVirtual;

		for (uint nPat = 0; nPat < pColumn->get_size(); ++nPat) {
			H2Core::Pattern *pat = pColumn->get( nPat );

			if (drawnAsVirtual.find(pat) == drawnAsVirtual.end()) {
			    int position = -1;
			    // find the position in pattern list
			    for (uint j = 0; j < listLength; j++) {
				    H2Core::Pattern *pat2 = patList->get( j );
				    if (pat == pat2) {
					    position = j;
					    break;
				    }
			    }
			    if (position == -1) {
				    WARNINGLOG( QString("[drawSequence] position == -1, group = %1").arg( i ) );
			    }
			    drawPattern( i, position, false );
			}//if
			
			for (std::set<Pattern*>::const_iterator virtualIter = pat->virtual_pattern_transitive_closure_set.begin(); virtualIter != pat->virtual_pattern_transitive_closure_set.end(); ++virtualIter) {
			    if (drawnAsVirtual.find(*virtualIter) == drawnAsVirtual.end()) {
				int position = -1;
				// find the position in pattern list
				for (uint j = 0; j < listLength; j++) {
				    H2Core::Pattern *pat2 = patList->get( j );
				    if (*virtualIter == pat2) {
					    position = j;
					    break;
				    }//if
				}//for
				if (position == -1) {
				    WARNINGLOG( QString("[drawSequence] position == -1, group = %1").arg( i ) );
				}
				drawPattern( i, position, true );
				
				drawnAsVirtual.insert(*virtualIter);
			    }//if
			}//for
		}
	}

	// Moving cells
	p.begin( m_pSequencePixmap );
//	p.setRasterOp( Qt::XorROP );

// comix: this composition mode seems to be not available on Mac
	p.setCompositionMode( QPainter::CompositionMode_Xor );
	QPen pen( Qt::gray );
	pen.setStyle( Qt::DotLine );
	p.setPen( pen );
	for ( uint i = 0; i < m_movingCells.size(); i++ ) {
		int x = 10 + m_nGridWidth * ( m_movingCells[ i ] ).x();
		int y = m_nGridHeight * ( m_movingCells[ i ] ).y();

		QColor patternColor;
		patternColor.setRgb( 255, 255, 255 );
		p.fillRect( x + 2, y + 4, m_nGridWidth - 3, m_nGridHeight - 7, patternColor );
	}

}



void SongEditor::drawPattern( int pos, int number, bool invertColour )
{
	Preferences *pref = Preferences::get_instance();
	UIStyle *pStyle = pref->getDefaultUIStyle();
	QPainter p( m_pSequencePixmap );
	QColor patternColor( pStyle->m_songEditor_pattern1Color.getRed(), pStyle->m_songEditor_pattern1Color.getGreen(), pStyle->m_songEditor_pattern1Color.getBlue() );
	
	if (true == invertColour) {
	    patternColor = patternColor.darker(200);
	}//if

	bool bIsSelected = false;
	for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
		QPoint point = m_selectedCells[ i ];
		if ( point.x() == pos && point.y() == number ) {
			bIsSelected = true;
			break;
		}
	}

	if ( bIsSelected ) {
		patternColor = patternColor.dark( 130 );
	}

	int x = 10 + m_nGridWidth * pos;
	int y = m_nGridHeight * number;

// 	p.setPen( patternColor.light( 120 ) );  // willie - For the bevel - haven't yet figured how it's supposed to work...
	p.fillRect( x + 1, y + 3, m_nGridWidth - 1, m_nGridHeight - 5, patternColor );
}





// :::::::::::::::::::





SongEditorPatternList::SongEditorPatternList( QWidget *parent )
 : QWidget( parent )
 , Object( "SongEditorPatternList" )
 , EventListener()
 , m_pBackgroundPixmap( NULL )
{
	m_nWidth = 200;
	m_nGridHeight = 18;
	setAttribute(Qt::WA_NoBackground);
	
	setAcceptDrops(true);

	patternBeingEdited = NULL;
	
	line = new QLineEdit( "Inline Pattern Name", this );
	line->setFrame( false );
	line->hide();
	connect( line, SIGNAL(editingFinished()), this, SLOT(inlineEditingFinished()) );
	connect( line, SIGNAL(returnPressed()), this, SLOT(inlineEditingEntered()) );

	this->resize( m_nWidth, m_nInitialHeight );

	m_labelBackgroundLight.load( Skin::getImagePath() + "/songEditor/songEditorLabelBG.png" );
	m_labelBackgroundDark.load( Skin::getImagePath() + "/songEditor/songEditorLabelABG.png" );
	m_labelBackgroundSelected.load( Skin::getImagePath() + "/songEditor/songEditorLabelSBG.png" );
	m_playingPattern_on_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_on.png" );
	m_playingPattern_off_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_off.png" );

	m_pPatternPopup = new QMenu( this );
	m_pPatternPopup->addAction( trUtf8("Edit"),  this, SLOT( patternPopup_edit() ) );
	m_pPatternPopup->addAction( trUtf8("Copy"),  this, SLOT( patternPopup_copy() ) );
	m_pPatternPopup->addAction( trUtf8("Delete"),  this, SLOT( patternPopup_delete() ) );
	m_pPatternPopup->addAction( trUtf8("Fill/Clear ..."),  this, SLOT( patternPopup_fill() ) );
	m_pPatternPopup->addAction( trUtf8("Properties"),  this, SLOT( patternPopup_properties() ) );
	m_pPatternPopup->addAction( trUtf8("Load Pattern"),  this, SLOT( patternPopup_load() ) );
	m_pPatternPopup->addAction( trUtf8("Save Pattern"),  this, SLOT( patternPopup_save() ) );
	m_pPatternPopup->addAction( trUtf8("Virtual Pattern"), this, SLOT( patternPopup_virtualPattern() ) );

	HydrogenApp::get_instance()->addEventListener( this );

	createBackground();
	update();
}



SongEditorPatternList::~SongEditorPatternList()
{
}


void SongEditorPatternList::patternChangedEvent() {

	createBackground();
	update();
	///here we check the timeline  && m_pSong->get_mode() == Song::SONG_MODE
	Hydrogen *engine = Hydrogen::get_instance();
	if ( ( Preferences::get_instance()->__usetimeline ) && ( engine->getSong()->get_mode() == Song::SONG_MODE ) ){
		for ( int i = 0; i < static_cast<int>(engine->m_timelinevector.size()); i++){
			if ( ( engine->m_timelinevector[i].m_htimelinebeat == engine->getPatternPos() )
				&& ( engine->getNewBpmJTM() != engine->m_timelinevector[i].m_htimelinebpm ) ){
				engine->setBPM( engine->m_timelinevector[i].m_htimelinebpm );
			}//if
		}//for
	}//if
}


/// Single click, select the next pattern
void SongEditorPatternList::mousePressEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();

	if ( row >= (int)patternList->get_size() ) {
		return;
	}

	if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::RightButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ){
		togglePattern( row );
	} else {
		engine->setSelectedPatternNumber( row );
		if (ev->button() == Qt::RightButton)  {
	/*
			if ( song->getMode() == Song::PATTERN_MODE ) {
	
				PatternList *pCurrentPatternList = engine->getCurrentPatternList();
				if ( pCurrentPatternList->get_size() == 0 ) {
					// nessun pattern e' attivo. seleziono subito questo.
					pCurrentPatternList->add( patternList->get( row ) );
				}
				else {
					engine->setNextPattern( row );
				}
			}
	*/
			m_pPatternPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
		}
	}

	createBackground();
	update();
}


///
/// Start/stop playing a pattern in "pattern mode"
///
void SongEditorPatternList::togglePattern( int row ) {

	Hydrogen *engine = Hydrogen::get_instance();
/*	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();*/

// 	PatternList *pCurrentPatternList = engine->getCurrentPatternList();

// 	bool isPatternPlaying = false;
	engine->sequencer_setNextPattern( row, false, true );

// 	for ( uint i = 0; i < pCurrentPatternList->get_size(); ++i ) {
// 		if ( pCurrentPatternList->get( i ) == patternList->get( row ) ) {
// 			// the pattern is already playing, stop it!
// 			isPatternPlaying = true;
// 			break;
// 		}
// 	}
//
// 	if ( isPatternPlaying ) {
// 		//pCurrentPatternList->del( patternList->get( row ) );
// 		engine->sequencer_setNextPattern( row, false, true );	// remove from the playing pattern list
// 	}
// 	else {
// 		// the pattern is not playing, add it to the list
// 		//pCurrentPatternList->add( patternList->get( row ) );
// 		engine->sequencer_setNextPattern( row, true, false );	// add to the playing pattern list
// 	}

	createBackground();
	update();
}


void SongEditorPatternList::mouseDoubleClickEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);
	inlineEditPatternName( row );
}

void SongEditorPatternList::inlineEditPatternName( int row )
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();

	if ( row >= (int)patternList->get_size() ) {
		return;
	}
	patternBeingEdited = patternList->get( row );
	line->setGeometry( 23, row * m_nGridHeight , m_nWidth - 23, m_nGridHeight  );
	line->setText( patternBeingEdited->get_name() );
	line->selectAll();
	line->show();
	line->setFocus();
}

void SongEditorPatternList::inlineEditingEntered()
{
	assert( patternBeingEdited != NULL );
	if ( PatternPropertiesDialog::nameCheck( line->text() ) )
	{
		patternBeingEdited->set_name( line->text() );
		Hydrogen::get_instance()->getSong()->__is_modified = true;
		EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
		createBackground();
		update();
	}
// 	patternBeingEdited = NULL;
}


void SongEditorPatternList::inlineEditingFinished()
{
	patternBeingEdited = NULL;
	line->hide();
}


void SongEditorPatternList::paintEvent( QPaintEvent *ev )
{
	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, ev->rect() );
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
	Preferences *pref = Preferences::get_instance();
	UIStyle *pStyle = pref->getDefaultUIStyle();
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );

	QString family = pref->getApplicationFontFamily();
	int size = pref->getApplicationFontPointSize();
	QFont textFont( family, size );

	QFont boldTextFont( textFont);
	boldTextFont.setPointSize(10);
	boldTextFont.setBold( true );

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	int nPatterns = pSong->get_pattern_list()->get_size();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();

	static int oldHeight = -1;
	int newHeight = m_nGridHeight * nPatterns;

	if (oldHeight != newHeight) {
		if (newHeight == 0) {
			newHeight = 1;	// the pixmap should not be empty
		}
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( m_nWidth, newHeight );	// initialize the pixmap
		this->resize( m_nWidth, newHeight );
	}
	m_pBackgroundPixmap->fill( Qt::black );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( boldTextFont );

	for ( int i = 0; i < nPatterns; i++ ) {
		uint y = m_nGridHeight * i;
		if ( i == nSelectedPattern ) {
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

	/// paint the foreground (pattern name etc.)
	for ( int i = 0; i < nPatterns; i++ ) {
		H2Core::Pattern *pPattern = pSong->get_pattern_list()->get(i);
		//uint y = m_nGridHeight * i;

		// Text
		bool bNext = false, bActive = false;
/*		for (uint j = 0; j < pCurrentPatternList->get_size(); j++) {
			if ( pPattern == pCurrentPatternList->get(j) ) {
				bActive = true;
				break;
			}
		}*/
		if ( pCurrentPatternList->index_of( pPattern ) != -1 ) bActive = true;
		if ( pEngine->getNextPatterns()->index_of( pPattern ) != -1 ) bNext = true;

		if ( i == nSelectedPattern ) {
			p.setPen( QColor( 0,0,0 ) );
		}
		else {
			p.setPen( textColor );
		}

		uint text_y = i * m_nGridHeight;
		if ( bNext ) {
			p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_off_Pixmap );
		}
		else if (bActive) {
//			p.drawText( 5, text_y - 1 - m_nGridHeight, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, ">" );
		
			//mark active pattern with triangular
			if( ! pref->patternModePlaysSelected() ){
				p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_on_Pixmap );
			}
		}


		p.drawText( 25, text_y - 1, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, pPattern->get_name() );
	}

}

void SongEditorPatternList::patternPopup_virtualPattern()
{
    Hydrogen *pEngine = Hydrogen::get_instance();
    int nSelectedPattern = pEngine->getSelectedPatternNumber();
    VirtualPatternDialog *dialog = new VirtualPatternDialog( this );
    SongEditorPanel *pSEPanel = HydrogenApp::get_instance()->getSongEditorPanel();
    int tmpselectedpatternpos = pEngine->getSelectedPatternNumber();    

    dialog->patternList->setSortingEnabled(1);
    
    Song *song = pEngine->getSong();
    PatternList *pPatternList = song->get_pattern_list();
    H2Core::Pattern *selectedPattern = pPatternList->get(tmpselectedpatternpos);
    
    std::map<QString, Pattern*> patternNameMap;
    
    int listsize = pPatternList->get_size();    
    for (unsigned int index = 0; index < listsize; ++index) {
	H2Core::Pattern *curPattern = pPatternList->get( index );
	QString patternName = curPattern->get_name();
	
	if (patternName == selectedPattern->get_name()) {
	    continue;
	}//if
	
	patternNameMap[patternName] = curPattern;
	
	QListWidgetItem *newItem = new QListWidgetItem(patternName, dialog->patternList);
	dialog->patternList->insertItem(0, newItem );
	
	if (selectedPattern->virtual_pattern_set.find(curPattern) != selectedPattern->virtual_pattern_set.end()) {
	    dialog->patternList->setItemSelected(newItem, true);
	}//if
    }//for
    
    if ( dialog->exec() == QDialog::Accepted ) {
	selectedPattern->virtual_pattern_set.clear();
	for (unsigned int index = 0; index < listsize-1; ++index) {
	    QListWidgetItem *listItem = dialog->patternList->item(index);
	    if (dialog->patternList->isItemSelected(listItem) == true) {
		if (patternNameMap.find(listItem->text()) != patternNameMap.end()) {
		    selectedPattern->virtual_pattern_set.insert(patternNameMap[listItem->text()]);
		}//if
	    }//if
	}//for
	
	pSEPanel->updateAll();
    }//if
    
    dialog->computeVirtualPatternTransitiveClosure(pPatternList);

    delete dialog;
}//patternPopup_virtualPattern

void SongEditorPatternList::patternPopup_load()
{

	Hydrogen *engine = Hydrogen::get_instance();
	int tmpselectedpatternpos = engine->getSelectedPatternNumber();
	Song *song = engine->getSong();
	PatternList *pPatternList = song->get_pattern_list();
	Instrument *instr = song->get_instrument_list()->get( 0 );
	assert( instr );
	
	QDir dirPattern( Preferences::get_instance()->getDataDirectory() + "/patterns" );
	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Pattern (*.h2pattern)") );
	fd->setDirectory(dirPattern );

	fd->setWindowTitle( trUtf8( "Open Pattern" ) );

	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}
	else
	{
		return;
	}

	LocalFileMng mng;
	LocalFileMng fileMng;
	Pattern* err = fileMng.loadPattern( filename );
	if ( err == 0 ) {
		_ERRORLOG( "Error loading the pattern" );
	}else{
		H2Core::Pattern *pNewPattern = err;
		pPatternList->add( pNewPattern );
		song->__is_modified = true;
		createBackground();
		update();
	}

	int listsize = pPatternList->get_size();
	engine->setSelectedPatternNumber( listsize -1 );
	Pattern *pTemp = pPatternList->get( engine->getSelectedPatternNumber() );
	pPatternList->replace( pPatternList->get( tmpselectedpatternpos ), listsize -1);
	pPatternList->replace( pTemp, tmpselectedpatternpos );
	listsize = pPatternList->get_size();
	engine->setSelectedPatternNumber( listsize -1 );
	patternPopup_delete();
	engine->setSelectedPatternNumber( tmpselectedpatternpos );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();

}



void SongEditorPatternList::patternPopup_save()
{	
	Hydrogen *engine = Hydrogen::get_instance();
	int nSelectedPattern = engine->getSelectedPatternNumber();
	Song *song = engine->getSong();
	Pattern *pat = song->get_pattern_list()->get( nSelectedPattern );

	QString patternname = pat->get_name();

	LocalFileMng fileMng;
	int err = fileMng.savePattern( song , nSelectedPattern, patternname, patternname, 1 );
	if ( err == 1 ) {
		int res = QMessageBox::information( this, "Hydrogen", tr( "The pattern-file exists. \nOverwrite the existing pattern?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if ( res == 0 ) {
			int err2 = fileMng.savePattern( song , nSelectedPattern, patternname, patternname, 3 );
			if( err2 == 1){
				_ERRORLOG( "Error saving the pattern" );
				return;
			} //if err2
		}else{ // res cancel 
			return;
		} //if res	
	} //if err
	

#ifdef WIN32
	Sleep ( 10 );
#else
	usleep ( 10000 );
#endif 
	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
}



void SongEditorPatternList::patternPopup_edit()
{
	HydrogenApp::get_instance()->getPatternEditorPanel()->show();
	HydrogenApp::get_instance()->getPatternEditorPanel()->setFocus();
}



void SongEditorPatternList::patternPopup_properties()
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();

	int nSelectedPattern = engine->getSelectedPatternNumber();
	H2Core::Pattern *pattern = patternList->get( nSelectedPattern );

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog(this, pattern, false);
	if (dialog->exec() == QDialog::Accepted) {
// 		Hydrogen *engine = Hydrogen::get_instance();
// 		Song *song = engine->getSong();
		song->__is_modified = true;
		EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
		createBackground();
		update();
	}
	delete dialog;
	dialog = NULL;
}



void SongEditorPatternList::patternPopup_delete()
{
	Hydrogen *pEngine = Hydrogen::get_instance();

//	int state = engine->getState();
// 	// per ora non lascio possibile la cancellazione del pattern durante l'esecuzione
// 	// da togliere quando correggo il bug
//         if (state == PLAYING) {
//                 QMessageBox::information( this, "Hydrogen", trUtf8("Can't delete the pattern while the audio engine is playing"));
//                 return;
//         }

	if ( pEngine->getSong()->get_mode() == Song::PATTERN_MODE ) {
		pEngine->sequencer_setNextPattern( -1, false, false );	// reimposto il prossimo pattern a NULL, altrimenti viene scelto quello che sto distruggendo ora...
	}

//	pEngine->sequencer_stop();

// "lock engine" I am not sure, but think this is unnecessarily. -wolke-
//	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *song = pEngine->getSong();
	PatternList *pSongPatternList = song->get_pattern_list();

	H2Core::Pattern *pattern = pSongPatternList->get( pEngine->getSelectedPatternNumber() );
	INFOLOG( QString("[patternPopup_delete] Delete pattern: %1 @%2").arg(pattern->get_name()).arg( (long)pattern ) );
	pSongPatternList->del(pattern);

	vector<PatternList*> *patternGroupVect = song->get_pattern_group_vector();

	uint i = 0;
	while (i < patternGroupVect->size() ) {
		PatternList *list = (*patternGroupVect)[i];

		uint j = 0;
		while ( j < list->get_size() ) {
			H2Core::Pattern *pOldPattern = list->get( j );
			if (pOldPattern == pattern ) {
				list->del( j );
				continue;
			}
			j++;
		}
// 		for (uint j = 0; j < list->get_size(); j++) {
// 			Pattern *pOldPattern = list->get( j );
// 			if (pOldPattern == pattern ) {
// 				list->del( j );
// 			}
// 		}

/*		if (list->get_size() == 0 ) {
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
	if ( pSongPatternList->get_size() > 0 ) {
		H2Core::Pattern *pFirstPattern = pSongPatternList->get( 0 );
		list->add( pFirstPattern );
		// Cambio due volte...cosi' il pattern editor viene costretto ad aggiornarsi
		pEngine->setSelectedPatternNumber( -1 );
		pEngine->setSelectedPatternNumber( 0 );
	}
	else {
		// there's no patterns..	
		Pattern *emptyPattern = Pattern::get_empty_pattern();
		emptyPattern->set_name( trUtf8("Pattern 1") );
		emptyPattern->set_category( trUtf8("not_categorized") );
		pSongPatternList->add( emptyPattern );
		pEngine->setSelectedPatternNumber( -1 );
		pEngine->setSelectedPatternNumber( 0 );
	}
	
	for (unsigned int index = 0; index < pSongPatternList->get_size(); ++index) {
	    H2Core::Pattern *curPattern = pSongPatternList->get(index);
	    
	    std::set<Pattern*>::iterator virtIter = curPattern->virtual_pattern_set.find(pattern);
	    if (virtIter != curPattern->virtual_pattern_set.end()) {
		curPattern->virtual_pattern_set.erase(virtIter);
	    }//if
	}//for

	VirtualPatternDialog::computeVirtualPatternTransitiveClosure(pSongPatternList);

	delete pattern;

	song->__is_modified = true;

// "unlock" I am not sure, but think this is unnecessarily. -wolke-
//	AudioEngine::get_instance()->unlock();

	( HydrogenApp::get_instance() )->getSongEditorPanel()->updateAll();
}



void SongEditorPatternList::patternPopup_copy()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	H2Core::Pattern *pPattern = pPatternList->get( nSelectedPattern );

	H2Core::Pattern *pNewPattern = pPattern->copy();
	pPatternList->add( pNewPattern );

	// rename the copied pattern
	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, pNewPattern, true );
	if ( dialog->exec() == QDialog::Accepted ) {
		pSong->__is_modified = true;
		pEngine->setSelectedPatternNumber(pPatternList->get_size() - 1);	// select the last pattern (the copied one)
		if (pSong->get_mode() == Song::PATTERN_MODE) {
			pEngine->sequencer_setNextPattern( pPatternList->get_size() - 1, false, false );	// select the last pattern (the new copied pattern)
		}
	}
	else {
		pPatternList->del( pNewPattern );
		delete pNewPattern;
	}
	delete dialog;

	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
}

void SongEditorPatternList::patternPopup_fill()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	FillRange range;
	PatternFillDialog *dialog = new PatternFillDialog( this, &range );
	SongEditorPanel *pSEPanel = HydrogenApp::get_instance()->getSongEditorPanel();


	// use a PatternFillDialog to get the range and mode data
	if ( dialog->exec() == QDialog::Accepted ) {
		fillRangeWithPattern(&range, nSelectedPattern);
		pSEPanel->updateAll();
	}

	delete dialog;

}


void SongEditorPatternList::fillRangeWithPattern(FillRange* pRange, int nPattern)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	AudioEngine::get_instance()->lock( RIGHT_HERE );


	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	H2Core::Pattern *pPattern = pPatternList->get( nPattern );
	vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();	// E' la lista di "colonne" di pattern
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
		for ( nColumnIndex = 0; pColumn && nColumnIndex < (int)pColumn->get_size(); nColumnIndex++) {

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
			int nSize = pList->get_size();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
				delete pList;
			}
			else {
				break;
			}
		}
	AudioEngine::get_instance()->unlock();


	// Update
	pSong->__is_modified = true;
}


///drag & drop
void SongEditorPatternList::dragEnterEvent(QDragEnterEvent *event)
{
	if ( event->mimeData()->hasFormat("text/plain") ) {
			event->acceptProposedAction();
	}
}


void SongEditorPatternList::dropEvent(QDropEvent *event)
{
	QString sText = event->mimeData()->text();

	if( sText.startsWith("Songs:") || sText.startsWith("move instrument:") ){
		event->acceptProposedAction();
		return;
	}

	if (sText.startsWith("move pattern:")) {
		Hydrogen *engine = Hydrogen::get_instance();
		int nSourcePattern = engine->getSelectedPatternNumber();

		int nTargetPattern = event->pos().y() / m_nGridHeight;

		if ( nSourcePattern == nTargetPattern ) {
			event->acceptProposedAction();
			return;
		}

		movePatternLine( nSourcePattern , nTargetPattern );

		event->acceptProposedAction();
	}else {


		PatternList *pPatternList = Hydrogen::get_instance()->getSong()->get_pattern_list();

		QStringList tokens = sText.split( "::" );
		QString sPatternName = tokens.at( 1 );

		int nTargetPattern = event->pos().y() / m_nGridHeight;

		LocalFileMng mng;
		Pattern* err = mng.loadPattern( sPatternName );
		if ( err == 0 ) {
			_ERRORLOG( "Error loading the pattern" );
		}else{
			H2Core::Pattern *pNewPattern = err;
			pPatternList->add( pNewPattern );

			for (int nPatr = pPatternList->get_size() +1 ; nPatr >= nTargetPattern; nPatr--) {
				H2Core::Pattern *pPattern = pPatternList->get(nPatr - 1);
				pPatternList->replace( pPattern, nPatr );
			}
			pPatternList->replace( pNewPattern, nTargetPattern );

			Hydrogen::get_instance()->getSong()->__is_modified = true;
			createBackground();
			update();
		}
		HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
		event->acceptProposedAction();
		
	}
}



void SongEditorPatternList::movePatternLine( int nSourcePattern , int nTargetPattern )
{
		Hydrogen *engine = Hydrogen::get_instance();

		Song *pSong = engine->getSong();
		PatternList *pPatternList = pSong->get_pattern_list();



		// move instruments...
		H2Core::Pattern *pSourcePattern = pPatternList->get( nSourcePattern );//Instrument *pSourceInstr = pPatternList->get(nSourcePattern);
		if ( nSourcePattern < nTargetPattern) {
			for (int nPatr = nSourcePattern; nPatr < nTargetPattern; nPatr++) {
				H2Core::Pattern *pPattern = pPatternList->get(nPatr + 1);
				pPatternList->replace( pPattern, nPatr );
			}
			pPatternList->replace( pSourcePattern, nTargetPattern );
		}
		else {
			for (int nPatr = nSourcePattern; nPatr >= nTargetPattern; nPatr--) {
				H2Core::Pattern *pPattern = pPatternList->get(nPatr - 1);
				pPatternList->replace( pPattern, nPatr );
			}
			pPatternList->replace( pSourcePattern, nTargetPattern );
		}
		engine->setSelectedPatternNumber( nTargetPattern );
		HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
		
}


void SongEditorPatternList::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	if ( abs(event->pos().y() - __drag_start_position.y()) < (int)m_nGridHeight) {
		return;
	}


	QString sText = QString("move pattern:%1");

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData);
	//drag->setPixmap(iconPixmap);

	pDrag->start( Qt::CopyAction | Qt::MoveAction );

	QWidget::mouseMoveEvent(event);
}

// ::::::::::::::::::::::::::



SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent )
 , Object( "SongEditorPositionRuler" )
 , m_bRightBtnPressed( false )
{
	setAttribute(Qt::WA_NoBackground);

	m_nGridWidth = 16;
	Preferences *pref = Preferences::get_instance();
	m_nMaxPatternSequence = pref->getMaxBars();

	m_nInitialWidth = m_nMaxPatternSequence * 16;

	resize( m_nInitialWidth, m_nHeight );
	setFixedHeight( m_nHeight );

	m_pBackgroundPixmap = new QPixmap( m_nInitialWidth, m_nHeight );	// initialize the pixmap

	createBackground();	// create m_backgroundPixmap pixmap

	// create tick position pixmap
	bool ok = m_tickPositionPixmap.load( Skin::getImagePath() + "/patternEditor/tickPosition.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	update();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updatePosition()));
	m_pTimer->start(200);
}



SongEditorPositionRuler::~SongEditorPositionRuler() {
	m_pTimer->stop();
}


uint SongEditorPositionRuler::getGridWidth()
{
	return m_nGridWidth;
}

void SongEditorPositionRuler::setGridWidth( uint width )
{
	if ( SONG_EDITOR_MIN_GRID_WIDTH <= width && SONG_EDITOR_MAX_GRID_WIDTH >= width )
	{
		m_nGridWidth = width;
		createBackground ();
	}
}


void SongEditorPositionRuler::createBackground()
{
	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );
	QColor textColorAlpha( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue(), 45 );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );

	m_pBackgroundPixmap->fill( backgroundColor );

	Preferences *pref = Preferences::get_instance();
	QString family = pref->getApplicationFontFamily();
	int size = pref->getApplicationFontPointSize();
	QFont font( family, size );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( font );

	p.fillRect( 0, 0, width(), 24, QColor( 67, 72, 83, 105) );
	char tmp[10];
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * m_nGridWidth;	
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinetagvector.size()); t++){
			if ( Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetagbeat == i ) {
				p.setPen( Qt::cyan );
				p.drawText( x - m_nGridWidth / 2 , 12, m_nGridWidth * 2, height() , Qt::AlignCenter, "T");
			}
		}

		if ( (i % 4) == 0 ) {
			p.setPen( textColor );
			sprintf( tmp, "%d", i + 1 );
			p.drawText( x - m_nGridWidth, 12, m_nGridWidth * 2, height(), Qt::AlignCenter, tmp );


		}
		else {
			p.setPen( textColor );
			p.drawLine( x, 32, x, 40 );
		}

	}


//draw tempo content
	if(pref->__usetimeline){
		p.setPen( textColor );
	}else
	{
		p.setPen( textColorAlpha );
	}
	char tempo[10];
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * m_nGridWidth;
		p.drawLine( x, 2, x, 5 );
		p.drawLine( x, 19, x, 20 );
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinevector.size()); t++){
			if ( Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat == i ) {
				sprintf( tempo, "%d",  ((int)Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebpm) );
				p.drawText( x - m_nGridWidth, 3, m_nGridWidth * 2, height() / 2 - 5, Qt::AlignCenter, tempo );
			}
		}
	}

	p.setPen( QColor(35, 39, 51) );
	p.drawLine( 0, 0, width(), 0 );

	p.fillRect ( 0, height() - 27, width(), 1, QColor(35, 39, 51) );
	p.fillRect ( 0, height() - 3, width(), 2, alternateRowColor );

}



void SongEditorPositionRuler::mouseMoveEvent(QMouseEvent *ev)
{
	if ( !m_bRightBtnPressed ) {
		// Click+drag triggers same action as clicking at new position
		mousePressEvent( ev );
	}
	else {
		// Right-click+drag
		int column = (ev->x() / m_nGridWidth);
		Preferences* pPref = Preferences::get_instance();
		if ( column >= (int)Hydrogen::get_instance()->getSong()->get_pattern_group_vector()->size() ) {
			pPref->setPunchOutPos(-1);
			return;
		}
		if ( Hydrogen::get_instance()->getSong()->get_mode() == Song::PATTERN_MODE ) {
			return;
		}
		pPref->setPunchOutPos(column-1);
		update();
	}
}



void SongEditorPositionRuler::mousePressEvent( QMouseEvent *ev )
{

	if (ev->button() == Qt::LeftButton && ev->y() >= 26) {
		int column = (ev->x() / m_nGridWidth);
		m_bRightBtnPressed = false;

		if ( column >= (int)Hydrogen::get_instance()->getSong()->get_pattern_group_vector()->size() ) {
			return;
		}
	
		// disabling son relocates while in pattern mode as it causes weird behaviour. (jakob lund)
		if ( Hydrogen::get_instance()->getSong()->get_mode() == Song::PATTERN_MODE ) {
			return;
		}

		int nPatternPos = Hydrogen::get_instance()->getPatternPos();
		if ( nPatternPos != column ) {
			Hydrogen::get_instance()->setPatternPos( column );
			update();
		}

		//time line test
		Hydrogen::get_instance()->setTimelineBpm();

	}
	else if (ev->button() == Qt::MidButton && ev->y() >= 26) {
		int column = (ev->x() / m_nGridWidth);
		SongEditorPanelTagWidget dialog( this , column );
		if (dialog.exec() == QDialog::Accepted) {
			createBackground();
		}
	}
	else if (ev->button() == Qt::RightButton && ev->y() >= 26) {
		int column = (ev->x() / m_nGridWidth);
		Preferences* pPref = Preferences::get_instance();
		if ( column >= (int)Hydrogen::get_instance()->getSong()->get_pattern_group_vector()->size() ) {
			pPref->unsetPunchArea();
			return;
		}
		if ( Hydrogen::get_instance()->getSong()->get_mode() == Song::PATTERN_MODE ) {
			return;
		}
		m_bRightBtnPressed = true;
		// Disable until mouse is moved
		pPref->setPunchInPos(column);
		pPref->setPunchOutPos(-1);
		update();
	}
	else if( ( ev->button() == Qt::LeftButton || ev->button() == Qt::RightButton ) && ev->y() <= 25 && Preferences::get_instance()->__usetimeline ){
		int column = (ev->x() / m_nGridWidth);
		SongEditorPanelBpmWidget dialog( this , column );
		if (dialog.exec() == QDialog::Accepted) {
			createBackground();
		}
	}

}




void SongEditorPositionRuler::mouseReleaseEvent(QMouseEvent *ev)
{
	UNUSED( ev );
	m_bRightBtnPressed = false;
}


void SongEditorPositionRuler::paintEvent( QPaintEvent *ev )
{
	if (!isVisible()) {
		return;
	}
	
	Hydrogen *H = Hydrogen::get_instance();

	float fPos = H->getPatternPos();
	int pIPos = Preferences::get_instance()->getPunchInPos();
	int pOPos = Preferences::get_instance()->getPunchOutPos();

	if ( H->getCurrentPatternList()->get_size() != 0 ) {
		H2Core::Pattern *pPattern = H->getCurrentPatternList()->get( 0 );

		if (pPattern != NULL){
		    fPos += (float)H->getTickPosition() / (float)pPattern->get_length();
		} else {
		    fPos += (float)H->getTickPosition() / (float)MAX_NOTES;
		}
	}
	else {
		// nessun pattern, uso la grandezza di default
		fPos += (float)H->getTickPosition() / (float)MAX_NOTES;
	}

	if ( H->getSong()->get_mode() == Song::PATTERN_MODE ) {
		fPos = -1;
		pIPos = 0;
		pOPos = -1;
	}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, ev->rect() );

	if (fPos != -1) {
		uint x = (int)( 10 + fPos * m_nGridWidth - 11 / 2 );
		painter.drawPixmap( QRect( x, height() / 2, 11, 8), m_tickPositionPixmap, QRect(0, 0, 11, 8) );
		painter.setPen( QColor(35, 39, 51) );
		painter.drawLine( x + 5 , 8, x +5 , 24 );
	}

	if ( pIPos <= pOPos ) {
		int xIn = (int)( 10 + pIPos * m_nGridWidth );
		int xOut = (int)( 9 + (pOPos+1) * m_nGridWidth );
		painter.fillRect( xIn, 30, xOut-xIn+1, 12, QColor(200, 100, 100, 100) );
		QPen pen(QColor(200, 100, 100));
		painter.setPen(pen);
		painter.drawRect( xIn, 30, xOut-xIn+1, 12 );
	}

}



void SongEditorPositionRuler::updatePosition()
{
	update();
}


