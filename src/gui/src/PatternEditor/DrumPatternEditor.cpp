/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "DrumPatternEditor.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"

#include <core/Globals.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences.h>
#include <core/EventQueue.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine.h>
#include <core/Helpers/Xml.h>

#include "UndoActions.h"
#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
#include "../Skin.h"

#include <math.h>
#include <cassert>
#include <algorithm>

using namespace H2Core;

const char* DrumPatternEditor::__class_name = "DrumPatternEditor";

DrumPatternEditor::DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel)
 : PatternEditor( parent, __class_name, panel )
{
	m_nGridHeight = Preferences::get_instance()->getPatternEditorGridHeight();
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;
	resize( m_nEditorWidth, m_nEditorHeight );

	Hydrogen::get_instance()->setSelectedInstrumentNumber( 0 );
}



DrumPatternEditor::~DrumPatternEditor()
{
}



void DrumPatternEditor::updateEditor( bool bPatternOnly )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	// check engine state
	int state = pHydrogen->getState();
	if ( (state != STATE_READY) && (state != STATE_PLAYING) ) {
		ERRORLOG( "FIXME: skipping pattern editor update (state should be READY or PLAYING)" );
		return;
	}

	updatePatternInfo();

	if ( m_pPattern ) {
		m_nEditorWidth = m_nMargin + m_nGridWidth * m_pPattern->get_length();
	}
	else {
		m_nEditorWidth = m_nMargin + m_nGridWidth * pHydrogen->getSong()->getDefaultPatternSize();
	}
	resize( m_nEditorWidth, height() );

	// redraw all
	update( 0, 0, width(), height() );
}


void DrumPatternEditor::addOrRemoveNote( int nColumn, int nRealColumn, int row,
										 bool bDoAdd, bool bDoDelete ) {
	Song *pSong = Hydrogen::get_instance()->getSong();
	Instrument *pSelectedInstrument = pSong->getInstrumentList()->get( row );
	H2Core::Note *pOldNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument );

	int oldLength = -1;
	float oldVelocity = 0.8f;
	float oldPan_L = 0.5f;
	float oldPan_R = 0.5f;
	float oldLeadLag = 0.0f;
	Note::Key oldNoteKeyVal = Note::C;
	Note::Octave oldOctaveKeyVal = Note::P8;
	bool isNoteOff = false;

	if ( pOldNote && !bDoDelete ) {
		// Found an old note, but we don't want to delete, so just return.
		return;
	} else if ( !pOldNote && !bDoAdd ) {
		// No note there, but we don't want to add a new one, so return.
		return;
	}

	if ( pOldNote ) {
		oldLength = pOldNote->get_length();
		oldVelocity = pOldNote->get_velocity();
		oldPan_L = pOldNote->get_pan_l();
		oldPan_R = pOldNote->get_pan_r();
		oldLeadLag = pOldNote->get_lead_lag();
		oldNoteKeyVal = pOldNote->get_key();
		oldOctaveKeyVal = pOldNote->get_octave();
		isNoteOff = pOldNote->get_note_off();
	}

	SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( nColumn,
																	 row,
																	 m_nSelectedPatternNumber,
																	 oldLength,
																	 oldVelocity,
																	 oldPan_L,
																	 oldPan_R,
																	 oldLeadLag,
																	 oldNoteKeyVal,
																	 oldOctaveKeyVal,
																	 pOldNote != nullptr,
																	 Preferences::get_instance()->getHearNewNotes(),
																	 false,
																	 false,
																	 isNoteOff );

	HydrogenApp::get_instance()->m_pUndoStack->push( action );


}


void DrumPatternEditor::mouseClickEvent( QMouseEvent *ev )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( m_pPattern == nullptr ) {
		return;
	}
	Song *pSong = pHydrogen->getSong();
	int nInstruments = pSong->getInstrumentList()->size();
	int row = (int)( ev->y()  / (float)m_nGridHeight);
	if (row >= nInstruments) {
		return;
	}
	int nColumn = getColumn( ev->x(), /* bUseFineGrained=*/ true );
	int nRealColumn = 0;
	if( ev->x() > m_nMargin ) {
		nRealColumn = ev->x() / static_cast<float>(m_nGridWidth) - m_nMargin;
	}
	if ( nColumn >= (int)m_pPattern->get_length() ) {
		update( 0, 0, width(), height() );
		return;
	}
	Instrument *pSelectedInstrument = pSong->getInstrumentList()->get( row );

	if( ev->button() == Qt::LeftButton && (ev->modifiers() & Qt::ShiftModifier) )
	{
		//shift + leftClick: add noteOff note
		HydrogenApp *pApp = HydrogenApp::get_instance();
		Note *pNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, false );
		if ( pNote != nullptr ) {
			SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( nColumn,
																			 row,
																			 m_nSelectedPatternNumber,
																			 pNote->get_length(),
																			 pNote->get_velocity(),
																			 pNote->get_pan_l(),
																			 pNote->get_pan_r(),
																			 pNote->get_lead_lag(),
																			 pNote->get_key(),
																			 pNote->get_octave(),
																			 true,
																			 false,
																			 false,
																			 false,
																			 pNote->get_note_off() );
			pApp->m_pUndoStack->push( action );
		} else {
			// Add stop-note
			SE_addNoteOffAction *action = new SE_addNoteOffAction( nColumn, row, m_nSelectedPatternNumber,
																   pNote != nullptr );
			pApp->m_pUndoStack->push( action );
		}
	}
	else if ( ev->button() == Qt::LeftButton ) {

		pHydrogen->setSelectedInstrumentNumber( row );
		addOrRemoveNote( nColumn, nRealColumn, row );
		m_selection.clearSelection();

	} else if ( ev->button() == Qt::RightButton ) {

		m_pPopupMenu->popup( ev->globalPos() );
		pHydrogen->setSelectedInstrumentNumber( row );

	} else {
		// Other clicks may also set instrument
		pHydrogen->setSelectedInstrumentNumber( row );
	}

	m_pPatternEditorPanel->setCursorPosition( nColumn );
	HydrogenApp::get_instance()->setHideKeyboardCursor( true );
	update();
}

void DrumPatternEditor::mouseDragStartEvent( QMouseEvent *ev )
{
	int row = (int)( ev->y()  / (float)m_nGridHeight);
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	int nColumn = getColumn( ev->x() );
	if ( ev->button() == Qt::RightButton ) {
		// Right button drag: adjust note length
		int nRealColumn = 0;
		Instrument *pSelectedInstrument = pSong->getInstrumentList()->get( row );

		m_pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, false );
		// needed for undo note length
		__nRealColumn = nRealColumn;
		__nColumn = nColumn;
		__row = row;
		if( m_pDraggedNote ){
			__oldLength = m_pDraggedNote->get_length();
		} else {
			__oldLength = -1;
		}
	} else {
		// Other drag (selection or move) we'll set the cursor input position to the start of the gesture
		pHydrogen->setSelectedInstrumentNumber( row );
		m_pPatternEditorPanel->setCursorPosition( nColumn );
		HydrogenApp::get_instance()->setHideKeyboardCursor( true );
	}
}

