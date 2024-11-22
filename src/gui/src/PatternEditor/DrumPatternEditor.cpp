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
#include "PatternEditorInstrumentList.h"
#include "../CommonStrings.h"

#include <core/Globals.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/EventQueue.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Helpers/Xml.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include "UndoActions.h"
#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
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
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	if ( pAudioEngine->getState() != H2Core::AudioEngine::State::Ready &&
		 pAudioEngine->getState() != H2Core::AudioEngine::State::Playing ) {
		ERRORLOG( "FIXME: skipping pattern editor update (state should be READY or PLAYING)" );
		return;
	}

	updateWidth();

	auto pSong = pHydrogen->getSong();
	int nInstruments = pSong->getDrumkit()->getInstruments()->size();

	if ( m_nEditorHeight != (int)( m_nGridHeight * nInstruments ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * nInstruments;
	}
	resize( m_nEditorWidth, m_nEditorHeight );

	// redraw all
	invalidateBackground();
	update();
}

void DrumPatternEditor::mouseClickEvent( QMouseEvent *ev )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	if ( pHydrogen->getSong() == nullptr ) {
		return;
	}

	int nRow, nColumn, nRealColumn;
	mouseEventToColumnRow( ev, &nColumn, &nRow, &nRealColumn,
						   /* fineGrained */true );
	const auto drumPatternRow = m_pPatternEditorPanel->getRowDB( nRow );
	if ( drumPatternRow.nInstrumentID == EMPTY_INSTR_ID &&
		 drumPatternRow.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row clicked. y: %1, m_nGridHeight: %2, nRow: %3" )
				  .arg( ev->y() ).arg( m_nGridHeight ).arg( nRow ) );
		return;
	}

	if ( nColumn >= pPattern->getLength() ) {
		DEBUGLOG( QString( "Clicked beyond pattern length. x: %1, nColumn: %2, nMargin: %3, m_fGridWidth: %4, nRealColumn: %5" )
				  .arg( ev->x() ).arg( nColumn ).arg( PatternEditor::nMargin )
				  .arg( m_fGridWidth ).arg( nRealColumn ) );
		return;
	}

	if ( ev->button() == Qt::LeftButton ) {
		// Pressing Shift causes the added note to be of NoteOff type.
		addOrRemoveNote( nColumn, nRealColumn, nRow, KEY_MIN, OCTAVE_DEFAULT,
						 true, true, ev->modifiers() & Qt::ShiftModifier );
		m_selection.clearSelection();

	}
	else if ( ev->button() == Qt::RightButton ) {
		showPopupMenu( ev->globalPos() );
	}

	m_pPatternEditorPanel->setCursorColumn( nColumn );

	// Cursor either just got hidden or was moved.
	if ( ! pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getInstrumentList()->repaintInstrumentLines();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}
	update();
}

void DrumPatternEditor::mousePressEvent( QMouseEvent* ev ) {
	if ( ev->x() > m_nActiveWidth ) {
		return;
	}
	
	PatternEditor::mousePressEvent( ev );
	
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		return;
	}

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	pHydrogenApp->setHideKeyboardCursor( true );

	// Cursor just got hidden.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getInstrumentList()->repaintInstrumentLines();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		update();
	}

	int nRow, nColumn;
	mouseEventToColumnRow( ev, &nColumn, &nRow, nullptr, /* fineGrained */true );
	const auto drumPatternRow = m_pPatternEditorPanel->getRowDB( nRow );
	if ( drumPatternRow.nInstrumentID == EMPTY_INSTR_ID &&
		 drumPatternRow.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row clicked. y: %1, m_nGridHeight: %2, nRow: %3" )
				  .arg( ev->y() ).arg( m_nGridHeight ).arg( nRow ) );
		return;
	}

	m_pPatternEditorPanel->setSelectedRowDB( nRow );

	// Update cursor position
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		const auto pPattern = m_pPatternEditorPanel->getPattern();
		if ( pPattern != nullptr &&
			 nColumn >= m_pPatternEditorPanel->getPatternEditorRuler()->
				 getWidthActive() ) {
			return;
		}

		m_pPatternEditorPanel->setCursorColumn( nColumn );
	
		update();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}
}
	

