/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <assert.h>
#include <algorithm>
#include <memory>

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/EventQueue.h>
#include <core/Helpers/Files.h>
#include <core/Basics/Instrument.h>
#include <core/Helpers/Xml.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
using namespace H2Core;

#include "UndoActions.h"
#include "MainForm.h"
#include "SongEditor.h"
#include "SongEditorPanel.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanelTagWidget.h"
#include "PatternFillDialog.h"
#include "PlaybackTrackWaveDisplay.h"
#include "VirtualPatternDialog.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../InstrumentRack.h"
#include "../PatternPropertiesDialog.h"
#include "../SongPropertiesDialog.h"
#include "../Skin.h"




#ifdef WIN32
#include <time.h>
#include <windows.h>
#endif

struct PatternDisplayInfo {
	bool bActive;
	bool bNext;
	QString sPatternName;
};


SongEditor::SongEditor( QWidget *parent, QScrollArea *pScrollView, SongEditorPanel *pSongEditorPanel )
 : QWidget( parent )
 , m_bSequenceChanged( true )
 , m_pScrollView( pScrollView )
 , m_pSongEditorPanel( pSongEditorPanel )
 , m_selection( this )
 , m_pHydrogen( nullptr )
 , m_pAudioEngine( nullptr )
 , m_bEntered( false )
 , m_pBackgroundPixmap( nullptr )
 , m_pSequencePixmap( nullptr )
{
	m_pHydrogen = Hydrogen::get_instance();
	m_pAudioEngine = m_pHydrogen->getAudioEngine();
	
	Preferences* pPref = Preferences::get_instance();
	m_nMaxPatternColors = pPref->getMaxPatternColors(); // no need to
														// update this one.

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &SongEditor::onPreferencesChanged );
	connect( m_pScrollView->verticalScrollBar(), SIGNAL( valueChanged( int ) ), this, SLOT( scrolled( int ) ) );
	connect( m_pScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ), this, SLOT( scrolled( int ) ) );

	setAttribute(Qt::WA_OpaquePaintEvent);
	setFocusPolicy (Qt::StrongFocus);

	m_nGridWidth = pPref->getSongEditorGridWidth();
	m_nGridHeight = pPref->getSongEditorGridHeight();

	m_nCursorRow = 0;
	m_nCursorColumn = 0;

	int nInitialWidth = SongEditor::nMargin + pPref->getMaxBars() * m_nGridWidth;
	int nInitialHeight = 10;

	this->resize( QSize( nInitialWidth, nInitialHeight ) );

	createBackground();	// create m_backgroundPixmap pixmap

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, &SongEditor::cut );
	m_pPopupMenu->addAction( tr( "&Copy" ), this, &SongEditor::copy );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, &SongEditor::paste );
	m_pPopupMenu->addAction( tr( "&Delete" ), this, &SongEditor::deleteSelection );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &SongEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &SongEditor::selectNone );
	m_pPopupMenu->setObjectName( "SongEditorPopup" );

	HydrogenApp::get_instance()->addEventListener( this );
}



SongEditor::~SongEditor()
{
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
	if ( m_pSequencePixmap ) {
		delete m_pSequencePixmap;
	}
}


/// Calculate a target Y scroll value for tracking a playing song
///
/// Songs with many patterns may not fit in the current viewport of the Song Editor. Depending on how the song
/// is structured, as the viewport scrolls to show different times, the playing patterns may end up being
/// off-screen. It would be ideal to be able to follow the progression of the song in a meaningful and useful
/// way, but since multiple patterns may be active at any one time, it's non-trivial to define a useful
/// behaviour that captures more than the simple case of a song with one pattern active at a time.
///
/// As an attempt to define a useful behaviour which captures what the user might expect to happen, we define
/// the behaviour as follows:
///   * If there are no currently playing patterns which are entirely visible:
///       * Find the position with the smallest amount of scrolling from the current location which:
///           * Fits the maximum number of currently playing patterns in view at the same time.
///
/// This covers the trivial cases where only a single pattern is playing, and gives some intuitive behaviour
/// for songs containing multiple playing patterns where the general progression is diagonal but with constant
/// (or near-constant) background elements, and the "minimum scrolling" allows the user to hint if we stray
/// off the path.
///
int SongEditor::yScrollTarget( QScrollArea *pScrollArea, int *pnPatternInView )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	const int nScroll = pScrollArea->verticalScrollBar()->value();
	int nHeight = pScrollArea->height();

	auto pPlayingPatterns = m_pAudioEngine->getPlayingPatterns();

	// If no patterns are playing, no scrolling needed either.
	if ( pPlayingPatterns->size() == 0 ) {
		return nScroll;
	}

	m_pAudioEngine->lock( RIGHT_HERE );

	PatternList *pSongPatterns = pHydrogen->getSong()->getPatternList();

	// Duplicate the playing patterns vector before finding the pattern numbers of the playing patterns. This
	// avoids doing a linear search in the critical section.
	std::vector<Pattern *> currentPatterns;
	for ( int ii = 0; ii < pPlayingPatterns->size(); ++ii ) {
		currentPatterns.push_back( pPlayingPatterns->get( ii ) );
	}
	m_pAudioEngine->unlock();

	std::vector<int> playingRows;
	for ( Pattern *pPattern : currentPatterns ) {
		playingRows.push_back( pSongPatterns->index( pPattern ) );
	}

	// Occasionally the detection of playing patterns glitches at the
	// transition to empty columns.
	if ( playingRows.size() == 0 ) {
		return nScroll;
	}

	// Check if there are any currently playing patterns which are entirely visible.
	for ( int r : playingRows ) {
		if ( r * m_nGridHeight >= nScroll
			 && (r+1) * m_nGridHeight <= nScroll + nHeight) {
			// Entirely visible. Our current scroll value is good.
			if ( pnPatternInView ) {
				*pnPatternInView = r;
			}
			return nScroll;
		}
	}

	// Find the maximum number of patterns that will fit in the viewport. We do this by sorting the playing
	// patterns on their row value, and traversing in order, considering each pattern in turn as visible just
	// at the bottom of the viewport. The pattern visible nearest the top of the viewport is tracked, and the
	// number of patterns visible in the viewport is given by the difference of the indices in the pattern
	// array.
	//
	// We track the maximum number of patterns visible, and record the patterns to scroll to differently
	// depending on whether the pattern is above or below the current viewport: for patterns above, we record
	// the topmost pattern in the maximal group, and for those below, record the bottommost pattern, as these
	// define the minimum amount of scrolling needed to fit the patterns in and don't want to scroll further
	// just to expose empty cells.

	std::sort( playingRows.begin(), playingRows.end() );

	int nTopIdx = 0;
	int nAboveMax = 0, nAbovePattern = -1, nAboveClosestPattern = -1,
		nBelowMax = 0, nBelowPattern = -1, nBelowClosestPattern = -1;

	for ( int nBottomIdx = 0; nBottomIdx < playingRows.size(); nBottomIdx++) {
		int nBottom = playingRows[ nBottomIdx ] * m_nGridHeight;
		int nTop;
		// Each bottom pattern is further down the list, so update the top pattern to track the top of the
		// viewport.
		for (;;) {
			nTop = ( playingRows[ nTopIdx ] +1 ) * m_nGridHeight -1;
			if ( nTop < nBottom - nHeight ) {
				nTopIdx++;
				assert( nTopIdx <= nBottomIdx && nTopIdx < playingRows.size() );
			} else {
				break;
			}
		}
		int nPatternsInViewport = nBottomIdx - nTopIdx +1;
		if ( nBottom < nScroll ) {
			// Above the viewport, accept any new maximal group, to find the maximal group closest to the
			// current viewport.
			if ( nPatternsInViewport >= nAboveMax ) {
				nAboveMax = nPatternsInViewport;
				// Above the viewport, we want to move only so far as to get the top pattern into the
				// viewport. Record the top pattern.
				nAbovePattern = playingRows[ nTopIdx ];
				nAboveClosestPattern = playingRows[ nBottomIdx ];
			}
		} else {
			// Below the viewport, only accept a new maximal group if it's greater than the current maximal
			// group.
			if ( nPatternsInViewport > nBelowMax ) {
				nBelowMax = nPatternsInViewport;
				// Below the viewport, we want to scroll down to get the bottom pattern into view, so record
				// the bottom pattern.
				nBelowPattern = playingRows[ nBottomIdx ];
				nBelowClosestPattern = playingRows[ nTopIdx ];
			}
		}
	}

	// Pick between moving up, or moving down.
	int nAboveY = nAbovePattern * m_nGridHeight;
	int nBelowY = (nBelowPattern +1) * m_nGridHeight - nHeight;
	enum { Up, Down } direction = Down;
	if ( nAboveMax != 0) {
		if ( nAboveMax > nBelowMax ) {
			// Move up to capture more active patterns
			direction = Up;
		} else if ( nBelowMax > nAboveMax ) {
			// Move down to capture more active patterns
			direction = Down;
		} else {
			// Tie-breaker. Which is closer?
			assert( nAboveY <= nScroll &&  nScroll <= nBelowY );
			if ( nScroll - nAboveY < nBelowY - nScroll ) {
				direction = Up;
			} else {
				direction = Down;
			}
		}
	} else {
		assert( nBelowMax != 0 );
		// Move down
		direction = Down;
	}

	if ( direction == Up ) {
		if ( pnPatternInView ) {
			*pnPatternInView = nAboveClosestPattern;
		}
		return nAboveY;
	} else {
		if ( pnPatternInView ) {
			*pnPatternInView = nBelowClosestPattern;
		}
		return nBelowY;
	}
}


int SongEditor::getGridWidth ()
{
	return m_nGridWidth;
}



void SongEditor::setGridWidth( uint width )
{
	if ( ( SONG_EDITOR_MIN_GRID_WIDTH <= width ) && ( SONG_EDITOR_MAX_GRID_WIDTH >= width ) ) {
		m_nGridWidth = width;
		resize( SongEditor::nMargin +
				Preferences::get_instance()->getMaxBars() * m_nGridWidth, height() );
		createBackground();
		update();
	}
}

QPoint SongEditor::xyToColumnRow( QPoint p )
{
	return QPoint( (p.x() - SongEditor::nMargin) / (int)m_nGridWidth, p.y() / (int)m_nGridHeight );
}

QPoint SongEditor::columnRowToXy( QPoint p )
{
	return QPoint( SongEditor::nMargin + p.x() * m_nGridWidth, p.y() * m_nGridHeight );
}


void SongEditor::togglePatternActive( int nColumn, int nRow ) {
	SE_togglePatternAction *action = new SE_togglePatternAction( nColumn, nRow );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}

