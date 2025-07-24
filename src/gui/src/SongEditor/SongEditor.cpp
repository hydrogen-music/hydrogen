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

#include "SongEditor.h"

#include <assert.h>
#include <algorithm>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include "UndoActions.h"
#include "SongEditorPanel.h"
#include "SongEditorPatternList.h"
#include "SongEditorPositionRuler.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

using namespace H2Core;

SongEditor::SongEditor( QWidget *parent, QScrollArea *pScrollView,
						SongEditorPanel *pSongEditorPanel )
	: Editor::Base<Elem>( parent )
	, m_pScrollView( pScrollView )
	, m_pSongEditorPanel( pSongEditorPanel )
{
	m_instance = Editor::Instance::SongEditor;
	m_type = Editor::Type::Grid;

	const auto pPref = Preferences::get_instance();

	connect( m_pScrollView->verticalScrollBar(), SIGNAL( valueChanged( int ) ),
			 this, SLOT( scrolled( int ) ) );
	connect( m_pScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 this, SLOT( scrolled( int ) ) );

	setAttribute( Qt::WA_OpaquePaintEvent );
	setFocusPolicy( Qt::StrongFocus );
	setMouseTracking( true );

	m_nGridWidth = pPref->getSongEditorGridWidth();
	m_nGridHeight = pPref->getSongEditorGridHeight();

	m_nCursorRow = 0;
	m_nCursorColumn = 0;

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		m_nEditorHeight = pSong->getPatternList()->size() * m_nGridWidth;
	}
	else {
		m_nEditorHeight = SongEditor::nMinimumHeight;
	}
	m_nEditorWidth = SongEditor::nMargin + pPref->getMaxBars() * m_nGridWidth;

	resize( QSize( m_nEditorWidth, m_nEditorHeight ) );
	updatePixmapSize();

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, [&]() {
		cut();
	});
	m_pPopupMenu->addAction( tr( "&Copy" ), this, [&]() {
		copy();
	});
	m_pPopupMenu->addAction( tr( "&Paste" ), this, [&]() {
		paste();
	});
	m_pPopupMenu->addAction( tr( "&Delete" ), this, [&]() {
		deleteSelection();
	});
	m_pPopupMenu->addAction( tr( "Select &all" ), this, [&]() {
		selectAll();
	});
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, [&]() {
		clearSelection();
	});
	m_pPopupMenu->setObjectName( "SongEditorPopup" );
}

SongEditor::~SongEditor() {
}

