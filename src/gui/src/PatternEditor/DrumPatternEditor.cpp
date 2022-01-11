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

#include "DrumPatternEditor.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"

#include <core/Globals.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/EventQueue.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Xml.h>

#include "UndoActions.h"
#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"

#include <math.h>
#include <cassert>
#include <algorithm>
#include <stack>

using namespace H2Core;

DrumPatternEditor::DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel)
 : PatternEditor( parent, panel )
{
	auto pPref = H2Core::Preferences::get_instance();

	m_nGridHeight = pPref->getPatternEditorGridHeight();
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;
	resize( m_nEditorWidth, m_nEditorHeight );

	Hydrogen::get_instance()->setSelectedInstrumentNumber( 0 );
}

DrumPatternEditor::~DrumPatternEditor()
{
}



void DrumPatternEditor::updateEditor( bool bPatternOnly )
{
	auto pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	if ( pAudioEngine->getState() != H2Core::AudioEngine::State::Ready &&
		 pAudioEngine->getState() != H2Core::AudioEngine::State::Playing ) {
		ERRORLOG( "FIXME: skipping pattern editor update (state should be READY or PLAYING)" );
		return;
	}

	updatePatternInfo();

	if ( m_pPattern ) {
		m_nEditorWidth = m_nMargin + m_fGridWidth * m_pPattern->get_length();
	}
	else {
		m_nEditorWidth = m_nMargin + m_fGridWidth * MAX_NOTES;
	}
	resize( m_nEditorWidth, height() );

	// redraw all
	update( 0, 0, width(), height() );
}


