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

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
using namespace H2Core;

#include <cassert>

#include "../Compatibility/MouseEvent.h"
#include "../Compatibility/WheelEvent.h"
#include "../HydrogenApp.h"

#include "UndoActions.h"
#include "NotePropertiesRuler.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "DrumPatternEditor.h"
#include "PianoRollEditor.h"
#include "../Skin.h"

int NotePropertiesRuler::nNoteKeyHeight =
	NotePropertiesRuler::nNoteKeyOctaveHeight +
	NotePropertiesRuler::nNoteKeyLineHeight * KEYS_PER_OCTAVE;


NotePropertiesRuler::NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, PatternEditor::Mode mode )
	: PatternEditor( parent, pPatternEditorPanel )
	, m_bEntered( false )
{

	m_editor = PatternEditor::Editor::NotePropertiesRuler;
	m_mode = mode;

	m_fGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );

	m_fLastSetValue = 0.0;
	m_bValueHasBeenSet = false;

	if ( m_mode == PatternEditor::Mode::NoteKey ) {
		m_nEditorHeight = NotePropertiesRuler::nNoteKeyHeight;
	}
	else {
		m_nEditorHeight = NotePropertiesRuler::nDefaultHeight;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumHeight( m_nEditorHeight );

	updateEditor();
	show();

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &NotePropertiesRuler::onPreferencesChanged );

	setFocusPolicy( Qt::StrongFocus );

	// Generic pattern editor menu contains some operations that don't apply here, and we will want to add
	// menu options specific to this later.
	delete m_pPopupMenu;
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, SLOT( selectAll() ) );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, SLOT( selectNone() ) );

	setMouseTracking( true );
}




NotePropertiesRuler::~NotePropertiesRuler()
{
}


//! Scroll wheel gestures will adjust the property of notes under the mouse cursor (or selected notes, if
//! any). Unlike drag gestures, each individual wheel movement will result in an undo/redo action since the
//! events are discrete.
void NotePropertiesRuler::wheelEvent(QWheelEvent *ev )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<WheelEvent*>( ev );

	prepareUndoAction( pEv->position().x() ); //get all old values

	float fDelta;
	if ( ev->modifiers() == Qt::ControlModifier ||
		 ev->modifiers() == Qt::AltModifier ) {
		fDelta = 0.01; // fine control
	} else {
		fDelta = 0.05; // coarse control
	}
	if ( ev->angleDelta().y() < 0 ) {
		fDelta = fDelta * -1.0;
	}

	int nColumn = getColumn( pEv->position().x() );

	m_pPatternEditorPanel->setCursorPosition( nColumn );

	auto pHydrogenApp = HydrogenApp::get_instance();
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	pHydrogenApp->setHideKeyboardCursor( true );

	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	// Gather notes to act on: selected or under the mouse cursor
	std::list< Note *> notes;
	if ( m_selection.begin() != m_selection.end() ) {
		for ( Note *pNote : m_selection ) {
			notes.push_back( pNote );
		}
	} else {
		FOREACH_NOTE_CST_IT_BOUND_LENGTH( m_pPattern->get_notes(), it, nColumn, m_pPattern ) {
			notes.push_back( it->second );
		}
	}
	
	bool bValueChanged = false;
	for ( Note *pNote : notes ) {
		assert( pNote );
		if ( pNote->get_instrument() != pSelectedInstrument && !m_selection.isSelected( pNote ) ) {
			continue;
		}
		bValueChanged = true;
		adjustNotePropertyDelta( pNote, fDelta, /* bMessage=*/ true );
	}
	
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		if ( ! bValueChanged ) {
			update();
		}
	}

	if ( bValueChanged ) {
		addUndoAction();
		invalidateBackground();
		update();
	}
}


void NotePropertiesRuler::mouseClickEvent( QMouseEvent *ev ) {
	auto pEv = static_cast<MouseEvent*>( ev );

	if ( ev->button() == Qt::RightButton ) {
		m_pPopupMenu->popup( pEv->globalPosition().toPoint() );

	} else {
		// Treat single click as an instantaneous drag
		propertyDragStart( ev );
		propertyDragUpdate( ev );
		propertyDragEnd();
	}
}

void NotePropertiesRuler::mousePressEvent( QMouseEvent* ev ) {
	auto pEv = static_cast<MouseEvent*>( ev );

	if ( pEv->position().x() > m_nActiveWidth ) {
		return;
	}

	PatternEditor::mousePressEvent( ev );

	auto pHydrogenApp = HydrogenApp::get_instance();

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	pHydrogenApp->setHideKeyboardCursor( true );

	// Cursor just got hidden.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		update();
	}
	
	// Update cursor position
	if ( ! pHydrogenApp->hideKeyboardCursor() ) {
		int nColumn = getColumn( pEv->position().x(), /* bUseFineGrained=*/ true );
		if ( ( m_pPattern != nullptr &&
			   nColumn >= (int)m_pPattern->get_length() ) ||
			 nColumn >= MAX_INSTRUMENTS ) {
			return;
		}

		m_pPatternEditorPanel->setCursorPosition( nColumn );
	
		update();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}
}

void NotePropertiesRuler::mouseDragStartEvent( QMouseEvent *ev ) {
	auto pEv = static_cast<MouseEvent*>( ev );

	if ( m_selection.isMoving() ) {
		prepareUndoAction( pEv->position().x() );
		selectionMoveUpdateEvent( ev );
	} else {
		propertyDragStart( ev );
		propertyDragUpdate( ev );
	}
}

void NotePropertiesRuler::mouseDragUpdateEvent( QMouseEvent *ev ) {
	propertyDragUpdate( ev );
}

void NotePropertiesRuler::mouseDragEndEvent( QMouseEvent *ev ) {
	propertyDragEnd();
}