void DrumPatternEditor::addOrDeleteNoteAction(	int nColumn,
												int row,
												int selectedPatternNumber,
												int oldLength,
												float oldVelocity,
												float oldPan_L,
												float oldPan_R,
												float oldLeadLag,
												int oldNoteKeyVal,
												int oldOctaveKeyVal,
												bool listen,
												bool isMidi,
												bool isInstrumentMode,
												bool isNoteOff,
												bool isDelete )
{

	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	H2Core::Pattern *pPattern = nullptr;

	if ( ( selectedPatternNumber != -1 ) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}

	assert(pPattern);

	Song *pSong = Hydrogen::get_instance()->getSong();

	Instrument *pSelectedInstrument = pSong->getInstrumentList()->get( row );

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine


	if ( isDelete ) {
		// Find and delete an existing (matching) note.
		Pattern::notes_t *notes = (Pattern::notes_t *)pPattern->get_notes();
		bool bFound = false;
		FOREACH_NOTE_IT_BOUND( notes, it, nColumn ) {
			Note *pNote = it->second;
			assert( pNote );
			if ( ( isNoteOff && pNote->get_note_off() )
				 || ( pNote->get_instrument() == pSelectedInstrument
					  && pNote->get_key() == oldNoteKeyVal 
					  && pNote->get_octave() == oldOctaveKeyVal
					  && pNote->get_velocity() == oldVelocity ) ) {
				delete pNote;
				notes->erase( it );
				bFound = true;
				break;
			}
		}
		if ( !bFound ) {
			ERRORLOG( "Did not find note to delete" );
		}

	} else {
		// create the new note
		unsigned nPosition = nColumn;
		float fVelocity = oldVelocity;
		float fPan_L = oldPan_L ;
		float fPan_R = oldPan_R;
		int nLength = oldLength;


		if ( isNoteOff )
		{
			fVelocity = 0.0f;
			fPan_L = 0.5f;
			fPan_R = 0.5f;
			nLength = 1;
		}

		float fPitch = 0.f;
		
		Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch );
		pNote->set_note_off( isNoteOff );
		if ( !isNoteOff ) {
			pNote->set_lead_lag( oldLeadLag );
		}
		pNote->set_key_octave( (Note::Key)oldNoteKeyVal, (Note::Octave)oldOctaveKeyVal );
		pPattern->insert_note( pNote );

		if ( m_bSelectNewNotes ) {
			m_selection.addToSelection( pNote );
		}

		if ( isMidi ) {
			pNote->set_just_recorded(true);
		}
		// hear note
		if ( listen && !isNoteOff ) {
			fPitch = pSelectedInstrument->get_pitch_offset();
			Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan_L, fPan_R, nLength, fPitch);
			AudioEngine::get_instance()->get_sampler()->noteOn(pNote2);
		}
	}
	pSong->setIsModified( true );
	AudioEngine::get_instance()->unlock(); // unlock the audio engine

	m_pPatternEditorPanel->updateEditors();
}


// Find a note that matches pNote, and move it from (nColumn, nRow) to (nNewColumn, nNewRow)
void DrumPatternEditor::moveNoteAction( int nColumn,
										int nRow,
										int nPattern,
										int nNewColumn,
										int nNewRow,
										Note *pNote)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	PatternList *pPatternList = pSong->getPatternList();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();
	Pattern *pPattern = m_pPattern;
	Note *pFoundNote = nullptr;

	if ( nPattern < 0 || nPattern > pPatternList->size() ) {
		ERRORLOG( "Invalid pattern number" );
		AudioEngine::get_instance()->unlock();
		return;
	}

	Instrument *pFromInstrument = pInstrumentList->get( nRow ),
		*pToInstrument = pInstrumentList->get( nNewRow );

	FOREACH_NOTE_IT_BOUND((Pattern::notes_t *)pPattern->get_notes(), it, nColumn) {
		Note *pCandidateNote = it->second;
		if ( pCandidateNote->get_instrument() == pFromInstrument
			 && pCandidateNote->get_key() == pNote->get_key()
			 && pCandidateNote->get_octave() == pNote->get_octave()
			 && pCandidateNote->get_velocity() == pNote->get_velocity()
			 && pCandidateNote->get_lead_lag() == pNote->get_lead_lag()
			 && pCandidateNote->get_pan_r() == pNote->get_pan_r()
			 && pCandidateNote->get_pan_l() == pNote->get_pan_r()
			 && pCandidateNote->get_note_off() == pNote->get_note_off() ) {
			pFoundNote = pCandidateNote;
			if ( m_selection.isSelected( pCandidateNote ) ) {
				// If a candidate note is in the selection, this will be the one to move.
				break;
			}
		}
	}
	if ( pFoundNote == nullptr ) {
		ERRORLOG( "Couldn't find note to move" );
		AudioEngine::get_instance()->unlock();
		return;
	}

	pPattern->remove_note( pFoundNote );
	if ( pFromInstrument == pToInstrument ) {
		// Note can simply be moved.
		pFoundNote->set_position( nNewColumn );
		pPattern->insert_note( pFoundNote );
	} else {
		pPattern->remove_note( pFoundNote );
		Note *pNewNote = new Note( pFoundNote, pToInstrument );

		if ( m_selection.isSelected( pFoundNote) ) {
			m_selection.removeFromSelection( pFoundNote );
			m_selection.addToSelection( pNewNote );
		}
		pNewNote->set_position( nNewColumn );
		m_selection.addToSelection( pNewNote );
		pPattern->insert_note( pNewNote );
		delete pFoundNote;
	}

	AudioEngine::get_instance()->unlock();

	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::mouseDragEndEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	unsetCursor();

	if (m_pPattern == nullptr) {
		return;
	}

	if ( m_pDraggedNote ) {
		if ( m_pDraggedNote->get_note_off() ) return;

		SE_editNoteLenghtAction *action = new SE_editNoteLenghtAction( m_pDraggedNote->get_position(),  m_pDraggedNote->get_position(), __row, m_pDraggedNote->get_length(),__oldLength, m_nSelectedPatternNumber );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
		m_pDraggedNote = nullptr;
	}
}