void SongEditor::addOrRemovePatternCellAction( const QPoint& gridPoint,
											   Editor::Action action ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( gridPoint.x() < 0 || gridPoint.y() >= pSong->getPatternList()->size() ||
		 gridPoint.y() < 0 ) {
		return;
	}

	const bool bGridPointActive = pSong->isPatternActive(
		gridPoint.x(), gridPoint.y() );

	if ( ( action == Editor::Action::Toggle ) ||
		 ( ( action == Editor::Action::Add ) && ! bGridPointActive ) ||
		 ( ( action == Editor::Action::Delete ) && bGridPointActive ) ) {
		CoreActionController::toggleGridCell( gridPoint.x(), gridPoint.y() );
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
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	const int nScroll = pScrollArea->verticalScrollBar()->value();
	int nHeight = pScrollArea->height();

	auto pPlayingPatterns = pAudioEngine->getPlayingPatterns();

	// If no patterns are playing, no scrolling needed either.
	if ( pPlayingPatterns->size() == 0 || pSong == nullptr ) {
		return nScroll;
	}

	pAudioEngine->lock( RIGHT_HERE );

	auto pSongPatterns = pSong->getPatternList();

	// Duplicate the playing patterns vector before finding the pattern numbers of the playing patterns. This
	// avoids doing a linear search in the critical section.
	std::vector<std::shared_ptr<Pattern>> currentPatterns;
	for ( int ii = 0; ii < pPlayingPatterns->size(); ++ii ) {
		currentPatterns.push_back( pPlayingPatterns->get( ii ) );
	}
	pAudioEngine->unlock();

	std::vector<int> playingRows;
	for ( const auto& pPattern : currentPatterns ) {
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



void SongEditor::setGridWidth( int nNewWidth ) {
	const int nWidth = std::clamp(
		nNewWidth, SongEditor::nMinGridWidth, SongEditor::nMaxGridWidth );
	if ( m_nGridWidth != nWidth ) {
		m_nGridWidth = nWidth;
		updateWidth();
		updateEditor();
	}
}

QPoint SongEditor::xyToColumnRow( const QPoint& p ) const
{
	return QPoint( (p.x() - SongEditor::nMargin) / (int)m_nGridWidth, p.y() / (int)m_nGridHeight );
}

QPoint SongEditor::columnRowToXy( const QPoint& p ) const
{
	return QPoint( SongEditor::nMargin + p.x() * m_nGridWidth, p.y() * m_nGridHeight );
}


void SongEditor::togglePatternActive( int nColumn, int nRow ) {
	SE_togglePatternAction *action = new SE_togglePatternAction( nColumn, nRow );
	HydrogenApp::get_instance()->pushUndoCommand( action );
}

void SongEditor::setPatternActive( int nColumn, int nRow, bool bActivate )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	bool bPatternIsActive = pSong->isPatternActive( nColumn, nRow );

	if ( bPatternIsActive && ! bActivate || ! bPatternIsActive && bActivate ) {
		pHydrogenApp->pushUndoCommand(
			new SE_togglePatternAction( nColumn, nRow ) );
	}
}


void SongEditor::selectAll() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
	auto pColumns = pSong->getPatternGroupVector();
	m_selection.clearSelection();
	for ( int nRow = 0; nRow < pPatternList->size(); nRow++ ) {
		auto pPattern = pPatternList->get( nRow );
		for ( int nCol = 0; nCol < pColumns->size(); nCol++ ) {
			auto pColumn = ( *pColumns )[ nCol ];
			for ( int i = 0; i < pColumn->size(); i++) {
				if ( pColumn->get(i) == pPattern ) { // esiste un pattern in questa posizione
					m_selection.addToSelection( QPoint( nCol, nRow ) );
				}
			}
		}
	}
	updateEditor( true );
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
	int nDeltaColumn = 0, nDeltaRow = 0;
	auto pSong = Hydrogen::get_instance()->getSong();
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

			const auto pCommonStrings =
				HydrogenApp::get_instance()->getCommonStrings();

			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_modifyPatternCellsAction(
					addCells, deleteCells, mergeCells,
					pCommonStrings->getActionPastePatternCells() ) );
		}
	}
}

void SongEditor::keyPressEvent( QKeyEvent * ev )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	const int nBlockSize = 5, nWordSize = 5;
	
	bool bIsSelectionKey = false;
	bool bUnhideCursor = true;

	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	H2Core::Song::ActionMode actionMode = pHydrogen->getActionMode();
		
	if ( actionMode == H2Core::Song::ActionMode::selectMode ) {
		bIsSelectionKey = m_selection.keyPressEvent( ev );
	}

	auto pPatternList = pSong->getPatternList();
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
			clearSelection();
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
		updateEditor( true );
	}
	if ( m_selection.isMoving() ) {
		// If a selection is being moved, it will need to be repainted
		updateEditor( true );
	}

	QPoint cursorCentre = columnRowToXy( QPoint( m_nCursorColumn, m_nCursorRow ) ) + centre;
	m_pScrollView->ensureVisible( cursorCentre.x(), cursorCentre.y() );
	m_selection.updateKeyboardCursorPosition();

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
	updateEditor();

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
	updateEditor();

	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPatternList()->update();
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler()->update();
	}
}

void SongEditor::handleElements( QInputEvent* pEvent, Editor::Action action ) {

	// Retrieve the coordinates
	QPoint point;
	if ( dynamic_cast<QMouseEvent*>(pEvent) != nullptr ) {
		// Element added via mouse.
		auto pEv = static_cast<MouseEvent*>( pEvent );
		point = xyToColumnRow( pEv->position().toPoint() );
	}
	else if ( dynamic_cast<QKeyEvent*>(pEvent) != nullptr ) {
		point = QPoint( m_nCursorColumn, m_nCursorRow );
	}
	else {
		ERRORLOG( "Unknown event" );
		return;
	}

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_addOrRemovePatternCellAction( point, action ) );
}