void NotePropertiesRuler::selectionMoveUpdateEvent( QMouseEvent *ev ) {
	if ( m_pPattern == nullptr ) {
		return;
	}
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}
	
	float fDelta;

	QPoint movingOffset = m_selection.movingOffset();
	if ( m_mode == PatternEditor::Mode::NoteKey ) {
		fDelta = (float)-movingOffset.y() /
			static_cast<float>(NotePropertiesRuler::nNoteKeyLineHeight);
	} else {
		fDelta = (float)-movingOffset.y() / height();
	}

	// Only send a status message for the update in case a single note
	// was selected.
	bool bSendStatusMsg = false;
	int nNotes = 0;
	for ( Note *pNote : m_selection ) {
		++nNotes;
	}
	if ( nNotes == 1 ) {
		bSendStatusMsg = true;
	}

	bool bValueChanged = false;
	for ( Note *pNote : m_selection ) {
		if ( pNote->get_instrument() == pSelectedInstrument || m_selection.isSelected( pNote ) ) {

			// Record original note if not already recorded
			if ( m_oldNotes.find( pNote ) == m_oldNotes.end() ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}

			adjustNotePropertyDelta( pNote, fDelta, bSendStatusMsg );
			bValueChanged = true;
		}
	}

	if ( bValueChanged ) {
		invalidateBackground();
		update();
	}
}

void NotePropertiesRuler::selectionMoveEndEvent( QInputEvent *ev ) {
	//! The "move" has already been reflected in the notes. Now just complete Undo event.
	addUndoAction();
	invalidateBackground();
	update();
}

void NotePropertiesRuler::clearOldNotes() {
	for ( auto it : m_oldNotes ) {
		delete it.second;
	}
	m_oldNotes.clear();
}

//! Move of selection is cancelled. Revert notes to preserved state.
void NotePropertiesRuler::selectionMoveCancelEvent() {
	for ( auto it : m_oldNotes ) {
		Note *pNote = it.first, *pOldNote = it.second;
		switch ( m_mode ) {
		case PatternEditor::Mode::Velocity:
			pNote->set_velocity( pOldNote->get_velocity() );
			break;
		case PatternEditor::Mode::Pan:
			pNote->setPan( pOldNote->getPan() );
			break;
		case PatternEditor::Mode::LeadLag:
			pNote->set_lead_lag( pOldNote->get_lead_lag() );
			break;
		case PatternEditor::Mode::NoteKey:
			pNote->set_key_octave( pOldNote->get_key(), pOldNote->get_octave() );
			break;
		case PatternEditor::Mode::Probability:
			pNote->set_probability( pOldNote->get_probability() );
			break;
		default:
			break;
		}
	}

	if ( m_oldNotes.size() == 0 ) {
		for ( const auto& it : m_oldNotes ){
			PatternEditor::triggerStatusMessage( it.second, m_mode );
		}
	}

	clearOldNotes();
}


void NotePropertiesRuler::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	
	if ( ev->buttons() == Qt::NoButton ) {
		int nColumn = getColumn( pEv->position().x() );
		bool bFound = false;
		FOREACH_NOTE_CST_IT_BOUND_LENGTH( m_pPattern->get_notes(), it, nColumn, m_pPattern ) {
			bFound = true;
			break;
		}
		if ( bFound ) {
			setCursor( Qt::PointingHandCursor );
		} else {
			unsetCursor();
		}

	} else {
		PatternEditor::mouseMoveEvent( ev );
	}
}


void NotePropertiesRuler::propertyDragStart( QMouseEvent *ev )
{
	auto pEv = static_cast<MouseEvent*>( ev );

	setCursor( Qt::CrossCursor );
	prepareUndoAction( pEv->position().x() );
	invalidateBackground();
	update();
}


//! Preserve current note properties at position x (or in selection, if any) for use in later UndoAction.
void NotePropertiesRuler::prepareUndoAction( int x )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	clearOldNotes();

	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}
	const int nColumn = getColumn( x );

	if ( m_selection.begin() != m_selection.end() ) {
		// If there is a selection, preserve the initial state of all the selected notes.
		for ( Note *pNote : m_selection ) {
			if ( pNote->get_instrument() == pSelectedInstrument || m_selection.isSelected( pNote ) ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}
		}

	} else {
		// No notes are selected. The target notes to adjust are all those at
		// column given by 'x', so we preserve these.
		FOREACH_NOTE_CST_IT_BOUND_LENGTH( m_pPattern->get_notes(), it,
										  nColumn, m_pPattern ) {
			Note *pNote = it->second;
			if ( pNote->get_instrument() == pSelectedInstrument ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}
		}
	}

	m_nDragPreviousColumn = nColumn;
}