///
/// Move or copy notes.
///
/// Moves or copies notes at the end of a Selection move, handling the
/// behaviours necessary for out-of-range moves or copies.
///
void DrumPatternEditor::selectionMoveEndEvent( QInputEvent *ev )
{
	updateModifiers( ev );
	QPoint offset = movingGridOffset();
	if ( offset.x() == 0 && offset.y() == 0 ) {
		// Move with no effect.
		return;
	}
	InstrumentList *pInstrumentList = Hydrogen::get_instance()->getSong()->getInstrumentList();

	validateSelection();

	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	if (m_bCopyNotMove) {
		pUndo->beginMacro( "copy notes" );
	} else {
		pUndo->beginMacro( "move notes" );
	}
	std::list< Note * > selectedNotes;
	for ( auto pNote : m_selection ) {
		selectedNotes.push_back( pNote );
	}

	if ( m_bCopyNotMove ) {
		// Clear selection so the new notes can be selection instead
		// of the originals.
		m_selection.clearSelection();
	}

	m_bSelectNewNotes = true;

	for ( auto pNote : selectedNotes ) {
		int nInstrument = pInstrumentList->index( pNote->get_instrument() );
		int nPosition = pNote->get_position();
		int nNewInstrument = nInstrument + offset.y();
		int nNewPosition = nPosition + offset.x();
		if ( nNewInstrument < 0 || nNewInstrument >= pInstrumentList->size()
			 || nNewPosition < 0 || nNewPosition >= m_pPattern->get_length() ) {

			if ( m_bCopyNotMove ) {
				// Copying a note to an out-of-range location. Nothing to do.
			} else {
				// Note is moved out of range. Delete it.
				pUndo->push( new SE_addOrDeleteNoteAction( nPosition,
														   nInstrument,
														   m_nSelectedPatternNumber,
														   pNote->get_length(),
														   pNote->get_velocity(),
														   pNote->get_pan_l(),
														   pNote->get_pan_r(),
														   pNote->get_lead_lag(),
														   pNote->get_key(),
														   pNote->get_octave(),
														   true,
														   false,
														   false,
														   false,
														   true ) );
			}

		} else {
			if ( m_bCopyNotMove ) {
				// Copy note to a new note.
				pUndo->push( new SE_addOrDeleteNoteAction( nNewPosition,
														   nNewInstrument,
														   m_nSelectedPatternNumber,
														   pNote->get_length(),
														   pNote->get_velocity(),
														   pNote->get_pan_l(),
														   pNote->get_pan_r(),
														   pNote->get_lead_lag(),
														   pNote->get_key(),
														   pNote->get_octave(),
														   false,
														   false,
														   false,
														   false,
														   false ) );
			} else {
				// Move note
				pUndo->push( new SE_moveNoteAction( nPosition, nInstrument, m_nSelectedPatternNumber,
													nNewPosition, nNewInstrument, pNote ) );
			}
		}
	}
	m_bSelectNewNotes = false;
	pUndo->endMacro();
}


void DrumPatternEditor::editNoteLengthAction( int nColumn, int nRealColumn, int row, int length, int selectedPatternNumber )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();

	H2Core::Pattern *pPattern = nullptr;
	if ( (selectedPatternNumber != -1) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}

	if( pPattern ) {
		Note *pDraggedNote = nullptr;
		Song *pSong = pEngine->getSong();
		Instrument *pSelectedInstrument = pSong->getInstrumentList()->get( row );

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		pDraggedNote = pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, false );
		if( pDraggedNote ){
			pDraggedNote->set_length( length );
		}

		AudioEngine::get_instance()->unlock();

		m_pPatternEditorPanel->updateEditors();
	}
}


///
/// Update the state during a Selection drag.
///
void DrumPatternEditor::mouseDragUpdateEvent( QMouseEvent *ev )
{
	if (m_pPattern == nullptr) {
		return;
	}

	int row = MAX_INSTRUMENTS - 1 - (ev->y()  / (int)m_nGridHeight);
	if (row >= MAX_INSTRUMENTS) {
		return;
	}

	if ( m_pDraggedNote ) {
		if ( m_pDraggedNote->get_note_off() ) return;
		int nTickColumn = getColumn( ev->x() );

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->get_position();

		if (nLen <= 0) {
			nLen = -1;
		}

		float fNotePitch = m_pDraggedNote->get_octave() * 12 + m_pDraggedNote->get_key();
		float fStep = 0;
		if(nLen > -1){
			fStep = pow( 1.0594630943593, ( double )fNotePitch );
		}else
		{
			fStep = 1.0;
		}
		m_pDraggedNote->set_length( nLen * fStep);

		Hydrogen::get_instance()->getSong()->setIsModified( true );
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		m_pPatternEditorPanel->updateEditors();
	}

}


///
/// Handle key press events.
///
/// Events are passed to Selection first, which may claim them (in which case they are ignored here).
///
void DrumPatternEditor::keyPressEvent( QKeyEvent *ev )
{
	Hydrogen *pH2 = Hydrogen::get_instance();
	int nSelectedInstrument = pH2->getSelectedInstrumentNumber();
	int nMaxInstrument = pH2->getSong()->getInstrumentList()->size();
	bool bUnhideCursor = true;

	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Key was claimed by Selection
	} else if ( ev->matches( QKeySequence::MoveToNextChar ) || ev->matches( QKeySequence::SelectNextChar ) ) {
		// ->
		m_pPatternEditorPanel->moveCursorRight();

	} else if ( ev->matches( QKeySequence::MoveToEndOfLine ) || ev->matches( QKeySequence::SelectEndOfLine ) ) {
		// -->|
		m_pPatternEditorPanel->setCursorPosition( m_pPattern->get_length() );

	} else if ( ev->matches( QKeySequence::MoveToPreviousChar ) || ev->matches( QKeySequence::SelectPreviousChar ) ) {
		// <-
		m_pPatternEditorPanel->moveCursorLeft();

	} else if ( ev->matches( QKeySequence::MoveToStartOfLine ) || ev->matches( QKeySequence::SelectStartOfLine ) ) {
		// |<--
		m_pPatternEditorPanel->setCursorPosition( 0 );

	} else if ( ev->matches( QKeySequence::MoveToNextLine ) || ev->matches( QKeySequence::SelectNextLine ) ) {
		if ( nSelectedInstrument + 1 < nMaxInstrument ) {
			pH2->setSelectedInstrumentNumber( nSelectedInstrument + 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToNextPage ) || ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down, scroll by the number of instruments that fit into the viewport
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		nSelectedInstrument += pParent->height() / m_nGridHeight;

		if ( nSelectedInstrument >= nMaxInstrument ) {
			nSelectedInstrument = nMaxInstrument - 1;
		}
		pH2->setSelectedInstrumentNumber( nSelectedInstrument );

	} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) || ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		pH2->setSelectedInstrumentNumber( nMaxInstrument-1 );

	} else if ( ev->matches( QKeySequence::MoveToPreviousLine ) || ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( nSelectedInstrument > 0 ) {
			pH2->setSelectedInstrumentNumber( nSelectedInstrument - 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToPreviousPage ) || ev->matches( QKeySequence::SelectPreviousPage ) ) {
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		nSelectedInstrument -= pParent->height() / m_nGridHeight;
		if ( nSelectedInstrument < 0 ) {
			nSelectedInstrument = 0;
		}
		pH2->setSelectedInstrumentNumber( nSelectedInstrument );

	} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) || ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		pH2->setSelectedInstrumentNumber( 0 );

	} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter / Return: add or remove note at current position
		m_selection.clearSelection();
		addOrRemoveNote( m_pPatternEditorPanel->getCursorPosition(), -1, nSelectedInstrument );

	} else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete / Backspace: delete selected notes, or note under keyboard cursor
		bUnhideCursor = false;
		if ( m_selection.begin() != m_selection.end() ) {
			// Delete selected notes if any
			deleteSelection();
		} else {
			// Delete note under the keyboard cursor.
			addOrRemoveNote(  m_pPatternEditorPanel->getCursorPosition(), -1, nSelectedInstrument,
							  /*bDoAdd=*/false, /*bDoDelete=*/true);

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
		return;
	}
	if ( bUnhideCursor ) {
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
	}
	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	m_pPatternEditorPanel->ensureCursorVisible();
	update();
	ev->accept();

}


