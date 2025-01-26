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

DrumPatternEditor::DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel)
 : PatternEditor( parent, panel )
{
	m_editor = PatternEditor::Editor::DrumPattern;
	const auto pPref = H2Core::Preferences::get_instance();

	m_nGridHeight = pPref->getPatternEditorGridHeight();
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;
	m_nActiveWidth = m_nEditorWidth;
	resize( m_nEditorWidth, m_nEditorHeight );

	Hydrogen::get_instance()->setSelectedInstrumentNumber( 0 );
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

	updatePatternInfo();
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


void DrumPatternEditor::addOrRemoveNote( int nColumn, int nRealColumn, int nRow,
										 bool bDoAdd, bool bDoDelete,
										 bool bIsNoteOff ) {

	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}
	
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set" );
		return;
	}

	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( nRow );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nRow ) );
		return;
	}
	
	H2Core::Note *pOldNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument );

	int oldLength = -1;
	float oldVelocity = 0.8f;
	float fOldPan = 0.f;
	float oldLeadLag = 0.0f;
	float fProbability = 1.0f;
	Note::Key oldNoteKeyVal = Note::C;
	Note::Octave oldOctaveKeyVal = Note::P8;
	bool isNoteOff = bIsNoteOff;

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
																	 nRow,
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
	auto pHydrogenApp = HydrogenApp::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		return;
	}
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	int nInstruments = pSong->getDrumkit()->getInstruments()->size();
	int row = (int)( ev->y()  / (float)m_nGridHeight);
	if (row >= nInstruments) {
		return;
	}
	int nColumn = getColumn( ev->x(), /* bUseFineGrained=*/ true );
	int nRealColumn = 0;

	if( ev->x() > PatternEditor::nMargin ) {
		nRealColumn = ( ev->x() - PatternEditor::nMargin) / static_cast<float>(m_fGridWidth);
	}

	if ( nColumn >= (int)m_pPattern->get_length() ) {
		return;
	}
	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( row );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( row ) );
		return;
	}

	if ( ev->button() == Qt::LeftButton ) {

		// Pressing Shift causes the added note to be of NoteOff type.
		addOrRemoveNote( nColumn, nRealColumn, row, true, true,
						 ev->modifiers() & Qt::ShiftModifier );
		m_selection.clearSelection();

	} else if ( ev->button() == Qt::RightButton ) {

		showPopupMenu( ev->globalPos() );
	}

	m_pPatternEditorPanel->setCursorPosition( nColumn );

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
	auto pSong = pHydrogen->getSong();
	int nInstruments = pSong->getDrumkit()->getInstruments()->size();
	int nRow = static_cast<int>( ev->y() / static_cast<float>(m_nGridHeight) );
	if ( nRow >= nInstruments || nRow < 0 ) {
		return;
	}
	
	pHydrogen->setSelectedInstrumentNumber( nRow );

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

	// Update cursor position
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		int nColumn = getColumn( ev->x(), /* bUseFineGrained=*/ true );
		if ( ( m_pPattern != nullptr &&
			   nColumn >= (int)m_pPattern->get_length() ) ||
			 nColumn >= MAX_INSTRUMENTS ) {
			return;
		}

		pHydrogen->setSelectedInstrumentNumber( nRow );
		m_pPatternEditorPanel->setCursorPosition( nColumn );
	
		update();
		m_pPatternEditorPanel->getInstrumentList()->selectedInstrumentChangedEvent();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}
}
	

