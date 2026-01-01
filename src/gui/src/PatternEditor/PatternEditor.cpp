/*
 * Hydrogen
 * Copyright(c) 2002-2008 by the Hydrogen Team
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

#include "PatternEditor.h"

#include "NotePropertiesRuler.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "PatternEditorSidebar.h"
#include "PianoRollEditor.h"

#include "../CommonStrings.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../UndoActions.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Globals.h>
#include <core/Preferences/Preferences.h>

#include <QtMath>

using namespace std;
using namespace H2Core;

PatternEditor::PatternEditor( QWidget *pParent )
	: Object()
	, Editor::Base<Elem>( pParent )
	, m_property( Property::None )
	, m_dragType( DragType::None )
	, m_nDragStartColumn( 0 )
	, m_nDragY( 0 )
	, m_dragStart( QPoint() )
	, m_nTick( -1 )
	, m_drawPreviousPosition( QPointF( 0, 0 ) )
	, m_drawPreviousGridPoint( GridPoint( -1, -1 ) )
	, m_drawPreviousKey( Note::Key::Invalid )
	, m_drawPreviousOctave( Note::Octave::Invalid )
	, m_nCursorPitch( 0 )
{
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	const auto pPref = H2Core::Preferences::get_instance();

	m_fGridWidth = pPref->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * 4 * 4 *
		H2Core::nTicksPerQuarter;
	m_nEditorHeight = height();
	m_nActiveWidth = m_nEditorWidth;

	updateWidth();
	updatePixmapSize();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking( true );

	// Popup context menu
	m_selectionActions.push_back(
		m_pPopupMenu->addAction( tr( "&Cut" ), this, [&]() {
			popupSetup();
			cut();
			popupTeardown(); } ) );
	m_selectionActions.push_back(
		m_pPopupMenu->addAction( tr( "&Copy" ), this, [&]() {
			popupSetup();
			copy();
			popupTeardown(); } ) );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, [&]() {
		popupSetup();
		paste();
		popupTeardown(); } );
	m_selectionActions.push_back(
		m_pPopupMenu->addAction( tr( "&Delete" ), this, [&]() {
			popupSetup();
			deleteSelection();
			popupTeardown(); } ) );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "A&lign to grid" ), this, SLOT( alignToGrid() ) ) );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "Randomize velocity" ), this, SLOT( randomizeVelocity() ) ) );
	m_pPopupMenu->addAction( tr( "Select &all" ), this,
							 [&]() { selectAll();
								 updateVisibleComponents( Editor::Update::Content ); } );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, [&]() {
		m_selection.clearSelection();
		updateVisibleComponents( Editor::Update::Content );
	} );
	connect( m_pPopupMenu, &QMenu::aboutToShow, [&]() {
		popupMenuAboutToShow(); } );
	connect( m_pPopupMenu, &QMenu::aboutToHide, [&]() {
		popupMenuAboutToHide(); } );
}

PatternEditor::~PatternEditor() {
	m_draggedNotes.clear();
}

void PatternEditor::addOrRemoveNoteAction( int nPosition,
										   Instrument::Id id,
										   const Instrument::Type& sType,
										   int nPatternNumber,
										   int nOldLength,
										   float fOldVelocity,
										   float fOldPan,
										   float fOldLeadLag,
										   Note::Key oldKey,
										   Note::Octave oldOctave,
										   float fOldProbability,
										   Editor::Action action,
										   bool bIsNoteOff,
										   bool bIsMappedToDrumkit,
										   Editor::ActionModifier modifier )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pPatternList = pSong->getPatternList();
	if ( nPatternNumber < 0 ||
		 nPatternNumber >= pPatternList->size() ) {
		ERRORLOG( QString( "Pattern number [%1] out of bound [0,%2]" )
				  .arg( nPatternNumber ).arg( pPatternList->size() ) );
		return;
	}

	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Pattern found for pattern number [%1] is not valid" )
				  .arg( nPatternNumber ) );
		return;
	}

	if ( id == Instrument::EmptyId && sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row" ) );
		return;
	}

	auto pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();
	auto pVisibleEditor = pPatternEditorPanel->getVisibleEditor();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );	// lock the audio engine

	if ( action == Editor::Action::Delete ) {
		// Find and delete an existing (matching) note.

		// In case there are multiple notes at this position, use all provided
		// properties to find right one.
		std::vector< std::shared_ptr<Note> > notesFound;
		const auto pNotes = pPattern->getNotes();
		for ( auto it = pNotes->lower_bound( nPosition );
			  it != pNotes->end() && it->first <= nPosition; ++it ) {
			auto ppNote = it->second;
			if ( ppNote != nullptr && ppNote->getInstrumentId() == id &&
				 ppNote->getType() == sType && ppNote->getKey() == oldKey &&
				 ppNote->getOctave() == oldOctave ) {
				notesFound.push_back( ppNote );
			}
		}

		auto removeNote = [&]( std::shared_ptr<Note> pNote ) {
			if ( pVisibleEditor->m_selection.isSelected( pNote ) ) {
				pVisibleEditor->m_selection.removeFromSelection( pNote, false );
			}
			pPattern->removeNote( pNote );
		};

		if ( notesFound.size() == 1 ) {
			// There is just a single note at this position. Remove it
			// regardless of its properties.
			removeNote( notesFound[ 0 ] );
		}
		else if ( notesFound.size() > 1 ) {
			bool bFound = false;

			for ( const auto& ppNote : notesFound ) {
				if ( ppNote->getLength() == nOldLength &&
					 ppNote->getVelocity() == fOldVelocity &&
					 ppNote->getPan() == fOldPan &&
					 ppNote->getLeadLag() == fOldLeadLag &&
					 ppNote->getProbability() == fOldProbability &&
					 ppNote->getNoteOff() == bIsNoteOff ) {
					bFound = true;
					removeNote( ppNote );
				}
			}

			if ( ! bFound ) {
				QStringList noteStrings;
				for ( const auto& ppNote : notesFound ) {
					noteStrings << "\n - " << ppNote->toQString();
				}
				ERRORLOG( QString( "length: %1, velocity: %2, pan: %3, lead&lag: %4, probability: %5, noteOff: %6 not found amongst notes:%7" )
						  .arg( nOldLength ).arg( fOldVelocity ).arg( fOldPan )
						  .arg( fOldLeadLag ).arg( fOldProbability )
						  .arg( bIsNoteOff ).arg( noteStrings.join( "" ) ) );
			}
		}
		else {
			ERRORLOG( "Did not find note to delete" );
		}

	}
	else {
		// create the new note
		float fVelocity = fOldVelocity;
		float fPan = fOldPan ;
		int nLength = nOldLength;

		if ( bIsNoteOff ) {
			fVelocity = VELOCITY_MIN;
			fPan = PAN_DEFAULT;
			nLength = 1;
		}

		std::shared_ptr<Instrument> pInstrument = nullptr;
		if ( id != Instrument::EmptyId && bIsMappedToDrumkit ) {
			// Can still be nullptr for notes in unmapped rows.
			pInstrument =
				pSong->getDrumkit()->getInstruments()->find( id );
		}

		auto pNote = std::make_shared<Note>( pInstrument, nPosition, fVelocity,
											 fPan, nLength );
		pNote->setInstrumentId( id );
		pNote->setType( sType );
		pNote->setNoteOff( bIsNoteOff );
		pNote->setLeadLag( fOldLeadLag );
		pNote->setProbability( fOldProbability );
		pNote->setKeyOctave( oldKey, oldOctave );
		pPattern->insertNote( pNote );

		if ( static_cast<char>(modifier) &
			 static_cast<char>(Editor::ActionModifier::AddToSelection) ) {
			pVisibleEditor->m_selection.addToSelection( pNote );
		}

		if ( static_cast<char>(modifier) &
			 static_cast<char>(Editor::ActionModifier::MoveCursorTo) ) {
			pPatternEditorPanel->setCursorColumn( pNote->getPosition() );
			pPatternEditorPanel->setSelectedRowDB(
				pPatternEditorPanel->findRowDB( pNote ) );
		}
	}
	pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
	pHydrogen->setIsModified( true );

	pVisibleEditor->updateMouseHoveredElements( nullptr );
	pVisibleEditor->updateKeyboardHoveredElements();

	pPatternEditorPanel->updateEditors( Editor::Update::Content );
}

void PatternEditor::deselectAndOverwriteNotes(
	const std::vector< std::shared_ptr<H2Core::Note> >& selected,
	const std::vector< std::shared_ptr<H2Core::Note> >& overwritten )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	// Iterate over all the notes in 'selected' and 'overwrite' by erasing any *other* notes occupying the
	// same position.
	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	Pattern::notes_t *pNotes = const_cast< Pattern::notes_t *>( pPattern->getNotes() );
	for ( auto pSelectedNote : selected ) {
		m_selection.removeFromSelection( pSelectedNote, /* bCheck=*/false );
		bool bFoundExact = false;
		int nPosition = pSelectedNote->getPosition();
		for ( auto it = pNotes->lower_bound( nPosition ); it != pNotes->end() && it->first == nPosition; ) {
			auto pNote = it->second;
			if ( !bFoundExact && pSelectedNote->match( pNote ) ) {
				// Found an exact match. We keep this.
				bFoundExact = true;
				++it;
			}
			else if ( pNote->getInstrumentId() == pSelectedNote->getInstrumentId() &&
					  pNote->getType() == pSelectedNote->getType() &&
					  pNote->getKey() == pSelectedNote->getKey() &&
					  pNote->getOctave() == pSelectedNote->getOctave() &&
					  pNote->getPosition() == pSelectedNote->getPosition() ) {
				// Something else occupying the same position (which may or may not be an exact duplicate)
				it = pNotes->erase( it );
			}
			else {
				// Any other note
				++it;
			}
		}
	}
	pHydrogen->getAudioEngine()->unlock();
	pHydrogen->setIsModified( true );
}

void PatternEditor::undoDeselectAndOverwriteNotes(
	const std::vector< std::shared_ptr<H2Core::Note> >& selected,
	const std::vector< std::shared_ptr<H2Core::Note> >& overwritten )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	// Restore previously-overwritten notes, and select notes that were selected before.
	m_selection.clearSelection( /* bCheck=*/false );
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	for ( const auto& ppNote : overwritten ) {
		auto pNewNote = std::make_shared<Note>( ppNote );
		pPattern->insertNote( pNewNote );
	}
	// Select the previously-selected notes
	for ( auto pNote : selected ) {
		FOREACH_NOTE_CST_IT_BOUND_END( pPattern->getNotes(), it, pNote->getPosition() ) {
			if ( pNote->match( it->second ) ) {
				m_selection.addToSelection( it->second );
				break;
			}
		}
	}
	pHydrogen->getAudioEngine()->unlock();
	pHydrogen->setIsModified( true );
	updateVisibleComponents( Editor::Update::Content );
}

void PatternEditor::editNotePropertiesAction( const Property& property,
											  int nPatternNumber,
											  int nPosition,
											  Instrument::Id oldId,
											  Instrument::Id newId,
											  const Instrument::Type& sOldType,
											  const Instrument::Type& sNewType,
											  float fVelocity,
											  float fPan,
											  float fLeadLag,
											  float fProbability,
											  int nLength,
											  Note::Key newKey,
											  Note::Key oldKey,
											  Note::Octave newOctave,
											  Note::Octave oldOctave )
{
	auto pPatternEditorPanel =
		HydrogenApp::get_instance()->getPatternEditorPanel();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
	std::shared_ptr<H2Core::Pattern> pPattern;

	if ( nPatternNumber != -1 &&
		 nPatternNumber < pPatternList->size() ) {
		pPattern = pPatternList->get( nPatternNumber );
	}
	if ( pPattern == nullptr ) {
		return;
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	// Find the note to edit
	auto pNote =
		pPattern->findNote( nPosition, oldId, sOldType, oldKey, oldOctave );
	if ( pNote == nullptr && property == Property::Type ) {
		// Maybe the type of an unmapped note was set to one already present in
		// the drumkit. In this case the instrument id of the note is remapped
		// and might not correspond to the value used to create the undo/redo
		// action.
		//
		bool bOk;
		const auto kitId =
			pSong->getDrumkit()->toDrumkitMap()->getId( sOldType, &bOk );
		if ( bOk ) {
			pNote = pPattern->findNote(
				nPosition, kitId, sOldType, oldKey, oldOctave
			);
		}
	}
	else if ( pNote == nullptr && property == Property::InstrumentId ) {
		// When adding an instrument to a row on typed but unmapped notes, the
		// redo part of the instrument ID is done automatically as part of the
		// mapping to the updated kit. Only the undo part needs to be covered in
		// here.
		pHydrogen->getAudioEngine()->unlock();
		return;
	}

	bool bValueChanged = false;

	if ( pNote != nullptr ){
		switch ( property ) {
		case Property::Velocity:
			if ( pNote->getVelocity() != fVelocity ) {
				pNote->setVelocity( fVelocity );
				bValueChanged = true;
			}
			break;
		case Property::Pan:
			if ( pNote->getPan() != fPan ) {
				pNote->setPan( fPan );
				bValueChanged = true;
			}
			break;
		case Property::LeadLag:
			if ( pNote->getLeadLag() != fLeadLag ) {
				pNote->setLeadLag( fLeadLag );
				bValueChanged = true;
			}
			break;
		case Property::KeyOctave:
			if ( pNote->getKey() != newKey ||
				 pNote->getOctave() != newOctave ) {
				pNote->setKeyOctave( newKey, newOctave );
				bValueChanged = true;
			}
			break;
		case Property::Probability:
			if ( pNote->getProbability() != fProbability ) {
				pNote->setProbability( fProbability );
				bValueChanged = true;
			}
			break;
		case Property::Length:
			if ( pNote->getLength() != nLength ) {
				pNote->setLength( nLength );
				bValueChanged = true;
			}
			break;
		case Property::Type:
			if ( pNote->getType() != sNewType ||
				 pNote->getInstrumentId() != newId ) {
				pNote->setInstrumentId( newId );
				pNote->setType( sNewType );

				auto pInstrument = pSong->getDrumkit()->mapInstrument(
					sNewType, newId );

				pNote->mapToInstrument( pInstrument );

				// Changing a type is effectively moving the note to another row
				// of the DrumPatternEditor. This could result in overlapping
				// notes at the same position. To guard against this, select all
				// adjusted notes to harness the checkDeselectElements
				// capabilities.
				pPatternEditorPanel->getVisibleEditor()->
					m_selection.addToSelection( pNote );

				bValueChanged = true;
			}
			break;
		case Property::InstrumentId:
			if ( pNote->getInstrumentId() != newId ) {
				pNote->setInstrumentId( newId );
				bValueChanged = true;
			}
			break;
		case Property::None:
		default:
			ERRORLOG("No property set. No note property adjusted.");
		}
	} else {
		ERRORLOG("note could not be found");
	}

	pHydrogen->getAudioEngine()->unlock();

	if ( bValueChanged ) {
		pHydrogen->setIsModified( true );
		std::vector< std::shared_ptr<Note > > notes{ pNote };

		if ( property == Property::Type || property == Property::InstrumentId ) {
			pPatternEditorPanel->updateDB();
			pPatternEditorPanel->updateEditors( Editor::Update::Content );
			pPatternEditorPanel->resizeEvent( nullptr );
		}
		else {
			pPatternEditorPanel->updateEditors( Editor::Update::Content );
		}
	}
}

bool PatternEditor::isSelectionMoving() const {
	return m_selection.isMoving();
}

GridPoint PatternEditor::movingGridOffset() const {
	const QPoint rawOffset = m_selection.movingOffset();

	// Quantization in y direction is mandatory. A note can not be placed
	// between lines.
	const int nQuantY = m_nGridHeight;
	int nBiasY = nQuantY / 2;
	if ( rawOffset.y() < 0 ) {
		nBiasY = -nBiasY;
	}
	const int nOffsetY = (rawOffset.y() + nBiasY) / nQuantY;

	const float fX = static_cast<float>(rawOffset.x());
	float fGranularity = 1.0;
	if ( m_pPatternEditorPanel->isQuantized() ) {
		fGranularity = static_cast<float>(granularity());
	}
	// A bias of m_fGridWidth * fGranularity / 2 -> half the distance between to
	// grid point is introduced to "round" to the nearest grid point.
	const int nOffsetX = static_cast<int>(
		fGranularity * std::floor( ( fX + m_fGridWidth * fGranularity / 2 ) /
								   ( m_fGridWidth * fGranularity ) ) );

	return GridPoint( nOffsetX, nOffsetY);
}

void PatternEditor::setCursorPitch( int nCursorPitch ) {
	const int nMinPitch =
		Note::octaveKeyToPitch( Note::OctaveMin, Note::KeyMin );
	const int nMaxPitch =
		Note::octaveKeyToPitch( Note::OctaveMax, Note::KeyMax );

	if ( nCursorPitch < nMinPitch ) {
		nCursorPitch = nMinPitch;
	}
	else if ( nCursorPitch >= nMaxPitch ) {
		nCursorPitch = nMaxPitch;
	}

	if ( nCursorPitch == m_nCursorPitch ) {
		return;
	}

	m_nCursorPitch = nCursorPitch;

	// Highlight selected row.
	if ( m_instance == Editor::Instance::PianoRoll ) {
		m_update = Editor::Update::Background;
		update();
	}

	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		m_pPatternEditorPanel->ensureCursorIsVisible();
		m_pPatternEditorPanel->getSidebar()->updateEditor();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getVisiblePropertiesRuler()
			->updateEditor( Editor::Update::Transient );
	}
}