///
/// Find all elements which intersect a selection area.
///
std::vector<DrumPatternEditor::SelectionIndex> DrumPatternEditor::elementsIntersecting( QRect r )
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	InstrumentList * pInstrList = pSong->getInstrumentList();
	uint h = m_nGridHeight / 3;

	// Expand the region by approximately the size of the note
	// ellipse, equivalent to testing for intersection between `r'
	// and the equivalent rect around the note.  We'll also allow
	// a few extra pixels if it's a single point click, to make it
	// easier to grab notes.

	r = r.normalized();
	if ( r.top() == r.bottom() && r.left() == r.right() ) {
		r += QMargins( 2, 2, 2, 2 );
	}
	r += QMargins( 4, h/2, 4, h/2 );


	// Calculate the first and last position values that this rect will intersect with
	int x_min = (r.left() - m_nMargin - 1) / m_nGridWidth;
	int x_max = (r.right() - m_nMargin) / m_nGridWidth;

	const Pattern::notes_t* notes = m_pPattern->get_notes();
	std::vector<SelectionIndex> result;

	for (auto it = notes->lower_bound( x_min ); it != notes->end() && it->first <= x_max; ++it ) {
		Note *note = it->second;
		int nInstrument = pInstrList->index( note->get_instrument() );
		uint x_pos = m_nMargin + (it->first * m_nGridWidth);
		uint y_pos = ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3;

		if ( r.contains( QPoint( x_pos, y_pos + h/2) ) ) {
			result.push_back( note );
		}
	}

	return std::move( result );
}

///
/// The screen area occupied by the keyboard cursor
///
QRect DrumPatternEditor::getKeyboardCursorRect()
{

	uint x = m_nMargin + m_pPatternEditorPanel->getCursorPosition() * m_nGridWidth;
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	uint y = nSelectedInstrument * m_nGridHeight;
	return QRect( x-m_nGridWidth*3, y+2, m_nGridWidth*6, m_nGridHeight-3 );

}

void DrumPatternEditor::selectAll()
{
	m_selection.clearSelection();
	FOREACH_NOTE_CST_IT_BEGIN_END(m_pPattern->get_notes(), it) {
		m_selection.addToSelection( it->second );
	}
	m_selection.updateWidgetGroup();
}


void DrumPatternEditor::deleteSelection()
{
	if ( m_selection.begin() != m_selection.end() ) {
		// Selection exists, delete it.
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		InstrumentList *pInstrumentList = pHydrogen->getSong()->getInstrumentList();
		QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
		validateSelection();

		// Construct list of UndoActions to perform before performing any of them, as the
		// addOrDeleteNoteAction may delete duplicate notes in undefined order.
		std::list< QUndoCommand *> actions;
		for ( Note *pNote : m_selection ) {
			if ( m_selection.isSelected( pNote ) ) {
				actions.push_back( new SE_addOrDeleteNoteAction( pNote->get_position(),
																 pInstrumentList->index( pNote->get_instrument() ),
																 m_nSelectedPatternNumber,
																 pNote->get_length(),
																 pNote->get_velocity(),
																 pNote->get_pan_l(),
																 pNote->get_pan_r(),
																 pNote->get_lead_lag(),
																 pNote->get_key(),
																 pNote->get_octave(),
																 true, // noteExisted
																 false, // listen
																 false,
																 false,
																 pNote->get_note_off() ) );
			}
		}

		pUndo->beginMacro("delete notes");
		for ( QUndoCommand *pAction : actions ) {
			pUndo->push( pAction );
		}
		pUndo->endMacro();
		m_selection.clearSelection();
	}
}


///
/// Paste selection
///
/// Selection is XML containing notes, contained in a root 'noteSelection' element.
///
void DrumPatternEditor::paste()
{
	QClipboard *clipboard = QApplication::clipboard();
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	InstrumentList *pInstrList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	XMLNode noteList;
	int nDeltaPos = 0, nDeltaInstrument = 0;

	XMLDoc doc;
	if ( ! doc.setContent( clipboard->text() ) ) {
		// Pasted something that's not valid XML.
		return;
	}

	XMLNode selection = doc.firstChildElement( "noteSelection" );
	if ( ! selection.isNull() ) {
		// Found a noteSelection. Structure is:
		// <noteSelection>
		//   <noteList>
		//     <note> ...
		noteList = selection.firstChildElement( "noteList" );
		if ( noteList.isNull() ) {
			return;
		}

		XMLNode positionNode = selection.firstChildElement( "sourcePosition" );

		// If position information is supplied in the selection, use
		// it to adjust the location relative to the current keyboard
		// input cursor.
		if ( !positionNode.isNull() ) {
			int nCurrentPos = m_pPatternEditorPanel->getCursorPosition();
			int nCurrentInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();

			nDeltaPos = nCurrentPos - positionNode.read_int( "position", nCurrentPos );
			nDeltaInstrument = nCurrentInstrument - positionNode.read_int( "instrument", nCurrentInstrument );
		}

	} else {
		XMLNode instrumentLine = doc.firstChildElement( "instrument_line" );
		if ( ! instrumentLine.isNull() ) {
			// Found 'instrument_line', structure is:
			// <instrument_line>
			//   <patternList>
			//     <pattern>
			//       <noteList>
			//         <note> ...
			XMLNode patternList = instrumentLine.firstChildElement( "patternList" );
			if ( patternList.isNull() ) {
				return;
			}
			XMLNode pattern = patternList.firstChildElement( "pattern" );
			if ( pattern.isNull() ) {
				return;
			}
			// Don't attempt to paste multiple patterns
			if ( ! pattern.nextSiblingElement( "pattern" ).isNull() ) {
				QMessageBox::information( this, "Hydrogen", tr( "Cannot paste multi-pattern selection" ) );
				return;
			}
			noteList = pattern.firstChildElement( "noteList" );
			if ( noteList.isNull() ) {
				return;
			}
		}
	}

	m_selection.clearSelection();
	m_bSelectNewNotes = true;

	if ( noteList.hasChildNodes() ) {

		pUndo->beginMacro( "paste notes" );
		for ( XMLNode n = noteList.firstChildElement( "note" ); ! n.isNull(); n = n.nextSiblingElement() ) {
			Note *pNote = Note::load_from( &n, pInstrList );
			int nPos = pNote->get_position() + nDeltaPos;
			int nInstrument = pInstrList->index( pNote->get_instrument() ) + nDeltaInstrument;

			if ( nPos >= 0 && nPos < m_pPattern->get_length()
				 && nInstrument >= 0 && nInstrument < pInstrList->size() ) {
				pUndo->push( new SE_addOrDeleteNoteAction( nPos,
														   nInstrument,
														   m_nSelectedPatternNumber,
														   pNote->get_length(),
														   pNote->get_velocity(),
														   pNote->get_pan_l(),
														   pNote->get_pan_r(),
														   pNote->get_lead_lag(),
														   pNote->get_key(),
														   pNote->get_octave(),
														   false, // isDelete
														   false, // listen
														   false, // isMidi
														   false, // isInstrumentMode
														   pNote->get_note_off()
														   ) );
			}
			delete pNote;
		}
		pUndo->endMacro();
	}

	m_bSelectNewNotes = false;
}