//! Update notes for a property adjust drag, in response to the mouse moving. This modifies the values of the
//! notes as the mouse moves, but does not complete an undo action until the notes final value has been
//! set. This occurs either when the mouse is released, or when the pointer moves off of the note's column.
void NotePropertiesRuler::propertyDragUpdate( QMouseEvent *ev )
{
	if (m_pPattern == nullptr) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	int nColumn = getColumn( pEv->position().x() );

	m_pPatternEditorPanel->setCursorPosition( nColumn );

	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	pHydrogenApp->setHideKeyboardCursor( true );

	if ( m_nDragPreviousColumn != nColumn ) {
		// Complete current undo action, and start a new one.
		addUndoAction();
		prepareUndoAction( pEv->position().x() );
	}

	float val = height() - pEv->position().y();
	if (val > height()) {
		val = height();
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	val = val / height(); // val is normalized, in [0;1]
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	bool bValueSet = false;

	FOREACH_NOTE_CST_IT_BOUND_LENGTH( m_pPattern->get_notes(), it, nColumn, m_pPattern ) {
		Note *pNote = it->second;

		if ( pNote->get_instrument() != pSelectedInstrument &&
			 !m_selection.isSelected( pNote ) ) {
			continue;
		}
		if ( m_mode == PatternEditor::Mode::Velocity && !pNote->get_note_off() ) {
			pNote->set_velocity( val );
			m_fLastSetValue = val;
			bValueSet = true;
		}
		else if ( m_mode == PatternEditor::Mode::Pan && !pNote->get_note_off() ){
			if ( (ev->button() == Qt::MiddleButton)
					|| (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				val = 0.5; // central pan
			}
			pNote->setPanWithRangeFrom0To1( val ); // checks the boundaries
			m_fLastSetValue = pNote->getPanWithRangeFrom0To1();
			bValueSet = true;
			
		}
		else if ( m_mode == PatternEditor::Mode::LeadLag ){
			if ( (ev->button() == Qt::MiddleButton) ||
				 (ev->modifiers() == Qt::ControlModifier &&
				  ev->button() == Qt::LeftButton) ) {
				pNote->set_lead_lag(0.0);
				m_fLastSetValue = 0.0;
				bValueSet = true;
			}
			else {
				m_fLastSetValue = val * -2.0 + 1.0;
				bValueSet = true;
				pNote->set_lead_lag( m_fLastSetValue );
			}
		}
		else if ( m_mode == PatternEditor::Mode::NoteKey ){
			if ( ev->button() != Qt::MiddleButton &&
				 ! ( ev->modifiers() == Qt::ControlModifier &&
					 ev->button() == Qt::LeftButton ) ) {
				int nKey = 666;
				int nOctave = 666;
				if ( pEv->position().y() > 0 &&
					 pEv->position().y() <= NotePropertiesRuler::nNoteKeyOctaveHeight ) {
					nOctave = std::round(
						( NotePropertiesRuler::nNoteKeyOctaveHeight / 2 +
						  NotePropertiesRuler::nNoteKeyLineHeight / 2 -
						  pEv->position().y() -
						  NotePropertiesRuler::nNoteKeyLineHeight / 2 ) /
						NotePropertiesRuler::nNoteKeyLineHeight );
					nOctave = std::clamp( nOctave, OCTAVE_MIN, OCTAVE_MAX );
				}
				else if ( pEv->position().y() >= NotePropertiesRuler::nNoteKeyOctaveHeight &&
						  pEv->position().y() < NotePropertiesRuler::nNoteKeyHeight ) {
					nKey = ( height() - pEv->position().y() -
							 NotePropertiesRuler::nNoteKeyLineHeight / 2 ) /
						NotePropertiesRuler::nNoteKeyLineHeight;
					nKey = std::clamp( nKey, KEY_MIN, KEY_MAX );
				}

				if ( nKey != 666 || nOctave != 666 ) {
					m_fLastSetValue = nOctave * KEYS_PER_OCTAVE + nKey;
					bValueSet = true;
					pNote->set_key_octave((Note::Key)nKey,(Note::Octave)nOctave); // won't set wrong values see Note::set_key_octave
				}
			}
		}
		else if ( m_mode == PatternEditor::Mode::Probability && !pNote->get_note_off() ) {
			m_fLastSetValue = val;
			bValueSet = true;
			pNote->set_probability( val );
		}
		
		if ( bValueSet ) {
			PatternEditor::triggerStatusMessage( pNote, m_mode );
			m_bValueHasBeenSet = true;
			Hydrogen::get_instance()->setIsModified( true );
		}
	}

	// Cursor just got hidden.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}

	m_nDragPreviousColumn = nColumn;
	invalidateBackground();
	update();

	m_pPatternEditorPanel->getPianoRollEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}

void NotePropertiesRuler::propertyDragEnd()
{
	addUndoAction();
	unsetCursor();
	invalidateBackground();
	update();
}

//! Adjust a note's property by applying a delta to the current value, and clipping to the appropriate
//! range. Optionally, show a message with the value for some properties.
void NotePropertiesRuler::adjustNotePropertyDelta( Note *pNote, float fDelta, bool bMessage )
{
	Note *pOldNote = m_oldNotes[ pNote ];
	assert( pOldNote );

	bool bValueSet = false;
	
	switch (m_mode) {
	case PatternEditor::Mode::Velocity:
		if ( !pNote->get_note_off() ) {
			float fVelocity = qBound(  VELOCITY_MIN, (pOldNote->get_velocity() + fDelta), VELOCITY_MAX );
			pNote->set_velocity( fVelocity );
			m_fLastSetValue = fVelocity;
			bValueSet = true;
		}
		break;
	case PatternEditor::Mode::Pan:
		if ( !pNote->get_note_off() ) {
			float fVal = pOldNote->getPanWithRangeFrom0To1() + fDelta; // value in [0,1] or slight out of boundaries
			pNote->setPanWithRangeFrom0To1( fVal ); // checks the boundaries as well
			m_fLastSetValue = pNote->getPanWithRangeFrom0To1();
			bValueSet = true;
		}
		break;
	case PatternEditor::Mode::LeadLag:
		{
			float fLeadLag = qBound( LEAD_LAG_MIN, pOldNote->get_lead_lag() - fDelta, LEAD_LAG_MAX );
			pNote->set_lead_lag( fLeadLag );
			m_fLastSetValue = fLeadLag;
			bValueSet = true;
		}
		break;
	case PatternEditor::Mode::Probability:
		if ( !pNote->get_note_off() ) {
			float fProbability = qBound( 0.0f, pOldNote->get_probability() + fDelta, 1.0f );
			pNote->set_probability( fProbability );
			m_fLastSetValue = fProbability;
			bValueSet = true;
		}
		break;
	case PatternEditor::Mode::NoteKey:
		int nPitch = qBound( KEYS_PER_OCTAVE * OCTAVE_MIN, (int)( pOldNote->get_notekey_pitch() + fDelta ),
							 KEYS_PER_OCTAVE * OCTAVE_MAX + KEY_MAX );
		Note::Octave octave;
		if ( nPitch >= 0 ) {
			octave = (Note::Octave)( nPitch / KEYS_PER_OCTAVE );
		} else {
			octave = (Note::Octave)( (nPitch-11) / KEYS_PER_OCTAVE );
		}
		Note::Key key = (Note::Key)( nPitch - KEYS_PER_OCTAVE * (int)octave );

		pNote->set_key_octave( key, octave );
		m_fLastSetValue = KEYS_PER_OCTAVE * octave + key;

		bValueSet = true;
		break;
	}

	if ( bValueSet ) {
		Hydrogen::get_instance()->setIsModified( true );
		m_bValueHasBeenSet = true;
		if ( bMessage ) {
			PatternEditor::triggerStatusMessage( pNote, m_mode );
		}
	}
}

void NotePropertiesRuler::keyPressEvent( QKeyEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	const int nWordSize = 5;
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bUnhideCursor = true;

	bool bValueChanged = false;

	if ( bIsSelectionKey ) {
		// Key was claimed by selection
	} else if ( ev->matches( QKeySequence::MoveToNextChar ) || ev->matches( QKeySequence::SelectNextChar ) ) {
		// ->
		m_pPatternEditorPanel->moveCursorRight();

	} else if ( ev->matches( QKeySequence::MoveToNextWord ) || ev->matches( QKeySequence::SelectNextWord ) ) {
		// ->
		m_pPatternEditorPanel->moveCursorRight( nWordSize );

	} else if ( ev->matches( QKeySequence::MoveToEndOfLine ) || ev->matches( QKeySequence::SelectEndOfLine ) ) {
		// -->|
		m_pPatternEditorPanel->setCursorPosition( m_pPattern->get_length() );

	} else if ( ev->matches( QKeySequence::MoveToPreviousChar ) || ev->matches( QKeySequence::SelectPreviousChar ) ) {
		// <-
		m_pPatternEditorPanel->moveCursorLeft();

	} else if ( ev->matches( QKeySequence::MoveToPreviousWord ) || ev->matches( QKeySequence::SelectPreviousWord ) ) {
		// <-
		m_pPatternEditorPanel->moveCursorLeft( nWordSize );

	} else if ( ev->matches( QKeySequence::MoveToStartOfLine ) || ev->matches( QKeySequence::SelectStartOfLine ) ) {
		// |<--
		m_pPatternEditorPanel->setCursorPosition(0);

	} else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete / Backspace: delete selected notes, or note under keyboard cursor
		bUnhideCursor = false;
		if ( m_selection.begin() != m_selection.end() ) {
			// Delete selected notes if any
			m_pPatternEditorPanel->getDrumPatternEditor()->
				deleteSelection();
		} else {
			// Delete note under the keyboard cursor.
			m_pPatternEditorPanel->getDrumPatternEditor()->
				addOrRemoveNote( m_pPatternEditorPanel->getCursorPosition(), -1,
								 pHydrogen->getSelectedInstrumentNumber(),
								 /*bDoAdd=*/false, /*bDoDelete=*/true );
		}

	} else {

		// Value adjustments
		float fDelta = 0.0;
		bool bRepeatLastValue = false;

		if ( ev->matches( QKeySequence::MoveToPreviousLine ) ) {
			// Key: Up: increase note parameter value
			fDelta = 0.1;

		} else if ( ev->key() == Qt::Key_Up && ev->modifiers() & Qt::AltModifier ) {
			// Key: Alt+Up: increase parameter slightly
			fDelta = 0.01;

		} else if ( ev->matches( QKeySequence::MoveToNextLine ) ) {
			// Key: Down: decrease note parameter value
			fDelta = -0.1;

		} else if ( ev->key() == Qt::Key_Down && ev->modifiers() & Qt::AltModifier ) {
			// Key: Alt+Up: decrease parameter slightly
			fDelta = -0.01;

		} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ) {
			// Key: MoveToStartOfDocument: increase parameter to maximum value
			fDelta = 1.0;

		} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ) {
			// Key: MoveEndOfDocument: decrease parameter to minimum value
			fDelta = -1.0;

		} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
			// Key: Enter/Return: repeat last parameter value set.
			if ( m_bValueHasBeenSet ) {
				bRepeatLastValue = true;
			}

		} else if ( ev->matches( QKeySequence::SelectAll ) ) {
			// Key: Ctrl + A: Select all
			bUnhideCursor = false;
			selectAll();

		} else if ( ev->matches( QKeySequence::Deselect ) ) {
			// Key: Shift + Ctrl + A: clear selection
			bUnhideCursor = false;
			selectNone();

		}

		if ( fDelta != 0.0 || bRepeatLastValue ) {
			int column = m_pPatternEditorPanel->getCursorPosition();

			auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
			if ( pSelectedInstrument == nullptr ) {
				ERRORLOG( "No instrument selected" );
				return;
			}
			
			int nNotes = 0;

			// Collect notes to apply the change to
			std::list< Note *> notes;
			if ( m_selection.begin() != m_selection.end() ) {
				for ( Note *pNote : m_selection ) {
					nNotes++;
					notes.push_back( pNote );
				}
			} else {
				FOREACH_NOTE_CST_IT_BOUND_LENGTH( m_pPattern->get_notes(), it, column, m_pPattern ) {
					Note *pNote = it->second;
					assert( pNote );
					assert( pNote->get_position() == column );
					if ( pNote->get_instrument() == pSelectedInstrument ) {
						nNotes++;
						notes.push_back( pNote );
					}
				}
			}

			// For the NoteKeyEditor, adjust the pitch by a whole semitone
			if ( m_mode == PatternEditor::Mode::NoteKey ) {
				if ( fDelta > 0.0 ) {
					fDelta = 1;
				} else if ( fDelta < 0.0 ) {
					fDelta = -1;
				}
			}

			prepareUndoAction( PatternEditor::nMargin + column * m_fGridWidth );

			for ( Note *pNote : notes ) {
				bValueChanged = true;

				if ( !bRepeatLastValue ) {
					
					// Apply delta to the property
					adjustNotePropertyDelta( pNote, fDelta, nNotes == 1 );

				} else {

					bool bValueSet = false;
					
					// Repeating last value
					switch (m_mode) {
					case PatternEditor::Mode::Velocity:
						if ( !pNote->get_note_off() ) {
							pNote->set_velocity( m_fLastSetValue );
							bValueSet = true;
						}
						break;
					case PatternEditor::Mode::Pan:
						if ( !pNote->get_note_off() ) {
							if ( m_fLastSetValue > 1. ) { // TODO whats this for? is it ever reached?
								printf( "reached  m_fLastSetValue > 1 in NotePropertiesRuler.cpp\n" );
								pNote->setPanWithRangeFrom0To1( m_fLastSetValue );
							}
							bValueSet = true;
						}
						break;
					case PatternEditor::Mode::LeadLag:
						pNote->set_lead_lag( m_fLastSetValue );
							bValueSet = true;
						break;
					case PatternEditor::Mode::Probability:
						if ( !pNote->get_note_off() ) {
							pNote->set_probability( m_fLastSetValue );
							bValueSet = true;
						}
						break;
					case PatternEditor::Mode::NoteKey:
						pNote->set_key_octave( (Note::Key)( (int)m_fLastSetValue % 12 ),
											   (Note::Octave)( (int)m_fLastSetValue / 12 ) );
						bValueSet = true;
						break;
					}

					if ( bValueSet ) {
						if ( nNotes == 1 ) {
							PatternEditor::triggerStatusMessage( pNote, m_mode );
						}
						Hydrogen::get_instance()->setIsModified( true );
					}
				}
			}
			addUndoAction();
		} else {
			pHydrogenApp->setHideKeyboardCursor( true );
			ev->ignore();
			
			// Cursor either just got hidden.
			if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
				// Immediate update to prevent visual delay.
				m_pPatternEditorPanel->getPatternEditorRuler()->update();
				update();
			}
			return;
		}
	}
	if ( bUnhideCursor ) {
		pHydrogenApp->setHideKeyboardCursor( false );
	}

	// Cursor either just got hidden or was moved.
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() || 
		bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}

	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	
	if ( bValueChanged ) {
		invalidateBackground();
	}
	update();
	
	ev->accept();

}

