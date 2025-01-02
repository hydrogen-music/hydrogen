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
#include "NotePropertiesRuler.h"

#include "../HydrogenApp.h"

#include "UndoActions.h"
#include "PatternEditorPanel.h"
#include "PianoRollEditor.h"

#include <cassert>

#include <core/Hydrogen.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>

using namespace H2Core;

int NotePropertiesRuler::nKeyOctaveHeight =
	NotePropertiesRuler::nOctaveHeight +
	NotePropertiesRuler::nKeyLineHeight * KEYS_PER_OCTAVE;


NotePropertiesRuler::NotePropertiesRuler( QWidget *parent,
										  PatternEditor::Mode mode, Layout layout )
	: PatternEditor( parent )
	, m_nDrawPreviousColumn( -1 )
	, m_layout( layout )
{

	m_editor = PatternEditor::Editor::NotePropertiesRuler;
	m_mode = mode;

	m_fGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );

	if ( m_mode == PatternEditor::Mode::KeyOctave ) {
		m_nEditorHeight = NotePropertiesRuler::nKeyOctaveHeight;
	}
	else {
		m_nEditorHeight = NotePropertiesRuler::nDefaultHeight;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumHeight( m_nEditorHeight );

	setFocusPolicy( Qt::StrongFocus );

	// Generic pattern editor menu contains some operations that don't apply
	// here, and we will want to add menu options specific to this later.
	delete m_pPopupMenu;
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, SLOT( selectAll() ) );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, SLOT( selectNone() ) );
}

NotePropertiesRuler::~NotePropertiesRuler() {
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

	QString sUndoContext = "NotePropertiesRuler::wheelEvent";
	bool bUpdate = false;

	// We interact only with hovered notes. If any of them are part of the
	// current selection, we alter the values of all selected notes. It not, we
	// discard the selection.
	const auto notesUnderPoint = getElementsAtPoint(
		point, getCursorMargin( ev ), pPattern );
	if ( notesUnderPoint.size() == 0 ) {
		return;
	}

	bool bSelectionHovered = false;
	for ( const auto& ppNote : notesUnderPoint ) {
		if ( m_selection.isSelected( ppNote ) ) {
			bSelectionHovered = true;
			break;
		}
	}

	std::vector< std::shared_ptr<Note> > notes;
	std::vector< std::shared_ptr<Note> > notesStatusMessage;
	if ( bSelectionHovered ) {
		for ( const auto& ppNote : m_selection ) {
			if ( ppNote != nullptr ) {
				notes.push_back( ppNote );
			}
		}

		// We only show status messages for notes at point. In this case, only
		// for the selected ones.
		for ( const auto& ppNote : notesUnderPoint ) {
			if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
				notesStatusMessage.push_back( ppNote );
			}
		}
	}
	else {
		m_selection.clearSelection();
		notes = notesUnderPoint;
		notesStatusMessage = notesUnderPoint;
	}

	float fDelta;
	if ( m_mode == Mode::KeyOctave ) {
		// The available values in both key and octave sections are so few that
		// we do not provide a fine grained option using Alt.
		fDelta = ev->modifiers() == Qt::ControlModifier ? 3 : 1;
	}
	else if ( ev->modifiers() == Qt::AltModifier ) {
		fDelta = 0.01; // fine control
	}
	else if ( ev->modifiers() == Qt::ControlModifier ) {
		fDelta = 0.15; // coarse control
	}
	else {
		fDelta = 0.05; // regular
	}

	// Pressing Alt results in horizontal scrolling.
	if ( ev->angleDelta().y() < 0 ||
		 ( ev->modifiers() & Qt::AltModifier && ev->angleDelta().x() < 0 ) ) {
		fDelta = fDelta * -1.0;
	}

	m_oldNotes.clear();
	for ( auto& ppNote : notes ) {
		if ( ppNote == nullptr ) {
			continue;
		}

		m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
	}

	// Check whether the wheel event was triggered while mouse was in octave or
	// key section.
	const bool bKey = point.y() >= NotePropertiesRuler::nOctaveHeight;

	// Apply delta to the property
	const bool bValueChanged = adjustNotePropertyDelta( notes, fDelta, bKey );

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	handleKeyboardCursor( false );

	if ( bUpdate || bValueChanged ) {

		if ( bValueChanged ) {
			PatternEditor::triggerStatusMessage( notesStatusMessage, m_mode );
			addUndoAction( sUndoContext );
		}

		if ( m_mode == Mode::Velocity ) {
			m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
		}

		m_update = Update::Pattern;
		update();
	}
}


void NotePropertiesRuler::mouseClickEvent( QMouseEvent *ev ) {
	if ( m_pPatternEditorPanel->getPattern() == nullptr ) {
		return;
	}

	if ( ev->button() == Qt::LeftButton ) {
		// Treat single click as an instantaneous drag
		propertyDrawStart( ev );
		propertyDrawUpdate( ev );
		propertyDrawEnd();
	}

	PatternEditor::mouseClickEvent( ev );
}

