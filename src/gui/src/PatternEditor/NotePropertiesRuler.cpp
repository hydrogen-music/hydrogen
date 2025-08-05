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
#include "NotePropertiesRuler.h"

#include "../Compatibility/MouseEvent.h"
#include "../Compatibility/WheelEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include "UndoActions.h"
#include "PatternEditorPanel.h"
#include "PianoRollEditor.h"

#include <cassert>

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Hydrogen.h>

using namespace H2Core;

// +1 to fit all the labels and another +1 to have enough room to show the focus
// at the bottom of the editor.
int NotePropertiesRuler::nKeyOctaveHeight =
	NotePropertiesRuler::nOctaveHeight +
	NotePropertiesRuler::nKeyLineHeight * KEYS_PER_OCTAVE + 2 -
	std::floor( NotePropertiesRuler::nKeyLineHeight / 2 );

KeyOctaveLabel::KeyOctaveLabel( QWidget* pParent, const QString& sText, int nY,
								bool bAlternateBackground, Type type )
	: QLabel( pParent )
	, m_bAlternateBackground( bAlternateBackground )
	, m_type( type )
{
	setText( sText );

	move( 1, nY );

	if ( type == Type::Key ) {
		setAlignment( Qt::AlignLeft );
		setIndent( 4 );
		setFixedSize( PatternEditor::nMarginSidebar,
					  NotePropertiesRuler::nKeyLineHeight );
	}
	else {
		setAlignment( Qt::AlignRight );
		setIndent( 3 );
		setFixedSize( PatternEditor::nMarginSidebar / 2,
					  NotePropertiesRuler::nKeyLineHeight );
	}

	updateColors();
	updateFont();
}

KeyOctaveLabel::~KeyOctaveLabel() {}

void KeyOctaveLabel::updateColors() {
	auto pPref = Preferences::get_instance();

	if ( m_type == Type::Key ) {
		QColor backgroundColor;
		if ( m_bAlternateBackground ) {
			backgroundColor =
				pPref->getTheme().m_color.m_patternEditor_alternateRowColor;
		}
		else {
			backgroundColor =
				pPref->getTheme().m_color.m_patternEditor_octaveRowColor;
		}
		m_backgroundColor =
			backgroundColor.darker( Skin::nListBackgroundColorScaling );
	}

	setStyleSheet( QString( "QLabel{ color: %1; }" )
				   .arg( pPref->getTheme().m_color.m_patternEditor_textColor.name() ) );
}

void KeyOctaveLabel::updateFont() {
	auto pPref = Preferences::get_instance();

	int nMargin;
    switch( pPref->getTheme().m_font.m_fontSize ) {
    case H2Core::FontTheme::FontSize::Small:
		nMargin = 2;
		break;
    case H2Core::FontTheme::FontSize::Medium:
		nMargin = 2;
		break;
    case H2Core::FontTheme::FontSize::Large:
		nMargin = 0;
		break;
	}

	QFont font( pPref->getTheme().m_font.m_sLevel2FontFamily );
	font.setPixelSize( NotePropertiesRuler::nKeyLineHeight - nMargin );
	font.setBold( true );

	setFont( font );
}

void KeyOctaveLabel::paintEvent( QPaintEvent* pEvent ) {
	auto p = QPainter( this );

	if ( m_type == Type::Key ) {
		Skin::drawListBackground( &p, QRect( 0, 0, width(), height() ),
								  m_backgroundColor, false );
	}

	QLabel::paintEvent( pEvent );
}