void SongEditor::deleteElements( std::vector<QPoint> points ) {
	if ( points.size() == 0 ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	pHydrogenApp->beginUndoMacro( pCommonStrings->getActionDeletePatternCells() );
	for ( const auto& ppoint : points ) {
		pHydrogenApp->pushUndoCommand(
			new SE_addOrRemovePatternCellAction(
				ppoint, Editor::Action::Delete ) );
	}
	pHydrogenApp->endUndoMacro();
}

std::vector<QPoint> SongEditor::getElementsAtPoint( const QPoint& point,
													int /*nCursorMargin*/,
													std::shared_ptr<Pattern> ) {
	// Cursor margin and pattern aren't used within the song editor.
	std::vector<QPoint> vec;

	const auto gridPoint = xyToColumnRow( point ) ;

	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr &&
		 pSong->isPatternActive( gridPoint.x(), gridPoint.y() ) ) {
		vec.push_back( gridPoint );
	}

	return std::move( vec );
}

bool SongEditor::updateMouseHoveredElements( QMouseEvent* pEvent ) {
	// Check whether the mouse pointer is Outside of the current widget.
	const QPoint globalPos = QCursor::pos();
	const QPoint widgetPos = mapFromGlobal( globalPos );
	if ( widgetPos.x() < 0 || widgetPos.x() >= width() ||
		 widgetPos.y() < 0 || widgetPos.y() >= height() ) {
		if ( m_hoveredCells.size() > 0 ) {
			m_hoveredCells.clear();
			return true;
		}
		else {
			return false;
		}
	}

	if ( pEvent == nullptr ) {
		// The update was triggered outside of one of Qt's mouse events. We have
		// to create an artifical one instead.
		pEvent = new QMouseEvent(
			QEvent::MouseButtonRelease, widgetPos, globalPos, Qt::LeftButton,
			Qt::LeftButton, Qt::NoModifier );
	}
	const auto pEv = static_cast<MouseEvent*>( pEvent );

	const auto hoveredCells = getElementsAtPoint( pEv->position().toPoint(), 0 );
	if ( hoveredCells.size() == m_hoveredCells.size() ) {
		bool bIdentical = true;
		for ( int ii = 0; ii < hoveredCells.size(); ++ii ) {
			if ( hoveredCells[ ii ] != m_hoveredCells[ ii ] ) {
				bIdentical = false;
				break;
			}
		}

		if ( bIdentical ) {
			return false;
		}
	}

	// The current and the last hovered elments differ
	m_hoveredCells.clear();
	for ( auto& ppoint : hoveredCells ) {
		m_hoveredCells.push_back( ppoint );
	}

	return true;
}

void SongEditor::mouseDrawStart( QMouseEvent* pEvent ) {
	auto pEv = static_cast<MouseEvent*>( pEvent );
	m_drawPreviousPosition = pEv->position();
}

void SongEditor::mouseDrawUpdate( QMouseEvent* pEvent ) {
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pEv = static_cast<MouseEvent*>( pEvent );

	const auto end = pEv->position();

	// Check whether we are still at the same grid point as in the last update.
	// We do not want to toggle the same note twice.
	const QPoint endGridPoint = xyToColumnRow( end.toPoint() );
	if ( endGridPoint.y() == m_nDrawPreviousColumn &&
		 endGridPoint.x() == m_nDrawPreviousRow ) {
		m_drawPreviousPosition = end;
		return;
	}

	// Toggle all cells between this and the previous position in individual
	// undo/redo actions bundled into a single undo macro.

	const auto start = m_drawPreviousPosition;
	const auto sUndoContext =
		QString( "%1::draw" ).arg( Editor::instanceToQString( m_instance ) );

	// We assume the cursor path was a straight line between both points
	// ( y = fM * x + fN ).
	double fM;
	if ( end.x() != start.x() ) {
		fM = std::min( std::abs( ( end.y() - start.y() ) /
								 ( end.x() - start.x() ) ),
					   static_cast<double>(m_nGridHeight) / 2 );
	} else {
		fM = static_cast<double>(m_nGridHeight) / 2;
	}

	// Since we have to properly handle all hovered notes (already present) we
	// have to assume the smallest possible resolution: grid on the x axis being
	// turned off. We will project this smallest increment onto the straight
	// line while ensuring we do not miss rows on almost vertical movements to
	// get our increment.
	const QPointF increment( start.x() <= end.x() ? 1 : -1,
							 start.y() <= end.y() ? fM : ( -1 * fM ) );

	int nLastColumn = m_nDrawPreviousColumn;
	int nLastRow = m_nDrawPreviousRow;
	// Since we can only toggle notes on the grid, we use the projection of the
	// movement on the x axis to drive the loop. This ensures that we are always
	// on grid.
	for ( auto ppoint = start; ( ppoint - start ).manhattanLength() <=
			  ( end - start ).manhattanLength(); ppoint += increment ) {
		// We prioritize existing notes
		const QPoint gridPoint = xyToColumnRow( ppoint.toPoint() );
		if ( gridPoint.x() != nLastRow || gridPoint.y() != nLastColumn ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					gridPoint, Editor::Action::Toggle ), sUndoContext );
			nLastRow = gridPoint.x();
			nLastColumn = gridPoint.y();
		}
	}

	m_drawPreviousPosition = end;
	m_nDrawPreviousColumn = nLastColumn;
	m_nDrawPreviousRow = nLastRow;
}

