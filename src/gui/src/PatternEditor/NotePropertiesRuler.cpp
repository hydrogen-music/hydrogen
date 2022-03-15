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

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
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

NotePropertiesRuler::NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode )
	: PatternEditor( parent, pPatternEditorPanel )
	, m_bEntered( false )
{
	auto pPref = H2Core::Preferences::get_instance();

	m_Mode = mode;

	m_fGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );

	m_fLastSetValue = 0.0;
	m_bValueHasBeenSet = false;
	m_bNeedsUpdate = true;

	if (m_Mode == VELOCITY ) {
		m_nEditorHeight = 100;
	}
	else if ( m_Mode == PAN ) {
		m_nEditorHeight = 100;
	}
	else if ( m_Mode == LEADLAG ) {
		m_nEditorHeight = 100;
	}
	else if ( m_Mode == NOTEKEY ) {
		m_nEditorHeight = 210;
	}
	if (m_Mode == PROBABILITY ) {
		m_nEditorHeight = 100;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumSize( m_nEditorWidth, m_nEditorHeight );

	updateEditor();
	show();

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &NotePropertiesRuler::onPreferencesChanged );

	setFocusPolicy( Qt::StrongFocus );

	// Generic pattern editor menu contains some operations that don't apply here, and we will want to add
	// menu options specific to this later.
	delete m_pPopupMenu;
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &PatternEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &PatternEditor::selectNone );

	setMouseTracking( true );
}




NotePropertiesRuler::~NotePropertiesRuler()
{
	//infoLog("DESTROY");
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

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	prepareUndoAction( ev->position().x() ); //get all old values
#else
	prepareUndoAction( ev->x() ); //get all old values
#endif

	float fDelta;
	if ( ev->modifiers() == Qt::ControlModifier || ev->modifiers() == Qt::AltModifier ) {
		fDelta = 0.01; // fine control
	} else {
		fDelta = 0.05; // coarse control
	}
	if ( ev->angleDelta().y() < 0 ) {
		fDelta = fDelta * -1.0;
	}

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	int nColumn = getColumn( ev->position().x() );
#else
	int nColumn = getColumn( ev->x() );
#endif

	m_pPatternEditorPanel->setCursorPosition( nColumn );
	HydrogenApp::get_instance()->setHideKeyboardCursor( true );

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( pHydrogen->getSelectedInstrumentNumber() );

	// Gather notes to act on: selected or under the mouse cursor
	std::list< Note *> notes;
	if ( m_selection.begin() != m_selection.end() ) {
		for ( Note *pNote : m_selection ) {
			notes.push_back( pNote );
		}
	} else {
		FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, nColumn ) {
			notes.push_back( it->second );
		}
	}

	for ( Note *pNote : notes ) {
		assert( pNote );
		if ( pNote->get_instrument() != pSelectedInstrument && !m_selection.isSelected( pNote ) ) {
			continue;
		}
		adjustNotePropertyDelta( pNote, fDelta, /* bMessage=*/ true );
	}

	addUndoAction();
	updateEditor();
}


void NotePropertiesRuler::mouseClickEvent( QMouseEvent *ev ) {
	if ( ev->button() == Qt::RightButton ) {
		m_pPopupMenu->popup( ev->globalPos() );

	} else {
		// Treat single click as an instantaneous drag
		propertyDragStart( ev );
		propertyDragUpdate( ev );
		propertyDragEnd();
	}
}

