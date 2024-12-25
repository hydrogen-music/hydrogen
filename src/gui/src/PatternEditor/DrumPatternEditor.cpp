/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "DrumPatternEditor.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "PatternEditorSidebar.h"

#include <core/Globals.h>
#include <core/Hydrogen.h>
#include <core/EventQueue.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Helpers/Xml.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include "../HydrogenApp.h"
#include "../Skin.h"

#include <math.h>
#include <cassert>
#include <algorithm>
#include <stack>

using namespace H2Core;

DrumPatternEditor::DrumPatternEditor( QWidget* parent )
 : PatternEditor( parent )
{
	m_editor = PatternEditor::Editor::DrumPattern;
	const auto pPref = H2Core::Preferences::get_instance();

	m_nGridHeight = pPref->getPatternEditorGridHeight();
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;
	m_nActiveWidth = m_nEditorWidth;
	resize( m_nEditorWidth, m_nEditorHeight );

	createBackground();
}

DrumPatternEditor::~DrumPatternEditor()
{
}

void DrumPatternEditor::updateEditor( bool bPatternOnly )
{
	updateWidth();

	int nTargetHeight = m_pPatternEditorPanel->getRowNumberDB() * m_nGridHeight;
	if ( m_nEditorHeight != nTargetHeight ) {
		m_nEditorHeight = nTargetHeight;
	}
	resize( m_nEditorWidth, m_nEditorHeight );

	// redraw all
	invalidateBackground();
	update();
}

///
/// Handle key press events.
///
/// Events are passed to Selection first, which may claim them (in which case they are ignored here).
///
void DrumPatternEditor::keyPressEvent( QKeyEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	
	const int nBlockSize = 5;
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();
	const int nMaxRows = m_pPatternEditorPanel->getRowNumberDB();
	auto selectedRow = m_pPatternEditorPanel->getRowDB( nSelectedRow );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row [%1]" ).arg( nSelectedRow ) );
		return;
	}

	bool bEventUsed = true;
	const bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Key was claimed by Selection
	}
	else if ( ev->matches( QKeySequence::MoveToNextLine ) ||
				ev->matches( QKeySequence::SelectNextLine ) ) {
		if ( nSelectedRow + 1 < nMaxRows ) {
			m_pPatternEditorPanel->setSelectedRowDB( nSelectedRow + 1 );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) ||
				ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		m_pPatternEditorPanel->setSelectedRowDB(
			std::min( nSelectedRow + nBlockSize, nMaxRows-1 ) );
	}
	else if ( ev->matches( QKeySequence::MoveToNextPage ) ||
				ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down, scroll by the number of instruments that fit into the
		// viewport
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		m_pPatternEditorPanel->setSelectedRowDB(
			std::min( nMaxRows - 1,
					  nSelectedRow + static_cast<int>(pParent->height() /
													  m_nGridHeight) ) );
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ||
				ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		m_pPatternEditorPanel->setSelectedRowDB( nMaxRows-1 );
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousLine ) ||
				ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( nSelectedRow > 0 ) {
			m_pPatternEditorPanel->setSelectedRowDB( nSelectedRow - 1 );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) ||
				ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		m_pPatternEditorPanel->setSelectedRowDB(
			std::max( nSelectedRow - nBlockSize, 0 ) );
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousPage ) ||
				ev->matches( QKeySequence::SelectPreviousPage ) ) {
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		m_pPatternEditorPanel->setSelectedRowDB(
			std::max( nSelectedRow - static_cast<int>(pParent->height() /
													  m_nGridHeight),
					  0 ) );
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ||
				ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		m_pPatternEditorPanel->setSelectedRowDB( 0 );
	}
	else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter / Return: add or remove note at current position
		m_selection.clearSelection();
		addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
						 nSelectedRow );
	}
	else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete / Backspace: delete selected notes, or note under
		// keyboard cursor
		if ( m_selection.begin() != m_selection.end() ) {
			// Delete selected notes if any
			deleteSelection();
		} else {
			// Delete note under the keyboard cursor.
			addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
							 nSelectedRow, KEY_MIN, OCTAVE_DEFAULT,
							 /*bDoAdd=*/false, /*bDoDelete=*/true );

		}
	}
	else {
		bEventUsed = false;
	}

	if ( ! bEventUsed ) {
		ev->setAccepted( false );
	}

	PatternEditor::keyPressEvent( ev );
}