void SongEditor::mouseDrawEnd() {
	HydrogenApp::get_instance()->endUndoContext();

	m_drawPreviousPosition = QPointF( 0, 0 );
	m_nDrawPreviousColumn = -1;
	m_nDrawPreviousRow = -1;
}

void SongEditor::updateAllComponents( bool bContentOnly ) {
	updateVisibleComponents( bContentOnly );
}

void SongEditor::updateVisibleComponents( bool bContentOnly ) {
	auto pHydrogenApp = HydrogenApp::get_instance();
	pHydrogenApp->getSongEditorPanel()->getSongEditorPatternList()->update();
	pHydrogenApp->getSongEditorPanel()->getSongEditorPositionRuler()->update();
	updateEditor( bContentOnly );
}

bool SongEditor::updateWidth() {
	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return false ;
	}

	const int nEditorWidth =
		SongEditor::nMargin + pPref->getMaxBars() * m_nGridHeight;
	const int nEditorHeight = std::max(
		pSong->getPatternList()->size() * m_nGridHeight,
		SongEditor::nMinimumHeight );

	if ( m_nEditorHeight != nEditorHeight || m_nEditorWidth != nEditorWidth ) {
		m_nEditorHeight = nEditorHeight;
		m_nEditorWidth = nEditorWidth;
		resize( m_nEditorWidth, m_nEditorHeight );
		updatePixmapSize();
		return true;
	}

	return false;
}

void SongEditor::selectionMoveEndEvent( QInputEvent *ev )
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
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

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_modifyPatternCellsAction(
			addCells, deleteCells, mergeCells,
			(m_bCopyNotMove ? pCommonStrings->getActionCopyPatternCells()
			 : pCommonStrings->getActionMovePatternCells() ) ) );
}

//! Modify pattern cells by first deleting some, then adding some.
//! deleteCells and addCells *may* safely overlap
void SongEditor::modifyPatternCellsAction( const std::vector<QPoint>& addCells,
										   const std::vector<QPoint>& deleteCells,
										   const std::vector<QPoint>& selectCells ) {
	
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
		bool bCellBoundaryCrossed = xyToColumnRow( m_previousMousePosition ) !=
			xyToColumnRow( m_currentMousePosition );
		// Selection must redraw the pattern when a cell boundary is crossed, as the selected cells are
		// drawn when drawing the pattern.
		if ( bCellBoundaryCrossed ) {
			m_update = Editor::Update::Content;
		}
		update();
	} else {
		// Other reasons: force update
		m_update = Editor::Update::Content;
		update();
	}
	m_previousMousePosition = m_currentMousePosition;
}