void NotePropertiesRuler::addUndoAction()
{
	if ( m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	auto pInstrumentList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	int nSize = m_oldNotes.size();
	if ( nSize != 0 ) {
		QUndoStack *pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;

		if ( nSize != 1 ) {
			pUndoStack->beginMacro( QString( tr( "Edit [%1] property of [%2] notes" ) )
									.arg( NotePropertiesRuler::modeToQString( m_mode ) )
									.arg( nSize ) );
		}
		for ( auto it : m_oldNotes ) {
			Note *pNewNote = it.first, *pOldNote = it.second;
			pUndoStack->push( new SE_editNotePropertiesVolumeAction( pNewNote->get_position(),
																	 m_mode,
																	 m_nSelectedPatternNumber,
																	 pInstrumentList->index( pNewNote->get_instrument() ),
																	 pNewNote->get_velocity(),
																	 pOldNote->get_velocity(),
																	 pNewNote->getPan(),
																	 pOldNote->getPan(),
																	 pNewNote->get_lead_lag(),
																	 pOldNote->get_lead_lag(),
																	 pNewNote->get_probability(),
																	 pOldNote->get_probability(),
																	 pNewNote->get_key(),
																	 pOldNote->get_key(),
																	 pNewNote->get_octave(),
																	 pOldNote->get_octave() ) );
		}
		if ( nSize != 1 ) {
			pUndoStack->endMacro();
		}
	}
	clearOldNotes();
}

void NotePropertiesRuler::paintEvent( QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}

	auto pPref = Preferences::get_instance();
	
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_bBackgroundInvalid ) {
		createBackground();
	}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap,
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
		painter.drawLine( nX, 0, nX, height() );
	}
	
	drawFocus( painter );
	
	m_selection.paintSelection( &painter );

	// cursor
	if ( hasFocus() && ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		uint x = PatternEditor::nMargin + m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;

		QPen pen( pPref->getColorTheme()->m_cursorColor );
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.setBrush( Qt::NoBrush );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( QRect( x-m_fGridWidth*3, 0 + 3, m_fGridWidth*6, height() - 6 ), 4, 4 );
	}
}