void SongEditor::setPatternActive( int nColumn, int nRow, bool bActivate )
{
	HydrogenApp* h2app = HydrogenApp::get_instance();
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	bool bPatternIsActive = pSong->isPatternActive( nColumn, nRow );

	if ( bPatternIsActive && ! bActivate || ! bPatternIsActive && bActivate ) {
		h2app->m_pUndoStack->push( new SE_togglePatternAction( nColumn, nRow ) );
	}
}


void SongEditor::selectAll() {
	PatternList *pPatternList = m_pHydrogen->getSong()->getPatternList();
	std::vector<PatternList*>* pColumns = m_pHydrogen->getSong()->getPatternGroupVector();
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
//! * \<patternSelection\>
//!    * \<sourcePosition\>
//! * \<cellList\>
//!    * \<cell\>
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
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
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
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	const int nBlockSize = 5, nWordSize = 5;
	
	bool bIsSelectionKey = false;
	bool bUnhideCursor = true;

	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	H2Core::Song::ActionMode actionMode = pHydrogen->getActionMode();
		
	if ( actionMode == H2Core::Song::ActionMode::selectMode ) {
		bIsSelectionKey = m_selection.keyPressEvent( ev );
	}

	PatternList *pPatternList = pSong->getPatternList();
	const QPoint centre = QPoint( m_nGridWidth / 2, m_nGridHeight / 2 );
	bool bSelectionKey = false;

	int nMaxPatternSequence = Preferences::get_instance()->getMaxBars();

	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Key was claimed by selection
	} else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete: delete selected pattern cells, or cell at current position
		if ( m_selection.begin() != m_selection.end() ) {
			deleteSelection();
		} else {
			// No selection, delete at the current cursor position
			setPatternActive( m_nCursorColumn, m_nCursorRow, false );
		}

	} else if ( ev->matches( QKeySequence::MoveToNextChar ) || ( bSelectionKey = ev->matches( QKeySequence::SelectNextChar ) ) ) {
		// ->
		if ( m_nCursorColumn < nMaxPatternSequence -1 ) {
			m_nCursorColumn += 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToNextWord ) || ( bSelectionKey = ev->matches( QKeySequence::SelectNextWord ) ) ) {
		// -->
		m_nCursorColumn = std::min( (int)nMaxPatternSequence, m_nCursorColumn + nWordSize );

	} else if ( ev->matches( QKeySequence::MoveToEndOfLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectEndOfLine ) ) ) {
		// ->|
		m_nCursorColumn = nMaxPatternSequence -1;

	} else if ( ev->matches( QKeySequence::MoveToPreviousChar ) || ( bSelectionKey = ev->matches( QKeySequence::SelectPreviousChar ) ) ) {
		// <-
		if ( m_nCursorColumn > 0 ) {
			m_nCursorColumn -= 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToPreviousWord ) || ( bSelectionKey = ev->matches( QKeySequence::SelectPreviousWord ) ) ) {
		// <--
		m_nCursorColumn = std::max( 0, m_nCursorColumn - nWordSize );

	} else if ( ev->matches( QKeySequence::MoveToStartOfLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectStartOfLine ) ) ) {
		// |<-
		m_nCursorColumn = 0;

	} else if ( ev->matches( QKeySequence::MoveToNextLine ) || ( bSelectionKey = ev->matches( QKeySequence::SelectNextLine ) ) ) {
		if ( m_nCursorRow < pPatternList->size()-1 ) {
			m_nCursorRow += 1;
		}

	} else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) || ( bSelectionKey = ev->matches( QKeySequence::SelectEndOfBlock ) ) ) {
		m_nCursorRow = std::min( pPatternList->size()-1, m_nCursorRow + nBlockSize );

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

	} else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) || ( bSelectionKey = ev->matches( QKeySequence::SelectStartOfBlock ) ) ) {
		m_nCursorRow = std::max( 0, m_nCursorRow - nBlockSize );


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
		pHydrogenApp->setHideKeyboardCursor( true );

		if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
			pHydrogenApp->getSongEditorPanel()->getSongEditorPatternList()->update();
			pHydrogenApp->getSongEditorPanel()->getSongEditorPositionRuler()->update();
			update();
		}
		return;
	}
	if ( bUnhideCursor ) {
		pHydrogenApp->setHideKeyboardCursor( false );
	}

	if ( bSelectionKey ) {
		// If a "select" key movement is used in "draw" mode, it's probably a good idea to go straight into
		// "select" mode.
		if ( actionMode == H2Core::Song::ActionMode::drawMode ) {
			pHydrogen->setActionMode( H2Core::Song::ActionMode::selectMode );
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

	if ( ! pHydrogenApp->hideKeyboardCursor() ) {
		pHydrogenApp->getSongEditorPanel()->getSongEditorPatternList()->update();
		pHydrogenApp->getSongEditorPanel()->getSongEditorPositionRuler()->update();
	}	
	update();
	ev->accept();
}

void SongEditor::keyReleaseEvent( QKeyEvent * ev ) {
	updateModifiers( ev );
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

	// If there are some patterns selected, we have to switch their
	// border color inactive <-> active.
	createBackground();
	update();
	
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPatternList()->update();
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler()->update();
	}
}

// Make cursor hidden
void SongEditor::focusOutEvent( QFocusEvent *ev )
{
	UNUSED( ev );

	// If there are some patterns selected, we have to switch their
	// border color inactive <-> active.
	createBackground();
	update();
	
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPatternList()->update();
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler()->update();
	}
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
	auto pHydrogenApp = HydrogenApp::get_instance();
	updateModifiers( ev );
	m_currentMousePosition = ev->pos();
	m_bSequenceChanged = true;

	// Update keyboard cursor position
	QPoint p = xyToColumnRow( ev->pos() );
	m_nCursorColumn = p.x();
	m_nCursorRow = p.y();

	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	pHydrogenApp->setHideKeyboardCursor( true );

	if ( Hydrogen::get_instance()->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_selection.mousePressEvent( ev );
		if ( ! pHydrogenApp->hideKeyboardCursor() ) {
			pHydrogenApp->getSongEditorPanel()->getSongEditorPatternList()->update();
			pHydrogenApp->getSongEditorPanel()->getSongEditorPositionRuler()->update();
			update();
		}

	} else {
		if ( ev->button() == Qt::LeftButton ) {
			// Start of a drawing gesture. Pick up whether we are painting Active or Inactive cells.
			QPoint p = xyToColumnRow( ev->pos() );
			m_bDrawingActiveCell = Hydrogen::get_instance()->getSong()->isPatternActive( p.x(), p.y() );
			setPatternActive( p.x(), p.y(), ! m_bDrawingActiveCell );
			m_pSongEditorPanel->updatePlaybackTrackIfNecessary();

		} else if ( ev->button() == Qt::RightButton ) {
			m_pPopupMenu->popup( ev->globalPos() );
		}
	}

	// Cursor just got hidden.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		pHydrogenApp->getSongEditorPanel()->getSongEditorPatternList()->update();
		pHydrogenApp->getSongEditorPanel()->getSongEditorPositionRuler()->update();
		update();
	}
}


void SongEditor::updateModifiers( QInputEvent *ev )
{
	if ( ev->modifiers() == Qt::ControlModifier ) {
		m_bCopyNotMove = true;
	} else {
		m_bCopyNotMove = false;
	}

	if ( QKeyEvent *pEv = dynamic_cast<QKeyEvent*>( ev ) ) {
		// Keyboard events for press and release of modifier keys don't have those keys in the modifiers set,
		// so explicitly update these.
		if ( pEv->key() == Qt::Key_Control ) {
			m_bCopyNotMove = ( ev->type() == QEvent::KeyPress );
		}
	}

	if ( m_selection.isMouseGesture() && m_selection.isMoving() ) {
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

void SongEditor::mouseMoveEvent(QMouseEvent *ev)
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pSong = Hydrogen::get_instance()->getSong();
	updateModifiers( ev );
	m_currentMousePosition = ev->pos();
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();

	if ( Hydrogen::get_instance()->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_selection.mouseMoveEvent( ev );
	} else {
		if ( ev->x() < SongEditor::nMargin ) {
			return;
		}

		QPoint p = xyToColumnRow( ev->pos() );
		if ( m_nCursorColumn == p.x() && m_nCursorRow == p.y() ) {
			// Cursor has not entered a different cell yet.
			return;
		}
		m_nCursorColumn = p.x();
		m_nCursorRow = p.y();
		HydrogenApp::get_instance()->setHideKeyboardCursor( true );

		if ( m_nCursorRow >= pSong->getPatternList()->size() ) {
			// We are below the bottom of the pattern list.
			return;
		}

		// Drawing mode: continue drawing over other cells
		setPatternActive( p.x(), p.y(), ! m_bDrawingActiveCell );
	}

	// Cursor just got hidden.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		pHydrogenApp->getSongEditorPanel()->getSongEditorPatternList()->update();
		pHydrogenApp->getSongEditorPanel()->getSongEditorPositionRuler()->update();
		update();
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
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
	assert( m_pHydrogen->getActionMode() == H2Core::Song::ActionMode::selectMode );
	if ( ev->button() == Qt::LeftButton ) {
		QPoint p = xyToColumnRow( ev->pos() );

		m_selection.clearSelection();
		togglePatternActive( p.x(), p.y() );
		m_bSequenceChanged = true;
		update();
		if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
			HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPatternList()->update();
			HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler()->update();
		}

	} else if ( ev->button() == Qt::RightButton ) {
		m_pPopupMenu->popup( ev->globalPos() );
	}
}

void SongEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	if ( m_pHydrogen->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_selection.mouseReleaseEvent( ev );
		return;
	}
}

void SongEditor::patternModifiedEvent() {
	createBackground();
	update();
}


//! Modify pattern cells by first deleting some, then adding some.
//! deleteCells and addCells *may* safely overlap
void SongEditor::modifyPatternCellsAction( std::vector<QPoint> & addCells, std::vector<QPoint> & deleteCells, std::vector<QPoint> & selectCells ) {
	
	for ( QPoint cell : deleteCells ) {
		setPatternActive( cell.x(), cell.y(), false );
	}

	m_selection.clearSelection();
	for ( QPoint cell : addCells ) {
		setPatternActive( cell.x(), cell.y(), true );
		m_selection.addToSelection( cell );
	}
	// Select additional cells (probably merged cells on redo)
	for ( QPoint cell : selectCells ) {
		m_selection.addToSelection( cell );
	}
}

