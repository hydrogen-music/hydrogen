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

#include <core/Hydrogen.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
using namespace H2Core;

#include <cassert>

#include "../HydrogenApp.h"

#include "UndoActions.h"
#include "NotePropertiesRuler.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "DrumPatternEditor.h"
#include "PianoRollEditor.h"
#include "../Skin.h"

int NotePropertiesRuler::nKeyOctaveHeight =
	NotePropertiesRuler::nOctaveHeight +
	NotePropertiesRuler::nKeyLineHeight * KEYS_PER_OCTAVE;


NotePropertiesRuler::NotePropertiesRuler( QWidget *parent,
										  PatternEditor::Mode mode, Layout layout )
	: PatternEditor( parent )
	, m_bEntered( false )
	, m_layout( layout )
{

	m_editor = PatternEditor::Editor::NotePropertiesRuler;
	m_mode = mode;

	m_fGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );

	m_fLastSetValue = 0.0;
	m_bValueHasBeenSet = false;

	if ( m_mode == PatternEditor::Mode::KeyOctave ) {
		m_nEditorHeight = NotePropertiesRuler::nKeyOctaveHeight;
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

	// Generic pattern editor menu contains some operations that don't apply
	// here, and we will want to add menu options specific to this later.
	delete m_pPopupMenu;
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, SLOT( selectAll() ) );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, SLOT( selectNone() ) );
}




NotePropertiesRuler::~NotePropertiesRuler()
{
}


//! Scroll wheel gestures will adjust the property of notes under the mouse
//! cursor (or selected notes, if any). Unlike drag gestures, each individual
//! wheel movement will result in an undo/redo action since the events are
//! discrete.
void NotePropertiesRuler::wheelEvent(QWheelEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	QPoint point;
#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	point = ev->position().toPoint();
#else
	point = QPoint( ev->x(), 0 );
#endif

	bool bUpdate = false;

	// When interacting with note(s) not already in a selection, we will discard
	// the current selection and add these notes under point to a transient one.
	const auto notesUnderPoint = getNotesAtPoint( pPattern, point, true );
	if ( notesUnderPoint.size() > 0 ) {
		m_selection.clearSelection();
		for ( const auto& ppNote : notesUnderPoint ) {
			m_selection.addToSelection( ppNote );
		}
		bUpdate = true;
	}

	prepareUndoAction( point ); //get all old values

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

	bool bValueChanged = false;
	for ( auto& ppNote : m_selection ) {
		bValueChanged =
			adjustNotePropertyDelta( ppNote, fDelta, /* bMessage=*/ true ) ||
			bValueChanged;
	}

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	handleKeyboardCursor( false );

	if ( bUpdate ) {
		if ( bValueChanged ) {
			addUndoAction();
		}

		if ( notesUnderPoint.size() > 0 ) {
			m_selection.clearSelection();
		}

		invalidateBackground();
		m_pPatternEditorPanel->getVisibleEditor()->updateEditor();
		update();
	}
}


void NotePropertiesRuler::mouseClickEvent( QMouseEvent *ev ) {
	if ( m_pPatternEditorPanel->getPattern() == nullptr ) {
		return;
	}

	if ( ev->button() == Qt::LeftButton ) {
		// Treat single click as an instantaneous drag
		propertyDragStart( ev );
		propertyDragUpdate( ev );
		propertyDragEnd();
	}

	PatternEditor::mouseClickEvent( ev );
}

void NotePropertiesRuler::mouseDragStartEvent( QMouseEvent *ev ) {
	if ( m_selection.isMoving() ) {
		prepareUndoAction( ev->pos() );
		selectionMoveUpdateEvent( ev );
	}
	else {
		if ( ev->buttons() == Qt::RightButton ) {
			propertyDragStart( ev );
			propertyDragUpdate( ev );
		}
	}
}

void NotePropertiesRuler::mouseDragUpdateEvent( QMouseEvent *ev ) {
	if ( ev->buttons() == Qt::RightButton ) {
		propertyDragUpdate( ev );
	}
}

void NotePropertiesRuler::mouseDragEndEvent( QMouseEvent *ev ) {
	if ( ev->buttons() == Qt::RightButton ) {
		propertyDragEnd();
	}
}


