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

#include <cassert>

#include "PianoRollEditor.h"
#include "PatternEditorPanel.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>


using namespace H2Core;

PianoRollEditor::PianoRollEditor( QWidget *pParent, QScrollArea *pScrollView)
	: PatternEditor( pParent )
	, m_pScrollView( pScrollView )
{
	m_editor = PatternEditor::Editor::PianoRoll;
	
	m_nGridHeight = 10;

	setAttribute(Qt::WA_OpaquePaintEvent);

	m_nEditorHeight = OCTAVE_NUMBER * KEYS_PER_OCTAVE * m_nGridHeight;

	resize( m_nEditorWidth, m_nEditorHeight );
	
	HydrogenApp::get_instance()->addEventListener( this );

	m_bSelectNewNotes = false;
}



PianoRollEditor::~PianoRollEditor()
{
	INFOLOG( "DESTROY" );
}

QPoint PianoRollEditor::noteToPoint( H2Core::Note* pNote ) const {
	if ( pNote == nullptr ) {
		return QPoint();
	}

	return QPoint(
		PatternEditor::nMargin + pNote->get_position() * m_fGridWidth,
		m_nGridHeight *
		Note::pitchToLine( pNote->get_pitch_from_key_octave() ) + 1 );
}

void PianoRollEditor::updateEditor( bool bPatternOnly )
{
	const bool bFullUpdate = updateWidth();

	if ( bPatternOnly && ! bFullUpdate ) {
		m_update = Update::Pattern;
	} else {
		m_update = Update::Background;
	}

	update();
}

void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}
	
	const auto pPref = Preferences::get_instance();
	
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_bBackgroundInvalid || m_update == Update::Background ) {
		createBackground();
	}

	QPainter painter( this );

	if ( m_update == Update::Background || m_update == Update::Pattern ) {
		drawPattern();
		m_update = Update::None;
	}


	painter.drawPixmap( ev->rect(), *m_pPatternPixmap,
						QRectF( pixelRatio * ev->rect().x(),
								pixelRatio * ev->rect().y(),
								pixelRatio * ev->rect().width(),
								pixelRatio * ev->rect().height() ) );

	// Draw playhead
	if ( m_nTick != -1 ) {

		int nOffset = Skin::getPlayheadShaftOffset();
		int nX = static_cast<int>(static_cast<float>(PatternEditor::nMargin) +
								  static_cast<float>(m_nTick) *
								  m_fGridWidth );
		Skin::setPlayheadPen( &painter, false );
		painter.drawLine( nX, 2, nX, height() - 2 );
	}

	drawFocus( painter );

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// Draw hovered note
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		const auto baseStyle = static_cast<NoteStyle>(
			( ppPattern == pPattern ? NoteStyle::Foreground :
			  NoteStyle::Background ) | NoteStyle::Hovered);
		for ( const auto& ppNote : nnotes ) {
			if ( ppNote != nullptr && ppNote->getType() == row.sType &&
				 ppNote->get_instrument_id() == row.nInstrumentID ) {
				const auto style = static_cast<NoteStyle>(
					m_selection.isSelected( ppNote ) ?
					NoteStyle::Selected | baseStyle : baseStyle );
				drawNote( painter, ppNote, style );
			}
		}
	}

	m_selection.paintSelection( &painter );

	// Draw cursor
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		QPoint pos = getCursorPosition();

		QPen pen( pPref->getTheme().m_color.m_cursorColor );
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.setBrush( Qt::NoBrush );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( getKeyboardCursorRect(), 4, 4 );
	}
}

void PianoRollEditor::drawFocus( QPainter& painter ) {

	const auto pPref = H2Core::Preferences::get_instance();
	
	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}
	
	QColor color = pPref->getTheme().m_color.m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	const auto pScrollArea = m_pPatternEditorPanel->getPianoRollEditorScrollArea();
	int nStartY = pScrollArea->verticalScrollBar()->value();
	int nStartX = pScrollArea->horizontalScrollBar()->value();
	int nEndY = nStartY + pScrollArea->viewport()->size().height();
	int nEndX = std::min( nStartX + pScrollArea->viewport()->size().width(), width() );

	QPen pen( color );
	pen.setWidth( 4 );
	painter.setPen( pen );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nEndX, nStartY ) );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nStartX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nStartY ), QPoint( nEndX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nEndY ), QPoint( nStartX, nEndY ) );
}

