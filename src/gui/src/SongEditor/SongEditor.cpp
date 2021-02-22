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

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine.h>
#include <core/EventQueue.h>
#include <core/Helpers/Files.h>
#include <core/Basics/Instrument.h>
#include <core/LocalFileMng.h>
#include <core/Timeline.h>
#include <core/Helpers/Xml.h>
#include <core/IO/DiskWriterDriver.h>
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

const char* SongEditor::__class_name = "SongEditor";

struct PatternDisplayInfo {
	bool bActive;
	bool bNext;
	QString sPatternName;
};


SongEditor::SongEditor( QWidget *parent, QScrollArea *pScrollView, SongEditorPanel *pSongEditorPanel )
 : QWidget( parent )
 , Object( __class_name )
 , m_bSequenceChanged( true )
 , m_pScrollView( pScrollView )
 , m_pSongEditorPanel( pSongEditorPanel )
 , m_selection( this )
{
	Preferences* pPref = Preferences::get_instance();

	setAttribute(Qt::WA_OpaquePaintEvent);
	setFocusPolicy (Qt::StrongFocus);

	m_nGridWidth = pPref->getSongEditorGridWidth();
	m_nGridHeight = pPref->getSongEditorGridHeight();

	m_nCursorRow = 0;
	m_nCursorColumn = 0;

	m_nMaxPatternSequence = pPref->getMaxBars();
	int m_nInitialWidth = m_nMargin + m_nMaxPatternSequence * m_nGridWidth;
	int m_nInitialHeight = 10;

	this->resize( QSize(m_nInitialWidth, m_nInitialHeight) );

	createBackground();	// create m_backgroundPixmap pixmap

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, &SongEditor::cut );
	m_pPopupMenu->addAction( tr( "&Copy" ), this, &SongEditor::copy );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, &SongEditor::paste );
	m_pPopupMenu->addAction( tr( "&Delete" ), this, &SongEditor::deleteSelection );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &SongEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &SongEditor::selectNone );


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
		this->resize ( m_nMargin + m_nMaxPatternSequence * m_nGridWidth, height() );
	}
}

QPoint SongEditor::xyToColumnRow( QPoint p )
{
	return QPoint( (p.x() - m_nMargin) / (int)m_nGridWidth, p.y() / (int)m_nGridHeight );
}

QPoint SongEditor::columnRowToXy( QPoint p )
{
	return QPoint( m_nMargin + p.x() * m_nGridWidth, p.y() * m_nGridHeight );
}


bool SongEditor::togglePatternActive( int nColumn, int nRow )
{
	HydrogenApp* h2app = HydrogenApp::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if ( nRow >= pPatternList->size() || nRow < 0 || nColumn < 0 ) {
		return true;
	}

	H2Core::Pattern *pPattern = pPatternList->get( nRow );
	assert( pPattern != nullptr );

	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
	if ( nColumn < pColumns->size() ) {
		PatternList *pColumn = ( *pColumns )[ nColumn ];
		unsigned nColumnIndex = pColumn->index( pPattern );

		if ( nColumnIndex != -1 ) {
			// Delete pattern
			h2app->m_pUndoStack->push( new SE_deletePatternAction( nColumn, nRow ) );
			return false;

		} else {
			h2app->m_pUndoStack->push( new SE_addPatternAction( nColumn, nRow ) );
			return true;
		}
	}
	else {
		SE_addPatternAction *action = new SE_addPatternAction( nColumn, nRow ) ;
		h2app->m_pUndoStack->push( action );
		return true;
	}
}

void SongEditor::setPatternActive( int nColumn, int nRow, bool value )
{
	HydrogenApp* h2app = HydrogenApp::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if ( nRow >= pPatternList->size() || nRow < 0 || nColumn < 0 ) {
		return;
	}

	H2Core::Pattern *pPattern = pPatternList->get( nRow );
	assert( pPattern != nullptr );

	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
	if ( nColumn < pColumns->size() ) {
		PatternList *pColumn = ( *pColumns )[ nColumn ];
		unsigned nColumnIndex = pColumn->index( pPattern );

		if ( nColumnIndex != -1 && value == false ) {
			// Delete existing pattern
			h2app->m_pUndoStack->push( new SE_deletePatternAction( nColumn, nRow ) );

		} else if ( nColumnIndex == -1 && value == true ) {
			// Add pattern to  column
			h2app->m_pUndoStack->push( new SE_addPatternAction( nColumn, nRow ) );
		}
	} else if ( value == true ) {
		// Add a new pattern
		SE_addPatternAction *action = new SE_addPatternAction( nColumn, nRow ) ;
		h2app->m_pUndoStack->push( action );
	}

}


void SongEditor::selectAll() {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	std::vector<PatternList*>* pColumns = pHydrogen->getSong()->getPatternGroupVector();
	m_selection.clearSelection();
	for ( int nRow = 0; nRow < pPatternList->size(); nRow++ ) {
		H2Core::Pattern *pPattern = pPatternList->get( nRow );
		for ( int nCol = 0; nCol < pColumns->size(); nCol++ ) {
			PatternList *pColumn = ( *pColumns )[ nCol ];
			for ( uint i = 0; i < pColumn->size(); i++) {
				if ( pColumn->get(i) == pPattern ) { // esiste un pattern in questa posizione
					m_selection.addToSelection( QPoint( nCol, nRow ) );
				}
			}
		}
	}
	m_bSequenceChanged = true;
	update();
}

void SongEditor::selectNone() {
	m_selection.clearSelection();

	m_bSequenceChanged = true;
	update();
}

void SongEditor::deleteSelection() {
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	std::vector< QPoint > addCells, deleteCells, mergeCells;
	for ( QPoint cell : m_selection ) {
		deleteCells.push_back( cell );
	}
	pUndo->push( new SE_modifyPatternCellsAction( addCells, deleteCells, mergeCells,
												  tr( "Delete selected cells" ) ) );
	m_selection.clearSelection();
}