void NotePropertiesRuler::mouseDragStartEvent( QMouseEvent *ev ) {
	if ( m_selection.isMoving() ) {
		prepareUndoAction( ev );
		selectionMoveUpdateEvent( ev );
	}
	else if ( ev->buttons() == Qt::RightButton ) {
		propertyDrawStart( ev );
		propertyDrawUpdate( ev );
	}
}

void NotePropertiesRuler::mouseDragUpdateEvent( QMouseEvent *ev ) {
	if ( ev->buttons() == Qt::RightButton ) {
		propertyDrawUpdate( ev );
	}
}

void NotePropertiesRuler::mouseDragEndEvent( QMouseEvent *ev ) {
	propertyDrawEnd();
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
	bool bKey = true;

	QPoint movingOffset = m_selection.movingOffset();
	if ( m_mode == PatternEditor::Mode::KeyOctave ) {
		// Check whether the drag started within the key or octave section.
		bKey = ( ev->y() - movingOffset.y() ) >=
			NotePropertiesRuler::nOctaveHeight;

		fDelta = static_cast<float>(-movingOffset.y()) /
			static_cast<float>(NotePropertiesRuler::nKeyLineHeight);
	} else {
		fDelta = (float)-movingOffset.y() / height();
	}

	std::vector< std::shared_ptr<Note> > notes;
	for ( const auto& ppNote : m_selection ) {
		if ( ppNote != nullptr &&
			( ( ppNote->getInstrumentId() == selectedRow.nInstrumentID &&
			   ppNote->getType() == selectedRow.sType ) ||
			  m_selection.isSelected( ppNote ) ) ) {

			// Record original note if not already recorded
			if ( m_oldNotes.find( ppNote ) == m_oldNotes.end() ) {
				m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
			}
			notes.push_back( ppNote );
		}
	}

	const bool bValueChanged = adjustNotePropertyDelta( notes, fDelta, bKey );

	// We only show status messages for notes at point.
	std::vector< std::shared_ptr<Note> > notesStatusMessage;
	for ( const auto& ppNote : m_notesHoveredOnDragStart ) {
		if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
			notesStatusMessage.push_back( ppNote );
		}
	}

	if ( bValueChanged ) {
		PatternEditor::triggerStatusMessage( notesStatusMessage, m_mode );
		m_update = Update::Pattern;
		update();
	}
}

void NotePropertiesRuler::selectionMoveEndEvent( QInputEvent *ev ) {
	//! The "move" has already been reflected in the notes. Now just complete Undo event.
	addUndoAction( "" );

	m_update = Update::Pattern;
	update();
}

//! Move of selection is cancelled. Revert notes to preserved state.
void NotePropertiesRuler::selectionMoveCancelEvent() {
	for ( auto it : m_oldNotes ) {
		std::shared_ptr<Note> pNote = it.first, pOldNote = it.second;
		switch ( m_mode ) {
		case PatternEditor::Mode::Velocity:
			pNote->setVelocity( pOldNote->getVelocity() );
			break;
		case PatternEditor::Mode::Pan:
			pNote->setPan( pOldNote->getPan() );
			break;
		case PatternEditor::Mode::LeadLag:
			pNote->setLeadLag( pOldNote->getLeadLag() );
			break;
		case PatternEditor::Mode::KeyOctave:
			pNote->setKeyOctave( pOldNote->getKey(), pOldNote->getOctave() );
			break;
		case PatternEditor::Mode::Probability:
			pNote->setProbability( pOldNote->getProbability() );
			break;
		case PatternEditor::Mode::None:
		default:
			break;
		}
	}

	if ( m_notesHoveredOnDragStart.size() != 0 ) {
		PatternEditor::triggerStatusMessage( m_notesHoveredOnDragStart, m_mode );
	}

	m_oldNotes.clear();
}

void NotePropertiesRuler::propertyDrawStart( QMouseEvent *ev )
{
	setCursor( Qt::CrossCursor );
	prepareUndoAction( ev );

	m_update = Update::Pattern;
	update();
}


//! Preserve current note properties at position x (or in selection, if any) for
//! use in later UndoAction.
void NotePropertiesRuler::prepareUndoAction( QMouseEvent* pEvent )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	m_oldNotes.clear();

	const auto notesUnderPoint = getElementsAtPoint(
		pEvent->pos(), getCursorMargin( pEvent ), pPattern );
	for ( const auto& ppNote : notesUnderPoint ) {
		if ( ppNote != nullptr ) {
			m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
		}
	}

	if ( notesUnderPoint.size() > 0 ) {
		m_nDrawPreviousColumn = notesUnderPoint[ 0 ]->getPosition();
	}
}