void SongEditor::updateWidget() {
	// Only update the drawn sequence if necessary. This is only possible when the c
	if ( m_selection.isMoving() ) {
		QPoint currentGridOffset = movingGridOffset();
		// Moving a selection never has to update the sequence (it's drawn on top of the sequence). Update
		// is only ever needed when the move delta (in grid spaces) changes
		if ( m_previousGridOffset != currentGridOffset ) {
			update();
			m_previousGridOffset = currentGridOffset;
		}
	} else if ( m_selection.isLasso() ) {
		bool bCellBoundaryCrossed = xyToColumnRow( m_previousMousePosition ) != xyToColumnRow( m_currentMousePosition );
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


void SongEditor::updatePosition( float fTick ) {
	if ( fTick != m_fTick ) {
		float fDiff = static_cast<float>(m_nGridWidth) * (fTick - m_fTick);
		m_fTick = fTick;
		int nX = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								   m_fTick * static_cast<float>(m_nGridWidth) -
								   static_cast<float>(Skin::nPlayheadWidth) / 2 );
		int nOffset = Skin::getPlayheadShaftOffset();
		QRect updateRect( nX + nOffset -2, 0, 4, height() );
		update( updateRect );
		if ( fDiff > 1.0 || fDiff < -1.0 ) {
			// New cursor is far enough away from the old one that the single update rect won't cover both. So
			// update at the old location as well.
			updateRect.translate( -fDiff, 0 );
			update( updateRect );
		}
	}
}

void SongEditor::paintEvent( QPaintEvent *ev )
{
	// ridisegno tutto solo se sono cambiate le note
	if (m_bSequenceChanged) {
		m_bSequenceChanged = false;
		drawSequence();
	}
	
	auto pPref = Preferences::get_instance();

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
	// Draw playhead
	if ( m_fTick != -1 ) {
		int nX = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								   m_fTick * static_cast<float>(m_nGridWidth) -
								   static_cast<float>(Skin::nPlayheadWidth) / 2 );
		int nOffset = Skin::getPlayheadShaftOffset();
		Skin::setPlayheadPen( &painter, false );
		painter.drawLine( nX + nOffset, 0, nX + nOffset, height() );
	}

	drawFocus( painter );

	m_selection.paintSelection( &painter );

	// Draw cursor
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() && hasFocus() ) {
		QPen p( pPref->getColorTheme()->m_cursorColor );
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
}

void SongEditor::drawFocus( QPainter& painter ) {

	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}
	
	QColor color = H2Core::Preferences::get_instance()->getColorTheme()->m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	int nStartX = m_pScrollView->horizontalScrollBar()->value();
	int nEndX = std::min( nStartX + m_pScrollView->viewport()->size().width(), width() );
	int nStartY = m_pScrollView->verticalScrollBar()->value();
	int nEndY = std::min( static_cast<int>( m_nGridHeight ) * m_pHydrogen->getSong()->getPatternList()->size(),
						  nStartY + m_pScrollView->viewport()->size().height() );

	QPen pen( color );
	pen.setWidth( 4 );
	painter.setPen( pen );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nEndX, nStartY ) );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nStartX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nStartY ), QPoint( nEndX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nEndY ), QPoint( nStartX, nEndY ) );
}

void SongEditor::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

void SongEditor::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void SongEditor::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void SongEditor::createBackground()
{
	auto pPref = H2Core::Preferences::get_instance();
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();

	uint nPatterns = pSong->getPatternList()->size();
	int nSelectedPatternNumber = m_pHydrogen->getSelectedPatternNumber();
	int nMaxPatternSequence = pPref->getMaxBars();

	static int nOldHeight = -1;
	int nNewHeight = m_nGridHeight * nPatterns;

	if (nOldHeight != nNewHeight) {
		// cambiamento di dimensioni...
		if (nNewHeight == 0) {
			nNewHeight = 1;	// the pixmap should not be empty
		}
		if ( m_pBackgroundPixmap ) {
			delete m_pBackgroundPixmap;
		}
		if ( m_pSequencePixmap ) {
			delete m_pSequencePixmap;
		}
		m_pBackgroundPixmap = new QPixmap( width(), nNewHeight );	// initialize the pixmap
		m_pSequencePixmap = new QPixmap( width(), nNewHeight );	// initialize the pixmap
		this->resize( QSize( width(), nNewHeight ) );
	}

	
	m_pBackgroundPixmap->fill( pPref->getColorTheme()->m_songEditor_backgroundColor );

	QPainter p( m_pBackgroundPixmap );
	
	for ( int ii = 0; ii < nPatterns + 1; ii++) {
		if ( ( ii % 2 ) == 0 &&
			 ii != nSelectedPatternNumber ) {
			continue;
		}
		
		int y = m_nGridHeight * ii;
		
		if ( ii == nSelectedPatternNumber ) {
			p.fillRect( 0, y, nMaxPatternSequence * m_nGridWidth, m_nGridHeight,
						pPref->getColorTheme()->m_songEditor_selectedRowColor );
		} else {
			p.fillRect( 0, y, nMaxPatternSequence * m_nGridWidth, m_nGridHeight,
						pPref->getColorTheme()->m_songEditor_alternateRowColor );
		}
	}

	p.setPen( QPen( pPref->getColorTheme()->m_songEditor_lineColor, 1,
					Qt::SolidLine ) );

	// vertical lines
	for ( float ii = 0; ii <= nMaxPatternSequence + 1; ii++) {
		float x = SongEditor::nMargin + ii * m_nGridWidth;
		p.drawLine( x, 0, x, m_nGridHeight * nPatterns );
	}
	
	// horizontal lines
	for (uint i = 0; i < nPatterns; i++) {
		uint y = m_nGridHeight * i;

		p.drawLine( 0, y, (nMaxPatternSequence * m_nGridWidth), y );
	}

	//~ celle
	m_bSequenceChanged = true;
}

void SongEditor::cleanUp(){

	delete m_pBackgroundPixmap;
	m_pBackgroundPixmap = nullptr;
	delete m_pSequencePixmap;
	m_pSequencePixmap = nullptr;
}

// Update the GridCell representation.
void SongEditor::updateGridCells() {

	m_gridCells.clear();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
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

	updateGridCells();

	// Draw using GridCells representation
	for ( auto it : m_gridCells ) {
		if ( ! m_selection.isSelected( QPoint( it.first.x(), it.first.y() ) ) ) {
			drawPattern( it.first.x(), it.first.y(),
						 it.second.m_bDrawnVirtual, it.second.m_fWidth );
		}
	}
	// We draw all selected patterns in a second run to ensure their
	// border does have the proper color (else the bottom and left one
	// could be overwritten by an adjecent, unselected pattern).
	for ( auto it : m_gridCells ) {
		if ( m_selection.isSelected( QPoint( it.first.x(), it.first.y() ) ) ) {
			drawPattern( it.first.x(), it.first.y(),
						 it.second.m_bDrawnVirtual, it.second.m_fWidth );
		}
	}
}



void SongEditor::drawPattern( int nPos, int nNumber, bool bInvertColour, double fWidth )
{
	QPainter p( m_pSequencePixmap );
	/*
	 * The default color of the cubes in rgb is 97,167,251.
	 */
	auto pPref = H2Core::Preferences::get_instance();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	QColor patternColor;
	/*
	 * The following color modes are available:
	 *
	 * Automatic: Steps = Number of pattern in song and colors will be
	 *            chosen internally.
	 * Custom: Number of steps as well as the colors used are defined
	 *            by the user.
	 */
	if ( pPref->getColoringMethod() == H2Core::InterfaceTheme::ColoringMethod::Automatic ) {
		int nSteps = pPatternList->size();

		if( nSteps == 0 ) {
			//beware of the division by zero..
			nSteps = 1;
		}

		int nHue = ( (nNumber % nSteps) * (300 / nSteps) + 213) % 300;
		patternColor.setHsv( nHue , 156 , 249);
	} else {
		int nIndex = nNumber % pPref->getVisiblePatternColors();
		if ( nIndex > m_nMaxPatternColors ) {
			nIndex = m_nMaxPatternColors;
		}
		patternColor = pPref->getPatternColors()[ nIndex ].toHsv();
	}

	if ( true == bInvertColour ) {
		patternColor = patternColor.darker(200);
	}

	bool bIsSelected = m_selection.isSelected( QPoint( nPos, nNumber ) );

	if ( bIsSelected ) {
		patternColor = patternColor.darker( 130 );
	}

	patternColor.setAlpha( 230 );

	int x = SongEditor::nMargin + m_nGridWidth * nPos;
	int y = m_nGridHeight * nNumber;

	p.fillRect( x + 1, y + 1, fWidth * (m_nGridWidth - 1), m_nGridHeight - 1, patternColor );

	// To better distinguish between the individual patterns, they
	// will have a pronounced border.
	QColor borderColor;
	if ( bIsSelected ){
		if ( hasFocus() ) {
			borderColor = pPref->getColorTheme()->m_selectionHighlightColor;
		} else {
			borderColor = pPref->getColorTheme()->m_selectionInactiveColor;
		}
	} else {
		borderColor = QColor( 0, 0, 0 );
	}
	p.setPen( borderColor );
	p.drawRect( x, y, fWidth * m_nGridWidth, m_nGridHeight );
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	m_pAudioEngine->lock( RIGHT_HERE );

	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	//before deleting the sequence, write a temp sequence file to disk
	pSong->writeTempPatternList( filename );

	std::vector<PatternList*> *pPatternGroupsVect = pSong->getPatternGroupVector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		PatternList *pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
		delete pPatternList;
	}
	pPatternGroupsVect->clear();
	pHydrogen->updateSongSize();
	
	m_pAudioEngine->unlock();

	pHydrogen->setIsModified( true );
	m_bSequenceChanged = true;
	update();
}

void SongEditor::updateEditorandSetTrue()
{
	m_bSequenceChanged = true;
	update();
}

void SongEditor::onPreferencesChanged( H2Core::Preferences::Changes changes ) 
{
	if ( changes & ( H2Core::Preferences::Changes::GeneralTab |
					 H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::AppearanceTab ) ) {
		resize( SongEditor::nMargin +
				Preferences::get_instance()->getMaxBars() * m_nGridWidth, height() );

		m_bSequenceChanged = true;

		// Required to be called at least once in order to make the
		// scroll bars match the (potential) new width.
		HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
	}
}

// :::::::::::::::::::


