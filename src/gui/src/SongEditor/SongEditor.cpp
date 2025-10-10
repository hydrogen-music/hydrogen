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
#include <core/Basics/GridPoint.h>
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
#include "../MainToolBar/MainToolBar.h"
#include "../Skin.h"

using namespace H2Core;

SongEditor::SongEditor( QWidget *parent, QScrollArea *pScrollView,
						SongEditorPanel *pSongEditorPanel )
	: Editor::Base<Elem>( parent )
	, m_pScrollView( pScrollView )
	, m_pSongEditorPanel( pSongEditorPanel )
	, m_cursor( GridPoint( 0, 0 ) )
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

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		m_nEditorHeight = pSong->getPatternList()->size() * m_nGridWidth;
	}
	else {
		m_nEditorHeight = 0;
	}
	m_nEditorWidth = SongEditor::nMargin + pPref->getMaxBars() * m_nGridWidth;

	resize( QSize( m_nEditorWidth, m_nEditorHeight ) );
	updatePixmapSize();

	// Popup context menu
	m_selectionActions.push_back(
		m_pPopupMenu->addAction( tr( "&Cut" ), this, [&]() {
			popupSetup();
			cut();
			popupTeardown(); } ) );
	m_selectionActions.push_back(
		m_pPopupMenu->addAction( tr( "&Copy" ), this, [&]() {
			popupSetup();
			copy();
			popupTeardown(); } ) );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, [&]() {
		popupSetup();
		paste();
		popupTeardown(); } );
	m_selectionActions.push_back(
		m_pPopupMenu->addAction( tr( "&Delete" ), this, [&]() {
			popupSetup();
			deleteSelection();
			popupTeardown(); } ) );
	m_pPopupMenu->addAction( tr( "Select &all" ), this,
							 [&]() { selectAll();
								 updateVisibleComponents( Editor::Update::Content ); } );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, [&]() {
		m_selection.clearSelection();
		updateVisibleComponents( Editor::Update::Content );
	} );
	connect( m_pPopupMenu, &QMenu::aboutToShow, [&]() {
		popupMenuAboutToShow(); } );
	connect( m_pPopupMenu, &QMenu::aboutToHide, [&]() {
		popupMenuAboutToHide(); } );
}

SongEditor::~SongEditor() {
}

void SongEditor::addOrRemovePatternCellAction( const GridPoint& gridPoint,
											   Editor::Action action,
											   Editor::ActionModifier modifier ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( gridPoint.getColumn() < 0 ||
		 gridPoint.getRow() >= pSong->getPatternList()->size() ||
		 gridPoint.getRow() < 0 ) {
		return;
	}

	std::shared_ptr<GridCell> pOldCell;
	if ( m_gridCells.find( gridPoint ) != m_gridCells.end() ) {
		pOldCell = m_gridCells.at( gridPoint );
	}

	const bool bGridPointActive = pSong->isPatternActive( gridPoint );

	if ( ( action == Editor::Action::Toggle ) ||
		 ( ( action == Editor::Action::Add ) && ! bGridPointActive ) ||
		 ( ( action == Editor::Action::Delete ) && bGridPointActive ) ) {
		CoreActionController::toggleGridCell( gridPoint );
		// Immediate update of all grid cells to allow retrieving the added one
		// to the selection and to get the hovered cells straight.
		updateGridCells();
	}

	if ( static_cast<char>(modifier) &
		 static_cast<char>(Editor::ActionModifier::AddToSelection) &&
		 action != Editor::Action::Delete ) {

		std::shared_ptr<GridCell> pCell;
		if ( m_gridCells.find( gridPoint ) != m_gridCells.end() ) {
			pCell = m_gridCells.at( gridPoint );
		}

		if ( pCell != nullptr && ! m_selection.isSelected( pCell ) ) {
			m_selection.addToSelection( pCell );
		}
	}

	if ( action == Editor::Action::Delete && pOldCell != nullptr ) {
		m_selection.removeFromSelection( pOldCell );
	}
}