//! Copy a selection of cells to an XML representation in the clipboard
//!
//! * <patternSelection>
//!    * <sourcePosition>
//! * <cellList>
//!    * <cell>
//!    * ...
void SongEditor::copy() {
	XMLDoc doc;
	XMLNode selection = doc.set_root( "patternSelection" );
	XMLNode cellList = selection.createNode( "cellList" );
	XMLNode positionNode = selection.createNode( "sourcePosition" );
	// Top left of selection
	int nMinX, nMinY;
	bool bWrotePattern = false;

	for ( QPoint cell : m_selection ) {
		XMLNode cellNode = cellList.createNode( "cell" );
		cellNode.write_int( "x", cell.x() );
		cellNode.write_int( "y", cell.y() );
		if ( bWrotePattern ) {
			nMinX = std::min( nMinX, cell.x() );
			nMinY = std::min( nMinY, cell.y() );
		} else {
			nMinX = cell.x();
			nMinY = cell.y();
			bWrotePattern = true;
		}
	}
	if ( !bWrotePattern) {
		nMinX = m_nCursorColumn;
		nMinY = m_nCursorRow;
	}
	positionNode.write_int( "column", nMinX );
	positionNode.write_int( "row", nMinY );

	QApplication::clipboard()->setText( doc.toString() );

	// Show the keyboard cursor so the user knows where the insertion point will be
	HydrogenApp::get_instance()->setHideKeyboardCursor( false );
}

void SongEditor::paste() {
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	int nDeltaColumn = 0, nDeltaRow = 0;
	Song *pSong = Hydrogen::get_instance()->getSong();
	int nPatterns = pSong->getPatternList()->size();

	XMLDoc doc;
	if ( ! doc.setContent( QApplication::clipboard()->text() ) ) {
		// Pasted something that's not valid XML.
		return;
	}

	m_selection.clearSelection();
	updateGridCells();

	XMLNode selection = doc.firstChildElement( "patternSelection" );
	if ( ! selection.isNull() ) {
		// Got pattern selection.
		std::vector< QPoint > addCells, deleteCells, mergeCells;

		XMLNode cellList = selection.firstChildElement( "cellList" );
		if ( cellList.isNull() ) {
			return;
		}

		XMLNode positionNode = selection.firstChildElement( "sourcePosition" );

		// If position information is supplied in the selection, use
		// it to adjust the location relative to the current keyboard
		// input cursor.
		if ( !positionNode.isNull() ) {

			nDeltaColumn = m_nCursorColumn - positionNode.read_int( "column", m_nCursorColumn );
			nDeltaRow = m_nCursorRow - positionNode.read_int( "row", m_nCursorRow );
		}

		if ( cellList.hasChildNodes() ) {
			for ( XMLNode cellNode = cellList.firstChildElement( "cell" );
				  ! cellNode.isNull();
				  cellNode = cellNode.nextSiblingElement() ) {
				int nCol = cellNode.read_int( "x", m_nCursorColumn ) + nDeltaColumn;
				int nRow = cellNode.read_int( "y", m_nCursorRow ) + nDeltaRow;
				if ( nCol >= 0 && nRow >= 0 && nRow < nPatterns ) {
					// Paste cells
					QPoint p = QPoint( nCol, nRow );
					if ( m_gridCells.find( p ) == m_gridCells.end() ) {
						// Cell is not active. Activate it.
						addCells.push_back( p );
					} else {
						// Merge cell with existing
						mergeCells.push_back( p );
					}
				}
			}

			pUndo->push( new SE_modifyPatternCellsAction( addCells, deleteCells, mergeCells,
														  tr( "Paste cells" ) ) );
		}
	}
}

void SongEditor::cut() {
	copy();
	deleteSelection();
}

void SongEditor::keyPressEvent( QKeyEvent * ev )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	
	bool bIsSelectionKey = false;
	bool bUnhideCursor = true;

	H2Core::Song::ActionMode actionMode = pHydrogen->getSong()->getActionMode();
		
	if ( actionMode == H2Core::Song::ActionMode::selectMode ) {
		bIsSelectionKey = m_selection.keyPressEvent( ev );
	}

	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	const QPoint centre = QPoint( m_nGridWidth / 2, m_nGridHeight / 2 );
	bool bSelectionKey = false;

	if ( bIsSelectionKey ) {
		// Key was claimed by selection
	} else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete: delete selected pattern cells, or cell at current position
		if ( m_selection.begin() != m_selection.end() ) {
			deleteSelection();
		} else {
			// No selection, delete at the current cursor position
			QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
			pUndo->push( new SE_deletePatternAction( m_nCursorColumn, m_nCursorRow ) );
		}

	} else if ( ev->matches( QKeySequence::MoveToNextChar ) || ( bSelectionKey = ev->matches( QKeySequence::SelectNextChar ) ) ) {
		// ->
		if ( m_nCursorColumn < m_nMaxPatternSequence -1 ) {
			m_nCursorColumn += 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToEndOfLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectEndOfLine ) ) ) {
		// ->|
		m_nCursorColumn = m_nMaxPatternSequence -1;

	} else if ( ev->matches( QKeySequence::MoveToPreviousChar ) || ( bSelectionKey = ev->matches( QKeySequence::SelectPreviousChar ) ) ) {
		// <-
		if ( m_nCursorColumn > 0 ) {
			m_nCursorColumn -= 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToStartOfLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectStartOfLine ) ) ) {
		// |<-
		m_nCursorColumn = 0;

	} else if ( ev->matches( QKeySequence::MoveToNextLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectNextLine ) ) ) {
		if ( m_nCursorRow < pPatternList->size()-1 ) {
			m_nCursorRow += 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToNextPage ) || ( bSelectionKey = ev->matches( QKeySequence::SelectNextPage ) ) ) {
		// Page down, scroll by the number of patterns that fit into the viewport
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		m_nCursorRow += pParent->height() / m_nGridHeight;

		if ( m_nCursorRow >= pPatternList->size() ) {
			m_nCursorRow = pPatternList->size()-1;
		}

	} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) || ( bSelectionKey = ev->matches( QKeySequence::SelectEndOfDocument ) ) ) {
		m_nCursorRow = pPatternList->size() -1;

	} else if ( ev->matches( QKeySequence::MoveToPreviousLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectPreviousLine ) ) ) {
		if ( m_nCursorRow > 0 ) {
			m_nCursorRow -= 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToPreviousPage ) || ( bSelectionKey = ev->matches( QKeySequence::SelectPreviousPage ) ) ) {
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		m_nCursorRow -= pParent->height() / m_nGridHeight;

		if ( m_nCursorRow < 0 ) {
			m_nCursorRow = 0;
		}

	} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) || ( bSelectionKey = ev->matches( QKeySequence::SelectStartOfDocument ) ) ) {
		m_nCursorRow = 0;

	} else if ( ev->matches( QKeySequence::SelectAll ) ) {
		// Key: Ctrl + A: Select all pattern
		bSelectionKey = true;
		bUnhideCursor = false;
		if ( actionMode == H2Core::Song::ActionMode::selectMode ) {
			selectAll();
		}

	} else if ( ev->matches( QKeySequence::Deselect ) ) {
		// Key: Shift + Ctrl + A: deselect any selected cells
		bSelectionKey = true;
		bUnhideCursor = false;
		if ( actionMode == H2Core::Song::ActionMode::selectMode ) {
			selectNone();
			m_bSequenceChanged = false;
		}

	} else if ( ev->matches( QKeySequence::Copy ) ) {
		bUnhideCursor = false;
		copy();

	} else if ( ev->matches( QKeySequence::Paste ) ) {
		bUnhideCursor = false;
		paste();
	} else if ( ev->matches( QKeySequence::Cut ) ) {
		bUnhideCursor = false;
		cut();

	} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Return: Set or clear cell (draw mode), or start/end selection or move (select mode)

		// In DRAW mode, Enter's obvious action is the same as a
		// click - insert or delete pattern.
		togglePatternActive( m_nCursorColumn, m_nCursorRow );

	} else {
		ev->ignore();
		HydrogenApp::get_instance()->setHideKeyboardCursor( true );
		return;
	}
	if ( bUnhideCursor ) {
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
	}

	if ( bSelectionKey ) {
		// If a "select" key movement is used in "draw" mode, it's probably a good idea to go straight into
		// "select" mode.
		if ( actionMode == H2Core::Song::ActionMode::drawMode ) {
			Hydrogen::get_instance()->getSong()->setActionMode( H2Core::Song::ActionMode::selectMode );
		}
		// Any selection key may need a repaint of the selection
		m_bSequenceChanged = true;
	}
	if ( m_selection.isMoving() ) {
		// If a selection is being moved, it will need to be repainted
		m_bSequenceChanged = true;
	}

	QPoint cursorCentre = columnRowToXy( QPoint( m_nCursorColumn, m_nCursorRow ) ) + centre;
	m_pScrollView->ensureVisible( cursorCentre.x(), cursorCentre.y() );
	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	update();
	ev->accept();
}