SongEditorPatternList::SongEditorPatternList( QWidget *parent )
 : QWidget( parent )
 , EventListener()
 , WidgetWithHighlightedList()
 , m_pBackgroundPixmap( nullptr )
 , m_nRowHovered( -1 )
{
	m_pHydrogen = Hydrogen::get_instance();
	m_pAudioEngine = m_pHydrogen->getAudioEngine();

	auto pPref = Preferences::get_instance();
	
	m_nWidth = 200;
	m_nGridHeight = pPref->getSongEditorGridHeight();
	setAttribute(Qt::WA_OpaquePaintEvent);

	setAcceptDrops(true);
	setMouseTracking( true );

	m_pPatternBeingEdited = nullptr;

	m_pLineEdit = new QLineEdit( "Inline Pattern Name", this );
	m_pLineEdit->setFrame( false );
	m_pLineEdit->hide();
	m_pLineEdit->setAcceptDrops( false );
	connect( m_pLineEdit, SIGNAL(editingFinished()), this, SLOT(inlineEditingFinished()) );
	connect( m_pLineEdit, SIGNAL(returnPressed()), this, SLOT(inlineEditingEntered()) );

	this->resize( m_nWidth, m_nInitialHeight );

	m_playingPattern_on_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_on.png" );
	m_playingPattern_off_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_off.png" );
	m_playingPattern_empty_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_empty.png" );

	m_pPatternPopup = new QMenu( this );
	m_pPatternPopup->addAction( tr("Duplicate"),  this, SLOT( patternPopup_duplicate() ) );
	m_pPatternPopup->addAction( tr("Delete"),  this, SLOT( patternPopup_delete() ) );
	m_pPatternPopup->addAction( tr("Fill/Clear..."),  this, SLOT( patternPopup_fill() ) );
	m_pPatternPopup->addAction( tr("Properties"),  this, SLOT( patternPopup_properties() ) );
	m_pPatternPopup->addAction( tr("Load Pattern"),  this, SLOT( patternPopup_load() ) );
	m_pPatternPopup->addAction( tr("Save Pattern"),  this, SLOT( patternPopup_save() ) );
	m_pPatternPopup->addAction( tr("Export Pattern"),  this, SLOT( patternPopup_export() ) );
	m_pPatternPopup->addAction( tr("Virtual Pattern"), this, SLOT( patternPopup_virtualPattern() ) );
	m_pPatternPopup->setObjectName( "PatternListPopup" );

	// Reset the clicked row once the popup is closed by clicking at
	// any position other than at an action of the popup.
	connect( m_pPatternPopup, &QMenu::aboutToHide, [=](){
		if ( m_rowSelection == RowSelection::Popup ) {
			setRowSelection( RowSelection::None );
		}
	});

	HydrogenApp::get_instance()->addEventListener( this );

	QScrollArea *pScrollArea = dynamic_cast< QScrollArea * >( parentWidget()->parentWidget() );
	assert( pScrollArea );
	m_pDragScroller = new DragScroller( pScrollArea );
	
	m_pHighlightLockedTimer = new QTimer( this );
	m_pHighlightLockedTimer->setSingleShot( true );
	connect(m_pHighlightLockedTimer, &QTimer::timeout,
			[=](){ HydrogenApp::get_instance()->getSongEditorPanel()->highlightPatternEditorLocked( false ); } );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nWidth * pixelRatio,
									   height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	
	createBackground();
	update();
}



SongEditorPatternList::~SongEditorPatternList()
{
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
	delete m_pDragScroller;
}


void SongEditorPatternList::patternChangedEvent() {
	createBackground();
	update();
}

void SongEditorPatternList::setRowSelection( RowSelection rowSelection ) {
	m_rowSelection = rowSelection;
	createBackground();
	update();
}

void SongEditorPatternList::patternModifiedEvent() {
	createBackground();
	update();
}

void SongEditorPatternList::selectedPatternChangedEvent() {
	createBackground();
	update();
}

void SongEditorPatternList::stackedPatternsChangedEvent() {
	createBackground();
	update();
}

/// Single click, select the next pattern
void SongEditorPatternList::mousePressEvent( QMouseEvent *ev )
{
	__drag_start_position = ev->pos();
	
	// -1 to compensate for the 1 pixel offset to align shadows and
	// -grid lines.
	int nRow = (( ev->y() - 1 ) / m_nGridHeight);

	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	auto pPatternList = pSong->getPatternList();

	if ( nRow < 0 || nRow >= (int)pPatternList->size() ) {
		ERRORLOG( QString( "Row [%1] out of bound" ).arg( nRow ) );
		return;
	}

	if ( ( ev->button() == Qt::MiddleButton ||
		   ( ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::RightButton ) ||
		   ( ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton ) ||
		   ev->pos().x() < 15 ) &&
		 m_pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
		
		m_pHydrogen->toggleNextPattern( nRow );
	}
	else {
		if ( ! ( m_pHydrogen->isPatternEditorLocked() &&
				 m_pHydrogen->getAudioEngine()->getState() ==
				 AudioEngine::State::Playing ) ) {
			m_pHydrogen->setSelectedPatternNumber( nRow );
		} else {
			// Notify the users why nothing just happened by
			// highlighting the pattern locked button in the
			// SongEditorPanel.
			HydrogenApp::get_instance()->getSongEditorPanel()->highlightPatternEditorLocked( true );
			m_pHighlightLockedTimer->start( 250 );
		}
		
		if (ev->button() == Qt::RightButton)  {

			if ( m_rowSelection == RowSelection::Dialog ) {
				// There is still a dialog window opened from the last
				// time. It needs to be closed before the popup will
				// be shown again.
				ERRORLOG( "A dialog is still opened. It needs to be closed first." );
				return;
			}
			
			m_nRowClicked = nRow;
			setRowSelection( RowSelection::Popup );
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

	m_pHydrogen->toggleNextPattern( row );
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
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if ( row >= (int)pPatternList->size() ) {
		return;
	}
	m_pPatternBeingEdited = pPatternList->get( row );
	m_pLineEdit->setGeometry( 23, row * m_nGridHeight + 1 , m_nWidth - 23, m_nGridHeight  );
	m_pLineEdit->setText( m_pPatternBeingEdited->get_name() );
	m_pLineEdit->selectAll();
	m_pLineEdit->show();
	m_pLineEdit->setFocus();
}

void SongEditorPatternList::inlineEditingEntered()
{
	assert( m_pPatternBeingEdited != nullptr );
	
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	
	/*
	 * Make sure that the entered pattern name is unique.
	 * If it is not, use an unused pattern name.
	 */
	
	QString patternName = pPatternList->find_unused_pattern_name( m_pLineEdit->text(), m_pPatternBeingEdited );

	SE_modifyPatternPropertiesAction *action =
		new SE_modifyPatternPropertiesAction( m_pPatternBeingEdited->get_name(),
											  m_pPatternBeingEdited->get_info(),
											  m_pPatternBeingEdited->get_category(),
											  patternName,
											  m_pPatternBeingEdited->get_info(),
											  m_pPatternBeingEdited->get_category(),
											  pPatternList->index( m_pPatternBeingEdited ) );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}


void SongEditorPatternList::inlineEditingFinished()
{
	m_pPatternBeingEdited = nullptr;
	m_pLineEdit->hide();
}


void SongEditorPatternList::paintEvent( QPaintEvent *ev )
{
	auto pPref = Preferences::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pSongEditor = pHydrogenApp->getSongEditorPanel()->getSongEditor();
	
	QPainter painter(this);
	qreal pixelRatio = devicePixelRatio();
	if ( width() != m_pBackgroundPixmap->width() ||
		 height() != m_pBackgroundPixmap->height() ||
		 pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ) {
		createBackground();
	}
	QRectF srcRect(
			pixelRatio * ev->rect().x(),
			pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(),
			pixelRatio * ev->rect().height()
	);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, srcRect );

	// In case a row was right-clicked or the cursor is positioned on
	// a grid cell within this row, highlight it using a border.
	if ( ( ! pHydrogenApp->hideKeyboardCursor() &&
		   pSongEditor->hasFocus() ) ||
		 m_rowSelection != RowSelection::None ) {
		QColor colorHighlight = pPref->getColorTheme()->m_highlightColor;
		QPen pen;

		int nStartY;
		if ( m_rowSelection != RowSelection::None ) {
			// In case a row was right-clicked, highlight it using a border.
			pen.setColor( pPref->getColorTheme()->m_highlightColor);
			nStartY = m_nRowClicked * m_nGridHeight;
		} else {
			pen.setColor( pPref->getColorTheme()->m_cursorColor );
			nStartY = pSongEditor->getCursorRow() * m_nGridHeight;
		}
		pen.setWidth( 2 );
		painter.setRenderHint( QPainter::Antialiasing );
			
		painter.setPen( pen );
		painter.drawRoundedRect( QRect( 1, nStartY + 1, m_nWidth - 2,
										m_nGridHeight - 1 ), 4, 4 );
	}
}


void SongEditorPatternList::updateEditor()
{
	if(!isVisible()) {
		return;
	}

	update();
}

void SongEditorPatternList::songModeActivationEvent() {

	// Refresh pattern list display if in stacked mode
	if ( Hydrogen::get_instance()->getPatternMode() ==
		 Song::PatternMode::Stacked ) {
		createBackground();
		update();
	}
}

void SongEditorPatternList::createBackground()
{
	auto pPref = H2Core::Preferences::get_instance();

	QFont boldTextFont( pPref->getLevel2FontFamily(), getPointSize( pPref->getFontSize() ) );
	boldTextFont.setBold( true );

	//Do not redraw anything if Export is active.
	//https://github.com/hydrogen-music/hydrogen/issues/857	
	if( m_pHydrogen->getIsExportSessionActive() ) {
		return;
	}
	
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	int nPatterns = pSong->getPatternList()->size();
	int nSelectedPattern = m_pHydrogen->getSelectedPatternNumber();

	int newHeight = m_nGridHeight * nPatterns + 1;

	if ( m_nWidth != m_pBackgroundPixmap->width() ||
		 newHeight != m_pBackgroundPixmap->height() ||
		 m_pBackgroundPixmap->devicePixelRatio() != devicePixelRatio() ) {
		if (newHeight == 0) {
			newHeight = 1;	// the pixmap should not be empty
		}
		delete m_pBackgroundPixmap;
		qreal pixelRatio = devicePixelRatio();
		m_pBackgroundPixmap = new QPixmap( m_nWidth  * pixelRatio , newHeight * pixelRatio );	// initialize the pixmap
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		this->resize( m_nWidth, newHeight );
	}

	QColor backgroundColor = pPref->getColorTheme()->m_songEditor_backgroundColor.darker( 120 );
	QColor backgroundColorSelected = pPref->getColorTheme()->m_songEditor_selectedRowColor.darker( 114 );
	QColor backgroundColorAlternate = pPref->getColorTheme()->m_songEditor_alternateRowColor.darker( 132 );

	QPainter p( m_pBackgroundPixmap );


	// Offset the pattern list by one pixel to align the dark shadows
	// at the bottom of each row with the grid lines in the song editor.
	p.fillRect( QRect( 0, 0, width(), 1 ), pPref->getColorTheme()->m_windowColor );
	
	p.setFont( boldTextFont );
	for ( int ii = 0; ii < nPatterns; ii++ ) {
		uint y = m_nGridHeight * ii + 1;
		
		if ( ii == nSelectedPattern ) {
			Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
									  backgroundColorSelected, false );
		} else {
			if ( ( ii % 2 ) == 0 ) {
				Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
										  backgroundColor,
										  ii == m_nRowHovered );
			} else {
				Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
										  backgroundColorAlternate,
										  ii == m_nRowHovered );
			}
		}
	}

	std::unique_ptr<PatternDisplayInfo[]> PatternArray{new PatternDisplayInfo[nPatterns]};

	m_pAudioEngine->lock( RIGHT_HERE );
	auto pPlayingPatterns = m_pAudioEngine->getPlayingPatterns();

	//assemble the data..
	for ( int i = 0; i < nPatterns; i++ ) {
		H2Core::Pattern *pPattern = pSong->getPatternList()->get(i);
		if ( pPattern == nullptr ) {
			continue;
		}

		if ( pPlayingPatterns->index( pPattern ) != -1 ) {
			PatternArray[i].bActive = true;
		} else {
			PatternArray[i].bActive = false;
		}

		if ( m_pAudioEngine->getNextPatterns()->index( pPattern ) != -1 ) {
			PatternArray[i].bNext = true;
		} else {
			PatternArray[i].bNext = false;
		}

		PatternArray[i].sPatternName = pPattern->get_name();
	}
	m_pAudioEngine->unlock();

	/// paint the foreground (pattern name etc.)
	for ( int i = 0; i < nPatterns; i++ ) {
		if ( i == nSelectedPattern ) {
			p.setPen( pPref->getColorTheme()->m_songEditor_selectedRowTextColor );
		}
		else {
			p.setPen( pPref->getColorTheme()->m_songEditor_textColor );
		}

		uint text_y = i * m_nGridHeight;

		p.drawText( 25, text_y - 1, m_nWidth - 25, m_nGridHeight + 2,
					Qt::AlignVCenter, PatternArray[i].sPatternName);

		Skin::Stacked mode = Skin::Stacked::None;
		if ( PatternArray[i].bNext && PatternArray[i].bActive) {
			mode = Skin::Stacked::OffNext;
		}
		else if ( PatternArray[i].bNext ) {
			mode = Skin::Stacked::OnNext;
		}
		else if (PatternArray[i].bActive) {
			mode = Skin::Stacked::On;
		}
		else if ( m_pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
			mode = Skin::Stacked::Off;
		}
		
		if ( mode != Skin::Stacked::None ) {
			Skin::drawStackedIndicator( &p, 5, text_y + 4, mode );
		}

	}
}