///
/// Find all elements which intersect a selection area.
///
std::vector<DrumPatternEditor::SelectionIndex> DrumPatternEditor::elementsIntersecting( const QRect& r )
{
	std::vector<SelectionIndex> result;
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return std::move( result );
	}
	
	uint h = m_nGridHeight / 3;

	// Expand the region by approximately the size of the note
	// ellipse, equivalent to testing for intersection between `r'
	// and the equivalent rect around the note.  We'll also allow
	// a few extra pixels if it's a single point click, to make it
	// easier to grab notes.

	auto rNormalized = r.normalized();
	if ( rNormalized.top() == rNormalized.bottom() &&
		 rNormalized.left() == rNormalized.right() ) {
		rNormalized += QMargins( 2, 2, 2, 2 );
	}
	rNormalized += QMargins( 4, h/2, 4, h/2 );


	// Calculate the first and last position values that this rect will intersect with
	int x_min = (rNormalized.left() - PatternEditor::nMargin - 1) / m_fGridWidth;
	int x_max = (rNormalized.right() - PatternEditor::nMargin) / m_fGridWidth;

	const Pattern::notes_t* notes = pPattern->getNotes();

	for (auto it = notes->lower_bound( x_min ); it != notes->end() && it->first <= x_max; ++it ) {
		Note* ppNote = it->second;
		const int nRow = m_pPatternEditorPanel->findRowDB( ppNote );
		uint x_pos = PatternEditor::nMargin + (it->first * m_fGridWidth);
		uint y_pos = ( nRow * m_nGridHeight) + ( m_nGridHeight / 2 ) - 3;

		if ( rNormalized.contains( QPoint( x_pos, y_pos + h / 2 ) ) ) {
			result.push_back( ppNote );
		}
	}

	return std::move( result );
}

void DrumPatternEditor::selectAll()
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	
	m_selection.clearSelection();
	FOREACH_NOTE_CST_IT_BEGIN_LENGTH(pPattern->getNotes(), it, pPattern) {
		m_selection.addToSelection( it->second );
	}
	m_selection.updateWidgetGroup();
}

///
/// Draws a pattern
///
void DrumPatternEditor::drawPattern(QPainter& painter)
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	const auto pPref = H2Core::Preferences::get_instance();
	const QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
					  getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	const QColor textColor(
		pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor );
	QColor textBackgroundColor( textColor );
	textBackgroundColor.setAlpha( 150 );

	validateSelection();

	// We count notes in each position so we can display markers for rows which
	// have more than one note in the same position (a chord or genuine
	// duplicates).
	int nLastColumn = -1;
	// Maps the row to the number of notes shown in this very column.
	std::map<int,int> notesAtRow;
	struct PosCount {
		int nRow;
		int nColumn;
		int nNoteCount;
	};
	std::vector<PosCount> posCounts;
	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		posCounts.clear();
		const auto baseStyle = ppPattern == pPattern ?
			NoteStyle::Foreground : NoteStyle::Background;

		const auto fontColor = ppPattern == pPattern ?
			textColor : textBackgroundColor;

		for ( const auto& [ nnColumn, ppNote ] : *ppPattern->getNotes() ) {
			if ( nnColumn >= ppPattern->getLength() ) {
				// Notes are located beyond the active length of the editor and
				// aren't visible even when drawn.
				break;
			}
			if ( ppNote == nullptr ) {
				continue;
			}

			auto nRow = m_pPatternEditorPanel->findRowDB( ppNote );
			auto row = m_pPatternEditorPanel->getRowDB( nRow );
			if ( nRow == -1 ||
				 ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) ) {
				ERRORLOG( QString( "Note [%1] not associated with DB" )
						  .arg( ppNote->toQString() ) );
				m_pPatternEditorPanel->printDB();
				continue;
			}

			// Check for duplicates
			if ( nnColumn != nLastColumn ) {
				// New column

				for ( const auto& [ nnRow, nnNotes ] : notesAtRow ) {
					if ( nnNotes > 1 ) {
						posCounts.push_back( { nnRow, nLastColumn, nnNotes } );
					}
				}

				if ( nLastColumn != nnColumn ) {
					nLastColumn = nnColumn;
				}
				notesAtRow.clear();
			}

			// Since the all notes have the same size, we just point the first
			// one (except it is selected. In case the user presses Cancel on
			// the deduplication dialog in checkDeselectElements(), there could
			// be several notes at one position with only a subset being
			// selected).
			if ( notesAtRow.find( nRow ) == notesAtRow.end() ||
				 m_selection.isSelected( ppNote ) ) {
				const auto style = static_cast<NoteStyle>(
					m_selection.isSelected( ppNote ) ?
					NoteStyle::Selected | baseStyle : baseStyle );
				drawNote( painter, ppNote, style );
			}

			if ( notesAtRow.find( nRow ) == notesAtRow.end() ) {
				notesAtRow[ nRow ] = 1;
			}
			else {
				++notesAtRow[ nRow ];
			}
		}

		// Go through used rows list and draw markers for superimposed notes
		for ( const auto [ nnRow, nnColumn, nnNotes ] : posCounts ) {
			// Draw "2x" text to the left of the note
			const int x = PatternEditor::nMargin +
				( nnColumn * m_fGridWidth );
			const int y = nnRow * m_nGridHeight;
			const int boxWidth = 128;

			painter.setFont( font );
			painter.setPen( fontColor );

			painter.drawText(
				QRect( x - boxWidth - 6, y, boxWidth, m_nGridHeight ),
				Qt::AlignRight | Qt::AlignVCenter,
				( QString( "%1" ) + QChar( 0x00d7 )).arg( nnNotes ) );
		}
	}
}