void NotePropertiesRuler::selectionMoveUpdateEvent( QMouseEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row clicked" );
		return;
	}

	float fDelta;

	QPoint movingOffset = m_selection.movingOffset();
	if ( m_mode == PatternEditor::Mode::KeyOctave ) {
		fDelta = (float)-movingOffset.y() /
			static_cast<float>(NotePropertiesRuler::nKeyLineHeight);
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
		if ( ( pNote->get_instrument_id() == selectedRow.nInstrumentID &&
			   pNote->getType() == selectedRow.sType ) ||
			 m_selection.isSelected( pNote ) ) {

			// Record original note if not already recorded
			if ( m_oldNotes.find( pNote ) == m_oldNotes.end() ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}

			bValueChanged = adjustNotePropertyDelta(
				pNote, fDelta, bSendStatusMsg );
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
		case PatternEditor::Mode::KeyOctave:
			pNote->set_key_octave( pOldNote->get_key(), pOldNote->get_octave() );
			break;
		case PatternEditor::Mode::Probability:
			pNote->set_probability( pOldNote->get_probability() );
			break;
		case PatternEditor::Mode::None:
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

void NotePropertiesRuler::propertyDragStart( QMouseEvent *ev )
{
	setCursor( Qt::CrossCursor );
	prepareUndoAction( ev->pos() );
	invalidateBackground();
	update();
}


//! Preserve current note properties at position x (or in selection, if any) for
//! use in later UndoAction.
void NotePropertiesRuler::prepareUndoAction( const QPoint& point )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row clicked" );
		return;
	}

	clearOldNotes();

	int nColumn, nRow, nRealColumn;
	eventPointToColumnRow( point, &nColumn, &nRow, &nRealColumn );

	if ( m_selection.begin() != m_selection.end() ) {
		// If there is a selection, preserve the initial state of all the
		// selected notes.
		for ( auto& ppNote : m_selection ) {
			if ( ppNote != nullptr ) {
				m_oldNotes[ ppNote ] = new Note( ppNote );
			}
		}

	} else {
		// No notes are selected. The target notes to adjust are all those at
		// column given by 'x', so we preserve these.
		FOREACH_NOTE_CST_IT_BOUND_LENGTH( pPattern->getNotes(), it,
										  nRealColumn, pPattern ) {
			Note *pNote = it->second;
			if ( pNote->get_instrument_id() == selectedRow.nInstrumentID ||
				 pNote->getType() == selectedRow.sType ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}
		}
	}

	m_nDragPreviousColumn = nRealColumn;
}