void SongEditorPatternList::stackedModeActivationEvent( int ) {
	createBackground();
	update();
}

void SongEditorPatternList::patternPopup_virtualPattern()
{
	setRowSelection( RowSelection::Dialog );
	
	VirtualPatternDialog *dialog = new VirtualPatternDialog( this );
	SongEditorPanel *pSEPanel = HydrogenApp::get_instance()->getSongEditorPanel();

	dialog->patternList->setSortingEnabled(1);

	std::shared_ptr<Song> song = m_pHydrogen->getSong();
	PatternList *pPatternList = song->getPatternList();
	auto pPatternClicked = pPatternList->get( m_nRowClicked );

	std::map<QString, Pattern*> patternNameMap;

	int listsize = pPatternList->size();
	for (unsigned int index = 0; index < listsize; ++index) {
		H2Core::Pattern *curPattern = pPatternList->get( index );
		QString patternName = curPattern->get_name();

		if (patternName == pPatternClicked->get_name()) {
			continue;
		}//if

		patternNameMap[patternName] = curPattern;

		QListWidgetItem *newItem = new QListWidgetItem(patternName, dialog->patternList);
		dialog->patternList->insertItem(0, newItem );

		if (pPatternClicked->get_virtual_patterns()->find(curPattern) !=
			pPatternClicked->get_virtual_patterns()->end()) {
			newItem->setSelected( true );
		}//if
	}//for

	if ( dialog->exec() == QDialog::Accepted ) {
		pPatternClicked->virtual_patterns_clear();
		for (unsigned int index = 0; index < listsize-1; ++index) {
			QListWidgetItem *listItem = dialog->patternList->item(index);
			if (listItem->isSelected() == true) {
				if (patternNameMap.find(listItem->text()) != patternNameMap.end()) {
					pPatternClicked->virtual_patterns_add(patternNameMap[listItem->text()]);
				}//if
			}//if
		}//for

		pSEPanel->updateAll();
	}//if

	pPatternList->flattened_virtual_patterns_compute();

	delete dialog;

	setRowSelection( RowSelection::None );
}//patternPopup_virtualPattern



void SongEditorPatternList::patternPopup_load()
{
	setRowSelection( RowSelection::Dialog );
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		setRowSelection( RowSelection::None );
		return;
	}
	
	Pattern* pPattern = pSong->getPatternList()->get( m_nRowClicked );

	QString sPath = Preferences::get_instance()->getLastOpenPatternDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::patterns_dir();
	}

	QFileDialog fd(this);
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setDirectory( sPath );
	fd.setWindowTitle( QString( tr( "Open Pattern to Replace " )
								.append( pPattern->get_name() ) ) );

	if (fd.exec() != QDialog::Accepted) {
		setRowSelection( RowSelection::None );
		return;
	}
	QString patternPath = fd.selectedFiles().first();

	QString prevPatternPath =
		Files::savePatternTmp( pPattern->get_name(), pPattern, pSong,
							   pHydrogen->getLastLoadedDrumkitName() );
	if ( prevPatternPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
		setRowSelection( RowSelection::None );
		return;
	}
	QString sequencePath = Filesystem::tmp_file_path( "SEQ.xml" );
	if ( !pSong->writeTempPatternList( sequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		setRowSelection( RowSelection::None );
		return;
	}
	Preferences::get_instance()->setLastOpenPatternDirectory( fd.directory().absolutePath() );

	SE_loadPatternAction *action =
		new SE_loadPatternAction( patternPath, prevPatternPath, sequencePath,
								  m_nRowClicked, false );
	HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
	hydrogenApp->m_pUndoStack->push( action );
	
	setRowSelection( RowSelection::None );
}

void SongEditorPatternList::patternPopup_export()
{
	setRowSelection( RowSelection::Dialog );
	HydrogenApp::get_instance()->getMainForm()->action_file_export_pattern_as( m_nRowClicked );

	setRowSelection( RowSelection::None );
	return;
}

void SongEditorPatternList::patternPopup_save()
{
	setRowSelection( RowSelection::Dialog );
	
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPattern = pSong->getPatternList()->get( m_nRowClicked );

	QString sPath = Files::savePatternNew( pPattern->get_name(), pPattern,
										   pSong, pHydrogen->getLastLoadedDrumkitName() );
	if ( sPath.isEmpty() ) {
		if ( QMessageBox::information( this, "Hydrogen", tr( "The pattern-file exists. \nOverwrite the existing pattern?"),
									   pCommonStrings->getButtonOk(),
									   pCommonStrings->getButtonCancel(),
									   nullptr, 1 ) != 0 ) {
			setRowSelection( RowSelection::None );
			return;
		}
		sPath = Files::savePatternOver( pPattern->get_name(), pPattern,
										pSong, pHydrogen->getLastLoadedDrumkitName() );
	}

	if ( sPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
		setRowSelection( RowSelection::None );
		return;
	}

	pHydrogenApp->showStatusBarMessage( tr( "Pattern saved." ) );

	pHydrogen->getSoundLibraryDatabase()->updatePatterns();
	
	setRowSelection( RowSelection::None );
}



void SongEditorPatternList::patternPopup_edit()
{
	HydrogenApp::get_instance()->getPatternEditorPanel()->show();
	HydrogenApp::get_instance()->getPatternEditorPanel()->setFocus();
}



void SongEditorPatternList::patternPopup_properties()
{
	
	setRowSelection( RowSelection::Dialog );
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = pHydrogen->getSong()->getPatternList()->get( m_nRowClicked );

	PatternPropertiesDialog *dialog =
		new PatternPropertiesDialog( this, pPattern, m_nRowClicked, false);
	dialog->exec();
	delete dialog;
	dialog = nullptr;

	setRowSelection( RowSelection::None );
}


void SongEditorPatternList::acceptPatternPropertiesDialogSettings(QString newPatternName, QString newPatternInfo, QString newPatternCategory, int patternNr)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	PatternList *patternList = pSong->getPatternList();
	H2Core::Pattern *pattern = patternList->get( patternNr );
	pattern->set_name( newPatternName );
	pattern->set_info( newPatternInfo );
	pattern->set_category( newPatternCategory );
	pHydrogen->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, -1 );
	createBackground();
	update();
}


void SongEditorPatternList::revertPatternPropertiesDialogSettings(QString oldPatternName, QString oldPatternInfo, QString oldPatternCategory, int patternNr)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	PatternList *patternList = pSong->getPatternList();
	H2Core::Pattern *pattern = patternList->get( patternNr );
	pattern->set_name( oldPatternName );
	pattern->set_category( oldPatternCategory );
	pHydrogen->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, -1 );
	createBackground();
	update();
}


void SongEditorPatternList::patternPopup_delete()
{
	setRowSelection( RowSelection::Dialog );
	
	auto pSong = m_pHydrogen->getSong();
	auto pPattern = pSong->getPatternList()->get( m_nRowClicked );

	QString patternPath =
		Files::savePatternTmp( pPattern->get_name(), pPattern, pSong,
							   m_pHydrogen->getLastLoadedDrumkitName() );
	if ( patternPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
		setRowSelection( RowSelection::None );
		return;
	}
	QString sequencePath = Filesystem::tmp_file_path( "SEQ.xml" );
	if ( !pSong->writeTempPatternList( sequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		setRowSelection( RowSelection::None );
		return;
	}

	SE_deletePatternFromListAction *action =
		new SE_deletePatternFromListAction( patternPath, sequencePath,
											m_nRowClicked );
	HydrogenApp *hydrogenApp = HydrogenApp::get_instance();
	hydrogenApp->m_pUndoStack->push( action );

	setRowSelection( RowSelection::None );
}

void SongEditorPatternList::patternPopup_duplicate()
{
	setRowSelection( RowSelection::Dialog );
	
	auto pSong = m_pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( m_nRowClicked );

	H2Core::Pattern *pNewPattern = new Pattern( pPattern );
	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, pNewPattern, m_nRowClicked, true );

	if ( dialog->exec() == QDialog::Accepted ) {
		QString filePath = Files::savePatternTmp( pNewPattern->get_name(),
												  pNewPattern, pSong,
												  m_pHydrogen->getLastLoadedDrumkitName() );
		if ( filePath.isEmpty() ) {
			QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
			setRowSelection( RowSelection::None );
			return;
		}
		SE_duplicatePatternAction *action =
			new SE_duplicatePatternAction( filePath, m_nRowClicked + 1 );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}

	delete dialog;
	delete pNewPattern;

	setRowSelection( RowSelection::None );
}