void DrumPatternEditor::drawBackground( QPainter& p)
{
	const auto pPref = H2Core::Preferences::get_instance();

	QColor lineColor( pPref->getTheme().m_color.m_patternEditor_lineColor );
	// Row clicked by the user.
	QColor selectedRowColor(
		pPref->getTheme().m_color.m_patternEditor_selectedRowColor );

	// Rows for which there is a corresponding instrument in the current
	// drumkit.
	QColor backgroundColor(
		pPref->getTheme().m_color.m_patternEditor_backgroundColor );
	QColor alternateRowColor(
		pPref->getTheme().m_color.m_patternEditor_alternateRowColor );

	// Everything beyond the current pattern (used when another, larger pattern
	// is played as well).
	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
		backgroundColor = backgroundColor.darker( PatternEditor::nOutOfFocusDim );
		alternateRowColor = alternateRowColor.darker( PatternEditor::nOutOfFocusDim );
		selectedRowColor = selectedRowColor.darker( PatternEditor::nOutOfFocusDim );
	}

	const int nRows = m_pPatternEditorPanel->getRowNumberDB();
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();

	p.fillRect( 0, 0, m_nActiveWidth, m_nEditorHeight, backgroundColor );
	if ( m_nActiveWidth < m_nEditorWidth ) {
		p.fillRect( m_nActiveWidth, 0, m_nEditorWidth - m_nActiveWidth,
					m_nEditorHeight, backgroundInactiveColor);
	}

	for ( int ii = 0; ii < nRows; ii++ ) {
		const int y = static_cast<int>(m_nGridHeight) * ii;
		if ( ii == nSelectedRow ) {
			p.fillRect( 0, y, m_nActiveWidth, m_nGridHeight,
							  selectedRowColor );
		}
		else if ( ( ii % 2 ) != 0 ) {
			p.fillRect( 0, y, m_nActiveWidth, m_nGridHeight, alternateRowColor );
		}
	}

	// We skip the grid and cursor in case there is no pattern. This
	// way it may be more obvious that it is not armed and does not
	// expect user interaction.
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern != nullptr ) {
		drawGridLines( p );

		// The grid lines above are drawn full height. We will erase the upper
		// part.
		for ( int ii = 0; ii < nRows; ii++ ) {
			const int y = static_cast<int>(m_nGridHeight) * ii;
			if ( ii == nSelectedRow ) {
				p.fillRect(
					0, y, m_nActiveWidth, static_cast<int>( m_nGridHeight * 0.7 ),
					selectedRowColor );
			}
			else {
				if ( ( ii % 2 ) == 0 ) {
					p.fillRect(
						0, y, m_nActiveWidth, static_cast<int>( m_nGridHeight * 0.7 ),
						backgroundColor );
				}
				else {
					p.fillRect(
						0, y, m_nActiveWidth, static_cast<int>( m_nGridHeight * 0.7 ),
						alternateRowColor );
				}
			}

			p.fillRect( m_nActiveWidth, y, m_nEditorWidth - m_nActiveWidth,
						static_cast<int>( m_nGridHeight * 0.7 ),
						backgroundInactiveColor );
		}
	}

	// horizontal lines
	p.setPen( QPen( lineColor, 1, Qt::SolidLine ) );
	for ( uint i = 0; i < (uint)nRows; i++ ) {
		uint y = m_nGridHeight * i + m_nGridHeight;
		p.drawLine( 0, y, m_nActiveWidth, y);
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( QPen( lineInactiveColor, 1, Qt::SolidLine ) );
		for ( uint i = 0; i < (uint)nRows; i++ ) {
			uint y = m_nGridHeight * i + m_nGridHeight;
			p.drawLine( m_nActiveWidth, y, m_nEditorWidth, y);
		}
	}

	// borders
	p.setPen( lineColor );
	p.drawLine( 0, m_nEditorHeight -1 , m_nActiveWidth - 1, m_nEditorHeight - 1 );
	
	if ( m_nEditorWidth > m_nActiveWidth + 1 ) {
		p.setPen( lineInactiveColor );
		p.drawLine( m_nActiveWidth - 1, m_nEditorHeight - 1, m_nEditorWidth - 1, m_nEditorHeight - 1 );
	}
	
	p.setPen( QPen( lineColor, 2, Qt::SolidLine ) );
	p.drawLine( m_nEditorWidth, 0, m_nEditorWidth, m_nEditorHeight );

}