/// Calculate a target Y scroll value for tracking a playing song
///
/// Songs with many patterns may not fit in the current viewport of the Song
/// Editor. Depending on how the song is structured, as the viewport scrolls to
/// show different times, the playing patterns may end up being off-screen. It
/// would be ideal to be able to follow the progression of the song in a
/// meaningful and useful way, but since multiple patterns may be active at any
/// one time, it's non-trivial to define a useful behaviour that captures more
/// than the simple case of a song with one pattern active at a time.
///
/// As an attempt to define a useful behaviour which captures what the user
/// might expect to happen, we define the behaviour as follows:
/// * If there are no currently playing patterns which are entirely visible:
///   -> Find the position with the smallest amount of scrolling from the
///      current location which:
///      -> Fits the maximum number of currently playing patterns in view at the
///         same time.
///
/// This covers the trivial cases where only a single pattern is playing, and
/// gives some intuitive behaviour for songs containing multiple playing
/// patterns where the general progression is diagonal but with constant (or
/// near-constant) background elements, and the "minimum scrolling" allows the
/// user to hint if we stray off the path.
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
		updateEditor( Editor::Update::Background );
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
	for ( const auto& [ _, ppCell ] : m_gridCells ) {
		if ( ppCell != nullptr ) {
			m_selection.addToSelection( ppCell );
		}
	}
	updateEditor( Editor::Update::Content );
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
	XMLNode gridPointList = selection.createNode( "gridPointList" );
	XMLNode positionNode = selection.createNode( "sourcePosition" );
	// Top left of selection
	int nMinColumn, nMinRow;
	bool bWrotePattern = false;

	for ( const auto& ppCell : m_selection ) {
		if ( ppCell == nullptr ) {
			continue;
		}
		XMLNode gridPointNode = gridPointList.createNode( "gridPoint" );
		gridPointNode.write_int( "column", ppCell->getColumn() );
		gridPointNode.write_int( "row", ppCell->getRow() );
		if ( bWrotePattern ) {
			nMinColumn = std::min( nMinColumn, ppCell->getColumn() );
			nMinRow = std::min( nMinRow, ppCell->getRow() );
		} else {
			nMinColumn = ppCell->getColumn();
			nMinRow = ppCell->getRow();
			bWrotePattern = true;
		}
	}
	if ( ! bWrotePattern ) {
		nMinColumn = m_cursor.getColumn();
		nMinRow = m_cursor.getRow();
	}
	positionNode.write_int( "column", nMinColumn );
	positionNode.write_int( "row", nMinRow );

	QApplication::clipboard()->setText( doc.toString() );

	// Show the keyboard cursor so the user knows where the insertion point will be
	HydrogenApp::get_instance()->setHideKeyboardCursor( false );
}

void SongEditor::paste() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	XMLDoc doc;
	if ( ! doc.setContent( QApplication::clipboard()->text() ) ) {
		// Pasted something that's not valid XML.
		return;
	}

	m_selection.clearSelection();
	updateGridCells();

	// Retrieves all cells to be activated
	std::vector<GridPoint> newGridPoints, mergedGridPoints;
	const XMLNode selection = doc.firstChildElement( "patternSelection" );
	if ( ! selection.isNull() ) {
		const XMLNode gridPointList = selection.firstChildElement( "gridPointList" );
		if ( gridPointList.isNull() ) {
			return;
		}

		const XMLNode positionNode = selection.firstChildElement( "sourcePosition" );

		// If position information is supplied in the selection, use
		// it to adjust the location relative to the current keyboard
		// input cursor.
		int nDeltaColumn = 0, nDeltaRow = 0;
		if ( ! positionNode.isNull() ) {
			nDeltaColumn = m_cursor.getColumn() -
				positionNode.read_int( "column", m_cursor.getColumn() );
			nDeltaRow = m_cursor.getRow() -
				positionNode.read_int( "row", m_cursor.getRow() );
		}

		const int nMaxRow = pSong->getPatternList()->size() - 1;
		const int nMaxColumn = Preferences::get_instance()->getMaxBars() - 1;
		if ( gridPointList.hasChildNodes() ) {
			for ( XMLNode gridPointNode =
					  gridPointList.firstChildElement( "gridPoint" );
				  ! gridPointNode.isNull();
				  gridPointNode = gridPointNode.nextSiblingElement() ) {
				const int nCol = gridPointNode.read_int(
					"column", m_cursor.getColumn() ) +
					nDeltaColumn;
				const int nRow = gridPointNode.read_int(
					"row", m_cursor.getRow() ) +
					nDeltaRow;
				if ( nCol >= 0 && nRow >= 0 && nRow <= nMaxRow &&
					 nCol <= nMaxColumn ) {
					// Paste cells
					const GridPoint gridPoint( nCol, nRow );
					if ( m_gridCells.find( gridPoint ) == m_gridCells.end() ) {
						// Cell is not active. Activate it.
						newGridPoints.push_back( gridPoint );
					}
					else {
						// This cell already exists. We do not have to add but
						// just to select it.
						mergedGridPoints.push_back( gridPoint );
					}
				}
			}
		}
	}

	if ( newGridPoints.size() > 0 || mergedGridPoints.size() ) {
		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();

		pHydrogenApp->beginUndoMacro( pCommonStrings->getActionPastePatternCells() );
		for ( const auto& ggridPoint : newGridPoints ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					ggridPoint, Editor::Action::Add,
					Editor::ActionModifier::AddToSelection ) );
		}
		for ( const auto& ggridPoint : mergedGridPoints ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					ggridPoint, Editor::Action::None,
					Editor::ActionModifier::AddToSelection ) );
		}
		pHydrogenApp->endUndoMacro();
	}
}