//! Update notes for a property adjust drag, in response to the mouse moving.
//! This modifies the values of the notes as the mouse moves, but does not
//! complete an undo action until the notes final value has been set. This
//! occurs either when the mouse is released, or when the pointer moves off of
//! the note's column.
void NotePropertiesRuler::propertyDragUpdate( QMouseEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row clicked" );
		return;
	}

	int nColumn, nRow, nRealColumn;
	eventPointToColumnRow( ev->pos(), &nColumn, &nRow, &nRealColumn );

	if ( m_nDragPreviousColumn != nColumn ) {
		// Complete current undo action, and start a new one.
		addUndoAction();
		prepareUndoAction( ev->pos() );
	}

	float val = height() - ev->y();
	if ( val > height() ) {
		val = height();
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	val = val / height(); // val is normalized, in [0;1]

	bool bValueSet = false;

	FOREACH_NOTE_CST_IT_BOUND_LENGTH( pPattern->getNotes(), it, nRealColumn, pPattern ) {
		Note *pNote = it->second;

		if ( ! ( pNote->get_instrument_id() == selectedRow.nInstrumentID &&
				 pNote->getType() == selectedRow.sType ) &&
			 ! m_selection.isSelected( pNote ) ) {
			continue;
		}
		if ( m_mode == PatternEditor::Mode::Velocity && !pNote->get_note_off() ) {
			pNote->set_velocity( val );
			m_fLastSetValue = val;
			bValueSet = true;
		}
		else if ( m_mode == PatternEditor::Mode::Pan && !pNote->get_note_off() ){
			if ( (ev->button() == Qt::MiddleButton)
					|| (ev->modifiers() == Qt::ControlModifier &&
						ev->button() == Qt::LeftButton) ) {
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
		else if ( m_mode == PatternEditor::Mode::KeyOctave &&
				  ! pNote->get_note_off() ) {
			if ( ev->button() != Qt::MiddleButton &&
				 ! ( ev->modifiers() == Qt::ControlModifier &&
					 ev->button() == Qt::LeftButton ) ) {
				int nKey = 666;
				int nOctave = 666;
				if ( ev->y() > 0 &&
					 ev->y() <= NotePropertiesRuler::nOctaveHeight ) {
					nOctave = std::round(
						( NotePropertiesRuler::nOctaveHeight / 2 +
						  NotePropertiesRuler::nKeyLineHeight / 2 -
						  ev->y() -
						  NotePropertiesRuler::nKeyLineHeight / 2 ) /
						NotePropertiesRuler::nKeyLineHeight );
					nOctave = std::clamp( nOctave, OCTAVE_MIN, OCTAVE_MAX );
				}
				else if ( ev->y() >= NotePropertiesRuler::nOctaveHeight &&
						  ev->y() < NotePropertiesRuler::nKeyOctaveHeight ) {
					nKey = ( height() - ev->y() -
							 NotePropertiesRuler::nKeyLineHeight / 2 ) /
						NotePropertiesRuler::nKeyLineHeight;
					nKey = std::clamp( nKey, KEY_MIN, KEY_MAX );
				}

				if ( nKey != 666 || nOctave != 666 ) {
					m_fLastSetValue = nOctave * KEYS_PER_OCTAVE + nKey;
					bValueSet = true;
					pNote->set_key_octave((Note::Key)nKey,(Note::Octave)nOctave); // won't set wrong values see Note::set_key_octave
				}
			}
		}
		else if ( m_mode == PatternEditor::Mode::Probability ) {
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

	m_nDragPreviousColumn = nColumn;
	invalidateBackground();
	update();

	if ( m_mode == PatternEditor::Mode::Velocity ) {
		// A note's velocity determines its color in the other pattern editors
		// as well.
		m_pPatternEditorPanel->getPianoRollEditor()->updateEditor();
		m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
	}
}

void NotePropertiesRuler::propertyDragEnd()
{
	addUndoAction();
	unsetCursor();
	invalidateBackground();
	update();
}

//! Adjust a note's property by applying a delta to the current value, and
//! clipping to the appropriate range. Optionally, show a message with the value
//! for some properties.
bool NotePropertiesRuler::adjustNotePropertyDelta( Note *pNote,
												   float fDelta,
												   bool bMessage )
{
	if ( pNote == nullptr ) {
		ERRORLOG( "invaild note" );
		return false;
	}

	Note *pOldNote = m_oldNotes[ pNote ];
	if ( pOldNote == nullptr ) {
		ERRORLOG( QString( "Could not find note corresponding to [%1]" )
				  .arg( pNote->toQString() ) );
		return false;
	}

	bool bValueChanged = false;
	
	switch( m_mode ) {
	case PatternEditor::Mode::Velocity: {
		if ( ! pNote->get_note_off() ) {
			const float fVelocity = qBound(
				VELOCITY_MIN, (pOldNote->get_velocity() + fDelta), VELOCITY_MAX );

			if ( fVelocity != pNote->get_velocity() ) {
				pNote->set_velocity( fVelocity );
				m_fLastSetValue = fVelocity;
				bValueChanged = true;
			}
		}
		break;
	}
	case PatternEditor::Mode::Pan: {
		if ( ! pNote->get_note_off() ) {
			// value in [0,1] or slight out of boundaries
			const float fVal = pOldNote->getPanWithRangeFrom0To1() + fDelta;
			if ( fVal != pNote->getPanWithRangeFrom0To1() ) {
				// Does check boundaries internally.
				pNote->setPanWithRangeFrom0To1( fVal );
				m_fLastSetValue = pNote->getPanWithRangeFrom0To1();
				bValueChanged = true;
			}
		}
		break;
	}
	case PatternEditor::Mode::LeadLag: {
		const float fLeadLag = qBound(
			LEAD_LAG_MIN, pOldNote->get_lead_lag() - fDelta, LEAD_LAG_MAX );
		if ( fLeadLag != pNote->get_lead_lag() ) {
			pNote->set_lead_lag( fLeadLag );
			m_fLastSetValue = fLeadLag;
			bValueChanged = true;
		}
		break;
	}
	case PatternEditor::Mode::Probability: {
		if ( ! pNote->get_note_off() ) {
			const float fProbability = qBound(
				PROBABILITY_MIN, pOldNote->get_probability() + fDelta,
				PROBABILITY_MAX );
			if ( fProbability != pNote->get_probability() ) {
				pNote->set_probability( fProbability );
				m_fLastSetValue = fProbability;
				bValueChanged = true;
			}
		}
		break;
	}
	case PatternEditor::Mode::KeyOctave: {
		const int nPitch = qBound(
			KEYS_PER_OCTAVE * OCTAVE_MIN,
			static_cast<int>(pOldNote->get_pitch_from_key_octave() + fDelta ),
			KEYS_PER_OCTAVE * OCTAVE_MAX + KEY_MAX );
		Note::Octave octave;
		if ( nPitch >= 0 ) {
			octave = static_cast<Note::Octave>( nPitch / KEYS_PER_OCTAVE );
		} else {
			octave = static_cast<Note::Octave>( (nPitch-11) / KEYS_PER_OCTAVE );
		}
		Note::Key key = static_cast<Note::Key>(
			nPitch - KEYS_PER_OCTAVE * static_cast<int>(octave) );

		if ( key != pNote->get_key() || octave != pNote->get_octave() ) {
			pNote->set_key_octave( key, octave );
			m_fLastSetValue = KEYS_PER_OCTAVE * octave + key;
			bValueChanged = true;
		}
		break;
	}
	case PatternEditor::Mode::None:
	default:
		ERRORLOG("No mode set. No note property adjusted.");
	}

	if ( bValueChanged ) {
		Hydrogen::get_instance()->setIsModified( true );
		m_bValueHasBeenSet = true;
		if ( bMessage ) {
			PatternEditor::triggerStatusMessage( pNote, m_mode );
		}
	}

	return bValueChanged;
}

void NotePropertiesRuler::keyPressEvent( QKeyEvent *ev )
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
	
	const bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bUnhideCursor = true;

	bool bValueChanged = false;
	// Value adjustments
	float fDelta = 0.0;
	bool bRepeatLastValue = false;


	if ( bIsSelectionKey ) {
		// Key was claimed by selection
	}
	else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete / Backspace: delete selected notes, or note under keyboard cursor
		bUnhideCursor = false;
		if ( m_selection.begin() != m_selection.end() ) {
			// Delete selected notes if any
			m_pPatternEditorPanel->getDrumPatternEditor()->
				deleteSelection();
		} else {
			// Delete note under the keyboard cursor.
			addOrRemoveNote( m_pPatternEditorPanel->getCursorColumn(), -1,
							 m_pPatternEditorPanel->getSelectedRowDB(),
							 KEY_MIN, OCTAVE_DEFAULT,
							 /*bDoAdd=*/false, /*bDoDelete=*/true );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousLine ) ) {
		// Key: Up: increase note parameter value
		fDelta = 0.1;
	}
	else if ( ev->key() == Qt::Key_Up && ev->modifiers() & Qt::AltModifier ) {
		// Key: Alt+Up: increase parameter slightly
		fDelta = 0.01;
	}
	else if ( ev->matches( QKeySequence::MoveToNextLine ) ) {
		// Key: Down: decrease note parameter value
		fDelta = -0.1;
	}
	else if ( ev->key() == Qt::Key_Down && ev->modifiers() & Qt::AltModifier ) {
		// Key: Alt+Up: decrease parameter slightly
		fDelta = -0.01;
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ) {
		// Key: MoveToStartOfDocument: increase parameter to maximum value
		fDelta = 1.0;
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ) {
		// Key: MoveEndOfDocument: decrease parameter to minimum value
		fDelta = -1.0;
	}
	else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter/Return: repeat last parameter value set.
		if ( m_bValueHasBeenSet ) {
			bRepeatLastValue = true;
		}
	}
	else {
		PatternEditor::keyPressEvent( ev );
		return;
	}

	// Value change
	if ( fDelta != 0.0 || bRepeatLastValue ) {
		const int nColumn = m_pPatternEditorPanel->getCursorColumn();

		// Collect notes to apply the change to
		std::list<Note*> notes;
		if ( m_selection.begin() != m_selection.end() ) {
			for ( Note *pNote : m_selection ) {
				if ( pNote != nullptr ) {
					notes.push_back( pNote );
				}
			}
		}
		else {
			FOREACH_NOTE_CST_IT_BOUND_LENGTH( pPattern->getNotes(), it, nColumn, pPattern ) {
				Note *pNote = it->second;
				if ( pNote != nullptr && pNote->get_position() == nColumn &&
					 pNote->get_instrument_id() == selectedRow.nInstrumentID &&
					 pNote->getType() == selectedRow.sType ) {
					notes.push_back( pNote );
				}
			}
		}

		// For the KeyOctave Editor, adjust the pitch by a whole semitone
		if ( m_mode == PatternEditor::Mode::KeyOctave ) {
			if ( fDelta > 0.0 ) {
				fDelta = 1;
			} else if ( fDelta < 0.0 ) {
				fDelta = -1;
			}
		}

		prepareUndoAction(
			QPoint( PatternEditor::nMargin + nColumn * m_fGridWidth, 0 ) );

		for ( Note *pNote : notes ) {

			if ( ! bRepeatLastValue ) {
				// Apply delta to the property
				bValueChanged = adjustNotePropertyDelta(
					pNote, fDelta, notes.size() == 1 );
			}
			else {
				// Repeating last value
				switch ( m_mode ) {
				case PatternEditor::Mode::Velocity:
					if ( ! pNote->get_note_off() ) {
						pNote->set_velocity( m_fLastSetValue );
						bValueChanged = true;
					}
					break;
				case PatternEditor::Mode::Pan:
					if ( ! pNote->get_note_off() ) {
						pNote->setPanWithRangeFrom0To1( m_fLastSetValue );
						bValueChanged = true;
					}
					break;
				case PatternEditor::Mode::LeadLag:
					pNote->set_lead_lag( m_fLastSetValue );
					bValueChanged = true;
					break;
				case PatternEditor::Mode::Probability:
					if ( ! pNote->get_note_off() ) {
						pNote->set_probability( m_fLastSetValue );
						bValueChanged = true;
					}
					break;
				case PatternEditor::Mode::KeyOctave:
					pNote->set_key_octave( (Note::Key)( (int)m_fLastSetValue % 12 ),
										   (Note::Octave)( (int)m_fLastSetValue / 12 ) );
					bValueChanged = true;
						break;

				case PatternEditor::Mode::None:
				default:
					ERRORLOG("No mode set. No note property adjusted.");
				}

				if ( bValueChanged ) {
					if ( notes.size() == 1 ) {
						PatternEditor::triggerStatusMessage( pNote, m_mode );
					}
					Hydrogen::get_instance()->setIsModified( true );
				}
			}
		}
		addUndoAction();
	}

	if ( bValueChanged ) {
		invalidateBackground();
		m_pPatternEditorPanel->getVisibleEditor()->updateEditor();
	}

	if ( bUnhideCursor ) {
		handleKeyboardCursor( bUnhideCursor );
	}

	update();
	ev->accept();
}

void NotePropertiesRuler::addUndoAction()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	int nSize = m_oldNotes.size();
	if ( nSize != 0 ) {
		QUndoStack *pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;

		if ( nSize != 1 ) {
			pUndoStack->beginMacro( QString( tr( "Edit [%1] property of [%2] notes" ) )
									.arg( PatternEditor::modeToQString( m_mode ) )
									.arg( nSize ) );
		}
		for ( auto it : m_oldNotes ) {
			Note *pNewNote = it.first, *pOldNote = it.second;

			const int nNewKey = pNewNote->get_key();
			const int nNewOctave = pNewNote->get_octave();
			if ( pNewNote->get_key() != pOldNote->get_key() ||
				 pNewNote->get_octave() != pOldNote->get_octave() ) {
				// Note pitch was altered during the editing (drag update). We
				// have to temporarily reset the note key/octave (without
				// redrawing!) in order to allow for the redo part of the action
				// below to find the corresponding note.
				//
				// For all other note property edits this is not critical as the
				// note will be found and one the edit will be skip since the
				// note already holds the proper value.
				pNewNote->set_key_octave( pOldNote->get_key(),
										  pOldNote->get_octave() );
			}

			pUndoStack->push( new SE_editNotePropertiesAction(
								  m_mode,
								  m_pPatternEditorPanel->getPatternNumber(),
								  pNewNote->get_position(),
								  pOldNote->get_instrument_id(),
								  pOldNote->getType(),
								  pNewNote->get_velocity(),
								  pOldNote->get_velocity(),
								  pNewNote->getPan(),
								  pOldNote->getPan(),
								  pNewNote->get_lead_lag(),
								  pOldNote->get_lead_lag(),
								  pNewNote->get_probability(),
								  pOldNote->get_probability(),
								  pNewNote->get_length(),
								  pOldNote->get_length(),
								  nNewKey,
								  pOldNote->get_key(),
								  nNewOctave,
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

	const auto pPref = Preferences::get_instance();
	
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

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// Draw hovered notes
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		const auto baseStyle = static_cast<NoteStyle>(
			( ppPattern == pPattern ? NoteStyle::Foreground :
			  NoteStyle::Background ) | NoteStyle::Hovered);
		int nOffsetX = 0;
		for ( const auto& ppNote : nnotes ) {
			const auto style = static_cast<NoteStyle>(
				m_selection.isSelected( ppNote ) ?
				NoteStyle::Selected | baseStyle : baseStyle );
			drawNote( painter, ppNote, style, nOffsetX );

			if ( m_layout != Layout::KeyOctave ) {
				// Within the key/octave view notes should be unique.
				++nOffsetX;
			}
		}
	}


	m_selection.paintSelection( &painter );

	// cursor
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		QPen pen( pPref->getTheme().m_color.m_cursorColor );
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.setBrush( Qt::NoBrush );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( getKeyboardCursorRect(), 4, 4 );
	}
}

void NotePropertiesRuler::drawFocus( QPainter& painter ) {

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

	const QScrollArea* pScrollArea;
	
	switch ( m_mode ) {
	case PatternEditor::Mode::Velocity:
		pScrollArea = m_pPatternEditorPanel->getNoteVelocityScrollArea();
		break;
	case PatternEditor::Mode::Pan:
		pScrollArea = m_pPatternEditorPanel->getNotePanScrollArea();
		break;
	case PatternEditor::Mode::LeadLag:
		pScrollArea = m_pPatternEditorPanel->getNoteLeadLagScrollArea();
		break;
	case PatternEditor::Mode::KeyOctave:
		pScrollArea = m_pPatternEditorPanel->getNoteKeyOctaveScrollArea();
		break;
	case PatternEditor::Mode::Probability:
		pScrollArea = m_pPatternEditorPanel->getNoteProbabilityScrollArea();
		break;
	case PatternEditor::Mode::None:
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

void NotePropertiesRuler::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void NotePropertiesRuler::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void NotePropertiesRuler::drawDefaultBackground( QPainter& painter, int nHeight,
												 int nIncrement ) {
	
	const auto pPref = H2Core::Preferences::get_instance();

	const QColor borderColor(
		pPref->getTheme().m_color.m_patternEditor_lineColor );
	const QColor lineColor(
		pPref->getTheme().m_color.m_patternEditor_line5Color );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );
	const QColor backgroundColor(
		pPref->getTheme().m_color.m_patternEditor_backgroundColor );
	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );

	if ( nHeight == 0 ) {
		nHeight = height();
	}
	if ( nIncrement == 0 ) {
		nIncrement = nHeight / 10;
	}

	painter.fillRect( 0, 0, m_nActiveWidth, height(), backgroundColor );
	painter.fillRect( m_nActiveWidth, 0, m_nEditorWidth - m_nActiveWidth,
					  height(), backgroundInactiveColor );

	if ( m_pPatternEditorPanel->getPattern() == nullptr ) {
		return;
	}

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

void NotePropertiesRuler::drawNote( QPainter& p, H2Core::Note* pNote,
									NoteStyle noteStyle, int nOffsetX )
{
	if ( pNote == nullptr ) {
		return;
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// NoteOff notes can have a custom probability and lead lag. But having a
	// velocity and pan would not make any sense for them.
	if ( pNote->get_note_off() &&
		 ! ( m_mode == PatternEditor::Mode::Probability ||
			 m_mode == PatternEditor::Mode::LeadLag ) ) {
		return;
	}

	const auto pPref = H2Core::Preferences::get_instance();

	QPen highlightPen( highlightedNoteColor( noteStyle ) );
	highlightPen.setWidth( 2 );
	const int nLineWidth = 3;

	QColor color;
	if ( ! pNote->get_note_off() ) {
		color = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
	} else {
		color = pPref->getTheme().m_color.m_patternEditor_noteOffColor;
	}
	const QColor noteColor(
		pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor );
	const QColor noteInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 150 ) );
	const QColor noteoffInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor );

	const int nX = nOffsetX + PatternEditor::nMargin +
		pNote->get_position() * m_fGridWidth;

	QBrush noteBrush( color );
	QPen notePen( noteColor );
	if ( noteStyle & NoteStyle::Background ) {

		if ( nX >= m_nActiveWidth ) {
			notePen.setColor( noteInactiveColor );
		}

		noteBrush.setStyle( Qt::Dense4Pattern );
		notePen.setStyle( Qt::DotLine );
	}
	p.setPen( notePen );
	p.setRenderHint( QPainter::Antialiasing );

	if ( m_layout == Layout::Centered || m_layout == Layout::Normalized ) {
		float fValue = 0;
		if ( m_mode == PatternEditor::Mode::Velocity ) {
			fValue = std::round( pNote->get_velocity() * height() );
		}
		else if ( m_mode == PatternEditor::Mode::Probability ) {
			fValue = std::round( pNote->get_probability() * height() );
		}
		else if ( m_mode == PatternEditor::Mode::Pan ) {
			// Rounding in order to not miss the center due to rounding errors
			// introduced in the Note class internals.
			fValue = std::round( pNote->getPan() * 100 ) / 100;
		}
		else if ( m_mode == PatternEditor::Mode::LeadLag ) {
			fValue = -1 * std::round( pNote->get_lead_lag() * 100 ) / 100;
		}


		if ( m_layout == Layout::Centered && fValue == 0 ) {
			// value is centered - draw circle
			const int nY = static_cast<int>(std::round( height() * 0.5 ) );
			p.setBrush( noteBrush );
			p.drawEllipse( nX - 4, nY - 4, 8, 8);
			p.setBrush( Qt::NoBrush );

			if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
				p.setPen( highlightPen );
				p.setRenderHint( QPainter::Antialiasing );
				p.drawEllipse( nX - 6, nY - 6, 12, 12);
			}
		}
		else {
			int nY, nHeight;
			if ( m_layout == Layout::Centered ) {
				nHeight = 0.5 * height() * std::abs( fValue ) + 5;
				nY = height() * 0.5 - 2;
				if ( fValue >= 0 ) {
					nY = nY - nHeight + 5;
				}
			}
			else {
				nY = height() - fValue;
				nHeight = fValue;
			}

			p.fillRect( nX - 1, nY, nLineWidth, nHeight, noteBrush );
			p.drawRoundedRect( nX - 1 - 1, nY - 1, nLineWidth + 2, nHeight + 2,
							   2, 2 );

			if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
				p.setPen( highlightPen );
				p.drawRoundedRect( nX - 1 - 2, nY - 2, nLineWidth + 4,
								   nHeight + 4, 4, 4 );
			}
		}
	}
	else {
		// KeyOctave layout

		// paint the octave
		const int nRadiusOctave = 3;
		const int nOctaveY = ( 4 - pNote->get_octave() ) *
			NotePropertiesRuler::nKeyLineHeight;
		p.setBrush( noteBrush );
		p.drawEllipse( QPoint( nX, nOctaveY ), nRadiusOctave, nRadiusOctave );

		// paint note
		const int nRadiusKey = 5;
		const int nKeyY = NotePropertiesRuler::nKeyOctaveHeight -
			( ( pNote->get_key() + 1 ) * NotePropertiesRuler::nKeyLineHeight );
		p.drawEllipse( QPoint( nX, nKeyY ), nRadiusKey, nRadiusKey);
		p.setBrush( Qt::NoBrush );

		// Paint selection outlines
		if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
			p.setPen( highlightPen );
			// Octave
			p.drawEllipse( QPoint( nX, nOctaveY ), nRadiusOctave + 1,
						   nRadiusOctave + 1 );

			// Key
			p.drawEllipse( QPoint( nX, nKeyY ), nRadiusKey + 1,
						   nRadiusKey + 1 );
		}
	}
}