void NotePropertiesRuler::drawFocus( QPainter& painter ) {

	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}
	
	auto pPref = H2Core::Preferences::get_instance();
	
	QColor color = pPref->getColorTheme()->m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	const QScrollArea* pScrollArea;
	
	switch ( m_mode ) {
	case PatternEditor::Mode::Velocity:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteVelocityScrollArea();
		break;
	case PatternEditor::Mode::Pan:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNotePanScrollArea();
		break;
	case PatternEditor::Mode::LeadLag:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteLeadLagScrollArea();
		break;
	case PatternEditor::Mode::NoteKey:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteNoteKeyScrollArea();
		break;
	case PatternEditor::Mode::Probability:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteProbabilityScrollArea();
		break;
	default:
		return;
	}
	int nStartY = pScrollArea->verticalScrollBar()->value();
	int nStartX = pScrollArea->horizontalScrollBar()->value();
	int nEndY = nStartY + pScrollArea->viewport()->size().height();
	// In order to match the width used in the DrumPatternEditor.
	int nEndX = std::min( nStartX + pScrollArea->viewport()->size().width(),
						  static_cast<int>( m_nEditorWidth ) );

	int nMargin;
	if ( nEndX == static_cast<int>( m_nEditorWidth ) ) {
		nEndX = nEndX - 2;
		nMargin = 1;
	} else {
		nMargin = 0;
	}

	QPen pen( color );
	pen.setWidth( 4 );
	painter.setPen( pen );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nEndX, nStartY ) );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nStartX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nEndY ), QPoint( nStartX, nEndY ) );

	if ( nMargin != 0 ) {
		// Since for all other lines we are drawing at a border with just
		// half of the line being painted in the visual viewport, there
		// has to be some tweaking since the NotePropertiesRuler is
		// paintable to the right.
		pen.setWidth( 2 );
		painter.setPen( pen );
	}
	painter.drawLine( QPoint( nEndX + nMargin, nStartY ), QPoint( nEndX + nMargin, nEndY ) );
		
}

void NotePropertiesRuler::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