void SongEditorPatternList::patternPopup_fill()
{
	setRowSelection( RowSelection::Dialog );
	
	FillRange range;
	PatternFillDialog *dialog = new PatternFillDialog( this, &range );

	// use a PatternFillDialog to get the range and mode data
	if ( dialog->exec() == QDialog::Accepted ) {

		SE_fillRangePatternAction *action =
			new SE_fillRangePatternAction( &range, m_nRowClicked );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}

	delete dialog;

	setRowSelection( RowSelection::None );
}


void SongEditorPatternList::fillRangeWithPattern( FillRange* pRange, int nPattern )
{
	m_pAudioEngine->lock( RIGHT_HERE );

	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
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
	m_pAudioEngine->unlock();


	// Update
	m_pHydrogen->setIsModified( true );
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
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	
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
		QStringList tokens = sText.split( ":" );
		bool bOK = true;

		int nSourcePattern = tokens[1].toInt(&bOK);
		if ( ! bOK ) {
			return;
		}

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
		Hydrogen *pHydrogen = Hydrogen::get_instance();

		std::shared_ptr<Song> pSong = pHydrogen->getSong();
		PatternList *pPatternList = pSong->getPatternList();



		// move patterns...
		H2Core::Pattern *pSourcePattern = pPatternList->get( nSourcePattern );
		if ( nSourcePattern < nTargetPattern) {
			for (int nPatr = nSourcePattern; nPatr < nTargetPattern; nPatr++) {
				H2Core::Pattern *pPattern = pPatternList->get(nPatr + 1);
				pPatternList->replace( nPatr, pPattern );
			}
			pPatternList->replace( nTargetPattern, pSourcePattern );
		}
		else {
			for (int nPatr = nSourcePattern; nPatr > nTargetPattern; nPatr--) {
				H2Core::Pattern *pPattern = pPatternList->get(nPatr - 1);
				pPatternList->replace( nPatr, pPattern );
			}
			pPatternList->replace( nTargetPattern, pSourcePattern );
		}

		if ( pHydrogen->isPatternEditorLocked() ) {
			pHydrogen->updateSelectedPattern();
		} else  {
			pHydrogen->setSelectedPatternNumber( nTargetPattern );
		}
		HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
		pHydrogen->setIsModified( true );
}

void SongEditorPatternList::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_nRowHovered = -1;
	createBackground();
	update();
}

void SongEditorPatternList::mouseMoveEvent(QMouseEvent *event)
{
	// Update the highlighting of the hovered row.
	if ( event->pos().y() / m_nGridHeight != m_nRowHovered ) {
		m_nRowHovered = event->pos().y() / m_nGridHeight;
		createBackground();
		update();
	}
	
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	if ( (event->pos().y() / m_nGridHeight) == (__drag_start_position.y() / m_nGridHeight) ) {
		return;
	}
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	int row = (__drag_start_position.y() / m_nGridHeight);
	if ( row >= (int)pPatternList->size() ) {
		return;
	}
	Pattern *pPattern = pPatternList->get( row );
	QString sName = "<unknown>";
	if ( pPattern ) {
		sName = pPattern->get_name();
	}
	QString sText = QString("move pattern:%1:%2").arg( row ).arg( sName );

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData);
	//drag->setPixmap(iconPixmap);

	m_pDragScroller->startDrag();
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
	m_pDragScroller->endDrag();

	QWidget::mouseMoveEvent(event);
}


void SongEditorPatternList::timelineUpdateEvent( int nEvent )
{
	HydrogenApp::get_instance()->getSongEditorPanel()->updateAll();
}

void SongEditorPatternList::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		
		createBackground();
		update();
	}
}

// ::::::::::::::::::::::::::

SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent )
 , m_bRightBtnPressed( false )
 , m_nActiveBpmWidgetColumn( -1 )
 , m_nHoveredColumn( -1 )
 , m_hoveredRow( HoveredRow::None )
 , m_nTagHeight( 6 )
 , m_fTick( 0 )
 , m_nColumn( 0 )
{

	auto pPref = H2Core::Preferences::get_instance();

	HydrogenApp::get_instance()->addEventListener( this );

	m_pHydrogen = Hydrogen::get_instance();
	m_pAudioEngine = m_pHydrogen->getAudioEngine();

	setAttribute(Qt::WA_OpaquePaintEvent);
	setMouseTracking( true );

	m_nGridWidth = pPref->getSongEditorGridWidth();

	int nInitialWidth = SongEditor::nMargin +
		Preferences::get_instance()->getMaxBars() * m_nGridWidth;
	
	m_nActiveColumns = m_pHydrogen->getSong()->getPatternGroupVector()->size();

	resize( nInitialWidth, m_nHeight );
	setFixedHeight( m_nHeight );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( nInitialWidth * pixelRatio, m_nHeight * pixelRatio );	// initialize the pixmap
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );

	createBackground();	// create m_backgroundPixmap pixmap
	update();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, &QTimer::timeout, [=]() {
		if ( H2Core::Hydrogen::get_instance()->getAudioEngine()->getState() ==
			 H2Core::AudioEngine::State::Playing ) {
			updatePosition();
		}
	});
	m_pTimer->start(200);
}



SongEditorPositionRuler::~SongEditorPositionRuler() {
	m_pTimer->stop();
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
}

void SongEditorPositionRuler::relocationEvent() {
	updatePosition();
}

void SongEditorPositionRuler::songSizeChangedEvent() {
	m_nActiveColumns = m_pHydrogen->getSong()->getPatternGroupVector()->size();
	createBackground();
	update();
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
		resize( SongEditor::nMargin +
				Preferences::get_instance()->getMaxBars() * m_nGridWidth, height() );
		createBackground();
		update();
	}
}


void SongEditorPositionRuler::createBackground()
{
	Preferences *pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pTimeline = pHydrogen->getTimeline();
	auto tagVector = pTimeline->getAllTags();
	
	QColor textColor( pPref->getColorTheme()->m_songEditor_textColor );
	QColor textColorAlpha( textColor );
	textColorAlpha.setAlpha( 45 );

	QColor backgroundColor = pPref->getColorTheme()->m_songEditor_alternateRowColor.darker( 115 );
	QColor backgroundInactiveColor = pPref->getColorTheme()->m_midLightColor;
	QColor backgroundColorTempoMarkers = backgroundColor.darker( 120 );

	QColor colorHighlight = pPref->getColorTheme()->m_highlightColor;

	QColor lineColor = pPref->getColorTheme()->m_songEditor_lineColor;
	QColor lineColorAlpha( lineColor );
	lineColorAlpha.setAlpha( 45 );
		
	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ||
		 m_pBackgroundPixmap->width() != width() ||
		 m_pBackgroundPixmap->height() != height()  ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( font );

	int nActiveWidth = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
										 static_cast<float>(m_nActiveColumns) *
										 static_cast<float>(m_nGridWidth) );
	p.fillRect( 0, 0, width(), height(), backgroundColorTempoMarkers );
	p.fillRect( 0, 25, nActiveWidth, height() - 25, backgroundColor );
	p.fillRect( nActiveWidth, 25, width() - nActiveWidth, height() - 25,
				backgroundInactiveColor );
	char tmp[10];
	
	int nMaxPatternSequence = pPref->getMaxBars();
	
	QColor textColorGrid( textColor );
	textColorGrid.setAlpha( 200 );
	p.setPen( QPen( textColorGrid, 1, Qt::SolidLine ) );
	for ( int ii = 0; ii < nMaxPatternSequence + 1; ii++) {
		int x = SongEditor::nMargin + ii * m_nGridWidth;

		if ( ( ii % 4 ) == 0 ) {
			p.drawLine( x, height() - 14, x, height() - 1);
		}
		else {
			p.drawLine( x, height() - 6, x, height() - 1);
		}
	}

	// Add every 4th number to the grid
	p.setPen( textColor );
	for (uint i = 0; i < nMaxPatternSequence + 1; i += 4) {
		uint x = SongEditor::nMargin + i * m_nGridWidth;

		sprintf( tmp, "%d", i + 1 );
		if ( i < 10 ) {
			p.drawText( x, height() / 2 + 3, m_nGridWidth, height() / 2 - 7,
						Qt::AlignHCenter, tmp );
		} else {
			p.drawText( x + 2, height() / 2 + 3, m_nGridWidth * 3.5, height() / 2 - 7,
						Qt::AlignLeft, tmp );
		}
	}
	
	// draw tags
	p.setPen( pPref->getColorTheme()->m_accentTextColor );
	
	QFont font2( pPref->getApplicationFontFamily(), 5 );
	p.setFont( font2 );
		
	for ( const auto& ttag : tagVector ){
		int x = SongEditor::nMargin + ttag->nColumn * m_nGridWidth + 4;
		QRect rect( x, height() / 2 - 1 - m_nTagHeight,
					m_nGridWidth - 6, m_nTagHeight );

		p.fillRect( rect, pPref->getColorTheme()->m_highlightColor.darker( 135 ) );
		p.drawText( rect, Qt::AlignCenter, "T");
	}
	p.setFont( font );

	// draw tempo content
	
	// Draw tempo marker grid.
	if ( ! pHydrogen->isTimelineEnabled() ) {
		p.setPen( textColorAlpha );
	} else {
		QColor tempoMarkerGridColor( textColor );
		tempoMarkerGridColor.setAlpha( 170 );
		p.setPen( tempoMarkerGridColor );
	}
	for (uint ii = 0; ii < nMaxPatternSequence + 1; ii++) {
		uint x = SongEditor::nMargin + ii * m_nGridWidth;

		p.drawLine( x, 1, x, 4 );
		p.drawLine( x, height() / 2 - 5, x, height() / 2 );
	}

	// Draw tempo markers
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	for ( const auto& ttempoMarker : tempoMarkerVector ){
		drawTempoMarker( ttempoMarker, false, p );				
	}

	p.setPen( QColor(35, 39, 51) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, height() - 25, width(), height() - 25 );
	p.drawLine( 0, height(), width(), height() );
}