void DrumPatternEditor::mouseDragStartEvent( QMouseEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	// Set the selected instrument _before_ it will be stored in
	// PatternEditor::mouseDragStartEvent.
	int nRow = std::floor(static_cast<float>(ev->y()) /
						  static_cast<float>(m_nGridHeight));
	pHydrogen->setSelectedInstrumentNumber( nRow );
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();

	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nRow ) );
		return;
	}

	// Handles cursor repositioning and hiding and stores general
	// properties.
	PatternEditor::mouseDragStartEvent( ev );
	
	int nColumn = getColumn( ev->x() );
	
	if ( ev->button() == Qt::RightButton ) {
		// Right button drag: adjust note length
		int nRealColumn = 0;

		if( ev->x() > PatternEditor::nMargin ) {
			nRealColumn =
				static_cast<int>(std::floor(
					static_cast<float>((ev->x() - PatternEditor::nMargin)) /
					m_fGridWidth));
		}

		m_pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn,
												pSelectedInstrument, false );

		// Store note-specific properties.
		storeNoteProperties( m_pDraggedNote );
		
		m_nRow = nRow;
	}
}

///
/// Update the state during a Selection drag.
///
void DrumPatternEditor::mouseDragUpdateEvent( QMouseEvent *ev )
{
	int nRow = MAX_INSTRUMENTS - 1 -
		static_cast<int>(std::floor(static_cast<float>(ev->y())  /
									static_cast<float>(m_nGridHeight)));
	if ( nRow >= MAX_INSTRUMENTS ) {
		return;
	}

	PatternEditor::mouseDragUpdateEvent( ev );
}

void DrumPatternEditor::addOrDeleteNoteAction(	int nColumn,
												int nInstrumentRow,
												int nSelectedPatternNumber,
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
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	PatternList *pPatternList = pSong->getPatternList();

	if ( nSelectedPatternNumber < 0 ||
		 nSelectedPatternNumber >= pPatternList->size() ) {
		ERRORLOG( QString( "Invalid pattern number [%1]" )
				  .arg( nSelectedPatternNumber ) );
		return;
	}
	
	auto pPattern = pPatternList->get( nSelectedPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Pattern found for pattern number [%1] is not valid" )
				  .arg( nSelectedPatternNumber ) );
		return;
	}

	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( nInstrumentRow );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nInstrumentRow ) );
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

	if ( isDelete ) {

		// Find and delete an existing (matching) note.
		Pattern::notes_t *notes = (Pattern::notes_t *)pPattern->get_notes();
		bool bFound = false;
		FOREACH_NOTE_IT_BOUND_END( notes, it, nColumn ) {
			Note *pNote = it->second;
			if ( pNote == nullptr ) {
				ERRORLOG( "Invalid note" );
				continue;
			}
			if ( pNote->get_instrument_id() == pSelectedInstrument->get_id() &&
				 ( ( isNoteOff && pNote->get_note_off() ) ||
				   ( pNote->get_key() == oldNoteKeyVal &&
					 pNote->get_octave() == oldOctaveKeyVal &&
					 pNote->get_velocity() == oldVelocity &&
					 pNote->get_probability() == fProbability ) ) ) {
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
		
		Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan, nLength );
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
		if ( listen && !isNoteOff && pSelectedInstrument->hasSamples() ) {
			Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan, nLength);
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
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	m_pAudioEngine->lock( RIGHT_HERE );
	PatternList *pPatternList = pSong->getPatternList();
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	Pattern *pPattern = m_pPattern;
	Note *pFoundNote = nullptr;

	if ( nPattern < 0 || nPattern > pPatternList->size() ) {
		ERRORLOG( "Invalid pattern number" );
		m_pAudioEngine->unlock();
		return;
	}

	auto pFromInstrument = pInstrumentList->get( nRow );
	auto pToInstrument = pInstrumentList->get( nNewRow );

	FOREACH_NOTE_IT_BOUND_END((Pattern::notes_t *)pPattern->get_notes(), it, nColumn) {
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

///
/// Move or copy notes.
///
/// Moves or copies notes at the end of a Selection move, handling the
/// behaviours necessary for out-of-range moves or copies.
///
void DrumPatternEditor::selectionMoveEndEvent( QInputEvent *ev )
{
	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	updateModifiers( ev );
	QPoint offset = movingGridOffset();
	if ( offset.x() == 0 && offset.y() == 0 ) {
		// Move with no effect.
		return;
	}
	auto pInstrumentList = Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments();

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
														   pNote->get_note_off() ) );
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
														   pNote->get_note_off() ) );
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