///
/// Draws a pattern
///
void DrumPatternEditor::__draw_pattern(QPainter& painter)
{
	const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	const QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );

	__create_background( painter );

	if (m_pPattern == nullptr) {
		return;
	}

	int nNotes = m_pPattern->get_length();
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = Hydrogen::get_instance()->getSong();

	InstrumentList * pInstrList = pSong->getInstrumentList();


	if ( m_nEditorHeight != (int)( m_nGridHeight * pInstrList->size() ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * pInstrList->size();
		resize( width(), m_nEditorHeight );
	}

	for ( uint nInstr = 0; nInstr < pInstrList->size(); ++nInstr ) {
		uint y = m_nGridHeight * nInstr;
		if ( nInstr == (uint)nSelectedInstrument ) {	// selected instrument
			painter.fillRect( 0, y + 1, ( m_nMargin + nNotes * m_nGridWidth ), m_nGridHeight - 1, selectedRowColor );
		}
	}


	// draw the grid
	__draw_grid( painter );


	// Draw cursor
	if ( hasFocus() && !HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		uint x = m_nMargin + m_pPatternEditorPanel->getCursorPosition() * m_nGridWidth;
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		uint y = nSelectedInstrument * m_nGridHeight;
		QPen p( Qt::black );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( QRect( x-m_nGridWidth*3, y+2, m_nGridWidth*6, m_nGridHeight-3 ), 4, 4 );
	}


	/*
		BUGFIX

		if m_pPattern is not renewed every time we draw a note,
		hydrogen will crash after you save a song and create a new one.
		-smoors
	*/
	updatePatternInfo();

	if( m_pPattern ) {
		const Pattern::notes_t* pNotes = m_pPattern->get_notes();
		if( pNotes->size() == 0) return;

		validateSelection();

		FOREACH_NOTE_CST_IT_BEGIN_END(pNotes,it) {
			Note *note = it->second;
			assert( note );
			__draw_note( note, painter );
		}
	}
}



///
/// Draws a note
///
void DrumPatternEditor::__draw_note( Note *note, QPainter& p )
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor noteColor( pStyle->m_patternEditor_noteColor.getRed(), pStyle->m_patternEditor_noteColor.getGreen(), pStyle->m_patternEditor_noteColor.getBlue() );
	static const QColor noteoffColor( pStyle->m_patternEditor_noteoffColor.getRed(), pStyle->m_patternEditor_noteoffColor.getGreen(), pStyle->m_patternEditor_noteoffColor.getBlue() );

	p.setRenderHint( QPainter::Antialiasing );

	int nInstrument = -1;
	InstrumentList * pInstrList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	for ( uint nInstr = 0; nInstr < pInstrList->size(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr == note->get_instrument() ) {
 			nInstrument = nInstr;
			break;
		}
	}
	if ( nInstrument == -1 ) {
		ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	uint pos = note->get_position();

	QColor color = computeNoteColor( note->get_velocity() );

	uint w = 8;
	uint h =  m_nGridHeight / 3;
	uint x_pos = m_nMargin + (pos * m_nGridWidth);
	uint y_pos = ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3;

	bool bSelected = m_selection.isSelected( note );

	if ( bSelected ) {
		QPen selectedPen( selectedNoteColor( pStyle ) );
		selectedPen.setWidth( 2 );
		p.setPen( selectedPen );
		p.setBrush( Qt::NoBrush );
	}

	bool bMoving = bSelected && m_selection.isMoving();
	QPen movingPen( noteColor );
	QPoint movingOffset;

	if ( bMoving ) {
		movingPen.setStyle( Qt::DotLine );
		movingPen.setWidth( 2 );
		QPoint delta = movingGridOffset();
		movingOffset = QPoint( delta.x() * m_nGridWidth,
							   delta.y() * m_nGridHeight );
	}

	if ( note->get_length() == -1 && note->get_note_off() == false ) {	// trigger note

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
		}
		p.setPen(noteColor);
		p.setBrush( color );
		p.drawEllipse( x_pos -4 , y_pos, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + x_pos -4 -2, movingOffset.y() + y_pos -2 , w + 4, h + 4 );
		}

	}
	else if ( note->get_length() == 1 && note->get_note_off() == true ){

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
		}
		p.setPen( noteoffColor );
		p.setBrush(QColor( noteoffColor));
		p.drawEllipse( x_pos -4 , y_pos, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + x_pos -4 -2, movingOffset.y() + y_pos -2, w + 4, h + 4 );
		}

	}
	else {
		float fNotePitch = note->get_octave() * 12 + note->get_key();
		float fStep = pow( 1.0594630943593, ( double )fNotePitch );

		uint x = m_nMargin + (pos * m_nGridWidth);
		int w = m_nGridWidth * note->get_length() / fStep;
		w = w - 1;	// lascio un piccolo spazio tra una nota ed un altra

		int y = (int) ( ( nInstrument ) * m_nGridHeight  + (m_nGridHeight / 100.0 * 30.0) );
		int h = (int) (m_nGridHeight - ((m_nGridHeight / 100.0 * 30.0) * 2.0) );
		if ( bSelected ) {
			p.drawRoundedRect( x-2, y+1-2, w+4, h+1+4, 4, 4 );
		}
		p.setPen(noteColor);
		p.setBrush( color );
		p.fillRect( x, y + 1, w, h + 1, color );	/// \todo: definire questo colore nelle preferenze
		p.drawRect( x, y + 1, w, h + 1 );
		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawRoundedRect( movingOffset.x() + x-2, movingOffset.y() + y+1-2, w+4, h+1+4, 4, 4 );
		}

	}
}