// Make cursor visible on focus
void SongEditor::focusInEvent( QFocusEvent *ev )
{
	if ( ev->reason() == Qt::TabFocusReason || ev->reason() == Qt::BacktabFocusReason ) {
		QPoint pos = columnRowToXy( QPoint( m_nCursorColumn, m_nCursorRow ))
			+ QPoint( m_nGridWidth / 2, m_nGridHeight / 2 );
		m_pScrollView->ensureVisible( pos.x(), pos.y() );
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
	}
	update();
}


// Implement comparison between QPoints needed for std::set
int operator<( QPoint a, QPoint b ) {
	int nAx = a.x(), nBx = b.x();
	if ( nAx != nBx ) {
		return nAx < nBx;
	} else {
		int nAy = a.y(), nBy = b.y();
		return nAy < nBy;
	}
}

void SongEditor::mousePressEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_currentMousePosition = ev->pos();
	m_bSequenceChanged = true;

	// Update keyboard cursor position
	QPoint p = xyToColumnRow( ev->pos() );
	m_nCursorColumn = p.x();
	m_nCursorRow = p.y();
	HydrogenApp::get_instance()->setHideKeyboardCursor( true );

	if ( Hydrogen::get_instance()->getSong()->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_selection.mousePressEvent( ev );

	} else {
		if ( ev->button() == Qt::LeftButton ) {
			// Start of a drawing gesture. Pick up whether we are painting Active or Inactive cells.
			QPoint p = xyToColumnRow( ev->pos() );
			m_bDrawingActiveCell = togglePatternActive( p.x(), p.y() );
			m_pSongEditorPanel->updatePlaybackTrackIfNecessary();

		} else if ( ev->button() == Qt::RightButton ) {
			m_pPopupMenu->popup( ev->globalPos() );
		}
	}
}


void SongEditor::updateModifiers( QInputEvent *ev )
{
	if ( ev->modifiers() == Qt::ControlModifier ) {
		m_bCopyNotMove = true;
	} else {
		m_bCopyNotMove = false;
	}

	if ( m_selection.isMoving() ) {
		// If a selection is currently being moved, change the cursor
		// appropriately. Selection will change it back after the move
		// is complete (or abandoned)
		if ( m_bCopyNotMove &&  cursor().shape() != Qt::DragCopyCursor ) {
			setCursor( QCursor( Qt::DragCopyCursor ) );
		} else if ( !m_bCopyNotMove && cursor().shape() != Qt::DragMoveCursor ) {
			setCursor( QCursor( Qt::DragMoveCursor ) );
		}
	}

}


void SongEditor::addPattern( int nColumn , int nRow )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	H2Core::Pattern *pPattern = pPatternList->get( nRow );
	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	if ( nColumn < (int)pColumns->size() ) {
		PatternList *pColumn = ( *pColumns )[ nColumn ];
		// ADD PATTERN
		pColumn->add( pPattern );

	} else {
		//we need to add some new columns..
		PatternList *pColumn = new PatternList();
		int nSpaces = nColumn - pColumns->size();

		pColumns->push_back( pColumn );

		for ( int i = 0; i < nSpaces; i++ ) {
			pColumn = new PatternList();
			pColumns->push_back( pColumn );
		}
		pColumn->add( pPattern );
	}
	pSong->setIsModified( true );
	AudioEngine::get_instance()->unlock();
	m_bSequenceChanged = true;
	update();
}


void SongEditor::deletePattern( int nColumn , int nRow )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	H2Core::Pattern *pPattern = pPatternList->get( nRow );
	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();

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
	pSong->setIsModified( true );
	AudioEngine::get_instance()->unlock();
	m_bSequenceChanged = true;
	update();
}