//! Update notes for a property adjust drag, in response to the mouse moving.
//! This modifies the values of the notes as the mouse moves, but does not
//! complete an undo action until the notes final value has been set. This
//! occurs either when the mouse is released, or when the pointer moves off of
//! the note's column.
void NotePropertiesRuler::propertyDrawUpdate( QMouseEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	// Issuing redo/undo actions bases on draw changes are issued in batches. In
	// case the cursor is moved slowly, we might have updates without any new
	// notes. If it is moved rapidly, it might have passed several columns since
	// the last update. We will take all notes between the current position and
	// the last one into account.
	int nRealColumn;
	eventPointToColumnRow( ev->pos(), nullptr, nullptr, &nRealColumn );
	const auto row = m_pPatternEditorPanel->getRowDB(
			m_pPatternEditorPanel->getSelectedRowDB() );

	if ( m_nDrawPreviousColumn == -1 ) {
		m_nDrawPreviousColumn = nRealColumn;
	}

	const int nDrawStart = std::min( m_nDrawPreviousColumn, nRealColumn );
	const int nDrawEnd = std::max( m_nDrawPreviousColumn, nRealColumn );
	std::vector< std::shared_ptr<Note> > notesSinceLastAction;
	const auto notes = pPattern->getNotes();
	for ( auto it = notes->lower_bound( nDrawStart );
		  it != notes->end() && it->first <= nDrawEnd; ++it ) {
		const auto ppNote = it->second;
		if ( ppNote != nullptr &&
			 ( ( ppNote->getInstrumentId() == row.nInstrumentID &&
				 ppNote->getType() == row.sType ) ||
			   m_selection.isSelected( ppNote ) ) ) {
			notesSinceLastAction.push_back( ppNote );
		}
	}

	if ( notesSinceLastAction.size() == 0 ) {
		return;
	}

	if ( m_nDrawPreviousColumn != nRealColumn ) {
		// Complete current undo action, and start a new one.
		addUndoAction( "NotePropertiesRuler::propertyDraw" );
		for ( const auto& ppNote : notesSinceLastAction ) {
			m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
		}
		m_nDrawPreviousColumn = nRealColumn;
	}

	// normalized
	const double fHeight = static_cast<double>(height());
	float fValue = static_cast<float>(
		std::clamp( ( fHeight - static_cast<double>(ev->y()) )/ fHeight,
					0.0, 1.1 ));

	// centered layouts support resetting the value to the baseline.
	if ( m_layout == Layout::Centered &&
		 ( ev->button() == Qt::MiddleButton ||
		   ( ev->modifiers() == Qt::ControlModifier &&
			 ev->button() == Qt::LeftButton ) )  ) {
		fValue = 0.5;
	}

	bool bValueChanged = false;

	for ( const auto& ppNote : notesSinceLastAction ) {
		// If a subset of notes is selected, we only act on them.
		if ( ! m_selection.isEmpty() == ! m_selection.isSelected( ppNote ) ) {
			continue;
		}
		if ( m_mode == PatternEditor::Mode::Velocity && !ppNote->getNoteOff() ) {
			if ( ppNote->getVelocity() != fValue ) {
				ppNote->setVelocity( fValue );
				bValueChanged = true;
			}
		}
		else if ( m_mode == PatternEditor::Mode::Pan && !ppNote->getNoteOff() ){
			if ( ppNote->getPanWithRangeFrom0To1() != fValue ) {
				ppNote->setPanWithRangeFrom0To1( fValue );
				bValueChanged = true;
			}
		}
		else if ( m_mode == PatternEditor::Mode::LeadLag ){
			if ( ppNote->getLeadLag() != ( fValue * -2.0 + 1.0 ) ) {
				ppNote->setLeadLag( fValue * -2.0 + 1.0 );
				bValueChanged = true;
			}
		}
		else if ( m_mode == PatternEditor::Mode::KeyOctave &&
				  ! ppNote->getNoteOff() ) {
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

			if ( ( nKey != 666 &&
				   nKey != static_cast<int>(ppNote->getKey()) ) ||
				 ( nOctave != 666 &&
				   nOctave != static_cast<int>(ppNote->getOctave()) ) ) {
				ppNote->setKeyOctave(
					static_cast<Note::Key>(nKey),
					static_cast<Note::Octave>(nOctave));
				bValueChanged = true;
			}
		}
		else if ( m_mode == PatternEditor::Mode::Probability ) {
			if ( ppNote->getProbability() != fValue ) {
				ppNote->setProbability( fValue );
				bValueChanged = true;
			}
		}
		
		if ( bValueChanged ) {
			PatternEditor::triggerStatusMessage( notesSinceLastAction, m_mode );
		}
	}

	if ( bValueChanged ) {
		Hydrogen::get_instance()->setIsModified( true );
		m_update = Update::Pattern;
		update();
		if ( m_mode == PatternEditor::Mode::Velocity ) {
			// A note's velocity determines its color in the other pattern
			// editors as well.
			m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
		}
	}
}