void NotePropertiesRuler::mousePressEvent( QMouseEvent* ev ) {
	PatternEditor::mousePressEvent( ev );

	// Update cursor position
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		int nColumn = getColumn( ev->x(), /* bUseFineGrained=*/ true );
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
	if ( m_selection.isMoving() ) {
		prepareUndoAction( ev->x() );
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

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( pHydrogen->getSelectedInstrumentNumber() );
	float fDelta;

	QPoint movingOffset = m_selection.movingOffset();
	if ( m_Mode == NOTEKEY ) {
		fDelta = (float)-movingOffset.y() / 10;
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
	
	for ( Note *pNote : m_selection ) {
		if ( pNote->get_instrument() == pSelectedInstrument || m_selection.isSelected( pNote ) ) {

			// Record original note if not already recorded
			if ( m_oldNotes.find( pNote ) == m_oldNotes.end() ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}

			adjustNotePropertyDelta( pNote, fDelta, bSendStatusMsg );
		}
	}
	updateEditor();
}

void NotePropertiesRuler::selectionMoveEndEvent( QInputEvent *ev ) {
	//! The "move" has already been reflected in the notes. Now just complete Undo event.
	addUndoAction();
	updateEditor();
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
		switch ( m_Mode ) {
		case VELOCITY:
			pNote->set_velocity( pOldNote->get_velocity() );
			break;
		case PAN:
			pNote->setPan( pOldNote->getPan() );
			break;
		case LEADLAG:
			pNote->set_lead_lag( pOldNote->get_lead_lag() );
			break;
		case NOTEKEY:
			pNote->set_key_octave( pOldNote->get_key(), pOldNote->get_octave() );
			break;
		case PROBABILITY:
			pNote->set_probability( pOldNote->get_probability() );
			break;
		default:
			break;
		}
	}

	if ( m_oldNotes.size() == 0 ) {
		for ( const auto& it : m_oldNotes ){
			triggerStatusMessage( it.second );
		}
	}

	clearOldNotes();
}


void NotePropertiesRuler::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	if ( ev->buttons() == Qt::NoButton ) {
		int nColumn = getColumn( ev->x() );
		bool bFound = false;
		FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, nColumn ) {
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
	setCursor( Qt::CrossCursor );
	prepareUndoAction( ev->x() );
	updateEditor();
}


//! Preserve current note properties at position x (or in selection, if any) for use in later UndoAction.
void NotePropertiesRuler::prepareUndoAction( int x )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	clearOldNotes();

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( nSelectedInstrument );

	if ( m_selection.begin() != m_selection.end() ) {
		// If there is a selection, preserve the initial state of all the selected notes.
		for ( Note *pNote : m_selection ) {
			if ( pNote->get_instrument() == pSelectedInstrument || m_selection.isSelected( pNote ) ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}
		}

	} else {
		// No notes are selected. The target notes to adjust are all those at column given by 'x', so we preserve these.
		int nColumn = getColumn( x );
		FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, nColumn ) {
			Note *pNote = it->second;
			if ( pNote->get_instrument() == pSelectedInstrument ) {
				m_oldNotes[ pNote ] = new Note( pNote );
			}
		}
	}
}