NotePropertiesRuler::NotePropertiesRuler( QWidget *parent,
										  PatternEditor::Property property,
										  Layout layout )
	: PatternEditor( parent )
	, m_nDrawPreviousColumn( -1 )
	, m_layout( layout )
{
	m_type = Editor::Type::Horizontal;
	m_instance = Editor::Instance::NotePropertiesRuler;

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	m_property = property;

	m_fGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * 4 * 4 *
		H2Core::nTicksPerQuarter;

	if ( m_property == PatternEditor::Property::KeyOctave ) {
		m_nEditorHeight = NotePropertiesRuler::nKeyOctaveHeight;
	}
	else {
		m_nEditorHeight = NotePropertiesRuler::nDefaultHeight;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumHeight( m_nEditorHeight );

	updatePixmapSize();

	setFocusPolicy( Qt::StrongFocus );

	// Generic pattern editor menu contains some operations that don't apply
	// here, and we will want to add menu options specific to this later.
	delete m_pPopupMenu;
	m_selectionActions.clear();
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, [&]() {
		selectAll();
		updateVisibleComponents( Editor::Update::Content );
	} );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, [&]() {
		m_selection.clearSelection();
		updateVisibleComponents( Editor::Update::Content );
	} );

	// Create a small sidebar containing labels
	if ( layout == Layout::KeyOctave ) {

		for ( int nnOctave = 0; nnOctave < OCTAVE_NUMBER; ++nnOctave ) {
			m_labels.push_back(
				new KeyOctaveLabel(
					this, QString::number( 5 - nnOctave ),
					nnOctave * NotePropertiesRuler::nKeyLineHeight + 1 +
					std::floor( NotePropertiesRuler::nKeyLineHeight / 2 ),
					false, KeyOctaveLabel::Type::Octave ) );
		}

		// Annotate with note class names
		static QStringList noteNames = QStringList()
			<< pCommonStrings->getNotePitchB()
			<< pCommonStrings->getNotePitchASharp()
			<< pCommonStrings->getNotePitchA()
			<< pCommonStrings->getNotePitchGSharp()
			<< pCommonStrings->getNotePitchG()
			<< pCommonStrings->getNotePitchFSharp()
			<< pCommonStrings->getNotePitchF()
			<< pCommonStrings->getNotePitchE()
			<< pCommonStrings->getNotePitchDSharp()
			<< pCommonStrings->getNotePitchD()
			<< pCommonStrings->getNotePitchCSharp()
			<< pCommonStrings->getNotePitchC();

		for ( int nnKey = 0; nnKey < KEYS_PER_OCTAVE; ++nnKey ) {

			const int nY = NotePropertiesRuler::nOctaveHeight -
				NotePropertiesRuler::nKeyLineHeight / 2 + 1 +
				nnKey * NotePropertiesRuler::nKeyLineHeight;

			bool bAlternate = false;
			if ( nnKey == 1 ||  nnKey == 3 || nnKey == 5 || nnKey == 8 ||
				 nnKey == 10 ) {
				bAlternate = true;
			}

			m_labels.push_back(
				new KeyOctaveLabel( this, noteNames.at( nnKey ), nY, bAlternate,
									KeyOctaveLabel::Type::Key ) );
		}
	}
}

NotePropertiesRuler::~NotePropertiesRuler() {
}

bool NotePropertiesRuler::applyProperty( std::shared_ptr<Note> pNote,
										 PatternEditor::Property property,
										 float fYValue ) {
	if ( pNote == nullptr ) {
		return false;
	}

	switch( property ) {
		case PatternEditor::Property::Velocity:
			if ( ! pNote->getNoteOff() && pNote->getVelocity() != fYValue ) {
				pNote->setVelocity( fYValue );
				return true;
			}
			break;

		case PatternEditor::Property::Pan:
			if ( ! pNote->getNoteOff() &&
				 pNote->getPanWithRangeFrom0To1() != fYValue ) {
				pNote->setPanWithRangeFrom0To1( fYValue );
				return true;
			}
			break;

		case PatternEditor::Property::LeadLag:
			if ( pNote->getLeadLag() != ( fYValue * -2.0 + 1.0 ) ) {
				pNote->setLeadLag( fYValue * -2.0 + 1.0 );
				return true;
			}
			break;

		case PatternEditor::Property::KeyOctave:
			if ( ! pNote->getNoteOff() ) {
				const int nY = static_cast<int>(fYValue);
				int nKey = KEY_INVALID;
				int nOctave = OCTAVE_INVALID;
				if ( nY > 0 &&
					 nY <= NotePropertiesRuler::nOctaveHeight ) {
					nOctave = std::round(
						( NotePropertiesRuler::nOctaveHeight / 2 +
						  NotePropertiesRuler::nKeyLineHeight / 2 - nY -
						  NotePropertiesRuler::nKeyLineHeight / 2 ) /
						NotePropertiesRuler::nKeyLineHeight );
					nOctave = std::clamp( nOctave, OCTAVE_MIN, OCTAVE_MAX );
				}
				else if ( nY >= NotePropertiesRuler::nOctaveHeight &&
						  nY < NotePropertiesRuler::nKeyOctaveHeight ) {
					nKey = ( NotePropertiesRuler::nKeyOctaveHeight - nY -
							 NotePropertiesRuler::nKeyLineHeight / 2 ) /
						NotePropertiesRuler::nKeyLineHeight;
					nKey = std::clamp( nKey, KEY_MIN, KEY_MAX );
				}

				if ( ( nKey != KEY_INVALID &&
					   nKey != static_cast<int>(pNote->getKey()) ) ||
					 ( nOctave != KEY_INVALID &&
					   nOctave != static_cast<int>(pNote->getOctave()) ) ) {
					pNote->setKeyOctave(
						static_cast<Note::Key>(nKey),
						static_cast<Note::Octave>(nOctave));
					return true;
				}
			}
			break;

		case PatternEditor::Property::Probability:
			if ( pNote->getProbability() != fYValue ) {
				pNote->setProbability( fYValue );
				return true;
			}
			break;
	}

	return false;
}