bool PatternEditor::syncLasso() {

	const int nMargin = 5;

	bool bUpdate = false;
	if ( m_instance == Editor::Instance::NotePropertiesRuler ) {
		auto pVisibleEditor = m_pPatternEditorPanel->getVisibleEditor();

		QRect prevLasso;
		QRect cursorStart = m_selection.getKeyboardCursorStart();
		QRect lasso = m_selection.getLasso();
		QRect cursor = getKeyboardCursorRect();

		// Ensure lasso is full height as we do not support lasso selecting
		// notes by property value.
		lasso.setY( cursor.y() );
		lasso.setHeight( cursor.height() );
		cursorStart.setY( cursor.y() );
		m_selection.syncLasso(
			m_selection.getSelectionState(), cursorStart, lasso );

		if ( dynamic_cast<DrumPatternEditor*>(pVisibleEditor) != nullptr ) {
			// The ruler does not feature a proper y and height coordinate. We
			// have to ensure to either keep the one already present in the
			// others or use the current line as fallback.
			if ( pVisibleEditor->m_selection.isLasso() ) {
				cursor = pVisibleEditor->m_selection.getKeyboardCursorStart();
				prevLasso = pVisibleEditor->m_selection.getLasso();
			}
			else {
				cursor = pVisibleEditor->getKeyboardCursorRect();
				prevLasso = cursor;
			}
		}
		else {
			// PianoRollEditor
			//
			// All notes shown in the NotePropertiesRuler are shown in
			// PianoRollEditor as well. But scattered all over the place. In
			// DrumPatternEditor we just have to mark a row. In PRE we have to
			// ensure that all notes are properly covered by the lasso. In here
			// we expect all selected notes already being added and adjust lasso
			// dimensions to cover them.
			auto pPianoRoll = dynamic_cast<PianoRollEditor*>(pVisibleEditor);
			if ( pPianoRoll == nullptr ) {
				ERRORLOG( "this ain't piano roll" );
				return false;
			}
			if ( pVisibleEditor->m_selection.isLasso() ) {
				cursor = pVisibleEditor->m_selection.getKeyboardCursorStart();
				prevLasso = pVisibleEditor->m_selection.getLasso();
			}
			else {
				cursor = pVisibleEditor->getKeyboardCursorRect();
				prevLasso = cursor;
			}

			// The selection can be started in DrumPatternEditor and contain
			// notes not shown in PianoRollEditor.
			const auto row = m_pPatternEditorPanel->getRowDB(
				m_pPatternEditorPanel->getSelectedRowDB() );

			for ( const auto& ppNote : m_selection ) {
				if ( ppNote != nullptr && row.contains( ppNote ) ) {
					const QPoint np = elementToPoint( ppNote );
					const QRect noteRect = QRect(
						np.x() - cursor.width() / 2, np.y() - cursor.height() / 2,
						cursor.width(), cursor.height() );
					if ( ! prevLasso.intersects( noteRect ) ) {
						prevLasso = prevLasso.united( noteRect );
					}
				}
			}
		}
		cursorStart.setY( cursor.y() );
		cursorStart.setHeight( cursor.height() );
		lasso.setY( prevLasso.y() );
		lasso.setHeight( prevLasso.height() );

		bUpdate = pVisibleEditor->m_selection.syncLasso(
			m_selection.getSelectionState(), cursorStart, lasso );
	}
	else {
		// DrumPattern or Piano roll
		const auto pVisibleRuler =
			m_pPatternEditorPanel->getVisiblePropertiesRuler();

		// The ruler does not feature a proper y coordinate and height. We have
		// to use the entire height instead.
		QRect cursorStart = m_selection.getKeyboardCursorStart();
		QRect lasso = m_selection.getLasso();
		QRect lassoStart = m_selection.getKeyboardCursorStart();
		const QRect cursor = pVisibleRuler->getKeyboardCursorRect();
		cursorStart.setY( cursor.y() );
		cursorStart.setHeight( cursor.height() );
		lasso.setY( cursor.y() );
		lasso.setHeight( cursor.height() );

		pVisibleRuler->m_selection.syncLasso(
			m_selection.getSelectionState(), cursorStart, lasso );

		// We force a full update lasso could have been changed in vertical
		// direction (note selection).
		bUpdate = true;
	}

	return bUpdate;
}

void PatternEditor::triggerStatusMessage(
	const std::vector< std::shared_ptr<Note> > notes, const Property& property,
	bool bSquash ) {
	QString sCaller( _class_name() );
	QString sUnit( tr( "ticks" ) );

	// Aggregate all values of the provided notes
	QStringList values;
	float fValue;
	for ( const auto& ppNote : notes ) {
		if ( ppNote == nullptr ) {
			continue;
		}

		if ( ! bSquash ) {
			// Allow the status message widget to squash all changes
			// corresponding to the same property of the same set to notes.
			sCaller.append( QString( "::%1:%2" )
							.arg( ppNote->getPosition() )
							.arg( m_pPatternEditorPanel->findRowDB( ppNote ) ) );
		}

		switch ( property ) {
		case PatternEditor::Property::Velocity:
			if ( ! ppNote->getNoteOff() ) {
				values << QString( "%1").arg( ppNote->getVelocity(), 2, 'f', 2 );
			}
			break;

		case PatternEditor::Property::Pan:
			if ( ! ppNote->getNoteOff() ) {

				// Round the pan to not miss the center due to fluctuations
				fValue = ppNote->getPan() * 100;
				fValue = std::round( fValue );
				fValue = fValue / 100;

				if ( fValue > 0.0 ) {
					values << QString( "%1 (%2)" ).arg( fValue / 2, 2, 'f', 2 )
						/*: Direction used when panning a note. */
						.arg( tr( "right" ) );
				}
				else if ( fValue < 0.0 ) {
					values << QString( "%1 (%2)" )
						/*: Direction used when panning a note. */
						.arg( -1 * fValue / 2, 2, 'f', 2 ).arg( tr( "left" ) );
				}
				else {
					/*: Direction used when panning a note. */
					values <<  tr( "centered" );
				}
			}
			break;

		case PatternEditor::Property::LeadLag:
			// Round the pan to not miss the center due to fluctuations
			fValue = ppNote->getLeadLag() * 100;
			fValue = std::round( fValue );
			fValue = fValue / 100;
			if ( fValue < 0.0 ) {
				values << QString( "%1 (%2)" )
					.arg( fValue * -1 * AudioEngine::getLeadLagInTicks(),
						  2, 'f', 2 )
					/*: Relative temporal position when setting note lead & lag. */
					.arg( tr( "lead" ) );
			}
			else if ( fValue > 0.0 ) {
				values << QString( "%1 (%2)" )
					.arg( fValue * AudioEngine::getLeadLagInTicks(), 2, 'f', 2 )
					/*: Relative temporal position when setting note lead & lag. */
					.arg( tr( "lag" ) );
			}
			else {
				/*: Relative temporal position when setting note lead & lag. */
				values << tr( "on beat" );
			}
			break;

		case PatternEditor::Property::KeyOctave:
			if ( ! ppNote->getNoteOff() ) {
				values << QString( "%1 : %2" )
					.arg( Note::KeyToQString( ppNote->getKey() ) )
					.arg( static_cast<int>(ppNote->getOctave()) );
			}
			break;

		case PatternEditor::Property::Probability:
			values << QString( "%1" ).arg( ppNote->getProbability(), 2, 'f', 2 );
			break;

		case PatternEditor::Property::Length:
			if ( ! ppNote->getNoteOff() ) {
				values << QString( "%1" )
					.arg( ppNote->getProbability(), 2, 'f', 2 );
			}
			break;
		case PatternEditor::Property::InstrumentId:
			return;
		case PatternEditor::Property::Type:
		case PatternEditor::Property::None:
		default:
			break;
		}
	}

	if ( values.size() == 0 && property != PatternEditor::Property::Type ) {
		return;
	}

	// Compose the actual status message
	QString s;
	switch ( property ) {
	case PatternEditor::Property::Velocity:
		s = QString( tr( "Set note velocity" ) )
				.append( QString( ": [%1]").arg( values.join( ", " ) ) );
		sCaller.append( ":Velocity" );
		break;

	case PatternEditor::Property::Pan:
		s = QString( tr( "Set note pan" ) )
				.append( QString( ": [%1]").arg( values.join( ", " ) ) );
		sCaller.append( ":Pan" );
		break;

	case PatternEditor::Property::LeadLag:
		s = QString( tr( "Set note lead/lag" ) )
			.append( QString( ": [%1]").arg( values.join( ", " ) ) );
		sCaller.append( ":LeadLag" );
		break;

	case PatternEditor::Property::KeyOctave:
		s = QString( tr( "Set note pitch" ) )
			.append( QString( ": [%1]").arg( values.join( ", " ) ) );
		break;

	case PatternEditor::Property::Probability:
		s = tr( "Set note probability" )
			.append( QString( ": [%1]" ).arg( values.join( ", " ) ) );
		sCaller.append( ":Probability" );
		break;

	case PatternEditor::Property::Length:
		s = tr( "Set note length" )
			.append( QString( ": [%1]" ).arg( values.join( ", " ) ) );
		sCaller.append( ":Length" );
		break;

	case PatternEditor::Property::Type:
		// All notes should have the same type. No need to aggregate in here.
		s = tr( "Set note type" )
			.append( QString( ": [%1]" ).arg( notes[ 0 ]->getType() ) );
		sCaller.append( ":Type" );
		break;

	default:
		ERRORLOG( PatternEditor::propertyToQString( property ) );
	}

	if ( ! s.isEmpty() ) {
		HydrogenApp::get_instance()->showStatusBarMessage( s, sCaller );
	}
}

void PatternEditor::updatePosition( float fTick ) {
	if ( m_nTick == (int)fTick ) {
		return;
	}

	float fDiff = m_fGridWidth * (fTick - m_nTick);

	m_nTick = fTick;

	int nOffset = Skin::getPlayheadShaftOffset();
	int nX = static_cast<int>(static_cast<float>(PatternEditor::nMargin) +
							  static_cast<float>(m_nTick) *
							  m_fGridWidth );

	QRect updateRect( nX -2, 0, 4 + Skin::nPlayheadWidth, height() );
	update( updateRect );
	if ( fDiff > 1.0 || fDiff < -1.0 ) {
		// New cursor is far enough away from the old one that the single update rect won't cover both. So
		// update at the old location as well.
		updateRect.translate( -fDiff, 0 );
		update( updateRect );
	}
}

void PatternEditor::zoomIn()
{
	if (m_fGridWidth >= 3) {
		m_fGridWidth *= 2;
	} else {
		m_fGridWidth *= 1.5;
	}
}

void PatternEditor::zoomLasso( float fOldGridWidth ) {
	if ( m_selection.isLasso() ) {
		const float fScale = m_fGridWidth / fOldGridWidth;
		m_selection.scaleLasso( fScale, PatternEditor::nMargin );
	}
}

void PatternEditor::zoomOut()
{
	if ( m_fGridWidth > 1.5 ) {
		if (m_fGridWidth > 3) {
			m_fGridWidth /= 2;
		} else {
			m_fGridWidth /= 1.5;
		}
	}
}

void PatternEditor::keyPressEvent( QKeyEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	Editor::Base<Elem>::keyPressEvent( ev );

	// synchronize lassos
	auto pVisibleEditor = m_pPatternEditorPanel->getVisibleEditor();
	// In case we use keyboard events to _continue_ an existing lasso in
	// NotePropertiesRuler started in DrumPatternEditor (followed by moving
	// focus to NPR using tab key), DrumPatternEditor has to be used to update
	// the shared set of selected notes. Else, only notes of the current row
	// will be added after an update.
	if ( m_instance == Editor::Instance::NotePropertiesRuler &&
		 pVisibleEditor->m_selection.isLasso() && m_selection.isLasso() &&
		 dynamic_cast<DrumPatternEditor*>(pVisibleEditor) != nullptr ) {
		pVisibleEditor->m_selection.updateKeyboardCursorPosition();
		if ( pVisibleEditor->syncLasso() ) {
			updateVisibleComponents( Editor::Update::Transient );
		}
	}
}

void PatternEditor::keyReleaseEvent( QKeyEvent *ev ) {
	// Don't call updateModifiers( ev ) in here because we want to apply the
	// state of the modifiers used during the last update/rendering. Else the
	// user might position a note carefully and it jumps to different place
	// because she released the Alt modifier slightly earlier than the mouse
	// button.
}

void PatternEditor::mousePressEvent( QMouseEvent *ev ) {
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	// Property drawing in the ruler is allowed to start within the margin.
	// There is currently no plan to introduce a widget within this margin and
	// in contrast to lasso selection this action is unique to the ruler.
	if ( pEv->position().x() > m_nActiveWidth ||
		 ( pEv->position().x() <= PatternEditor::nMarginSidebar &&
		   ! ( m_instance == Editor::Instance::NotePropertiesRuler &&
			   ev->button() == Qt::RightButton ) ) ) {
		if ( ! m_selection.isEmpty() ) {
			m_selection.clearSelection();
			updateVisibleComponents( Editor::Update::Content );
		}
		return;
	}

	Editor::Base<Elem>::mousePressEvent( ev );

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	handleKeyboardCursor( false );
}