void SongEditor::mouseMoveEvent(QMouseEvent *ev)
{
	updateModifiers( ev );
	m_currentMousePosition = ev->pos();

	if ( Hydrogen::get_instance()->getSong()->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_selection.mouseMoveEvent( ev );
	} else {
		if ( ev->x() < m_nMargin ) {
			return;
		}

		QPoint p = xyToColumnRow( ev->pos() );
		m_nCursorColumn = p.x();
		m_nCursorRow = p.y();
		HydrogenApp::get_instance()->setHideKeyboardCursor( true );

		// Drawing mode: continue drawing over other cells
		setPatternActive( p.x(), p.y(), m_bDrawingActiveCell );
	}
}

void SongEditor::mouseDragStartEvent( QMouseEvent *ev )
{
}

void SongEditor::mouseDragUpdateEvent( QMouseEvent *ev )
{
}

void SongEditor::mouseDragEndEvent( QMouseEvent *ev )
{
	unsetCursor();
}

void SongEditor::selectionMoveEndEvent( QInputEvent *ev )
{
	HydrogenApp *pApp = HydrogenApp::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	int nMaxPattern = pPatternList->size();

	updateModifiers( ev );
	QPoint offset = movingGridOffset();
	if ( offset == QPoint( 0, 0 ) ) {
		return;
	}
	std::vector< QPoint > addCells, deleteCells, mergeCells;

	updateGridCells();

	for ( QPoint cell : m_selection ) {
		// Remove original active cell
		if ( ! m_bCopyNotMove ) {
			deleteCells.push_back( cell );
		}
		QPoint newCell = cell + offset;
		// Place new cell if not already active
		if ( newCell.x() >= 0 && newCell.y() >= 0 && newCell.y() < nMaxPattern ) {
			if ( m_gridCells.find( newCell ) == m_gridCells.end() || m_selection.isSelected( newCell ) ) {
				addCells.push_back( newCell );
			} else {
				// Cell is moved, but merges with existing cell
				mergeCells.push_back( newCell );
			}
		}
	}

	pApp->m_pUndoStack->push( new SE_modifyPatternCellsAction( addCells, deleteCells, mergeCells,
															   (m_bCopyNotMove
																? tr( "Copy selected cells" )
																: tr( "Move selected cells" ) ) ) );
}


void SongEditor::mouseClickEvent( QMouseEvent *ev )
{
	assert( Hydrogen::get_instance()->getSong()->getActionMode() == H2Core::Song::ActionMode::selectMode );
	if ( ev->button() == Qt::LeftButton ) {
		QPoint p = xyToColumnRow( ev->pos() );

		m_selection.clearSelection();
		togglePatternActive( p.x(), p.y() );
		m_bSequenceChanged = true;
		update();

	} else if ( ev->button() == Qt::RightButton ) {
		m_pPopupMenu->popup( ev->globalPos() );
	}
}

void SongEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	if ( Hydrogen::get_instance()->getSong()->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_selection.mouseReleaseEvent( ev );
		return;
	}
}


//! Modify pattern cells by first deleting some, then adding some.
//! deleteCells and addCells *may* safely overlap
void SongEditor::modifyPatternCellsAction( std::vector<QPoint> & addCells, std::vector<QPoint> & deleteCells, std::vector<QPoint> & selectCells ) {

	for ( QPoint cell : deleteCells ) {
		deletePattern( cell.x(), cell.y() );
	}

	m_selection.clearSelection();
	for ( QPoint cell : addCells ) {
		addPattern( cell.x(), cell.y() );
		m_selection.addToSelection( cell );
	}
	// Select additional cells (probably merged cells on redo)
	for ( QPoint cell : selectCells ) {
		m_selection.addToSelection( cell );
	}
}

void SongEditor::updateWidget() {
	bool bCellBoundaryCrossed = xyToColumnRow( m_previousMousePosition ) != xyToColumnRow( m_currentMousePosition );
	// Only update the drawn sequence if necessary. This is only possible when the c
	if ( m_selection.isMoving() ) {
		// Moving a selection never has to update the sequence (it's drawn on top of the sequence). Update
		// is only ever needed when moving across a cell boundary.
		if ( bCellBoundaryCrossed ) {
			update();
		}
	} else if ( m_selection.isLasso() ) {
		// Selection must redraw the pattern when a cell boundary is crossed, as the selected cells are
		// drawn when drawing the pattern.
		if ( bCellBoundaryCrossed ) {
			m_bSequenceChanged = true;
		}
		update();
	} else {
		// Other reasons: force update
		m_bSequenceChanged = true;
		update();
	}
	m_previousMousePosition = m_currentMousePosition;
}


void SongEditor::paintEvent( QPaintEvent *ev )
{

	// ridisegno tutto solo se sono cambiate le note
	if (m_bSequenceChanged) {
		m_bSequenceChanged = false;
		drawSequence();
	}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pSequencePixmap, ev->rect() );

	// Draw moving selected cells
	QColor patternColor( 0, 0, 0 );
	if ( m_selection.isMoving() ) {
		QPoint offset = movingGridOffset();
		for ( QPoint point : m_selection ) {
			int nWidth = m_gridCells[ point ].m_fWidth * m_nGridWidth;
			QRect r = QRect( columnRowToXy( point + offset ),
							 QSize( nWidth, m_nGridHeight ) )
				.marginsRemoved( QMargins( 2, 4, 1 , 3 ) );
			painter.fillRect( r, patternColor );
		}
	}

	// Draw cursor
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() && hasFocus() ) {
		QPen p( Qt::black );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setRenderHint( QPainter::Antialiasing );
		// Aim to leave a visible gap between the border of the
		// pattern cell, and the cursor line, for consistency and
		// visibility.
		painter.drawRoundedRect( QRect( QPoint(0, 1 ) + columnRowToXy( QPoint(m_nCursorColumn, m_nCursorRow ) ),
										QSize( m_nGridWidth+1, m_nGridHeight-1 ) ),
								 4, 4 );
	}

	m_selection.paintSelection( &painter );

}


