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

#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/helpers/files.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/timeline.h>
using namespace H2Core;

#include "UndoActions.h"
#include "MainForm.h"
#include "SongEditor.h"
#include "SongEditorPanel.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanelTagWidget.h"
#include "PatternFillDialog.h"
#include "VirtualPatternDialog.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SoundLibrary/SoundLibraryDatastructures.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../Widgets/Button.h"
#include "../PatternPropertiesDialog.h"
#include "../SongPropertiesDialog.h"
#include "../Skin.h"




#ifdef WIN32
#include <time.h>
#include <windows.h>
#endif

using namespace std;

const char* SongEditor::__class_name = "SongEditor";

struct PatternDisplayInfo {
	bool bActive;
	bool bNext;
	QString sPatternName;
};


SongEditorGridRepresentationItem::SongEditorGridRepresentationItem(int x, int y, bool value)
{
	this->x = x;
	this->y = y;
	this->value = value;
}


SongEditor::SongEditor( QWidget *parent )
 : QWidget( parent )
 , Object( __class_name )
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

	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)m_nGridWidth;

	if ( ev->modifiers() == Qt::ControlModifier ) {
		INFOLOG( "[mousePressEvent] CTRL pressed!" );
		m_bIsCtrlPressed = true;
	}
	else {
		m_bIsCtrlPressed = false;
	}

	HydrogenApp* h2app = HydrogenApp::get_instance();
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
		PatternList *pPatternList = pSong->get_pattern_list();

	// don't lock the audio driver before checking that...
	if ( nRow >= (int)pPatternList->size() || nRow < 0 || nColumn < 0 ) { return; }


	SongEditorActionMode actionMode = HydrogenApp::get_instance()->getSongEditorPanel()->getActionMode();
	if ( actionMode == SELECT_ACTION ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
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
		AudioEngine::get_instance()->unlock();
		// update
		m_bSequenceChanged = true;
		update();
	}
	else if ( actionMode == DRAW_ACTION ) {
		H2Core::Pattern *pPattern = pPatternList->get( nRow );
		vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();	// E' la lista di "colonne" di pattern
		if ( nColumn < (int)pColumns->size() ) {
			PatternList *pColumn = ( *pColumns )[ nColumn ];

			bool bFound = false;
			unsigned nColumnIndex = 0;
			for ( nColumnIndex = 0; nColumnIndex < pColumn->size(); nColumnIndex++) {
				if ( pColumn->get( nColumnIndex ) == pPattern ) { // il pattern e' gia presente
					bFound = true;
					break;
				}
			}

			if ( bFound ) {//Delete pattern
				SE_deletePatternAction *action = new SE_deletePatternAction( nColumn, nRow ) ;
				h2app->m_pUndoStack->push( action );

			}
			else {
				if ( nColumn < (int)pColumns->size() ) {
					SE_addPatternAction *action = new SE_addPatternAction( nColumn, nRow ) ;
					h2app->m_pUndoStack->push( action );
				}
			}
		}
		else {
			SE_addPatternAction *action = new SE_addPatternAction( nColumn, nRow ) ;
			h2app->m_pUndoStack->push( action );
		}
	}

	h2app->getSongEditorPanel()->updatePlaybackTrackIfNecessary();
}


void SongEditor::addPattern( int nColumn , int nRow )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	H2Core::Pattern *pPattern = pPatternList->get( nRow );
	vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	if ( nColumn < (int)pColumns->size() ) {
		PatternList *pColumn = ( *pColumns )[ nColumn ];
		// ADD PATTERN
		m_selectedCells.clear();
		pColumn->add( pPattern );

	} else {
		//we need to add some new columns..
		PatternList *pColumn = new PatternList();
		m_selectedCells.clear();
		int nSpaces = nColumn - pColumns->size();

		pColumns->push_back( pColumn );

		for ( int i = 0; i < nSpaces; i++ ) {
			pColumn = new PatternList();
			pColumns->push_back( pColumn );
		}
		pColumn->add( pPattern );
	}
	pSong->set_is_modified( true );
	AudioEngine::get_instance()->unlock();
	m_bSequenceChanged = true;
	update();
}