void PatternEditor::paintEvent( QPaintEvent* ev ) {
	if (!isVisible()) {
		return;
	}

	auto pPattern = m_pPatternEditorPanel->getPattern();

	const auto pPref = Preferences::get_instance();

	const qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_update == Editor::Update::Background ) {
		createBackground();
	}

	if ( m_update == Editor::Update::Background ||
		 m_update == Editor::Update::Content ) {
		drawPattern();
		m_update = Editor::Update::Transient;
	}

	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pContentPixmap,
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
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() &&
		 m_pPatternEditorPanel->hasPatternEditorFocus() &&
		 pPattern != nullptr ) {
		QColor cursorColor( pPref->getColorTheme()->m_cursorColor );
		if ( ! hasFocus() ) {
			cursorColor.setAlpha( Skin::nInactiveCursorAlpha );
		}

		QPen p( cursorColor );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setBrush( Qt::NoBrush );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( getKeyboardCursorRect(), 4, 4 );
	}
}

bool PatternEditor::checkDeselectElements(
	const std::vector<SelectionIndex>& elements )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return false;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	//	Hydrogen *pH = Hydrogen::get_instance();
	std::set< std::shared_ptr<Note> > duplicates;
	for ( const auto& ppNote : elements ) {
		if ( duplicates.find( ppNote ) != duplicates.end() ) {
			// Already marked ppNote as a duplicate of some other ppNote. Skip it.
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND_END( pPattern->getNotes(), it, ppNote->getPosition() ) {
			// Duplicate note of a selected note is anything occupying the same
			// position. Multiple notes sharing the same location might be
			// selected; we count these as duplicates too. They will appear in
			// both the duplicates and selection lists.
			if ( it->second != ppNote && ppNote->matchPosition( it->second ) ) {
				duplicates.insert( it->second );
			}
		}
	}
	if ( !duplicates.empty() ) {
		auto pPref = Preferences::get_instance();
		bool bOk = true;

		if ( pPref->getShowNoteOverwriteWarning() ) {
			m_selection.cancelGesture();
			QString sMsg ( tr( "Placing these notes here will overwrite %1 duplicate notes." ) );
			QMessageBox messageBox ( QMessageBox::Warning, "Hydrogen", sMsg.arg( duplicates.size() ),
									 QMessageBox::Cancel | QMessageBox::Ok, this );
			messageBox.setCheckBox( new QCheckBox( pCommonStrings->getMutableDialog() ) );
			messageBox.checkBox()->setChecked( false );
			bOk = messageBox.exec() == QMessageBox::Ok;
			if ( messageBox.checkBox()->isChecked() ) {
				pPref->setShowNoteOverwriteWarning( false );
			}
		}

		if ( bOk ) {
			std::vector< std::shared_ptr<Note> >overwritten;
			for ( const auto& pNote : duplicates ) {
				overwritten.push_back( pNote );
			}
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_deselectAndOverwriteNotesAction( elements, overwritten ) );

		} else {
			return false;
		}
	}
	return true;
}

// Ensure updateModifiers() was called on the input event before calling this
// action!
int PatternEditor::getCursorMargin( QInputEvent* pEvent ) const {

	// Disabled quantization is used for more fine grained control throughout
	// Hydrogen and will diminish the cursor margin.
	if ( pEvent != nullptr && ! m_pPatternEditorPanel->isQuantized() ) {
		return 0;
	}

	const int nResolution = m_pPatternEditorPanel->getResolution();
	if ( nResolution < 32 ) {
		return Editor::nDefaultCursorMargin;
	}
	else if ( nResolution < 4 * H2Core::nTicksPerQuarter ) {
		return Editor::nDefaultCursorMargin / 2;
	}
	else {
		return 0;
	}
}

QRect PatternEditor::getKeyboardCursorRect() {
	const auto pos = gridPointToPoint( getCursorPosition() );

	float fHalfWidth;
	if ( m_pPatternEditorPanel->getResolution() != 4 * H2Core::nTicksPerQuarter ) {
		// Corresponds to the distance between grid lines on 1/64 resolution.
		fHalfWidth = m_fGridWidth * 3;
	} else {
		// Corresponds to the distance between grid lines set to resolution
		// "off".
		fHalfWidth = m_fGridWidth;
	}

	// gridPointToPoint() is tailored to retrieve the position of an element
	// (note) from a grid point. As it is located in the center of a line,
	// we have to account for that.
	const int nHalfHeight = m_nGridHeight / 2;
	if ( m_instance == Editor::Instance::DrumPattern ) {
		return QRect( pos.x() - fHalfWidth, pos.y() - nHalfHeight + 2,
					  fHalfWidth * 2, m_nGridHeight - 3 );
	}
	else if ( m_instance == Editor::Instance::PianoRoll ){
		return QRect( pos.x() - fHalfWidth, pos.y() + 2,
					  fHalfWidth * 2, m_nGridHeight - 3 );
	}
	else {
		if ( hasFocus() ) {
			return QRect( pos.x() - fHalfWidth, 3, fHalfWidth * 2, height() - 6 );
		}
		else {
			// We do not have to compensate for the focus highlight.
			return QRect( pos.x() - fHalfWidth, 1, fHalfWidth * 2, height() - 2 );
		}
	}
}

// Selection manager interface
void PatternEditor::selectionMoveEndEvent( QInputEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	// Don't call updateModifiers( ev ) in here because we want to apply the
	// state of the modifiers used during the last update/rendering. Else the
	// user might position a note carefully and it jumps to different place
	// because she released the Alt modifier slightly earlier than the mouse
	// button.

	const auto offset = movingGridOffset();
	if ( offset.getColumn() == 0 && offset.getRow() == 0 ) {
		// Move with no effect.
		return;
	}

	validateSelection();

	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();

	 auto pHydrogenApp = HydrogenApp::get_instance();

	if ( m_bCopyNotMove ) {
		pHydrogenApp->beginUndoMacro( tr( "copy notes" ) );
	} else {
		pHydrogenApp->beginUndoMacro( tr( "move notes" ) );
	}
	std::list< std::shared_ptr<Note> > selectedNotes;
	for ( auto pNote : m_selection ) {
		selectedNotes.push_back( pNote );
	}

	// When moving a bulk of notes, it is important to delete them all first.
	// Moving notes one by one could cause note loss in case the target grid
	// point of an early point is the source grid point of a latter one.
	std::vector<QUndoCommand*> deleteActions, addActions;

	for ( auto pNote : selectedNotes ) {
		if ( pNote == nullptr ) {
			continue;
		}
		const int nPosition = pNote->getPosition();
		const int nNewPosition = nPosition + offset.getColumn();

		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
		int nNewRow = nRow;
		// For all other editors the moved/copied note is still associated to
		// the same instrument.
		if ( m_instance == Editor::Instance::DrumPattern &&
			 offset.getRow() != 0 ) {
			nNewRow += offset.getRow();
		}
		const auto row = m_pPatternEditorPanel->getRowDB( nRow );
		const auto newRow = m_pPatternEditorPanel->getRowDB( nNewRow );

		auto newKey = pNote->getKey();
		auto newOctave = pNote->getOctave();
		int nNewPitch = pNote->getPitchFromKeyOctave();
		if ( m_instance == Editor::Instance::PianoRoll && offset.getRow() != 0 ) {
			nNewPitch -= offset.getRow();
			newKey = Note::pitchToKey( nNewPitch );
			newOctave = Note::pitchToOctave( nNewPitch );
		}

		// For NotePropertiesRuler there is no vertical displacement.
		bool bNoteInRange =
			nNewPosition >= 0 && nNewPosition <= pPattern->getLength();
		if ( m_instance == Editor::Instance::DrumPattern ) {
			bNoteInRange = bNoteInRange && nNewRow >= 0 &&
						   nNewRow <= m_pPatternEditorPanel->getRowNumberDB();
		}
		else if ( m_instance == Editor::Instance::PianoRoll ) {
			bNoteInRange = bNoteInRange &&
						   static_cast<int>( newOctave ) >=
							   static_cast<int>( Note::OctaveMin ) &&
						   static_cast<int>( newOctave ) <=
							   static_cast<int>( Note::OctaveMax );
		}

		// Cache note properties since a potential first note deletion will also
		// call the note's destructor.
		const int nLength = pNote->getLength();
		const float fVelocity = pNote->getVelocity();
		const float fPan = pNote->getPan();
		const float fLeadLag = pNote->getLeadLag();
		const auto key = pNote->getKey();
		const auto octave = pNote->getOctave();
		const float fProbability = pNote->getProbability();
		const bool bNoteOff = pNote->getNoteOff();
		const bool bIsMappedToDrumkit = pNote->getInstrument() != nullptr;

		// We'll either select the new, duplicated note or the new, moved
		// replacement of the note.
		m_selection.removeFromSelection( pNote, false );

		if ( ! m_bCopyNotMove ) {
			// Note is moved either out of range or to a new position. Delete
			// the note at the source position.
			deleteActions.push_back(
				new SE_addOrRemoveNoteAction(
					nPosition,
					pNote->getInstrumentId(),
					pNote->getType(),
					m_pPatternEditorPanel->getPatternNumber(),
					nLength,
					fVelocity,
					fPan,
					fLeadLag,
					key,
					octave,
					fProbability,
					Editor::Action::Delete,
					bNoteOff,
					bIsMappedToDrumkit,
					Editor::ActionModifier::None ) );
		}

		auto modifier = Editor::ActionModifier::AddToSelection;
		// Check whether the note was hovered when the drag move action was
		// started. If so, we will move the keyboard cursor to the resulting
		// position.
		for ( const auto ppHoveredNote : m_elementsHoveredOnDragStart ) {
			if ( ppHoveredNote == pNote ) {
				modifier = static_cast<Editor::ActionModifier>(
					static_cast<char>(Editor::ActionModifier::AddToSelection) |
					static_cast<char>(Editor::ActionModifier::MoveCursorTo) );
				break;
			}
		}

		if ( bNoteInRange ) {
			// Create a new note at the target position
			addActions.push_back(
				new SE_addOrRemoveNoteAction(
					nNewPosition,
					newRow.id,
					newRow.sType,
					m_pPatternEditorPanel->getPatternNumber(),
					nLength,
					fVelocity,
					fPan,
					fLeadLag,
					newKey,
					newOctave,
					fProbability,
					Editor::Action::Add,
					bNoteOff,
					bIsMappedToDrumkit,
					modifier ) );
		}
	}

	for ( const auto ppAction : deleteActions ) {
		pHydrogenApp->pushUndoCommand( ppAction );
	}
	for ( const auto ppAction : addActions ) {
		pHydrogenApp->pushUndoCommand( ppAction );
	}

	// Selecting the clicked row
	auto pMouseEvent = dynamic_cast<QMouseEvent*>(ev);
	if ( pMouseEvent != nullptr ) {
		const auto gridPoint = pointToGridPoint( pMouseEvent->pos(), true );

		if ( m_instance == Editor::Instance::DrumPattern ) {
			m_pPatternEditorPanel->setSelectedRowDB( gridPoint.getRow() );
		}
		else if ( m_instance == Editor::Instance::PianoRoll ) {
			setCursorPitch( Note::lineToPitch( gridPoint.getRow() ) );
		}

		auto hoveredNotes = getElementsAtPoint(
			pMouseEvent->pos(), Editor::InputSource::Mouse,
			getCursorMargin( ev ), true
		);
		if ( hoveredNotes.size() > 0 ) {
			m_pPatternEditorPanel->setCursorColumn(
				hoveredNotes[0]->getPosition(), true
			);
		}
	}

	pHydrogenApp->endUndoMacro();
}

///
/// Ensure selection only refers to valid notes, and does not contain any stale references to deleted notes.
///
void PatternEditor::validateSelection() {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	// Rebuild selection from valid notes.
	std::set< std::shared_ptr<Note> > valid;
	std::vector< std::shared_ptr<Note> > invalidated;
	FOREACH_NOTE_CST_IT_BEGIN_END(pPattern->getNotes(), it) {
		if ( m_selection.isSelected( it->second ) ) {
			valid.insert( it->second );
		}
	}
	for (auto i : m_selection ) {
		if ( valid.find(i) == valid.end()) {
			// Keep the note to invalidate, but don't remove from the selection while walking the selection
			// set.
			invalidated.push_back( i );
		}
	}
	for ( auto i : invalidated ) {
		m_selection.removeFromSelection( i, /* bCheck=*/false );
	}
}

void PatternEditor::handleElements( QInputEvent* ev, Editor::Action action )
{
	if ( action == Editor::Action::None ) {
		return;
	}

	// Retrieve the coordinates
	GridPoint gridPoint;
	if ( dynamic_cast<QMouseEvent*>( ev ) != nullptr ) {
		// Element added via mouse.
		auto pEv = static_cast<MouseEvent*>( ev );

		auto notesAtPoint = getElementsAtPoint(
			pEv->position().toPoint(), Editor::InputSource::Mouse,
			getCursorMargin( ev ), true
		);
		if ( notesAtPoint.size() > 0 ) {
			if ( action == Editor::Action::Delete ||
				 action == Editor::Action::Toggle ) {
				deleteElements( notesAtPoint );
			}
			return;
		}

		// Nothing found at point. Add a new note.
		gridPoint = pointToGridPoint( pEv->position().toPoint(), true );
		if ( m_instance == Editor::Instance::PianoRoll ) {
			// Ensure we add the new note at the right spot.
			setCursorPitch( Note::lineToPitch( gridPoint.getRow() ) );
		}
	}
	else if ( dynamic_cast<QKeyEvent*>( ev ) != nullptr ) {
		gridPoint.setColumn( m_pPatternEditorPanel->getCursorColumn() );
		gridPoint.setRow( m_pPatternEditorPanel->getSelectedRowDB() );
	}
	else {
		ERRORLOG( "Unknown event" );
		return;
	}

	if ( m_instance != Editor::Instance::DrumPattern ) {
		gridPoint.setRow( m_pPatternEditorPanel->getSelectedRowDB() );
	}

	auto key = Note::Key::Invalid;
	auto octave = Note::Octave::Invalid;
	if ( m_instance == Editor::Instance::PianoRoll ) {
		// Use the row of the DrumPatternEditor/DB for further note
		// interactions.
		octave = Note::pitchToOctave( m_nCursorPitch );
		key = Note::pitchToKey( m_nCursorPitch );
	}

	const bool bNoteOff = ev->modifiers() & Qt::ShiftModifier;

	float fYValue = std::nan( "" );
	auto property = m_property;
	if ( m_instance == Editor::Instance::NotePropertiesRuler &&
		 dynamic_cast<QMouseEvent*>( ev ) != nullptr ) {
		fYValue = static_cast<NotePropertiesRuler*>( this )->eventToYValue(
			dynamic_cast<QMouseEvent*>( ev )
		);

        // Ensure to add distinct notes when clicking the KeyOctave view in the
        // ruler.
		NotePropertiesRuler::yToKeyOctave( fYValue, &key, &octave );
		if ( key == Note::Key::Invalid ) {
            key = Note::KeyMin;
		}
		if ( octave == Note::Octave::Invalid ) {
            octave = Note::OctaveDefault;
		}
	}

	m_pPatternEditorPanel->addOrRemoveNotes(
		gridPoint, key, octave, bNoteOff, fYValue, property, action,
		static_cast<Editor::ActionModifier>(
			static_cast<int>( Editor::ActionModifier::Playback ) |
			static_cast<int>( Editor::ActionModifier::MoveCursorTo )
		)
	);
}