void NotePropertiesRuler::propertyDrawEnd()
{
	m_nDrawPreviousColumn = -1;
	addUndoAction( "NotePropertiesRuler::propertyDraw" );
	unsetCursor();

	m_update = Update::Pattern;
	update();
}

//! Adjust note properties by applying a delta to the current values, and
//! clipping to the appropriate range.
bool NotePropertiesRuler::adjustNotePropertyDelta(
	std::vector< std::shared_ptr<Note> > notes, float fDelta, bool bKey )
{
	bool bValueChanged = false;

	for ( auto& ppNote : notes ) {
		if ( ppNote == nullptr ) {
			continue;
		}

		auto pOldNote = m_oldNotes[ ppNote ];
		if ( pOldNote == nullptr ) {
			ERRORLOG( QString( "Could not find note corresponding to [%1]" )
					  .arg( ppNote->toQString() ) );
			continue;
		}

		switch( m_mode ) {
		case PatternEditor::Mode::Velocity: {
			if ( ! ppNote->getNoteOff() ) {
				const float fVelocity = qBound(
					VELOCITY_MIN, (pOldNote->getVelocity() + fDelta), VELOCITY_MAX );
				if ( fVelocity != ppNote->getVelocity() ) {
					ppNote->setVelocity( fVelocity );
					bValueChanged = true;
				}
			}
			break;
		}
		case PatternEditor::Mode::Pan: {
			if ( ! ppNote->getNoteOff() ) {
				// value in [0,1] or slight out of boundaries
				const float fVal = pOldNote->getPanWithRangeFrom0To1() + fDelta;
				if ( fVal != ppNote->getPanWithRangeFrom0To1() ) {
					// Does check boundaries internally.
					ppNote->setPanWithRangeFrom0To1( fVal );
					bValueChanged = true;
				}
			}
			break;
		}
		case PatternEditor::Mode::LeadLag: {
			// while most values in the ruler are defined between 0 and 1, lead
			// and lag is defined between -1 and 1. To still provide the same
			// feeling as for the other properties, we scale the delta by a
			// factor of 2.
			const float fLeadLag = qBound(
				LEAD_LAG_MIN, pOldNote->getLeadLag() - fDelta * 2, LEAD_LAG_MAX );
			if ( fLeadLag != ppNote->getLeadLag() ) {
				ppNote->setLeadLag( fLeadLag );
				bValueChanged = true;
			}
			break;
		}
		case PatternEditor::Mode::Probability: {
			if ( ! ppNote->getNoteOff() ) {
				const float fProbability = qBound(
					PROBABILITY_MIN, pOldNote->getProbability() + fDelta,
					PROBABILITY_MAX );
				if ( fProbability != ppNote->getProbability() ) {
					ppNote->setProbability( fProbability );
					bValueChanged = true;
				}
			}
			break;
		}
		case PatternEditor::Mode::KeyOctave: {
			const int nPitch = qBound(
				KEYS_PER_OCTAVE * OCTAVE_MIN,
				static_cast<int>(pOldNote->getPitchFromKeyOctave() +
								 std::round( fDelta) *
								 ( bKey ? 1 : KEYS_PER_OCTAVE )),
				KEYS_PER_OCTAVE * OCTAVE_MAX + KEY_MAX );
			Note::Octave octave;
			if ( nPitch >= 0 ) {
				octave = static_cast<Note::Octave>( nPitch / KEYS_PER_OCTAVE );
			} else {
				octave = static_cast<Note::Octave>( (nPitch-11) / KEYS_PER_OCTAVE );
			}
			Note::Key key = static_cast<Note::Key>(
				nPitch - KEYS_PER_OCTAVE * static_cast<int>(octave) );

			if ( key != ppNote->getKey() || octave != ppNote->getOctave() ) {
				ppNote->setKeyOctave( key, octave );
				bValueChanged = true;
			}
			break;
		}
		case PatternEditor::Mode::None:
		default:
			ERRORLOG("No mode set. No note property adjusted.");
		}
	}

	if ( bValueChanged ) {
		Hydrogen::get_instance()->setIsModified( true );
	}

	return bValueChanged;
}