//! Update notes for a property adjust drag, in response to the mouse moving. This modifies the values of the
//! notes as the mouse moves, but does not complete an undo action until the notes final value has been
//! set. This occurs either when the mouse is released, or when the pointer moves off of the note's column.
void NotePropertiesRuler::propertyDragUpdate( QMouseEvent *ev )
{
	if (m_pPattern == nullptr) {
		return;
	}

	int nColumn = getColumn( ev->x() );

	m_pPatternEditorPanel->setCursorPosition( nColumn );

	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	
	pHydrogenApp->setHideKeyboardCursor( true );

	if ( m_nDragPreviousColumn != nColumn ) {
		// Complete current undo action, and start a new one.
		addUndoAction();
		prepareUndoAction( ev->x() );
	}

	float val = height() - ev->y();
	if (val > height()) {
		val = height();
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	int keyval = val;
	val = val / height(); // val is normalized, in [0;1]
	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( nSelectedInstrument );

	bool bValueSet = false;

	FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, nColumn ) {
		Note *pNote = it->second;

		if ( pNote->get_instrument() != pSelectedInstrument &&
			 !m_selection.isSelected( pNote ) ) {
			continue;
		}
		if ( m_Mode == VELOCITY && !pNote->get_note_off() ) {
			pNote->set_velocity( val );
			m_fLastSetValue = val;
			bValueSet = true;
		}
		else if ( m_Mode == PAN && !pNote->get_note_off() ){
			if ( (ev->button() == Qt::MiddleButton)
					|| (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				val = 0.5; // central pan
			}
			pNote->setPanWithRangeFrom0To1( val ); // checks the boundaries
			m_fLastSetValue = pNote->getPanWithRangeFrom0To1();
			bValueSet = true;
			
		}
		else if ( m_Mode == LEADLAG ){
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
		else if ( m_Mode == NOTEKEY ){
			if ( ev->button() != Qt::MiddleButton &&
				 ! ( ev->modifiers() == Qt::ControlModifier &&
					 ev->button() == Qt::LeftButton ) ) {
				//set the note height
				int k = 666;
				int o = 666;
				if( keyval >= 6 && keyval <= 125 ) {
					k = ( keyval - 6 ) / 10;
				}
				else if( keyval >= 135 && keyval <= 205 ) {
					o = ( keyval - 166 ) / 10;
					if ( o == -4 ) {
						o = -3; // 135
					}
				}
				m_fLastSetValue = o * 12 + k;
				bValueSet = true;
				pNote->set_key_octave((Note::Key)k,(Note::Octave)o); // won't set wrong values see Note::set_key_octave
			}
		}
		else if ( m_Mode == PROBABILITY && !pNote->get_note_off() ) {
			m_fLastSetValue = val;
			bValueSet = true;
			pNote->set_probability( val );
		}
		
		if ( bValueSet ) {
			triggerStatusMessage( pNote );
			m_bValueHasBeenSet = true;
			Hydrogen::get_instance()->setIsModified( true );
		}
	}


	m_nDragPreviousColumn = nColumn;
	updateEditor();

	m_pPatternEditorPanel->getPianoRollEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}

void NotePropertiesRuler::propertyDragEnd()
{
	addUndoAction();
	unsetCursor();
	updateEditor();
}

//! Adjust a note's property by applying a delta to the current value, and clipping to the appropriate
//! range. Optionally, show a message with the value for some properties.
void NotePropertiesRuler::adjustNotePropertyDelta( Note *pNote, float fDelta, bool bMessage )
{
	Note *pOldNote = m_oldNotes[ pNote ];
	assert( pOldNote );

	bool bValueSet = false;
	
	switch (m_Mode) {
	case VELOCITY:
		if ( !pNote->get_note_off() ) {
			float fVelocity = qBound(  VELOCITY_MIN, (pOldNote->get_velocity() + fDelta), VELOCITY_MAX );
			pNote->set_velocity( fVelocity );
			m_fLastSetValue = fVelocity;
			bValueSet = true;
		}
		break;
	case PAN:
		if ( !pNote->get_note_off() ) {
			float fVal = pOldNote->getPanWithRangeFrom0To1() + fDelta; // value in [0,1] or slight out of boundaries
			pNote->setPanWithRangeFrom0To1( fVal ); // checks the boundaries as well
			m_fLastSetValue = pNote->getPanWithRangeFrom0To1();
			bValueSet = true;
		}
		break;
	case LEADLAG:
		{
			float fLeadLag = qBound( LEAD_LAG_MIN, pOldNote->get_lead_lag() - fDelta, LEAD_LAG_MAX );
			pNote->set_lead_lag( fLeadLag );
			m_fLastSetValue = fLeadLag;
			bValueSet = true;
		}
		break;
	case PROBABILITY:
		if ( !pNote->get_note_off() ) {
			float fProbability = qBound( 0.0f, pOldNote->get_probability() + fDelta, 1.0f );
			pNote->set_probability( fProbability );
			m_fLastSetValue = fProbability;
			bValueSet = true;
		}
		break;
	case NOTEKEY:
		int nPitch = qBound( 12 * OCTAVE_MIN, (int)( pOldNote->get_notekey_pitch() + fDelta ),
							 12 * OCTAVE_MAX + KEY_MAX );
		Note::Octave octave;
		if ( nPitch >= 0 ) {
			octave = (Note::Octave)( nPitch / 12 );
		} else {
			octave = (Note::Octave)( (nPitch-11) / 12 );
		}
		Note::Key key = (Note::Key)( nPitch - 12 * (int)octave );

		pNote->set_key_octave( key, octave );
		m_fLastSetValue = 12 * octave + key;

		bValueSet = true;
		break;
	}

	if ( bValueSet ) {
		Hydrogen::get_instance()->setIsModified( true );
		m_bValueHasBeenSet = true;
		if ( bMessage ) {
			triggerStatusMessage( pNote );
		}
	}
}

void NotePropertiesRuler::keyPressEvent( QKeyEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	const int nWordSize = 5;
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bUnhideCursor = true;

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
			int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
			std::shared_ptr<Song> pSong = (Hydrogen::get_instance())->getSong();
			int nNotes = 0;

			// Collect notes to apply the change to
			std::list< Note *> notes;
			if ( m_selection.begin() != m_selection.end() ) {
				for ( Note *pNote : m_selection ) {
					nNotes++;
					notes.push_back( pNote );
				}
			} else {
				FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, column ) {
					Note *pNote = it->second;
					assert( pNote );
					assert( pNote->get_position() == column );
					if ( pNote->get_instrument() ==
						 pSong->getInstrumentList()->get( nSelectedInstrument ) ) {
						nNotes++;
						notes.push_back( pNote );
					}
				}
			}

			// For the NoteKeyEditor, adjust the pitch by a whole semitone
			if ( m_Mode == NOTEKEY ) {
				if ( fDelta > 0.0 ) {
					fDelta = 1;
				} else if ( fDelta < 0.0 ) {
					fDelta = -1;
				}
			}

			prepareUndoAction( PatternEditor::nMargin + column * m_fGridWidth );

			for ( Note *pNote : notes ) {

				if ( !bRepeatLastValue ) {
					
					// Apply delta to the property
					adjustNotePropertyDelta( pNote, fDelta, nNotes == 1 );

				} else {

					bool bValueSet = false;
					
					// Repeating last value
					switch (m_Mode) {
					case VELOCITY:
						if ( !pNote->get_note_off() ) {
							pNote->set_velocity( m_fLastSetValue );
							bValueSet = true;
						}
						break;
					case PAN:
						if ( !pNote->get_note_off() ) {
							if ( m_fLastSetValue > 1. ) { // TODO whats this for? is it ever reached?
								printf( "reached  m_fLastSetValue > 1 in NotePropertiesRuler.cpp\n" );
								pNote->setPanWithRangeFrom0To1( m_fLastSetValue );
							}
							bValueSet = true;
						}
						break;
					case LEADLAG:
						pNote->set_lead_lag( m_fLastSetValue );
							bValueSet = true;
						break;
					case PROBABILITY:
						if ( !pNote->get_note_off() ) {
							pNote->set_probability( m_fLastSetValue );
							bValueSet = true;
						}
						break;
					case NOTEKEY:
						pNote->set_key_octave( (Note::Key)( (int)m_fLastSetValue % 12 ),
											   (Note::Octave)( (int)m_fLastSetValue / 12 ) );
						bValueSet = true;
						break;
					}

					if ( bValueSet ) {
						if ( nNotes == 1 ) {
							triggerStatusMessage( pNote );
						}
						Hydrogen::get_instance()->setIsModified( true );
					}
				}
			}
			addUndoAction();
		} else {
			HydrogenApp::get_instance()->setHideKeyboardCursor( true );
			ev->ignore();
			return;
		}
	}
	if ( bUnhideCursor ) {
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
	}
	
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}

	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	updateEditor();
	ev->accept();

}