void DrumPatternEditor::addOrRemoveNote( int nColumn, int nRealColumn, int row,
										 bool bDoAdd, bool bDoDelete ) {
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( row );
	H2Core::Note *pOldNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument );

	int oldLength = -1;
	float oldVelocity = 0.8f;
	float fOldPan = 0.f;
	float oldLeadLag = 0.0f;
	float fProbability = 1.0f;
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
		fOldPan = pOldNote->getPan();
		oldLeadLag = pOldNote->get_lead_lag();
		oldNoteKeyVal = pOldNote->get_key();
		oldOctaveKeyVal = pOldNote->get_octave();
		isNoteOff = pOldNote->get_note_off();
		fProbability = pOldNote->get_probability();
	}

	SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( nColumn,
																	 row,
																	 m_nSelectedPatternNumber,
																	 oldLength,
																	 oldVelocity,
																	 fOldPan,
																	 oldLeadLag,
																	 oldNoteKeyVal,
																	 oldOctaveKeyVal,
																	 fProbability,
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	int nInstruments = pSong->getInstrumentList()->size();
	int row = (int)( ev->y()  / (float)m_nGridHeight);
	if (row >= nInstruments) {
		return;
	}
	int nColumn = getColumn( ev->x(), /* bUseFineGrained=*/ true );
	int nRealColumn = 0;

	if( ev->x() > m_nMargin ) {
		nRealColumn = ( ev->x() - m_nMargin) / static_cast<float>(m_fGridWidth);
	}

	if ( nColumn >= (int)m_pPattern->get_length() ) {
		update( 0, 0, width(), height() );
		return;
	}
	auto pSelectedInstrument = pSong->getInstrumentList()->get( row );

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
																			 pNote->getPan(),
																			 pNote->get_lead_lag(),
																			 pNote->get_key(),
																			 pNote->get_octave(),
																			 pNote->get_probability(),
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	int nColumn = getColumn( ev->x() );
	if ( ev->button() == Qt::RightButton ) {
		// Right button drag: adjust note length
		int nRealColumn = 0;
		auto pSelectedInstrument = pSong->getInstrumentList()->get( row );

		if( ev->x() > m_nMargin ) {
			nRealColumn = ( ev->x() - m_nMargin) / static_cast<float>(m_fGridWidth);
		}

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
												float fOldPan,
												float oldLeadLag,
												int oldNoteKeyVal,
												int oldOctaveKeyVal,
												float fProbability,
												bool listen,
												bool isMidi,
												bool isInstrumentMode, // TODO not used arg
												bool isNoteOff,
												bool isDelete )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	H2Core::Pattern *pPattern = nullptr;

	if ( ( selectedPatternNumber != -1 ) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}

	assert(pPattern);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	auto pSelectedInstrument = pSong->getInstrumentList()->get( row );

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine


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
					  && pNote->get_velocity() == oldVelocity
					  && pNote->get_probability() == fProbability ) ) {
				notes->erase( it );
				delete pNote;
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
		float fPan = fOldPan ;
		int nLength = oldLength;


		if ( isNoteOff ) {
			fVelocity = 0.0f;
			fPan = 0.f;
			nLength = 1;
			fProbability = 1.0;
		}

		float fPitch = 0.f;
		
		Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan, nLength, fPitch );
		pNote->set_note_off( isNoteOff );
		if ( !isNoteOff ) {
			pNote->set_lead_lag( oldLeadLag );
			pNote->set_probability( fProbability );
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
			Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan, nLength, fPitch);
			m_pAudioEngine->getSampler()->noteOn(pNote2);
		}
	}
	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock(); // unlock the audio engine

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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	m_pAudioEngine->lock( RIGHT_HERE );
	PatternList *pPatternList = pSong->getPatternList();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();
	Pattern *pPattern = m_pPattern;
	Note *pFoundNote = nullptr;

	if ( nPattern < 0 || nPattern > pPatternList->size() ) {
		ERRORLOG( "Invalid pattern number" );
		m_pAudioEngine->unlock();
		return;
	}

	auto pFromInstrument = pInstrumentList->get( nRow );
	auto pToInstrument = pInstrumentList->get( nNewRow );

	FOREACH_NOTE_IT_BOUND((Pattern::notes_t *)pPattern->get_notes(), it, nColumn) {
		Note *pCandidateNote = it->second;
		if ( pCandidateNote->get_instrument() == pFromInstrument
			 && pCandidateNote->get_key() == pNote->get_key()
			 && pCandidateNote->get_octave() == pNote->get_octave()
			 && pCandidateNote->get_velocity() == pNote->get_velocity()
			 && pCandidateNote->get_lead_lag() == pNote->get_lead_lag()
			 && pCandidateNote->getPan() == pNote->getPan()
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
		m_pAudioEngine->unlock();
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
			m_selection.removeFromSelection( pFoundNote, /* bCheck=*/false  );
			m_selection.addToSelection( pNewNote );
		}
		pNewNote->set_position( nNewColumn );
		m_selection.addToSelection( pNewNote );
		pPattern->insert_note( pNewNote );
		delete pFoundNote;
	}

	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock();

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
														   pNote->getPan(),
														   pNote->get_lead_lag(),
														   pNote->get_key(),
														   pNote->get_octave(),
														   pNote->get_probability(),
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
														   pNote->getPan(),
														   pNote->get_lead_lag(),
														   pNote->get_key(),
														   pNote->get_octave(),
														   pNote->get_probability(),
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();

	H2Core::Pattern *pPattern = nullptr;
	if ( (selectedPatternNumber != -1) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}

	if( pPattern ) {
		Note *pDraggedNote = nullptr;
		std::shared_ptr<Song> pSong = pHydrogen->getSong();
		auto pSelectedInstrument = pSong->getInstrumentList()->get( row );

		m_pAudioEngine->lock( RIGHT_HERE );

		pDraggedNote = pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, false );
		if( pDraggedNote ){
			pDraggedNote->set_length( length );
		}

		pHydrogen->setIsModified( true );
		m_pAudioEngine->unlock();

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

		m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->get_position();

		if (nLen <= 0) {
			nLen = -1;
		}

		float fNotePitch = m_pDraggedNote->get_octave() * 12 + m_pDraggedNote->get_key();
		float fStep = 0;
		if(nLen > -1){
			fStep = Note::pitchToFrequency( ( double )fNotePitch );
		}else
		{
			fStep = 1.0;
		}
		m_pDraggedNote->set_length( nLen * fStep);

		Hydrogen::get_instance()->setIsModified( true );
		m_pAudioEngine->unlock(); // unlock the audio engine

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
	const int nBlockSize = 5, nWordSize = 5;
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
		m_pPatternEditorPanel->setCursorPosition( 0 );

	} else if ( ev->matches( QKeySequence::MoveToNextLine ) || ev->matches( QKeySequence::SelectNextLine ) ) {
		if ( nSelectedInstrument + 1 < nMaxInstrument ) {
			pH2->setSelectedInstrumentNumber( nSelectedInstrument + 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) || ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		pH2->setSelectedInstrumentNumber( std::min( nSelectedInstrument + nBlockSize,
													nMaxInstrument-1 ) );

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
	} else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) || ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		pH2->setSelectedInstrumentNumber( std::max( nSelectedInstrument - nBlockSize, 0 ) );

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