void DrumPatternEditor::mouseDragStartEvent( QMouseEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	
	int nRow, nColumn, nRealColumn;
	mouseEventToColumnRow( ev, &nColumn, &nRow, &nRealColumn,
						   /* fineGrained */true );
	const auto drumPatternRow = m_pPatternEditorPanel->getRowDB( nRow );
	if ( drumPatternRow.nInstrumentID == EMPTY_INSTR_ID &&
		 drumPatternRow.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row clicked. y: %1, m_nGridHeight: %2, nRow: %3" )
				  .arg( ev->y() ).arg( m_nGridHeight ).arg( nRow ) );
		return;
	}

	m_pPatternEditorPanel->setSelectedRowDB( nRow );

	// Handles cursor repositioning and hiding and note property editing.
	PatternEditor::mouseDragStartEvent( ev );
}

///
/// Update the state during a Selection drag.
///
void DrumPatternEditor::mouseDragUpdateEvent( QMouseEvent *ev )
{
	int nRow;
	mouseEventToColumnRow( ev, nullptr, &nRow );

	if ( nRow <= -1 ) {
		return;
	}

	PatternEditor::mouseDragUpdateEvent( ev );
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
	
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	const int nBlockSize = 5, nWordSize = 5;
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();
	const int nMaxRows = m_pPatternEditorPanel->getRowNumberDB();
	bool bUnhideCursor = true;

	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Key was claimed by Selection
	} else if ( ev->matches( QKeySequence::MoveToNextChar ) ||
				ev->matches( QKeySequence::SelectNextChar ) ) {
		// ->
		m_pPatternEditorPanel->moveCursorRight();

	} else if ( ev->matches( QKeySequence::MoveToNextWord ) ||
				ev->matches( QKeySequence::SelectNextWord ) ) {
		// ->
		m_pPatternEditorPanel->moveCursorRight( nWordSize );

	} else if ( ev->matches( QKeySequence::MoveToEndOfLine ) ||
				ev->matches( QKeySequence::SelectEndOfLine ) ) {
		// -->|
		m_pPatternEditorPanel->setCursorColumn( pPattern->getLength() );

	} else if ( ev->matches( QKeySequence::MoveToPreviousChar ) ||
				ev->matches( QKeySequence::SelectPreviousChar ) ) {
		// <-
		m_pPatternEditorPanel->moveCursorLeft();

	} else if ( ev->matches( QKeySequence::MoveToPreviousWord ) ||
				ev->matches( QKeySequence::SelectPreviousWord ) ) {
		// <-
		m_pPatternEditorPanel->moveCursorLeft( nWordSize );

	} else if ( ev->matches( QKeySequence::MoveToStartOfLine ) ||
				ev->matches( QKeySequence::SelectStartOfLine ) ) {
		// |<--
		m_pPatternEditorPanel->setCursorColumn( 0 );

	} else if ( ev->matches( QKeySequence::MoveToNextLine ) ||
				ev->matches( QKeySequence::SelectNextLine ) ) {
		if ( nSelectedRow + 1 < nMaxRows ) {
			m_pPatternEditorPanel->setSelectedRowDB( nSelectedRow + 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) ||
				ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		m_pPatternEditorPanel->setSelectedRowDB(
			std::min( nSelectedRow + nBlockSize, nMaxRows-1 ) );

	} else if ( ev->matches( QKeySequence::MoveToNextPage ) ||
				ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down, scroll by the number of instruments that fit into the
		// viewport
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		m_pPatternEditorPanel->setSelectedRowDB(
			std::min( nMaxRows - 1,
					  nSelectedRow + static_cast<int>(pParent->height() /
													  m_nGridHeight) ) );

	} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ||
				ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		m_pPatternEditorPanel->setSelectedRowDB( nMaxRows-1 );

	} else if ( ev->matches( QKeySequence::MoveToPreviousLine ) ||
				ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( nSelectedRow > 0 ) {
			m_pPatternEditorPanel->setSelectedRowDB( nSelectedRow - 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) ||
				ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		m_pPatternEditorPanel->setSelectedRowDB(
			std::max( nSelectedRow - nBlockSize, 0 ) );

	} else if ( ev->matches( QKeySequence::MoveToPreviousPage ) ||
				ev->matches( QKeySequence::SelectPreviousPage ) ) {
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		m_pPatternEditorPanel->setSelectedRowDB(
			std::max( nSelectedRow - static_cast<int>(pParent->height() /
													  m_nGridHeight),
					  0 ) );

	} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ||
				ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		m_pPatternEditorPanel->setSelectedRowDB( 0 );

	} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter / Return: add or remove note at current position
		m_selection.clearSelection();
		addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
						 nSelectedRow );

	} else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete / Backspace: delete selected notes, or note under
		// keyboard cursor
		bUnhideCursor = false;
		if ( m_selection.begin() != m_selection.end() ) {
			// Delete selected notes if any
			deleteSelection();
		} else {
			// Delete note under the keyboard cursor.
			addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
							 nSelectedRow, KEY_MIN, OCTAVE_DEFAULT,
							 /*bDoAdd=*/false, /*bDoDelete=*/true );

		}

	} else if ( ev->matches( QKeySequence::SelectAll ) ) {
		bUnhideCursor = false;
		selectAll();

	} else if ( ev->matches( QKeySequence::Deselect ) ) {
		bUnhideCursor = false;
		selectNone();

	} else if ( ev->matches( QKeySequence::Copy ) ) {
		bUnhideCursor = false;
		copy();

	} else if ( ev->matches( QKeySequence::Paste ) ) {
		bUnhideCursor = false;
		paste();

	} else if ( ev->matches( QKeySequence::Cut ) ) {
		bUnhideCursor = false;
		cut();

	} else {
		ev->ignore();
		pHydrogenApp->setHideKeyboardCursor( true );
		
		if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
			m_pPatternEditorPanel->getInstrumentList()->repaintInstrumentLines();
			m_pPatternEditorPanel->getPatternEditorRuler()->update();
			update();
		}
		return;
	}
	if ( bUnhideCursor ) {
		pHydrogenApp->setHideKeyboardCursor( false );
	}
	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	m_pPatternEditorPanel->ensureCursorVisible();

	if ( m_selection.isLasso() ) {
		// Since event was used to alter the note selection, we invalidate
		// background and force a repainting of all note symbols (including
		// whether or not they are selected).
		invalidateBackground();
	}

	if ( ! pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getInstrumentList()->repaintInstrumentLines();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}
	update();
	ev->accept();

}