void PatternEditor::deleteElements(
	std::vector< std::shared_ptr<H2Core::Note> > notes )
{
	if ( notes.size() == 0 ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	pHydrogenApp->beginUndoMacro( pCommonStrings->getActionDeleteNotes() );
	for ( const auto& ppNote : notes ) {
		pHydrogenApp->pushUndoCommand(
			new SE_addOrRemoveNoteAction(
				ppNote->getPosition(),
				ppNote->getInstrumentId(),
				ppNote->getType(),
				m_pPatternEditorPanel->getPatternNumber(),
				ppNote->getLength(),
				ppNote->getVelocity(),
				ppNote->getPan(),
				ppNote->getLeadLag(),
				ppNote->getKey(),
				ppNote->getOctave(),
				ppNote->getProbability(),
				Editor::Action::Delete,
				/* bIsNoteOff */ ppNote->getNoteOff(),
				ppNote->getInstrument() != nullptr,
				Editor::ActionModifier::None ) );
	}
	pHydrogenApp->endUndoMacro();
}

std::vector<std::shared_ptr<Note> > PatternEditor::getElementsAtPoint(
	const QPoint& point,
	Editor::InputSource inputSource,
	int nCursorMargin,
	bool bIncludeHovered,
	std::shared_ptr<H2Core::Pattern> pPattern
)
{
	std::vector< std::shared_ptr<Note> > notesUnderPoint;
	if ( pPattern == nullptr ) {
		pPattern = m_pPatternEditorPanel->getPattern();
		if ( pPattern == nullptr ) {
			return std::move( notesUnderPoint );
		}
	}

	const auto gridPoint = pointToGridPoint( point, false );
	const auto gridPointLower = pointToGridPoint(
		point - QPoint( nCursorMargin, 0 ), false );
	const auto gridPointUpper = pointToGridPoint(
		point + QPoint( nCursorMargin, 0 ), false );

	// Assemble all notes to be edited.
	DrumPatternRow row;
	if ( m_instance == Editor::Instance::DrumPattern ) {
		row = m_pPatternEditorPanel->getRowDB( gridPoint.getRow() );
	}
	else {
		row = m_pPatternEditorPanel->getRowDB(
			m_pPatternEditorPanel->getSelectedRowDB() );
	}

	// Prior to version 2.0 notes were selected by clicking its grid cell,
	// while this caused only notes on the current grid to be accessible it also
	// made them quite easy select. Just using the position of the mouse cursor
	// would feel like a regression, as it would be way harded to hit the notes.
	// Instead, we introduce a certain rectangle (manhattan distance) around the
	// cursor which can select notes but only return those nearest to the
	// center.
	int nLastDistance = gridPointUpper.getColumn() - gridPoint.getColumn() + 1;

	// We have to ensure to only provide notes from a single position. In case
	// the cursor is placed exactly in the middle of two notes, the left one
	// wins.
	int nLastPosition = -1;

	const auto notes = pPattern->getNotes();
	for ( auto it = notes->lower_bound( gridPointLower.getColumn() );
		  it != notes->end() && it->first <= gridPointUpper.getColumn(); ++it ) {
		const auto ppNote = it->second;
		if ( ppNote != nullptr && row.contains( ppNote ) &&
			 ppNote->getPosition() < pPattern->getLength() ) {

			const int nDistance =
				std::abs( ppNote->getPosition() - gridPoint.getColumn() );

			if ( nDistance < nLastDistance ) {
				// This note is nearer than (potential) previous ones.
				notesUnderPoint.clear();
				nLastDistance = nDistance;
				nLastPosition = ppNote->getPosition();
			}

			if ( nDistance <= nLastDistance &&
				 ppNote->getPosition() == nLastPosition ) {
				// In case of the PianoRoll BaseEditor::editor we do have to additionally
				// differentiate between different pitches.
				if ( m_instance != Editor::Instance::PianoRoll ||
					 ( m_instance == Editor::Instance::PianoRoll &&
					   ppNote->getKey() ==
					   Note::pitchToKey( Note::lineToPitch( gridPoint.getRow() ) ) &&
					   ppNote->getOctave() ==
					   Note::pitchToOctave( Note::lineToPitch( gridPoint.getRow() ) ) ) ) {
					notesUnderPoint.push_back( ppNote );
				}
			}
		}
	}

	return std::move( notesUnderPoint );
}

QPoint PatternEditor::elementToPoint( std::shared_ptr<Note> pNote ) const {
	if ( pNote == nullptr ) {
		return QPoint();
	}

	GridPoint gridPoint( pNote->getPosition(),
						 m_pPatternEditorPanel->findRowDB( pNote ) );

	return gridPointToPoint( gridPoint );
}

QPoint PatternEditor::gridPointToPoint( const GridPoint& gridPoint ) const {
	return QPoint(
		PatternEditor::nMargin + gridPoint.getColumn() * m_fGridWidth,
		gridPoint.getRow() * m_nGridHeight + m_nGridHeight / 2 );
}

GridPoint PatternEditor::pointToGridPoint( const QPoint& point,
										   bool bHonorQuantization ) const {
	const int nRow = static_cast<int>(
		std::floor( static_cast<float>(point.y()) /
					static_cast<float>(m_nGridHeight) ) );

	int nColumn = 0;
	if ( point.x() > PatternEditor::nMargin ) {
		int nGranularity = 1;
		if ( bHonorQuantization && m_pPatternEditorPanel->isQuantized() ) {
			nGranularity = granularity();
		}
		const int nWidth = m_fGridWidth * nGranularity;
		// We add half the distance between two grid points (nWidth/2) in order
		// for point.x() to be "rounded" to the nearest grid point.
		nColumn = std::round(
			( point.x() - PatternEditor::nMargin + (nWidth / 2) ) /
			nWidth ) * nGranularity ;
		nColumn = std::max( 0, nColumn );
	}

	return GridPoint( nColumn, nRow );
}

void PatternEditor::copy() {
	XMLDoc doc;
	XMLNode selection = doc.set_root( "noteSelection" );
	XMLNode noteList = selection.createNode( "noteList");
	XMLNode positionNode = selection.createNode( "sourcePosition" );
	bool bWroteNote = false;
	// "Top left" of selection, in the three dimensional time*instrument*pitch space.
	int nMinColumn, nMinRow, nMaxPitch;

	for ( const auto& ppNote : m_selection ) {
		const int nPitch = ppNote->getPitchFromKeyOctave();
		const int nColumn = ppNote->getPosition();
		const int nRow = m_pPatternEditorPanel->findRowDB( ppNote );
		if ( bWroteNote ) {
			nMinColumn = std::min( nColumn, nMinColumn );
			nMinRow = std::min( nRow, nMinRow );
			nMaxPitch = std::max( nPitch, nMaxPitch );
		} else {
			nMinColumn = nColumn;
			nMinRow = nRow;
			nMaxPitch = nPitch;
			bWroteNote = true;
		}
		XMLNode note_node = noteList.createNode( "note" );
		ppNote->saveTo( note_node );
	}

	if ( bWroteNote ) {
		positionNode.write_int( "minColumn", nMinColumn );
		positionNode.write_int( "minRow", nMinRow );
		positionNode.write_int( "maxPitch", nMaxPitch );
	} else {
		positionNode.write_int( "minColumn",
								m_pPatternEditorPanel->getCursorColumn() );
		positionNode.write_int( "minRow",
								m_pPatternEditorPanel->getSelectedRowDB() );
	}

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText( doc.toString() );
}

void PatternEditor::paste() {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	QClipboard *clipboard = QApplication::clipboard();
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();
	const auto selectedRow = m_pPatternEditorPanel->getRowDB( nSelectedRow );
	if ( selectedRow.id == Instrument::EmptyId &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row" );
		return;
	}

	XMLNode noteList;
	int nDeltaPos = 0, nDeltaRow = 0, nDeltaPitch = 0;

	XMLDoc doc;
	if ( ! doc.setContent( clipboard->text() ) ) {
		// Pasted something that's not valid XML.
		return;
	}

	XMLNode selection = doc.firstChildElement( "noteSelection" );
	if ( ! selection.isNull() ) {

		// Got a noteSelection.
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
		if ( ! positionNode.isNull() ) {
			int nCurrentPos = m_pPatternEditorPanel->getCursorColumn();
			nDeltaPos = nCurrentPos -
				positionNode.read_int( "minColumn", nCurrentPos );

			// In NotePropertiesRuler there is no vertical offset.
			if ( m_instance == Editor::Instance::PianoRoll ) {
				nDeltaPitch = m_nCursorPitch -
					positionNode.read_int( "maxPitch", m_nCursorPitch );
			}
			else if ( m_instance == Editor::Instance::DrumPattern ) {
				nDeltaRow = nSelectedRow -
					positionNode.read_int( "minRow", nSelectedRow );
			}
		}
	}
	else {

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
				QMessageBox::information( this, "Hydrogen",
										  tr( "Cannot paste multi-pattern selection" ) );
				return;
			}
			noteList = pattern.firstChildElement( "noteList" );
			if ( noteList.isNull() ) {
				return;
			}
		}
	}

	m_selection.clearSelection();
	bool bAppendedToDB = false;

	if ( noteList.hasChildNodes() ) {

		pHydrogenApp->beginUndoMacro( tr( "paste notes" ) );
		for ( XMLNode n = noteList.firstChildElement( "note" ); ! n.isNull();
			  n = n.nextSiblingElement() ) {
			auto pNote = Note::loadFrom( n );
			if ( pNote == nullptr ) {
				ERRORLOG( QString( "Unable to load note from XML node [%1]" )
						  .arg( n.toQString() ) );
				continue;
			}

			const int nPos = pNote->getPosition() + nDeltaPos;
			if ( nPos < 0 || nPos >= pPattern->getLength() ) {
				continue;
			}

			Instrument::Id id;
			Instrument::Type sType;
			DrumPatternRow targetRow;
			if ( m_instance == Editor::Instance::DrumPattern ) {
				const auto nNoteRow =
					m_pPatternEditorPanel->findRowDB( pNote, true );
				if ( nNoteRow != -1 ) {
					// Note belongs to a row already present in the DB.
					const int nRow = nNoteRow + nDeltaRow;
					if ( nRow < 0 ||
						 nRow >= m_pPatternEditorPanel->getRowNumberDB() ) {
						continue;
					}
					targetRow = m_pPatternEditorPanel->getRowDB( nRow );
					id = targetRow.id;
					sType = targetRow.sType;
				}
				else {
					// Note can not be represented in the current DB. This means
					// it might be a type-only one copied from a a different
					// pattern. We will append it to the DB.
					id = pNote->getInstrumentId();
					sType = pNote->getType();
					bAppendedToDB = true;
				}
			}
			else {
				targetRow = m_pPatternEditorPanel->getRowDB( nSelectedRow );
				id = targetRow.id;
				sType = targetRow.sType;
			}

			Note::Octave octave;
            Note::Key key;
			if ( m_instance == Editor::Instance::PianoRoll ) {
				const int nPitch = pNote->getPitchFromKeyOctave() + nDeltaPitch;
				if ( nPitch < KEYS_PER_OCTAVE *
								  static_cast<int>( Note::OctaveMin ) ||
					 nPitch >=
						 KEYS_PER_OCTAVE *
							 ( static_cast<int>( Note::OctaveMax ) + 1 ) ) {
					continue;
				}

				key = Note::pitchToKey( nPitch );
				octave = Note::pitchToOctave( nPitch );
			}
			else {
				key = pNote->getKey();
				octave = pNote->getOctave();
			}

			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemoveNoteAction(
					nPos,
					id,
					sType,
					m_pPatternEditorPanel->getPatternNumber(),
					pNote->getLength(),
					pNote->getVelocity(),
					pNote->getPan(),
					pNote->getLeadLag(),
					key,
					octave,
					pNote->getProbability(),
					Editor::Action::Add,
					/* bIsNoteOff */ pNote->getNoteOff(),
					targetRow.bMappedToDrumkit,
					Editor::ActionModifier::AddToSelection ) );
		}
		pHydrogenApp->endUndoMacro();
	}

	if ( bAppendedToDB ) {
		// We added a note to the pattern currently not represented by the DB.
		// We have to force its update in order to avoid inconsistencies.
		const int nOldSize = m_pPatternEditorPanel->getRowNumberDB();
		m_pPatternEditorPanel->updateDB();
		m_pPatternEditorPanel->updateEditors( Editor::Update::Content );
		m_pPatternEditorPanel->resizeEvent( nullptr );

		// Select the append line
		m_pPatternEditorPanel->setSelectedRowDB( nOldSize );
	}
}

void PatternEditor::ensureCursorIsVisible() {
	m_pPatternEditorPanel->ensureCursorIsVisible();
}

GridPoint PatternEditor::getCursorPosition() const {
	return GridPoint( m_pPatternEditorPanel->getCursorColumn(),
					  m_pPatternEditorPanel->getSelectedRowDB() );
}

void PatternEditor::moveCursorLeft( QKeyEvent* ev, Editor::Step step ) {
	m_pPatternEditorPanel->moveCursorLeft( ev, step );
}

void PatternEditor::moveCursorRight( QKeyEvent* ev, Editor::Step step ) {
	m_pPatternEditorPanel->moveCursorRight( ev, step );
}

void PatternEditor::setCursorTo( std::shared_ptr<H2Core::Note> pNote ) {
	if ( pNote == nullptr ) {
		return;
	}

	m_pPatternEditorPanel->setCursorColumn( pNote->getPosition() );
}

void PatternEditor::setCursorTo( QMouseEvent* ev ) {
	auto pEv = static_cast<MouseEvent*>( ev );

	const auto gridPoint = pointToGridPoint( pEv->position().toPoint(), true );

	m_pPatternEditorPanel->setCursorColumn( gridPoint.getColumn() );
}

bool PatternEditor::updateKeyboardHoveredElements() {
	std::vector< std::pair< std::shared_ptr<Pattern>,
							std::vector< std::shared_ptr<Note> > > > hovered;
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// cursor visible

		PatternEditor* pEditor = this;
		if ( m_instance == Editor::Instance::NotePropertiesRuler ) {
			auto pVisibleEditor = m_pPatternEditorPanel->getVisibleEditor();
			if ( dynamic_cast<DrumPatternEditor*>( pVisibleEditor ) != nullptr ) {
				pEditor = pVisibleEditor;
			}
		}

		const auto point = pEditor->gridPointToPoint(
			pEditor->getCursorPosition() );

		for ( const auto& ppPattern :
			  m_pPatternEditorPanel->getPatternsToShow() ) {
			const auto hoveredNotes = pEditor->getElementsAtPoint(
				point, Editor::InputSource::Keyboard, 0, false, ppPattern
			);
			if ( hoveredNotes.size() > 0 ) {
				hovered.push_back( std::make_pair( ppPattern, hoveredNotes ) );
			}
		}
	}

	return m_pPatternEditorPanel->setHoveredNotesKeyboard( hovered );
}