///
/// Handle key press events.
///
/// Events are passed to Selection first, which may claim them (in which case they are ignored here).
///
void DrumPatternEditor::keyPressEvent( QKeyEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	auto pHydrogenApp = HydrogenApp::get_instance();
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	const int nBlockSize = 5, nWordSize = 5;
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	int nMaxInstrument = pHydrogen->getSong()->getDrumkit()->getInstruments()->size();
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
			pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument + 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) || ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		pHydrogen->setSelectedInstrumentNumber( std::min( nSelectedInstrument + nBlockSize,
													nMaxInstrument-1 ) );

	} else if ( ev->matches( QKeySequence::MoveToNextPage ) || ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down, scroll by the number of instruments that fit into the viewport
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		nSelectedInstrument += pParent->height() / m_nGridHeight;

		if ( nSelectedInstrument >= nMaxInstrument ) {
			nSelectedInstrument = nMaxInstrument - 1;
		}
		pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument );

	} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) || ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		pHydrogen->setSelectedInstrumentNumber( nMaxInstrument-1 );

	} else if ( ev->matches( QKeySequence::MoveToPreviousLine ) || ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( nSelectedInstrument > 0 ) {
			pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument - 1 );
		}
	} else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) || ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		pHydrogen->setSelectedInstrumentNumber( std::max( nSelectedInstrument - nBlockSize, 0 ) );

	} else if ( ev->matches( QKeySequence::MoveToPreviousPage ) || ev->matches( QKeySequence::SelectPreviousPage ) ) {
		QWidget *pParent = dynamic_cast< QWidget *>( parent() );
		assert( pParent );
		nSelectedInstrument -= pParent->height() / m_nGridHeight;
		if ( nSelectedInstrument < 0 ) {
			nSelectedInstrument = 0;
		}
		pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument );

	} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) || ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		pHydrogen->setSelectedInstrumentNumber( 0 );

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
	if ( m_pPattern == nullptr ) {
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

	const Pattern::notes_t* notes = m_pPattern->get_notes();

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

///
/// The screen area occupied by the keyboard cursor
///
QRect DrumPatternEditor::getKeyboardCursorRect()
{

	const uint x = PatternEditor::nMargin +
		m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;
	const int nSelectedInstrument =
		Hydrogen::get_instance()->getSelectedInstrumentNumber();
	const uint y = nSelectedInstrument * m_nGridHeight;
	float fHalfWidth;
	if ( m_nResolution != MAX_NOTES ) {
		// Corresponds to the distance between grid lines on 1/64 resolution.
		fHalfWidth = m_fGridWidth * 3;
	} else {
		// Corresponds to the distance between grid lines set to resolution
		// "off".
		fHalfWidth = m_fGridWidth;
	}
	return QRect( x - fHalfWidth, y + 2, fHalfWidth * 2, m_nGridHeight - 3 );

}

void DrumPatternEditor::selectAll()
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	m_selection.clearSelection();
	FOREACH_NOTE_CST_IT_BEGIN_LENGTH(m_pPattern->get_notes(), it, m_pPattern) {
		m_selection.addToSelection( it->second );
	}
	m_selection.updateWidgetGroup();
}