void NotePropertiesRuler::keyPressEvent( QKeyEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bEventUsed = true;
	QString sUndoContext = "NotePropertiesRuler::keyPressEvent";

	// Value adjustments
	float fDelta = 0.0;

	if ( bIsSelectionKey ) {
		// Key was claimed by selection
	}
	else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete / Backspace: delete selected notes, or note under
		// keyboard cursor
		if ( ! m_selection.isEmpty() ) {
			deleteSelection();
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
		// Key: Alt+Down decrease parameter slightly
		fDelta = -0.01;
	}
	else if ( ev->key() == Qt::Key_Up && ev->modifiers() & Qt::ControlModifier ) {
		// Key: Ctrl+Up: increase parameter in a big jump
		fDelta = 0.5;
	}
	else if ( ev->key() == Qt::Key_Down && ev->modifiers() & Qt::ControlModifier ) {
		// Key: Ctrl+Up: decrease parameter in a big jump
		fDelta = -0.5;
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ) {
		// Key: MoveToStartOfDocument: increase parameter to maximum value
		fDelta = 1.0;
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ) {
		// Key: MoveEndOfDocument: decrease parameter to minimum value
		fDelta = -1.0;
	}
	else {
		bEventUsed = false;
	}

	bool bFullUpdate = false;
	// Value change
	if ( fDelta != 0.0 ) {
		// We interact only with notes under cursor. If any of them are part of
		// the current selection, we alter the values of all selected notes. It
		// not, we discard the selection.
		const auto notesUnderPoint =
			getElementsAtPoint( getCursorPosition(), 0, pPattern );
		if ( notesUnderPoint.size() == 0 ) {
			return;
		}

		bool bSelectionHovered = false;
		for ( const auto& ppNote : notesUnderPoint ) {
			if ( m_selection.isSelected( ppNote ) ) {
				bSelectionHovered = true;
				break;
			}
		}

		std::vector< std::shared_ptr<Note> > notes;
		std::vector< std::shared_ptr<Note> > notesStatusMessage;
		if ( bSelectionHovered ) {
			for ( const auto& ppNote : m_selection ) {
				if ( ppNote != nullptr ) {
					notes.push_back( ppNote );
				}
			}

			// We only show status messages for notes at cursor. In this case,
			// only for the selected ones.
			for ( const auto& ppNote : notesUnderPoint ) {
				if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
					notesStatusMessage.push_back( ppNote );
				}
			}
		}
		else {
			m_selection.clearSelection();
			notes = notesUnderPoint;
			notesStatusMessage = notesUnderPoint;
		}

		// For the KeyOctave Editor, adjust the pitch by a whole semitone
		if ( m_mode == PatternEditor::Mode::KeyOctave ) {
			if ( fDelta > 0.0 ) {
				fDelta = 1;
			} else if ( fDelta < 0.0 ) {
				fDelta = -1;
			}
		}

		m_oldNotes.clear();
		for ( auto& ppNote : notes ) {
			if ( ppNote == nullptr ) {
				continue;
			}

			m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
		}

		// Apply delta to the property
		const bool bValueChanged = adjustNotePropertyDelta(
			notes, fDelta, ! ( ev->modifiers() & Qt::ControlModifier ) );

		if ( bValueChanged ) {
			PatternEditor::triggerStatusMessage( notesStatusMessage, m_mode );

			addUndoAction( sUndoContext );

			Hydrogen::get_instance()->setIsModified( true );

			bFullUpdate = true;
		}
	}

	if ( ! bEventUsed ) {
		ev->setAccepted( false );
	}

	PatternEditor::keyPressEvent( ev, bFullUpdate );
}

void NotePropertiesRuler::addUndoAction( const QString& sUndoContext )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	int nSize = m_oldNotes.size();
	if ( nSize != 0 ) {
		auto pHydrogenApp = HydrogenApp::get_instance();

		if ( nSize != 1 ) {
			pHydrogenApp->beginUndoMacro(
				QString( tr( "Edit [%1] property of [%2] notes" ) )
				.arg( PatternEditor::modeToQString( m_mode ) ).arg( nSize ),
				sUndoContext );
		}
		for ( auto it : m_oldNotes ) {
			std::shared_ptr<Note> pNewNote = it.first, pOldNote = it.second;

			const int nNewKey = pNewNote->getKey();
			const int nNewOctave = pNewNote->getOctave();
			if ( pNewNote->getKey() != pOldNote->getKey() ||
				 pNewNote->getOctave() != pOldNote->getOctave() ) {
				// Note pitch was altered during the editing (drag update). We
				// have to temporarily reset the note key/octave (without
				// redrawing!) in order to allow for the redo part of the action
				// below to find the corresponding note.
				//
				// For all other note property edits this is not critical as the
				// note will be found and one the edit will be skip since the
				// note already holds the proper value.
				pNewNote->setKeyOctave( pOldNote->getKey(),
										  pOldNote->getOctave() );
			}

			pHydrogenApp->pushUndoCommand(
				new SE_editNotePropertiesAction(
					m_mode,
					m_pPatternEditorPanel->getPatternNumber(),
					pNewNote->getPosition(),
					pOldNote->getInstrumentId(),
					pOldNote->getType(),
					pNewNote->getVelocity(),
					pOldNote->getVelocity(),
					pNewNote->getPan(),
					pOldNote->getPan(),
					pNewNote->getLeadLag(),
					pOldNote->getLeadLag(),
					pNewNote->getProbability(),
					pOldNote->getProbability(),
					pNewNote->getLength(),
					pOldNote->getLength(),
					nNewKey,
					pOldNote->getKey(),
					nNewOctave,
					pOldNote->getOctave() ),
				sUndoContext );
		}
		if ( nSize != 1 ) {
			pHydrogenApp->endUndoMacro( sUndoContext );
		}
	}
	m_oldNotes.clear();
}