bool PatternEditor::updateMouseHoveredElements( QMouseEvent* ev ) {

	// Check whether the mouse pointer is Outside of the current widget.
	const QPoint globalPos = QCursor::pos();
	const QPoint widgetPos = mapFromGlobal( globalPos );
	if ( widgetPos.x() < 0 || widgetPos.x() >= width() ||
		 widgetPos.y() < 0 || widgetPos.y() >= height() ) {
		// Clear all hovered notes.
		std::vector< std::pair< std::shared_ptr<Pattern>,
								std::vector< std::shared_ptr<Note> > > > empty;
		return m_pPatternEditorPanel->setHoveredNotesMouse( empty );
	}

	if ( ev == nullptr ) {
		// The update was triggered outside of one of Qt's mouse events. We have
		// to create an artifical one instead.
		ev = new QMouseEvent(
			QEvent::MouseButtonRelease, widgetPos, globalPos, Qt::LeftButton,
			Qt::LeftButton, Qt::NoModifier );
	}
	const auto pEv = static_cast<MouseEvent*>( ev );
	const int nCursorMargin = getCursorMargin( ev );

	const auto gridPoint = pointToGridPoint( pEv->position().toPoint(), false );
	const auto gridPointUpper = pointToGridPoint(
		pEv->position().toPoint() + QPoint( nCursorMargin, 0 ), false );

	// getElementsAtPoint is generous in finding notes by taking a margin around
	// the cursor into account as well. We have to ensure we only use to closest
	// notes reported.
	int nLastDistance = gridPointUpper.getColumn() - gridPoint.getColumn() + 1;

	// In addition, we have to ensure to only provide notes from a single
	// position. In case the cursor is placed exactly in the middle of two
	// notes, the left one wins.
	int nLastPosition = -1;

	std::vector< std::pair< std::shared_ptr<Pattern>,
							std::vector< std::shared_ptr<Note> > > > hovered;
	// We do not highlight hovered notes during a property drag. Else, the
	// hovered ones would appear in front of the dragged one in the ruler,
	// hiding the newly adjusted value.
	if ( m_dragType == DragType::None &&
		 pEv->position().x() > PatternEditor::nMarginSidebar ) {
		for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
			const auto hoveredNotes = getElementsAtPoint(
				pEv->position().toPoint(), Editor::InputSource::Mouse,
				nCursorMargin, false, ppPattern
			);
			if ( hoveredNotes.size() > 0 ) {
				const int nDistance = std::abs(
					hoveredNotes[ 0 ]->getPosition() - gridPoint.getColumn() );
				if ( nDistance < nLastDistance ) {
					// This batch of notes is nearer than (potential) previous ones.
					hovered.clear();
					nLastDistance = nDistance;
					nLastPosition = hoveredNotes[ 0 ]->getPosition();
				}

				if ( hoveredNotes[ 0 ]->getPosition() == nLastPosition ) {
					hovered.push_back( std::make_pair( ppPattern, hoveredNotes ) );
				}
			}
		}
	}

	return m_pPatternEditorPanel->setHoveredNotesMouse( hovered );
}

Editor::Input PatternEditor::getInput() const {
	return m_pPatternEditorPanel->getInput();
}

void PatternEditor::mouseDrawStart( QMouseEvent* ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );


	m_drawPreviousPosition = pEv->position();
}

void PatternEditor::mouseDrawUpdate( QMouseEvent* ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	// When creating a lasso to select elements or adjusting a property of
	// elements using the delta of a cursor movement, it make sense to move out
	// of the editor. But in drawing mode we just want to create/delete elements
	// witin the editor.
	const QPointF end(
		std::clamp( static_cast<int>(pEv->position().x()), 0, width() ),
		std::clamp( static_cast<int>(pEv->position().y()), 0, height() ) );
	const int nCursorMargin = getCursorMargin( ev );

	auto pointToRowColumn = [&]( QPoint point, GridPoint* pGridPoint,
								 Note::Key* key, Note::Octave* octave ) {
		const auto notes = getElementsAtPoint(
			point, Editor::InputSource::Mouse, nCursorMargin, true, pPattern
		);
		if ( notes.size() > 0 && notes[0] != nullptr ) {
			pGridPoint->setColumn( notes[ 0 ]->getPosition() );
			if ( m_instance == Editor::Instance::DrumPattern ) {
				pGridPoint->setRow( m_pPatternEditorPanel->findRowDB( notes[ 0 ] ) );
				*key = Note::KeyMin;
				*octave = Note::OctaveDefault;
			}
			else {
				pGridPoint->setRow( m_pPatternEditorPanel->getSelectedRowDB() );
				*key = notes[ 0 ]->getKey();
				*octave = notes[ 0 ]->getOctave();
			}
		}
		else {
			// Determine the point on the grid to toggle the note
			*pGridPoint = pointToGridPoint( point, true );
			if ( m_instance == Editor::Instance::DrumPattern ) {
				*key = Note::KeyMin;
				*octave = Note::OctaveDefault;
			}
			else {
				const auto nPitch = Note::lineToPitch( pGridPoint->getRow() );
				pGridPoint->setRow( m_pPatternEditorPanel->getSelectedRowDB() );
				*key = Note::pitchToKey( nPitch );
				*octave = Note::pitchToOctave( nPitch );
			}
		}
	};

	// Check whether we are still at the same grid point as in the last update.
	// We do not want to toggle the same note twice.
	Note::Octave endOctave;
    Note::Key endKey;
	GridPoint endGridPoint;
	pointToRowColumn( end.toPoint(), &endGridPoint, &endKey, &endOctave );
	if ( endGridPoint == m_drawPreviousGridPoint &&
		 endKey == m_drawPreviousKey && endOctave == m_drawPreviousOctave ) {
		m_drawPreviousPosition = end;
		return;
	}

	// Toggle all notes between this and the previous position in individual
	// undo/redo actions bundled into a single undo macro.

	const auto start = m_drawPreviousPosition;
	const auto sUndoContext =
		QString( "%1::draw" ).arg( Editor::instanceToQString( m_instance ) );

	// We assume the cursor path was a straight line between both points
	// ( y = fM * x + fN ).
	double fM;
	if ( end.x() != start.x() ) {
		fM = std::min( std::abs( ( end.y() - start.y() ) /
								 ( end.x() - start.x() ) ),
					   static_cast<double>(m_nGridHeight) / 2 );
	} else {
		fM = static_cast<double>(m_nGridHeight) / 2;
	}

	// Since we have to properly handle all hovered notes (already present) we
	// have to assume the smallest possible resolution: grid on the x axis being
	// turned off. We will project this smallest increment onto the straight
	// line while ensuring we do not miss rows on almost vertical movements to
	// get our increment.
	const QPointF increment( start.x() <= end.x() ? 1 : -1,
							 start.y() <= end.y() ? fM : ( -1 * fM ) );

	GridPoint gridPoint;
	Note::Octave octave;
    Note::Key key;
	GridPoint lastGridPoint( m_drawPreviousGridPoint );
	auto lastKey = m_drawPreviousKey;
	auto lastOctave = m_drawPreviousOctave;
	// Since we can only toggle notes on the grid, we use the projection of the
	// movement on the x axis to drive the loop. This ensures that we are always
	// on grid.
	for ( auto ppoint = start; ( ppoint - start ).manhattanLength() <=
			  ( end - start ).manhattanLength(); ppoint += increment ) {
		// We prioritize existing notes
		pointToRowColumn( ppoint.toPoint(), &gridPoint, &key, &octave );

		if ( gridPoint != lastGridPoint || key != lastKey ||
			 octave != lastOctave ) {
			m_pPatternEditorPanel->addOrRemoveNotes(
				gridPoint, key, octave,
				/* bNoteOff */ ev->modifiers() & Qt::ShiftModifier,
				std::nan( "" ), Property::None,
				Editor::Action::Toggle,
				Editor::ActionModifier::Playback, sUndoContext );
			lastGridPoint = gridPoint;
			lastKey = key;
			lastOctave = octave;
		}
	}

	m_drawPreviousPosition = end;
	m_drawPreviousGridPoint = lastGridPoint;
	m_drawPreviousKey = lastKey;
	m_drawPreviousOctave = lastOctave;
}

void PatternEditor::mouseDrawEnd() {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	HydrogenApp::get_instance()->endUndoContext();

	// Drawing can result in duplicated notes - e.g. creating many notes in a
	// column of the PianoRollEditor and drawing in the KeyOctave section of the
	// NotePropertiesRuler. We have to check all notes.
	std::vector< std::shared_ptr<Note> > notes;
	for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
		if ( ppNote != nullptr ) {
			notes.push_back( ppNote );
		}
	}
	if ( notes.size() > 0 ) {
		checkDeselectElements( notes );
	}

	m_drawPreviousPosition = QPointF( 0, 0 );
	m_drawPreviousGridPoint = GridPoint( -1, -1 );
}

void PatternEditor::mouseEditStart( QMouseEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	m_property = m_pPatternEditorPanel->getSelectedNoteProperty();

	// Adjusting note properties.
	const auto notesAtPoint = getElementsAtPoint(
		pEv->position().toPoint(), Editor::InputSource::Mouse,
		getCursorMargin( ev ), true
	);
	if ( notesAtPoint.size() == 0 ) {
		return;
	}

	// Focus cursor on dragged note(s).
	m_pPatternEditorPanel->setCursorColumn(
		notesAtPoint[ 0 ]->getPosition() );
	m_pPatternEditorPanel->setSelectedRowDB(
		m_pPatternEditorPanel->findRowDB( notesAtPoint[ 0 ] ) );

	m_draggedNotes.clear();
	// Either all or none of the notes at point should be selected. It is safe
	// to just check the first one.
	if ( m_selection.isSelected( notesAtPoint[ 0 ] ) ) {
		// The clicked note is part of the current selection. All selected notes
		// will be edited.
		for ( const auto& ppNote : m_selection ) {
			if ( ppNote != nullptr &&
				 ! ( ppNote->getNoteOff() &&
					 ( m_property != Property::LeadLag &&
					   m_property != Property::Probability ) ) ) {
				m_draggedNotes[ ppNote ] = std::make_shared<Note>( ppNote );
			}
		}
	}
	else {
		for ( const auto& ppNote : notesAtPoint ) {
			// NoteOff notes can have both a custom lead/lag and probability.
			// But all other properties won't take effect.
			if ( ! ( ppNote->getNoteOff() &&
					 ( m_property != Property::LeadLag &&
					   m_property != Property::Probability ) ) ) {
				m_draggedNotes[ ppNote ] = std::make_shared<Note>( ppNote );
			}
		}
	}
	// All notes at located at the same point.
	m_nDragStartColumn = notesAtPoint[ 0 ]->getPosition();
	m_nDragY = pEv->position().y();
	m_dragStart = pEv->position().toPoint();
}

void PatternEditor::mouseEditUpdate( QMouseEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr || m_draggedNotes.size() == 0 ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	auto pHydrogen = Hydrogen::get_instance();

	const auto gridPoint = pointToGridPoint( pEv->position().toPoint(), true );

	// In case this is the first drag update, decided whether we deal with a
	// length or property drag.
	if ( m_dragType == DragType::None ) {
		const int nDiffY = std::abs( pEv->position().y() - m_dragStart.y() );
		const int nDiffX = std::abs( pEv->position().x() - m_dragStart.x() );

		if ( nDiffX == nDiffY ) {
			// User is dragging diagonally and hasn't decided yet.
			return;
		}
		else if ( nDiffX > nDiffY ) {
			m_dragType = DragType::Length;
		}
		else {
			m_dragType = DragType::Property;
		}
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	int nLen = gridPoint.getColumn() - m_nDragStartColumn;

	if ( nLen <= 0 ) {
		nLen = -1;
	}

	for ( auto& [ ppNote, _ ] : m_draggedNotes ) {
		if ( m_dragType == DragType::Length ) {
			float fStep = 1.0;
			if ( nLen > -1 ){
				fStep = Note::pitchToFrequency( ppNote->getPitchFromKeyOctave() );
			}
			ppNote->setLength( nLen * fStep );

			triggerStatusMessage( m_elementsHoveredOnDragStart, Property::Length );
		}
		else if ( m_dragType == DragType::Property &&
				  m_property != Property::KeyOctave ) {
			// edit note property. We do not support the note key property.
			float fValue = 0.0;
			if ( m_property == Property::Velocity ) {
				fValue = ppNote->getVelocity();
			}
			else if ( m_property == Property::Pan ) {
				fValue = ppNote->getPanWithRangeFrom0To1();
			}
			else if ( m_property == Property::LeadLag ) {
				fValue = ( ppNote->getLeadLag() - 1.0 ) / -2.0 ;
			}
			else if ( m_property == Property::Probability ) {
				fValue = ppNote->getProbability();
			}

			fValue = fValue +
				static_cast<float>(m_nDragY - pEv->position().y()) / 100;
			if ( fValue > 1 ) {
				fValue = 1;
			}
			else if ( fValue < 0.0 ) {
				fValue = 0.0;
			}

			if ( m_property == Property::Velocity ) {
				ppNote->setVelocity( fValue );
			}
			else if ( m_property == Property::Pan ) {
				ppNote->setPanWithRangeFrom0To1( fValue );
			}
			else if ( m_property == Property::LeadLag ) {
				ppNote->setLeadLag( ( fValue * -2.0 ) + 1.0 );
			}
			else if ( m_property == Property::Probability ) {
				ppNote->setProbability( fValue );
			}

			triggerStatusMessage( m_elementsHoveredOnDragStart, m_property );
		}
	}

	m_nDragY = pEv->position().y();

	pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
	pHydrogen->setIsModified( true );

	updateVisibleComponents( Editor::Update::Content );
}

void PatternEditor::mouseEditEnd() {

	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		m_dragType = DragType::None;
		return;
	}

	if ( m_draggedNotes.size() == 0 ||
		 ( m_dragType == DragType::Property &&
		   m_property == Property::KeyOctave ) ) {
		m_dragType = DragType::None;
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	bool bMacroStarted = false;
	if ( m_draggedNotes.size() > 1 ) {

		auto sMacro = tr( "Drag edit note property:" );
		if ( m_dragType == DragType::Length ) {
			sMacro.append(
				QString( " %1" ).arg( pCommonStrings->getNotePropertyLength() ) );
		}
		else if ( m_dragType == DragType::Property ) {
			switch ( m_property ) {
			case Property::Velocity:
				sMacro.append( QString( " %1" ).arg(
								   pCommonStrings->getNotePropertyVelocity() ) );
				break;
			case Property::Pan:
				sMacro.append( QString( " %1" ).arg(
								   pCommonStrings->getNotePropertyPan() ) );
				break;
			case Property::LeadLag:
				sMacro.append( QString( " %1" ).arg(
								   pCommonStrings->getNotePropertyLeadLag() ) );
				break;
			case Property::Probability:
				sMacro.append( QString( " %1" ).arg(
								   pCommonStrings->getNotePropertyProbability() ) );
				break;
			default:
				ERRORLOG( "property not supported" );
			}
		}

		pHydrogenApp->beginUndoMacro( sMacro );
		bMacroStarted = true;
	}

	auto editNoteProperty = [=]( PatternEditor::Property property,
								 std::shared_ptr<Note> pNewNote,
								 std::shared_ptr<Note> pOldNote ) {
		if ( m_dragType == DragType::Length ) {
			pHydrogenApp->pushUndoCommand(
				new SE_editNotePropertiesAction(
					property,
					m_pPatternEditorPanel->getPatternNumber(),
					pOldNote->getPosition(),
					pOldNote->getInstrumentId(),
					pOldNote->getInstrumentId(),
					pOldNote->getType(),
					pOldNote->getType(),
					pOldNote->getVelocity(),
					pOldNote->getVelocity(),
					pOldNote->getPan(),
					pOldNote->getPan(),
					pOldNote->getLeadLag(),
					pOldNote->getLeadLag(),
					pOldNote->getProbability(),
					pOldNote->getProbability(),
					pNewNote->getLength(),
					pOldNote->getLength(),
					pOldNote->getKey(),
					pOldNote->getKey(),
					pOldNote->getOctave(),
					pOldNote->getOctave() ) );
		}
		else if ( m_dragType == DragType::Property ) {
			pHydrogenApp->pushUndoCommand(
				new SE_editNotePropertiesAction(
					property,
					m_pPatternEditorPanel->getPatternNumber(),
					pOldNote->getPosition(),
					pOldNote->getInstrumentId(),
					pOldNote->getInstrumentId(),
					pOldNote->getType(),
					pOldNote->getType(),
					pNewNote->getVelocity(),
					pOldNote->getVelocity(),
					pNewNote->getPan(),
					pOldNote->getPan(),
					pNewNote->getLeadLag(),
					pOldNote->getLeadLag(),
					pNewNote->getProbability(),
					pOldNote->getProbability(),
					pOldNote->getLength(),
					pOldNote->getLength(),
					pOldNote->getKey(),
					pOldNote->getKey(),
					pOldNote->getOctave(),
					pOldNote->getOctave() ) );
		}
	};

	std::vector< std::shared_ptr<Note> > notesStatus;

	for ( const auto& [ ppUpdatedNote, ppOriginalNote ] : m_draggedNotes ) {
		if ( ppUpdatedNote == nullptr || ppOriginalNote == nullptr ) {
			continue;
		}

		if ( m_dragType == DragType::Length &&
			 ppUpdatedNote->getLength() != ppOriginalNote->getLength() ) {
			editNoteProperty( Property::Length, ppUpdatedNote, ppOriginalNote );

			// We only trigger status messages for notes hovered by the user.
			for ( const auto ppNote : m_elementsHoveredOnDragStart ) {
				if ( ppNote == ppOriginalNote ) {
					notesStatus.push_back( ppUpdatedNote );
				}
			}
		}
		else if ( m_dragType == DragType::Property &&
				  ( ppUpdatedNote->getVelocity() !=
					ppOriginalNote->getVelocity() ||
					ppUpdatedNote->getPan() != ppOriginalNote->getPan() ||
					ppUpdatedNote->getLeadLag() != ppOriginalNote->getLeadLag() ||
					ppUpdatedNote->getProbability() !=
					ppOriginalNote->getProbability() ) ) {
			editNoteProperty( m_property, ppUpdatedNote, ppOriginalNote );

			// We only trigger status messages for notes hovered by the user.
			for ( const auto ppNote : m_elementsHoveredOnDragStart ) {
				if ( ppNote == ppOriginalNote ) {
					notesStatus.push_back( ppUpdatedNote );
				}
			}
		}
	}

	if ( m_draggedNotes.size() > 0 ) {
		if ( m_dragType == DragType::Length ) {
			triggerStatusMessage( notesStatus, Property::Length );
		}
		else if ( m_dragType == DragType::Property ) {
			triggerStatusMessage( notesStatus, m_property );
		}
		else {
			ERRORLOG( "Invalid drag type" );
		}
	}

	if ( bMacroStarted ) {
		pHydrogenApp->endUndoMacro();
	}

	m_draggedNotes.clear();
	m_dragType = DragType::None;
}

void PatternEditor::updateAllComponents( Editor::Update update ) {
	m_pPatternEditorPanel->getSidebar()->updateEditor();
	m_pPatternEditorPanel->getPatternEditorRuler()->update();
	updateVisibleComponents( update );
}

void PatternEditor::updateVisibleComponents( Editor::Update update ) {
	m_pPatternEditorPanel->getVisibleEditor()->updateEditor( update );
	m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor( update );
}

void PatternEditor::updateModifiers( QInputEvent *ev ) {
	m_pPatternEditorPanel->updateQuantization( ev );

	Editor::Base<Elem>::updateModifiers( ev );
}

bool PatternEditor::updateWidth() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();

	int nEditorWidth, nActiveWidth;
	if ( pPattern != nullptr ) {
		nActiveWidth = PatternEditor::nMargin + m_fGridWidth *
			pPattern->getLength();

		// In case there are other patterns playing which are longer
		// than the selected one, their notes will be placed using a
		// different color set between m_nActiveWidth and
		// m_nEditorWidth.
		if ( pHydrogen->getMode() == Song::Mode::Song &&
			 pPattern != nullptr && pPattern->isVirtual() &&
			 ! pHydrogen->isPatternEditorLocked() ) {
			nEditorWidth =
				std::max( PatternEditor::nMargin + m_fGridWidth *
						  pPattern->longestVirtualPatternLength() + 1,
						  static_cast<float>(nActiveWidth) );
		}
		else if ( PatternEditorPanel::isUsingAdditionalPatterns( pPattern ) ) {
			nEditorWidth =
				std::max( PatternEditor::nMargin + m_fGridWidth *
						  pHydrogen->getAudioEngine()->getPlayingPatterns()->longestPatternLength( false ) + 1,
						  static_cast<float>(nActiveWidth) );
		}
		else {
			nEditorWidth = nActiveWidth;
		}
	}
	else {
		nEditorWidth = PatternEditor::nMargin + 4 * H2Core::nTicksPerQuarter *
			m_fGridWidth;
		nActiveWidth = nEditorWidth;
	}

	if ( m_nEditorWidth != nEditorWidth || m_nActiveWidth != nActiveWidth ) {
		m_nEditorWidth = nEditorWidth;
		m_nActiveWidth = nActiveWidth;

		resize( m_nEditorWidth, m_nEditorHeight );

		updatePixmapSize();

		return true;
	}

	return false;
}