void NotePropertiesRuler::addUndoAction()
{
	if ( m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	InstrumentList *pInstrumentList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	int nSize = m_oldNotes.size();
	if ( nSize != 0 ) {
		QUndoStack *pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;
		QString sMode;
		switch ( m_Mode ) {
		case VELOCITY:
			sMode = "VELOCITY";
			break;
		case PAN:
			sMode = "PAN";
			break;
		case LEADLAG:
			sMode = "LEADLAG";
			break;
		case NOTEKEY:
			sMode = "NOTEKEY";
			break;
		case PROBABILITY:
			sMode = "PROBABILITY";
			break;
		default:
			break;
		}

		if ( nSize != 1 ) {
			pUndoStack->beginMacro( QString( tr( "Edit %1 property of %2 notes" ) )
									.arg( sMode.toLower() )
									.arg( nSize ) );
		}
		for ( auto it : m_oldNotes ) {
			Note *pNewNote = it.first, *pOldNote = it.second;
			pUndoStack->push( new SE_editNotePropertiesVolumeAction( pNewNote->get_position(),
																	 sMode,
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
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ) {
		createBackground();
	}

	QPainter painter(this);
	if ( m_bNeedsUpdate ) {
		createBackground();
	}
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, ev->rect() );

	// Draw playhead
	if ( m_nTick != -1 ) {

		int nOffset = Skin::getPlayheadShaftOffset();
		int nX = static_cast<int>(static_cast<float>(PatternEditor::nMargin) +
								  static_cast<float>(m_nTick) *
								  m_fGridWidth ) + 1;
		Skin::setPlayheadPen( &painter, false );
		painter.drawLine( nX, 2, nX, height() - 2 );
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
	
	switch ( m_Mode ) {
	case VELOCITY:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteVelocityScrollArea();
		break;
	case PAN:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNotePanScrollArea();
		break;
	case LEADLAG:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteLeadLagScrollArea();
		break;
	case NOTEKEY:
		pScrollArea = HydrogenApp::get_instance()->getPatternEditorPanel()->getNoteNoteKeyScrollArea();
		break;
	case PROBABILITY:
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



void NotePropertiesRuler::createVelocityBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	QColor res_1( pPref->getColorTheme()->m_patternEditor_line1Color );

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	QColor horizLinesColor( backgroundColor.red() - 20,
							backgroundColor.green() - 20,
							backgroundColor.blue() - 20 );

	unsigned nNotes = MAX_NOTES;
	if ( m_pPattern != nullptr ) {
		nNotes = m_pPattern->get_length();
	}

	QPainter p( pixmap );

	p.fillRect( 0, 0, PatternEditor::nMargin + nNotes * m_fGridWidth,
				height(), backgroundColor );

	drawGridLines( p, Qt::DotLine );

	// Horizontal lines at 10% intervals
	p.setPen( horizLinesColor );
	for (unsigned y = 0; y < m_nEditorHeight; y = y + (m_nEditorHeight / 10)) {
		p.drawLine( PatternEditor::nMargin, y,
					PatternEditor::nMargin + nNotes * m_fGridWidth, y );
	}

	// draw velocity lines
	if (m_pPattern != nullptr) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND(notes,coit,pos) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->getInstrumentList()->get( nSelectedInstrument )
					 && !m_selection.isSelected( pNote ) ) {
					continue;
				}
				uint x_pos = PatternEditor::nMargin + pos * m_fGridWidth;
				uint line_end = height();


				uint value = 0;
				if ( m_Mode == VELOCITY ) {
					value = (uint)(pNote->get_velocity() * height());
				}
				else if ( m_Mode == PROBABILITY ) {
					value = (uint)(pNote->get_probability() * height());
				}
				uint line_start = line_end - value;
				QColor centerColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, line_start - 2,
									   nLineWidth + 4,  line_end - line_start + 4 ,
									   4, 4 );
				}

				p.fillRect( x_pos - 1 + xoffset, line_start, nLineWidth,  line_end - line_start , centerColor );
				xoffset++;
			}
		}
	}
	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}