void DrumPatternEditor::keyReleaseEvent( QKeyEvent *ev ) {
	updateModifiers( ev );
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
	
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	auto  pInstrList = pSong->getDrumkit()->getInstruments();
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
		Note *note = it->second;
		int nInstrument = pInstrList->index( note->get_instrument() );
		uint x_pos = PatternEditor::nMargin + (it->first * m_fGridWidth);
		uint y_pos = ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3;

		if ( rNormalized.contains( QPoint( x_pos, y_pos + h/2) ) ) {
			result.push_back( note );
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

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto  pInstrList = pSong->getDrumkit()->getInstruments();

	validateSelection();

	for ( const auto& ppPattern : getPatternsToShow() ) {
		const Pattern::notes_t *pNotes = ppPattern->getNotes();
		if ( pNotes->size() == 0 ) {
			continue;
		}
		bool bIsForeground = ( ppPattern == pPattern );

		std::vector<int> noteCount; // instrument_id -> count
		std::stack<std::shared_ptr<Instrument>> instruments;

		// Process notes in batches by note position, counting the notes at each instrument so we can display
		// markers for instruments which have more than one note in the same position (a chord or genuine
		// duplicates)
		for ( auto posIt = pNotes->begin(); posIt != pNotes->end(); ) {
			if ( posIt->first >= ppPattern->getLength() ) {
				// Notes are located beyond the active length of the
				// editor and aren't visible even when drawn.
				break;
			}

			// Notes not associated with any instrument can not be copied for
			// now.
			if ( posIt->second == nullptr || posIt->second->get_instrument() == nullptr ) {
				++posIt;
				continue;
			}

			int nPosition = posIt->second->get_position();

			// Process all notes at this position
			auto noteIt = posIt;
			while ( noteIt != pNotes->end() && noteIt->second->get_position() == nPosition ) {
				Note *pNote = noteIt->second;
				// Notes not associated with any instrument can not be copied for
				// now.
				if ( pNote == nullptr || pNote->get_instrument() == nullptr ) {
					++noteIt;
					continue;
				}

				int nInstrumentID = pNote->get_instrument_id();
				// An ID of -1 corresponds to an empty instrument.
				if ( nInstrumentID >= 0 ) {
					if ( nInstrumentID >= noteCount.size() ) {
						noteCount.resize( nInstrumentID+1, 0 );
					}

					if ( ++noteCount[ nInstrumentID ] == 1) {
						instruments.push( pNote->get_instrument() );
					}

					drawNote( pNote, painter, bIsForeground );
				}

				++noteIt;
			}

			// Go through used instruments list, drawing markers for superimposed notes and zero'ing the
			// counts.
			while ( ! instruments.empty() ) {
				auto pInstrument = instruments.top();
				int nInstrumentID = pInstrument->get_id();
				if ( noteCount[ nInstrumentID ] >  1 ) {
					// Draw "2x" text to the left of the note
					int nInstrument = pInstrList->index( pInstrument );
					int x = PatternEditor::nMargin + (nPosition * m_fGridWidth);
					int y = ( nInstrument * m_nGridHeight);
					const int boxWidth = 128;

					QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
					painter.setFont( font );
					painter.setPen( QColor( 0, 0, 0 ) );

					painter.drawText( QRect( x-boxWidth-6, y, boxWidth, m_nGridHeight),
									  Qt::AlignRight | Qt::AlignVCenter,
									  ( QString( "%1" ) + QChar( 0x00d7 )).arg( noteCount[ nInstrumentID ] ) );
				}
				noteCount[ nInstrumentID ] = 0;
				instruments.pop();
			}

			posIt = noteIt;
		}
	}
}



///
/// Draws a note
///
void DrumPatternEditor::drawNote( Note *note, QPainter& p, bool bIsForeground )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	auto pInstrList = Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments();
	int nInstrument = pInstrList->index( note->get_instrument() );
	if ( nInstrument == -1 ) {
		return;
	}

	QPoint pos ( PatternEditor::nMargin + note->get_position() * m_fGridWidth,
				 ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3 );

	drawNoteSymbol( p, pos, note, bIsForeground );
}