///
/// Align selected (or all) notes to the current grid
///
void PatternEditor::alignToGrid() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	popupSetup();

	validateSelection();
	if ( m_selection.isEmpty() ) {
		return;
	}

	// Every deleted note will be removed from the selection. Therefore, we can
	// not iterate the selection directly.
	std::vector< std::shared_ptr<Note> > notes;
	for ( const auto& ppNote : m_selection ) {
		notes.push_back( ppNote );
	}

	auto pHydrogenApp = HydrogenApp::get_instance();

	std::vector<QUndoCommand*> deleteCommands, addCommands;
	// When aligning notes to a more coarse-grained grid, it is likely to have
	// identical notes coinciding. In order to not show false positive error
	// messages, we ensure there will be no duplicates.
	std::vector< std::pair< std::shared_ptr<Note>, int > > alignedNotes;

	for ( const auto& ppNote : notes ) {
		if ( ppNote == nullptr ) {
			continue;
		}

		const int nRow = m_pPatternEditorPanel->findRowDB( ppNote );
		const auto row = m_pPatternEditorPanel->getRowDB( nRow );
		const int nPosition = ppNote->getPosition();
		const int nNewInstrument = nRow;
		const int nGranularity = granularity();

		// Round to the nearest position in the current grid. We add 1 to round
		// up when the note is precisely in the middle. This allows us to change
		// a 4/4 pattern to a 6/8 swing feel by changing the grid to 1/8th
		// triplest, and hitting 'align'.
		const int nNewPosition = nGranularity *
			( (nPosition+(nGranularity/2)+1) / nGranularity );

		// Cache note properties since a potential first note deletion will also
		// call the note's destructor.
		const auto id = ppNote->getInstrumentId();
		const auto sType = ppNote->getType();
		const int nLength = ppNote->getLength();
		const float fVelocity = ppNote->getVelocity();
		const float fPan = ppNote->getPan();
		const float fLeadLag = ppNote->getLeadLag();
		const auto key = ppNote->getKey();
		const auto octave = ppNote->getOctave();
		const float fProbability = ppNote->getProbability();
		const bool bNoteOff = ppNote->getNoteOff();
		const bool bIsMappedToDrumkit = ppNote->getInstrument() != nullptr;

		// Move note -> delete at source position
		deleteCommands.push_back( new SE_addOrRemoveNoteAction(
									  nPosition,
									  id,
									  sType,
									  m_pPatternEditorPanel->getPatternNumber(),
									  nLength,
									  fVelocity,
									  fPan,
									  fLeadLag,
									  key,
									  octave,
									  fProbability,
									  Editor::Action::Delete,
									  bNoteOff,
									  bIsMappedToDrumkit,
									  Editor::ActionModifier::None ) );

		bool bGridPointAlreadyAdded = false;
		for ( const auto& [ ppAlignedNote, nnNewPosition ] : alignedNotes ) {
			if ( ppNote->getInstrumentId() == ppAlignedNote->getInstrumentId() &&
				 ppNote->getType() == ppAlignedNote->getType() &&
				 ppNote->getKey() == ppAlignedNote->getKey() &&
				 ppNote->getOctave() == ppAlignedNote->getOctave() &&
				 nNewPosition == nnNewPosition ) {
				bGridPointAlreadyAdded = true;
				break;
			}
		}
		if ( bGridPointAlreadyAdded ) {
			continue;
		}
		alignedNotes.push_back( std::make_pair( ppNote, nNewPosition ) );

		auto modifier = Editor::ActionModifier::None;
		if ( m_elementsHoveredForPopup.size() > 0 ) {
			for ( const auto& ppHoveredNote : m_elementsHoveredForPopup ) {
				if ( ppNote == ppHoveredNote ) {
					modifier = Editor::ActionModifier::MoveCursorTo;
					break;
				}
			}
		}

		// Add at target position
		addCommands.push_back( new SE_addOrRemoveNoteAction(
								   nNewPosition,
								   id,
								   sType,
								   m_pPatternEditorPanel->getPatternNumber(),
								   nLength,
								   fVelocity,
								   fPan,
								   fLeadLag,
								   key,
								   octave,
								   fProbability,
								   Editor::Action::Add,
								   bNoteOff,
								   bIsMappedToDrumkit,
								   modifier ) );
	}

	// Move the notes
	pHydrogenApp->beginUndoMacro( tr( "Align notes to grid" ) );
	for ( const auto ppCommand : deleteCommands ) {
		pHydrogenApp->pushUndoCommand( ppCommand );
	}
	for ( const auto ppCommand : addCommands ) {
		pHydrogenApp->pushUndoCommand( ppCommand );
	}
	pHydrogenApp->endUndoMacro();

	popupTeardown();
}

void PatternEditor::randomizeVelocity() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected. Nothing to be randomized.
		return;
	}

	popupSetup();

	validateSelection();
	if ( m_selection.isEmpty() ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();

	pHydrogenApp->beginUndoMacro( tr( "Random velocity" ) );

	for ( const auto& ppNote : m_selection ) {
		if ( ppNote == nullptr ) {
			continue;
		}

		float fVal = ( rand() % 100 ) / 100.0;
		fVal = std::clamp( ppNote->getVelocity() + ( ( fVal - 0.50 ) / 2 ),
						   0.0, 1.0 );
		pHydrogenApp->pushUndoCommand(
			new SE_editNotePropertiesAction(
				PatternEditor::Property::Velocity,
				m_pPatternEditorPanel->getPatternNumber(),
				ppNote->getPosition(),
				ppNote->getInstrumentId(),
				ppNote->getInstrumentId(),
				ppNote->getType(),
				ppNote->getType(),
				fVal,
				ppNote->getVelocity(),
				ppNote->getPan(),
				ppNote->getPan(),
				ppNote->getLeadLag(),
				ppNote->getLeadLag(),
				ppNote->getProbability(),
				ppNote->getProbability(),
				ppNote->getLength(),
				ppNote->getLength(),
				ppNote->getKey(),
				ppNote->getKey(),
				ppNote->getOctave(),
				ppNote->getOctave() ) );
	}

	pHydrogenApp->endUndoMacro();

	std::vector< std::shared_ptr<Note> > notes;
	for ( const auto& ppNote : m_selection ) {
		if ( ppNote != nullptr ) {
			notes.push_back( ppNote );
		}
	}

	triggerStatusMessage( notes, Property::Velocity );

	popupTeardown();
}

void PatternEditor::selectAllNotesInRow( int nRow, int nPitch ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const auto row = m_pPatternEditorPanel->getRowDB( nRow );

	m_selection.clearSelection();

	if ( nPitch != PITCH_INVALID ) {
		const auto key = Note::pitchToKey( nPitch );
		const auto octave = Note::pitchToOctave( nPitch );
		for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
			if ( ppNote != nullptr && row.contains( ppNote ) &&
				 ppNote->getKey() == key && ppNote->getOctave() == octave ) {
				m_selection.addToSelection( ppNote );
			}
		}
	}
	else {
		for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
			if ( ppNote != nullptr && row.contains( ppNote ) ) {
				m_selection.addToSelection( ppNote );
			}
		}
	}
	updateVisibleComponents( Editor::Update::Content );
}

void PatternEditor::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

void PatternEditor::applyColor( std::shared_ptr<H2Core::Note> pNote,
								QPen* pNotePen, QBrush* pNoteBrush,
								QPen* pNoteTailPen, QBrush* pNoteTailBrush,
								QPen* pHighlightPen, QBrush* pHighlightBrush,
								QPen* pMovingPen, QBrush* pMovingBrush,
								NoteStyle noteStyle ) const
{
	const auto pColorTheme =
		H2Core::Preferences::get_instance()->getColorTheme();

	const auto backgroundPenStyle = Qt::DotLine;
	const auto backgroundBrushStyle = Qt::Dense4Pattern;
	const auto foregroundPenStyle = Qt::SolidLine;
	const auto foregroundBrushStyle = Qt::SolidPattern;
	const auto movingPenStyle = Qt::DotLine;
	const auto movingBrushStyle = Qt::NoBrush;

	int nHue, nSaturation, nValue;

	// Note color
	QColor noteFillColor;
	if ( ! pNote->getNoteOff() ) {
		noteFillColor = PatternEditor::computeNoteColor( pNote->getVelocity() );
	} else {
		noteFillColor = pColorTheme->m_patternEditor_noteOffColor;
	}

	// color base note will be filled with
	pNoteBrush->setColor( noteFillColor );

	if ( noteStyle & NoteStyle::Background ) {
		pNoteBrush->setStyle( backgroundBrushStyle );
	}
	else {
		pNoteBrush->setStyle( foregroundBrushStyle );
	}

	// outline color
	pNotePen->setColor( Qt::black );

	if ( noteStyle & NoteStyle::Background ) {
		pNotePen->setStyle( backgroundPenStyle );
	}
	else {
		pNotePen->setStyle( foregroundPenStyle );
	}

	// Tail color
	pNoteTailPen->setColor( pNotePen->color() );
	pNoteTailPen->setStyle( pNotePen->style() );

	if ( noteStyle & NoteStyle::EffectiveLength ) {
		// Use a more subtle version of the note off color. As this color is
		// surrounded by the note outline - which is always black - we do not
		// have to check the value but can always go for a more lighter color.
		QColor effectiveLengthColor( pColorTheme->m_patternEditor_noteOffColor );
		effectiveLengthColor = effectiveLengthColor.lighter( 125 );
		pNoteTailBrush->setColor( effectiveLengthColor );
	}
	else {
		pNoteTailBrush->setColor( pNoteBrush->color() );
	}
	pNoteTailBrush->setStyle( pNoteBrush->style() );

	// Highlight color
	QColor selectionColor;
	if ( m_pPatternEditorPanel->hasPatternEditorFocus() ) {
		selectionColor = pColorTheme->m_selectionHighlightColor;
	}
	else {
		selectionColor = pColorTheme->m_selectionInactiveColor;
	}

	QColor highlightColor;
	if ( noteStyle & NoteStyle::Selected ) {
		// Selected notes have the highest priority
		highlightColor = selectionColor;
	}
	else if ( noteStyle & NoteStyle::NoPlayback ) {
		// Notes that won't be played back maintain their special color.
		highlightColor = pColorTheme->m_muteColor;

		// The color of the mute button itself would be too flash and draw too
		// much attention to the note which are probably the ones the user does
		// not care about. We make the color more subtil.
		highlightColor.getHsv( &nHue, &nSaturation, &nValue );

		const int nSubtleValueFactor = 112;
		const int nSubtleSaturation = std::max(
			static_cast<int>(std::round( nSaturation * 0.85 )), 0 );
		highlightColor.setHsv( nHue, nSubtleSaturation, nValue );

		if ( Skin::moreBlackThanWhite( highlightColor ) ) {
			highlightColor = highlightColor.darker( nSubtleValueFactor );
		} else {
			highlightColor = highlightColor.lighter( nSubtleValueFactor );
		}
}
	else {
		highlightColor = selectionColor;
	}

	int nFactor = 100;
	if ( noteStyle & NoteStyle::Selected && noteStyle & NoteStyle::Hovered ) {
		nFactor = 107;
	}
	else if ( noteStyle & NoteStyle::Hovered ) {
		nFactor = 125;
	}

	if ( noteStyle & NoteStyle::Hovered ) {
		// Depending on the highlight color, we make it either darker or
		// lighter.
		if ( Skin::moreBlackThanWhite( highlightColor ) ) {
			highlightColor = highlightColor.lighter( nFactor );
		} else {
			highlightColor = highlightColor.darker( nFactor );
		}
	}

	pHighlightBrush->setColor( highlightColor );

	if ( noteStyle & NoteStyle::Background ) {
		pHighlightBrush->setStyle( backgroundBrushStyle );
	}
	else {
		pHighlightBrush->setStyle( foregroundBrushStyle );
	}

	if ( Skin::moreBlackThanWhite( highlightColor ) ) {
		pHighlightPen->setColor( Qt::white );
	} else {
		pHighlightPen->setColor( Qt::black );
	}

	if ( noteStyle & NoteStyle::Background ) {
		pHighlightPen->setStyle( backgroundPenStyle );
	}
	else {
		pHighlightPen->setStyle( foregroundPenStyle );
	}

	// Moving note color
	pMovingBrush->setStyle( movingBrushStyle );

	pMovingPen->setColor( Qt::black );
	pMovingPen->setStyle( movingPenStyle );
	pMovingPen->setWidth( 2 );
}