#ifdef H2CORE_HAVE_QT6
void NotePropertiesRuler::enterEvent( QEnterEvent *ev ) {
#else
void NotePropertiesRuler::enterEvent( QEvent *ev ) {
#endif
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void NotePropertiesRuler::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void NotePropertiesRuler::drawDefaultBackground( QPainter& painter, int nHeight, int nIncrement ) {
	
	auto pPref = H2Core::Preferences::get_instance();

	const QColor borderColor( pPref->getColorTheme()->m_patternEditor_lineColor );
	const QColor lineColor( pPref->getColorTheme()->m_patternEditor_line5Color );
	const QColor lineInactiveColor( pPref->getColorTheme()->m_windowTextColor.darker( 170 ) );
	const QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );
	const QColor backgroundInactiveColor( pPref->getColorTheme()->m_windowColor );

	if ( nHeight == 0 ) {
		nHeight = height();
	}
	if ( nIncrement == 0 ) {
		nIncrement = nHeight / 10;
	}

	painter.fillRect( 0, 0, m_nActiveWidth, height(), backgroundColor );
	painter.fillRect( m_nActiveWidth, 0, m_nEditorWidth - m_nActiveWidth,
					  height(), backgroundInactiveColor );

	drawGridLines( painter, Qt::DotLine );
	
	painter.setPen( lineColor );
	for (unsigned y = 0; y < nHeight; y += nIncrement ) {
		painter.drawLine( PatternEditor::nMargin, y, m_nActiveWidth, y );
	}
	
	painter.setPen( borderColor );
	painter.drawLine( 0, 0, m_nActiveWidth, 0 );
	painter.drawLine( 0, m_nEditorHeight - 1, m_nActiveWidth, m_nEditorHeight - 1 );

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		painter.setPen( lineInactiveColor );
		for (unsigned y = 0; y < nHeight; y += nIncrement ) {
			painter.drawLine( m_nActiveWidth, y, m_nEditorWidth, y );
		}
	
		painter.drawLine( m_nActiveWidth, 0, m_nEditorWidth, 0 );
		painter.drawLine( m_nActiveWidth, m_nEditorHeight - 1,
						  m_nEditorWidth, m_nEditorHeight - 1 );
	}
}

void NotePropertiesRuler::createNormalizedBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	QColor borderColor( pPref->getColorTheme()->m_patternEditor_lineColor );
	const QColor lineInactiveColor( pPref->getColorTheme()->m_windowTextColor.darker( 170 ) );
	QPainter p( pixmap );

	drawDefaultBackground( p );

	// draw velocity lines
	if ( m_pPattern != nullptr ) {
		auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
		if ( pSelectedInstrument == nullptr ) {
			ERRORLOG( "No instrument selected" );
			return;
		}

		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_LENGTH(notes,it, m_pPattern) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND_LENGTH(notes,coit,pos, m_pPattern) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSelectedInstrument
					 && !m_selection.isSelected( pNote ) ) {
					continue;
				}
				uint x_pos = PatternEditor::nMargin + pos * m_fGridWidth;
				uint line_end = height();


				uint value = 0;
				if ( m_mode == PatternEditor::Mode::Velocity ) {
					value = (uint)(pNote->get_velocity() * height());
				}
				else if ( m_mode == PatternEditor::Mode::Probability ) {
					value = (uint)(pNote->get_probability() * height());
				}
				uint line_start = line_end - value;
				QColor noteColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
				int nLineWidth = 3;

				p.fillRect( x_pos - 1 + xoffset, line_start,
							nLineWidth, line_end - line_start,
							noteColor );
				p.setPen( QPen( Qt::black, 1 ) );
				p.setRenderHint( QPainter::Antialiasing );
				p.drawRoundedRect( x_pos - 1 - 1 + xoffset, line_start - 1,
								   nLineWidth + 2, line_end - line_start + 2, 2, 2 );
				
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, line_start - 2,
									   nLineWidth + 4,  line_end - line_start + 4 ,
									   4, 4 );
				}
				xoffset++;
			}
		}
	}
	
	p.setPen( borderColor );
	p.setRenderHint( QPainter::Antialiasing );
	p.drawLine( 0, 0, m_nEditorWidth, 0 );
	p.setPen( QPen( borderColor, 2 ) );
	p.drawLine( 0, m_nEditorHeight, m_nEditorWidth, m_nEditorHeight );
	
	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		p.drawLine( m_nActiveWidth, 0, m_nEditorWidth, 0 );
		p.setPen( QPen( lineInactiveColor, 2 ) );
		p.drawLine( m_nActiveWidth, m_nEditorHeight,
					m_nEditorWidth, m_nEditorHeight );
	}
}