void NotePropertiesRuler::createPanBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	
	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	QColor horizLinesColor( backgroundColor.red() - 20,
							backgroundColor.green() - 20,
							backgroundColor.blue() - 20 );

	QColor res_1( pPref->getColorTheme()->m_patternEditor_line1Color );

	QPainter p( pixmap );

	unsigned nNotes = MAX_NOTES;
	if ( m_pPattern != nullptr ) {
		nNotes = m_pPattern->get_length();
	}
	p.fillRect( 0, 0, PatternEditor::nMargin + nNotes * m_fGridWidth,
				height(), backgroundColor );

	// central line
	p.setPen( horizLinesColor );
	p.drawLine(0, height() / 2.0, m_nEditorWidth, height() / 2.0);

	// vertical lines
	drawGridLines( p, Qt::DotLine );

	if ( m_pPattern != nullptr ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND(notes,coit,pos) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_note_off() || (pNote->get_instrument()
											   != pSong->getInstrumentList()->get( nSelectedInstrument )
											   && !m_selection.isSelected( pNote ) ) ) {
					continue;
				}
				uint x_pos = PatternEditor::nMargin + pNote->get_position() * m_fGridWidth;
				QColor centerColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );

				p.setPen( Qt::NoPen );
				if ( pNote->getPan() == 0.f ) {
					// pan value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( centerColor ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
				} else {
					int y_start = height() * 0.5;
					int y_width = (int)( - 0.5 * height() * pNote->getPan() );

					int nLineWidth = 3;
					p.fillRect( x_pos - 1 + xoffset, y_start, nLineWidth, y_width, QColor(  centerColor) );
					p.fillRect( x_pos - 1 + xoffset, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor(  centerColor ) );
				}

				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, 0,
									   nLineWidth + 4,  height() ,
									   4, 4 );
				}
				xoffset++;
			}
		}
	}

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}