void SongEditor::updatePosition( float fTick ) {
	if ( fTick != m_fTick ) {
		float fDiff = static_cast<float>(m_nGridWidth) * (fTick - m_fTick);
		m_fTick = fTick;
		int nX = SongEditorPositionRuler::tickToColumn( m_fTick, m_nGridWidth );
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

void SongEditor::paintEvent( QPaintEvent *ev ) {
	const qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_update == Editor::Update::Background ) {
		createBackground();
	}

	if ( m_update == Editor::Update::Background ||
		 m_update == Editor::Update::Content ) {
		drawSequence();
		m_update = Editor::Update::None;
	}

	const auto pPref = Preferences::get_instance();

	QColor hoverColor;
	QColor selectionColor = pPref->getTheme().m_color.m_selectionHighlightColor;
	int nFactor = 100;
	// if ( noteStyle & NoteStyle::Selected && noteStyle & NoteStyle::Hovered ) {
	// 	nFactor = 107;
	// }
	// else if ( noteStyle & NoteStyle::Hovered ) {
		nFactor = 125;
//}

	//if ( noteStyle & NoteStyle::Hovered ) {
		// Depending on the highlight color, we make it either darker or
		// lighter.
		if ( Skin::moreBlackThanWhite( selectionColor ) ) {
			hoverColor = selectionColor.lighter( nFactor );
		} else {
			hoverColor = selectionColor.darker( nFactor );
		}
	//}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pContentPixmap, ev->rect() );

	// Draw hovered cells
	for ( const auto& ppoint : m_hoveredCells ) {
		QPen p( hoverColor );
		painter.setPen( p );
		const int nWidth = m_gridCells[ ppoint ].m_fWidth * m_nGridWidth;
		const QRect r = QRect( columnRowToXy( ppoint ),
						 QSize( nWidth, m_nGridHeight ) );
		painter.drawRect( r );
	}

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
		int nX = SongEditorPositionRuler::tickToColumn( m_fTick, m_nGridWidth );
		int nOffset = Skin::getPlayheadShaftOffset();
		Skin::setPlayheadPen( &painter, false );
		painter.drawLine( nX + nOffset, 0, nX + nOffset, height() );
	}

	drawFocus( painter );

	m_selection.paintSelection( &painter );

	// Draw cursor
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() && hasFocus() ) {
		QPen p( pPref->getTheme().m_color.m_cursorColor );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setRenderHint( QPainter::Antialiasing );
		// Aim to leave a visible gap between the border of the
		// pattern cell, and the cursor line, for consistency and
		// visibility.
		painter.drawRoundedRect(
			QRect( QPoint( 0, 1 ) +
				   columnRowToXy( QPoint( m_nCursorColumn, m_nCursorRow ) ),
				   QSize( m_nGridWidth + 1, m_nGridHeight - 1 ) ), 4, 4 );
	}
}