void SongEditor::handleElements( QInputEvent* pEvent, Editor::Action action ) {

	// Retrieve the coordinates
	GridPoint gridPoint;
	if ( dynamic_cast<QMouseEvent*>(pEvent) != nullptr ) {
		// Element added via mouse.
		auto pEv = static_cast<MouseEvent*>( pEvent );
		gridPoint = pointToGridPoint( pEv->position().toPoint(), false );
	}
	else if ( dynamic_cast<QKeyEvent*>(pEvent) != nullptr ) {
		gridPoint = m_cursor;
	}
	else {
		ERRORLOG( "Unknown event" );
		return;
	}

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_addOrRemovePatternCellAction(
			gridPoint, action, Editor::ActionModifier::None ) );
}

void SongEditor::deleteElements( std::vector< std::shared_ptr<GridCell> > cells ) {
	if ( cells.size() == 0 ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	pHydrogenApp->beginUndoMacro( pCommonStrings->getActionDeletePatternCells() );
	for ( const auto& ccell : cells ) {
		if ( ccell != nullptr ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					ccell->getGridPoint(), Editor::Action::Delete,
					Editor::ActionModifier::None ) );
		}
	}
	pHydrogenApp->endUndoMacro();
}

std::vector< std::shared_ptr<GridCell> > SongEditor::getElementsAtPoint(
	const QPoint& point, int /*nCursorMargin*/, bool /*bIncludeHovered*/,
	std::shared_ptr<Pattern> )
{
	// Cursor margin and pattern aren't used within the song editor.
	std::vector< std::shared_ptr<GridCell> > vec;
	const auto gridPoint = pointToGridPoint( point, false ) ;
	if ( m_gridCells.find( gridPoint ) != m_gridCells.end() ) {
		vec.push_back( m_gridCells.at( gridPoint ) );
	}

	return std::move( vec );
}

QPoint SongEditor::elementToPoint( std::shared_ptr<GridCell> pCell ) const {
	if ( pCell == nullptr ) {
		return QPoint( -1, -1 );
	}

	return gridPointToPoint( pCell->getGridPoint() );
}

QPoint SongEditor::gridPointToPoint( const GridPoint& gridPoint ) const {
	return QPoint( SongEditor::nMargin + gridPoint.getColumn() * m_nGridWidth,
				   gridPoint.getRow() * m_nGridHeight );
}

GridPoint SongEditor::pointToGridPoint( const QPoint& point,
										bool /* bHonorQuantization */ ) const {
	return GridPoint( ( point.x() - SongEditor::nMargin ) / m_nGridWidth,
					  point.y() / m_nGridHeight );
}

void SongEditor::ensureCursorIsVisible() {
	m_pSongEditorPanel->ensureCursorIsVisible();
}

GridPoint SongEditor::getCursorPosition() const {
	return m_cursor;
}

void SongEditor::moveCursorDown( QKeyEvent* pEvent, Editor::Step step ) {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	const int nMax = pSong->getPatternList()->size() - 1;

	int nStep;
	switch( step ) {
	case Editor::Step::None:
		nStep = 0;
		break;
	case Editor::Step::Character:
	case Editor::Step::Tiny:
		nStep = 1;
		break;
	case Editor::Step::Word:
		nStep = Editor::nWordSize;
		break;
	case Editor::Step::Page:
		nStep = Editor::nPageSize;
		break;
	case Editor::Step::Document:
		m_cursor.setRow( nMax );
		return;
	}

	m_cursor.setRow( std::min( m_cursor.getRow() + nStep, nMax ) );
}

void SongEditor::moveCursorLeft( QKeyEvent* pEvent, Editor::Step step ) {
	int nStep;
	switch( step ) {
	case Editor::Step::None:
		nStep = 0;
		break;
	case Editor::Step::Character:
	case Editor::Step::Tiny:
		nStep = 1;
		break;
	case Editor::Step::Word:
		nStep = Editor::nWordSize;
		break;
	case Editor::Step::Page:
		nStep = Editor::nPageSize;
		break;
	case Editor::Step::Document:
		m_cursor.setColumn( 0 );
		return;
	}

	m_cursor.setColumn( std::max( m_cursor.getColumn() - nStep, 0 ) );
}