void DrumPatternEditor::__draw_grid( QPainter& p )
{
	// Start with generic pattern editor grid lining.
	drawGridLines( p );

	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	Song *pSong = Hydrogen::get_instance()->getSong();
	int nNotes;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	} else {
		nNotes = pSong->getDefaultPatternSize();
	}
	
	// fill the first half of the rect with a solid color
	static const QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	static const QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	int nInstruments = pSong->getInstrumentList()->size();
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + 1;
		if ( i == (uint)nSelectedInstrument ) {
			p.fillRect( 0, y, (m_nMargin + nNotes * m_nGridWidth), (int)( m_nGridHeight * 0.7 ), selectedRowColor );
		}
		else {
			p.fillRect( 0, y, (m_nMargin + nNotes * m_nGridWidth), (int)( m_nGridHeight * 0.7 ), backgroundColor );
		}
	}

}


void DrumPatternEditor::__create_background( QPainter& p)
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	static const QColor alternateRowColor( pStyle->m_patternEditor_alternateRowColor.getRed(), pStyle->m_patternEditor_alternateRowColor.getGreen(), pStyle->m_patternEditor_alternateRowColor.getBlue() );
	static const QColor lineColor( pStyle->m_patternEditor_lineColor.getRed(), pStyle->m_patternEditor_lineColor.getGreen(), pStyle->m_patternEditor_lineColor.getBlue() );

	Song *pSong = Hydrogen::get_instance()->getSong();
	int nNotes;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	} else {
		nNotes = pSong->getDefaultPatternSize();
	}

	int nInstruments = pSong->getInstrumentList()->size();

	if ( m_nEditorHeight != (int)( m_nGridHeight * nInstruments ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * nInstruments;
		resize( width(), m_nEditorHeight );
	}

	p.fillRect(0, 0, m_nMargin + nNotes * m_nGridWidth, height(), backgroundColor);
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i;
		if ( ( i % 2) != 0) {
			p.fillRect( 0, y, (m_nMargin + nNotes * m_nGridWidth), m_nGridHeight, alternateRowColor );
		}
	}

	// horizontal lines
	p.setPen( lineColor );
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + m_nGridHeight;
		p.drawLine( 0, y, (m_nMargin + nNotes * m_nGridWidth), y);
	}

	p.drawLine( 0, m_nEditorHeight, (m_nMargin + nNotes * m_nGridWidth), m_nEditorHeight );
}



void DrumPatternEditor::paintEvent( QPaintEvent* /*ev*/ )
{
	//INFOLOG( "paint" );
	//QWidget::paintEvent(ev);

	QPainter painter( this );
	__draw_pattern( painter );
	m_selection.paintSelection( &painter );
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



void DrumPatternEditor::focusInEvent ( QFocusEvent *ev )
{
	UNUSED( ev );
	if ( ev->reason() == Qt::TabFocusReason || ev->reason() == Qt::BacktabFocusReason ) {
		m_pPatternEditorPanel->ensureCursorVisible();
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
	}
	updateEditor();
}

void DrumPatternEditor::selectedInstrumentChangedEvent()
{
	update( 0, 0, width(), height() );
}


/// This method is called from another thread (audio engine)
void DrumPatternEditor::patternModifiedEvent()
{
	update( 0, 0, width(), height() );
}


void DrumPatternEditor::patternChangedEvent()
{
	updateEditor();
}


void DrumPatternEditor::selectedPatternChangedEvent()
{
	updateEditor();
}


///NotePropertiesRuler undo redo action
void DrumPatternEditor::undoRedoAction( int column,
					QString mode,
					int nSelectedPatternNumber,
					int nSelectedInstrument,
					float velocity,
					float pan_L,
					float pan_R,
					float leadLag,
					float probability,
					int noteKeyVal,
					int octaveKeyVal)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	Pattern *pPattern = nullptr;
	PatternList *pPatternList = pEngine->getSong()->getPatternList();

	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( nSelectedPatternNumber );
	}

	if(pPattern) {
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BOUND(notes,it,column) {
			Note *pNote = it->second;
			assert( pNote );
			assert( (int)pNote->get_position() == column );
			if ( pNote->get_instrument() != pSong->getInstrumentList()->get( nSelectedInstrument ) ) {
				continue;
			}

			if ( mode == "VELOCITY" && !pNote->get_note_off() ) {
				pNote->set_velocity( velocity );
			}
			else if ( mode == "PAN" ){

				pNote->set_pan_l( pan_L );
				pNote->set_pan_r( pan_R );
			}
			else if ( mode == "LEADLAG" ){
				pNote->set_lead_lag( leadLag );
			}
			else if ( mode == "NOTEKEY" ){
				pNote->set_key_octave( (Note::Key)noteKeyVal, (Note::Octave)octaveKeyVal );
			}
			else if ( mode == "PROBABILITY" ){
				pNote->set_probability( probability );
			}

			pSong->setIsModified( true );
			break;
		}

		m_pPatternEditorPanel->updateEditors();
	}
}


///==========================================================
///undo / redo actions from pattern editor instrument list

void DrumPatternEditor::functionClearNotesRedoAction( int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );

	Instrument *pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );

	pPattern->purge_instrument( pSelectedInstrument );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}