void DrumPatternEditor::drawBackground( QPainter& p)
{
	const auto pPref = H2Core::Preferences::get_instance();

	const QColor backgroundColor(
		pPref->getTheme().m_color.m_patternEditor_backgroundColor );
	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );
	const QColor alternateRowColor(
		pPref->getTheme().m_color.m_patternEditor_alternateRowColor );
	const QColor selectedRowColor(
		pPref->getTheme().m_color.m_patternEditor_selectedRowColor );
	const QColor lineColor( pPref->getTheme().m_color.m_patternEditor_lineColor );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	const int nRows = m_pPatternEditorPanel->getRowNumberDB();
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();

	p.fillRect(0, 0, m_nActiveWidth, m_nEditorHeight, backgroundColor);
	if ( m_nActiveWidth < m_nEditorWidth ) {
		p.fillRect( m_nActiveWidth, 0, m_nEditorWidth - m_nActiveWidth,
					m_nEditorHeight, backgroundInactiveColor);
	}

	for ( int ii = 0; ii < nRows; ii++ ) {
		int y = static_cast<int>(m_nGridHeight) * ii;
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
	if ( pPattern == nullptr ) {
		return;
	}
	drawGridLines( p );

	// The grid lines above are drawn full height. We will erase the
	// upper part.
	for ( int ii = 0; ii < nRows; ii++ ) {
		int y = static_cast<int>(m_nGridHeight) * ii;
		if ( ii == nSelectedRow ) {
			p.fillRect( 0, y, m_nActiveWidth, (int)( m_nGridHeight * 0.7 ), selectedRowColor );
		} else {
			if ( ( ii % 2 ) == 0 ) {
				p.fillRect( 0, y, m_nActiveWidth, (int)( m_nGridHeight * 0.7 ), backgroundColor );
			} else {
				p.fillRect( 0, y, m_nActiveWidth,
							(int)( m_nGridHeight * 0.7 ), alternateRowColor );
			}
		}

		p.fillRect( m_nActiveWidth, y, m_nEditorWidth - m_nActiveWidth,
					(int)( m_nGridHeight * 0.7 ), backgroundInactiveColor );
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
	
	m_selection.paintSelection( &painter );

	// Draw cursor
	if ( hasFocus() && !HydrogenApp::get_instance()->hideKeyboardCursor() ) {
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
	int nEndY = std::min( static_cast<int>( m_nGridHeight ) * Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments()->size(),
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


///==========================================================
///undo / redo actions from pattern editor instrument list

void DrumPatternEditor::functionClearNotesUndoAction( const std::list< H2Core::Note* >& noteList,
													  int patternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	PatternList* pPatternList = pHydrogen->getSong()->getPatternList();
	auto pPattern = pPatternList->get( patternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( patternNumber ) );
		return;
	}

	std::list < H2Core::Note *>::const_iterator pos;
	for ( pos = noteList.begin(); pos != noteList.end(); ++pos){
		Note *pNote;
		pNote = new Note(*pos);
		assert( pNote );
		pPattern->insertNote( pNote );
	}

	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesUndoAction(
	H2Core::PatternList* pAppliedNotesPatternList )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPatternList = pSong->getPatternList();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	for ( const auto& ppAppliedPattern : *pAppliedNotesPatternList ) {
		if ( ppAppliedPattern == nullptr ) {
			ERRORLOG( "invalid applied pattern" );
			continue;
		}

		// Find destination pattern to perform undo
		auto pPattern = pPatternList->find( ppAppliedPattern->getName() );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "No pattern [%1] found in current pattern list" )
					  .arg( ppAppliedPattern->getName() ) );
			continue;
		}

		// Remove all notes of applied pattern from destination pattern
		auto pAppliedNotes = ppAppliedPattern->getNotes();

		// Const removed by cast since we do inplace changees.
		Pattern::notes_t* pSongNotes = (Pattern::notes_t *)pPattern->getNotes();
		FOREACH_NOTE_CST_IT_BEGIN_END( pAppliedNotes, it ) {
			const auto pNote = it->second;
			if ( pNote == nullptr ) {
				ERRORLOG( QString( "Invalid note at position [%1]" )
						  .arg( it->first ) );
				continue;
			}

			// Check if note is not present
			FOREACH_NOTE_IT_BOUND_END( pSongNotes, it, pNote->get_position() ) {
				const auto pFoundNote = it->second;
				if ( pFoundNote == nullptr ) {
					ERRORLOG( QString( "Invalid note found at position [%1]" )
							  .arg( it->first ) );
					continue;
				}

				// For now all instruments not assigned to any instrument will
				// be treated the same.
				if ( pFoundNote->get_instrument() == pNote->get_instrument() ) {
					pSongNotes->erase(it);
					delete pFoundNote;
					break;
				}
			}
		}
	}

	pHydrogen->getAudioEngine()->unlock();

	// Update editors
	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesRedoAction(
	H2Core::PatternList* pCopiedNotesPatternList,
	H2Core::PatternList* pAppliedPatternList )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() ==  nullptr ) {
		return;
	}

	const auto pDrumkit = pSong->getDrumkit();
	auto pPatternList = pSong->getPatternList();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	// Add notes to pattern of the pattern list sharing the same name.
	for ( const auto& ppCopiedPattern : *pCopiedNotesPatternList ) {
		if ( ppCopiedPattern == nullptr ) {
			ERRORLOG( "Invalid pattern" );
			continue;
		}

		auto pPattern = pPatternList->find( ppCopiedPattern->getName() );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "No pattern [%1] found in current pattern list" )
					  .arg( ppCopiedPattern->getName() ) );
			continue;
		}

		// Create applied pattern. (We do not need an exact copie. The name is
		// enough)
		auto pApplied = std::make_shared<Pattern>( pPattern->getName() );

		// Add all notes of source pattern to destination pattern
		// and store all applied notes in applied pattern
		const Pattern::notes_t* pCopiedNotes = ppCopiedPattern->getNotes();
		const Pattern::notes_t* pSongNotes = pPattern->getNotes();
		FOREACH_NOTE_CST_IT_BEGIN_END( pCopiedNotes, it ) {
			const auto pNote = it->second;
			if ( pNote == nullptr ) {
				ERRORLOG( QString( "Invalid note at position [%1]" )
						  .arg( it->first ) );
				continue;
			}

			// Check if note is already present for the same instrument.
			bool bNoteExists = false;
			FOREACH_NOTE_CST_IT_BOUND_END( pSongNotes, it, pNote->get_position() ) {
				const auto pFoundNote = it->second;
				if ( pFoundNote == nullptr ) {
					ERRORLOG( QString( "Invalid note found at position [%1]" )
							  .arg( it->first ) );
					continue;
				}

				// For now all instruments not assigned to any instrument will
				// be treated the same.
				if ( pFoundNote->get_instrument() == pNote->get_instrument() ) {
					bNoteExists = true;
					break;
				}
			}

			// Apply note and store it as applied
			if ( ! bNoteExists ) {
				pApplied->insertNote( new Note( pNote ) );

				// The note inserted in the song's pattern list needs to be
				// armed properly.
				Note* pNewNote = new Note( pNote );
				pNewNote->mapTo( pDrumkit );
				pPattern->insertNote( pNewNote );
			}
		}

		// Add applied pattern to applied list
		pAppliedPatternList->add( pApplied );
	}
	pHydrogen->getAudioEngine()->unlock();

	// Update editors
	m_pPatternEditorPanel->updateEditors();
}