void NotePropertiesRuler::updateEditor( bool )
{
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

	m_bBackgroundInvalid = false;

	const auto pPref = H2Core::Preferences::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();

	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );
	const QColor lineColor(
		pPref->getTheme().m_color.m_patternEditor_lineColor );
	QColor textColor( pPref->getTheme().m_color.m_patternEditor_textColor );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );
	const QColor alternateRowColor =
		pPref->getTheme().m_color.m_patternEditor_alternateRowColor;
	const QColor octaveColor =
		pPref->getTheme().m_color.m_patternEditor_octaveRowColor;

	QPainter p( m_pBackgroundPixmap );

	if ( m_layout == Layout::KeyOctave ) {
		p.fillRect( 0, 0, m_nEditorWidth, m_nEditorHeight,
					backgroundInactiveColor );
		drawDefaultBackground( p, NotePropertiesRuler::nOctaveHeight -
							   NotePropertiesRuler::nKeyOctaveSpaceHeight,
							   NotePropertiesRuler::nKeyLineHeight );
	}
	else {
		drawDefaultBackground( p );
	}

	// draw layout specific background design
	if ( m_layout == Layout::Centered ) {
		// central line
		p.setPen( lineColor );
		p.drawLine( 0, height() / 2.0, m_nActiveWidth, height() / 2.0 );
		if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
			p.setPen( lineInactiveColor );
			p.drawLine( m_nActiveWidth, height() / 2.0,
						m_nEditorWidth, height() / 2.0 );
		}
	}
	else if ( m_layout == Layout::KeyOctave ) {
		// key / octave background
		for ( int yy = NotePropertiesRuler::nOctaveHeight;
			  yy < NotePropertiesRuler::nKeyOctaveHeight;
			  yy += NotePropertiesRuler::nKeyLineHeight ) {

			const int nRow = ( yy - NotePropertiesRuler::nOctaveHeight ) /
				NotePropertiesRuler::nKeyLineHeight;
			if ( nRow == 1 ||  nRow == 3 || nRow == 5 || nRow == 8 ||
				 nRow == 10 ) {
				// Draw rows of semi tones in a different color.
				p.setPen( QPen( alternateRowColor,
								NotePropertiesRuler::nKeyLineHeight - 1,
								Qt::SolidLine, Qt::FlatCap ) );
			}
			else {
				p.setPen( QPen( octaveColor,
								NotePropertiesRuler::nKeyLineHeight - 1,
								Qt::SolidLine, Qt::FlatCap ) );
			}

			p.drawLine( PatternEditor::nMargin, yy, m_nActiveWidth, yy );
		}

		if ( pPattern != nullptr ) {
			drawGridLines( p, Qt::DotLine );

			// Annotate with note class names
			static QStringList noteNames = QStringList()
				<< tr( "B" )
				<< tr( "A#" )
				<< tr( "A" )
				<< tr( "G#" )
				<< tr( "G" )
				<< tr( "F#" )
				<< tr( "F" )
				<< tr( "E" )
				<< tr( "D#" )
				<< tr( "D" )
				<< tr( "C#" )
				<< tr( "C" );

			QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
						getPointSize( pPref->getTheme().m_font.m_fontSize ) );

			p.setFont( font );
			p.setPen( textColor );
			for ( int n = 0; n < KEYS_PER_OCTAVE; n++ ) {
				p.drawText( 3, NotePropertiesRuler::nOctaveHeight +
							NotePropertiesRuler::nKeyLineHeight * n +3,
							noteNames[n] );
			}

			// Horizontal grid lines in the key region
			p.setPen( QPen( lineColor, 1, Qt::SolidLine));
			for ( int yy = NotePropertiesRuler::nOctaveHeight;
				  yy <= NotePropertiesRuler::nKeyOctaveHeight;
				  yy += NotePropertiesRuler::nKeyLineHeight ) {
				p.drawLine( PatternEditor::nMargin,
							yy - NotePropertiesRuler::nKeyLineHeight / 2,
							m_nActiveWidth,
							yy - NotePropertiesRuler::nKeyLineHeight / 2 );
			}

			if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
				p.setPen( lineInactiveColor );
				for ( int yy = NotePropertiesRuler::nOctaveHeight;
					  yy <= NotePropertiesRuler::nKeyOctaveHeight;
					  yy = yy + NotePropertiesRuler::nKeyLineHeight ) {
					p.drawLine( m_nActiveWidth,
								yy - NotePropertiesRuler::nKeyLineHeight / 2,
								m_nEditorWidth,
								yy - NotePropertiesRuler::nKeyLineHeight / 2 );
				}
			}
		}
	}

	// draw pattern
	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		const auto baseStyle = ppPattern == pPattern ?
			NoteStyle::Foreground : NoteStyle::Background;

		// Since properties of notes within the same row would end up being
		// painted on top of eachother, we go through the notes column by column
		// and add small horizontal offsets to each additional note to hint
		// their existence.
		int nLastPos = -1;
		int nOffsetX = 0;
		for ( const auto& [ nnPos, ppNote ] : *ppPattern->getNotes() ) {
			if ( ppNote == nullptr ) {
				continue;
			}

			if ( nLastPos != nnPos ) {
				nLastPos = nnPos;
				nOffsetX = 0;
			}

			// NoteOff notes can have a custom probability and lead lag. But
			// having a velocity and pan would not make any sense for them.
			if ( ( ppNote->get_note_off() &&
				   ! ( m_mode == PatternEditor::Mode::Probability ||
					   m_mode == PatternEditor::Mode::LeadLag ) ) ||
				 ! ( ppNote->get_instrument_id() == selectedRow.nInstrumentID &&
					 ppNote->getType() == selectedRow.sType ) &&
				 ! m_selection.isSelected( ppNote ) ) {
				continue;
			}

			const auto style = static_cast<NoteStyle>(
				m_selection.isSelected( ppNote ) ?
				NoteStyle::Selected | baseStyle : baseStyle );
			drawNote( p, ppNote, style, nOffsetX );

			if ( m_layout != Layout::KeyOctave ) {
				// Within the key/octave view notes should be unique.
				++nOffsetX;
			}
		}
	}

	// draw border
	p.setPen( lineColor );
	p.setRenderHint( QPainter::Antialiasing );
	p.drawLine( 0, 0, m_nEditorWidth, 0 );
	p.setPen( QPen( lineColor, 2 ) );
	p.drawLine( 0, m_nEditorHeight, m_nEditorWidth, m_nEditorHeight );

	// draw inactive region
	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		p.drawLine( m_nActiveWidth, 0, m_nEditorWidth, 0 );
		p.setPen( QPen( lineInactiveColor, 2 ) );
		p.drawLine( m_nActiveWidth, m_nEditorHeight,
					m_nEditorWidth, m_nEditorHeight );
	}
}