void SongEditor::drawFocus( QPainter& painter ) {

	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	QColor color = H2Core::Preferences::get_instance()->getTheme().m_color.m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	int nStartX = m_pScrollView->horizontalScrollBar()->value();
	int nEndX = std::min( nStartX + m_pScrollView->viewport()->size().width(), width() );
	int nStartY = m_pScrollView->verticalScrollBar()->value();
	int nEndY = std::min(
		static_cast<int>( m_nGridHeight ) * pSong->getPatternList()->size(),
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

void SongEditor::createBackground() {
	const auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	const int nPatterns = pSong->getPatternList()->size();
	const int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	const int nMaxPatternSequence = pPref->getMaxBars();

	updatePixmapSize();

	m_pBackgroundPixmap->fill( pPref->getTheme().m_color.m_songEditor_backgroundColor );

	QPainter p( m_pBackgroundPixmap );
	
	for ( int ii = 0; ii < nPatterns + 1; ii++) {
		if ( ( ii % 2 ) == 0 &&
			 ii != nSelectedPatternNumber ) {
			continue;
		}
		
		int y = m_nGridHeight * ii;
		
		if ( ii == nSelectedPatternNumber ) {
			p.fillRect( 0, y, nMaxPatternSequence * m_nGridWidth, m_nGridHeight,
						pPref->getTheme().m_color.m_songEditor_selectedRowColor );
		} else {
			p.fillRect( 0, y, nMaxPatternSequence * m_nGridWidth, m_nGridHeight,
						pPref->getTheme().m_color.m_songEditor_alternateRowColor );
		}
	}

	p.setPen( QPen( pPref->getTheme().m_color.m_songEditor_lineColor, 1,
					Qt::DotLine ) );

	// vertical lines
	for ( float ii = 0; ii <= nMaxPatternSequence + 1; ii++) {
		float x = SongEditor::nMargin + ii * m_nGridWidth;
		p.drawLine( x, 0, x, m_nGridHeight * nPatterns );
	}
	
	// horizontal lines
	for (int i = 0; i < nPatterns; i++) {
		int y = m_nGridHeight * i;

		p.drawLine( 0, y, (nMaxPatternSequence * m_nGridWidth), y );
	}
}

// Update the GridCell representation.
void SongEditor::updateGridCells() {

	m_gridCells.clear();
	auto pSong = Hydrogen::get_instance()->getSong();
	auto pPatternList = pSong->getPatternList();
	auto pColumns = pSong->getPatternGroupVector();

	for ( int nColumn = 0; nColumn < pColumns->size(); nColumn++ ) {
		auto pColumn = (*pColumns)[nColumn];
		int nMaxLength = pColumn->longestPatternLength();

		for ( int nPat = 0; nPat < pColumn->size(); nPat++ ) {
			auto pPattern = (*pColumn)[ nPat ];
			int y = pPatternList->index( pPattern );
			assert( y != -1 );
			GridCell *pCell = &( m_gridCells[ QPoint( nColumn, y ) ] );
			pCell->m_bActive = true;
			pCell->m_fWidth = (float) pPattern->getLength() / nMaxLength;

			for ( const auto& pVPattern : *( pPattern->getFlattenedVirtualPatterns() ) ) {
				GridCell *pVCell = &( m_gridCells[ QPoint( nColumn, pPatternList->index( pVPattern ) ) ] );
				pVCell->m_bDrawnVirtual = true;
				pVCell->m_fWidth = (float) pVPattern->getLength() / nMaxLength;
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

	p.begin( m_pContentPixmap );
	p.drawPixmap( rect(), *m_pBackgroundPixmap, rect() );
	p.end();

	updateGridCells();

	// Draw using GridCells representation
	for ( const auto& it : m_gridCells ) {
		if ( ! m_selection.isSelected( QPoint( it.first.x(), it.first.y() ) ) ) {
			drawPattern( it.first.x(), it.first.y(),
						 it.second.m_bDrawnVirtual, it.second.m_fWidth );
		}
	}
	// We draw all selected patterns in a second run to ensure their
	// border does have the proper color (else the bottom and left one
	// could be overwritten by an adjecent, unselected pattern).
	for ( const auto& it : m_gridCells ) {
		if ( m_selection.isSelected( QPoint( it.first.x(), it.first.y() ) ) ) {
			drawPattern( it.first.x(), it.first.y(),
						 it.second.m_bDrawnVirtual, it.second.m_fWidth );
		}
	}
}



void SongEditor::drawPattern( int nPos, int nNumber, bool bInvertColour, double fWidth )
{
	QPainter p( m_pContentPixmap );
	/*
	 * The default color of the cubes in rgb is 97,167,251.
	 */
	const auto pPref = H2Core::Preferences::get_instance();
	auto pSong = Hydrogen::get_instance()->getSong();
	auto pPatternList = pSong->getPatternList();

	QColor patternColor;
	/*
	 * The following color modes are available:
	 *
	 * Automatic: Steps = Number of pattern in song and colors will be
	 *            chosen internally.
	 * Custom: Number of steps as well as the colors used are defined
	 *            by the user.
	 */
	if ( pPref->getTheme().m_interface.m_coloringMethod ==
		 H2Core::InterfaceTheme::ColoringMethod::Automatic ) {
		int nSteps = pPatternList->size();

		if( nSteps == 0 ) {
			//beware of the division by zero..
			nSteps = 1;
		}

		int nHue = ( (nNumber % nSteps) * (300 / nSteps) + 213) % 300;
		patternColor.setHsv( nHue , 156 , 249);
	} else {
		int nIndex =
			std::clamp( nNumber % pPref->getTheme().m_interface.m_nVisiblePatternColors,
						0, InterfaceTheme::nMaxPatternColors );
		patternColor =
			pPref->getTheme().m_interface.m_patternColors[ nIndex ].toHsv();
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
		if ( hasFocus() || m_bEntered ) {
			borderColor = pPref->getTheme().m_color.m_selectionHighlightColor;
		} else {
			borderColor = pPref->getTheme().m_color.m_selectionInactiveColor;
		}
	} else {
		borderColor = QColor( 0, 0, 0 );
	}
	p.setPen( borderColor );
	p.drawRect( x, y, fWidth * m_nGridWidth, m_nGridHeight );
}

std::vector<SongEditor::SelectionIndex> SongEditor::elementsIntersecting( const QRect& r )
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

void SongEditor::clearThePatternSequenceVector( const QString& filename )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	pAudioEngine->lock( RIGHT_HERE );

	//before deleting the sequence, write a temp sequence file to disk
	pSong->saveTempPatternList( filename );

	auto pPatternGroupsVect = pSong->getPatternGroupVector();
	for (int i = 0; i < pPatternGroupsVect->size(); i++) {
		auto pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
	}
	pPatternGroupsVect->clear();
	pHydrogen->updateSongSize();
	
	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );
	updateEditor( true );
}