void DrumPatternEditor::keyReleaseEvent( QKeyEvent *ev ) {
	updateModifiers( ev );
}



///
/// Find all elements which intersect a selection area.
///
std::vector<DrumPatternEditor::SelectionIndex> DrumPatternEditor::elementsIntersecting( QRect r )
{
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
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
	int x_min = (r.left() - m_nMargin - 1) / m_fGridWidth;
	int x_max = (r.right() - m_nMargin) / m_fGridWidth;

	const Pattern::notes_t* notes = m_pPattern->get_notes();
	std::vector<SelectionIndex> result;

	for (auto it = notes->lower_bound( x_min ); it != notes->end() && it->first <= x_max; ++it ) {
		Note *note = it->second;
		int nInstrument = pInstrList->index( note->get_instrument() );
		uint x_pos = m_nMargin + (it->first * m_fGridWidth);
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

	uint x = m_nMargin + m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	uint y = nSelectedInstrument * m_nGridHeight;
	return QRect( x-m_fGridWidth*3, y+2, m_fGridWidth*6, m_nGridHeight-3 );

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
																 pNote->getPan(),
																 pNote->get_lead_lag(),
																 pNote->get_key(),
																 pNote->get_octave(),
																 pNote->get_probability(),
																 true, // noteExisted
																 false, // listen
																 false,
																 false,
																 pNote->get_note_off() ) );
			}
		}
		m_selection.clearSelection();

		pUndo->beginMacro("delete notes");
		for ( QUndoCommand *pAction : actions ) {
			pUndo->push( pAction );
		}
		pUndo->endMacro();
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
														   pNote->getPan(),
														   pNote->get_lead_lag(),
														   pNote->get_key(),
														   pNote->get_octave(),
														   pNote->get_probability(),
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
	auto pPref = H2Core::Preferences::get_instance();
	
	const QColor selectedRowColor( pPref->getColorTheme()->m_patternEditor_selectedRowColor );

	__create_background( painter );

	if (m_pPattern == nullptr) {
		return;
	}

	int nNotes = m_pPattern->get_length();
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

	InstrumentList * pInstrList = pSong->getInstrumentList();


	if ( m_nEditorHeight != (int)( m_nGridHeight * pInstrList->size() ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * pInstrList->size();
		resize( width(), m_nEditorHeight );
	}

	for ( uint nInstr = 0; nInstr < pInstrList->size(); ++nInstr ) {
		uint y = m_nGridHeight * nInstr;
		if ( nInstr == (uint)nSelectedInstrument ) {	// selected instrument
			painter.fillRect( 0, y + 1, ( m_nMargin + nNotes * m_fGridWidth ), m_nGridHeight - 1, selectedRowColor );
		}
	}


	// draw the grid
	__draw_grid( painter );


	// Draw cursor
	if ( hasFocus() && !HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		uint x = m_nMargin + m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		uint y = nSelectedInstrument * m_nGridHeight;
		QPen p( Qt::black );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( QRect( x-m_fGridWidth*3, y+2, m_fGridWidth*6, m_nGridHeight-3 ), 4, 4 );
	}


	/*
		BUGFIX

		if m_pPattern is not renewed every time we draw a note,
		hydrogen will crash after you save a song and create a new one.
		-smoors
	*/
	updatePatternInfo();
	validateSelection();


	for ( Pattern *pPattern : getPatternsToShow() ) {
		const Pattern::notes_t *pNotes = pPattern->get_notes();
		if ( pNotes->size() == 0 ) {
			continue;
		}
		bool bIsForeground = ( pPattern == m_pPattern );

		std::vector< int > noteCount; // instrument_id -> count
		std::stack<std::shared_ptr<Instrument>> instruments;

		// Process notes in batches by note position, counting the notes at each instrument so we can display
		// markers for instruments which have more than one note in the same position (a chord or genuine
		// duplicates)
		for ( auto posIt = pNotes->begin(); posIt != pNotes->end(); ) {
			int nPosition = posIt->second->get_position();

			// Process all notes at this position
			auto noteIt = posIt;
			while ( noteIt != pNotes->end() && noteIt->second->get_position() == nPosition ) {
				Note *pNote = noteIt->second;

				int nInstrumentID = pNote->get_instrument_id();
				if ( nInstrumentID >= noteCount.size() ) {
					noteCount.resize( nInstrumentID+1, 0 );
				}

				if ( ++noteCount[ nInstrumentID ] == 1) {
					instruments.push( pNote->get_instrument() );
				}

				__draw_note( pNote, painter, bIsForeground );
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
					int x = m_nMargin + (nPosition * m_fGridWidth);
					int y = ( nInstrument * m_nGridHeight);
					const int boxWidth = 128;

					QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
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
void DrumPatternEditor::__draw_note( Note *note, QPainter& p, bool bIsForeground )
{
	InstrumentList *pInstrList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	int nInstrument = pInstrList->index( note->get_instrument() );
	if ( nInstrument == -1 ) {
		ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	QPoint pos ( m_nMargin + note->get_position() * m_fGridWidth,
				 ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3 );

	drawNoteSymbol( p, pos, note, bIsForeground );
}




void DrumPatternEditor::__draw_grid( QPainter& p )
{
	
	auto pPref = H2Core::Preferences::get_instance();
	
	// Start with generic pattern editor grid lining.
	drawGridLines( p );

	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}
	
	// fill the first half of the rect with a solid color
	const QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );
	const QColor selectedRowColor( pPref->getColorTheme()->m_patternEditor_selectedRowColor );
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	int nInstruments = pSong->getInstrumentList()->size();
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + 1;
		if ( i == (uint)nSelectedInstrument ) {
			p.fillRect( 0, y, (m_nMargin + nNotes * m_fGridWidth), (int)( m_nGridHeight * 0.7 ), selectedRowColor );
		}
		else {
			p.fillRect( 0, y, (m_nMargin + nNotes * m_fGridWidth), (int)( m_nGridHeight * 0.7 ), backgroundColor );
		}
	}

}


void DrumPatternEditor::__create_background( QPainter& p)
{
	
	auto pPref = H2Core::Preferences::get_instance();
	
	const QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );
	const QColor alternateRowColor( pPref->getColorTheme()->m_patternEditor_alternateRowColor );
	const QColor lineColor( pPref->getColorTheme()->m_patternEditor_lineColor );

	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	int nInstruments = pSong->getInstrumentList()->size();

	if ( m_nEditorHeight != (int)( m_nGridHeight * nInstruments ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * nInstruments;
		resize( width(), m_nEditorHeight );
	}

	p.fillRect(0, 0, m_nMargin + nNotes * m_fGridWidth, height(), backgroundColor);
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i;
		if ( ( i % 2) != 0) {
			p.fillRect( 0, y, (m_nMargin + nNotes * m_fGridWidth), m_nGridHeight, alternateRowColor );
		}
	}

	// horizontal lines
	p.setPen( lineColor );
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + m_nGridHeight;
		p.drawLine( 0, y, (m_nMargin + nNotes * m_fGridWidth), y);
	}

	p.drawLine( 0, m_nEditorHeight, (m_nMargin + nNotes * m_fGridWidth), m_nEditorHeight );
}