void SongEditor::moveCursorRight( QKeyEvent* pEvent, Editor::Step step ) {
	const int nMax = Preferences::get_instance()->getMaxBars() - 1;

	int nStep;
	switch( step ) {
	case Editor::Step::None:
		nStep = 0;
		break;
	case Editor::Step::Character:
	case Editor::Step::Tiny:
		nStep = 1;
		break;
	case Editor::Step::Word:
		nStep = Editor::nWordSize;
		break;
	case Editor::Step::Page:
		nStep = Editor::nPageSize;
		break;
	case Editor::Step::Document:
		m_cursor.setColumn( nMax );
		return;
	}

	m_cursor.setColumn( std::min( m_cursor.getColumn() + nStep, nMax ) );
}

void SongEditor::moveCursorUp( QKeyEvent* pEvent, Editor::Step step ) {
	int nStep;
	switch( step ) {
	case Editor::Step::None:
		nStep = 0;
		break;
	case Editor::Step::Character:
	case Editor::Step::Tiny:
		nStep = 1;
		break;
	case Editor::Step::Word:
		nStep = Editor::nWordSize;
		break;
	case Editor::Step::Page:
		nStep = Editor::nPageSize;
		break;
	case Editor::Step::Document:
		m_cursor.setRow( 0 );
		return;
	}

	m_cursor.setRow( std::max( m_cursor.getRow() - nStep, 0 ) );
}

void SongEditor::setCursorTo( std::shared_ptr<GridCell> pCell ) {
	if ( pCell == nullptr ) {
		return;
	}

	m_cursor.setColumn( pCell->getColumn() );
	m_cursor.setRow( pCell->getRow() );
}

void SongEditor::setCursorTo( QMouseEvent* pEvent ) {
	const auto gridPoint = pointToGridPoint(
		static_cast<MouseEvent*>(pEvent)->position().toPoint(), true );
	m_cursor.setColumn( gridPoint.getColumn() );
	m_cursor.setRow( gridPoint.getRow() );
}

bool SongEditor::updateKeyboardHoveredElements() {
	if ( HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// Cursor is invisible and can not hover anything
		return updateHoveredCells( std::vector< std::shared_ptr<GridCell> >(),
								   Editor::Hover::Keyboard );
	}
	else {
		std::vector< std::shared_ptr<GridCell> > hoveredCells;
		if ( m_gridCells.find( m_cursor ) != m_gridCells.end() ) {
			hoveredCells.push_back( m_gridCells.at( m_cursor ) );
		}
		return updateHoveredCells( hoveredCells, Editor::Hover::Keyboard );
	}
}

bool SongEditor::updateMouseHoveredElements( QMouseEvent* pEvent ) {
	// Check whether the mouse pointer is Outside of the current widget.
	const QPoint globalPos = QCursor::pos();
	const QPoint widgetPos = mapFromGlobal( globalPos );
	if ( widgetPos.x() < 0 || widgetPos.x() >= width() ||
		 widgetPos.y() < 0 || widgetPos.y() >= height() ) {
		return updateHoveredCells( std::vector< std::shared_ptr<GridCell> >(),
								   Editor::Hover::Mouse );
	}

	if ( pEvent == nullptr ) {
		// The update was triggered outside of one of Qt's mouse events. We have
		// to create an artifical one instead.
		pEvent = new QMouseEvent(
			QEvent::MouseButtonRelease, widgetPos, globalPos, Qt::LeftButton,
			Qt::LeftButton, Qt::NoModifier );
	}

	const auto hoveredCells = getElementsAtPoint(
		static_cast<MouseEvent*>(pEvent)->position().toPoint(), 0, false );
	return updateHoveredCells( hoveredCells, Editor::Hover::Mouse );
}

Editor::Input SongEditor::getInput() const {
	return HydrogenApp::get_instance()->getMainToolBar()->getInput();
}

void SongEditor::mouseDrawStart( QMouseEvent* pEvent ) {
	auto pEv = static_cast<MouseEvent*>( pEvent );
	m_drawPreviousPosition = pEv->position();
}