QColor PatternEditor::computeNoteColor( float fVelocity ) {
	float fRed, fGreen, fBlue;

	const auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	QColor fullColor = pColorTheme->m_patternEditor_noteVelocityFullColor;
	QColor defaultColor = pColorTheme->m_patternEditor_noteVelocityDefaultColor;
	QColor halfColor = pColorTheme->m_patternEditor_noteVelocityHalfColor;
	QColor zeroColor = pColorTheme->m_patternEditor_noteVelocityZeroColor;

	// The colors defined in the Preferences correspond to fixed
	// velocity values. In case the velocity lies between two of those
	// the corresponding colors will be interpolated.
	float fWeightFull = 0;
	float fWeightDefault = 0;
	float fWeightHalf = 0;
	float fWeightZero = 0;

	if ( fVelocity >= VELOCITY_MAX ) {
		fWeightFull = 1.0;
	} else if ( fVelocity >= VELOCITY_DEFAULT ) {
		fWeightDefault = ( 1.0 - fVelocity )/ ( 1.0 - 0.8 );
		fWeightFull = 1.0 - fWeightDefault;
	} else if ( fVelocity >= 0.5 ) {
		fWeightHalf = ( 0.8 - fVelocity )/ ( 0.8 - 0.5 );
		fWeightDefault = 1.0 - fWeightHalf;
	} else {
		fWeightZero = ( 0.5 - fVelocity )/ ( 0.5 );
		fWeightHalf = 1.0 - fWeightZero;
	}

	fRed = fWeightFull * fullColor.redF() +
		fWeightDefault * defaultColor.redF() +
		fWeightHalf * halfColor.redF() + fWeightZero * zeroColor.redF();
	fGreen = fWeightFull * fullColor.greenF() +
		fWeightDefault * defaultColor.greenF() +
		fWeightHalf * halfColor.greenF() + fWeightZero * zeroColor.greenF();
	fBlue = fWeightFull * fullColor.blueF() +
		fWeightDefault * defaultColor.blueF() +
		fWeightHalf * halfColor.blueF() + fWeightZero * zeroColor.blueF();

	QColor color;
	color.setRedF( fRed );
	color.setGreenF( fGreen );
	color.setBlueF( fBlue );
	
	return color;
}

void PatternEditor::drawBorders( QPainter& p ) {
	const auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	const QColor borderColor( pColorTheme->m_patternEditor_lineColor );
	const QColor borderInactiveColor(
		pColorTheme->m_windowTextColor.darker( 170 ) );

	p.setPen( borderColor );
	p.drawLine( 0, 0, m_nActiveWidth, 0 );
	p.drawLine( 0, m_nEditorHeight - 1,
					  m_nActiveWidth, m_nEditorHeight - 1 );

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( QPen( borderInactiveColor, 1, Qt::SolidLine ) );
		p.drawLine( m_nActiveWidth, 0, m_nEditorWidth, 0 );
		p.drawLine( m_nActiveWidth, m_nEditorHeight - 1,
						  m_nEditorWidth, m_nEditorHeight - 1 );
		p.drawLine( m_nEditorWidth - 1, 0,
						  m_nEditorWidth - 1, m_nEditorHeight );
	}
	else {
		p.drawLine( m_nActiveWidth, 0,
						  m_nActiveWidth, m_nEditorHeight );
	}
}

void PatternEditor::drawFocus( QPainter& p ) {

	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}

	const auto pPref = H2Core::Preferences::get_instance();

	QColor color = pPref->getColorTheme()->m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't clicked it yet,
	// the highlight will be done more transparent to indicate that keyboard
	// inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	const QScrollArea* pScrollArea;

	if ( m_instance == Editor::Instance::DrumPattern ) {
		pScrollArea = m_pPatternEditorPanel->getDrumPatternEditorScrollArea();
	}
	else if ( m_instance == Editor::Instance::PianoRoll ) {
		pScrollArea = m_pPatternEditorPanel->getPianoRollEditorScrollArea();
	}
	else if ( m_instance == Editor::Instance::NotePropertiesRuler ) {
		switch ( m_property ) {
		case PatternEditor::Property::Velocity:
			pScrollArea = m_pPatternEditorPanel->getNoteVelocityScrollArea();
			break;
		case PatternEditor::Property::Pan:
			pScrollArea = m_pPatternEditorPanel->getNotePanScrollArea();
			break;
		case PatternEditor::Property::LeadLag:
			pScrollArea = m_pPatternEditorPanel->getNoteLeadLagScrollArea();
			break;
		case PatternEditor::Property::KeyOctave:
			pScrollArea = m_pPatternEditorPanel->getNoteKeyOctaveScrollArea();
			break;
		case PatternEditor::Property::Probability:
			pScrollArea = m_pPatternEditorPanel->getNoteProbabilityScrollArea();
			break;
		case PatternEditor::Property::None:
		default:
			return;
		}
	}

	const int nStartY = pScrollArea->verticalScrollBar()->value();
	int nStartX = pScrollArea->horizontalScrollBar()->value();
	if ( m_instance == Editor::Instance::PianoRoll ) {
        nStartX += PianoRollEditor::nMarginSidebar;
	}
	int nEndY = nStartY + pScrollArea->viewport()->size().height();
	if ( m_instance == Editor::Instance::DrumPattern ) {
		nEndY = std::min( static_cast<int>(m_nGridHeight) *
						  m_pPatternEditorPanel->getRowNumberDB(), nEndY );
	}
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
	p.setPen( pen );
	p.drawLine( QPoint( nStartX, nStartY ), QPoint( nEndX, nStartY ) );
	p.drawLine( QPoint( nStartX, nStartY ), QPoint( nStartX, nEndY ) );
	p.drawLine( QPoint( nEndX, nEndY ), QPoint( nStartX, nEndY ) );

	if ( nMargin != 0 ) {
		// Since for all other lines we are drawing at a border with just half
		// of the line being painted in the visual viewport, there has to be
		// some tweaking since the NotePropertiesRuler is paintable to the
		// right.
		pen.setWidth( 2 );
		p.setPen( pen );
	}
	p.drawLine( QPoint( nEndX + nMargin, nStartY ),
					  QPoint( nEndX + nMargin, nEndY ) );
}