void PianoRollEditor::createBackground()
{
	const auto pPref = H2Core::Preferences::get_instance();
	
	QColor backgroundColor = pPref->getTheme().m_color.m_patternEditor_backgroundColor;
	const QColor backgroundInactiveColor = pPref->getTheme().m_color.m_windowColor;
	QColor alternateRowColor = pPref->getTheme().m_color.m_patternEditor_alternateRowColor;
	QColor octaveColor = pPref->getTheme().m_color.m_patternEditor_octaveRowColor;
	// The line corresponding to the default pitch set to new notes
	// will be highlighted.
	const QColor baseNoteColor = octaveColor.lighter( 119 );
	QColor lineColor( pPref->getTheme().m_color.m_patternEditor_lineColor );
	const QColor lineInactiveColor( pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
		backgroundColor = backgroundColor.darker( PatternEditor::nOutOfFocusDim );
		alternateRowColor = alternateRowColor.darker( PatternEditor::nOutOfFocusDim );
		octaveColor = octaveColor.darker( PatternEditor::nOutOfFocusDim );
	}

	unsigned start_x = 0;
	unsigned end_x = m_nActiveWidth;

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio,
										   height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		delete m_pPatternPixmap;
		m_pPatternPixmap = new QPixmap( width()  * pixelRatio,
										height() * pixelRatio );
		m_pPatternPixmap->setDevicePixelRatio( pixelRatio );
	}

	m_pBackgroundPixmap->fill( backgroundInactiveColor );

	QPainter p( m_pBackgroundPixmap );

	for ( uint ooctave = 0; ooctave < OCTAVE_NUMBER; ++ooctave ) {
		unsigned start_y = ooctave * KEYS_PER_OCTAVE * m_nGridHeight;

		for ( int ii = 0; ii < KEYS_PER_OCTAVE; ++ii ) {
			if ( ii == 0 || ii == 2 || ii == 4 || ii == 6 || ii == 7 ||
				 ii == 9 || ii == 11 ) {
				if ( ooctave % 2 != 0 ) {
					p.fillRect( start_x, start_y + ii * m_nGridHeight,
								end_x - start_x, start_y + ( ii + 1 ) * m_nGridHeight,
								octaveColor );
				} else {
					p.fillRect( start_x, start_y + ii * m_nGridHeight,
								end_x - start_x, start_y + ( ii + 1 ) * m_nGridHeight,
								backgroundColor );
				}
			} else {
				p.fillRect( start_x, start_y + ii * m_nGridHeight,
							end_x - start_x, start_y + ( ii + 1 ) * m_nGridHeight,
							alternateRowColor );
			}
		}

		// Highlight base note pitch
		if ( ooctave == 3 ) {
			p.fillRect( start_x, start_y + 11 * m_nGridHeight,
						end_x - start_x, start_y + KEYS_PER_OCTAVE * m_nGridHeight,
						baseNoteColor );
		}
	}


	// horiz lines
	p.setPen( lineColor );
	for ( uint row = 0; row < ( KEYS_PER_OCTAVE * OCTAVE_NUMBER ); ++row ) {
		unsigned y = row * m_nGridHeight;
		p.drawLine( start_x, y, end_x, y );
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		for ( uint row = 0; row < ( KEYS_PER_OCTAVE * OCTAVE_NUMBER ); ++row ) {
			unsigned y = row * m_nGridHeight;
			p.drawLine( m_nActiveWidth, y, m_nEditorWidth, y );
		}
	}

	if ( m_pPatternEditorPanel->getPattern() != nullptr ) {
		//draw text
		QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
					getPointSize( pPref->getTheme().m_font.m_fontSize ) );
		p.setFont( font );
		p.setPen( pPref->getTheme().m_color.m_patternEditor_textColor );

		int offset = 0;
		int insertx = 3;
		for ( int oct = 0; oct < (int)OCTAVE_NUMBER; oct++ ){
			if( oct > 3 ){
				p.drawText( insertx, m_nGridHeight  + offset, "B" );
				p.drawText( insertx, 10 + m_nGridHeight  + offset, "A#" );
				p.drawText( insertx, 20 + m_nGridHeight  + offset, "A" );
				p.drawText( insertx, 30 + m_nGridHeight  + offset, "G#" );
				p.drawText( insertx, 40 + m_nGridHeight  + offset, "G" );
				p.drawText( insertx, 50 + m_nGridHeight  + offset, "F#" );
				p.drawText( insertx, 60 + m_nGridHeight  + offset, "F" );
				p.drawText( insertx, 70 + m_nGridHeight  + offset, "E" );
				p.drawText( insertx, 80 + m_nGridHeight  + offset, "D#" );
				p.drawText( insertx, 90 + m_nGridHeight  + offset, "D" );
				p.drawText( insertx, 100 + m_nGridHeight  + offset, "C#" );
				p.drawText( insertx, 110 + m_nGridHeight  + offset, "C" );
				offset += KEYS_PER_OCTAVE * m_nGridHeight;
			}
			else {
				p.drawText( insertx, m_nGridHeight  + offset, "b" );
				p.drawText( insertx, 10 + m_nGridHeight  + offset, "a#" );
				p.drawText( insertx, 20 + m_nGridHeight  + offset, "a" );
				p.drawText( insertx, 30 + m_nGridHeight  + offset, "g#" );
				p.drawText( insertx, 40 + m_nGridHeight  + offset, "g" );
				p.drawText( insertx, 50 + m_nGridHeight  + offset, "f#" );
				p.drawText( insertx, 60 + m_nGridHeight  + offset, "f" );
				p.drawText( insertx, 70 + m_nGridHeight  + offset, "e" );
				p.drawText( insertx, 80 + m_nGridHeight  + offset, "d#" );
				p.drawText( insertx, 90 + m_nGridHeight  + offset, "d" );
				p.drawText( insertx, 100 + m_nGridHeight  + offset, "c#" );
				p.drawText( insertx, 110 + m_nGridHeight  + offset, "c" );
				offset += KEYS_PER_OCTAVE * m_nGridHeight;
			}
		}

		drawGridLines( p, Qt::DashLine );
	}

	p.setPen( QPen( lineColor, 2, Qt::SolidLine ) );
	p.drawLine( m_nEditorWidth, 0, m_nEditorWidth, m_nEditorHeight );

	m_bBackgroundInvalid = false;
}