void NotePropertiesRuler::paintEvent( QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}

	PatternEditor::paintEvent( ev );

	QPainter painter( this );

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// Draw hovered notes
	int nOffsetX = 0;
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

			if ( m_offsetMap.find( ppNote ) != m_offsetMap.end() ) {
				nOffsetX = m_offsetMap[ ppNote ];
			}
			else {
				nOffsetX = 0;
			}

			drawNote( painter, ppNote, style, nOffsetX );

			if ( m_layout != Layout::KeyOctave ) {
				// Within the key/octave view notes should be unique.
				++nOffsetX;
			}
		}
	}

	// Draw moved notes
	auto pEditor = m_pPatternEditorPanel->getVisibleEditor();
	if ( ! m_selection.isEmpty() && (
			 pEditor->getSelectionState() == SelectionState::MouseMoving ||
			 pEditor->getSelectionState() == SelectionState::KeyboardMoving ) ) {
		for ( const auto& ppNote : m_selection ) {
			if ( m_offsetMap.find( ppNote ) != m_offsetMap.end() ) {
				nOffsetX = m_offsetMap[ ppNote ];
			}
			else {
				nOffsetX = 0;
			}

			drawNote( painter, ppNote, NoteStyle::Moved, nOffsetX );
		}
	}
}

void NotePropertiesRuler::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

void NotePropertiesRuler::drawDefaultBackground( QPainter& painter, int nHeight,
												 int nIncrement ) {
	
	const auto pPref = H2Core::Preferences::get_instance();

	const QColor borderColor(
		pPref->getTheme().m_color.m_patternEditor_lineColor );
	QColor lineColor(
		pPref->getTheme().m_color.m_patternEditor_line5Color );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );
	QColor backgroundColor(
		pPref->getTheme().m_color.m_patternEditor_backgroundColor );
	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
		backgroundColor = backgroundColor.darker( PatternEditor::nOutOfFocusDim );
	}

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