void SongEditor::createBackground()
{
	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );
	QColor linesColor( pStyle->m_songEditor_lineColor.getRed(), pStyle->m_songEditor_lineColor.getGreen(), pStyle->m_songEditor_lineColor.getBlue() );

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();

	uint nPatterns = pSong->getPatternList()->size();

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
		uint x = m_nMargin + i * m_nGridWidth;
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

// Update the GridCell representation.
void SongEditor::updateGridCells() {

	m_gridCells.clear();
	Song *pSong = Hydrogen::get_instance()->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	std::vector< PatternList* > *pColumns = pSong->getPatternGroupVector();

	for ( int nColumn = 0; nColumn < pColumns->size(); nColumn++ ) {
		PatternList *pColumn = (*pColumns)[nColumn];
		int nMaxLength = pColumn->longest_pattern_length();

		for ( uint nPat = 0; nPat < pColumn->size(); nPat++ ) {
			Pattern *pPattern = (*pColumn)[ nPat ];
			int y = pPatternList->index( pPattern );
			assert( y != -1 );
			GridCell *pCell = &( m_gridCells[ QPoint( nColumn, y ) ] );
			pCell->m_bActive = true;
			pCell->m_fWidth = (float) pPattern->get_length() / nMaxLength;

			for ( Pattern *pVPattern : *( pPattern->get_flattened_virtual_patterns() ) ) {
				GridCell *pVCell = &( m_gridCells[ QPoint( nColumn, pPatternList->index( pVPattern ) ) ] );
				pVCell->m_bDrawnVirtual = true;
				pVCell->m_fWidth = (float) pVPattern->get_length() / nMaxLength;
			}
		}
	}
}

// Return grid offset (in cell coordinate space) of moving selection
QPoint SongEditor::movingGridOffset( ) const {
	QPoint rawOffset = m_selection.movingOffset();
	// Quantize offset to multiples of m_nGrid{Width,Height}
	int x_bias = m_nGridWidth / 2, y_bias = m_nGridHeight / 2;
	if ( rawOffset.y() < 0 ) {
		y_bias = -y_bias;
	}
	if ( rawOffset.x() < 0 ) {
		x_bias = -x_bias;
	}
	int x_off = (rawOffset.x() + x_bias) / (int)m_nGridWidth;
	int y_off = (rawOffset.y() + y_bias) / (int)m_nGridHeight;
	return QPoint( x_off, y_off );
}


void SongEditor::drawSequence()
{
	QPainter p;
	p.begin( m_pSequencePixmap );
	p.drawPixmap( rect(), *m_pBackgroundPixmap, rect() );
	p.end();

	Song* song = Hydrogen::get_instance()->getSong();
	PatternList *patList = song->getPatternList();
	std::vector<PatternList*>* pColumns = song->getPatternGroupVector();
	uint listLength = patList->size();

	updateGridCells();

	// Draw using GridCells representation
	for ( auto it : m_gridCells ) {
		drawPattern( it.first.x(), it.first.y(), it.second.m_bDrawnVirtual, it.second.m_fWidth );
	}
}



void SongEditor::drawPattern( int pos, int number, bool invertColour, double width )
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
	PatternList *patList = song->getPatternList();

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

	bool bIsSelected = m_selection.isSelected( QPoint( pos, number ) );

	if ( bIsSelected ) {
		patternColor = patternColor.darker( 130 );
	}

	int x = m_nMargin + m_nGridWidth * pos;
	int y = m_nGridHeight * number;

	p.fillRect( x + 1, y + 3, width * (m_nGridWidth - 1), m_nGridHeight - 5, patternColor );
}

std::vector<SongEditor::SelectionIndex> SongEditor::elementsIntersecting( QRect r )
{
	std::vector<SelectionIndex> elems;
	for ( auto it : m_gridCells ) {
		if ( r.intersects( QRect( columnRowToXy( it.first ),
								  QSize( m_nGridWidth, m_nGridHeight) ) ) ) {
			if ( ! it.second.m_bDrawnVirtual ) {
				elems.push_back( it.first );
			}
		}
	}
	return elems;
}

QRect SongEditor::getKeyboardCursorRect() {
	return QRect( QPoint( 0, 1 ) + columnRowToXy( QPoint( m_nCursorColumn, m_nCursorRow ) ),
				  QSize( m_nGridWidth, m_nGridHeight -1 ) );
}

void SongEditor::clearThePatternSequenceVector( QString filename )
{
	Hydrogen *engine = Hydrogen::get_instance();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *song = engine->getSong();

	//before deleting the sequence, write a temp sequence file to disk
	song->writeTempPatternList( filename );

	std::vector<PatternList*> *pPatternGroupsVect = song->getPatternGroupVector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		PatternList *pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
		delete pPatternList;
	}
	pPatternGroupsVect->clear();

	song->setIsModified( true );
	AudioEngine::get_instance()->unlock();
	m_bSequenceChanged = true;
	update();
}

void SongEditor::updateEditorandSetTrue()
{
	Hydrogen::get_instance()->getSong()->setIsModified( true );
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
	m_nGridHeight = Preferences::get_instance()->getSongEditorGridHeight();
	setAttribute(Qt::WA_OpaquePaintEvent);

	setAcceptDrops(true);

	m_pPatternBeingEdited = nullptr;

	m_pLineEdit = new QLineEdit( "Inline Pattern Name", this );
	m_pLineEdit->setFrame( false );
	m_pLineEdit->hide();
	m_pLineEdit->setAcceptDrops( false );
	connect( m_pLineEdit, SIGNAL(editingFinished()), this, SLOT(inlineEditingFinished()) );
	connect( m_pLineEdit, SIGNAL(returnPressed()), this, SLOT(inlineEditingEntered()) );

	this->resize( m_nWidth, m_nInitialHeight );

	m_labelBackgroundLight.load( Skin::getImagePath() + "/songEditor/songEditorLabelBG.png" );
	m_labelBackgroundDark.load( Skin::getImagePath() + "/songEditor/songEditorLabelABG.png" );
	m_labelBackgroundSelected.load( Skin::getImagePath() + "/songEditor/songEditorLabelSBG.png" );
	m_playingPattern_on_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_on.png" );
	m_playingPattern_off_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_off.png" );

	m_pPatternPopup = new QMenu( this );
	m_pPatternPopup->addAction( tr("Duplicate"),  this, SLOT( patternPopup_duplicate() ) );
	m_pPatternPopup->addAction( tr("Delete"),  this, SLOT( patternPopup_delete() ) );
	m_pPatternPopup->addAction( tr("Fill/Clear..."),  this, SLOT( patternPopup_fill() ) );
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
	
	///here we check the timeline  && m_pSong->getMode() == Song::SONG_MODE
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	
#ifdef H2CORE_HAVE_JACK
	if ( pHydrogen->haveJackTransport() ) {
		return;
	}
#endif

	// The disk writer runs at it's own pace. Due to the following
	// lines of code the GUI, instead, just sets the speed to 0 BPM.
	auto pDriver = pHydrogen->getAudioOutput();
	if ( pDriver != nullptr ) {
		if ( DiskWriterDriver::class_name() == pDriver->class_name() ) {
			return;
		}
	}
	
	Timeline* pTimeline = pHydrogen->getTimeline();
	if ( ( Preferences::get_instance()->getUseTimelineBpm() ) &&
		 ( pHydrogen->getSong()->getMode() == Song::SONG_MODE ) ){

		float fTimelineBpm = pTimeline->getTempoAtBar( pHydrogen->getPatternPos(), false );

		if ( pHydrogen->getNewBpmJTM() != fTimelineBpm ){
			pHydrogen->setBPM( fTimelineBpm );
		}
	}
}