float NotePropertiesRuler::eventToYValue( QMouseEvent* pEvent ) const {
	auto pEv = static_cast<MouseEvent*>( pEvent );

	if ( m_property == PatternEditor::Property::KeyOctave ) {
		return pEv->position().y();
	}

	// normalized
	const double fHeight = static_cast<double>(height());
	float fValue = static_cast<float>(
		std::clamp( ( fHeight - static_cast<double>(pEv->position().y()) )/ fHeight,
					0.0, 1.1 ));

	// centered layouts support resetting the value to the baseline.
	if ( m_layout == Layout::Centered &&
		 ( pEvent->button() == Qt::MiddleButton ||
		   ( pEvent->modifiers() == Qt::ControlModifier &&
			 pEvent->button() == Qt::LeftButton ) )  ) {
		fValue = 0.5;
	}

	return fValue;
}

void NotePropertiesRuler::moveCursorDown( QKeyEvent* ev, Editor::Step step ) {
	float fStep;
	switch( step ) {
	case Editor::Step::None:
		fStep = 0;
		break;
	case Editor::Step::Character:
		fStep = m_property == PatternEditor::Property::KeyOctave ? 1 : 0.1;
		break;
	case Editor::Step::Tiny:
		fStep = m_property == PatternEditor::Property::KeyOctave ? 1 : 0.01;
		break;
	case Editor::Step::Word:
		fStep = m_property == PatternEditor::Property::KeyOctave ?
			Editor::nWordSize : 0.25;
		break;
	case Editor::Step::Page:
		fStep = m_property == PatternEditor::Property::KeyOctave ?
			Editor::nPageSize : 0.5;
		break;
	case Editor::Step::Document:
		fStep = m_property == PatternEditor::Property::KeyOctave ?
			( PITCH_MAX - PITCH_MIN ) : 1;
		break;
	}

	applyCursorDelta( -1 * fStep );
}

void NotePropertiesRuler::moveCursorUp( QKeyEvent* ev, Editor::Step step ) {
	float fStep;
	switch( step ) {
	case Editor::Step::None:
		fStep = 0;
		break;
	case Editor::Step::Character:
		fStep = m_property == PatternEditor::Property::KeyOctave ? 1 : 0.1;
		break;
	case Editor::Step::Tiny:
		fStep = m_property == PatternEditor::Property::KeyOctave ? 1 : 0.01;
		break;
	case Editor::Step::Word:
		fStep = m_property == PatternEditor::Property::KeyOctave ?
			Editor::nWordSize : 0.25;
		break;
	case Editor::Step::Page:
		fStep = m_property == PatternEditor::Property::KeyOctave ?
			Editor::nPageSize : 0.5;
		break;
	case Editor::Step::Document:
		fStep = m_property == PatternEditor::Property::KeyOctave ?
			( PITCH_MAX - PITCH_MIN ) : 1;
		break;
	}

	applyCursorDelta( fStep );
}

void NotePropertiesRuler::updateColors() {
	for ( auto& ppLabel : m_labels ) {
		ppLabel->updateColors();
	}
}