void DrumPatternEditor::functionFillNotesUndoAction( const QStringList& noteList,
													 int nRow,
													 int nPatternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	PatternList* pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( nPatternNumber ) );
		return;
	}
	
	const auto row = m_pPatternEditorPanel->getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row [%1]" ).arg( nRow ) );
		return;
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );	// lock the audio engine

	for (int i = 0; i < noteList.size(); i++ ) {
		int nColumn  = noteList.value(i).toInt();
		Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->getNotes();
		FOREACH_NOTE_IT_BOUND_END(notes,it,nColumn) {
			auto pNote = it->second;
			if ( pNote != nullptr &&
				 pNote->get_instrument_id() == row.nInstrumentID &&
				 pNote->getType() == row.sType ) {
				// the note exists...remove it!
				notes->erase( it );
				delete pNote;
				break;
			}
		}
	}
	pHydrogen->getAudioEngine()->unlock();	// unlock the audio engine

	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionFillNotesRedoAction( const QStringList& noteList,
													 int nRow,
													 int nPatternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	PatternList* pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( nPatternNumber ) );
		return;
	}
	
	const auto row = m_pPatternEditorPanel->getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row [%1]" ).arg( nRow ) );
		return;
	}

	std::shared_ptr<Instrument> pInstrument = nullptr;
	if ( row.nInstrumentID != EMPTY_INSTR_ID ) {
		pInstrument =
			pSong->getDrumkit()->getInstruments()->find( row.nInstrumentID );
		if ( pInstrument == nullptr ) {
			ERRORLOG( QString( "Couldn't find instrument of ID [%1]" )
					  .arg( row.nInstrumentID ) );
			return;
		}
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );	// lock the audio engine
	for (int i = 0; i < noteList.size(); i++ ) {

		// create the new note
		int position = noteList.value(i).toInt();
		Note *pNote = new Note( pInstrument, position );
		pNote->set_instrument_id( row.nInstrumentID );
		pNote->setType( row.sType );
		pPattern->insertNote( pNote );
	}
	pHydrogen->getAudioEngine()->unlock();	// unlock the audio engine

	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionMoveInstrumentAction( int nSourceInstrument,
													  int nTargetInstrument )
{
		auto pHydrogen = Hydrogen::get_instance();
		pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

		std::shared_ptr<Song> pSong = pHydrogen->getSong();
		auto pInstrumentList = pSong->getDrumkit()->getInstruments();

		if ( ( nTargetInstrument > (int)pInstrumentList->size() ) || ( nTargetInstrument < 0) ) {
			pHydrogen->getAudioEngine()->unlock();
			return;
		}

		pInstrumentList->move( nSourceInstrument, nTargetInstrument );

		pHydrogen->renameJackPorts( pSong );

		pHydrogen->getAudioEngine()->unlock();
		m_pPatternEditorPanel->setSelectedRowDB( nTargetInstrument );

		pHydrogen->setIsModified( true );
}

/// ~undo / redo actions from pattern editor instrument list
///==========================================================