void DrumPatternEditor::createBackground() {
	m_bBackgroundInvalid = false;

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width() * pixelRatio, height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}
	
	QPainter painter( m_pBackgroundPixmap );

	drawBackground( painter );
	
	drawPattern( painter );
}

void DrumPatternEditor::paintEvent( QPaintEvent* ev )
{
	if (!isVisible()) {
		return;
	}
	
	const auto pPref = Preferences::get_instance();
	
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_bBackgroundInvalid ) {
		createBackground();
	}
	
	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * ev->rect().x(),
								pixelRatio * ev->rect().y(),
								pixelRatio * ev->rect().width(),
								pixelRatio * ev->rect().height() ) );

	// Draw playhead
	if ( m_nTick != -1 ) {

		const int nOffset = Skin::getPlayheadShaftOffset();
		const int nX = static_cast<int>(
			static_cast<float>(PatternEditor::nMargin) +
			static_cast<float>(m_nTick) * m_fGridWidth );
		Skin::setPlayheadPen( &painter, false );
		painter.drawLine( nX, 0, nX, height() );
	}
	
	drawFocus( painter );

	// Draw hovered note
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		const auto baseStyle = static_cast<NoteStyle>(
			( ppPattern == pPattern ? NoteStyle::Foreground :
			  NoteStyle::Background ) | NoteStyle::Hovered);
		for ( const auto& ppNote : nnotes ) {
			const auto style = static_cast<NoteStyle>(
				m_selection.isSelected( ppNote ) ?
				NoteStyle::Selected | baseStyle : baseStyle );
			drawNote( painter, ppNote, style );
		}
	}
	
	m_selection.paintSelection( &painter );

	// Draw cursor
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		QPen p( pPref->getTheme().m_color.m_cursorColor );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setBrush( Qt::NoBrush );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( getKeyboardCursorRect(), 4, 4 );
	}
}

void DrumPatternEditor::drawFocus( QPainter& painter ) {

	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}
	
	const auto pPref = H2Core::Preferences::get_instance();
	
	QColor color = pPref->getTheme().m_color.m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	int nStartY = m_pPatternEditorPanel->getVerticalScrollBar()->value();
	int nStartX = m_pPatternEditorPanel->getHorizontalScrollBar()->value();
	int nEndY = std::min(
		static_cast<int>(m_nGridHeight) * m_pPatternEditorPanel->getRowNumberDB(),
		nStartY + m_pPatternEditorPanel->getDrumPatternEditorScrollArea()->viewport()->size().height() );
	int nEndX = std::min( nStartX + m_pPatternEditorPanel->getDrumPatternEditorScrollArea()->viewport()->size().width(), width() );

	QPen pen( color );
	pen.setWidth( 4 );
	painter.setPen( pen );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nEndX, nStartY ) );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nStartX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nStartY ), QPoint( nEndX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nEndY ), QPoint( nStartX, nEndY ) );
}

void DrumPatternEditor::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
	updateEditor();
}



void DrumPatternEditor::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
}

void DrumPatternEditor::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateEditor();
	}
}