void NotePropertiesRuler::createLeadLagBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	
	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	QColor horizLinesColor( backgroundColor.red() - 20,
							backgroundColor.green() - 20,
							backgroundColor.blue() - 20 );

	QColor res_1( pPref->getColorTheme()->m_patternEditor_line1Color );

	QPainter p( pixmap );

	unsigned nNotes = MAX_NOTES;
	if ( m_pPattern != nullptr ) {
		nNotes = m_pPattern->get_length();
	}
	p.fillRect( 0, 0, PatternEditor::nMargin + nNotes * m_fGridWidth,
				height(), backgroundColor );

	// central line
	p.setPen( horizLinesColor );
	p.drawLine(0, height() / 2.0, m_nEditorWidth, height() / 2.0);

	// vertical lines
	drawGridLines( p, Qt::DotLine );

	if ( m_pPattern != nullptr ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND(notes,coit,pos) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->getInstrumentList()->get( nSelectedInstrument )
					 && !m_selection.isSelected( pNote ) ) {
					continue;
				}

				uint x_pos = PatternEditor::nMargin +
					pNote->get_position() * m_fGridWidth;

				int red1 = (int) (pNote->get_velocity() * 255);
				int green1;
				int blue1;
				blue1 = ( 255 - (int) red1 )* .33;
				green1 =  ( 255 - (int) red1 );

				p.setPen( Qt::NoPen );
				if (pNote->get_lead_lag() == 0) {
				
					// leadlag value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( 0 , 0 , 0 ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
				} else {
					int y_start = (int)( height() * 0.5 );
					int y_end = y_start + ((pNote->get_lead_lag()/2) * height());
		
					int nLineWidth = 3;
					int red;
					int green;
					int blue = (int) (pNote->get_lead_lag() * 255);
					if (blue < 0)  {
						red = blue *-1;
						blue = (int) red * .33;
						green = (int) red * .33;
					} else {
						red = (int) blue * .33;
						green = (int) blue * .33;
					}
					p.fillRect( x_pos - 1 + xoffset, y_start, nLineWidth, y_end - y_start, QColor( red, green ,blue ) );
		
					p.fillRect( x_pos - 1 + xoffset, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor( red1, green1 ,blue1 ) );
				}

				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, 0,
									   nLineWidth + 4,  height() ,
									   4, 4 );
				}

				xoffset++;
 			}
		}
	}

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}