void NotePropertiesRuler::createCenteredBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	
	QColor baseLineColor( pPref->getColorTheme()->m_patternEditor_lineColor );
	QColor borderColor( pPref->getColorTheme()->m_patternEditor_lineColor );
	const QColor lineInactiveColor( pPref->getColorTheme()->m_windowTextColor.darker( 170 ) );

	QPainter p( pixmap );

	drawDefaultBackground( p );

	// central line
	p.setPen( baseLineColor );
	p.drawLine(0, height() / 2.0, m_nActiveWidth, height() / 2.0);
	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		p.drawLine( m_nActiveWidth, height() / 2.0,
					m_nEditorWidth, height() / 2.0);
	}

	if ( m_pPattern != nullptr ) {
		auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
		if ( pSelectedInstrument == nullptr ) {
			ERRORLOG( "No instrument selected" );
			return;
		}
		
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_LENGTH(notes,it, m_pPattern) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND_LENGTH(notes,coit,pos, m_pPattern) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_note_off() || (pNote->get_instrument()
											   != pSelectedInstrument
											   && !m_selection.isSelected( pNote ) ) ) {
					continue;
				}
				uint x_pos = PatternEditor::nMargin + pNote->get_position() * m_fGridWidth;
				QColor noteColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );

				p.setPen( Qt::NoPen );

				float fValue = 0;
				if ( m_mode == PatternEditor::Mode::Pan ) {
					fValue = pNote->getPan();
				} else if ( m_mode == PatternEditor::Mode::LeadLag ) {
					fValue = -1 * pNote->get_lead_lag();
				}

				// Rounding in order to not miss the center due to
				// rounding errors introduced in the Note class
				// internals.
				fValue *= 100;
				fValue = std::round( fValue );
				fValue /= 100;

				int nLineWidth = 3;
				p.setPen( QPen( Qt::black, 1 ) );
				p.setRenderHint( QPainter::Antialiasing );
				if ( fValue == 0.f ) {
					// value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( noteColor ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
					p.setBrush( Qt::NoBrush );

					if ( m_selection.isSelected( pNote ) ) {
						p.setPen( selectedPen );
						p.setRenderHint( QPainter::Antialiasing );
						p.drawEllipse( x_pos - 6 + xoffset, y_pos - 6,
									   12, 12);
					}
				}
				else {
					// value was altered - draw a rectangle
					int nHeight = 0.5 * height() * std::abs( fValue ) + 5;
					int nStartY = height() * 0.5 - 2;
					if ( fValue >= 0 ) {
						nStartY = nStartY - nHeight + 5;
					}

					p.fillRect( x_pos - 1 + xoffset, nStartY,
								nLineWidth, nHeight, QColor( noteColor ) );
					p.drawRoundedRect( x_pos - 1 + xoffset - 1, nStartY - 1,
									   nLineWidth + 2, nHeight + 2, 2, 2 );

					if ( m_selection.isSelected( pNote ) ) {
						p.setPen( selectedPen );
						p.drawRoundedRect( x_pos - 1 - 2 + xoffset, nStartY - 2,
										   nLineWidth + 4, nHeight + 4,
										   4, 4 );
					}
				}
				xoffset++;
			}
		}
	}

	
	p.setPen( borderColor );
	p.setRenderHint( QPainter::Antialiasing );
	p.drawLine( 0, 0, m_nEditorWidth, 0 );
	p.setPen( QPen( borderColor, 2 ) );
	p.drawLine( 0, m_nEditorHeight, m_nEditorWidth, m_nEditorHeight );
	
	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		p.drawLine( m_nActiveWidth, 0, m_nEditorWidth, 0 );
		p.setPen( QPen( lineInactiveColor, 2 ) );
		p.drawLine( m_nActiveWidth, m_nEditorHeight,
					m_nEditorWidth, m_nEditorHeight );
	}
}

void NotePropertiesRuler::createNoteKeyBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	QColor backgroundColor = pPref->getColorTheme()->m_patternEditor_backgroundColor;
	const QColor backgroundInactiveColor( pPref->getColorTheme()->m_windowColor );
	QColor alternateRowColor = pPref->getColorTheme()->m_patternEditor_alternateRowColor;
	QColor octaveColor = pPref->getColorTheme()->m_patternEditor_octaveRowColor;
	QColor lineColor( pPref->getColorTheme()->m_patternEditor_lineColor );
	const QColor lineInactiveColor( pPref->getColorTheme()->m_windowTextColor.darker( 170 ) );
	QColor textColor( pPref->getColorTheme()->m_patternEditor_textColor );

	QPainter p( pixmap );
	p.fillRect( 0, 0, m_nEditorWidth, m_nEditorHeight, backgroundInactiveColor );
	drawDefaultBackground( p, NotePropertiesRuler::nNoteKeyOctaveHeight -
						   NotePropertiesRuler::nNoteKeySpaceHeight,
						   NotePropertiesRuler::nNoteKeyLineHeight );

	// fill the background of the key region;
	for ( unsigned y = NotePropertiesRuler::nNoteKeyOctaveHeight;
		  y < NotePropertiesRuler::nNoteKeyHeight;
		  y = y + NotePropertiesRuler::nNoteKeyLineHeight ) {

		const int nRow = ( y - NotePropertiesRuler::nNoteKeyOctaveHeight ) /
			NotePropertiesRuler::nNoteKeyLineHeight;
		if ( nRow == 1 ||  nRow == 3 || nRow == 5 || nRow == 8 || nRow == 10 ) {
			// Draw rows of semi tones in a different color.
			p.setPen( QPen( alternateRowColor,
							NotePropertiesRuler::nNoteKeyLineHeight - 1,
							Qt::SolidLine, Qt::FlatCap ) );
		}
		else {
			p.setPen( QPen( octaveColor,
							NotePropertiesRuler::nNoteKeyLineHeight - 1,
							Qt::SolidLine, Qt::FlatCap ) );
		}
					
		p.drawLine( PatternEditor::nMargin, y, m_nActiveWidth, y );
	}

	drawGridLines( p, Qt::DotLine );

	// Annotate with note class names
	static QString noteNames[] = { tr( "B" ), tr( "A#" ), tr( "A" ), tr( "G#" ), tr( "G" ), tr( "F#" ),
								   tr( "F" ), tr( "E" ), tr( "D#" ), tr( "D" ), tr( "C#" ), tr( "C" ) };
	
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	
	p.setFont( font );
	p.setPen( textColor );
	for ( int n = 0; n < KEYS_PER_OCTAVE; n++ ) {
		p.drawText( 3, NotePropertiesRuler::nNoteKeyOctaveHeight +
					NotePropertiesRuler::nNoteKeyLineHeight * n +3,
					noteNames[n] );
	}

	// Horizontal grid lines in the key region
	p.setPen( QPen( lineColor, 1, Qt::SolidLine));
	for ( unsigned y = NotePropertiesRuler::nNoteKeyOctaveHeight;
		  y <= NotePropertiesRuler::nNoteKeyHeight;
		  y = y + NotePropertiesRuler::nNoteKeyLineHeight ) {
		p.drawLine( PatternEditor::nMargin,
					y - NotePropertiesRuler::nNoteKeyLineHeight / 2,
					m_nActiveWidth,
					y - NotePropertiesRuler::nNoteKeyLineHeight / 2 );
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		for ( unsigned y = NotePropertiesRuler::nNoteKeyOctaveHeight;
			  y <= NotePropertiesRuler::nNoteKeyHeight;
			  y = y + NotePropertiesRuler::nNoteKeyLineHeight ) {
			p.drawLine( m_nActiveWidth,
						y - NotePropertiesRuler::nNoteKeyLineHeight / 2,
						m_nEditorWidth,
						y - NotePropertiesRuler::nNoteKeyLineHeight / 2 );
		}
	}

	if ( m_pPattern != nullptr ) {
		auto pSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrument();
		if ( pSelectedInstrument == nullptr ) {
			DEBUGLOG( "No instrument selected" );
			return;
		}
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_LENGTH(notes,it, m_pPattern) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSelectedInstrument
				 && !m_selection.isSelected( pNote ) ) {
				continue;
			}
			if ( !pNote->get_note_off() ) {
				// paint the octave
				const int nRadiusOctave = 3;
				const int nX = PatternEditor::nMargin +
					pNote->get_position() * m_fGridWidth;
				const int nOctaveY = ( 4 - pNote->get_octave() ) *
					NotePropertiesRuler::nNoteKeyLineHeight;
				p.setPen( QPen( Qt::black, 1 ) );
				p.setBrush( DrumPatternEditor::computeNoteColor(
								pNote->get_velocity() ) );
				p.drawEllipse( QPoint( nX, nOctaveY ), nRadiusOctave,
							   nRadiusOctave );

				// paint note
				const int nRadiusKey = 5;
				const int nKeyY = NotePropertiesRuler::nNoteKeyHeight -
					( ( pNote->get_key() + 1 ) *
					  NotePropertiesRuler::nNoteKeyLineHeight );

				p.setBrush( DrumPatternEditor::computeNoteColor(
								pNote->get_velocity() ) );
				p.drawEllipse( QPoint( nX, nKeyY ), nRadiusKey, nRadiusKey);

				// Paint selection outlines
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					// Octave
					p.drawEllipse( QPoint( nX, nOctaveY ), nRadiusOctave + 1,
								   nRadiusOctave + 1 );

					// Key
					p.drawEllipse( QPoint( nX, nKeyY ), nRadiusKey + 1,
								   nRadiusKey + 1 );
				}
			}
		}
	}
	
	p.setPen( lineColor );
	p.setRenderHint( QPainter::Antialiasing );
	p.drawLine( 0, 0, m_nEditorWidth, 0 );
	p.setPen( QPen( lineColor, 2 ) );
	p.drawLine( 0, m_nEditorHeight, m_nEditorWidth, m_nEditorHeight );
	
	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		p.drawLine( m_nActiveWidth, 0, m_nEditorWidth, 0 );
		p.setPen( QPen( lineInactiveColor, 2 ) );
		p.drawLine( m_nActiveWidth, m_nEditorHeight,
					m_nEditorWidth, m_nEditorHeight );
	}
}