void SongEditor::mouseDrawUpdate( QMouseEvent* pEvent ) {
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pEv = static_cast<MouseEvent*>( pEvent );

	// When creating a lasso to select elements or adjusting a property of
	// elements using the delta of a cursor movement, it make sense to move out
	// of the editor. But in drawing mode we just want to create/delete elements
	// witin the editor.
	const QPointF end(
		std::clamp( static_cast<int>(pEv->position().x()), 0, width() ),
		std::clamp( static_cast<int>(pEv->position().y()), 0, height() ) );

	// Check whether we are still at the same grid point as in the last update.
	// We do not want to toggle the same note twice.
	const auto endGridPoint = pointToGridPoint( end.toPoint(), true );
	if ( endGridPoint == m_drawPreviousGridPoint ) {
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

	auto lastGridPoint = m_drawPreviousGridPoint;
	// Since we can only toggle notes on the grid, we use the projection of the
	// movement on the x axis to drive the loop. This ensures that we are always
	// on grid.
	for ( auto ppoint = start; ( ppoint - start ).manhattanLength() <=
			  ( end - start ).manhattanLength(); ppoint += increment ) {
		// We prioritize existing notes
		const auto gridPoint = pointToGridPoint( ppoint.toPoint(), true );
		if ( gridPoint != lastGridPoint ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					gridPoint, Editor::Action::Toggle,
					Editor::ActionModifier::None ), sUndoContext );
			lastGridPoint = gridPoint;
		}
	}

	m_drawPreviousPosition = end;
	m_drawPreviousGridPoint = lastGridPoint;
}

void SongEditor::mouseDrawEnd() {
	HydrogenApp::get_instance()->endUndoContext();

	m_drawPreviousPosition = QPointF( 0, 0 );
	m_drawPreviousGridPoint = GridPoint( -1, -1 );
}

void SongEditor::updateAllComponents( Editor::Update update ) {
	updateVisibleComponents( update );
}