void NotePropertiesRuler::drawNote( QPainter& p,
									std::shared_ptr<H2Core::Note> pNote,
									NoteStyle noteStyle, int nOffsetX )
{
	if ( pNote == nullptr ) {
		return;
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// NoteOff notes can have a custom probability and lead lag. But having a
	// velocity and pan would not make any sense for them.
	if ( pNote->getNoteOff() &&
		 ! ( m_mode == PatternEditor::Mode::Probability ||
			 m_mode == PatternEditor::Mode::LeadLag ) ) {
		return;
	}

	const auto pPref = H2Core::Preferences::get_instance();
	const int nLineWidth = 3;

	QColor color;
	if ( ! pNote->getNoteOff() ) {
		color = PatternEditor::computeNoteColor( pNote->getVelocity() );
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
		pNote->getPosition() * m_fGridWidth;

	QPen highlightPen;
	QBrush highlightBrush;
	applyHighlightColor( &highlightPen, &highlightBrush, noteStyle );

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

	// Silhouette to show when a note is selected and moved to a different
	// position (in another editor!).
	auto pEditor = m_pPatternEditorPanel->getVisibleEditor();
	QPen movingPen( noteColor );
	QPoint movingOffset, delta;
	if ( noteStyle & NoteStyle::Moved ) {
		movingPen.setStyle( Qt::DotLine );
		movingPen.setWidth( 2 );
		delta = pEditor->movingGridOffset();
		movingOffset = QPoint( delta.x() * m_fGridWidth, 0 );
	}

	if ( m_layout == Layout::Centered || m_layout == Layout::Normalized ) {
		float fValue = 0;
		if ( m_mode == PatternEditor::Mode::Velocity ) {
			fValue = std::round( pNote->getVelocity() * height() );
		}
		else if ( m_mode == PatternEditor::Mode::Probability ) {
			fValue = std::round( pNote->getProbability() * height() );
		}
		else if ( m_mode == PatternEditor::Mode::Pan ) {
			// Rounding in order to not miss the center due to rounding errors
			// introduced in the Note class internals.
			fValue = std::round( pNote->getPan() * 100 ) / 100;
		}
		else if ( m_mode == PatternEditor::Mode::LeadLag ) {
			fValue = -1 * std::round( pNote->getLeadLag() * 100 ) / 100;
		}


		if ( m_layout == Layout::Centered && fValue == 0 ) {
			// value is centered - draw circle
			const int nY = static_cast<int>(std::round( height() * 0.5 ) );

			if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
				p.setPen( highlightPen );
				p.setBrush( highlightBrush );
				p.drawEllipse( nX - 7, nY - 7, 14, 14 );
			}

			if ( ! ( noteStyle & NoteStyle::Moved ) ) {
				p.setPen( notePen );
				p.setBrush( noteBrush );
				p.drawEllipse( nX - 4, nY - 4, 8, 8);
				p.setBrush( Qt::NoBrush );
			}
			else {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawEllipse( movingOffset.x() + nX - 6, nY - 6, 12, 12 );
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

			if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
				p.setPen( highlightPen );
				p.setBrush( highlightBrush );
				p.drawRoundedRect( nX - 1 - 4, nY - 4, nLineWidth + 8,
								   nHeight + 8, 5, 5 );
			}

			if ( ! ( noteStyle & NoteStyle::Moved ) ) {
				p.setPen( notePen );
				p.setBrush( noteBrush );
				p.drawRoundedRect( nX - 1 - 1, nY - 1,
								   nLineWidth + 2, nHeight + 2, 2, 2 );
				p.setBrush( Qt::NoBrush );
			}
			else {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawRoundedRect( movingOffset.x() + nX - 1 - 2, nY - 2,
								   nLineWidth + 4, nHeight + 4, 5, 5 );
			}
		}
	}
	else {
		// KeyOctave layout
		const int nRadiusOctave = 3;
		const int nOctaveY = ( 4 - pNote->getOctave() ) *
			NotePropertiesRuler::nKeyLineHeight;
		const int nRadiusKey = 5;
		const int nKeyY = NotePropertiesRuler::nKeyOctaveHeight -
			( ( pNote->getKey() + 1 ) * NotePropertiesRuler::nKeyLineHeight );

		// Paint selection outlines
		if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
			p.setPen( highlightPen );
			p.setBrush( highlightBrush );
			// Octave
			p.drawEllipse( QPoint( nX, nOctaveY ), nRadiusOctave + 3,
						   nRadiusOctave + 3 );
			// Key
			p.drawEllipse( QPoint( nX, nKeyY ), nRadiusKey + 3,
						   nRadiusKey + 3 );
		}

		if ( ! ( noteStyle & NoteStyle::Moved ) ) {
			// paint the octave
			p.setBrush( noteBrush );
			p.drawEllipse( QPoint( nX, nOctaveY ), nRadiusOctave, nRadiusOctave );

			// paint note
			p.drawEllipse( QPoint( nX, nKeyY ), nRadiusKey, nRadiusKey);
			p.setBrush( Qt::NoBrush );
		}
		else {
			// In case the note was moved to a different row in PianoRollEditor,
			// we have to adjust the pitch in here as well.
			int nMovedKeyY = nKeyY;
			int nMovedOctaveY = nOctaveY;
			bool bDrawMoveSilhouettes = true;
			if ( dynamic_cast<PianoRollEditor*>( pEditor ) != nullptr ) {
				const int nGridHeight = pEditor->getGridHeight();
				const int nNewPitch = pNote->getPitchFromKeyOctave() - delta.y();
				if ( nNewPitch < KEYS_PER_OCTAVE * OCTAVE_MIN ||
					 nNewPitch >= KEYS_PER_OCTAVE * ( OCTAVE_MAX + 1 ) ) {
					bDrawMoveSilhouettes = false;
				}

				nMovedKeyY = NotePropertiesRuler::nKeyOctaveHeight -
					( ( Note::pitchToKey( nNewPitch ) + 1 ) *
					  NotePropertiesRuler::nKeyLineHeight );
				nMovedOctaveY = ( 4 - Note::pitchToOctave( nNewPitch ) ) *
					NotePropertiesRuler::nKeyLineHeight;
			}

			if ( bDrawMoveSilhouettes ) {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawEllipse( QPoint( movingOffset.x() + nX, nMovedOctaveY ),
							   nRadiusOctave + 1, nRadiusOctave + 1 );

				// Key
				p.drawEllipse( QPoint( movingOffset.x() + nX, nMovedKeyY ),
							   nRadiusKey + 1, nRadiusKey + 1 );
			}
		}
	}
}

