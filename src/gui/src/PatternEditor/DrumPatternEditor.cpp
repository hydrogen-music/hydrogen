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

#include "DrumPatternEditor.h"

#include "PatternEditorPanel.h"
#include "../Skin.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Globals.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>


#include <math.h>
#include <cassert>
#include <algorithm>
#include <stack>

using namespace H2Core;
using namespace Editor;

DrumPatternEditor::DrumPatternEditor( QWidget* parent )
	: PatternEditor( parent )
{
	m_type = Editor::Type::Grid;
	m_instance = Editor::Instance::DrumPattern;

	const auto pPref = H2Core::Preferences::get_instance();

	m_nGridHeight = pPref->getPatternEditorGridHeight();
	m_nEditorHeight = m_pPatternEditorPanel->getRowNumberDB() * m_nGridHeight;
	m_nActiveWidth = m_nEditorWidth;
	resize( m_nEditorWidth, m_nEditorHeight );

	updatePixmapSize();
}

DrumPatternEditor::~DrumPatternEditor()
{
}

void DrumPatternEditor::moveCursorDown( QKeyEvent* ev, Editor::Step step ) {
	const int nMaxRow = m_pPatternEditorPanel->getRowNumberDB() - 1;

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
		m_pPatternEditorPanel->setSelectedRowDB( nMaxRow );
		return;
	}

	m_pPatternEditorPanel->setSelectedRowDB(
		std::min( m_pPatternEditorPanel->getSelectedRowDB() + nStep, nMaxRow ) );
}

void DrumPatternEditor::moveCursorUp( QKeyEvent* ev, Editor::Step step ) {
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
		m_pPatternEditorPanel->setSelectedRowDB( 0 );
		return;
	}

	m_pPatternEditorPanel->setSelectedRowDB(
		std::max( m_pPatternEditorPanel->getSelectedRowDB() - nStep, 0 ) );
}

void DrumPatternEditor::updateEditor( bool bPatternOnly )
{
	const int nTargetHeight =
		m_pPatternEditorPanel->getRowNumberDB() * m_nGridHeight;
	if ( m_nEditorHeight != nTargetHeight ) {
		m_nEditorHeight = nTargetHeight;
		resize( m_nEditorWidth, m_nEditorHeight );
		m_update = Editor::Update::Background;
	}

	Editor::Base<Elem>::updateEditor( bPatternOnly );
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


	// Calculate the first and last position values that this rect will
	// intersect with
	int x_min = (rNormalized.left() - PatternEditor::nMargin - 1) / m_fGridWidth;
	int x_max = (std::min( rNormalized.right(), m_nActiveWidth ) -
				 PatternEditor::nMargin ) / m_fGridWidth;

	const Pattern::notes_t* notes = pPattern->getNotes();

	for (auto it = notes->lower_bound( x_min ); it != notes->end() && it->first <= x_max; ++it ) {
		auto ppNote = it->second;
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

void DrumPatternEditor::createBackground() {
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
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	// Indicate chosen editor mode.
	QColor backgroundInactiveColor;
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Pattern ) {
		backgroundInactiveColor =
			pPref->getTheme().m_color.m_windowColor.lighter(
				Skin::nEditorActiveScaling );
	}
	else {
		backgroundInactiveColor = pPref->getTheme().m_color.m_windowColor;
	}

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
		backgroundColor = backgroundColor.darker( PatternEditor::nOutOfFocusDim );
		alternateRowColor = alternateRowColor.darker( PatternEditor::nOutOfFocusDim );
		selectedRowColor = selectedRowColor.darker( PatternEditor::nOutOfFocusDim );
	}

	updatePixmapSize();

	m_pBackgroundPixmap->fill( backgroundInactiveColor );

	QPainter p( m_pBackgroundPixmap );

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
	if ( m_pPatternEditorPanel->getPattern() != nullptr ) {
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
	p.setPen( QPen( lineColor, 1, Qt::DotLine ) );
	for ( uint i = 0; i < (uint)nRows; i++ ) {
		uint y = m_nGridHeight * i + m_nGridHeight;
		p.drawLine( 0, y, m_nActiveWidth, y);
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( QPen( lineInactiveColor, 1, Qt::DotLine ) );
		for ( uint i = 0; i < (uint)nRows; i++ ) {
			uint y = m_nGridHeight * i + m_nGridHeight;
			p.drawLine( m_nActiveWidth, y, m_nEditorWidth, y);
		}
	}

	// borders
	drawBorders( p );
}

void DrumPatternEditor::paintEvent( QPaintEvent* ev )
{
	if (!isVisible()) {
		return;
	}

	PatternEditor::paintEvent( ev );

	QPainter painter( this );

	// Draw hovered notes
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		const auto baseStyle = static_cast<NoteStyle>(
			( ppPattern == pPattern ? NoteStyle::Foreground :
			  NoteStyle::Background ) | NoteStyle::Hovered);
		sortAndDrawNotes( painter, nnotes, baseStyle );
	}

	// Draw moved notes
	if ( ! m_selection.isEmpty() && m_selection.isMoving() ) {
		for ( const auto& ppNote : m_selection ) {
			drawNote( painter, ppNote, NoteStyle::Moved );
		}
	}
}