void SongEditorPositionRuler::tempoChangedEvent( int ) {
	auto pTimeline = Hydrogen::get_instance()->getTimeline();
	if ( ! pTimeline->isFirstTempoMarkerSpecial() ) {
		return;
	}

	// There is just the special tempo marker -> no tempo markers set
	// by the user. In this case the special marker isn't drawn and
	// doesn't need to be update.
	if ( pTimeline->getAllTempoMarkers().size() == 1 ) {
		return;
	}

	createBackground();
	update();
}

void SongEditorPositionRuler::patternModifiedEvent() {
	// This can change the size of the song and affect the position of
	// the playhead.
	update();
}

void SongEditorPositionRuler::patternChangedEvent() {
	// Triggered every time the column of the SongEditor grid
	// changed. Either by rolling transport or by relocation.
	update();
}

void SongEditorPositionRuler::leaveEvent( QEvent* ev ){
	m_nHoveredColumn = -1;
	m_hoveredRow = HoveredRow::None;
	update();

	QWidget::leaveEvent( ev );
}

void SongEditorPositionRuler::mouseMoveEvent(QMouseEvent *ev)
{
	auto pHydrogen = Hydrogen::get_instance();
	
	int nColumn = ( std::max( ev->x() - SongEditor::nMargin, 0 ) ) / m_nGridWidth;

	HoveredRow row;
	if ( ev->y() > 22 ) {
		row = HoveredRow::Ruler;
	} else if ( ev->y() > 22 - 1 - m_nTagHeight ) {
		row = HoveredRow::Tag;
	} else {
		row = HoveredRow::TempoMarker;
	}
	
	if ( nColumn != m_nHoveredColumn ||
		 row != m_hoveredRow ) {
		// Cursor has moved into a region where the above caching
		// became invalid.
		m_hoveredRow = row;
		m_nHoveredColumn = nColumn;
		 
		update();
	}
	
	if ( !m_bRightBtnPressed && ev->buttons() & Qt::LeftButton ) {
		// Click+drag triggers same action as clicking at new position
		mousePressEvent( ev );
	} else if ( ev->buttons() & Qt::RightButton ) {
		// Right-click+drag
		Preferences* pPref = Preferences::get_instance();
		
		if ( nColumn > (int)Hydrogen::get_instance()->getSong()->getPatternGroupVector()->size() ) {
			pPref->setPunchOutPos(-1);
			return;
		}
		if ( Hydrogen::get_instance()->getMode() == Song::Mode::Pattern ) {
			return;
		}
		pPref->setPunchOutPos( nColumn - 1 );
		update();
	}
}

bool SongEditorPositionRuler::event( QEvent* ev ) {
	if ( ev->type() == QEvent::ToolTip ) {
		showToolTip( dynamic_cast<QHelpEvent*>(ev) );
		return 0;
	}

	return QWidget::event( ev );
}

void SongEditorPositionRuler::songModeActivationEvent() {
	updatePosition();
	createBackground();
	update();
}

void SongEditorPositionRuler::timelineActivationEvent() {
	createBackground();
	update();
}

void SongEditorPositionRuler::jackTimebaseStateChangedEvent() {
	createBackground();
	update();
}

void SongEditorPositionRuler::updateSongEvent( int nValue ) {

	if ( nValue == 0 ) { // different song opened
		updatePosition();
		songSizeChangedEvent();
	}
}

void SongEditorPositionRuler::showToolTip( QHelpEvent* ev ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pTimeline = pHydrogen->getTimeline();
	
	if ( pHydrogen->isTimelineEnabled() &&
		 pTimeline->isFirstTempoMarkerSpecial() &&
		 m_hoveredRow == HoveredRow::TempoMarker &&
		 ev->x() < SongEditor::nMargin + m_nGridWidth ) { // first tempo marker 
		QToolTip::showText( ev->globalPos(), tr( "The tempo set in the BPM widget will be used as a default for the beginning of the song. Left-click to overwrite it." ), this );
		
	} else if ( m_hoveredRow == HoveredRow::Tag ) {
		// Row containing the tags
		int nColumn = std::max( ev->x() - SongEditor::nMargin, 0 ) / m_nGridWidth;
		if ( pTimeline->hasColumnTag( nColumn ) ) {
			QToolTip::showText( ev->globalPos(),
								pTimeline->getTagAtColumn( nColumn ), this );
		}
	}
}

void SongEditorPositionRuler::showTagWidget( int nColumn )
{
	SongEditorPanelTagWidget dialog( this , nColumn );
	dialog.exec();
}

void SongEditorPositionRuler::showBpmWidget( int nColumn )
{
	bool bTempoMarkerPresent =
		Hydrogen::get_instance()->getTimeline()->hasColumnTempoMarker( nColumn );
	m_nActiveBpmWidgetColumn = nColumn;
	update();
	
	SongEditorPanelBpmWidget dialog( this , nColumn, bTempoMarkerPresent );
	dialog.exec();
	
	m_nActiveBpmWidgetColumn = -1;
	update();
}


void SongEditorPositionRuler::mousePressEvent( QMouseEvent *ev )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
		
	int nColumn = ( std::max( ev->x() - SongEditor::nMargin, 0 ) / m_nGridWidth);
	
	if (ev->button() == Qt::LeftButton ) {
		if ( ev->y() > 22 ) {
			// Relocate transport using position ruler
			m_bRightBtnPressed = false;

			if ( nColumn > (int) m_pHydrogen->getSong()->getPatternGroupVector()->size() ) {
				return;
			}

			if ( m_pHydrogen->getMode() == Song::Mode::Pattern ) {
				pCoreActionController->activateSongMode( true );
				m_pHydrogen->setIsModified( true );
			}

			m_pHydrogen->getCoreActionController()->locateToColumn( nColumn );
			update();
		} else if ( ev->y() > 22 - 1 - m_nTagHeight ) {
			showTagWidget( nColumn );
			
		} else if ( m_pHydrogen->isTimelineEnabled() ){
			showBpmWidget( nColumn );
		}
	} else if ( ev->button() == Qt::MiddleButton ) {
		showTagWidget( nColumn );
	} else if (ev->button() == Qt::RightButton && ev->y() >= 26) {
		Preferences* pPref = Preferences::get_instance();
		if ( nColumn >= (int) m_pHydrogen->getSong()->getPatternGroupVector()->size() ) {
			pPref->unsetPunchArea();
			return;
		}
		if ( m_pHydrogen->getMode() == Song::Mode::Pattern ) {
			return;
		}
		m_bRightBtnPressed = true;
		// Disable until mouse is moved
		pPref->setPunchInPos( nColumn );
		pPref->setPunchOutPos(-1);
		update();
	}

}




void SongEditorPositionRuler::mouseReleaseEvent(QMouseEvent *ev)
{
	UNUSED( ev );
	m_bRightBtnPressed = false;
}