void NotePropertiesRuler::updateFont() {
	for ( auto& ppLabel : m_labels ) {
		ppLabel->updateFont();
	}
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

	auto pEv = static_cast<WheelEvent*>( ev );

	QString sUndoContext = "NotePropertiesRuler::wheelEvent";
	bool bUpdate = false;

	// We interact only with hovered notes. If any of them are part of the
	// current selection, we alter the values of all selected notes. It not, we
	// discard the selection.
	const auto notesUnderPoint = getElementsAtPoint(
		pEv->position().toPoint(), getCursorMargin( nullptr ), true );
	if ( notesUnderPoint.size() == 0 ) {
		return;
	}

	// Focus cursor on hovered note(s).
	m_pPatternEditorPanel->setCursorColumn( notesUnderPoint[ 0 ]->getPosition() );

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
	if ( m_property == Property::KeyOctave ) {
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
	const bool bKey = pEv->position().y() >= NotePropertiesRuler::nOctaveHeight;

	// Apply delta to the property
	const bool bValueChanged = adjustNotePropertyDelta( notes, fDelta, bKey );

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	handleKeyboardCursor( false );

	if ( bUpdate || bValueChanged ) {

		if ( bValueChanged ) {
			triggerStatusMessage( notesStatusMessage, m_property );
			addUndoAction( sUndoContext );
		}

		if ( m_property == Property::Velocity ) {
			m_pPatternEditorPanel->getVisibleEditor()
				->updateEditor( Editor::Update::Content );
		}

		updateEditor( Editor::Update::Content );
	}
}

void NotePropertiesRuler::selectionMoveUpdateEvent( QMouseEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

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
	if ( m_property == PatternEditor::Property::KeyOctave ) {
		// Check whether the drag started within the key or octave section.
		bKey = ( pEv->position().y() - movingOffset.y() ) >=
			NotePropertiesRuler::nOctaveHeight;

		fDelta = static_cast<float>(-movingOffset.y()) /
			static_cast<float>(NotePropertiesRuler::nKeyLineHeight);
	}
	else {
		fDelta = (float)-movingOffset.y() / height();
	}

	std::vector< std::shared_ptr<Note> > notes;
	for ( const auto& ppNote : m_selection ) {
		if ( ppNote != nullptr && ( selectedRow.contains( ppNote ) ||
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
	for ( const auto& ppNote : m_elementsHoveredOnDragStart ) {
		if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
			notesStatusMessage.push_back( ppNote );
		}
	}

	// Move cursor to dragged note(s).
	if ( m_elementsHoveredOnDragStart.size() > 0 ) {
		m_pPatternEditorPanel->setCursorColumn(
			m_elementsHoveredOnDragStart[ 0 ]->getPosition() );
	}

	if ( bValueChanged ) {
		triggerStatusMessage( notesStatusMessage, m_property );
		updateEditor( Editor::Update::Content );

		if ( m_property == Property::Velocity ) {
			// Update note color.
			m_pPatternEditorPanel->getVisibleEditor()
				->updateEditor( Editor::Update::Content );
		}
	}
}

void NotePropertiesRuler::selectionMoveEndEvent( QInputEvent *ev ) {
	//! The "move" has already been reflected in the notes. Now just complete Undo event.
	addUndoAction( "" );

	updateEditor( Editor::Update::Content );
}

//! Move of selection is cancelled. Revert notes to preserved state.
void NotePropertiesRuler::selectionMoveCancelEvent() {
	for ( auto it : m_oldNotes ) {
		std::shared_ptr<Note> pNote = it.first, pOldNote = it.second;
		switch ( m_property ) {
		case PatternEditor::Property::Velocity:
			pNote->setVelocity( pOldNote->getVelocity() );
			break;
		case PatternEditor::Property::Pan:
			pNote->setPan( pOldNote->getPan() );
			break;
		case PatternEditor::Property::LeadLag:
			pNote->setLeadLag( pOldNote->getLeadLag() );
			break;
		case PatternEditor::Property::KeyOctave:
			pNote->setKeyOctave( pOldNote->getKey(), pOldNote->getOctave() );
			break;
		case PatternEditor::Property::Probability:
			pNote->setProbability( pOldNote->getProbability() );
			break;
		case PatternEditor::Property::None:
		default:
			break;
		}
	}

	if ( m_elementsHoveredOnDragStart.size() != 0 ) {
		triggerStatusMessage( m_elementsHoveredOnDragStart, m_property );
	}

	m_oldNotes.clear();
}

QPoint NotePropertiesRuler::gridPointToPoint( const GridPoint& gridPoint ) const {
	auto point = PatternEditor::gridPointToPoint( gridPoint );

	// In a horizontal editor we do not keep track of the y coordinate. We make
	// this design decision transparent by always setting y to 0.
	point.setY( 0 );
	return point;
}

GridPoint NotePropertiesRuler::pointToGridPoint( const QPoint& point,
												 bool bHonorQuantization ) const {
	auto gridPoint = PatternEditor::pointToGridPoint( point, bHonorQuantization );

	// In a horizontal editor we do not keep track of the y coordinate. We make
	// this design decision transparent by always returning an invalid row.
	gridPoint.setRow( -1 );
	return gridPoint;
}

void NotePropertiesRuler::mouseDrawStart( QMouseEvent *ev )
{
	setCursor( Qt::CrossCursor );
	prepareUndoAction( ev );

	updateEditor( Editor::Update::Content );
}


//! Preserve current note properties at position x (or in selection, if any) for
//! use in later UndoAction.
void NotePropertiesRuler::prepareUndoAction( QMouseEvent* pEvent )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( pEvent );

	m_oldNotes.clear();

	updateModifiers( pEvent );

	const auto notesUnderPoint = getElementsAtPoint(
		pEv->position().toPoint(), getCursorMargin( pEvent ), true );
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
void NotePropertiesRuler::mouseDrawUpdate( QMouseEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	updateModifiers( ev );

	auto pEv = static_cast<MouseEvent*>( ev );

	// Issuing redo/undo actions bases on draw changes are issued in batches. In
	// case the cursor is moved slowly, we might have updates without any new
	// notes. If it is moved rapidly, it might have passed several columns since
	// the last update. We will take all notes between the current position and
	// the last one into account.
	const auto gridPoint = pointToGridPoint( pEv->position().toPoint(), false );
	const auto row = m_pPatternEditorPanel->getRowDB(
			m_pPatternEditorPanel->getSelectedRowDB() );

	if ( m_nDrawPreviousColumn == -1 ) {
		m_nDrawPreviousColumn = gridPoint.getColumn();
	}

	const int nDrawStart = std::min( m_nDrawPreviousColumn, gridPoint.getColumn() );
	const int nDrawEnd = std::max( m_nDrawPreviousColumn, gridPoint.getColumn() );
	std::vector< std::shared_ptr<Note> > notesSinceLastAction;
	const auto notes = pPattern->getNotes();
	for ( auto it = notes->lower_bound( nDrawStart );
		  it != notes->end() && it->first <= nDrawEnd; ++it ) {
		const auto ppNote = it->second;
		if ( ppNote != nullptr && ( row.contains( ppNote ) ||
									m_selection.isSelected( ppNote ) ) ) {
			notesSinceLastAction.push_back( ppNote );
		}
	}

	if ( notesSinceLastAction.size() == 0 ) {
		return;
	}

	if ( m_nDrawPreviousColumn != gridPoint.getColumn() ) {
		// Complete current undo action, and start a new one.
		addUndoAction( "NotePropertiesRuler::mouseDraw" );
		for ( const auto& ppNote : notesSinceLastAction ) {
			m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
		}
		m_nDrawPreviousColumn = gridPoint.getColumn();
	}

	const auto fYValue = eventToYValue( ev );
	bool bValueChanged = false;

	for ( const auto& ppNote : notesSinceLastAction ) {
		// If a subset of notes is selected, we only act on them.
		if ( ! m_selection.isEmpty() && ! m_selection.isSelected( ppNote ) ) {
			continue;
		}

		if ( applyProperty( ppNote, m_property, fYValue ) ) {
			bValueChanged = true;
			triggerStatusMessage( notesSinceLastAction, m_property, true );
		}
	}

	if ( bValueChanged ) {
		Hydrogen::get_instance()->setIsModified( true );
		if ( m_property == PatternEditor::Property::Velocity ) {
			// A note's velocity determines its color in the other pattern
			// editors as well.
			updateVisibleComponents( Editor::Update::Content );
		}
		else {
			updateEditor( Editor::Update::Content );
		}
	}
}

void NotePropertiesRuler::mouseDrawEnd() {
	m_nDrawPreviousColumn = -1;
	addUndoAction( "NotePropertiesRuler::mouseDraw" );
	unsetCursor();
	updateEditor( Editor::Update::Content );
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

		switch( m_property ) {
		case PatternEditor::Property::Velocity: {
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
		case PatternEditor::Property::Pan: {
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
		case PatternEditor::Property::LeadLag: {
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
		case PatternEditor::Property::Probability: {
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
		case PatternEditor::Property::KeyOctave: {
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
		case PatternEditor::Property::None:
		default:
			ERRORLOG("No property set. No note property adjusted.");
		}
	}

	if ( bValueChanged ) {
		Hydrogen::get_instance()->setIsModified( true );
	}

	return bValueChanged;
}

void NotePropertiesRuler::applyCursorDelta( float fDelta ) {
	if ( fDelta == 0.0 ) {
		return;
	}

	// We interact only with notes under cursor. If any of them are part of the
	// current selection, we alter the values of all selected notes. It not, we
	// discard the selection.
	const auto notesUnderPoint = getElementsAtPoint(
		gridPointToPoint( getCursorPosition() ), 0, true );
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

		// We only show status messages for notes at cursor. In this case, only
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

	m_oldNotes.clear();
	for ( auto& ppNote : notes ) {
		if ( ppNote == nullptr ) {
			continue;
		}

		m_oldNotes[ ppNote ] = std::make_shared<Note>( ppNote );
	}

	// Apply delta to the property
	const bool bValueChanged = adjustNotePropertyDelta( notes, fDelta, false );

	if ( bValueChanged ) {
		triggerStatusMessage( notesStatusMessage, m_property );

		addUndoAction( "NotePropertiesRuler::keyPressEvent" );

		if ( m_property == PatternEditor::Property::Velocity ) {
			updateVisibleComponents( Editor::Update::Content );
		} else {
			updateEditor( Editor::Update::Content );
		}

		Hydrogen::get_instance()->setIsModified( true );
	}
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
				.arg( PatternEditor::propertyToQString( m_property ) )
				.arg( nSize ), sUndoContext );
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
					m_property,
					m_pPatternEditorPanel->getPatternNumber(),
					pNewNote->getPosition(),
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
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		const auto baseStyle = static_cast<NoteStyle>(
			( ppPattern == pPattern ? NoteStyle::Foreground :
			  NoteStyle::Background ) | NoteStyle::Hovered );
		sortAndDrawNotes( painter, nnotes, baseStyle, false );
	}

	// Draw moved notes
	int nOffsetX = 0;
	auto pEditor = m_pPatternEditorPanel->getVisibleEditor();
	if ( pEditor->isSelectionMoving() ) {
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

	QColor lineColor(
		pPref->getTheme().m_color.m_patternEditor_line5Color );
	QColor backgroundColor(
		pPref->getTheme().m_color.m_patternEditor_backgroundColor );

	// Everything beyond the current pattern (used when another, larger pattern
	// is played as well).
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	// Indicate chosen editor mode.
	QColor backgroundInactiveColor;
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Pattern ) {
		backgroundInactiveColor =
			pPref->getTheme().m_color.m_windowColor.lighter(
				Skin::nEditorActiveScaling );
	}
	else {
		backgroundInactiveColor = pPref->getTheme().m_color.m_windowColor;
	}

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
	
	painter.setPen( QPen( lineColor, 1, Qt::DotLine ) );
	for (unsigned y = 0; y < nHeight; y += nIncrement ) {
		painter.drawLine( 0, y, m_nActiveWidth, y );
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		painter.setPen( QPen( lineInactiveColor, 1, Qt::DotLine ) );
		for (unsigned y = 0; y < nHeight; y += nIncrement ) {
			painter.drawLine( m_nActiveWidth, y, m_nEditorWidth, y );
		}
	}

	drawBorders( painter );
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
		 ! ( m_property == PatternEditor::Property::Probability ||
			 m_property == PatternEditor::Property::LeadLag ) ) {
		return;
	}

	const int nLineWidth = 3;

	const int nX = nOffsetX + PatternEditor::nMargin +
		pNote->getPosition() * m_fGridWidth;

	// NoPlayback is handled in here in order to not bloat calling routines
	// (since it has to be calculated for every note drawn).
	if ( ! checkNotePlayback( pNote ) ) {
		noteStyle =
			static_cast<NoteStyle>(noteStyle | NoteStyle::NoPlayback);
	}

	QPen notePen, noteTailPen, highlightPen, movingPen;
	QBrush noteBrush, noteTailBrush, highlightBrush, movingBrush;
	applyColor( pNote, &notePen, &noteBrush, &noteTailPen, &noteTailBrush,
				&highlightPen, &highlightBrush, &movingPen, &movingBrush,
				noteStyle );

	p.setPen( notePen );
	p.setBrush( noteBrush );
	p.setRenderHint( QPainter::Antialiasing );

	// Silhouette to show when a note is selected and moved to a different
	// position (in another editor!).
	auto pEditor = m_pPatternEditorPanel->getVisibleEditor();
	QPoint movingOffsetPoint;
	GridPoint movingOffsetGridPoint;
	if ( noteStyle & NoteStyle::Moved ) {
		movingOffsetGridPoint = pEditor->movingGridOffset();
		movingOffsetPoint = QPoint(
			movingOffsetGridPoint.getColumn() * m_fGridWidth, 0 );
	}

	if ( m_layout == Layout::Centered || m_layout == Layout::Normalized ) {
		float fValue = 0;
		if ( m_property == PatternEditor::Property::Velocity ) {
			fValue = std::round( pNote->getVelocity() * height() );
		}
		else if ( m_property == PatternEditor::Property::Probability ) {
			fValue = std::round( pNote->getProbability() * height() );
		}
		else if ( m_property == PatternEditor::Property::Pan ) {
			// Rounding in order to not miss the center due to rounding errors
			// introduced in the Note class internals.
			fValue = std::round( pNote->getPan() * 100 ) / 100;
		}
		else if ( m_property == PatternEditor::Property::LeadLag ) {
			fValue = -1 * std::round( pNote->getLeadLag() * 100 ) / 100;
		}


		if ( m_layout == Layout::Centered && fValue == 0 ) {
			// value is centered - draw circle
			const int nY = static_cast<int>(std::round( height() * 0.5 ) );

			if ( ! ( noteStyle & NoteStyle::Moved ) ) {
				if ( noteStyle & ( NoteStyle::Selected |
								   NoteStyle::Hovered |
								   NoteStyle::NoPlayback ) ) {
					p.setPen( highlightPen );
					p.setBrush( highlightBrush );
					p.drawEllipse( nX - 7, nY - 7, 14, 14 );
				}

				p.setPen( notePen );
				p.setBrush( noteBrush );
				p.drawEllipse( nX - 4, nY - 4, 8, 8);
				p.setBrush( Qt::NoBrush );
			}
			else {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawEllipse( movingOffsetPoint.x() + nX - 6, nY - 6, 12, 12 );
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

			if ( ! ( noteStyle & NoteStyle::Moved ) ) {
				if ( noteStyle & ( NoteStyle::Selected |
								   NoteStyle::Hovered |
								   NoteStyle::NoPlayback ) ) {
					p.setPen( highlightPen );
					p.setBrush( highlightBrush );
					p.drawRoundedRect( nX - 1 - 4, nY - 4, nLineWidth + 8,
									   nHeight + 8, 5, 5 );
				}

				p.setPen( notePen );
				p.setBrush( noteBrush );
				p.drawRoundedRect( nX - 1 - 1, nY - 1,
								   nLineWidth + 2, nHeight + 2, 2, 2 );
				p.setBrush( Qt::NoBrush );
			}
			else {
				p.setPen( movingPen );
				p.setBrush( movingBrush );
				p.drawRoundedRect( movingOffsetPoint.x() + nX - 1 - 2, nY - 2,
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
		const int nKeyY = NotePropertiesRuler::nOctaveHeight +
			( ( KEYS_PER_OCTAVE - pNote->getKey() - 1 ) *
			  NotePropertiesRuler::nKeyLineHeight );

		// Paint selection outlines
		if ( noteStyle & ( NoteStyle::Selected |
						   NoteStyle::Hovered |
						   NoteStyle::NoPlayback ) ) {
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
			p.setPen( notePen );
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
				const int nNewPitch = pNote->getPitchFromKeyOctave() -
					movingOffsetGridPoint.getColumn();
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
				p.setBrush( movingBrush );
				p.drawEllipse( QPoint( movingOffsetPoint.x() + nX, nMovedOctaveY ),
							   nRadiusOctave + 1, nRadiusOctave + 1 );

				// Key
				p.drawEllipse( QPoint( movingOffsetPoint.x() + nX, nMovedKeyY ),
							   nRadiusKey + 1, nRadiusKey + 1 );
			}
		}
	}
}

void NotePropertiesRuler::sortAndDrawNotes( QPainter& p,
											std::vector< std::shared_ptr<Note> > notes,
											NoteStyle baseStyle,
											bool bUpdateOffsets ) {
	std::sort( notes.begin(), notes.end(), Note::compare );

	if ( bUpdateOffsets ) {
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
}

void NotePropertiesRuler::createBackground()
{
	const auto pPref = H2Core::Preferences::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();

	QColor lineColor(
		pPref->getTheme().m_color.m_patternEditor_lineColor );
	QColor textColor( pPref->getTheme().m_color.m_patternEditor_textColor );
	const QColor alternateRowColor =
		pPref->getTheme().m_color.m_patternEditor_alternateRowColor;
	const QColor octaveColor =
		pPref->getTheme().m_color.m_patternEditor_octaveRowColor;

	// Everything beyond the current pattern (used when another, larger pattern
	// is played as well).
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	// Indicate chosen editor mode.
	QColor backgroundInactiveColor;
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Pattern ) {
		backgroundInactiveColor =
			pPref->getTheme().m_color.m_windowColor.lighter(
				Skin::nEditorActiveScaling );
	}
	else {
		backgroundInactiveColor = pPref->getTheme().m_color.m_windowColor;
	}

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
	}

	updatePixmapSize();

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

			p.drawLine( PatternEditor::nMarginSidebar, yy, m_nActiveWidth, yy );
		}

		// Vertical border between sidebar and editor
		p.setPen( QPen( lineColor, 1, Qt::SolidLine ) );
		p.drawLine( PatternEditor::nMarginSidebar, 0,
					PatternEditor::nMarginSidebar, height() );

		if ( pPattern != nullptr ) {
			drawGridLines( p, Qt::DotLine );

			// Border between key and octave part
			p.setPen( QPen( lineColor, 1, Qt::SolidLine ) );
			p.drawLine( 0,
						NotePropertiesRuler::nOctaveHeight -
						NotePropertiesRuler::nKeyLineHeight / 2,
						m_nActiveWidth,
						NotePropertiesRuler::nOctaveHeight -
						NotePropertiesRuler::nKeyLineHeight / 2 );
			if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
				p.setPen( QPen( lineInactiveColor, 1, Qt::SolidLine ) );
				p.drawLine( m_nActiveWidth,
							NotePropertiesRuler::nOctaveHeight -
							NotePropertiesRuler::nKeyLineHeight / 2,
							m_nEditorWidth,
							NotePropertiesRuler::nOctaveHeight -
						NotePropertiesRuler::nKeyLineHeight / 2 );
			}

			// Horizontal grid lines in the key region
			p.setPen( QPen( lineColor, 1, Qt::DotLine));
			for ( int yy = NotePropertiesRuler::nOctaveHeight +
					  NotePropertiesRuler::nKeyLineHeight;
				  yy <= NotePropertiesRuler::nKeyOctaveHeight;
				  yy += NotePropertiesRuler::nKeyLineHeight ) {
				p.drawLine( PatternEditor::nMarginSidebar,
							yy - NotePropertiesRuler::nKeyLineHeight / 2,
							m_nActiveWidth,
							yy - NotePropertiesRuler::nKeyLineHeight / 2 );
			}

			if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
				p.setPen( QPen( lineInactiveColor, 1, Qt::DotLine ) );
				for ( int yy = NotePropertiesRuler::nOctaveHeight +
						  NotePropertiesRuler::nKeyLineHeight;
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

	validateSelection();

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	std::vector< std::shared_ptr<Note> > notes;

	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		const auto baseStyle = ppPattern == pPattern ?
			NoteStyle::Foreground : NoteStyle::Background;

		int nLastPos = -1;
		for ( const auto& [ nnPos, ppNote ] : *ppPattern->getNotes() ) {
			if ( ppNote == nullptr ) {
				continue;
			}

			if ( nnPos >= ppPattern->getLength() ) {
				break;
			}

			if ( nLastPos != nnPos ) {
				nLastPos = nnPos;
				sortAndDrawNotes( p, notes, baseStyle, true );
				notes.clear();
			}

			// NoteOff notes can have a custom probability and lead lag. But
			// having a velocity and pan would not make any sense for them.
			if ( ( ppNote->getNoteOff() &&
				   ! ( m_property == PatternEditor::Property::Probability ||
					   m_property == PatternEditor::Property::LeadLag ) ) ||
				 ! selectedRow.contains( ppNote ) &&
				 ! m_selection.isSelected( ppNote ) ) {
				continue;
			}

			notes.push_back( ppNote );
		}

		// Handle last column too
		if ( notes.size() > 0 ) {
			sortAndDrawNotes( p, notes, baseStyle, true );
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
		if ( ! selectedRow.contains( it->second ) &&
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
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const auto notes = getAllNotes();

	m_selection.clearSelection();
	for ( const auto& ppNote : notes ) {
		m_selection.addToSelection( ppNote );
	}
	updateVisibleComponents( Editor::Update::Content );
}

std::set< std::shared_ptr<H2Core::Note> > NotePropertiesRuler::getAllNotes() const {
	std::set< std::shared_ptr<Note> > notes;

	const auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return std::move( notes );
	}

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
		if ( ppNote != nullptr &&
			 ( m_selection.isSelected( ppNote ) || row.contains( ppNote ) ) ) {
			notes.insert( ppNote );
		}
	}

	// Add hovered notes as well. We rely on std::set to ensure uniqueness.
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		if ( ppPattern != pPattern ) {
			continue;
		}

		for ( const auto& ppHoveredNote : nnotes ) {
			if ( ppHoveredNote != nullptr ) {
				notes.insert( ppHoveredNote );
			}
		}
	}

	return std::move( notes );
}