void DrumPatternEditor::paintEvent( QPaintEvent* /*ev*/ )
{
	QPainter painter( this );
	__draw_pattern( painter );

	drawFocus( painter );
	
	m_selection.paintSelection( &painter );
}

void DrumPatternEditor::drawFocus( QPainter& painter ) {

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

	int nStartY = HydrogenApp::get_instance()->getPatternEditorPanel()->getVerticalScrollBar()->value();
	int nStartX = HydrogenApp::get_instance()->getPatternEditorPanel()->getHorizontalScrollBar()->value();
	int nEndY = std::min( static_cast<int>( m_nGridHeight ) * Hydrogen::get_instance()->getSong()->getInstrumentList()->size(),
						 nStartY + HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditorScrollArea()->viewport()->size().height() );
	int nEndX = std::min( nStartX + HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditorScrollArea()->viewport()->size().width(), width() );

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
					float fPan,
					float leadLag,
					float probability,
					int noteKeyVal,
					int octaveKeyVal)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	Pattern *pPattern = nullptr;
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();

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
				pNote->setPan( fPan );
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

			pHydrogen->setIsModified( true );
			break;
		}

		m_pPatternEditorPanel->updateEditors();
	}
}

void DrumPatternEditor::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateEditor();
	}
}


///==========================================================
///undo / redo actions from pattern editor instrument list