void DrumPatternEditor::functionClearNotesUndoAction( std::list< H2Core::Note* > noteList, int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = H->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );

	std::list < H2Core::Note *>::const_iterator pos;
	for ( pos = noteList.begin(); pos != noteList.end(); ++pos){
		Note *pNote;
		pNote = new Note(*pos);
		assert( pNote );
		pPattern->insert_note( pNote );
	}
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesUndoAction(std::list<H2Core::Pattern*> & appliedList)
{
	// Get song's pattern list
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *patternList = H->getSong()->getPatternList();

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

	while (appliedList.size() > 0)
	{
		// Get next applied pattern
		Pattern *pApplied = appliedList.front();
		assert(pApplied);

		// Find destination pattern to perform undo
		Pattern *pat = patternList->find(pApplied->get_name());

		if (pat != nullptr)
		{
			// Remove all notes of applied pattern from destination pattern
			const Pattern::notes_t* notes = pApplied->get_notes();
			FOREACH_NOTE_CST_IT_BEGIN_END(notes, it)
			{
				// Get note to remove
				Note *pNote = it->second;
				assert(pNote);

				// Check if note is not present
				Pattern::notes_t* notes = (Pattern::notes_t *)pat->get_notes();
				FOREACH_NOTE_IT_BOUND(notes, it, pNote->get_position())
				{
					Note *pFoundNote = it->second;
					if (pFoundNote->get_instrument() == pNote->get_instrument())
					{
						notes->erase(it);
						delete pFoundNote;
						break;
					}
				}
			}
		}

		// Remove applied pattern;
		delete pApplied;
		appliedList.pop_front();
	}

	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	// Update editors
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesRedoAction(std::list<H2Core::Pattern*> & changeList, std::list<H2Core::Pattern*> & appliedList)
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *patternList = H->getSong()->getPatternList();

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

	// Add notes to pattern
	std::list < H2Core::Pattern *>::iterator pos;
	for ( pos = changeList.begin(); pos != changeList.end(); ++pos)
	{
		Pattern *pPattern = *pos;
		assert(pPattern);

		Pattern *pat = patternList->find(pPattern->get_name()); // Destination pattern

		if (pat != nullptr)
		{
			// Create applied pattern
			Pattern *pApplied = new Pattern(
					pat->get_name(),
					pat->get_info(),
					pat->get_category(),
					pat->get_length());

			// Add all notes of source pattern to destination pattern
			// and store all applied notes in applied pattern
			const Pattern::notes_t* notes = pPattern->get_notes();
			FOREACH_NOTE_CST_IT_BEGIN_END(notes, it)
			{
				Note *pNote = it->second;
				assert(pNote);

				// Check if note is not present
				bool noteExists = false;
				const Pattern::notes_t* notes = pat->get_notes();
				FOREACH_NOTE_CST_IT_BOUND(notes, it, pNote->get_position())
				{
					Note *pFoundNote = it->second;
					if (pFoundNote->get_instrument() == pNote->get_instrument())
					{
						// note already exists
						noteExists = true;
						break;
					}
				}

				// Apply note and store it as applied
				if (!noteExists)
				{
					pat->insert_note(new Note(pNote));
					pApplied->insert_note(new Note(pNote));
				}
			}

			// Add applied pattern to applied list
			appliedList.push_back(pApplied);
		}
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	// Update editors
	m_pPatternEditorPanel->updateEditors();
}



void DrumPatternEditor::functionFillNotesUndoAction( QStringList noteList, int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );
	Instrument *pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

	for (int i = 0; i < noteList.size(); i++ ) {
		int nColumn  = noteList.value(i).toInt();
		Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->get_notes();
		FOREACH_NOTE_IT_BOUND(notes,it,nColumn) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() == pSelectedInstrument ) {
				// the note exists...remove it!
				notes->erase( it );
				delete pNote;
				break;
			}
		}
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionFillNotesRedoAction( QStringList noteList, int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );
	Instrument *pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );

	const float velocity = 0.8f;
	const float pan_L = 0.5f;
	const float pan_R = 0.5f;
	const float fPitch = 0.0f;
	const int nLength = -1;

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine
	for (int i = 0; i < noteList.size(); i++ ) {

		// create the new note
		int position = noteList.value(i).toInt();
		Note *pNote = new Note( pSelectedInstrument, position, velocity, pan_L, pan_R, nLength, fPitch );
		pPattern->insert_note( pNote );
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionRandomVelocityAction( QStringList noteVeloValue, int nSelectedInstrument, int selectedPatternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( selectedPatternNumber );
	Instrument *pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );


	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

	int nResolution = granularity();
	int positionCount = 0;
	for (int i = 0; i < pPattern->get_length(); i += nResolution) {
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BOUND(notes,it,i) {
			Note *pNote = it->second;
			if ( pNote->get_instrument() ==  pSelectedInstrument) {
				float velocity = noteVeloValue.value( positionCount ).toFloat();
				pNote->set_velocity(velocity);
				positionCount++;
			}
		}
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionMoveInstrumentAction( int nSourceInstrument,  int nTargetInstrument )
{
		Hydrogen *engine = Hydrogen::get_instance();
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		Song *pSong = engine->getSong();
		InstrumentList *pInstrumentList = pSong->getInstrumentList();

		if ( ( nTargetInstrument > (int)pInstrumentList->size() ) || ( nTargetInstrument < 0) ) {
			AudioEngine::get_instance()->unlock();
			return;
		}

		pInstrumentList->move( nSourceInstrument, nTargetInstrument );

		#ifdef H2CORE_HAVE_JACK
		engine->renameJackPorts( pSong );
		#endif

		AudioEngine::get_instance()->unlock();
		engine->setSelectedInstrumentNumber( nTargetInstrument );

		pSong->setIsModified( true );
}


void  DrumPatternEditor::functionDropInstrumentUndoAction( int nTargetInstrument, std::vector<int>* AddedComponents )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->removeInstrument( nTargetInstrument, false );

	std::vector<DrumkitComponent*>* pDrumkitComponents = pHydrogen->getSong()->getComponents();

	for (std::vector<int>::iterator it = AddedComponents->begin() ; it != AddedComponents->end(); ++it) {
		int p_compoID = *it;

		for ( int n = 0 ; n < pDrumkitComponents->size() ; n++ ) {
			DrumkitComponent* pTmpDrumkitComponent = pDrumkitComponents->at( n );
			if( pTmpDrumkitComponent->get_id() == p_compoID ) {
				pDrumkitComponents->erase( pDrumkitComponents->begin() + n );
				break;
			}
		}
	}

	AudioEngine::get_instance()->lock( RIGHT_HERE );
#ifdef H2CORE_HAVE_JACK
	Song *pSong = pHydrogen->getSong();
	pHydrogen->renameJackPorts( pSong );
#endif
	AudioEngine::get_instance()->unlock();
	updateEditor();
}


void  DrumPatternEditor::functionDropInstrumentRedoAction( QString sDrumkitName, QString sInstrumentName, int nTargetInstrument, std::vector<int>* AddedComponents)
{
		Instrument *pNewInstrument = Instrument::load_instrument( sDrumkitName, sInstrumentName );
		if( pNewInstrument == nullptr ){
			return;
		}

		Drumkit *pNewDrumkit = Drumkit::load_by_name( sDrumkitName, false );
		if( pNewDrumkit == nullptr ){
			return;
		}

		Hydrogen *pEngine = Hydrogen::get_instance();

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		std::vector<InstrumentComponent*>* pOldInstrumentComponents = new std::vector<InstrumentComponent*> ( pNewInstrument->get_components()->begin(), pNewInstrument->get_components()->end() );
		pNewInstrument->get_components()->clear();

		for (std::vector<DrumkitComponent*>::iterator it = pNewDrumkit->get_components()->begin() ; it != pNewDrumkit->get_components()->end(); ++it) {
			DrumkitComponent* pComponent = *it;
			int OldID = pComponent->get_id();
			int NewID = -1;

			NewID = findExistingCompo( pComponent->get_name() );

			if ( NewID == -1 ) {
				NewID = findFreeCompoID();

				AddedComponents->push_back( NewID );

				pComponent->set_id( NewID );
				pComponent->set_name( renameCompo( pComponent->get_name() ) );
				Hydrogen::get_instance()->getSong()->getComponents()->push_back( pComponent );
			}

			for ( std::vector<InstrumentComponent*>::iterator it2 = pOldInstrumentComponents->begin() ; it2 != pOldInstrumentComponents->end(); ++it2 ) {
				InstrumentComponent* pOldInstrCompo = *it2;
				if( pOldInstrCompo->get_drumkit_componentID() == OldID ) {
					InstrumentComponent* pNewInstrCompo = new InstrumentComponent( pOldInstrCompo );
					pNewInstrCompo->set_drumkit_componentID( NewID );

					pNewInstrument->get_components()->push_back( pNewInstrCompo );
				}
			}
		}

		pOldInstrumentComponents->clear();
		delete pOldInstrumentComponents;

		// create a new valid ID for this instrument
		int nID = -1;
		for ( uint i = 0; i < pEngine->getSong()->getInstrumentList()->size(); ++i ) {
			Instrument* pInstr = pEngine->getSong()->getInstrumentList()->get( i );
			if ( pInstr->get_id() > nID ) {
				nID = pInstr->get_id();
			}
		}
		++nID;

		pNewInstrument->set_id( nID );

		pEngine->getSong()->getInstrumentList()->add( pNewInstrument );

		#ifdef H2CORE_HAVE_JACK
		pEngine->renameJackPorts( pEngine->getSong() );
		#endif

		AudioEngine::get_instance()->unlock();
		//move instrument to the position where it was dropped
		functionMoveInstrumentAction(pEngine->getSong()->getInstrumentList()->size() - 1 , nTargetInstrument );

		// select the new instrument
		pEngine->setSelectedInstrumentNumber(nTargetInstrument);
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
		updateEditor();
}

QString DrumPatternEditor::renameCompo( QString OriginalName )
{
	std::vector<DrumkitComponent*>* pComponentList = Hydrogen::get_instance()->getSong()->getComponents();
	for (std::vector<DrumkitComponent*>::iterator it = pComponentList->begin() ; it != pComponentList->end(); ++it) {
		DrumkitComponent* pComponent = *it;
		if( pComponent->get_name().compare( OriginalName ) == 0 ){
			return renameCompo( OriginalName + "_new" );
		}
	}
	return OriginalName;
}

int DrumPatternEditor::findFreeCompoID( int startingPoint )
{
	bool FoundFreeSlot = true;
	std::vector<DrumkitComponent*>* pComponentList = Hydrogen::get_instance()->getSong()->getComponents();
	for (std::vector<DrumkitComponent*>::iterator it = pComponentList->begin() ; it != pComponentList->end(); ++it) {
		DrumkitComponent* pComponent = *it;
		if( pComponent->get_id() == startingPoint ) {
			FoundFreeSlot = false;
			break;
		}
	}

	if(FoundFreeSlot){
		return startingPoint;
	} else {
		return findFreeCompoID( startingPoint + 1 );
	}
}

int DrumPatternEditor::findExistingCompo( QString SourceName )
{
	std::vector<DrumkitComponent*>* pComponentList = Hydrogen::get_instance()->getSong()->getComponents();
	for (std::vector<DrumkitComponent*>::iterator it = pComponentList->begin() ; it != pComponentList->end(); ++it) {
		DrumkitComponent* pComponent = *it;
		if ( pComponent->get_name().compare( SourceName ) == 0 ){
			return pComponent->get_id();
		}
	}
	return -1;
}



void DrumPatternEditor::functionDeleteInstrumentUndoAction( std::list< H2Core::Note* > noteList, int nSelectedInstrument, QString sInstrumentName, QString sDrumkitName )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Instrument *pNewInstrument;
	if( sDrumkitName == "" ){
		pNewInstrument = new Instrument( pHydrogen->getSong()->getInstrumentList()->size() -1, sInstrumentName );
	} else {
		pNewInstrument = Instrument::load_instrument( sDrumkitName, sInstrumentName );
	}
	if( pNewInstrument == nullptr ) {
		return;
	}

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pHydrogen->getSong()->getInstrumentList()->size(); ++i ) {
		Instrument* pInstr = pHydrogen->getSong()->getInstrumentList()->get( i );
		if ( pInstr->get_id() > nID ) {
			nID = pInstr->get_id();
		}
	}
	++nID;

	pNewInstrument->set_id( nID );