void PianoRollEditor::drawPattern()
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	validateSelection();

	qreal pixelRatio = devicePixelRatio();
	
	QPainter p( m_pPatternPixmap );
	// copy the background image
	p.drawPixmap( rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * rect().x(),
								pixelRatio * rect().y(),
								pixelRatio * rect().width(),
								pixelRatio * rect().height() ) );

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// for each note...
	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		const auto baseStyle = ppPattern ==
			pPattern ? NoteStyle::Foreground : NoteStyle::Background;
		for ( const auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
			if ( ppNote != nullptr && ppNote->getType() == row.sType &&
				 ppNote->get_instrument_id() == row.nInstrumentID ) {
				const auto style = static_cast<NoteStyle>(
					m_selection.isSelected( ppNote ) ?
					NoteStyle::Selected | baseStyle : baseStyle );
				drawNote( p, ppNote, style );
			}
		}
	}
}

void PianoRollEditor::selectAll()
{
	selectAllNotesInRow( m_pPatternEditorPanel->getSelectedRowDB() );
}

void PianoRollEditor::keyPressEvent( QKeyEvent * ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row [%1]" );
		return;
	}

	const int nBlockSize = 5;
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bUnhideCursor = true;
	bool bEventUsed = true;
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Selection key, nothing more to do (other than update editor)
	}
	else if ( ev->matches( QKeySequence::MoveToNextLine ) ||
			  ev->matches( QKeySequence::SelectNextLine ) ) {
		if ( m_nCursorRow > Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
													(Note::Key)KEY_MIN ) ) {
			setCursorRow( m_nCursorRow - 1 );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) ||
			  ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		setCursorRow( std::max( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
														(Note::Key)KEY_MIN ),
								m_nCursorRow - nBlockSize ) );
	}
	else if ( ev->matches( QKeySequence::MoveToNextPage ) ||
			  ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down -- move down by a whole octave
		const int nMinPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
													  (Note::Key)KEY_MIN );
		setCursorRow( std::max( m_nCursorRow - KEYS_PER_OCTAVE, nMinPitch ) );
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ||
			  ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		setCursorRow( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
											  (Note::Key)KEY_MIN ) );
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousLine ) ||
			  ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( m_nCursorRow < Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
													(Note::Key)KEY_MAX ) ) {
			setCursorRow( m_nCursorRow + 1 );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) ||
			  ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		setCursorRow( std::min( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
														(Note::Key)KEY_MAX ),
								m_nCursorRow + nBlockSize ) );
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousPage ) ||
			  ev->matches( QKeySequence::SelectPreviousPage ) ) {
		const int nMaxPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
													  (Note::Key)KEY_MAX );
		setCursorRow( std::min( m_nCursorRow + KEYS_PER_OCTAVE, nMaxPitch ) );
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ||
			  ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		setCursorRow( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
											  (Note::Key)KEY_MAX ) );
	}
	else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter/Return : Place or remove note at current position
		m_selection.clearSelection();
		int pressedline = Note::pitchToLine( m_nCursorRow );
		int nPitch = Note::lineToPitch( pressedline );
		addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
						 m_pPatternEditorPanel->getSelectedRowDB(),
						 Note::pitchToKey( nPitch ),
						 Note::pitchToOctave( nPitch ) );
	}
	else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete: delete selection or note at keyboard cursor
		if ( m_selection.begin() != m_selection.end() ) {
			deleteSelection();
		} else {
			// Delete a note under the keyboard cursor
			int pressedline = Note::pitchToLine( m_nCursorRow );
			int nPitch = Note::lineToPitch( pressedline );
			addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
							 m_pPatternEditorPanel->getSelectedRowDB(),
							 Note::pitchToKey( nPitch ),
							 Note::pitchToOctave( nPitch ),
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

std::vector<PianoRollEditor::SelectionIndex> PianoRollEditor::elementsIntersecting( const QRect& r )
{
	std::vector<SelectionIndex> result;
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return std::move( result );
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row" );
		return std::move( result );
	}
	
	int w = 8;
	int h = m_nGridHeight - 2;

	auto rNormalized = r.normalized();
	if ( rNormalized.top() == rNormalized.bottom() &&
		 rNormalized.left() == rNormalized.right() ) {
		rNormalized += QMargins( 2, 2, 2, 2 );
	}

	// Calculate the first and last position values that this rect will intersect with
	int x_min = (rNormalized.left() - w - PatternEditor::nMargin) / m_fGridWidth;
	int x_max = (rNormalized.right() + w - PatternEditor::nMargin) / m_fGridWidth;

	const Pattern::notes_t* pNotes = pPattern->getNotes();

	for ( auto it = pNotes->lower_bound( x_min ); it != pNotes->end() && it->first <= x_max; ++it ) {
		Note *pNote = it->second;
		if ( pNote->get_instrument_id() == selectedRow.nInstrumentID ||
			 pNote->getType() == selectedRow.sType ) {
			const auto notePoint = noteToPoint( pNote );

			if ( rNormalized.intersects( QRect( notePoint.x() -4 , notePoint.y(),
												w, h ) ) ) {
				result.push_back( pNote );
			}
		}
	}
	updateEditor( true );
	return std::move( result );
}

void PianoRollEditor::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		m_update = Update::Background;
		update();
	}
}