void NotePropertiesRuler::createNoteKeyBackground(QPixmap *pixmap)
{
	auto pPref = H2Core::Preferences::get_instance();
	
	QColor res_1( pPref->getColorTheme()->m_patternEditor_line1Color );

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	QColor horizLinesColor( backgroundColor.red() - 100,
							backgroundColor.green() - 100,
							backgroundColor.blue() - 100 );

	unsigned nNotes = MAX_NOTES;
	if ( m_pPattern != nullptr ) {
		nNotes = m_pPattern->get_length();
	}
	QPainter p( pixmap );

	p.fillRect( 0, 0, PatternEditor::nMargin + nNotes * m_fGridWidth, height(), backgroundColor );

	p.setPen( horizLinesColor );
	for (unsigned y = 10; y < 80; y = y + 10 ) {
		p.setPen( QPen( res_1, 1, Qt::DashLine ) );
		if (y == 40) p.setPen( QPen( QColor(0,0,0), 1, Qt::SolidLine ) );
		p.drawLine( PatternEditor::nMargin, y,
					PatternEditor::nMargin + nNotes * m_fGridWidth, y );
	}

	for (unsigned y = 90; y < 210; y = y + 10 ) {
		p.setPen( QPen( QColor( 255, 255, 255 ), 9, Qt::SolidLine, Qt::FlatCap) );
		if ( y == 100 ||y == 120 ||y == 140 ||y == 170 ||y == 190) {
			p.setPen( QPen( QColor( 128, 128, 128 ), 9, Qt::SolidLine, Qt::FlatCap ) );
		}
		p.drawLine( PatternEditor::nMargin, y,
					PatternEditor::nMargin + nNotes * m_fGridWidth, y );
	}

	// Annotate with note class names
	static QString noteNames[] = { tr( "B" ), tr( "A#" ), tr( "A" ), tr( "G#" ), tr( "G" ), tr( "F#" ),
								   tr( "F" ), tr( "E" ), tr( "D#" ), tr( "D" ), tr( "C#" ), tr( "C" ) };
	
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	
	p.setFont( font );
	p.setPen( QColor( 0, 0, 0 ) );
	for ( int n = 0; n < 12; n++ ) {
		p.drawText( 5, 90 + 10 * n +3, noteNames[n] );
	}

	// vertical lines
	drawGridLines( p, Qt::DotLine );

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);


	// Black outline each key
	for (unsigned y = 90; y <= 210; y = y + 10 ) {
		p.setPen( QPen( QColor( 0, 0, 0 ), 1, Qt::SolidLine));
		p.drawLine( PatternEditor::nMargin, y - 5,
					PatternEditor::nMargin + nNotes * m_fGridWidth, y-5);
	}

	//paint the octave
	if ( m_pPattern != nullptr ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->getInstrumentList()->get( nSelectedInstrument )
				 && !m_selection.isSelected( pNote ) ) {
				continue;
			}
			if ( !pNote->get_note_off() ) {
				uint x_pos = 17 + pNote->get_position() * m_fGridWidth;
				uint y_pos = (4-pNote->get_octave())*10-3;
				p.setBrush(QColor( 99, 160, 233 ));
				p.drawEllipse( x_pos, y_pos, 6, 6);
			}
		}
	}

	//paint the note
	if ( m_pPattern != nullptr ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->getInstrumentList()->get( nSelectedInstrument )
				 && !m_selection.isSelected( pNote ) ) {
				continue;
			}

			if ( !pNote->get_note_off() ) {
				int d = 8;
				int k = pNote->get_key();
				uint x_pos = 16 + pNote->get_position() * m_fGridWidth;
				uint y_pos = 200-(k*10)-4;

				x_pos -= 1;
				y_pos -= 1;
				d += 2;
				p.setPen( Qt::NoPen );
				p.setBrush(QColor( 0, 0, 0));
				p.drawEllipse( x_pos, y_pos, d, d);

				// Paint selection outlines
				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 +3, 0,
									   nLineWidth + 4 + 4,  height() ,
									   4, 4 );
				}
			}
		}
	}
}


void NotePropertiesRuler::updateEditor( bool bPatternOnly )
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

	// update editor width
	if ( m_pPattern != nullptr ) {
		m_nEditorWidth = PatternEditor::nMargin + m_pPattern->get_length() * m_fGridWidth;
	}
	else {
		m_nEditorWidth = PatternEditor::nMargin + MAX_NOTES * m_fGridWidth;
	}

	if ( !m_bNeedsUpdate ) {
		m_bNeedsUpdate = true;
		createBackground();
		update();
	}
}

void NotePropertiesRuler::createBackground()
{
	assert( m_bNeedsUpdate );
	resize( m_nEditorWidth, height() );
	
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio ,
										   m_nEditorHeight * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	if ( m_Mode == VELOCITY || m_Mode == PROBABILITY ) {
		createVelocityBackground( m_pBackgroundPixmap );
	}
	else if ( m_Mode == PAN ) {
		createPanBackground( m_pBackgroundPixmap );
	}
	else if ( m_Mode == LEADLAG ) {
		createLeadLagBackground( m_pBackgroundPixmap );
	}
	else if ( m_Mode == NOTEKEY ) {
		createNoteKeyBackground( m_pBackgroundPixmap );
	}
	update();
}


void NotePropertiesRuler::selectedPatternChangedEvent()
{
	updateEditor();
}