void DrumPatternEditor::functionClearNotesRedoAction( int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );

	auto pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );

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

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

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

	m_pAudioEngine->unlock();	// unlock the audio engine

	// Update editors
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesRedoAction(std::list<H2Core::Pattern*> & changeList, std::list<H2Core::Pattern*> & appliedList)
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *patternList = H->getSong()->getPatternList();

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

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
	m_pAudioEngine->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	// Update editors
	m_pPatternEditorPanel->updateEditors();
}



void DrumPatternEditor::functionFillNotesUndoAction( QStringList noteList, int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );
	auto pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

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
	m_pAudioEngine->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionFillNotesRedoAction( QStringList noteList, int nSelectedInstrument, int patternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( patternNumber );
	auto pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );

	const float velocity = 0.8f;
	const float fPan = 0.f;
	const float fPitch = 0.0f;
	const int nLength = -1;

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine
	for (int i = 0; i < noteList.size(); i++ ) {

		// create the new note
		int position = noteList.value(i).toInt();
		Note *pNote = new Note( pSelectedInstrument, position, velocity, fPan, nLength, fPitch );
		pPattern->insert_note( pNote );
	}
	m_pAudioEngine->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionRandomVelocityAction( QStringList noteVeloValue, int nSelectedInstrument, int selectedPatternNumber )
{
	Hydrogen * H = Hydrogen::get_instance();
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern *pPattern = pPatternList->get( selectedPatternNumber );
	auto pSelectedInstrument = H->getSong()->getInstrumentList()->get( nSelectedInstrument );


	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

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
	H->setIsModified( true );
	m_pAudioEngine->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionMoveInstrumentAction( int nSourceInstrument,  int nTargetInstrument )
{
		auto pHydrogen = Hydrogen::get_instance();
		m_pAudioEngine->lock( RIGHT_HERE );

		std::shared_ptr<Song> pSong = pHydrogen->getSong();
		InstrumentList *pInstrumentList = pSong->getInstrumentList();

		if ( ( nTargetInstrument > (int)pInstrumentList->size() ) || ( nTargetInstrument < 0) ) {
			m_pAudioEngine->unlock();
			return;
		}

		pInstrumentList->move( nSourceInstrument, nTargetInstrument );

		#ifdef H2CORE_HAVE_JACK
		pHydrogen->renameJackPorts( pSong );
		#endif

		m_pAudioEngine->unlock();
		pHydrogen->setSelectedInstrumentNumber( nTargetInstrument );

		pHydrogen->setIsModified( true );
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

	m_pAudioEngine->lock( RIGHT_HERE );
#ifdef H2CORE_HAVE_JACK
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pHydrogen->renameJackPorts( pSong );
#endif
	m_pAudioEngine->unlock();
	updateEditor();
}


void  DrumPatternEditor::functionDropInstrumentRedoAction( QString sDrumkitName, QString sInstrumentName, int nTargetInstrument, std::vector<int>* AddedComponents, Filesystem::Lookup lookup)
{
	auto pNewInstrument = Instrument::load_instrument( sDrumkitName, sInstrumentName, lookup );
		if( pNewInstrument->get_name() == "Empty Instrument" &&
			pNewInstrument->get_drumkit_name() == "" ){
			// Under normal circumstances this should not been reached.
			QMessageBox::critical( this, "Hydrogen", tr( "Unable to load instrument" ) );
			return;
		}

		Drumkit *pNewDrumkit = Drumkit::load_by_name( sDrumkitName, false, lookup );
		if( pNewDrumkit == nullptr ){
			return;
		}

		Hydrogen *pHydrogen = Hydrogen::get_instance();

		m_pAudioEngine->lock( RIGHT_HERE );

		auto pOldInstrumentComponents = new std::vector<std::shared_ptr<InstrumentComponent>>( pNewInstrument->get_components()->begin(), pNewInstrument->get_components()->end() );
		pNewInstrument->get_components()->clear();

		for ( auto pComponent : *(pNewDrumkit->get_components()) ) {
			int OldID = pComponent->get_id();
			int NewID = -1;

			// Gets the ID of the drumkit component registered to the
			// current song that matches the name of the pComponent.
			NewID = findExistingCompo( pComponent->get_name() );

			if ( NewID == -1 ) {
				// No component in the currently loaded drumkit found
				// matching pComponent.
				//
				// Get an ID not used as drumkit component ID by the
				// drumkit currently loaded.
				NewID = findFreeCompoID();

				AddedComponents->push_back( NewID );

				pComponent->set_id( NewID );
				pComponent->set_name( renameCompo( pComponent->get_name() ) );
				DrumkitComponent* pNewComponent = new DrumkitComponent( pComponent );
				Hydrogen::get_instance()->getSong()->getComponents()->push_back( pNewComponent );
			}

			for ( auto pOldInstrCompo : *pOldInstrumentComponents ) {
				if( pOldInstrCompo->get_drumkit_componentID() == OldID ) {
					auto pNewInstrCompo = std::make_shared<InstrumentComponent>( pOldInstrCompo );
					pNewInstrCompo->set_drumkit_componentID( NewID );

					pNewInstrument->get_components()->push_back( pNewInstrCompo );
				}
			}
		}
		
		pOldInstrumentComponents->clear();
		delete pOldInstrumentComponents;
		delete pNewDrumkit;
		
		// create a new valid ID for this instrument
		int nID = -1;
		for ( uint i = 0; i < pHydrogen->getSong()->getInstrumentList()->size(); ++i ) {
			auto pInstr = pHydrogen->getSong()->getInstrumentList()->get( i );
			if ( pInstr->get_id() > nID ) {
				nID = pInstr->get_id();
			}
		}
		++nID;

		pNewInstrument->set_id( nID );

		pHydrogen->getSong()->getInstrumentList()->add( pNewInstrument );

		#ifdef H2CORE_HAVE_JACK
		pHydrogen->renameJackPorts( pHydrogen->getSong() );
		#endif

		pHydrogen->setIsModified( true );
		m_pAudioEngine->unlock();
		//move instrument to the position where it was dropped
		functionMoveInstrumentAction(pHydrogen->getSong()->getInstrumentList()->size() - 1 , nTargetInstrument );

		// select the new instrument
		pHydrogen->setSelectedInstrumentNumber(nTargetInstrument);
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
	std::shared_ptr<Instrument> pNewInstrument;
	if( sDrumkitName == "" ){
		pNewInstrument = std::make_shared<Instrument>( pHydrogen->getSong()->getInstrumentList()->size() -1, sInstrumentName );
	} else {
		pNewInstrument = Instrument::load_instrument( sDrumkitName, sInstrumentName );
	}
	if( pNewInstrument == nullptr ) {
		return;
	}

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pHydrogen->getSong()->getInstrumentList()->size(); ++i ) {
		auto pInstr = pHydrogen->getSong()->getInstrumentList()->get( i );
		if ( pInstr->get_id() > nID ) {
			nID = pInstr->get_id();
		}
	}
	++nID;

	pNewInstrument->set_id( nID );

	m_pAudioEngine->lock( RIGHT_HERE );
	pHydrogen->getSong()->getInstrumentList()->add( pNewInstrument );

	#ifdef H2CORE_HAVE_JACK
	pHydrogen->renameJackPorts( pHydrogen->getSong() );
	#endif

	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock();	// unlock the audio engine

	//move instrument to the position where it was dropped
	functionMoveInstrumentAction(pHydrogen->getSong()->getInstrumentList()->size() - 1 , nSelectedInstrument );

	// select the new instrument
	pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument );

	H2Core::Pattern *pPattern;
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();

	updateEditor();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	//restore all deleted instrument notes
	m_pAudioEngine->lock( RIGHT_HERE );
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
	m_pAudioEngine->unlock();	// unlock the audio engine
}

void DrumPatternEditor::functionAddEmptyInstrumentUndo()
{

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->removeInstrument( pHydrogen->getSong()->getInstrumentList()->size() -1 , false );

	m_pAudioEngine->lock( RIGHT_HERE );
#ifdef H2CORE_HAVE_JACK
	pHydrogen->renameJackPorts( pHydrogen->getSong() );
#endif
	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock();
	updateEditor();
}


void DrumPatternEditor::functionAddEmptyInstrumentRedo()
{
	m_pAudioEngine->lock( RIGHT_HERE );
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	InstrumentList* pList = pSong->getInstrumentList();

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pList->size(); ++i ) {
		auto pInstr = pList->get( i );
		if ( pInstr->get_id() > nID ) {
			nID = pInstr->get_id();
		}
	}
	++nID;

	auto pNewInstr = std::make_shared<Instrument>( nID, "New instrument");
	pList->add( pNewInstr );

	#ifdef H2CORE_HAVE_JACK
	pHydrogen->renameJackPorts( pSong );
	#endif

	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock();

	pHydrogen->setSelectedInstrumentNumber( pList->size() - 1 );

}
///~undo / redo actions from pattern editor instrument list
///==========================================================