/// Single click, select the next pattern
void SongEditorPatternList::mousePressEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->getPatternList();

	if ( row >= (int)patternList->size() ) {
		return;
	}

	if ( (ev->button() == Qt::MiddleButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::RightButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ){
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

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->sequencer_setNextPattern( row );
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if ( row >= (int)pPatternList->size() ) {
		return;
	}
	m_pPatternBeingEdited = pPatternList->get( row );
	m_pLineEdit->setGeometry( 23, row * m_nGridHeight , m_nWidth - 23, m_nGridHeight  );
	m_pLineEdit->setText( m_pPatternBeingEdited->get_name() );
	m_pLineEdit->selectAll();
	m_pLineEdit->show();
	m_pLineEdit->setFocus();
}

void SongEditorPatternList::inlineEditingEntered()
{
	assert( m_pPatternBeingEdited != nullptr );
	
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	
	/*
	 * Make sure that the entered pattern name is unique.
	 * If it is not, use an unused pattern name.
	 */
	
	QString patternName = pPatternList->find_unused_pattern_name( m_pLineEdit->text(), m_pPatternBeingEdited );

	int nSelectedPattern = pHydrogen->getSelectedPatternNumber();

	SE_modifyPatternPropertiesAction *action = new SE_modifyPatternPropertiesAction(  m_pPatternBeingEdited->get_name() , m_pPatternBeingEdited->get_info(), m_pPatternBeingEdited->get_category(),
												patternName, m_pPatternBeingEdited->get_info(), m_pPatternBeingEdited->get_category(), nSelectedPattern );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}


void SongEditorPatternList::inlineEditingFinished()
{
	m_pPatternBeingEdited = nullptr;
	m_pLineEdit->hide();
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

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	
	//Do not redraw anything if Export is active.
	//https://github.com/hydrogen-music/hydrogen/issues/857	
	if( pHydrogen->getIsExportSessionActive() ) {
		return;
	}
	
	Song *pSong = pHydrogen->getSong();
	int nPatterns = pSong->getPatternList()->size();
	int nSelectedPattern = pHydrogen->getSelectedPatternNumber();

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
	PatternList *pCurrentPatternList = pHydrogen->getCurrentPatternList();
	
	//assemble the data..
	for ( int i = 0; i < nPatterns; i++ ) {
		H2Core::Pattern *pPattern = pSong->getPatternList()->get(i);

		if ( pCurrentPatternList->index( pPattern ) != -1 ) {
			PatternArray[i].bActive = true;
		} else {
			PatternArray[i].bActive = false;
		}

		if ( pHydrogen->getNextPatterns()->index( pPattern ) != -1 ) {
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	VirtualPatternDialog *dialog = new VirtualPatternDialog( this );
	SongEditorPanel *pSEPanel = HydrogenApp::get_instance()->getSongEditorPanel();
	int tmpselectedpatternpos = pHydrogen->getSelectedPatternNumber();

	dialog->patternList->setSortingEnabled(1);

	Song *song = pHydrogen->getSong();
	PatternList *pPatternList = song->getPatternList();
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
	Pattern *pattern = song->getPatternList()->get( nSelectedPattern );

	QFileDialog fd(this);
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setDirectory( Filesystem::patterns_dir() );
	fd.setWindowTitle( tr( "Open Pattern" ) );

	if (fd.exec() != QDialog::Accepted) {
		return;
	}
	QString patternPath = fd.selectedFiles().first();

	QString prevPatternPath = Files::savePatternTmp( pattern->get_name(), pattern, song, engine->getCurrentDrumkitName() );
	if ( prevPatternPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	Pattern* pNewPattern = Pattern::load_file( afilename, pSong->getInstrumentList() );
	if ( pNewPattern == nullptr ) {
		_ERRORLOG( "Error loading the pattern" );
		return;
	}

	if( !pPatternList->check_name( pNewPattern->get_name() ) ) {
		pNewPattern->set_name( pPatternList->find_unused_pattern_name( pNewPattern->get_name() ) );
	}

	pPatternList->insert( position, pNewPattern );

	pHydrogen->setSelectedPatternNumber( position );
	pSong->setIsModified( true );
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
	Pattern *pattern = song->getPatternList()->get( engine->getSelectedPatternNumber() );

	QString path = Files::savePatternNew( pattern->get_name(), pattern, song, engine->getCurrentDrumkitName() );
	if ( path.isEmpty() ) {
		if ( QMessageBox::information( this, "Hydrogen", tr( "The pattern-file exists. \nOverwrite the existing pattern?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 ) != 0 ) {
			return;
		}
		path = Files::savePatternOver( pattern->get_name(), pattern, song, engine->getCurrentDrumkitName() );
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
	PatternList *patternList = song->getPatternList();

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
	PatternList *patternList = song->getPatternList();
	H2Core::Pattern *pattern = patternList->get( patternNr );
	pattern->set_name( newPatternName );
	pattern->set_info( newPatternInfo );
	pattern->set_category( newPatternCategory );
	song->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
	createBackground();
	update();
}


void SongEditorPatternList::revertPatternPropertiesDialogSettings(QString oldPatternName, QString oldPatternInfo, QString oldPatternCategory, int patternNr)
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->getPatternList();
	H2Core::Pattern *pattern = patternList->get( patternNr );
	pattern->set_name( oldPatternName );
	pattern->set_category( oldPatternCategory );
	song->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
	createBackground();
	update();
}


void SongEditorPatternList::patternPopup_delete()
{

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	int patternPosition = pHydrogen->getSelectedPatternNumber();
	Pattern *pattern = pSong->getPatternList()->get( patternPosition );

	QString patternPath = Files::savePatternTmp( pattern->get_name(), pattern, pSong, pHydrogen->getCurrentDrumkitName() );
	if ( patternPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
		return;
	}
	LocalFileMng fileMng;
	QString sequencePath = Filesystem::tmp_file_path( "SEQ.xml" );
	if ( !pSong->writeTempPatternList( sequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		return;
	}

	SE_deletePatternFromListAction *action = new 	SE_deletePatternFromListAction( patternPath , sequencePath, patternPosition );
	HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
	hydrogenApp->m_pUndoStack->push( action );

}


void SongEditorPatternList::deletePatternFromList( QString patternFilename, QString sequenceFileName, int patternPosition )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong()->getMode() == Song::PATTERN_MODE ) {
		pHydrogen->sequencer_setNextPattern( -1 );
	}

	Song *song = pHydrogen->getSong();
	PatternList *pSongPatternList = song->getPatternList();
	H2Core::Pattern *pattern = pSongPatternList->get( patternPosition );
	INFOLOG( QString("[patternPopup_delete] Delete pattern: %1 @%2").arg(pattern->get_name()).arg( (long long)pattern ) );
	pSongPatternList->del(pattern);

	std::vector<PatternList*> *patternGroupVect = song->getPatternGroupVector();

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

	PatternList *list = pHydrogen->getCurrentPatternList();
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
	
	pHydrogen->setSelectedPatternNumber( -1 );
	pHydrogen->setSelectedPatternNumber( 0 );

	for (unsigned int index = 0; index < pSongPatternList->size(); ++index) {
		H2Core::Pattern *curPattern = pSongPatternList->get(index);

		Pattern::virtual_patterns_cst_it_t it = curPattern->get_virtual_patterns()->find(pattern);
		if (it != curPattern->get_virtual_patterns()->end()) {
		curPattern->virtual_patterns_del(*it);
		}//if
	}//for

	pSongPatternList->flattened_virtual_patterns_compute();

	delete pattern;
	song->setIsModified( true );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();

}

void SongEditorPatternList::restoreDeletedPatternsFromList( QString patternFilename, QString sequenceFileName, int patternPosition )
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *pSong = engine->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	Pattern* pattern = Pattern::load_file( patternFilename, pSong->getInstrumentList() );
	if ( pattern == nullptr ) {
		_ERRORLOG( "Error loading the pattern" );
	}

	pPatternList->insert( patternPosition, pattern );

	pSong->setIsModified( true );
	createBackground();
	engine->setSelectedPatternNumber( patternPosition );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}


void SongEditorPatternList::patternPopup_duplicate()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	int nSelectedPattern = pHydrogen->getSelectedPatternNumber();
	H2Core::Pattern *pPattern = pPatternList->get( nSelectedPattern );

	H2Core::Pattern *pNewPattern = new Pattern( pPattern );
	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, pNewPattern, nSelectedPattern, true );

	if ( dialog->exec() == QDialog::Accepted ) {
		QString filePath = Files::savePatternTmp( pNewPattern->get_name(), pNewPattern, pSong, pHydrogen->getCurrentDrumkitName() );
		if ( filePath.isEmpty() ) {
			QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
			return;
		}
		SE_duplicatePatternAction *action = new SE_duplicatePatternAction( filePath, nSelectedPattern + 1 );
		HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
		hydrogenApp->m_pUndoStack->push( action );
	}

	delete dialog;
	delete pNewPattern;

	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
}


void SongEditorPatternList::patternPopup_duplicateAction( QString patternFilename, int patternposition )
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *pSong = engine->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	Pattern* pattern = Pattern::load_file( patternFilename, pSong->getInstrumentList() );
	if ( pattern == nullptr ) {
		_ERRORLOG( "Error loading the pattern" );
		return;
	}

	pPatternList->insert( patternposition, pattern );
	engine->setSelectedPatternNumber( patternposition );
	pSong->setIsModified( true );
	createBackground();
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}

void SongEditorPatternList::patternPopup_fill()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nSelectedPattern = pHydrogen->getSelectedPatternNumber();
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	H2Core::Pattern *pPattern = pPatternList->get( nPattern );
	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();	// E' la lista di "colonne" di pattern
	PatternList *pColumn = nullptr;

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
		
		assert( pColumn );

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
	pSong->setIsModified( true );
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	
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
		int nSourcePattern = pHydrogen->getSelectedPatternNumber();

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
		PatternList *pPatternList = pSong->getPatternList();
		QList<QUrl> urlList = mimeData->urls();

		int successfullyAddedPattern = 0;
		
		for (int i = 0; i < urlList.size(); i++)
		{
			QString patternFilePath = urlList.at(i).toLocalFile();
			if( patternFilePath.endsWith(".h2pattern") )
			{
				Pattern* pPattern = Pattern::load_file( patternFilePath, pSong->getInstrumentList() );
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
		Pattern *pPattern = pSong->getPatternList()->get( nTargetPattern );
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
		PatternList *pPatternList = pSong->getPatternList();



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
		pSong->setIsModified( true );
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

void SongEditorPatternList::timelineUpdateEvent( int nEvent ){
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
	Hydrogen::get_instance()->getSong()->setIsModified( true );
}

// ::::::::::::::::::::::::::

const char* SongEditorPositionRuler::__class_name = "SongEditorPositionRuler";

SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent )
 , Object( __class_name )
 , m_bRightBtnPressed( false )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

	Preferences *pPref = Preferences::get_instance();

	m_nGridWidth = pPref->getSongEditorGridWidth();
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
	auto tagVector = pTimeline->getAllTags();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	
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
		uint x = m_nMargin + i * m_nGridWidth;
		for ( int t = 0; t < static_cast<int>(tagVector.size()); t++){
			if ( tagVector[t]->nBar == i ) {
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
		uint x = m_nMargin + i * m_nGridWidth;
		p.drawLine( x, 2, x, 5 );
		p.drawLine( x, 19, x, 20 );
		for ( int t = 0; t < static_cast<int>(tempoMarkerVector.size()); t++){
			if ( tempoMarkerVector[t]->nBar == i ) {
				sprintf( tempo, "%d",  ((int)tempoMarkerVector[t]->fBpm) );
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
		
		if ( column > (int)Hydrogen::get_instance()->getSong()->getPatternGroupVector()->size() ) {
			pPref->setPunchOutPos(-1);
			return;
		}
		if ( Hydrogen::get_instance()->getSong()->getMode() == Song::PATTERN_MODE ) {
			return;
		}
		pPref->setPunchOutPos(column-1);
		update();
	}
}



void SongEditorPositionRuler::mousePressEvent( QMouseEvent *ev )
{
	auto pHydrogen = Hydrogen::get_instance();
	
	if (ev->button() == Qt::LeftButton && ev->y() >= 26) {
		int column = (ev->x() / m_nGridWidth);
		m_bRightBtnPressed = false;

		if ( column > (int)pHydrogen->getSong()->getPatternGroupVector()->size() ) {
			return;
		}

		// disabling son relocates while in pattern mode as it causes weird behaviour. (jakob lund)
		if ( pHydrogen->getSong()->getMode() == Song::PATTERN_MODE ) {
			return;
		}

		int nPatternPos = pHydrogen->getPatternPos();
		if ( nPatternPos != column ) {
			WARNINGLOG( "relocate via mouse click" );
			
			pHydrogen->getCoreActionController()->relocate( column );
			update();
		}
		
	} else if (ev->button() == Qt::MiddleButton && ev->y() >= 26) {
		int column = (ev->x() / m_nGridWidth);
		SongEditorPanelTagWidget dialog( this , column );
		if (dialog.exec() == QDialog::Accepted) {
			//createBackground();
		}
	} else if (ev->button() == Qt::RightButton && ev->y() >= 26) {
		int column = (ev->x() / m_nGridWidth);
		Preferences* pPref = Preferences::get_instance();
		if ( column >= (int)pHydrogen->getSong()->getPatternGroupVector()->size() ) {
			pPref->unsetPunchArea();
			return;
		}
		if ( pHydrogen->getSong()->getMode() == Song::PATTERN_MODE ) {
			return;
		}
		m_bRightBtnPressed = true;
		// Disable until mouse is moved
		pPref->setPunchInPos(column);
		pPref->setPunchOutPos(-1);
		update();
	} else if( ( ev->button() == Qt::LeftButton || ev->button() == Qt::RightButton ) && ev->y() <= 25 && Preferences::get_instance()->getUseTimelineBpm() ){
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

	Hydrogen *pHydrogen = Hydrogen::get_instance();

	float fPos = pHydrogen->getPatternPos();
	int pIPos = Preferences::get_instance()->getPunchInPos();
	int pOPos = Preferences::get_instance()->getPunchOutPos();

	AudioEngine *pAudioEngine = AudioEngine::get_instance();
	pAudioEngine->lock( RIGHT_HERE );

	if ( pHydrogen->getCurrentPatternList()->size() != 0 ) {
		int nLength = pHydrogen->getCurrentPatternList()->longest_pattern_length();
		fPos += (float)pHydrogen->getTickPosition() / (float)nLength;
	}
	else {
		// nessun pattern, uso la grandezza di default
		fPos += (float)pHydrogen->getTickPosition() / (float)MAX_NOTES;
	}

	if ( pHydrogen->getSong()->getMode() == Song::PATTERN_MODE ) {
		fPos = -1;
		pIPos = 0;
		pOPos = -1;
	}

	pAudioEngine->unlock();

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
		uint x = (int)( m_nMargin + fPos * m_nGridWidth - 11 / 2 );
		painter.drawPixmap( QRect( x, height() / 2, 11, 8), m_tickPositionPixmap, QRect(0, 0, 11, 8) );
		painter.setPen( QColor(35, 39, 51) );
		painter.drawLine( x + 5 , 8, x +5 , 24 );
	}

	if ( pIPos <= pOPos ) {
		int xIn = (int)( m_nMargin + pIPos * m_nGridWidth );
		int xOut = (int)( 9 + (pOPos+1) * m_nGridWidth );
		painter.fillRect( xIn, 30, xOut-xIn+1, 12, QColor(200, 100, 100, 100) );
		QPen pen(QColor(200, 100, 100));
		painter.setPen(pen);
		painter.drawRect( xIn, 30, xOut-xIn+1, 12 );
	}

}



void SongEditorPositionRuler::updatePosition()
{
	HydrogenApp::get_instance()->getSongEditorPanel()->updateTimelineUsage();
	update();
}


void SongEditorPositionRuler::editTimeLineAction( int nNewPosition, float fNewBpm )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	pHydrogen->getTimeline()->deleteTempoMarker( nNewPosition - 1 );
	pHydrogen->getTimeline()->addTempoMarker( nNewPosition - 1, fNewBpm );
	createBackground();
}

void SongEditorPositionRuler::deleteTimeLinePosition( int nPosition )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getTimeline()->deleteTempoMarker( nPosition - 1 );
	createBackground();
}


void SongEditorPositionRuler::editTagAction( QString text, int position, QString textToReplace)
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Timeline* pTimeline = pHydrogen->getTimeline();

	const QString sTag = pTimeline->getTagAtBar( position, false );
	if ( sTag == textToReplace ) {
		pTimeline->deleteTag( position );
		pTimeline->addTag( position, text );
	}
	
	createBackground();
}

void SongEditorPositionRuler::deleteTagAction( QString text, int position )
{

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Timeline* pTimeline = pHydrogen->getTimeline();

	const QString sTag = pTimeline->getTagAtBar( position, false );
	if ( sTag == text ) {
		pTimeline->deleteTag( position );
	}
	
	createBackground();
}