void SongEditor::updateVisibleComponents( Editor::Update update ) {
	m_pSongEditorPanel->getSongEditorPatternList()->update();
	m_pSongEditorPanel->getSongEditorPositionRuler()->update();
	updateEditor( update );
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
	const int nEditorHeight = pSong->getPatternList()->size() * m_nGridHeight;

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
	const GridPoint offset = movingGridOffset();
	if ( offset == GridPoint( 0, 0 ) ) {
		return;
	}
	std::vector<GridPoint> newGridPoints, mergeGridPoints, deleteGridPoints;

	updateGridCells();

	const int nMaxRow = pSong->getPatternList()->size() - 1;
	const int nMaxColumn = Preferences::get_instance()->getMaxBars() - 1;
	for ( const auto& ppCell : m_selection ) {
		if ( ppCell == nullptr ) {
			continue;
		}

		// Remove original active cell
		if ( ! m_bCopyNotMove ) {
			deleteGridPoints.push_back( ppCell->getGridPoint() );
		}
		const GridPoint newGridPoint = ppCell->getGridPoint() + offset;
		// Place new cell if not already active
		if ( newGridPoint.getColumn() >= 0 && newGridPoint.getRow() >= 0 &&
			 newGridPoint.getRow() <= nMaxRow &&
			 newGridPoint.getColumn() <= nMaxColumn ) {
			if ( m_gridCells.find( newGridPoint ) == m_gridCells.end() ||
				 ! m_bCopyNotMove ) {
				// Cell is not active. Activate it.
				newGridPoints.push_back( newGridPoint );
			} else {
				// This cell already exists. We do not have to add but just to
				// select it.
				mergeGridPoints.push_back( newGridPoint );
			}
		}
	}

	if ( newGridPoints.size() > 0 || mergeGridPoints.size() > 0 ||
		 deleteGridPoints.size() > 0) {
		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();

		pHydrogenApp->beginUndoMacro( pCommonStrings->getActionMovePatternCells() );
		for ( const auto& ggridPoint : deleteGridPoints ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					ggridPoint, Editor::Action::Delete,
					Editor::ActionModifier::None ) );
		}
		for ( const auto& ggridPoint : newGridPoints ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					ggridPoint, Editor::Action::Add,
					Editor::ActionModifier::AddToSelection ) );
		}
		for ( const auto& ggridPoint : mergeGridPoints ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemovePatternCellAction(
					ggridPoint, Editor::Action::None,
					Editor::ActionModifier::AddToSelection ) );
		}
		pHydrogenApp->endUndoMacro();
	}
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
		m_update = Editor::Update::Transient;
	}

	const auto pPref = Preferences::get_instance();

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pContentPixmap,
						QRectF( pixelRatio * ev->rect().x(),
								pixelRatio * ev->rect().y(),
								pixelRatio * ev->rect().width(),
								pixelRatio * ev->rect().height() ) );

	// Draw hovered cells
	for ( const auto& ppCell : m_hoveredCells ) {
		if ( ppCell != nullptr ) {
			drawPattern( painter, ppCell, CellStyle::Hovered );
		}
	}

	// Draw moving selected cells
	if ( m_selection.isMoving() ) {
		for ( const auto& ppCell : m_selection ) {
			if ( ppCell != nullptr ) {
				drawPattern( painter, ppCell, CellStyle::Moved );
				continue;
			}
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
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() &&
		 m_pSongEditorPanel->hasSongEditorFocus() ) {
		QPen cursorPen( pPref->getColorTheme()->m_cursorColor );
		cursorPen.setWidth( 2 );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.setPen( cursorPen );
		painter.setBrush( Qt::NoBrush );
		painter.drawRoundedRect(
			QRect( gridPointToPoint( m_cursor ),
				   QSize( m_nGridWidth, m_nGridHeight ) ), 4, 4 );
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
	const auto pColorTheme = pPref->getColorTheme();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	const int nPatterns = pSong->getPatternList()->size();
	const int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	const int nMaxPatternSequence = pPref->getMaxBars();

	updatePixmapSize();

	m_pBackgroundPixmap->fill( pColorTheme->m_songEditor_backgroundColor );

	QPainter p( m_pBackgroundPixmap );
	
	for ( int ii = 0; ii < nPatterns + 1; ii++) {
		if ( ( ii % 2 ) == 0 &&
			 ii != nSelectedPatternNumber ) {
			continue;
		}
		
		int y = m_nGridHeight * ii;
		
		if ( ii == nSelectedPatternNumber ) {
			p.fillRect( 0, y, nMaxPatternSequence * m_nGridWidth, m_nGridHeight,
						pColorTheme->m_songEditor_selectedRowColor );
		} else {
			p.fillRect( 0, y, nMaxPatternSequence * m_nGridWidth, m_nGridHeight,
						pColorTheme->m_songEditor_alternateRowColor );
		}
	}

	p.setPen( QPen( pColorTheme->m_songEditor_lineColor, 1,
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
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	// We check whether the contained grid cells are still valid, update those
	// whos pattern did change, and discard those being obsolete. We have to
	// ensure we do not invalidate pointers on the way as this would also affect
	// the current selection and other parts of the editor.
	std::map<GridPoint, std::shared_ptr<GridCell>> oldGridCells;
	for ( const auto& [ ggridPoint, ppCell] : m_gridCells ) {
		if ( ppCell != nullptr ) {
			oldGridCells[ ggridPoint ] = ppCell;
		}
	}
	m_gridCells.clear();

	auto pPatternList = pSong->getPatternList();
	auto pColumns = pSong->getPatternGroupVector();

	std::shared_ptr<GridCell> pCell;
	for ( int nColumn = 0; nColumn < pColumns->size(); nColumn++ ) {
		auto pColumn = (*pColumns)[nColumn];
		const int nMaxLength = pColumn->longestPatternLength();

		for ( int nPat = 0; nPat < pColumn->size(); nPat++ ) {
			auto pPattern = (*pColumn)[ nPat ];
			if ( pPattern == nullptr ) {
				continue;
			}
			const int y = pPatternList->index( pPattern );
			assert( y != -1 );
			const float fWidth = static_cast<float>(pPattern->getLength()) /
				static_cast<float>(nMaxLength);

			const GridPoint gridPoint( nColumn, y );

			// Check whether the cell was already created - either during the
			// last update or as part of a virtual pattern.
			pCell = nullptr;
			if ( oldGridCells.find( gridPoint ) != oldGridCells.end() ) {
				pCell = oldGridCells.at( gridPoint );
			}
			else if ( m_gridCells.find( gridPoint ) != m_gridCells.end() ) {
				pCell = m_gridCells.at( gridPoint );
			}

			if ( pCell != nullptr ) {
				pCell->setWidth( fWidth );
				pCell->setActive( true );
				pCell->setDrawnVirtual( false );
				if ( m_gridCells.find( gridPoint ) == m_gridCells.end() ) {
					m_gridCells.insert( { gridPoint, pCell } );
				}
				if ( oldGridCells.find( gridPoint ) != oldGridCells.end() ) {
					oldGridCells.erase( oldGridCells.find( gridPoint ) );
				}
			}
			else {
				const auto pCell = std::make_shared<GridCell>(
					gridPoint, true, fWidth, false );
				m_gridCells.insert( { gridPoint, pCell } );
			}

			for ( const auto& pVPattern : *( pPattern->getFlattenedVirtualPatterns() ) ) {
				if ( pVPattern == nullptr ) {
					continue;
				}
				const float fWidthVirtual = static_cast<float>(
						pVPattern->getLength()) / static_cast<float>(nMaxLength);
				const GridPoint gridPointVirtual(
					nColumn, pPatternList->index( pVPattern ) );
				if ( m_gridCells.find( gridPointVirtual ) != m_gridCells.end() ) {
					// In case the pattern is already present, we do not add it
					// as virtual one again.
					continue;
				}

				if ( oldGridCells.find( gridPointVirtual ) != oldGridCells.end() ) {
					pCell = oldGridCells.at( gridPointVirtual );
					pCell->setWidth( fWidthVirtual );
					pCell->setActive( false );
					pCell->setDrawnVirtual( true );
					m_gridCells.insert( { gridPointVirtual, pCell } );
					oldGridCells.erase( oldGridCells.find( gridPointVirtual ) );
				}
				else {
					const auto pCell = std::make_shared<GridCell>(
						gridPointVirtual, false, fWidthVirtual, true );
					m_gridCells.insert( { gridPointVirtual, pCell } );
				}
			}
		}
	}
}

bool SongEditor::updateHoveredCells(
	std::vector< std::shared_ptr<GridCell> > hoveredCells,
	Editor::Hover hover )
{
	bool bIdentical = true;
	std::vector< std::shared_ptr<GridCell> >* pCachedCells;
	if ( hover == Editor::Hover::Keyboard ) {
		pCachedCells = &m_keyboardHoveredCells;
	} else {
		pCachedCells = &m_mouseHoveredCells;
	}

	if ( hoveredCells.size() == pCachedCells->size() ) {
		for ( int ii = 0; ii < hoveredCells.size(); ++ii ) {
			if ( hoveredCells[ ii ] != nullptr &&
				 hoveredCells[ ii ] != pCachedCells->at( ii ) ) {
				bIdentical = false;
				break;
			}
		}

		if ( bIdentical ) {
			return false;
		}
	}

	// The current and the last hovered elments differ
	pCachedCells->clear();
	for ( auto& ccell : hoveredCells ) {
		pCachedCells->push_back( ccell );
	}

	m_hoveredCells.clear();
	for ( const auto& ccell : m_keyboardHoveredCells ) {
		m_hoveredCells.push_back( ccell );
	}
	for ( const auto& ccell : m_mouseHoveredCells ) {
		m_hoveredCells.push_back( ccell );
	}

	return true;
}

// Return grid offset (in cell coordinate space) of moving selection
GridPoint SongEditor::movingGridOffset() const {
	const QPoint rawOffset = m_selection.movingOffset();

	// Quantize offset to multiples of m_nGrid{Width,Height}
	int x_bias = m_nGridWidth / 2, y_bias = m_nGridHeight / 2;
	if ( rawOffset.y() < 0 ) {
		y_bias = -y_bias;
	}
	if ( rawOffset.x() < 0 ) {
		x_bias = -x_bias;
	}
	const int x_off = (rawOffset.x() + x_bias) / (int)m_nGridWidth;
	const int y_off = (rawOffset.y() + y_bias) / (int)m_nGridHeight;
	return GridPoint( x_off, y_off );
}

void SongEditor::drawSequence() {
	const qreal pixelRatio = devicePixelRatio();
	QPainter p( m_pContentPixmap );
	p.drawPixmap( rect(), *m_pBackgroundPixmap,
				  QRectF( pixelRatio * rect().x(),
						  pixelRatio * rect().y(),
						  pixelRatio * rect().width(),
						  pixelRatio * rect().height() ) );

	updateGridCells();

	// Draw using GridCells representation
	for ( const auto& [ _, ppCell ] : m_gridCells ) {
		if ( ppCell != nullptr && ! m_selection.isSelected( ppCell ) ) {
			drawPattern( p, ppCell, CellStyle::Default );
		}
	}
	// We draw all selected patterns in a second run to ensure their
	// border does have the proper color (else the bottom and left one
	// could be overwritten by an adjecent, unselected pattern).
	for ( const auto& [ _, ppCell ] : m_gridCells ) {
		if ( ppCell != nullptr && m_selection.isSelected( ppCell ) ) {
			drawPattern( p, ppCell, CellStyle::Selected );
		}
	}
}

void SongEditor::drawPattern( QPainter& painter, std::shared_ptr<GridCell> pCell,
							  CellStyle cellStyle ) {
	if ( pCell == nullptr ) {
		return;
	}
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
	const auto pPref = Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	if ( m_selection.isSelected( pCell ) ) {
		cellStyle = static_cast<CellStyle>(cellStyle | CellStyle::Selected);
	}
	if ( pCell->getDrawnVirtual() ) {
		cellStyle = static_cast<CellStyle>(cellStyle | CellStyle::Virtual);
	}

	const auto point = gridPointToPoint( pCell->getGridPoint() );

	QPoint movingOffset;
	if ( cellStyle & CellStyle::Moved ) {
		const auto delta = movingGridOffset();
		movingOffset = QPoint( delta.getColumn() * m_nGridWidth,
							   delta.getRow() * m_nGridHeight );
	}

	if ( ! ( cellStyle & CellStyle::Moved ) ) {
		// Draw cell content
		QColor cellColor;
		/* The following color modes are available:
		 *
		 * Automatic: Steps = Number of pattern in song and colors will be
		 *            chosen internally.
		 * Custom: Number of steps as well as the colors used are defined
		 *            by the user. */
		if ( pPref->getInterfaceTheme()->m_coloringMethod ==
			 H2Core::InterfaceTheme::ColoringMethod::Automatic ) {
			// beware of the division by zero..
			const int nSteps = std::max( 1, pPatternList->size() );
			const int nHue = ( (pCell->getRow() % nSteps) *
							   (300 / nSteps) + 213) % 300;
			cellColor.setHsv( nHue , 156 , 249 );
		}
		else {
			const int nIndex = std::clamp(
				pCell->getRow() %
				pPref->getInterfaceTheme()->m_nVisiblePatternColors,
				0, InterfaceTheme::nMaxPatternColors );
			cellColor = pPref->getInterfaceTheme()
				->m_patternColors[ nIndex ].toHsv();
		}

		if ( cellStyle & CellStyle::Virtual ) {
			cellColor = cellColor.darker( 200 );
		}

		// color base note will be filled with
		QBrush cellBrush( cellColor );

		if ( cellStyle & ( CellStyle::Selected | CellStyle::Hovered ) ) {
			QColor highlightColor;
			if ( m_pSongEditorPanel->hasSongEditorFocus() || m_bEntered ) {
				highlightColor = pColorTheme->m_selectionHighlightColor;
			} else {
				highlightColor = pColorTheme->m_selectionInactiveColor;
			}

			if ( cellStyle & CellStyle::Hovered ) {
				int nFactor = 125;
				if ( cellStyle & CellStyle::Selected ) {
					nFactor = 107;
				}

				// Depending on the highlight color, we make it either darker or
				// lighter.
				if ( Skin::moreBlackThanWhite( highlightColor ) ) {
					highlightColor = highlightColor.lighter( nFactor );
				} else {
					highlightColor = highlightColor.darker( nFactor );
				}
			}

			// Highlight
			painter.setRenderHint( QPainter::Antialiasing );
			painter.setPen( QPen( highlightColor ) );
			painter.setBrush( QBrush( Qt::black ) );
			painter.drawRect( point.x(), point.y(),
							  pCell->getWidth() * m_nGridWidth, m_nGridHeight );

			// Grid cell
			painter.fillRect( point.x() + 2, point.y() + 2,
							  pCell->getWidth() * m_nGridWidth - 4,
							  m_nGridHeight - 4, cellBrush );
		}
		else {
			// outline color
			painter.setPen( Qt::black );
			painter.setBrush( cellBrush );
			painter.drawRect( point.x(), point.y(),
							  pCell->getWidth() * m_nGridWidth, m_nGridHeight );
		}
	}
	else {
		QPen movingPen( Qt::black );
		movingPen.setStyle( Qt::DotLine );
		movingPen.setWidth( 2 );
		painter.setPen( movingPen );
		painter.setBrush( Qt::NoBrush );
		painter.drawRect( point.x() + movingOffset.x(),
						  point.y() + movingOffset.y(),
						  pCell->getWidth() * m_nGridWidth, m_nGridHeight );
	}
}

std::vector<SongEditor::SelectionIndex> SongEditor::elementsIntersecting( const QRect& r )
{
	std::vector<SelectionIndex> elems;
	for ( auto it : m_gridCells ) {
		if ( r.intersects( QRect( gridPointToPoint( it.first ),
								  QSize( m_nGridWidth, m_nGridHeight) ) ) ) {
			if ( it.second != nullptr && ! it.second->getDrawnVirtual() ) {
				elems.push_back( it.second );
			}
		}
	}
	return std::move( elems );
}

QRect SongEditor::getKeyboardCursorRect() {
	return QRect( QPoint( 0, 1 ) + gridPointToPoint( m_cursor ),
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
	updateEditor( Editor::Update::Content );
}