//! Draw lines for note grid.
void PatternEditor::drawGridLines( QPainter &p, const Qt::PenStyle& style ) const
{
	const auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();
	const std::vector<QColor> colorsActive = {
		QColor( pColorTheme->m_patternEditor_line1Color ),
		QColor( pColorTheme->m_patternEditor_line2Color ),
		QColor( pColorTheme->m_patternEditor_line3Color ),
		QColor( pColorTheme->m_patternEditor_line4Color ),
		QColor( pColorTheme->m_patternEditor_line5Color ),
	};
	const std::vector<QColor> colorsInactive = {
		QColor( pColorTheme->m_windowTextColor.darker( 170 ) ),
		QColor( pColorTheme->m_windowTextColor.darker( 190 ) ),
		QColor( pColorTheme->m_windowTextColor.darker( 210 ) ),
		QColor( pColorTheme->m_windowTextColor.darker( 230 ) ),
		QColor( pColorTheme->m_windowTextColor.darker( 250 ) ),
	};

	// In case quantization as turned off, notes can be moved at all possible
	// ticks. To indicate this state, we show less pronounced grid lines at all
	// additional positions.
	const auto lineStyleGridOff = Qt::DotLine;

	const bool bTriplets = m_pPatternEditorPanel->isUsingTriplets();

	auto lineStyle = style;

	// The following part is intended for the non-triplet grid lines. But
	// whenever quantization was turned off, we also use it to draw the less
	// pronounced grid lines.
	if ( ! bTriplets || ! m_pPatternEditorPanel->isQuantized() ) {
		// For each successive set of finer-spaced lines, the even
		// lines will have already been drawn at the previous coarser
		// pitch, so only the odd numbered lines need to be drawn.
		int nColour = 0;

		if ( bTriplets ) {
			nColour = colorsActive.size() - 1;
			lineStyle = lineStyleGridOff;
		}

		// Draw vertical lines. To minimise pen colour changes (and
		// avoid unnecessary division operations), we draw them in
		// multiple passes, of successively finer spacing (and
		// advancing the colour selection at each refinement) until
		// we've drawn enough to satisfy the resolution setting.
		//
		// The drawing sequence looks something like:
		// |       |       |       |         - first pass, all 1/4 notes
		// |   :   |   :   |   :   |   :     - second pass, odd 1/8th notes
		// | . : . | . : . | . : . | . : .   - third pass, odd 1/16th notes

		// First, quarter note markers. All the quarter note markers must be
		// drawn. These will be drawn on all resolutions.
		float fStep = H2Core::nTicksPerQuarter * m_fGridWidth;
		float x = PatternEditor::nMargin;
		p.setPen( QPen( colorsActive[ nColour ], 1, lineStyle ) );
		while ( x < m_nActiveWidth ) {
			p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			x += fStep;
		}

		p.setPen( QPen( colorsInactive[ nColour ], 1, lineStyle ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			x += fStep;
		}

		++nColour;

		// Resolution 4 was already taken into account above;
		std::vector<int> availableResolutions = { 8, 16, 32, 64,
		    4 * H2Core::nTicksPerQuarter };
		const int nResolution = m_pPatternEditorPanel->getResolution();

		for ( int nnRes : availableResolutions ) {
			if ( nnRes > nResolution ) {
				if ( m_pPatternEditorPanel->isQuantized() ) {
					break;
				}
				else {
					lineStyle = lineStyleGridOff;
					nColour = colorsActive.size();
				}
			}

			fStep = 4 * H2Core::nTicksPerQuarter / nnRes * m_fGridWidth;
			float x = PatternEditor::nMargin + fStep;
			p.setPen( QPen( colorsActive[ std::min( nColour, static_cast<int>(colorsActive.size()) - 1 ) ],
							1, lineStyle ) );

			if ( nnRes != 4 * H2Core::nTicksPerQuarter ) {
				// With each increase of resolution 1/4 -> 1/8 -> 1/16 -> 1/32
				// -> 1/64 the number of available notes doubles and all we need
				// to do is to draw another grid line right between two existing
				// ones.
				while ( x < m_nActiveWidth + fStep ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}
			else {
				// When turning resolution off, things get a bit more tricky.
				// Between 1/64 -> 1/192 (1/(4 * H2Core::nTicksPerQuarter)) the
				// space between existing grid line will be filled by two
				// instead of one new line.
				while ( x < m_nActiveWidth + fStep ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep;
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}

			p.setPen( QPen( colorsInactive[ std::min( nColour, static_cast<int>(colorsInactive.size()) - 1 ) ],
							1, lineStyle ) );
			if ( nnRes != 4 * H2Core::nTicksPerQuarter ||
				 pPref->getQuantizeEvents() ) {
				while ( x < m_nEditorWidth ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}
			else {
				while ( x < m_nEditorWidth ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep;
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}

			nColour++;
		}

	}

	if ( bTriplets ) {
		lineStyle = style;

		// Triplet line markers, we only differentiate colours on the
		// first of every triplet.
		float fStep = granularity() * m_fGridWidth;
		float x = PatternEditor::nMargin;
		p.setPen(  QPen( colorsActive[ 0 ], 1, lineStyle ) );
		while ( x < m_nActiveWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			x += fStep * 3;
		}

		p.setPen(  QPen( colorsInactive[ 0 ], 1, lineStyle ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			x += fStep * 3;
		}

		// Second and third marks
		x = PatternEditor::nMargin + fStep;
		p.setPen(  QPen( colorsActive[ 2 ], 1, lineStyle ) );
		while ( x < m_nActiveWidth + fStep ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
			x += fStep * 3;
		}

		p.setPen( QPen( colorsInactive[ 2 ], 1, lineStyle ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
			x += fStep * 3;
		}
	}
}

void PatternEditor::drawNote( QPainter &p, std::shared_ptr<H2Core::Note> pNote,
							  NoteStyle noteStyle ) const
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr || pNote == nullptr ) {
		return;
	}

	// Determine the center of the note symbol.
	auto point = elementToPoint( pNote );
	if ( m_instance == Editor::Instance::PianoRoll ) {
		const auto selectedRow = m_pPatternEditorPanel->getRowDB(
			m_pPatternEditorPanel->getSelectedRowDB() );
		if ( ! selectedRow.contains( pNote ) ) {
			ERRORLOG( QString( "Provided note [%1] is not part of selected row [%2]" )
					  .arg( pNote->toQString() ).arg( selectedRow.toQString() ) );
			return;
		}

		point.setY( m_nGridHeight *
				 Note::pitchToLine( pNote->getPitchFromKeyOctave() ) +
				 (m_nGridHeight / 2) );
	}
	point.setY( point.y() - 3 );

	p.setRenderHint( QPainter::Antialiasing );

	uint w = 8, h =  8;

	// NoPlayback is handled in here in order to not bloat calling routines
	// (since it has to be calculated for every note drawn).
	if ( ! checkNotePlayback( pNote ) ) {
		noteStyle =
			static_cast<NoteStyle>(noteStyle | NoteStyle::NoPlayback);
	}

	const int nNoteLength = calculateEffectiveNoteLength( pNote );
	if ( nNoteLength != pNote->getLength() ) {
		noteStyle =
			static_cast<NoteStyle>(noteStyle | NoteStyle::EffectiveLength);
	}

	QPen notePen, noteTailPen, highlightPen, movingPen;
	QBrush noteBrush, noteTailBrush, highlightBrush, movingBrush;
	applyColor( pNote, &notePen, &noteBrush, &noteTailPen, &noteTailBrush,
				&highlightPen, &highlightBrush, &movingPen, &movingBrush,
				noteStyle );

	QPoint movingOffset;
	if ( noteStyle & NoteStyle::Moved ) {
		const auto delta = movingGridOffset();
		movingOffset = QPoint( delta.getColumn() * m_fGridWidth,
							   delta.getRow() * m_nGridHeight );
	}

	if ( pNote->getNoteOff() == false ) {
		int width = w;

		if ( ! ( noteStyle & NoteStyle::Moved) &&
			 noteStyle & ( NoteStyle::Selected |
						   NoteStyle::Hovered |
						   NoteStyle::NoPlayback ) ) {
			p.setPen( highlightPen );
			p.setBrush( highlightBrush );
			p.drawEllipse( point.x() - 4 - 3, point.y() - 3, w + 6, h + 6 );
			p.setBrush( Qt::NoBrush );
		}

		// Draw tail
		if ( nNoteLength != LENGTH_ENTIRE_SAMPLE ) {
			if ( nNoteLength == pNote->getLength() ) {
				// When we deal with a genuine length of a note instead of an
				// indication when playback for this note will be stopped, we
				// have to take its pitch into account.
				float fNotePitch = pNote->getPitchFromKeyOctave();
				float fStep = Note::pitchToFrequency( ( double )fNotePitch );

				width = m_fGridWidth * nNoteLength / fStep;
			}
			else {
				width = m_fGridWidth * nNoteLength;
			}
			width = width - 1;	// lascio un piccolo spazio tra una nota ed un altra

			// Since the note body is transparent for an inactive note, we
			// try to start the tail at its boundary. For regular notes we
			// do not care about an overlap, as it ensures that there are no
			// white artifacts between tail and note body regardless of the
			// scale factor.
			if ( ! ( noteStyle & NoteStyle::Moved ) ) {
				if ( noteStyle & ( NoteStyle::Selected |
								   NoteStyle::Hovered |
								   NoteStyle::NoPlayback ) ) {
					p.setPen( highlightPen );
					p.setBrush( highlightBrush );
					// Tail highlight
					p.drawRect( point.x() - 3, point.y() - 1,
								width + 6, 3 + 6 );
					p.drawEllipse( point.x() - 4 - 3, point.y() - 3,
								   w + 6, h + 6 );
					p.fillRect( point.x() - 4, point.y(),
								width, 3 + 4, highlightBrush );
				}

				p.setPen( noteTailPen );
				p.setBrush( noteTailBrush );

				int nRectOnsetX = point.x();
				int nRectWidth = width;
				if ( noteStyle & NoteStyle::Background ) {
					nRectOnsetX = nRectOnsetX + w / 2;
					nRectWidth = nRectWidth - w / 2;
				}

				p.drawRect( nRectOnsetX, point.y() + 2, nRectWidth, 3 );
				p.drawLine( point.x() + width, point.y(),
							point.x() + width, point.y() + h );
			}
		}

		// Draw note
		if ( ! ( noteStyle & NoteStyle::Moved ) ) {
			p.setPen( notePen );
			p.setBrush( noteBrush );
			p.drawEllipse( point.x() -4 , point.y(), w, h );
		}
		else {
			p.setPen( movingPen );
			p.setBrush( movingBrush );

			if ( nNoteLength == LENGTH_ENTIRE_SAMPLE ) {
				p.drawEllipse( movingOffset.x() + point.x() - 4 - 2,
							   movingOffset.y() + point.y() - 2 , w + 4, h + 4 );
			}
			else {
				// Moving note with tail

				const int nDiameterNote = w + 4;
				const int nHeightTail = 7;
				// Angle of triangle at note center with note radius as hypotenuse
				// and half the tail height as opposite.
				const int nAngleIntersection = static_cast<int>(
					std::round( qRadiansToDegrees(
									qAsin( static_cast<qreal>(nHeightTail) /
										   static_cast<qreal>(nDiameterNote) ) ) ) );

				const int nMoveX = movingOffset.x() + point.x();
				const int nMoveY = movingOffset.y() + point.y();

				p.drawArc( nMoveX - 4 - 2, nMoveY - 2, nDiameterNote, nDiameterNote,
						   nAngleIntersection * 16,
						   ( 360 - 2 * nAngleIntersection ) * 16 );

				p.drawLine( nMoveX + w - 2, nMoveY,
							nMoveX + width + 2, nMoveY );
				p.drawLine( nMoveX + width + 2, nMoveY,
							nMoveX + width + 2, nMoveY + nHeightTail );
				p.drawLine( nMoveX + w - 2, nMoveY + nHeightTail,
							nMoveX + width + 2, nMoveY + nHeightTail );
			}
		}
	}
	else if ( pNote->getNoteOff() ) {

		if ( ! ( noteStyle & NoteStyle::Moved ) ) {

			if ( noteStyle & ( NoteStyle::Selected |
							   NoteStyle::Hovered |
							   NoteStyle::NoPlayback ) ) {
				p.setPen( highlightPen );
				p.setBrush( highlightBrush );
				p.drawEllipse( point.x() - 4 - 3, point.y() - 3, w + 6, h + 6 );
				p.setBrush( Qt::NoBrush );
			}

			p.setPen( notePen );
			p.setBrush( noteBrush );
			p.drawEllipse( point.x() - 4 , point.y(), w, h );
		}
		else {
			p.setPen( movingPen );
			p.setBrush( movingBrush );
			p.drawEllipse( movingOffset.x() + point.x() - 4 - 2,
						   movingOffset.y() + point.y() - 2,
						   w + 4, h + 4 );
		}
	}
}

void PatternEditor::drawPattern() {
	const qreal pixelRatio = devicePixelRatio();

	QPainter p( m_pContentPixmap );
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
	const auto pPref = H2Core::Preferences::get_instance();
	const QFont font( pPref->getFontTheme()->m_sApplicationFontFamily,
					  getPointSize( pPref->getFontTheme()->m_fontSize ) );
	const QColor textColor(
		pPref->getColorTheme()->m_patternEditor_noteVelocityDefaultColor );
	QColor textBackgroundColor( textColor );
	textBackgroundColor.setAlpha( 150 );

	validateSelection();

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// We count notes in each position so we can display markers for rows which
	// have more than one note in the same position (a chord or genuine
	// duplicates).
	int nLastColumn = -1;
	// Aggregates the notes for various rows (key) and one specific column.
	std::map< int, std::vector< std::shared_ptr<Note> > > notesAtRow;
	struct PosCount {
		int nRow;
		int nColumn;
		int nNotes;
	};
	std::vector<PosCount> posCounts;
	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		posCounts.clear();
		const auto baseStyle = ppPattern == pPattern ?
			NoteStyle::Foreground : NoteStyle::Background;

		const auto fontColor = ppPattern == pPattern ?
			textColor : textBackgroundColor;

		for ( const auto& [ nnColumn, ppNote ] : *ppPattern->getNotes() ) {
			if ( nnColumn >= ppPattern->getLength() ) {
				// Notes are located beyond the active length of the BaseEditor::editor and
				// aren't visible even when drawn.
				break;
			}
			if ( ppNote == nullptr ||
				 ( m_instance == Editor::Instance::PianoRoll &&
				   ! selectedRow.contains( ppNote ) ) ) {
				continue;
			}

			int nRow = -1;
			nRow = m_pPatternEditorPanel->findRowDB( ppNote );
			auto row = m_pPatternEditorPanel->getRowDB( nRow );
			if ( nRow == -1 ||
				 ( row.id == Instrument::EmptyId && row.sType.isEmpty() ) ) {
				ERRORLOG( QString( "Note [%1] not associated with DB" )
						  .arg( ppNote->toQString() ) );
				m_pPatternEditorPanel->printDB();
				continue;
			}

			if ( m_instance == Editor::Instance::PianoRoll ) {
				nRow = Note::pitchToLine( ppNote->getPitchFromKeyOctave() );
			}

			// Check for duplicates
			if ( nnColumn != nLastColumn ) {
				// New column

				for ( const auto& [ nnRow, nnotes ] : notesAtRow ) {
					sortAndDrawNotes( p, nnotes, baseStyle );
					if ( nnotes.size() > 1 ) {
						posCounts.push_back(
							{ nnRow, nLastColumn,
							  static_cast<int>(nnotes.size()) } );
					}
				}

				nLastColumn = nnColumn;
				notesAtRow.clear();
			}


			if ( notesAtRow.find( nRow ) == notesAtRow.end() ) {
				notesAtRow[ nRow ] =
					std::vector< std::shared_ptr<Note> >{ ppNote };
			}
			else {
				notesAtRow[ nRow ].push_back( ppNote);
			}
		}

		// Handle last column too
		for ( const auto& [ nnRow, nnotes ] : notesAtRow ) {
			sortAndDrawNotes( p, nnotes, baseStyle );
			if ( nnotes.size() > 1 ) {
				posCounts.push_back( { nnRow, nLastColumn,
						static_cast<int>(nnotes.size()) } );
			}
		}
		notesAtRow.clear();

		// Go through used rows list and draw markers for superimposed notes
		for ( const auto [ nnRow, nnColumn, nnNotes ] : posCounts ) {
			// Draw "2x" text to the left of the note
			const int x = PatternEditor::nMargin + ( nnColumn * m_fGridWidth );
			const int y = nnRow * m_nGridHeight;
			const int boxWidth = 128;

			p.setFont( font );
			p.setPen( fontColor );

			p.drawText(
				QRect( x - boxWidth - 6, y, boxWidth, m_nGridHeight ),
				Qt::AlignRight | Qt::AlignVCenter,
				( QString( "%1" ) + QChar( 0x00d7 )).arg( nnNotes ) );
		}
	}
}

void PatternEditor::sortAndDrawNotes( QPainter& p,
									  std::vector< std::shared_ptr<Note> > notes,
									  NoteStyle baseStyle ) {
	std::sort( notes.begin(), notes.end(), Note::compare );

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
		drawNote( p, ppNote, baseStyle );
	}
	auto selectedStyle =
		static_cast<NoteStyle>(NoteStyle::Selected | baseStyle);
	for ( const auto& ppNote : selectedNotes ) {
		drawNote( p, ppNote, selectedStyle );
	}
}

int PatternEditor::calculateEffectiveNoteLength(
	std::shared_ptr<H2Core::Note> pNote ) const
{
	if ( pNote == nullptr ) {
		return -1;
	}

	// Check for the closest note off or note of the same mute group.
	if ( Preferences::get_instance()->
		 getInterfaceTheme()->m_bIndicateEffectiveNoteLength ) {

		const auto pInstrument = pNote->getInstrument();

		// mute group
		const int nLargeNumber = 100000;
		int nEffectiveLength = nLargeNumber;
		if ( pNote->getInstrument() != nullptr &&
			 pNote->getInstrument()->getMuteGroup() != -1 ) {
			const int nMuteGroup = pNote->getInstrument()->getMuteGroup();
			for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
				for ( const auto& [ nnPosition, ppNote ] : *ppPattern->getNotes() ) {
					if ( ppNote != nullptr && ppNote->getInstrument() != nullptr &&
						 ppNote->getInstrument()->getMuteGroup() == nMuteGroup &&
						 ppNote->getInstrument() != pInstrument &&
						 ppNote->getPosition() > pNote->getPosition() &&
						 ( ppNote->getPosition() - pNote->getPosition() ) <
						 nEffectiveLength ) {
						nEffectiveLength = ppNote->getPosition() -
							pNote->getPosition();
					}
				}
			}
		}

		// Note Off
		if ( ! pNote->getNoteOff() && pNote->getInstrument() != nullptr ) {
			for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
				for ( const auto& [ nnPosition, ppNote ] : *ppPattern->getNotes() ) {
					if ( ppNote != nullptr && ppNote->getNoteOff() &&
						 ppNote->getInstrument() == pInstrument &&
						 ppNote->getPosition() > pNote->getPosition() &&
						 ( ppNote->getPosition() - pNote->getPosition() ) <
						 nEffectiveLength ) {
						nEffectiveLength = ppNote->getPosition() -
							pNote->getPosition();
					}
				}
			}
		}

		if ( nEffectiveLength == nLargeNumber ) {
			return pNote->getLength();
		}

		// We only apply this effective length (in ticks) in case it is indeed
		// smaller than the length (in frames) of the longest sample which can
		// be triggered by the note. We consider the current tempo to be
		// constant over the whole note length. This is done as we do not know
		// at which point of the song - thus using which tempo - the note will
		// be played back.
		const int nMaxFrames = pNote->getInstrument()->getLongestSampleFrames();

		// We also need to take the note's pitch into account as this
		// effectively scales the length of the note too.
		const float fCurrentTickSize = Hydrogen::get_instance()->getAudioEngine()
			->getTransportPosition()->getTickSize();
		const int nEffectiveFrames = static_cast<int>(
			TransportPosition::computeFrame(
				nEffectiveLength * Note::pitchToFrequency(
					static_cast<double>(pNote->getPitchFromKeyOctave() ) ),
				fCurrentTickSize ) );

		if ( nEffectiveFrames < nMaxFrames ) {
			return nEffectiveLength;
		}
	}

	return pNote->getLength();
}

bool PatternEditor::checkNotePlayback( std::shared_ptr<H2Core::Note> pNote ) const {
	if ( ! Preferences::get_instance()->
		 getInterfaceTheme()->m_bIndicateNotePlayback ) {
		return true;
	}

	if ( pNote == nullptr || pNote->getInstrument() == nullptr ) {
		return false;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	// If the note is part of a mute group, only the bottom most note at the
	// same position within the group will be rendered.
	if ( pNote->getInstrument()->getMuteGroup() != -1 &&
		 pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		const auto pInstrumentList = pSong->getDrumkit()->getInstruments();
		const int nMuteGroup = pNote->getInstrument()->getMuteGroup();
		for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
			for ( const auto& [ nnPosition, ppNote ] : *ppPattern->getNotes() ) {
				if ( ppNote != nullptr && ppNote->getInstrument() != nullptr &&
					 ppNote->getInstrument()->getMuteGroup() == nMuteGroup &&
					 ppNote->getPosition() == pNote->getPosition() &&
					 pInstrumentList->index( pNote->getInstrument() ) <
					 pInstrumentList->index( ppNote->getInstrument() ) ) {
					return false;
				}
			}
		}
	}

	// Check for a note off at the same position.
	if ( ! pNote->getNoteOff() ) {
		const auto pInstrument = pNote->getInstrument();

		for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
			for ( const auto& [ nnPosition, ppNote ] : *ppPattern->getNotes() ) {
				if ( ppNote != nullptr && ppNote->getNoteOff() &&
					 ppNote->getPosition() == pNote->getPosition() &&
					 ppNote->getInstrument() == pInstrument ) {
					return false;
				}
			}
		}
	}

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->findRowDB( pNote ) );
	return row.bPlaysBackAudio;
}

int PatternEditor::granularity() const {
	int nBase;
	if ( m_pPatternEditorPanel->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	return 4 * 4 * H2Core::nTicksPerQuarter /
		( nBase * m_pPatternEditorPanel->getResolution() );
}

QString PatternEditor::propertyToQString( const Property& property ) {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	QString s;

	switch ( property ) {
	case PatternEditor::Property::Velocity:
		s = pCommonStrings->getNotePropertyVelocity();
		break;
	case PatternEditor::Property::Pan:
		s = pCommonStrings->getNotePropertyPan();
		break;
	case PatternEditor::Property::LeadLag:
		s = pCommonStrings->getNotePropertyLeadLag();
		break;
	case PatternEditor::Property::KeyOctave:
		s = pCommonStrings->getNotePropertyKeyOctave();
		break;
	case PatternEditor::Property::Probability:
		s = pCommonStrings->getNotePropertyProbability();
		break;
	case PatternEditor::Property::Length:
		s = pCommonStrings->getNotePropertyLength();
		break;
	case PatternEditor::Property::Type:
		s = pCommonStrings->getInstrumentType();
		break;
	case PatternEditor::Property::InstrumentId:
		s = pCommonStrings->getInstrumentId();
		break;
	default:
		s = QString( "Unknown property [%1]" ).arg( static_cast<int>(property) ) ;
		break;
	}

	return s;
}

QString PatternEditor::DragTypeToQString( DragType dragType ) {
	switch( dragType ) {
	case DragType::Length:
		return "Length";
	case DragType::Property:
		return "Property";
	default:
		return QString( "Unknown type [%1]" ).arg( static_cast<int>(dragType) );
	}
}