void SongEditorPositionRuler::paintEvent( QPaintEvent *ev )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pSongEditor = pHydrogenApp->getSongEditorPanel()->getSongEditor();
	auto pTimeline = m_pHydrogen->getTimeline();
	auto pPref = Preferences::get_instance();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	
	if (!isVisible()) {
		return;
	}
	
	QColor textColor( pPref->getColorTheme()->m_songEditor_textColor );
	QColor textColorAlpha( textColor );
	textColorAlpha.setAlpha( 45 );
	QColor highlightColor = pPref->getColorTheme()->m_highlightColor;
	QColor colorHovered( highlightColor );
	colorHovered.setAlpha( 200 );
	QColor backgroundColor = pPref->getColorTheme()->m_songEditor_alternateRowColor.darker( 115 );
	QColor backgroundColorTempoMarkers = backgroundColor.darker( 120 );

	int pIPos = Preferences::get_instance()->getPunchInPos();
	int pOPos = Preferences::get_instance()->getPunchOutPos();

	QPainter painter(this);
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ) {
		createBackground();
	}
	QRectF srcRect(
			pixelRatio * ev->rect().x(),
			pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(),
			pixelRatio * ev->rect().height()
	);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, srcRect );
	
	// Which tempo marker is the currently used one?
	int nCurrentTempoMarkerColumn = -1;
	for ( const auto& tempoMarker : tempoMarkerVector ) {
		if ( tempoMarker->nColumn > m_nColumn ) {
			break;
		}
		nCurrentTempoMarkerColumn = tempoMarker->nColumn;
	}
	if ( nCurrentTempoMarkerColumn == -1 &&
		 tempoMarkerVector.size() != 0 ) {
		auto pTempoMarker = tempoMarkerVector[ tempoMarkerVector.size() - 1 ];
		if ( pTempoMarker != nullptr ) {
			nCurrentTempoMarkerColumn = pTempoMarker->nColumn;
		}
	}
	if ( nCurrentTempoMarkerColumn != -1 ) {
		auto pTempoMarker = pTimeline->getTempoMarkerAtColumn( nCurrentTempoMarkerColumn );
		if ( pTempoMarker != nullptr ) {
			drawTempoMarker( pTempoMarker, true, painter );
		}
	}

	// Draw playhead
	if ( m_fTick != -1 ) {
		int nX = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								   m_fTick * static_cast<float>(m_nGridWidth) -
								   static_cast<float>(Skin::nPlayheadWidth) / 2 );
		int nShaftOffset = Skin::getPlayheadShaftOffset();
		Skin::drawPlayhead( &painter, nX, height() / 2 + 2, false );
		painter.drawLine( nX + nShaftOffset, 0, nX + nShaftOffset, height() );
	}
			
	// Highlight hovered tick of the Timeline
	if ( m_hoveredRow == HoveredRow::Tag ||
		 ( m_hoveredRow == HoveredRow::TempoMarker &&
		   m_pHydrogen->isTimelineEnabled() ) ) {

		painter.setPen( QPen( highlightColor, 1 ) );
		int x = SongEditor::nMargin + m_nHoveredColumn * m_nGridWidth;

		painter.drawLine( x, 1, x, 4 );
		painter.drawLine( x, height() / 2 - 5, x, height() / 2 );
	}

	// Highlight tag
	bool bTagPresent = false;
	if ( m_hoveredRow == HoveredRow::Tag &&
		 pTimeline->hasColumnTag( m_nHoveredColumn ) ) {

		int x = SongEditor::nMargin + m_nHoveredColumn * m_nGridWidth + 4;
		QRect rect( x, height() / 2 - 1 - m_nTagHeight,
					m_nGridWidth - 6, m_nTagHeight );
	
		QFont font2( pPref->getApplicationFontFamily(), 5 );
		painter.setFont( font2 );
		
		painter.fillRect( rect, pPref->getColorTheme()->m_highlightColor );
		painter.setPen( pPref->getColorTheme()->m_highlightedTextColor );
		painter.drawText( rect, Qt::AlignCenter, "T");

		painter.setFont( font );
		bTagPresent = true;
	}

	// Draw a slight highlight around the tempo marker hovered using
	// mouse or touch events. This will also redraw the
	// tempo marker to ensure it's visible (they can overlap with
	// neighboring ones and be hardly readable).
	bool bTempoMarkerPresent = false;
	if ( ! bTagPresent &&
		 ( ( m_pHydrogen->isTimelineEnabled() &&
			 m_hoveredRow == HoveredRow::TempoMarker ) ||
		   m_nActiveBpmWidgetColumn != -1 ) ) {

		int nColumn;
		if ( m_nActiveBpmWidgetColumn != -1 ) {
			nColumn = m_nActiveBpmWidgetColumn;
		} else {
			nColumn = m_nHoveredColumn;
		}

		if ( pTimeline->hasColumnTempoMarker( nColumn ) ||
			 ( pTimeline->isFirstTempoMarkerSpecial() &&
			   nColumn == 0 ) ) {
		
			QRect rect( SongEditor::nMargin - SONG_EDITOR_MAX_GRID_WIDTH +
						nColumn * m_nGridWidth + m_nGridWidth / 2,
						6, 2 * SONG_EDITOR_MAX_GRID_WIDTH, 12 );
			painter.fillRect( rect, backgroundColorTempoMarkers );

			auto pTempoMarker = pTimeline->getTempoMarkerAtColumn( nColumn );
			if ( pTempoMarker != nullptr ) {
				drawTempoMarker( pTempoMarker,
								 pTempoMarker->nColumn == nCurrentTempoMarkerColumn, // emphasize
								 painter );
			}
				
			if ( m_nActiveBpmWidgetColumn == -1 ) {
				painter.setPen( QPen( colorHovered, 1 ) );
			} else {
				painter.setPen( QPen( highlightColor, 1 ) );
			}
			painter.drawRect( rect );

			bTempoMarkerPresent = true;
		}
	}

	// Draw hovering highlights in tempo marker row
	if ( ( m_nHoveredColumn > -1 &&
		   ( ( m_hoveredRow == HoveredRow::Tag && !bTagPresent ) ||
			 ( m_hoveredRow == HoveredRow::TempoMarker &&
			   m_pHydrogen->isTimelineEnabled() &&
			   ! bTempoMarkerPresent ) ) ) ||
		 ( m_nActiveBpmWidgetColumn != -1 &&
		   ! bTempoMarkerPresent ) ) {

		QColor color;
		if ( m_nActiveBpmWidgetColumn == -1 ) {
			color = colorHovered;
		} else {
			color = highlightColor;
		}
		QPen p( color );
		p.setWidth( 1 );
		painter.setPen( p );

		int nCursorX;
		if ( m_nActiveBpmWidgetColumn != -1 ) {
			nCursorX = SongEditor::nMargin +
				m_nActiveBpmWidgetColumn * m_nGridWidth + 3;
		} else {
			nCursorX = SongEditor::nMargin +
				m_nHoveredColumn * m_nGridWidth + 3;
		}

		if ( m_hoveredRow == HoveredRow::TempoMarker ||
			 m_nActiveBpmWidgetColumn != -1 ) {
			painter.drawRect( nCursorX, 6, m_nGridWidth - 5, 12 );
		} else {
			painter.drawRect( nCursorX, height() / 2 - 1 - m_nTagHeight,
							  m_nGridWidth - 5, m_nTagHeight - 1 );
		}
	}

	// Draw cursor
	if ( ! pHydrogenApp->hideKeyboardCursor() && pSongEditor->hasFocus() ) {
		int nCursorX = SongEditor::nMargin +
			pSongEditor->getCursorColumn() * m_nGridWidth + 2;

		QColor cursorColor = pPref->getColorTheme()->m_cursorColor;

		QPen p( cursorColor );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setRenderHint( QPainter::Antialiasing );
		// Aim to leave a visible gap between the border of the
		// pattern cell, and the cursor line, for consistency and
		// visibility.
		painter.drawLine( nCursorX, height() / 2 + 3,
						  nCursorX + m_nGridWidth - 3, height() / 2 + 3 );
		painter.drawLine( nCursorX, height() / 2 + 4,
						  nCursorX, height() / 2 + 5 );
		painter.drawLine( nCursorX + m_nGridWidth - 3, height() / 2 + 4,
						  nCursorX + m_nGridWidth - 3, height() / 2 + 5 );
		painter.drawLine( nCursorX, height() - 4,
						  nCursorX + m_nGridWidth - 3, height() - 4 );
		painter.drawLine( nCursorX, height() - 6,
						  nCursorX, height() - 5 );
		painter.drawLine( nCursorX + m_nGridWidth - 3, height() - 6,
						  nCursorX + m_nGridWidth - 3, height() - 5 );
	}

	// Faint playhead over hovered position marker.
	if ( m_nHoveredColumn > -1  &&
		 m_hoveredRow == HoveredRow::Ruler &&
		 m_nHoveredColumn <= m_nActiveColumns ) {

		int x = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								  static_cast<float>(m_nHoveredColumn) *
								  static_cast<float>(m_nGridWidth) -
								  static_cast<float>(Skin::nPlayheadWidth) / 2 );
		int nShaftOffset = Skin::getPlayheadShaftOffset();
		Skin::drawPlayhead( &painter, x, height() / 2 + 2, true );
		painter.drawLine( x + nShaftOffset, 0, x + nShaftOffset, height() / 2 + 1 );
		painter.drawLine( x + nShaftOffset, height() / 2 + 2 + Skin::nPlayheadHeight,
						  x + nShaftOffset, height() );
	}

	if ( pIPos <= pOPos ) {
		int xIn = (int)( SongEditor::nMargin + pIPos * m_nGridWidth );
		int xOut = (int)( 9 + (pOPos+1) * m_nGridWidth );
		painter.fillRect( xIn, 30, xOut-xIn+1, 12, QColor(200, 100, 100, 100) );
		QPen pen(QColor(200, 100, 100));
		painter.setPen(pen);
		painter.drawRect( xIn, 30, xOut-xIn+1, 12 );
	}

}

void SongEditorPositionRuler::drawTempoMarker( std::shared_ptr<const Timeline::TempoMarker> tempoMarker, bool bEmphasize, QPainter& painter ) {

	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pTimeline = pHydrogen->getTimeline();
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );

	QColor tempoMarkerColor;
	tempoMarkerColor = pPref->getColorTheme()->m_songEditor_textColor;
	if ( ! pHydrogen->isTimelineEnabled() ) {
		tempoMarkerColor.setAlpha( 45 );
	}
	
	if ( tempoMarker->nColumn == 0 && pTimeline->isFirstTempoMarkerSpecial() ) {
		if ( ! pHydrogen->isTimelineEnabled() ) {
			// Omit the special tempo marker.
			return;
		}
		painter.setPen( tempoMarkerColor.darker( 150 ) );
	} else {
		painter.setPen( tempoMarkerColor );
	}
				
	if ( bEmphasize ) {
		font.setBold( true );
	}
	painter.setFont( font );
		
	QRect rect( SongEditor::nMargin - SONG_EDITOR_MAX_GRID_WIDTH +
				tempoMarker->nColumn * m_nGridWidth + m_nGridWidth / 2,
				6, 2 * SONG_EDITOR_MAX_GRID_WIDTH, 12 );
				
	char tempo[10];
	sprintf( tempo, "%d",  static_cast<int>( tempoMarker->fBpm ) );
	painter.drawText( rect, Qt::AlignCenter, tempo );

	if ( bEmphasize ) {
		font.setBold( false );
	}
	painter.setFont( font );
}

void SongEditorPositionRuler::updatePosition()
{
	auto pTimeline = m_pHydrogen->getTimeline();
	auto pPref = Preferences::get_instance();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	
	float fTick = m_pAudioEngine->getColumn();

	m_pAudioEngine->lock( RIGHT_HERE );

	auto pPatternGroupVector = m_pHydrogen->getSong()->getPatternGroupVector();
	m_nColumn = std::max( m_pAudioEngine->getColumn(), 0 );

	if ( pPatternGroupVector->size() > m_nColumn &&
		 pPatternGroupVector->at( m_nColumn )->size() > 0 ) {
		int nLength = pPatternGroupVector->at( m_nColumn )->longest_pattern_length();
		fTick += (float)m_pAudioEngine->getPatternTickPosition() / (float)nLength;
	} else {
		// Empty column. Use the default length.
		fTick += (float)m_pAudioEngine->getPatternTickPosition() / (float)MAX_NOTES;
	}

	if ( m_pHydrogen->getMode() == Song::Mode::Pattern ) {
		fTick = -1;
	}
	else if ( fTick < 0 ) {
		// As some variables of the audio engine are initialized as or
		// reset to -1 we ensure this does not affect the position of
		// the playhead in the SongEditor.
		fTick = 0;
	}

	m_pAudioEngine->unlock();

	if ( fTick != m_fTick ) {
		float fDiff = static_cast<float>(m_nGridWidth) * (fTick - m_fTick);

		m_fTick = fTick;
		int nX = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								   m_fTick * static_cast<float>(m_nGridWidth) -
								   static_cast<float>(Skin::nPlayheadWidth) / 2 );

		QRect updateRect( nX -2, 0, 4 + Skin::nPlayheadWidth, height() );
		update( updateRect );
		if ( fDiff > 1.0 || fDiff < -1.0 ) {
			// New cursor is far enough away from the old one that the single update rect won't cover both. So
			// update at the old location as well.
			updateRect.translate( -fDiff, 0 );
			update( updateRect );
		}

		auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
		if ( pSongEditorPanel != nullptr ) {
			pSongEditorPanel->getSongEditor()->updatePosition( fTick );
			pSongEditorPanel->getPlaybackTrackWaveDisplay()->updatePosition( fTick );
			pSongEditorPanel->getAutomationPathView()->updatePosition( fTick );
		}
	}
}

void SongEditorPositionRuler::timelineUpdateEvent( int nValue )
{
	createBackground();
	update();
}

void SongEditorPositionRuler::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
			 
		resize( SongEditor::nMargin +
				Preferences::get_instance()->getMaxBars() * m_nGridWidth, height() );
		createBackground();
		update();
	}
}