//	pNewInstrument->set_adsr( new ADSR( 0, 0, 1.0, 1000 ) );

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	pHydrogen->getSong()->getInstrumentList()->add( pNewInstrument );

	#ifdef H2CORE_HAVE_JACK
	pHydrogen->renameJackPorts( pHydrogen->getSong() );
	#endif

	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	//move instrument to the position where it was dropped
	functionMoveInstrumentAction(pHydrogen->getSong()->getInstrumentList()->size() - 1 , nSelectedInstrument );

	// select the new instrument
	pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument );

	H2Core::Pattern *pPattern;
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();

	updateEditor();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	//restore all deleted instrument notes
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	if(noteList.size() > 0 ){
		std::list < H2Core::Note *>::const_iterator pos;
		for ( pos = noteList.begin(); pos != noteList.end(); ++pos){
			Note *pNote = new Note( *pos, pNewInstrument );
			assert( pNote );
			pPattern = pPatternList->get( pNote->get_pattern_idx() );
			assert (pPattern);
			pPattern->insert_note( pNote );
			//delete pNote;
		}
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine
}

void DrumPatternEditor::functionAddEmptyInstrumentUndo()
{

	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->removeInstrument( pEngine->getSong()->getInstrumentList()->size() -1 , false );

	AudioEngine::get_instance()->lock( RIGHT_HERE );
#ifdef H2CORE_HAVE_JACK
	pEngine->renameJackPorts( pEngine->getSong() );
#endif
	AudioEngine::get_instance()->unlock();
	updateEditor();
}


void DrumPatternEditor::functionAddEmptyInstrumentRedo()
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	Song* pSong = Hydrogen::get_instance()->getSong();
	InstrumentList* pList = pSong->getInstrumentList();

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pList->size(); ++i ) {
		Instrument* pInstr = pList->get( i );
		if ( pInstr->get_id() > nID ) {
			nID = pInstr->get_id();
		}
	}
	++nID;

	Instrument *pNewInstr = new Instrument( nID, "New instrument");
	pList->add( pNewInstr );

	#ifdef H2CORE_HAVE_JACK
	Hydrogen::get_instance()->renameJackPorts( pSong );
	#endif

	AudioEngine::get_instance()->unlock();

	Hydrogen::get_instance()->setSelectedInstrumentNumber( pList->size() - 1 );

}
///~undo / redo actions from pattern editor instrument list
///==========================================================