void NotePropertiesRuler::createBackground()
{
	const auto pPref = H2Core::Preferences::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();

	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );
	QColor lineColor(
		pPref->getTheme().m_color.m_patternEditor_lineColor );
	QColor textColor( pPref->getTheme().m_color.m_patternEditor_textColor );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );
	const QColor alternateRowColor =
		pPref->getTheme().m_color.m_patternEditor_alternateRowColor;
	const QColor octaveColor =
		pPref->getTheme().m_color.m_patternEditor_octaveRowColor;

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
	}

	const qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio ,
										   m_nEditorHeight * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		delete m_pPatternPixmap;
		m_pPatternPixmap = new QPixmap( m_nEditorWidth  * pixelRatio,
										m_nEditorHeight * pixelRatio );
		m_pPatternPixmap->setDevicePixelRatio( pixelRatio );
	}

	m_pBackgroundPixmap->fill( backgroundInactiveColor );

	QPainter p( m_pBackgroundPixmap );

	if ( m_layout == Layout::KeyOctave ) {
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

void NotePropertiesRuler::drawPattern() {

	m_offsetMap.clear();

	const qreal pixelRatio = devicePixelRatio();

	QPainter p( m_pPatternPixmap );
	// copy the background image
	p.drawPixmap( rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * rect().x(),
								pixelRatio * rect().y(),
								pixelRatio * rect().width(),
								pixelRatio * rect().height() ) );

	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	validateSelection();

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// Since properties of notes within the same row would end up being painted
	// on top of eachother, we go through the notes column by column and add
	// small horizontal offsets to each additional note to hint their existence.
	//
	// In addition, we first aggregate all notes residing at the same position
	// (column) in the same row and sort them according to their pitch. This way
	// they order is not seemingly random (else notes would be order according
	// to the insertion time into the pattern. An unintuitive measure from user
	// perspective with all our redo/undo facilities.)
	//
	// Also, we ensure selected notes will be rendered more prominently than not
	// selected ones.
	std::vector< std::shared_ptr<Note> > notes;
	auto sortAndDrawNotes = [&]( QPainter& p,
							std::vector< std::shared_ptr<Note> > notes,
							NoteStyle baseStyle ) {
		std::sort( notes.begin(), notes.end(), Note::compare );

		// Calculate a horizontal offset based on the order established above.
		if ( m_layout != Layout::KeyOctave ) {
			int nOffsetX = 0;
			for ( const auto& ppNote : notes ) {
				m_offsetMap[ ppNote ] = nOffsetX;
				++nOffsetX;
			}
		}
		else {
			// Duplicate are possible in here too since we show key and octave
			// separately. The pitch itself of the notes in here must be unqiue.
			std::multiset<Note::Key> keys;
			std::multiset<Note::Octave> octaves;
			for ( const auto& ppNote : notes ) {
				if ( ppNote == nullptr ) {
					continue;
				}
				m_offsetMap[ ppNote ] = std::max(
					keys.count( ppNote->getKey() ),
					octaves.count( ppNote->getOctave() ) );

				keys.insert( ppNote->getKey() );
				octaves.insert( ppNote->getOctave() );
			}
		}

		// Prioritze selected notes over not selected ones.
		std::vector< std::shared_ptr<Note> > selectedNotes, notSelectedNotes;
		for ( const auto& ppNote : notes ) {
			if ( m_selection.isSelected( ppNote ) ) {
				selectedNotes.push_back( ppNote );
			}
			else {
				notSelectedNotes.push_back( ppNote );
			}
		}

		for ( const auto& ppNote : notSelectedNotes ) {
			drawNote( p, ppNote, baseStyle, m_offsetMap[ ppNote] );
		}
		auto selectedStyle =
			static_cast<NoteStyle>(NoteStyle::Selected | baseStyle);
		for ( const auto& ppNote : selectedNotes ) {
			drawNote( p, ppNote, selectedStyle, m_offsetMap[ ppNote] );
		}
	};

	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		const auto baseStyle = ppPattern == pPattern ?
			NoteStyle::Foreground : NoteStyle::Background;

		int nLastPos = -1;
		for ( const auto& [ nnPos, ppNote ] : *ppPattern->getNotes() ) {
			if ( ppNote == nullptr ) {
				continue;
			}

			if ( nLastPos != nnPos ) {
				nLastPos = nnPos;
				sortAndDrawNotes( p, notes, baseStyle );
				notes.clear();
			}

			// NoteOff notes can have a custom probability and lead lag. But
			// having a velocity and pan would not make any sense for them.
			if ( ( ppNote->getNoteOff() &&
				   ! ( m_mode == PatternEditor::Mode::Probability ||
					   m_mode == PatternEditor::Mode::LeadLag ) ) ||
				 ! ( ppNote->getInstrumentId() == selectedRow.nInstrumentID &&
					 ppNote->getType() == selectedRow.sType ) &&
				 ! m_selection.isSelected( ppNote ) ) {
				continue;
			}

			notes.push_back( ppNote );
		}

		// Handle last column too
		if ( notes.size() > 0 ) {
			sortAndDrawNotes( p, notes, baseStyle );
			notes.clear();
		}
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
		if ( ! ( it->second->getInstrumentId() == selectedRow.nInstrumentID &&
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

	return std::move(result);
}

void NotePropertiesRuler::selectAll()
{
	selectAllNotesInRow( m_pPatternEditorPanel->getSelectedRowDB() );
}