void NotePropertiesRuler::selectedInstrumentChangedEvent()
{
	updateEditor();
}


std::vector<NotePropertiesRuler::SelectionIndex> NotePropertiesRuler::elementsIntersecting( QRect r ) {
	std::vector<SelectionIndex> result;
	if ( m_pPattern == nullptr ) {
		return std::move( result );
	}
	
	const Pattern::notes_t* notes = m_pPattern->get_notes();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	auto pInstrument = pSong->getInstrumentList()->get( nSelectedInstrument );

	// Account for the notional active area of the slider. We allow a
	// width of 8 as this is the size of the circle used for the zero
	// position on the lead/lag editor.
	r = r.normalized();
	if ( r.top() == r.bottom() && r.left() == r.right() ) {
		r += QMargins( 2, 2, 2, 2 );
	}
	r += QMargins( 4, 4, 4, 4 );

	FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
		if ( it->second->get_instrument() !=  pInstrument
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
	updateEditor();
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

void NotePropertiesRuler::selectAll() {
	selectInstrumentNotes( Hydrogen::get_instance()->getSelectedInstrumentNumber() );
}

void NotePropertiesRuler::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {

		m_bNeedsUpdate = true;
		update();
	}
}

void NotePropertiesRuler::triggerStatusMessage( Note* pNote ) const {
	QString s;
	QString sUnit( tr( "ticks" ) );
	float fValue;
	
	switch ( m_Mode ) {
	case VELOCITY:
		if ( ! pNote->get_note_off() ) {
			s = QString( tr( "Set note velocity" ) )
				.append( QString( ": [%1]")
						 .arg( pNote->get_velocity(), 2, 'f', 2 ) );
		}
		break;
		
	case PAN:
		if ( ! pNote->get_note_off() ) {

			// Round the pan to not miss the center due to fluctuations
			fValue = pNote->getPan() * 100;
			fValue = std::round( fValue );
			fValue = fValue / 100;
			
			if ( fValue > 0.0 ) {
				s = QString( tr( "Note panned to the right by" ) ).
					append( QString( ": [%1]" ).arg( fValue / 2, 2, 'f', 2 ) );
			} else if ( fValue < 0.0 ) {
				s = QString( tr( "Note panned to the left by" ) ).
					append( QString( ": [%1]" ).arg( -1 * fValue / 2, 2, 'f', 2 ) );
			} else {
				s = QString( tr( "Note centered" ) );
			}
		}
		break;
		
	case LEADLAG:;
		// Round the pan to not miss the center due to fluctuations
		fValue = pNote->get_lead_lag() * 100;
		fValue = std::round( fValue );
		fValue = fValue / 100;
		if ( fValue < 0.0 ) {
			s = QString( tr( "Leading beat by" ) )
				.append( QString( ": [%1] " )
						 .arg( fValue * -1 *
							   AudioEngine::getLeadLagInTicks() , 2, 'f', 2 ) )
				.append( sUnit );
		}
		else if ( fValue > 0.0 ) {
			s = QString( tr( "Lagging beat by" ) )
				.append( QString( ": [%1] " )
						 .arg( fValue *
							   AudioEngine::getLeadLagInTicks() , 2, 'f', 2 ) )
				.append( sUnit );
		}
		else {
			s = tr( "Note on beat" );
		}
		break;

	case NOTEKEY:
		s = QString( tr( "Set pitch" ) ).append( ": " ).append( tr( "key" ) )
			.append( QString( " [%1], " ).arg( Note::KeyToQString( pNote->get_key() ) ) )
			.append( tr( "octave" ) )
			.append( QString( ": [%1]" ).arg( pNote->get_octave() ) );
		break;

	case PROBABILITY:
		if ( ! pNote->get_note_off() ) {
			s = tr( "Set note probability to" )
				.append( QString( ": [%1]" ).arg( pNote->get_probability(), 2, 'f', 2 ) );
		}
		break;

	default:
		ERRORLOG( QString( "Unknown mode: [%1]" ).arg( m_Mode ) );
	}

	if ( ! s.isEmpty() ) {
		HydrogenApp::get_instance()->setStatusBarMessage( s, 2000 );
	}
}
					