void NotePropertiesRuler::updateEditor( bool )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}
	m_nSelectedPatternNumber = nSelectedPatternNumber;

	updateWidth();
	resize( m_nEditorWidth, height() );

	invalidateBackground();
	update();
}

void NotePropertiesRuler::createBackground()
{
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio ,
										   m_nEditorHeight * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	if ( m_mode == PatternEditor::Mode::Velocity ||
		 m_mode == PatternEditor::Mode::Probability ) {
		createNormalizedBackground( m_pBackgroundPixmap );
	}
	else if ( m_mode == PatternEditor::Mode::Pan ||
			  m_mode == PatternEditor::Mode::LeadLag ) {
		createCenteredBackground( m_pBackgroundPixmap );
	}
	else if ( m_mode == PatternEditor::Mode::NoteKey ) {
		createNoteKeyBackground( m_pBackgroundPixmap );
	}
	
	m_bBackgroundInvalid = false;
}


void NotePropertiesRuler::selectedPatternChangedEvent()
{
	updateEditor();
}

void NotePropertiesRuler::selectedInstrumentChangedEvent()
{
	updateEditor();
}

void NotePropertiesRuler::songModeActivationEvent() {
	updateEditor();
}

std::vector<NotePropertiesRuler::SelectionIndex> NotePropertiesRuler::elementsIntersecting( QRect r ) {
	std::vector<SelectionIndex> result;
	if ( m_pPattern == nullptr ) {
		return std::move( result );
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	
	const Pattern::notes_t* notes = m_pPattern->get_notes();
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return std::move( result );
	}

	// Account for the notional active area of the slider. We allow a
	// width of 8 as this is the size of the circle used for the zero
	// position on the lead/lag editor.
	r = r.normalized();
	if ( r.top() == r.bottom() && r.left() == r.right() ) {
		r += QMargins( 2, 2, 2, 2 );
	}
	r += QMargins( 4, 4, 4, 4 );

	FOREACH_NOTE_CST_IT_BEGIN_LENGTH(notes,it, m_pPattern) {
		if ( it->second->get_instrument() != pSelectedInstrument
			 && !m_selection.isSelected( it->second ) ) {
			continue;
		}

		int pos = it->first;
		uint x_pos = PatternEditor::nMargin + pos * m_fGridWidth;
		if ( r.intersects( QRect( x_pos, 0, 1, height() ) ) ) {
			result.push_back( it->second );
		}
	}

	// Updating selection, we may need to repaint the whole widget.
	invalidateBackground();
	update();

	return std::move(result);
}

///
/// The screen area occupied by the keyboard cursor
///
QRect NotePropertiesRuler::getKeyboardCursorRect()
{
	uint x = PatternEditor::nMargin +
		m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;
	return QRect( x-m_fGridWidth*3, 3, m_fGridWidth*6, height()-6 );
}

void NotePropertiesRuler::selectAll()
{
	selectInstrumentNotes( Hydrogen::get_instance()->getSelectedInstrumentNumber() );
}

void NotePropertiesRuler::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {

		invalidateBackground();
		update();
	}
}