void DrumPatternEditor::deleteSelection()
{
	if ( m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	if ( m_selection.begin() != m_selection.end() ) {
		// Selection exists, delete it.
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		auto pInstrumentList = pHydrogen->getSong()->getDrumkit()->getInstruments();
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
	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	QClipboard *clipboard = QApplication::clipboard();
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	const auto pDrumkit = Hydrogen::get_instance()->getSong()->getDrumkit();
	const auto pInstrList = pDrumkit->getInstruments();
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
			Note *pNote = Note::load_from( n );
			pNote->mapTo( pDrumkit );
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
void DrumPatternEditor::drawPattern(QPainter& painter)
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	const auto pPref = H2Core::Preferences::get_instance();

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto  pInstrList = pSong->getDrumkit()->getInstruments();

	/*
		BUGFIX

		if m_pPattern is not renewed every time we draw a note,
		hydrogen will crash after you save a song and create a new one.
		-smoors
	*/
	updatePatternInfo();
	validateSelection();


	for ( const auto& pPattern : getPatternsToShow() ) {
		const Pattern::notes_t *pNotes = pPattern->get_notes();
		if ( pNotes->size() == 0 ) {
			continue;
		}
		bool bIsForeground = ( pPattern == m_pPattern );

		std::vector<int> noteCount; // instrument_id -> count
		std::stack<std::shared_ptr<Instrument>> instruments;

		// Process notes in batches by note position, counting the notes at each instrument so we can display
		// markers for instruments which have more than one note in the same position (a chord or genuine
		// duplicates)
		for ( auto posIt = pNotes->begin(); posIt != pNotes->end(); ) {
			if ( posIt->first >= pPattern->get_length() ) {
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
	if ( m_pPattern == nullptr ) {
		return;
	}
	auto pInstrList = Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments();
	int nInstrument = pInstrList->index( note->get_instrument() );
	if ( nInstrument == -1 ) {
		return;
	}

	QPoint pos ( PatternEditor::nMargin +
				 ( note->get_lead_lag() * AudioEngine::getLeadLagInTicks() +
				   note->get_position() ) * m_fGridWidth,
				 ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3 );

	drawNoteSymbol( p, pos, note, bIsForeground );
}

void DrumPatternEditor::drawBackground( QPainter& p)
{
	const auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	const QColor backgroundColor( pPref->getTheme().m_color.m_patternEditor_backgroundColor );
	const QColor backgroundInactiveColor( pPref->getTheme().m_color.m_windowColor );
	const QColor alternateRowColor( pPref->getTheme().m_color.m_patternEditor_alternateRowColor );
	const QColor selectedRowColor( pPref->getTheme().m_color.m_patternEditor_selectedRowColor );
	const QColor lineColor( pPref->getTheme().m_color.m_patternEditor_lineColor );
	const QColor lineInactiveColor( pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	int nInstruments = pSong->getDrumkit()->getInstruments()->size();
	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

	p.fillRect(0, 0, m_nActiveWidth, m_nEditorHeight, backgroundColor);
	if ( m_nActiveWidth < m_nEditorWidth ) {
		p.fillRect(m_nActiveWidth, 0, m_nEditorWidth - m_nActiveWidth, m_nEditorHeight,
				   backgroundInactiveColor);
	}

	for ( int ii = 0; ii < nInstruments; ii++ ) {
		int y = static_cast<int>(m_nGridHeight) * ii;
		if ( ii == nSelectedInstrument ) {
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
	if ( m_pPattern == nullptr ) {
		return;
	}
	drawGridLines( p );

	// The grid lines above are drawn full height. We will erase the
	// upper part.
	for ( int ii = 0; ii < nInstruments; ii++ ) {
		int y = static_cast<int>(m_nGridHeight) * ii;
		if ( ii == nSelectedInstrument ) {
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
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + m_nGridHeight;
		p.drawLine( 0, y, m_nActiveWidth, y);
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( QPen( lineInactiveColor, 1, Qt::SolidLine ) );
		for ( uint i = 0; i < (uint)nInstruments; i++ ) {
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
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() || m_bBackgroundInvalid ) {
		createBackground();
	}
	
	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, QRectF( pixelRatio * ev->rect().x(),
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

	// Draw cursor
	if ( hasFocus() && !HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		uint x = PatternEditor::nMargin + m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		uint y = nSelectedInstrument * m_nGridHeight;
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

	int nStartY = HydrogenApp::get_instance()->getPatternEditorPanel()->getVerticalScrollBar()->value();
	int nStartX = HydrogenApp::get_instance()->getPatternEditorPanel()->getHorizontalScrollBar()->value();
	int nEndY = std::min( static_cast<int>( m_nGridHeight ) * Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments()->size(),
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

void DrumPatternEditor::selectedInstrumentChangedEvent()
{
	updateEditor();
}

void DrumPatternEditor::selectedPatternChangedEvent()
{
	updateEditor();
}

void DrumPatternEditor::drumkitLoadedEvent() {
	updateEditor();
}

void DrumPatternEditor::songModeActivationEvent() {
	updateEditor();
}

///NotePropertiesRuler undo redo action
void DrumPatternEditor::undoRedoAction( int column,
										const PatternEditor::Mode& mode,
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

	if( pPattern != nullptr ) {
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BOUND_END(notes,it,column) {
			Note *pNote = it->second;
			assert( pNote );
			assert( (int)pNote->get_position() == column );
			if ( pNote->get_instrument() != pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument ) ) {
				continue;
			}

			if ( mode == PatternEditor::Mode::Velocity &&
				 !pNote->get_note_off() ) {
				pNote->set_velocity( velocity );
			}
			else if ( mode == PatternEditor::Mode::Pan ){
				pNote->setPan( fPan );
			}
			else if ( mode == PatternEditor::Mode::LeadLag ){
				pNote->set_lead_lag( leadLag );
			}
			else if ( mode == PatternEditor::Mode::NoteKey ){
				pNote->set_key_octave( (Note::Key)noteKeyVal, (Note::Octave)octaveKeyVal );
			}
			else if ( mode == PatternEditor::Mode::Probability ){
				pNote->set_probability( probability );
			}

			pHydrogen->setIsModified( true );
			NotePropertiesRuler::triggerStatusMessage( pNote, mode );
			break;
		}

		m_pPatternEditorPanel->updateEditors();
	}
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

void DrumPatternEditor::functionClearNotesRedoAction( int nSelectedInstrument, int nPatternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	PatternList* pPatternList = pSong->getPatternList();
	Pattern* pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( nPatternNumber ) );
		return;
	}

	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return;
	}

	pPattern->purge_instrument( pSelectedInstrument );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}


void DrumPatternEditor::functionClearNotesUndoAction( const std::list< H2Core::Note* >& noteList,
													  int nSelectedInstrument,
													  int patternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	PatternList* pPatternList = pHydrogen->getSong()->getPatternList();
	Pattern* pPattern = pPatternList->get( patternNumber );
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
		pPattern->insert_note( pNote );
	}
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesUndoAction(
	H2Core::PatternList* pAppliedNotesPatternList )
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPatternList = pSong->getPatternList();

	m_pAudioEngine->lock( RIGHT_HERE );

	for ( const auto& ppAppliedPattern : *pAppliedNotesPatternList ) {
		if ( ppAppliedPattern == nullptr ) {
			ERRORLOG( "invalid applied pattern" );
			continue;
		}

		// Find destination pattern to perform undo
		auto pPattern = pPatternList->find( ppAppliedPattern->get_name() );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "No pattern [%1] found in current pattern list" )
					  .arg( ppAppliedPattern->get_name() ) );
			continue;
		}

		// Remove all notes of applied pattern from destination pattern
		auto pAppliedNotes = ppAppliedPattern->get_notes();

		// Const removed by cast since we do inplace changees.
		Pattern::notes_t* pSongNotes = (Pattern::notes_t *)pPattern->get_notes();
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

	m_pAudioEngine->unlock();

	// Update editors
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}

void DrumPatternEditor::functionPasteNotesRedoAction(
	H2Core::PatternList* pCopiedNotesPatternList,
	H2Core::PatternList* pAppliedPatternList )
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() ==  nullptr ) {
		return;
	}

	const auto pDrumkit = pSong->getDrumkit();
	auto pPatternList = pSong->getPatternList();

	m_pAudioEngine->lock( RIGHT_HERE );

	// Add notes to pattern of the pattern list sharing the same name.
	for ( const auto& ppCopiedPattern : *pCopiedNotesPatternList ) {
		if ( ppCopiedPattern == nullptr ) {
			ERRORLOG( "Invalid pattern" );
			continue;
		}

		auto pPattern = pPatternList->find( ppCopiedPattern->get_name() );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "No pattern [%1] found in current pattern list" )
					  .arg( ppCopiedPattern->get_name() ) );
			continue;
		}

		// Create applied pattern. (We do not need an exact copie. The name is
		// enough)
		auto pApplied = new Pattern( pPattern->get_name() );

		// Add all notes of source pattern to destination pattern
		// and store all applied notes in applied pattern
		const Pattern::notes_t* pCopiedNotes = ppCopiedPattern->get_notes();
		const Pattern::notes_t* pSongNotes = pPattern->get_notes();
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
				pApplied->insert_note( new Note( pNote ) );

				// The note inserted in the song's pattern list needs to be
				// armed properly.
				Note* pNewNote = new Note( pNote );
				pNewNote->mapTo( pDrumkit );
				pPattern->insert_note( pNewNote );
			}
		}

		// Add applied pattern to applied list
		pAppliedPatternList->add( pApplied );
	}
	m_pAudioEngine->unlock();

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	// Update editors
	m_pPatternEditorPanel->updateEditors();
}



void DrumPatternEditor::functionFillNotesUndoAction( const QStringList& noteList,
													 int nSelectedInstrument,
													 int patternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	PatternList* pPatternList = pSong->getPatternList();
	Pattern* pPattern = pPatternList->get( patternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( patternNumber ) );
		return;
	}
	
	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

	for (int i = 0; i < noteList.size(); i++ ) {
		int nColumn  = noteList.value(i).toInt();
		Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->get_notes();
		FOREACH_NOTE_IT_BOUND_END(notes,it,nColumn) {
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


void DrumPatternEditor::functionFillNotesRedoAction( const QStringList& noteList,
													 int nSelectedInstrument,
													 int patternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	PatternList* pPatternList = pSong->getPatternList();
	Pattern* pPattern = pPatternList->get( patternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( patternNumber ) );
		return;
	}
	
	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine
	for (int i = 0; i < noteList.size(); i++ ) {

		// create the new note
		int position = noteList.value(i).toInt();
		Note *pNote = new Note( pSelectedInstrument, position );
		pPattern->insert_note( pNote );
	}
	m_pAudioEngine->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionRandomVelocityAction( const QStringList& noteVeloValue,
													  int nSelectedInstrument,
													  int selectedPatternNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	PatternList* pPatternList = pSong->getPatternList();
	Pattern* pPattern = pPatternList->get( selectedPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" )
				  .arg( selectedPatternNumber ) );
		return;
	}
	
	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( nSelectedInstrument );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

	int nResolution = granularity();
	int positionCount = 0;
	for (int i = 0; i < pPattern->get_length(); i += nResolution) {
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BOUND_LENGTH(notes,it,i, pPattern) {
			Note *pNote = it->second;
			if ( pNote->get_instrument() ==  pSelectedInstrument) {
				float velocity = noteVeloValue.value( positionCount ).toFloat();
				pNote->set_velocity(velocity);
				positionCount++;
			}
		}
	}
	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock();	// unlock the audio engine

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	m_pPatternEditorPanel->updateEditors();
}


void DrumPatternEditor::functionMoveInstrumentAction( int nSourceInstrument,
													  int nTargetInstrument )
{
		auto pHydrogen = Hydrogen::get_instance();
		m_pAudioEngine->lock( RIGHT_HERE );

		std::shared_ptr<Song> pSong = pHydrogen->getSong();
		auto pInstrumentList = pSong->getDrumkit()->getInstruments();

		if ( ( nTargetInstrument > (int)pInstrumentList->size() ) || ( nTargetInstrument < 0) ) {
			m_pAudioEngine->unlock();
			return;
		}

		pInstrumentList->move( nSourceInstrument, nTargetInstrument );

		pHydrogen->renameJackPorts( pSong );

		m_pAudioEngine->unlock();
		pHydrogen->setSelectedInstrumentNumber( nTargetInstrument );

		pHydrogen->setIsModified( true );
}

/// ~undo / redo actions from pattern editor instrument list
///==========================================================