std::vector<NotePropertiesRuler::SelectionIndex> NotePropertiesRuler::elementsIntersecting( const QRect& r ) {
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

	const Pattern::notes_t* notes = pPattern->getNotes();

	// Account for the notional active area of the slider. We allow a
	// width of 8 as this is the size of the circle used for the zero
	// position on the lead/lag editor.
	auto rNormalized = r.normalized();
	if ( rNormalized.top() == rNormalized.bottom() &&
		 rNormalized.left() == rNormalized.right() ) {
		rNormalized += QMargins( 2, 2, 2, 2 );
	}
	rNormalized += QMargins( 4, 4, 4, 4 );

	FOREACH_NOTE_CST_IT_BEGIN_LENGTH(notes,it, pPattern) {
		if ( ! ( it->second->get_instrument_id() == selectedRow.nInstrumentID &&
				 it->second->getType() == selectedRow.sType ) &&
			 ! m_selection.isSelected( it->second ) ) {
			continue;
		}

		int pos = it->first;
		uint x_pos = PatternEditor::nMargin + pos * m_fGridWidth;
		if ( rNormalized.intersects( QRect( x_pos, 0, 1, height() ) ) ) {
			result.push_back( it->second );
		}
	}

	// Updating selection, we may need to repaint the whole widget.
	invalidateBackground();
	update();

	return std::move(result);
}

void NotePropertiesRuler::selectAll()
{
	selectAllNotesInRow( m_pPatternEditorPanel->getSelectedRowDB() );
}

void NotePropertiesRuler::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {

		invalidateBackground();
		update();
	}
}