void SongEditor::deletePattern( int nColumn , int nRow )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	H2Core::Pattern *pPattern = pPatternList->get( nRow );
	vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	PatternList *pColumn = ( *pColumns )[ nColumn ];
	pColumn->del( pPattern );

	// elimino le colonne vuote
	for ( int i = pColumns->size() - 1; i >= 0; i-- ) {
		PatternList *pColumn = ( *pColumns )[ i ];
		if ( pColumn->size() == 0 ) {
			pColumns->erase( pColumns->begin() + i );
			delete pColumn;
		}
		else {
			break;
		}
	}
	pSong->set_is_modified( true );
	AudioEngine::get_instance()->unlock();
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
				if ( nRow >= (int)pPatternList->size() || nRow < 0 || nCol < 0 ) {
					return;
				}
				H2Core::Pattern *pPattern = pPatternList->get( nRow );

				if ( nCol < (int)pColumns->size() ) {
					PatternList *pColumn = ( *pColumns )[ nCol ];

					for ( uint i = 0; i < pColumn->size(); i++) {
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
	UNUSED(ev);

	if ( m_bIsMoving ) {	// fine dello spostamento dei pattern
		// create the new patterns

		/*
		 * For the proper handling of undo events we have to make sure
		 * that the array m_movingCells does not include cells that are
		 * already existing.
		 *
		 * Example: A song consists of a sequence with the cells 0,1 and 3.
		 * Consider that we the two first cells 0 and 1 get moved one
		 * cell to the right (to 2,3). An undo action would now delete
		 * (2,3) for and re-create 0,1. Cell 3 got deleted now, but it existed
		 * before the first move operation.
		 */

		SongEditorGridRepresentationItem* item;
		m_existingCells.clear();
		for ( uint i = 0; i < m_movingCells.size(); i++ )
		{
			QPoint cell = m_movingCells[ i ];

			//looking for cell identified with (cell.x/cell.y) in the gridRepresentation
			bool found = false;
			foreach(item, gridRepresentation)
			{
				if(item->x == cell.x() && item->y == cell.y())
				{
					found = true;
				}
			}

			if( found ){
				m_existingCells.push_back(cell);
			}
		}

		SE_movePatternCellAction *action = new SE_movePatternCellAction( m_movingCells, m_selectedCells , m_existingCells, m_bIsCtrlPressed);
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}

	setCursor( QCursor( Qt::ArrowCursor ) );

	m_bShowLasso = false;
	m_bSequenceChanged = true;
	m_bIsCtrlPressed = false;
	update();
}

/**
 * @brief moves or copies a cell which represents a pattern
 * @param movingCells Target cells for move/copy action
 * @param selectedCells Currently selected cells for move/copy action
 * @param existingCells Cells which are included in selected/movingCells but where existing before(import for undo).
 * @param bIsCtrlPressed If ctrl is pressed, do copy instead of move
 * @param undo	Determine if this is an undo-operation
 */

void SongEditor::movePatternCellAction( std::vector<QPoint> movingCells, std::vector<QPoint> selectedCells, std::vector<QPoint> existingCells,  bool bIsCtrlPressed, bool undo )
{
	Hydrogen *pEngine = Hydrogen::get_instance();

	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	vector<PatternList*>* pColumns = pEngine->getSong()->get_pattern_group_vector();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	//create the new patterns
	for ( uint i = 0; i < movingCells.size(); i++ ) {
		QPoint cell = movingCells[ i ];
		if ( cell.x() < 0 || cell.y() < 0 || cell.y() >= (int)pPatternList->size() ) {
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

	if ( bIsCtrlPressed) //Copy
	{
				if( undo )
		{
			// remove the old patterns
			for ( uint i = 0; i < selectedCells.size(); i++ ) {
				QPoint cell = selectedCells[ i ];

				bool existing = false;
				for ( uint i = 0; i < existingCells.size(); i++ ) {
					QPoint existing_cell = existingCells[ i ];
					if(existing_cell.x() == cell.x() && existing_cell.y() == cell.y()) existing = true;
				}

				//this cell existed before. Don't delete it!
				if(existing){
					continue;
				}



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
	}
	else {	// MOVE
		// remove the old patterns
		for ( uint i = 0; i < selectedCells.size(); i++ ) {
			QPoint cell = selectedCells[ i ];
			PatternList* pColumn = NULL;

			/*
			 * Check first if pattern was present in movingCells.
			 * If it was, don't delete it!
			 */

			bool moved = false;
			for ( uint i = 0; i < movingCells.size(); i++ ) {
				QPoint cell2 = movingCells[ i ];
				if( cell.x() == cell2.x() && cell.y() == cell2.y() ){
					moved = true;
				}
			}

			if( moved )
			{
				continue;
			}

			if( undo )
			{
				bool existing = false;
				for ( uint i = 0; i < existingCells.size(); i++ ) {
					QPoint existing_cell = existingCells[ i ];
					if(existing_cell.x() == cell.x() && existing_cell.y() == cell.y()) existing = true;
				}

				//this cell existed before. Don't delete it!
				if(existing){
					continue;
				}
			}


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
		int nSize = pList->size();
		if ( nSize == 0 ) {
			pColumns->erase( pColumns->begin() + i );
			delete pList;
		}
		else {
			break;
		}
	}

	pEngine->getSong()->set_is_modified( true );
	AudioEngine::get_instance()->unlock();

	m_bIsMoving = false;
	m_movingCells.clear();
	m_selectedCells.clear();
	m_bSequenceChanged = true;
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

	uint nPatterns = pSong->get_pattern_list()->size();

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
	uint listLength = patList->size();

	//Drawing the pattern based on the gridRepresentation array

	while (!gridRepresentation.isEmpty())
		delete gridRepresentation.takeFirst();



	for (uint i = 0; i < pColumns->size(); i++) {
		PatternList* pColumn = (*pColumns)[ i ];

		std::set<Pattern*> drawnAsVirtual;

		for (uint nPat = 0; nPat < pColumn->size(); ++nPat) {
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
				//normal pattern

				gridRepresentation.append(new SongEditorGridRepresentationItem(i,position,false));
			}//if

			for ( Pattern::virtual_patterns_cst_it_t it = pat->get_flattened_virtual_patterns()->begin(); it != pat->get_flattened_virtual_patterns()->end(); ++it) {
				if (drawnAsVirtual.find(*it) == drawnAsVirtual.end()) {
					int position = patList->index(*it);
					if (position == -1) {
						WARNINGLOG( QString("[drawSequence] position == -1, group = %1").arg( i ) );
					}
					//virtual pattern
					gridRepresentation.append(new SongEditorGridRepresentationItem(i,position,true));
					drawnAsVirtual.insert(*it);
				}
			}
		}
	}


	//Draw the patterns according to the gridRepresentation
	SongEditorGridRepresentationItem* s;
	foreach(s, gridRepresentation)
	{
		drawPattern( s->x, s->y, s->value);
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

	/*
	 * The following color modes are available:
	 *
	 * Fixed: One color. Argument: specified color
	 * Steps: User defined number of steps.
	 * Automatic: Steps = Number of pattern in song
	 */

	int coloringMethod = pref->getColoringMethod();
	int coloringMethodAuxValue = pref->getColoringMethodAuxValue();
	int steps = 1;

	/*
	 * This coloring of the song editor "squares" is done using the hsv color model,
	 * see http://qt-project.org/doc/qt-4.8/qcolor.html#the-hsv-color-model for details.
	 *
	 * The default color of the cubes in rgb is 97,167,251.
	 * The hsv equivalent is 213,156,249.
	 */
	int hue = 213;

	Song* song = Hydrogen::get_instance()->getSong();
	PatternList *patList = song->get_pattern_list();

	switch(coloringMethod)
	{
		case 0:
			//Automatic
			steps = patList->size();

			if(steps == 0)
			{
				//beware of the division by zero..
				steps = 1;
			}

			hue = ((number % steps) * (300 / steps) + 213) % 300;
			patternColor.setHsv( hue , 156 , 249);
			break;
		case 1:
			//Steps
			steps = coloringMethodAuxValue;
			hue = ((number % steps) * (300 / steps) + 213) % 300;
			patternColor.setHsv( hue , 156, 249);
			break;
		case 2:
			//Fixed color
			hue = coloringMethodAuxValue;
			patternColor.setHsv( hue , 156, 249);
			break;
	}



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
		patternColor = patternColor.darker( 130 );
	}

	int x = 10 + m_nGridWidth * pos;
	int y = m_nGridHeight * number;

	p.fillRect( x + 1, y + 3, m_nGridWidth - 1, m_nGridHeight - 5, patternColor );
}


void SongEditor::clearThePatternSequenceVector( QString filename )
{
	Hydrogen *engine = Hydrogen::get_instance();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *song = engine->getSong();

	//before deleting the sequence, write a temp sequence file to disk
	LocalFileMng fileMng;
	bool success = song->writeTempPatternList( filename );

	vector<PatternList*> *pPatternGroupsVect = song->get_pattern_group_vector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		PatternList *pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
		delete pPatternList;
	}
	pPatternGroupsVect->clear();

	song->set_is_modified( true );
	AudioEngine::get_instance()->unlock();
	m_bSequenceChanged = true;
	update();
}

void SongEditor::updateEditorandSetTrue()
{
	Hydrogen::get_instance()->getSong()->set_is_modified( true );
	m_bSequenceChanged = true;
	update();
}
// :::::::::::::::::::


const char* SongEditorPatternList::__class_name = "SongEditorPatternList";

SongEditorPatternList::SongEditorPatternList( QWidget *parent )
 : QWidget( parent )
 , Object( __class_name )
 , EventListener()
 , m_pBackgroundPixmap( nullptr )
{
	m_nWidth = 200;
	m_nGridHeight = 18;
	setAttribute(Qt::WA_NoBackground);

	setAcceptDrops(true);

	patternBeingEdited = nullptr;

	line = new QLineEdit( "Inline Pattern Name", this );
	line->setFrame( false );
	line->hide();
	line->setAcceptDrops( false );
	connect( line, SIGNAL(editingFinished()), this, SLOT(inlineEditingFinished()) );
	connect( line, SIGNAL(returnPressed()), this, SLOT(inlineEditingEntered()) );

	this->resize( m_nWidth, m_nInitialHeight );

	m_labelBackgroundLight.load( Skin::getImagePath() + "/songEditor/songEditorLabelBG.png" );
	m_labelBackgroundDark.load( Skin::getImagePath() + "/songEditor/songEditorLabelABG.png" );
	m_labelBackgroundSelected.load( Skin::getImagePath() + "/songEditor/songEditorLabelSBG.png" );
	m_playingPattern_on_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_on.png" );
	m_playingPattern_off_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_off.png" );

	m_pPatternPopup = new QMenu( this );
	m_pPatternPopup->addAction( tr("Copy"),  this, SLOT( patternPopup_copy() ) );
	m_pPatternPopup->addAction( tr("Delete"),  this, SLOT( patternPopup_delete() ) );
	m_pPatternPopup->addAction( tr("Fill/Clear ..."),  this, SLOT( patternPopup_fill() ) );
	m_pPatternPopup->addAction( tr("Properties"),  this, SLOT( patternPopup_properties() ) );
	m_pPatternPopup->addAction( tr("Load Pattern"),  this, SLOT( patternPopup_load() ) );
	m_pPatternPopup->addAction( tr("Save Pattern"),  this, SLOT( patternPopup_save() ) );
	m_pPatternPopup->addAction( tr("Export Pattern"),  this, SLOT( patternPopup_export() ) );
	m_pPatternPopup->addAction( tr("Virtual Pattern"), this, SLOT( patternPopup_virtualPattern() ) );

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
	Timeline *pTimeline = engine->getTimeline();
		if ( ( Preferences::get_instance()->getUseTimelineBpm() ) && ( engine->getSong()->get_mode() == Song::SONG_MODE ) ){
		for ( int i = 0; i < static_cast<int>(pTimeline->m_timelinevector.size()); i++){
			if ( ( pTimeline->m_timelinevector[i].m_htimelinebeat == engine->getPatternPos() )
				&& ( engine->getNewBpmJTM() != pTimeline->m_timelinevector[i].m_htimelinebpm ) ){
				engine->setBPM( pTimeline->m_timelinevector[i].m_htimelinebpm );
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

	if ( row >= (int)patternList->size() ) {
		return;
	}

	if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::RightButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ){
		togglePattern( row );
	} else {
		engine->setSelectedPatternNumber( row );
		if (ev->button() == Qt::RightButton)  {
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

	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->sequencer_setNextPattern( row );
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
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	if ( row >= (int)pPatternList->size() ) {
		return;
	}
	patternBeingEdited = pPatternList->get( row );
	line->setGeometry( 23, row * m_nGridHeight , m_nWidth - 23, m_nGridHeight  );
	line->setText( patternBeingEdited->get_name() );
	line->selectAll();
	line->show();
	line->setFocus();
}

void SongEditorPatternList::inlineEditingEntered()
{
	assert( patternBeingEdited != nullptr );
	
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	
	/*
	 * Make sure that the entered pattern name is unique.
	 * If it is not, use an unused patten name.
	 */
	
	QString patternName = pPatternList->find_unused_pattern_name( line->text() );

	int nSelectedPattern = pEngine->getSelectedPatternNumber();

	SE_modifyPatternPropertiesAction *action = new SE_modifyPatternPropertiesAction(  patternBeingEdited->get_name() , patternBeingEdited->get_info(), patternBeingEdited->get_category(),
												patternName, patternBeingEdited->get_info(), patternBeingEdited->get_category(), nSelectedPattern );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}


void SongEditorPatternList::inlineEditingFinished()
{
	patternBeingEdited = nullptr;
	line->hide();
}


void SongEditorPatternList::paintEvent( QPaintEvent *ev )
{
	QPainter painter(this);
	qreal pixelRatio = devicePixelRatio();
	QRectF srcRect(
			pixelRatio * ev->rect().x(),
			pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(),
			pixelRatio * ev->rect().height()
	);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, srcRect );
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
	int nPatterns = pSong->get_pattern_list()->size();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();

	static int oldHeight = -1;
	int newHeight = m_nGridHeight * nPatterns;

	if (oldHeight != newHeight) {
		if (newHeight == 0) {
			newHeight = 1;	// the pixmap should not be empty
		}
		delete m_pBackgroundPixmap;
		qreal pixelRatio = devicePixelRatio();
		m_pBackgroundPixmap = new QPixmap( m_nWidth  * pixelRatio , newHeight * pixelRatio );	// initialize the pixmap
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
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

	std::unique_ptr<PatternDisplayInfo[]> PatternArray{new PatternDisplayInfo[nPatterns]};

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();
	
	//assemble the data..
	for ( int i = 0; i < nPatterns; i++ ) {
		H2Core::Pattern *pPattern = pSong->get_pattern_list()->get(i);

		if ( pCurrentPatternList->index( pPattern ) != -1 ) {
			PatternArray[i].bActive = true;
		} else {
			PatternArray[i].bActive = false;
		}

		if ( pEngine->getNextPatterns()->index( pPattern ) != -1 ) {
			PatternArray[i].bNext = true;
		} else {
			PatternArray[i].bNext = false;
		}

		PatternArray[i].sPatternName = pPattern->get_name();
	}
	AudioEngine::get_instance()->unlock();
	
	/// paint the foreground (pattern name etc.)
	for ( int i = 0; i < nPatterns; i++ ) {
		if ( i == nSelectedPattern ) {
			p.setPen( QColor( 0,0,0 ) );
		}
		else {
			p.setPen( textColor );
		}

		uint text_y = i * m_nGridHeight;
		if ( PatternArray[i].bNext ) {
			p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_off_Pixmap );
		}
		else if (PatternArray[i].bActive) {
			//mark active pattern with triangular
			if( ! pref->patternModePlaysSelected() ){
				p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_on_Pixmap );
			}
		}

		p.drawText( 25, text_y - 1, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, PatternArray[i].sPatternName);
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

	int listsize = pPatternList->size();
	for (unsigned int index = 0; index < listsize; ++index) {
		H2Core::Pattern *curPattern = pPatternList->get( index );
		QString patternName = curPattern->get_name();

		if (patternName == selectedPattern->get_name()) {
			continue;
		}//if

		patternNameMap[patternName] = curPattern;

		QListWidgetItem *newItem = new QListWidgetItem(patternName, dialog->patternList);
		dialog->patternList->insertItem(0, newItem );

		if (selectedPattern->get_virtual_patterns()->find(curPattern) != selectedPattern->get_virtual_patterns()->end()) {
			newItem->setSelected( true );
		}//if
	}//for

	if ( dialog->exec() == QDialog::Accepted ) {
		selectedPattern->virtual_patterns_clear();
		for (unsigned int index = 0; index < listsize-1; ++index) {
			QListWidgetItem *listItem = dialog->patternList->item(index);
			if (listItem->isSelected() == true) {
				if (patternNameMap.find(listItem->text()) != patternNameMap.end()) {
					selectedPattern->virtual_patterns_add(patternNameMap[listItem->text()]);
				}//if
			}//if
		}//for

		pSEPanel->updateAll();
	}//if

	pPatternList->flattened_virtual_patterns_compute();

	delete dialog;
}//patternPopup_virtualPattern



void SongEditorPatternList::patternPopup_load()
{
	Hydrogen *engine = Hydrogen::get_instance();
	int nSelectedPattern = engine->getSelectedPatternNumber();
	Song *song = engine->getSong();
	Pattern *pattern = song->get_pattern_list()->get( nSelectedPattern );

	QFileDialog fd(this);
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setDirectory( Filesystem::patterns_dir() );
	fd.setWindowTitle( tr( "Open Pattern" ) );

	if (fd.exec() != QDialog::Accepted) {
		return;
	}
	QString patternPath = fd.selectedFiles().first();

	QString prevPatternPath = Files::savePatternTmp( pattern->get_name(), pattern, song, engine->getCurrentDrumkitname() );
	if ( prevPatternPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
		return;
	}
	LocalFileMng fileMng;
	QString sequencePath = Filesystem::tmp_file_path( "SEQ.xml" );
	if ( !song->writeTempPatternList( sequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		return;
	}

	SE_loadPatternAction *action = new SE_loadPatternAction( patternPath, prevPatternPath, sequencePath, nSelectedPattern, false );
	HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
	hydrogenApp->m_pUndoStack->push( action );
}

void SongEditorPatternList::loadPatternAction( QString afilename, int position)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	Pattern* pNewPattern = Pattern::load_file( afilename, pSong->get_instrument_list() );
	if ( pNewPattern == nullptr ) {
		_ERRORLOG( "Error loading the pattern" );
		return;
	}

	if( !pPatternList->check_name( pNewPattern->get_name() ) ) {
		pNewPattern->set_name( pPatternList->find_unused_pattern_name( pNewPattern->get_name() ) );
	}

	pPatternList->insert( position, pNewPattern );

	pEngine->setSelectedPatternNumber( position );
	pSong->set_is_modified( true );
	createBackground();
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
}


void SongEditorPatternList::patternPopup_export()
{
	HydrogenApp::get_instance()->getMainForm()->action_file_export_pattern_as();
	return;
}

void SongEditorPatternList::patternPopup_save()
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	Pattern *pattern = song->get_pattern_list()->get( engine->getSelectedPatternNumber() );

	QString path = Files::savePatternNew( pattern->get_name(), pattern, song, engine->getCurrentDrumkitname() );
	if ( path.isEmpty() ) {
		if ( QMessageBox::information( this, "Hydrogen", tr( "The pattern-file exists. \nOverwrite the existing pattern?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 ) != 0 ) {
			return;
		}
		path = Files::savePatternOver( pattern->get_name(), pattern, song, engine->getCurrentDrumkitname() );
	}

	if ( path.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
		return;
	}

	HydrogenApp::get_instance()->setStatusBarMessage( tr( "Pattern saved." ), 10000 );

	SoundLibraryDatabase::get_instance()->updatePatterns();
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

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog(this, pattern, nSelectedPattern, false);
	dialog->exec();
	delete dialog;
	dialog = nullptr;
}


void SongEditorPatternList::acceptPatternPropertiesDialogSettings(QString newPatternName, QString newPatternInfo, QString newPatternCategory, int patternNr)
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();
	H2Core::Pattern *pattern = patternList->get( patternNr );
	pattern->set_name( newPatternName );
	pattern->set_info( newPatternInfo );
	pattern->set_category( newPatternCategory );
	song->set_is_modified( true );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
	createBackground();
	update();
}


void SongEditorPatternList::revertPatternPropertiesDialogSettings(QString oldPatternName, QString oldPatternInfo, QString oldPatternCategory, int patternNr)
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();
	H2Core::Pattern *pattern = patternList->get( patternNr );
	pattern->set_name( oldPatternName );
	pattern->set_category( oldPatternCategory );
	song->set_is_modified( true );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
	createBackground();
	update();
}


void SongEditorPatternList::patternPopup_delete()
{

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	PatternList *pSongPatternList = song->get_pattern_list();
	int patternPosition = pEngine->getSelectedPatternNumber();
	Pattern *pattern = song->get_pattern_list()->get( patternPosition );

	QString patternPath = Files::savePatternTmp( pattern->get_name(), pattern, song, pEngine->getCurrentDrumkitname() );
	if ( patternPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
		return;
	}
	LocalFileMng fileMng;
	QString sequencePath = Filesystem::tmp_file_path( "SEQ.xml" );
	if ( !song->writeTempPatternList( sequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		return;
	}

	SE_deletePatternFromListAction *action = new 	SE_deletePatternFromListAction( patternPath , sequencePath, patternPosition );
	HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
	hydrogenApp->m_pUndoStack->push( action );

}


void SongEditorPatternList::deletePatternFromList( QString patternFilename, QString sequenceFileName, int patternPosition )
{
	Hydrogen *pEngine = Hydrogen::get_instance();

	if ( pEngine->getSong()->get_mode() == Song::PATTERN_MODE ) {
		pEngine->sequencer_setNextPattern( -1 );
	}

	Song *song = pEngine->getSong();
	PatternList *pSongPatternList = song->get_pattern_list();
	H2Core::Pattern *pattern = pSongPatternList->get( patternPosition );
	INFOLOG( QString("[patternPopup_delete] Delete pattern: %1 @%2").arg(pattern->get_name()).arg( (long long)pattern ) );
	pSongPatternList->del(pattern);

	vector<PatternList*> *patternGroupVect = song->get_pattern_group_vector();

	uint i = 0;
	while (i < patternGroupVect->size() ) {
		PatternList *list = (*patternGroupVect)[i];

		uint j = 0;
		while ( j < list->size() ) {
			H2Core::Pattern *pOldPattern = list->get( j );
			if (pOldPattern == pattern ) {
				list->del( j );
				continue;
			}
			j++;
		}
		i++;

	}

	//Lock because PatternList will be modified
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	PatternList *list = pEngine->getCurrentPatternList();
	list->del( pattern );
	// se esiste, seleziono il primo pattern
	if ( pSongPatternList->size() > 0 ) {
		H2Core::Pattern *pFirstPattern = pSongPatternList->get( 0 );
		list->add( pFirstPattern );	}
	else {
		// there's no patterns..
		Pattern *pEmptyPattern = new Pattern();
		pEmptyPattern->set_name( tr("Pattern 1") );
		pEmptyPattern->set_category( tr("not_categorized") );
		pSongPatternList->add( pEmptyPattern );
	}

	AudioEngine::get_instance()->unlock();
	
	pEngine->setSelectedPatternNumber( -1 );
	pEngine->setSelectedPatternNumber( 0 );

	for (unsigned int index = 0; index < pSongPatternList->size(); ++index) {
		H2Core::Pattern *curPattern = pSongPatternList->get(index);

		Pattern::virtual_patterns_cst_it_t it = curPattern->get_virtual_patterns()->find(pattern);
		if (it != curPattern->get_virtual_patterns()->end()) {
		curPattern->virtual_patterns_del(*it);
		}//if
	}//for

	pSongPatternList->flattened_virtual_patterns_compute();

	delete pattern;
	song->set_is_modified( true );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();

}

void SongEditorPatternList::restoreDeletedPatternsFromList( QString patternFilename, QString sequenceFileName, int patternPosition )
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *pSong = engine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	Pattern* pattern = Pattern::load_file( patternFilename, pSong->get_instrument_list() );
	if ( pattern == nullptr ) {
		_ERRORLOG( "Error loading the pattern" );
	}

	pPatternList->insert( patternPosition, pattern );

	pSong->set_is_modified( true );
	createBackground();
	engine->setSelectedPatternNumber( patternPosition );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}


void SongEditorPatternList::patternPopup_copy()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	H2Core::Pattern *pPattern = pPatternList->get( nSelectedPattern );

	H2Core::Pattern *pNewPattern = new Pattern( pPattern );
	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, pNewPattern, nSelectedPattern, true );

	if ( dialog->exec() == QDialog::Accepted ) {
		QString filePath = Files::savePatternTmp( pNewPattern->get_name(), pNewPattern, pSong, pEngine->getCurrentDrumkitname() );
		if ( filePath.isEmpty() ) {
			QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
			return;
		}
		SE_copyPatternAction *action = new SE_copyPatternAction( filePath, nSelectedPattern + 1 );
		HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
		hydrogenApp->m_pUndoStack->push( action );
	}

	delete dialog;
	delete pNewPattern;

	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
}


void SongEditorPatternList::patternPopup_copyAction( QString patternFilename, int patternposition )
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *pSong = engine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	Pattern* pattern = Pattern::load_file( patternFilename, pSong->get_instrument_list() );
	if ( pattern == nullptr ) {
		_ERRORLOG( "Error loading the pattern" );
		return;
	}

	pPatternList->insert( patternposition, pattern );
	engine->setSelectedPatternNumber( patternposition );
	pSong->set_is_modified( true );
	createBackground();
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}

void SongEditorPatternList::patternPopup_fill()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	FillRange range;
	PatternFillDialog *dialog = new PatternFillDialog( this, &range );

	// use a PatternFillDialog to get the range and mode data
	if ( dialog->exec() == QDialog::Accepted ) {

		SE_fillRangePatternAction *action = new SE_fillRangePatternAction( &range, nSelectedPattern );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}

	delete dialog;

}


void SongEditorPatternList::fillRangeWithPattern( FillRange* pRange, int nPattern )
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
		for ( nColumnIndex = 0; pColumn && nColumnIndex < (int)pColumn->size(); nColumnIndex++) {

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
			int nSize = pList->size();
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
	pSong->set_is_modified( true );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
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
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	
	QString sText = event->mimeData()->text();
	const QMimeData* mimeData = event->mimeData();
	
	int nTargetPattern = 0;
	if(m_nGridHeight > 0)
	{
		nTargetPattern = event->pos().y() / m_nGridHeight;
	}
	
	if( sText.startsWith("Songs:") || sText.startsWith("move instrument:") || sText.startsWith("importInstrument:")){
		event->acceptProposedAction();
		return;
	}
	
	if ( sText.startsWith("move pattern:") ) {
		int nSourcePattern = pEngine->getSelectedPatternNumber();

		if ( nSourcePattern == nTargetPattern ) {
			event->acceptProposedAction();
			return;
		}

		SE_movePatternListItemAction *action = new SE_movePatternListItemAction( nSourcePattern , nTargetPattern ) ;
		HydrogenApp::get_instance()->m_pUndoStack->push( action );

		event->acceptProposedAction();
	} 
	else if( sText.startsWith("file://") && mimeData->hasUrls() )
	{
		//Dragging a file from an external file manager
		PatternList *pPatternList = pSong->get_pattern_list();
		QList<QUrl> urlList = mimeData->urls();

		int successfullyAddedPattern = 0;
		
		for (int i = 0; i < urlList.size(); i++)
		{
			QString patternFilePath = urlList.at(i).toLocalFile();
			if( patternFilePath.endsWith(".h2pattern") )
			{
				Pattern* pPattern = Pattern::load_file( patternFilePath, pSong->get_instrument_list() );
				if ( pPattern)
				{
					H2Core::Pattern *pNewPattern = pPattern;
			
					if(!pPatternList->check_name( pNewPattern->get_name() ) ){
						pNewPattern->set_name( pPatternList->find_unused_pattern_name( pNewPattern->get_name() ) );
					}
					
					SE_insertPatternAction* pInsertPatternAction = new SE_insertPatternAction( nTargetPattern + successfullyAddedPattern, pNewPattern );
					HydrogenApp::get_instance()->m_pUndoStack->push( pInsertPatternAction );
					
					successfullyAddedPattern++;
				}
				else
				{
					ERRORLOG( QString("Error loading pattern %1").arg(patternFilePath) );
				}
			}
		}
	} 
	else 
	{
		QStringList tokens = sText.split( "::" );
		QString sPatternName = tokens.at( 1 );

		//create a unique sequencefilename
		Pattern *pPattern = pSong->get_pattern_list()->get( nTargetPattern );
		HydrogenApp *pHydrogenApp = HydrogenApp::get_instance();

		QString oldPatternName = pPattern->get_name();

		QString sequenceFilename = Filesystem::tmp_file_path( "SEQ.xml" );
		bool drag = false;
		if( QString( tokens.at(0) ).contains( "drag pattern" )) drag = true;
		SE_loadPatternAction *pAction = new SE_loadPatternAction( sPatternName, oldPatternName, sequenceFilename, nTargetPattern, drag );

		pHydrogenApp->m_pUndoStack->push( pAction );
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
				pPatternList->replace( nPatr, pPattern );
			}
			pPatternList->replace( nTargetPattern, pSourcePattern );
		}
		else {
			for (int nPatr = nSourcePattern; nPatr >= nTargetPattern; nPatr--) {
				H2Core::Pattern *pPattern = pPatternList->get(nPatr - 1);
				pPatternList->replace( nPatr, pPattern );
			}
			pPatternList->replace( nTargetPattern, pSourcePattern );
		}
		engine->setSelectedPatternNumber( nTargetPattern );
		HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
		pSong->set_is_modified( true );
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

	pDrag->exec( Qt::CopyAction | Qt::MoveAction );

	QWidget::mouseMoveEvent(event);
}

// ::::::::::::::::::::::::::

const char* SongEditorPositionRuler::__class_name = "SongEditorPositionRuler";

SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent )
 , Object( __class_name )
 , m_bRightBtnPressed( false )
{
	setAttribute(Qt::WA_NoBackground);

	m_nGridWidth = 16;
	Preferences *pPref = Preferences::get_instance();
	m_nMaxPatternSequence = pPref->getMaxBars();

	m_nInitialWidth = m_nMaxPatternSequence * 16;

	resize( m_nInitialWidth, m_nHeight );
	setFixedHeight( m_nHeight );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nInitialWidth * pixelRatio, m_nHeight * pixelRatio );	// initialize the pixmap
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );

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
	Preferences *pPref = Preferences::get_instance();
	Timeline * pTimeline = Hydrogen::get_instance()->getTimeline();
	
	UIStyle *pStyle = pPref->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );
	QColor textColorAlpha( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue(), 45 );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );

	m_pBackgroundPixmap->fill( backgroundColor );

	QString family = pPref->getApplicationFontFamily();
	int size = pPref->getApplicationFontPointSize();
	QFont font( family, size );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( font );

	p.fillRect( 0, 0, width(), 24, QColor( 67, 72, 83, 105) );
	char tmp[10];
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * m_nGridWidth;
		for ( int t = 0; t < static_cast<int>(pTimeline->m_timelinetagvector.size()); t++){
			if ( pTimeline->m_timelinetagvector[t].m_htimelinetagbeat == i ) {
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
	if(pPref->getUseTimelineBpm()){
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
		for ( int t = 0; t < static_cast<int>(pTimeline->m_timelinevector.size()); t++){
			if ( pTimeline->m_timelinevector[t].m_htimelinebeat == i ) {
				sprintf( tempo, "%d",  ((int)pTimeline->m_timelinevector[t].m_htimelinebpm) );
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
		
		if ( column > (int)Hydrogen::get_instance()->getSong()->get_pattern_group_vector()->size() ) {
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

		if ( column > (int)Hydrogen::get_instance()->getSong()->get_pattern_group_vector()->size() ) {
			return;
		}

		// disabling son relocates while in pattern mode as it causes weird behaviour. (jakob lund)
		if ( Hydrogen::get_instance()->getSong()->get_mode() == Song::PATTERN_MODE ) {
			return;
		}

		int nPatternPos = Hydrogen::get_instance()->getPatternPos();
		if ( nPatternPos != column ) {
			WARNINGLOG( "relocate via mouse click" );
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
			//createBackground();
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
		else if( ( ev->button() == Qt::LeftButton || ev->button() == Qt::RightButton ) && ev->y() <= 25 && Preferences::get_instance()->getUseTimelineBpm() ){
		int column = (ev->x() / m_nGridWidth);
		SongEditorPanelBpmWidget dialog( this , column );
		if (dialog.exec() == QDialog::Accepted) {
			//createBackground();
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

	Hydrogen *pEngine = Hydrogen::get_instance();

	float fPos = pEngine->getPatternPos();
	int pIPos = Preferences::get_instance()->getPunchInPos();
	int pOPos = Preferences::get_instance()->getPunchOutPos();

	if ( pEngine->getCurrentPatternList()->size() != 0 ) {
		H2Core::Pattern *pPattern = pEngine->getCurrentPatternList()->get( 0 );

		if (pPattern != nullptr){
			fPos += (float)pEngine->getTickPosition() / (float)pPattern->get_length();
		} else {
			fPos += (float)pEngine->getTickPosition() / (float)MAX_NOTES;
		}
	}
	else {
		// nessun pattern, uso la grandezza di default
		fPos += (float)pEngine->getTickPosition() / (float)MAX_NOTES;
	}

	if ( pEngine->getSong()->get_mode() == Song::PATTERN_MODE ) {
		fPos = -1;
		pIPos = 0;
		pOPos = -1;
	}

	QPainter painter(this);
	qreal pixelRatio = devicePixelRatio();
	QRectF srcRect(
			pixelRatio * ev->rect().x(),
			pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(),
			pixelRatio * ev->rect().height()
	);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, srcRect );

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


void SongEditorPositionRuler::editTimeLineAction( int newPosition, float newBpm )
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	Timeline* pTimeline = pEngine->getTimeline();

	//erase the value to set the new value
	if( pTimeline->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < pTimeline->m_timelinevector.size(); t++){
			if ( pTimeline->m_timelinevector[t].m_htimelinebeat == newPosition -1 ) {
				pTimeline->m_timelinevector.erase( pTimeline->m_timelinevector.begin() +  t);
			}
		}
	}

	Timeline::HTimelineVector tlvector;

	tlvector.m_htimelinebeat = newPosition -1 ;

	if( newBpm < 30.0 ) newBpm = 30.0;
	if( newBpm > 500.0 ) newBpm = 500.0;
	tlvector.m_htimelinebpm = newBpm;
	pTimeline->m_timelinevector.push_back( tlvector );
	pTimeline->sortTimelineVector();
	createBackground();
}



void SongEditorPositionRuler::deleteTimeLinePosition( int position )
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	Timeline* pTimeline = pEngine->getTimeline();

	//erase the value to set the new value
	if( pTimeline->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < pTimeline->m_timelinevector.size(); t++){
			if ( pTimeline->m_timelinevector[t].m_htimelinebeat == position -1 ) {
				pTimeline->m_timelinevector.erase( pTimeline->m_timelinevector.begin() +  t);
			}
		}
	}
	createBackground();
}


void SongEditorPositionRuler::editTagAction( QString text, int position, QString textToReplace)
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	Timeline* pTimeline = pEngine->getTimeline();

	//check vector for old entries and remove them.
	for( int i = 0; i < pTimeline->m_timelinetagvector.size(); ++i ){
		if( ( pTimeline->m_timelinetagvector[i].m_htimelinetag == textToReplace ) &&
			( pTimeline->m_timelinetagvector[i].m_htimelinetagbeat == position ) ){

			pTimeline->m_timelinetagvector.erase( pTimeline->m_timelinetagvector.begin() + i );
			break;
		}
	}
	
	Timeline::HTimelineTagVector tlvector;
	tlvector.m_htimelinetagbeat = position;
	tlvector.m_htimelinetag = text;
	pTimeline->m_timelinetagvector.push_back( tlvector );
	pTimeline->sortTimelineTagVector();
	createBackground();
}

void SongEditorPositionRuler::deleteTagAction( QString text, int position )
{

	Hydrogen* pEngine = Hydrogen::get_instance();
	Timeline* pTimeline = pEngine->getTimeline();

	for( int i = 0; i < pTimeline->m_timelinetagvector.size(); ++i ){
		if( ( pTimeline->m_timelinetagvector[i].m_htimelinetag == text ) &&
			( pTimeline->m_timelinetagvector[i].m_htimelinetagbeat == position ) ){

			pTimeline->m_timelinetagvector.erase( pTimeline->m_timelinetagvector.begin() + i );
			break;
		}
	}
	
	pTimeline->sortTimelineTagVector();
	createBackground();
}
