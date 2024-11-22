/*
 * Hydrogen
 * Copyright(c) 2002-2008 by the Hydrogen Team
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

#include "PatternEditor.h"
#include "PatternEditorRuler.h"
#include "PatternEditorInstrumentList.h"
#include "PatternEditorPanel.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../EventListener.h"
#include "../UndoActions.h"
#include "../Skin.h"

#include <core/Globals.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/EventQueue.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Xml.h>


using namespace std;
using namespace H2Core;


PatternEditor::PatternEditor( QWidget *pParent )
	: Object()
	, QWidget( pParent )
	, m_selection( this )
	, m_bEntered( false )
	, m_bSelectNewNotes( false )
	, m_bFineGrained( false )
	, m_bCopyNotMove( false )
	, m_nTick( -1 )
	, m_editor( Editor::None )
	, m_mode( Mode::None )
	, m_nCursorRow( 0 )
	  , m_nDragStartColumn( 0 )
	  , m_nDragY( 0 )
{
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	const auto pPref = H2Core::Preferences::get_instance();

	m_nResolution = pPref->getPatternEditorGridResolution();
	m_bUseTriplets = pPref->isPatternEditorUsingTriplets();
	m_fGridWidth = pPref->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nActiveWidth = m_nEditorWidth;

	setFocusPolicy(Qt::StrongFocus);

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &PatternEditor::onPreferencesChanged );
	
	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "&Cut" ), this, SLOT( cut() ) ) );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "&Copy" ), this, SLOT( copy() ) ) );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, SLOT( paste() ) );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "&Delete" ), this, SLOT( deleteSelection() ) ) );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "A&lign to grid" ), this, SLOT( alignToGrid() ) ) );
	m_selectionActions.push_back( m_pPopupMenu->addAction( tr( "Randomize velocity" ), this, SLOT( randomizeVelocity() ) ) );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, SLOT( selectAll() ) );
	m_selectionActions.push_back( 	m_pPopupMenu->addAction( tr( "Clear selection" ), this, SLOT( selectNone() ) ) );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
									   height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	m_bBackgroundInvalid = true;
}

PatternEditor::~PatternEditor()
{
	clearDraggedNotes();

	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
}

void PatternEditor::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		
		update( 0, 0, width(), height() );
		m_pPatternEditorPanel->updateEditors();
	}
}

void PatternEditor::setResolution(uint res, bool bUseTriplets)
{
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;

	// redraw all
	update( 0, 0, width(), height() );
	m_pPatternEditorPanel->updateEditors();
}

void PatternEditor::zoomIn()
{
	if (m_fGridWidth >= 3) {
		m_fGridWidth *= 2;
	} else {
		m_fGridWidth *= 1.5;
	}
	updateEditor();
}

void PatternEditor::zoomOut()
{
	if ( m_fGridWidth > 1.5 ) {
		if (m_fGridWidth > 3) {
			m_fGridWidth /= 2;
		} else {
			m_fGridWidth /= 1.5;
		}
		updateEditor();
	}
}

QColor PatternEditor::computeNoteColor( float fVelocity ) {
	float fRed, fGreen, fBlue;

	const auto pPref = H2Core::Preferences::get_instance();

	QColor fullColor = pPref->getTheme().m_color.m_patternEditor_noteVelocityFullColor;
	QColor defaultColor = pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor;
	QColor halfColor = pPref->getTheme().m_color.m_patternEditor_noteVelocityHalfColor;
	QColor zeroColor = pPref->getTheme().m_color.m_patternEditor_noteVelocityZeroColor;

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


void PatternEditor::drawNoteSymbol( QPainter &p, const QPoint& pos,
									H2Core::Note *pNote,
									bool bIsForeground ) const
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	const auto pPref = H2Core::Preferences::get_instance();
	
	const QColor noteColor( pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor );
	const QColor noteInactiveColor( pPref->getTheme().m_color.m_windowTextColor.darker( 150 ) );
	const QColor noteoffColor( pPref->getTheme().m_color.m_patternEditor_noteOffColor );
	const QColor noteoffInactiveColor( pPref->getTheme().m_color.m_windowTextColor );

	p.setRenderHint( QPainter::Antialiasing );

	QColor color = computeNoteColor( pNote->get_velocity() );

	uint w = 8, h =  8;
	uint x_pos = pos.x(), y_pos = pos.y();

	bool bSelected = m_selection.isSelected( pNote );

	if ( bSelected ) {
		QPen selectedPen( selectedNoteColor() );
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
		movingOffset = QPoint( delta.x() * m_fGridWidth,
							   delta.y() * m_nGridHeight );
	}

	if ( pNote->get_note_off() == false ) {	// trigger note
		int width = w;

		QBrush noteBrush( color );
		QPen notePen( noteColor );
		if ( !bIsForeground ) {

			if ( x_pos >= m_nActiveWidth ) {
				noteBrush.setColor( noteInactiveColor );
				notePen.setColor( noteInactiveColor );
			}
			
			noteBrush.setStyle( Qt::Dense4Pattern );
			notePen.setStyle( Qt::DotLine );
		}

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
		}

		// Draw tail
		if ( pNote->get_length() != LENGTH_ENTIRE_SAMPLE ) {
			float fNotePitch = pNote->get_pitch_from_key_octave();
			float fStep = Note::pitchToFrequency( ( double )fNotePitch );

			// if there is a stop-note to the right of this note, only draw-
			// its length till there.
			int nLength = pNote->get_length();
			auto notes = pPattern->getNotes();
			for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
				if ( ppNote != nullptr &&
					 // noteOff note
					 ppNote->get_note_off() &&
					 // located in the same row
					 ppNote->get_instrument() == pNote->get_instrument() &&
					 // left of the NoteOff
					 pNote->get_position() < ppNote->get_position() &&
					 // custom length reaches beyond NoteOff
					 pNote->get_position() + pNote->get_length() >
					 ppNote->get_position() ) {
					// In case there are multiple stop-notes present, take the
					// shortest distance.
					nLength = std::min( ppNote->get_position() - pNote->get_position(), nLength );
				}
			}

			width = m_fGridWidth * nLength / fStep;
			width = width - 1;	// lascio un piccolo spazio tra una nota ed un altra

			if ( bSelected ) {
				p.drawRoundedRect( x_pos-2, y_pos, width+4, 3+4, 4, 4 );
			}
			p.setPen( notePen );
			p.setBrush( noteBrush );

			// Since the note body is transparent for an inactive note, we try
			// to start the tail at its boundary. For regular notes we do not
			// care about an overlap, as it ensures that there are no white
			// artifacts between tail and note body regardless of the scale
			// factor.
			int nRectOnsetX = x_pos;
			int nRectWidth = width;
			if ( ! bIsForeground ) {
				nRectOnsetX = nRectOnsetX + w/2;
				nRectWidth = nRectWidth - w/2;
			}

			p.drawRect( nRectOnsetX, y_pos +2, nRectWidth, 3 );
			p.drawLine( x_pos+width, y_pos, x_pos+width, y_pos + h );
		}

		p.setPen( notePen );
		p.setBrush( noteBrush );
		p.drawEllipse( x_pos -4 , y_pos, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + x_pos -4 -2, movingOffset.y() + y_pos -2 , w + 4, h + 4 );
			// Moving tail
			if ( pNote->get_length() != LENGTH_ENTIRE_SAMPLE ) {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawRoundedRect( movingOffset.x() + x_pos-2, movingOffset.y() + y_pos, width+4, 3+4, 4, 4 );
			}
		}
	}
	else if ( pNote->get_length() == 1 && pNote->get_note_off() == true ) {

		QBrush noteOffBrush( noteoffColor );
		if ( !bIsForeground ) {
			noteOffBrush.setStyle( Qt::Dense4Pattern );

			if ( x_pos >= m_nActiveWidth ) {
				noteOffBrush.setColor( noteoffInactiveColor );
			}
		}

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
		}

		p.setPen( Qt::NoPen );
		p.setBrush( noteOffBrush );
		p.drawEllipse( x_pos -4 , y_pos, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + x_pos -4 -2, movingOffset.y() + y_pos -2, w + 4, h + 4 );
		}
	}
}


int PatternEditor::getColumn( int x, bool bUseFineGrained ) const
{
	int nGranularity = 1;
	if ( !( bUseFineGrained && m_bFineGrained ) ) {
		nGranularity = granularity();
	}
	int nWidth = m_fGridWidth * nGranularity;
	int nColumn = ( x - PatternEditor::nMargin + (nWidth / 2) ) / nWidth;
	nColumn = nColumn * nGranularity;
	if ( nColumn < 0 ) {
		return 0;
	} else {
		return nColumn;
	}
}

void PatternEditor::mouseEventToColumnRow( QMouseEvent* pEvent, int* pColumn,
										   int* pRow, int* pRealColumn,
										   bool bUseFineGrained ) const {
	if ( pRow != nullptr ) {
		*pRow = static_cast<int>(
			std::floor( static_cast<float>(pEvent->y()) /
						static_cast<float>(m_nGridHeight) ) );
	}

	if ( pColumn != nullptr ) {
		*pColumn = getColumn( pEvent->x(), bUseFineGrained );
	}

	if ( pRealColumn != nullptr ) {
		if ( pEvent->x() > PatternEditor::nMargin ) {
			*pRealColumn = static_cast<int>(
				std::floor( ( pEvent->x() -
							  static_cast<float>(PatternEditor::nMargin) ) /
							static_cast<float>(m_fGridWidth) ) );
		}
		else {
			*pRealColumn = 0;
		}
	}
}

void PatternEditor::selectNone()
{
	m_selection.clearSelection();
	m_selection.updateWidgetGroup();
}

void PatternEditor::showPopupMenu( const QPoint &pos )
{
	// Enable or disable menu actions that only operate on selections.
	bool bEmpty = m_selection.isEmpty();
	for ( auto & action : m_selectionActions ) {
		action->setEnabled( !bEmpty );
	}

	m_pPopupMenu->popup( pos );
}

///
/// Copy selection to clipboard in XML
///
void PatternEditor::copy()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pInstrumentList = pHydrogen->getSong()->getDrumkit()->getInstruments();
	XMLDoc doc;
	XMLNode selection = doc.set_root( "noteSelection" );
	XMLNode noteList = selection.createNode( "noteList");
	XMLNode positionNode = selection.createNode( "sourcePosition" );
	bool bWroteNote = false;
	// "Top left" of selection, in the three dimensional time*instrument*pitch space.
	int nMinColumn, nMinRow, nMaxPitch;

	for ( Note *pNote : m_selection ) {
		const int nPitch = pNote->get_pitch_from_key_octave();
		const int nColumn = pNote->get_position();
		const int nRow = pInstrumentList->index( pNote->get_instrument() );
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
		pNote->save_to( note_node );
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

	// This selection will probably be pasted at some point. So show the
	// keyboard cursor as this is the place where the selection will be pasted.
	HydrogenApp::get_instance()->setHideKeyboardCursor( false );
}


void PatternEditor::cut()
{
	copy();
	deleteSelection();
}

///
/// Paste selection
///
/// Selection is XML containing notes, contained in a root 'note_selection' element.
///
void PatternEditor::paste()
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	QClipboard *clipboard = QApplication::clipboard();
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();
	const auto selectedRow = m_pPatternEditorPanel->getRowDB( nSelectedRow );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
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
			if ( m_editor == Editor::PianoRoll ) {
				nDeltaPitch = m_nCursorRow -
					positionNode.read_int( "maxPitch", m_nCursorRow );
			}
			else if ( m_editor == Editor::DrumPattern ) {
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
	m_bSelectNewNotes = true;

	if ( noteList.hasChildNodes() ) {

		pUndo->beginMacro( tr( "paste notes" ) );
		for ( XMLNode n = noteList.firstChildElement( "note" ); ! n.isNull();
			  n = n.nextSiblingElement() ) {
			auto pNote = Note::load_from( n );
			if ( pNote == nullptr ) {
				ERRORLOG( QString( "Unable to load note from XML node [%1]" )
						  .arg( n.toQString() ) );
				continue;
			}

			// Drumkit might have changed since the notes have been copied to
			// clipboard.
			pNote->mapTo( pSong->getDrumkit() );

			const int nPos = pNote->get_position() + nDeltaPos;
			if ( nPos < 0 || nPos >= pPattern->getLength() ) {
				delete pNote;
				continue;
			}

			int nRow;
			if ( m_editor == Editor::DrumPattern ) {
				nRow = m_pPatternEditorPanel->findRowDB( pNote ) + nDeltaRow;
			}
			else {
				nRow = nSelectedRow;
			}
			if ( nRow < 0 || nRow >= m_pPatternEditorPanel->getRowNumberDB() ) {
				delete pNote;
				continue;
			}

			int nKey, nOctave;
			if ( m_editor == Editor::PianoRoll ) {
				const int nPitch = pNote->get_pitch_from_key_octave() + nDeltaPitch;
				if ( nPitch < KEYS_PER_OCTAVE * OCTAVE_MIN ||
					 nPitch >= KEYS_PER_OCTAVE * ( OCTAVE_MAX + 1 ) ) {
					delete pNote;
					continue;
				}

				nKey = Note::pitchToKey( nPitch );
				nOctave = Note::pitchToOctave( nPitch );
			}
			else {
				nKey = pNote->get_key();
				nOctave = pNote->get_octave();
			}

			pUndo->push( new SE_addOrRemoveNoteAction(
							 nPos,
							 nRow,
							 m_pPatternEditorPanel->getPatternNumber(),
							 pNote->get_length(),
							 pNote->get_velocity(),
							 pNote->getPan(),
							 pNote->get_lead_lag(),
							 nKey,
							 nOctave,
							 pNote->get_probability(),
							 /* bIsDelete */ false,
							 /* bIsMidi */ false,
							 /* bIsNoteOff */ pNote->get_note_off() ) );
			delete pNote;
		}
		pUndo->endMacro();
	}

	m_bSelectNewNotes = false;
}

void PatternEditor::selectAllNotesInRow( int nRow )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto row = m_pPatternEditorPanel->getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
		DEBUGLOG( QString( "Invalid row [%1]" ).arg( nRow ) );
		return;
	}
	
	m_selection.clearSelection();
	for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
		if ( ppNote != nullptr &&
			 ppNote->get_instrument_id() == row.nInstrumentID &&
			 ppNote->getType() == row.sType ) {
			m_selection.addToSelection( ppNote );
		}
	}
	m_selection.updateWidgetGroup();
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

	validateSelection();
	if ( m_selection.isEmpty() ) {
		return;
	}

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

	// Move the notes
	pUndo->beginMacro( tr( "Align notes to grid" ) );

	for ( Note *pNote : m_selection ) {
		if ( pNote == nullptr ) {
			continue;
		}

		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
		const int nPosition = pNote->get_position();
		const int nNewInstrument = nRow;
		const int nGranularity = granularity();

		// Round to the nearest position in the current grid. We add 1 to round
		// up when the note is precisely in the middle. This allows us to change
		// a 4/4 pattern to a 6/8 swing feel by changing the grid to 1/8th
		// triplest, and hitting 'align'.
		const int nNewPosition = nGranularity *
			( (nPosition+(nGranularity/2)+1) / nGranularity );

		// Move note -> delete at source position
		pUndo->push( new SE_addOrRemoveNoteAction(
						 nPosition,
						 nRow,
						 m_pPatternEditorPanel->getPatternNumber(),
						 pNote->get_length(),
						 pNote->get_velocity(),
						 pNote->getPan(),
						 pNote->get_lead_lag(),
						 pNote->get_key(),
						 pNote->get_octave(),
						 pNote->get_probability(),
						 /* bIsDelete */ true,
						 /* bIsMidi */ false,
						 /* bIsNoteOff */ pNote->get_note_off() ) );

		// Add at target position
		pUndo->push( new SE_addOrRemoveNoteAction(
						 nNewPosition,
						 nRow,
						 m_pPatternEditorPanel->getPatternNumber(),
						 pNote->get_length(),
						 pNote->get_velocity(),
						 pNote->getPan(),
						 pNote->get_lead_lag(),
						 pNote->get_key(),
						 pNote->get_octave(),
						 pNote->get_probability(),
						 /* bIsDelete */ false,
						 /* bIsMidi */ false,
						 /* bIsNoteOff */ pNote->get_note_off() ) );
	}

	pUndo->endMacro();
}


void PatternEditor::randomizeVelocity() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected. Nothing to be randomized.
		return;
	}

	validateSelection();
	if ( m_selection.isEmpty() ) {
		return;
	}
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

	pUndo->beginMacro( tr( "Random velocity" ) );

	for ( const auto pNote : m_selection ) {
		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
		if ( nRow == -1 ) {
			ERRORLOG( "Selected note not found" );
			continue;
		}

		float fVal = ( rand() % 100 ) / 100.0;
		fVal = std::clamp( pNote->get_velocity() + ( ( fVal - 0.50 ) / 2 ),
						   0.0, 1.0 );
		pUndo->push( new SE_editNotePropertiesAction(
						 PatternEditor::Mode::Velocity,
						 m_pPatternEditorPanel->getPatternNumber(),
						 pNote->get_position(),
						 nRow,
						 fVal,
						 pNote->get_velocity(),
						 pNote->getPan(),
						 pNote->getPan(),
						 pNote->get_lead_lag(),
						 pNote->get_lead_lag(),
						 pNote->get_probability(),
						 pNote->get_probability(),
						 pNote->get_length(),
						 pNote->get_length(),
						 pNote->get_key(),
						 pNote->get_key(),
						 pNote->get_octave(),
						 pNote->get_octave() ) );
	}

	pUndo->endMacro();

}

void PatternEditor::mousePressEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mousePressEvent( ev );
}

void PatternEditor::mouseMoveEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	if ( m_selection.isMoving() ) {
		updateEditor( true );
	}
	m_selection.mouseMoveEvent( ev );
}

void PatternEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mouseReleaseEvent( ev );
}

void PatternEditor::updateModifiers( QInputEvent *ev ) {
	// Key: Alt + drag: move notes with fine-grained positioning
	m_bFineGrained = ev->modifiers() & Qt::AltModifier;
	// Key: Ctrl + drag: copy notes rather than moving
	m_bCopyNotMove = ev->modifiers() & Qt::ControlModifier;

	if ( QKeyEvent *pEv = dynamic_cast<QKeyEvent*>( ev ) ) {
		// Keyboard events for press and release of modifier keys don't have those keys in the modifiers set,
		// so explicitly update these.
		bool bPressed = ev->type() == QEvent::KeyPress;
		if ( pEv->key() == Qt::Key_Control ) {
			m_bCopyNotMove = bPressed;
		} else if ( pEv->key() == Qt::Key_Alt ) {
			m_bFineGrained = bPressed;
		}
	}

	if ( m_selection.isMouseGesture() && m_selection.isMoving() ) {
		// If a selection is currently being moved, change the cursor
		// appropriately. Selection will change it back after the move
		// is complete (or abandoned)
		if ( m_bCopyNotMove &&  cursor().shape() != Qt::DragCopyCursor ) {
			setCursor( QCursor( Qt::DragCopyCursor ) );
		} else if ( !m_bCopyNotMove && cursor().shape() != Qt::DragMoveCursor ) {
			setCursor( QCursor( Qt::DragMoveCursor ) );
		}
	}
}

bool PatternEditor::notesMatchExactly( Note *pNoteA, Note *pNoteB ) const {
	return ( pNoteA->match( pNoteB )
			 && pNoteA->get_position() == pNoteB->get_position()
			 && pNoteA->get_velocity() == pNoteB->get_velocity()
			 && pNoteA->getPan() == pNoteB->getPan()
			 && pNoteA->get_lead_lag() == pNoteB->get_lead_lag()
			 && pNoteA->get_probability() == pNoteB->get_probability() );
}

bool PatternEditor::checkDeselectElements( const std::vector<SelectionIndex>& elements )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return false;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	//	Hydrogen *pH = Hydrogen::get_instance();
	std::set< Note *> duplicates;
	for ( Note *pNote : elements ) {
		if ( duplicates.find( pNote ) != duplicates.end() ) {
			// Already marked pNote as a duplicate of some other pNote. Skip it.
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND_END( pPattern->getNotes(), it, pNote->get_position() ) {
			// Duplicate note of a selected note is anything occupying the same position. Multiple notes
			// sharing the same location might be selected; we count these as duplicates too. They will appear
			// in both the duplicates and selection lists.
			if ( it->second != pNote && pNote->match( it->second ) ) {
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
			QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

			std::vector< Note *>overwritten;
			for ( Note *pNote : duplicates ) {
				overwritten.push_back( pNote );
			}
			pUndo->push( new SE_deselectAndOverwriteNotesAction( elements, overwritten ) );

		} else {
			return false;
		}
	}
	return true;
}


void PatternEditor::deselectAndOverwriteNotes( const std::vector< H2Core::Note *>& selected,
											   const std::vector< H2Core::Note *>& overwritten )
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
		int nPosition = pSelectedNote->get_position();
		for ( auto it = pNotes->lower_bound( nPosition ); it != pNotes->end() && it->first == nPosition; ) {
			Note *pNote = it->second;
			if ( !bFoundExact && notesMatchExactly( pNote, pSelectedNote ) ) {
				// Found an exact match. We keep this.
				bFoundExact = true;
				++it;
			} else if ( pSelectedNote->match( pNote ) && pNote->get_position() == pSelectedNote->get_position() ) {
				// Something else occupying the same position (which may or may not be an exact duplicate)
				it = pNotes->erase( it );
				delete pNote;
			} else {
				// Any other note
				++it;
			}
		}
	}
	pHydrogen->getAudioEngine()->unlock();
	pHydrogen->setIsModified( true );
}


void PatternEditor::undoDeselectAndOverwriteNotes( const std::vector< H2Core::Note *>& selected,
												   const std::vector< H2Core::Note *>& overwritten )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	// Restore previously-overwritten notes, and select notes that were selected before.
	m_selection.clearSelection( /* bCheck=*/false );
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	for ( auto pNote : overwritten ) {
		Note *pNewNote = new Note( pNote );
		pPattern->insertNote( pNewNote );
	}
	// Select the previously-selected notes
	for ( auto pNote : selected ) {
		FOREACH_NOTE_CST_IT_BOUND_END( pPattern->getNotes(), it, pNote->get_position() ) {
			if ( notesMatchExactly( it->second, pNote ) ) {
				m_selection.addToSelection( it->second );
				break;
			}
		}
	}
	pHydrogen->getAudioEngine()->unlock();
	pHydrogen->setIsModified( true );
	m_pPatternEditorPanel->updateEditors();
}

QPoint PatternEditor::movingGridOffset( ) const {
	QPoint rawOffset = m_selection.movingOffset();
	// Quantize offset to multiples of m_nGrid{Width,Height}
	int nQuantX = m_fGridWidth, nQuantY = m_nGridHeight;
	float nFactor = 1;
	if ( ! m_bFineGrained ) {
		nFactor = granularity();
		nQuantX = m_fGridWidth * nFactor;
	}
	int x_bias = nQuantX / 2, y_bias = nQuantY / 2;
	if ( rawOffset.y() < 0 ) {
		y_bias = -y_bias;
	}
	if ( rawOffset.x() < 0 ) {
		x_bias = -x_bias;
	}
	int x_off = (rawOffset.x() + x_bias) / nQuantX;
	int y_off = (rawOffset.y() + y_bias) / nQuantY;
	return QPoint( nFactor * x_off, y_off);
}


//! Draw lines for note grid.
void PatternEditor::drawGridLines( QPainter &p, const Qt::PenStyle& style ) const
{
	const auto pPref = H2Core::Preferences::get_instance();
	const std::vector<QColor> colorsActive = {
		QColor( pPref->getTheme().m_color.m_patternEditor_line1Color ),
		QColor( pPref->getTheme().m_color.m_patternEditor_line2Color ),
		QColor( pPref->getTheme().m_color.m_patternEditor_line3Color ),
		QColor( pPref->getTheme().m_color.m_patternEditor_line4Color ),
		QColor( pPref->getTheme().m_color.m_patternEditor_line5Color ),
	};
	const std::vector<QColor> colorsInactive = {
		QColor( pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) ),
		QColor( pPref->getTheme().m_color.m_windowTextColor.darker( 190 ) ),
		QColor( pPref->getTheme().m_color.m_windowTextColor.darker( 210 ) ),
		QColor( pPref->getTheme().m_color.m_windowTextColor.darker( 230 ) ),
		QColor( pPref->getTheme().m_color.m_windowTextColor.darker( 250 ) ),
	};

	if ( !m_bUseTriplets ) {

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
		const int nRes = 4;
		float fStep = MAX_NOTES / nRes * m_fGridWidth;
		float x = PatternEditor::nMargin;
		p.setPen( QPen( colorsActive[ 0 ], 1, style ) );
		while ( x < m_nActiveWidth ) {
			p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			x += fStep;
		}
			
		p.setPen( QPen( colorsInactive[ 0 ], 1, style ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			x += fStep;
		}

		// Resolution 4 was already taken into account above;
		std::vector<int> availableResolutions = { 8, 16, 32, 64, MAX_NOTES };

		// For each successive set of finer-spaced lines, the even
		// lines will have already been drawn at the previous coarser
		// pitch, so only the odd numbered lines need to be drawn.
		int nColour = 1;
		for ( int nnRes : availableResolutions ) {
			if ( nnRes > m_nResolution ) {
				break;
			}

			fStep = MAX_NOTES / nnRes * m_fGridWidth;
			float x = PatternEditor::nMargin + fStep;
			p.setPen( QPen( colorsActive[ std::min( nColour, static_cast<int>(colorsActive.size()) - 1 ) ],
							1, style ) );

			if ( nnRes != MAX_NOTES ) {
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
				// Between 1/64 -> 1/192 (1/MAX_NOTES) the space between
				// existing grid line will be filled by two instead of one new
				// line.
				while ( x < m_nActiveWidth + fStep ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep;
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}

			p.setPen( QPen( colorsInactive[ std::min( nColour, static_cast<int>(colorsInactive.size()) - 1 ) ],
							1, style ) );
			if ( nnRes != MAX_NOTES ) {
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

	} else {

		// Triplet style markers, we only differentiate colours on the
		// first of every triplet.
		float fStep = granularity() * m_fGridWidth;
		float x = PatternEditor::nMargin;
		p.setPen(  QPen( colorsActive[ 0 ], 1, style ) );
		while ( x < m_nActiveWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			x += fStep * 3;
		}
		
		p.setPen(  QPen( colorsInactive[ 0 ], 1, style ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			x += fStep * 3;
		}
		
		// Second and third marks
		x = PatternEditor::nMargin + fStep;
		p.setPen(  QPen( colorsActive[ 2 ], 1, style ) );
		while ( x < m_nActiveWidth + fStep ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
			x += fStep * 3;
		}
		
		p.setPen( QPen( colorsInactive[ 2 ], 1, style ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
			x += fStep * 3;
		}
	}

}


QColor PatternEditor::selectedNoteColor() const {
	
	const auto pPref = H2Core::Preferences::get_instance();
	
	if ( hasFocus() ) {
		const QColor selectHighlightColor( pPref->getTheme().m_color.m_selectionHighlightColor );
		return selectHighlightColor;
	} else {
		const QColor selectInactiveColor( pPref->getTheme().m_color.m_selectionInactiveColor );
		return selectInactiveColor;
	}
}


///
/// Ensure selection only refers to valid notes, and does not contain any stale references to deleted notes.
///
void PatternEditor::validateSelection()
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	
	// Rebuild selection from valid notes.
	std::set<Note *> valid;
	std::vector< Note *> invalidated;
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

void PatternEditor::deleteSelection()
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	if ( m_selection.begin() != m_selection.end() ) {
		// Selection exists, delete it.
		QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
		validateSelection();

		// Construct list of UndoActions to perform before performing any of them, as the
		// addOrDeleteNoteAction may delete duplicate notes in undefined order.
		std::list< QUndoCommand *> actions;
		for ( const auto pNote : m_selection ) {
			if ( pNote != nullptr && m_selection.isSelected( pNote ) ) {
				actions.push_back( new SE_addOrRemoveNoteAction(
									   pNote->get_position(),
									   m_pPatternEditorPanel->findRowDB( pNote ),
									   m_pPatternEditorPanel->getPatternNumber(),
									   pNote->get_length(),
									   pNote->get_velocity(),
									   pNote->getPan(),
									   pNote->get_lead_lag(),
									   pNote->get_key(),
									   pNote->get_octave(),
									   pNote->get_probability(),
									   true, // bIsDelete
									   false, // bIsMidi
									   pNote->get_note_off() ) );
			}
		}
		m_selection.clearSelection();

		if ( actions.size() > 0 ) {
			pUndo->beginMacro( tr( "delete notes" ) );
			for ( QUndoCommand *pAction : actions ) {
				pUndo->push( pAction );
			}
			pUndo->endMacro();
		}
	}
}

// Selection manager interface
void PatternEditor::selectionMoveEndEvent( QInputEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	updateModifiers( ev );

	QPoint offset = movingGridOffset();
	if ( offset.x() == 0 && offset.y() == 0 ) {
		// Move with no effect.
		return;
	}

	validateSelection();

	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();

	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

	if ( m_bCopyNotMove ) {
		pUndo->beginMacro( tr( "copy notes" ) );
	} else {
		pUndo->beginMacro( tr( "move notes" ) );
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
		if ( pNote == nullptr ) {
			continue;
		}
		const int nPosition = pNote->get_position();
		const int nNewPosition = nPosition + offset.x();

		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
		int nNewRow = nRow;
		// For all other editors the moved/copied note is still associated to
		// the same instrument.
		if ( m_editor == Editor::DrumPattern && offset.y() != 0 ) {
			nNewRow += offset.y();
		}

		int nNewKey = pNote->get_key();
		int nNewOctave = pNote->get_octave();
		int nNewPitch = pNote->get_pitch_from_key_octave();
		if ( m_editor == Editor::PianoRoll && offset.y() != 0 ) {
			nNewPitch -= offset.y();
			nNewKey = Note::pitchToKey( nNewPitch );
			nNewOctave = Note::pitchToOctave( nNewPitch );
		}

		// For NotePropertiesRuler there is no vertical displacement.

		bool bNoteInRange = nNewPosition >= 0 &&
			nNewPosition <= pPattern->getLength();
		if ( m_editor == Editor::DrumPattern ) {
			bNoteInRange = bNoteInRange && nNewRow > 0 &&
				nNewRow <= m_pPatternEditorPanel->getRowNumberDB();
		}
		else if ( m_editor == Editor::PianoRoll ){
			bNoteInRange = bNoteInRange && nNewOctave >= OCTAVE_MIN &&
				nNewOctave <= OCTAVE_MAX;
		}

		if ( ! m_bCopyNotMove ) {
			// Note is moved either out of range or to a new position. Delete
			// the note at the source position.
			pUndo->push( new SE_addOrRemoveNoteAction(
							 nPosition,
							 nRow,
							 m_pPatternEditorPanel->getPatternNumber(),
							 pNote->get_length(),
							 pNote->get_velocity(),
							 pNote->getPan(),
							 pNote->get_lead_lag(),
							 pNote->get_key(),
							 pNote->get_octave(),
							 pNote->get_probability(),
							 /* bIsDelete */ true,
							 /* bIsMidi */ false,
							 /* bIsNoteOff */ pNote->get_note_off() ) );
		}

		if ( bNoteInRange ) {
			// Create a new note at the target position
			pUndo->push( new SE_addOrRemoveNoteAction(
							 nNewPosition,
							 nNewRow,
							 m_pPatternEditorPanel->getPatternNumber(),
							 pNote->get_length(),
							 pNote->get_velocity(),
							 pNote->getPan(),
							 pNote->get_lead_lag(),
							 nNewKey,
							 nNewOctave,
							 pNote->get_probability(),
							 /* bIsDelete */ false,
							 /* bIsMidi */ false,
							 /* bIsNoteOff */ pNote->get_note_off() ) );
		}
	}

	m_bSelectNewNotes = false;
	pUndo->endMacro();
}

void PatternEditor::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

void PatternEditor::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void PatternEditor::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void PatternEditor::focusInEvent( QFocusEvent *ev ) {
	UNUSED( ev );
	if ( ev->reason() == Qt::TabFocusReason || ev->reason() == Qt::BacktabFocusReason ) {
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
		m_pPatternEditorPanel->ensureCursorVisible();
	}
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getInstrumentList()->update();
	}

	// Update to show the focus border highlight
	update();
}

void PatternEditor::focusOutEvent( QFocusEvent *ev ) {
	UNUSED( ev );
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getInstrumentList()->update();
	}
	
	// Update to remove the focus border highlight
	update();
}

void PatternEditor::invalidateBackground() {
	m_bBackgroundInvalid = true;
}

void PatternEditor::createBackground() {
}

//! Get notes to show in pattern editor.
//! This may include "background" notes that are in currently-playing patterns
//! rather than the current pattern.
std::vector<std::shared_ptr<Pattern>> PatternEditor::getPatternsToShow( void )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::vector<std::shared_ptr<Pattern>> patterns;
	auto pPattern = m_pPatternEditorPanel->getPattern();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	// When using song mode without the pattern editor being locked
	// only the current pattern will be shown. In every other base
	// remaining playing patterns not selected by the user are added
	// as well.
	if ( ! ( pHydrogen->getMode() == Song::Mode::Song &&
			 ! pHydrogen->isPatternEditorLocked() ) ) {
		pAudioEngine->lock( RIGHT_HERE );
		if ( pAudioEngine->getPlayingPatterns()->size() > 0 ) {
			std::set<std::shared_ptr<Pattern>> patternSet;

			std::vector<const PatternList*> patternLists;
			patternLists.push_back( pAudioEngine->getPlayingPatterns() );
			if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
				patternLists.push_back( pAudioEngine->getNextPatterns() );
			}
		
			for ( const auto& pPatternList : patternLists ) {
				for ( int i = 0; i <  pPatternList->size(); i++) {
					auto ppPattern = pPatternList->get( i );
					if ( ppPattern != pPattern ) {
						patternSet.insert( ppPattern );
					}
				}
			}
			for ( const auto& ppPattern : patternSet ) {
				patterns.push_back( ppPattern );
			}
		}
		pAudioEngine->unlock();
	}
	else if ( pPattern != nullptr &&
			  pHydrogen->getMode() == Song::Mode::Song &&
			  pPattern->getVirtualPatterns()->size() > 0 ) {
		// A virtual pattern was selected in song mode without the
		// pattern editor being locked. Virtual patterns in selected
		// pattern mode are handled using the playing pattern above.
		for ( const auto ppVirtualPattern : *pPattern ) {
			patterns.push_back( ppVirtualPattern );
		}
	}
			  

	if ( pPattern != nullptr ) {
		patterns.push_back( pPattern );
	}

	return patterns;
}

bool PatternEditor::isUsingAdditionalPatterns( const std::shared_ptr<H2Core::Pattern> pPattern ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ||
		 ( pPattern != nullptr && pPattern->isVirtual() ) ||
		 ( pHydrogen->getMode() == Song::Mode::Song &&
		   pHydrogen->isPatternEditorLocked() ) ) {
		return true;
	}
	
	return false;
}

void PatternEditor::updateWidth() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	
	if ( pPattern != nullptr ) {
		m_nActiveWidth = PatternEditor::nMargin + m_fGridWidth *
			pPattern->getLength();
		
		// In case there are other patterns playing which are longer
		// than the selected one, their notes will be placed using a
		// different color set between m_nActiveWidth and
		// m_nEditorWidth.
		if ( pHydrogen->getMode() == Song::Mode::Song &&
			 pPattern != nullptr && pPattern->isVirtual() &&
			 ! pHydrogen->isPatternEditorLocked() ) {
			m_nEditorWidth = 
				std::max( PatternEditor::nMargin + m_fGridWidth *
						  pPattern->longestVirtualPatternLength() + 1,
						  static_cast<float>(m_nActiveWidth) );
		}
		else if ( isUsingAdditionalPatterns( pPattern ) ) {
			m_nEditorWidth =
				std::max( PatternEditor::nMargin + m_fGridWidth *
						  pHydrogen->getAudioEngine()->getPlayingPatterns()->longest_pattern_length( false ) + 1,
						  static_cast<float>(m_nActiveWidth) );
		}
		else {
			m_nEditorWidth = m_nActiveWidth;
		}
	}
	else {
		m_nEditorWidth = PatternEditor::nMargin + MAX_NOTES * m_fGridWidth;
		m_nActiveWidth = m_nEditorWidth;
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

void PatternEditor::mouseDragStartEvent( QMouseEvent *ev ) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	int nColumn, nRow, nRealColumn;
	mouseEventToColumnRow( ev, &nColumn, &nRow, &nRealColumn );


	// Move cursor.
	m_pPatternEditorPanel->setCursorColumn( nColumn );

	// Hide cursor.
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	pHydrogenApp->setHideKeyboardCursor( true );

	// Cursor either just got hidden or was moved.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getInstrumentList()->repaintInstrumentLines();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}

	if ( ev->button() == Qt::RightButton ) {
		// Adjusting note properties.

		// Assemble all notes to be edited.
		DrumPatternRow row;
		if ( m_editor == Editor::DrumPattern ) {
			row = m_pPatternEditorPanel->getRowDB( nRow );
		}
		else {
			row = m_pPatternEditorPanel->getRowDB(
				m_pPatternEditorPanel->getSelectedRowDB() );
		}
		if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
			DEBUGLOG( QString( "Empty row clicked. y: %1, m_nGridHeight: %2, nRow: %3" )
					  .arg( ev->y() ).arg( m_nGridHeight ).arg( nRow ) );
			return;
		}

		// Note clicked by the user.
		Note* pDraggedNote = nullptr;
		if ( m_editor == Editor::DrumPattern ) {
			pDraggedNote = pPattern->findNote(
				nColumn, nRealColumn, row.nInstrumentID, row.sType, false );
		}
		else if ( m_editor == Editor::PianoRoll ) {
			pDraggedNote = pPattern->findNote(
				nColumn, nRealColumn, row.nInstrumentID, row.sType,
				Note::pitchToKey( Note::lineToPitch( nRow ) ),
				Note::pitchToOctave( Note::lineToPitch( nRow ) ), false );
		}
		else {
			ERRORLOG( "general click-dragging not implemented for NotePropertiesRuler" );
			return;
		}

		if ( pDraggedNote == nullptr || pDraggedNote->get_note_off() ) {
			return;
		}

		clearDraggedNotes();

		if ( ! m_selection.isEmpty() ) {
			validateSelection();
		}
		if ( m_selection.isSelected( pDraggedNote ) ) {
			// The clicked note is part of the current selection. All selected
			// notes will be edited.
			for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
				if ( ppNote != nullptr && m_selection.isSelected( ppNote ) &&
					 ! ppNote->get_note_off() ) {
					m_draggedNotes[ ppNote ] = new Note( ppNote );
				}
			}
		}
		else {
			m_draggedNotes[ pDraggedNote ] = new Note( pDraggedNote );
		}
		m_nDragStartColumn = pDraggedNote->get_position();
		m_nDragY = ev->y();
	}
}

void PatternEditor::mouseDragUpdateEvent( QMouseEvent *ev) {

	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr || m_draggedNotes.size() == 0 ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	const int nTickColumn = getColumn( ev->x() );
	m_mode = m_pPatternEditorPanel->getNotePropertiesMode();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	int nLen = nTickColumn - m_nDragStartColumn;

	if ( nLen <= 0 ) {
		nLen = -1;
	}

	for ( auto& [ ppNote, _ ] : m_draggedNotes ) {
		float fNotePitch = ppNote->get_pitch_from_key_octave();
		float fStep = 0;
		if ( nLen > -1 ){
			fStep = Note::pitchToFrequency( ( double )fNotePitch );
		} else {
			fStep=  1.0;
		}
		ppNote->set_length( nLen * fStep );


		// edit note property. We do not support the note key property.
		if ( m_mode != Mode::KeyOctave ) {
			float fValue = 0.0;
			if ( m_mode == Mode::Velocity ) {
				fValue = ppNote->get_velocity();
			}
			else if ( m_mode == Mode::Pan ) {
				fValue = ppNote->getPanWithRangeFrom0To1();
			}
			else if ( m_mode == Mode::LeadLag ) {
				fValue = ( ppNote->get_lead_lag() - 1.0 ) / -2.0 ;
			}
			else if ( m_mode == Mode::Probability ) {
				fValue = ppNote->get_probability();
			}
		
			float fMoveY = m_nDragY - ev->y();
			fValue = fValue  + (fMoveY / 100);
			if ( fValue > 1 ) {
				fValue = 1;
			}
			else if ( fValue < 0.0 ) {
				fValue = 0.0;
			}

			if ( m_mode == Mode::Velocity ) {
				ppNote->set_velocity( fValue );
			}
			else if ( m_mode == Mode::Pan ) {
				ppNote->setPanWithRangeFrom0To1( fValue );
			}
			else if ( m_mode == Mode::LeadLag ) {
				ppNote->set_lead_lag( ( fValue * -2.0 ) + 1.0 );
			}
			else if ( m_mode == Mode::Probability ) {
				ppNote->set_probability( fValue );
			}

			PatternEditor::triggerStatusMessage( ppNote, m_mode );
		}
	}
	m_nDragY = ev->y();

	pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
	pHydrogen->setIsModified( true );

	m_pPatternEditorPanel->updateEditors( true );
}

void PatternEditor::mouseDragEndEvent( QMouseEvent* ev ) {

	UNUSED( ev );
	unsetCursor();

	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	if ( m_draggedNotes.size() == 0 ) {
		return;
	}

	auto pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;
	bool bMacroStarted = false;
	if ( m_draggedNotes.size() > 1 ) {
		pUndoStack->beginMacro( tr( "edit note properties by dragging" ) );
		bMacroStarted = true;
	}
	else {
		// Just a single note was edited.
		for ( const auto& [ ppUpdatedNote, ppOriginalNote ] : m_draggedNotes ) {
			if ( ( ppUpdatedNote->get_velocity() !=
				   ppOriginalNote->get_velocity() ||
				   ppUpdatedNote->getPan() !=
				   ppOriginalNote->getPan() ||
				   ppUpdatedNote->get_lead_lag() !=
				   ppOriginalNote->get_lead_lag() ||
				   ppUpdatedNote->get_probability() !=
				   ppOriginalNote->get_probability() ) &&
				 ppUpdatedNote->get_length() != ppOriginalNote->get_length() ) {
				// Both length and another property have been edited.
				pUndoStack->beginMacro( tr( "edit note properties by dragging" ) );
				bMacroStarted = true;
			}
		}
	}

	auto editNoteProperty = [=]( PatternEditor::Mode mode, Note* pNewNote,
								  Note* pOldNote ) {
		pUndoStack->push( new SE_editNotePropertiesAction(
							  mode,
							  m_pPatternEditorPanel->getPatternNumber(),
							  pNewNote->get_position(),
							  m_pPatternEditorPanel->findRowDB( pNewNote ),
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
							  pNewNote->get_key(),
							  pOldNote->get_key(),
							  pNewNote->get_octave(),
							  pOldNote->get_octave() ) );
	};

	for ( const auto& [ ppUpdatedNote, ppOriginalNote ] : m_draggedNotes ) {
		if ( ppUpdatedNote->get_length() != ppOriginalNote->get_length() ) {
			editNoteProperty( Mode::Length, ppUpdatedNote, ppOriginalNote );
		}

		if ( ppUpdatedNote->get_velocity() != ppOriginalNote->get_velocity() ||
			 ppUpdatedNote->getPan() != ppOriginalNote->getPan() ||
			 ppUpdatedNote->get_lead_lag() != ppOriginalNote->get_lead_lag() ||
			 ppUpdatedNote->get_probability() !=
			 ppOriginalNote->get_probability() ) {
			editNoteProperty( m_mode, ppUpdatedNote, ppOriginalNote );
		}
	}

	if ( bMacroStarted ) {
		pUndoStack->endMacro();
	}

	clearDraggedNotes();
}

void PatternEditor::editNotePropertiesAction( const Mode& mode,
											  int nPatternNumber,
											  int nColumn,
											  int nRowDB,
											  float fVelocity,
											  float fPan,
											  float fLeadLag,
											  float fProbability,
											  int nLength,
											  int nNewKey,
											  int nOldKey,
											  int nNewOctave,
											  int nOldOctave )
{
	auto pPatternEditorPanel =
		HydrogenApp::get_instance()->getPatternEditorPanel();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPatternList = pSong->getPatternList();
	std::shared_ptr<H2Core::Pattern> pPattern;

	if ( nPatternNumber != -1 &&
		 nPatternNumber < pPatternList->size() ) {
		pPattern = pPatternList->get( nPatternNumber );
	}
	if ( pPattern == nullptr ) {
		return;
	}

	const DrumPatternRow row = pPatternEditorPanel->getRowDB( nRowDB );

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	// Find the note to edit
	auto pNote = pPattern->findNote(
		nColumn, nColumn, row.nInstrumentID, row.sType,
		static_cast<Note::Key>(nOldKey),
		static_cast<Note::Octave>(nOldOctave), false );

	bool bValueChanged = false;

	if ( pNote != nullptr ){
		switch ( mode ) {
		case Mode::Velocity:
			if ( pNote->get_velocity() != fVelocity ) {
				pNote->set_velocity( fVelocity );
				bValueChanged = true;
			}
			break;
		case Mode::Pan:
			if ( pNote->getPan() != fPan ) {
				pNote->setPan( fPan );
				bValueChanged = true;
			}
			break;
		case Mode::LeadLag:
			if ( pNote->get_lead_lag() != fLeadLag ) {
				pNote->set_lead_lag( fLeadLag );
				bValueChanged = true;
			}
			break;
		case Mode::KeyOctave:
			if ( pNote->get_key() != nNewKey ||
				 pNote->get_octave() != nNewOctave ) {
				pNote->set_key_octave( static_cast<Note::Key>(nNewKey),
									   static_cast<Note::Octave>(nNewOctave) );
				bValueChanged = true;
			}
			break;
		case Mode::Probability:
			if ( pNote->get_probability() != fProbability ) {
				pNote->set_probability( fProbability );
				bValueChanged = true;
			}
			break;
		case Mode::Length:
			if ( pNote->get_length() != nLength ) {
				pNote->set_length( nLength );
				bValueChanged = true;
			}
			break;
		case Mode::None:
		default:
			ERRORLOG("No mode set. No note property adjusted.");
		}			
	} else {
		ERRORLOG("note could not be found");
	}

	pHydrogen->getAudioEngine()->unlock();

	if ( bValueChanged ) {
		pHydrogen->setIsModified( true );
		PatternEditor::triggerStatusMessage( pNote, mode );
		pPatternEditorPanel->updateEditors( true );
	}
}

void PatternEditor::addOrRemoveNote( int nColumn, int nRealColumn, int nRow,
									 int nKey, int nOctave,
									 bool bDoAdd, bool bDoDelete,
									 bool bIsNoteOff ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	auto row = m_pPatternEditorPanel->getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row [%1]" ).arg( nRow ) );
		return;
	}

	Note* pOldNote = nullptr;
	if ( m_editor == Editor::PianoRoll ) {
		pOldNote = pPattern->findNote(
			nColumn, nRealColumn, row.nInstrumentID, row.sType,
			static_cast<Note::Key>(nKey), static_cast<Note::Octave>(nOctave) );
	}
	else {
		// When deleting a note under cursor NotePropertiesRuler works the same
		// as DrumPatternEditor.
		pOldNote = pPattern->findNote(
			nColumn, nRealColumn, row.nInstrumentID, row.sType );
	}
	if ( pOldNote != nullptr && !bDoDelete ) {
		// Found an old note, but we don't want to delete, so just return.
		return;
	} else if ( pOldNote == nullptr && !bDoAdd ) {
		// No note there, but we don't want to add a new one, so return.
		return;
	}

	int nOldLength, nOldKey, nOldOctave;
	float fOldVelocity, fOldPan, fOldLeadLag, fProbability;
	bool bNoteOff;

	if ( pOldNote != nullptr ) {
		nOldLength = pOldNote->get_length();
		fOldVelocity = pOldNote->get_velocity();
		fOldPan = pOldNote->getPan();
		fOldLeadLag = pOldNote->get_lead_lag();
		nOldKey = pOldNote->get_key();
		nOldOctave = pOldNote->get_octave();
		fProbability = pOldNote->get_probability();
		bNoteOff = pOldNote->get_note_off();
	}
	else {
		nOldLength = LENGTH_ENTIRE_SAMPLE;
		fOldVelocity = VELOCITY_DEFAULT;
		fOldPan = PAN_DEFAULT;
		fOldLeadLag = LEAD_LAG_DEFAULT;
		nOldKey = KEY_MIN;
		nOldOctave = OCTAVE_DEFAULT;
		fProbability = PROBABILITY_DEFAULT;
		bNoteOff = bIsNoteOff;
	}

	if ( m_editor == Editor::PianoRoll ) {
		nOldKey = nKey;
		nOldOctave = nOctave;
	}

	// Playback notes added notes.
	if ( pOldNote == nullptr && Preferences::get_instance()->getHearNewNotes() &&
		 row.nInstrumentID != EMPTY_INSTR_ID ) {
		auto pSelectedInstrument = m_pPatternEditorPanel->getSelectedInstrument();
		if ( pSelectedInstrument != nullptr &&
			 pSelectedInstrument->hasSamples() ) {
			auto pNote2 = new Note( pSelectedInstrument );
			pNote2->set_key_octave( static_cast<Note::Key>(nKey),
									static_cast<Note::Octave>(nOctave) );
			Hydrogen::get_instance()->getAudioEngine()->getSampler()->
				noteOn( pNote2 );
		}
	}

	HydrogenApp::get_instance()->m_pUndoStack->push(
		new SE_addOrRemoveNoteAction(
			nColumn,
			nRow,
			m_pPatternEditorPanel->getPatternNumber(),
			nOldLength,
			fOldVelocity,
			fOldPan,
			fOldLeadLag,
			nOldKey,
			nOldOctave,
			fProbability,
			/* bIsDelete */ pOldNote != nullptr,
			/* bIsMidi */ false,
			bNoteOff ) );
}

void PatternEditor::addOrRemoveNoteAction( int nColumn,
										   int nRow,
										   int nPatternNumber,
										   int nOldLength,
										   float nOldVelocity,
										   float fOldPan,
										   float fOldLeadLag,
										   int nOldKey,
										   int nOldOctave,
										   float fOldProbability,
										   bool bIsDelete,
										   bool bIsMidi,
										   bool bIsNoteOff )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	PatternList *pPatternList = pSong->getPatternList();
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

	auto pPatternEditorPanel =
		HydrogenApp::get_instance()->getPatternEditorPanel();
	const auto row = pPatternEditorPanel->getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row [%1]" ).arg( nRow ) );
		return;
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );	// lock the audio engine

	if ( bIsDelete ) {
		// Find and delete an existing (matching) note.
		auto pNote = pPattern->findNote(
			nColumn, -1, row.nInstrumentID, row.sType,
			static_cast<Note::Key>(nOldKey),
			static_cast<Note::Octave>(nOldOctave) );
		if ( pNote != nullptr ) {
			pPattern->removeNote( pNote );
			delete pNote;
		}
		else {
			ERRORLOG( "Did not find note to delete" );
		}

	}
	else {
		// create the new note
		unsigned nPosition = nColumn;
		float fVelocity = nOldVelocity;
		float fPan = fOldPan ;
		int nLength = nOldLength;

		if ( bIsNoteOff ) {
			fVelocity = VELOCITY_MIN;
			fPan = PAN_DEFAULT;
			nLength = 1;
			fOldProbability = PROBABILITY_DEFAULT;
		}

		std::shared_ptr<Instrument> pInstrument = nullptr;
		if ( row.nInstrumentID != EMPTY_INSTR_ID ) {
			pInstrument =
				pSong->getDrumkit()->getInstruments()->find( row.nInstrumentID );
			if ( pInstrument == nullptr ) {
				ERRORLOG( QString( "Instrument [%1] could not be found" )
						  .arg( row.nInstrumentID ) );
				pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
				return;
			}
		}

		auto pNote = new Note( pInstrument, nPosition, fVelocity, fPan, nLength );
		pNote->set_instrument_id( row.nInstrumentID );
		pNote->setType( row.sType );
		pNote->set_note_off( bIsNoteOff );
		if ( ! bIsNoteOff ) {
			pNote->set_lead_lag( fOldLeadLag );
		}
		pNote->set_probability( fOldProbability );
		pNote->set_key_octave( static_cast<Note::Key>(nOldKey),
							   static_cast<Note::Octave>(nOldOctave) );
		pPattern->insertNote( pNote );

		auto pCurrentEditor = pPatternEditorPanel->getVisibleEditor();
		if ( pCurrentEditor->getSelectNewNotes() ) {
			pCurrentEditor->m_selection.addToSelection( pNote );
		}

		if ( bIsMidi ) {
			pNote->set_just_recorded(true);
		}
	}
	pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
	pHydrogen->setIsModified( true );

	pPatternEditorPanel->updateEditors( true );
}

QString PatternEditor::modeToQString( const Mode& mode ) {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	QString s;
	
	switch ( mode ) {
	case PatternEditor::Mode::Velocity:
		s = pCommonStrings->getNotePropertyVelocity();
		break;
	case PatternEditor::Mode::Pan:
		s = pCommonStrings->getNotePropertyPan();
		break;
	case PatternEditor::Mode::LeadLag:
		s = pCommonStrings->getNotePropertyLeadLag();
		break;
	case PatternEditor::Mode::KeyOctave:
		s = pCommonStrings->getNotePropertyKeyOctave();
		break;
	case PatternEditor::Mode::Probability:
		s = pCommonStrings->getNotePropertyProbability();
		break;
	case PatternEditor::Mode::Length:
		s = pCommonStrings->getNotePropertyLength();
		break;
	default:
		s = QString( "Unknown mode [%1]" ).arg( static_cast<int>(mode) ) ;
		break;
	}

	return s;
}

void PatternEditor::triggerStatusMessage( Note* pNote, const Mode& mode ) {
	QString s;
	QString sCaller( _class_name() );
	QString sUnit( tr( "ticks" ) );
	float fValue;
	
	switch ( mode ) {
	case PatternEditor::Mode::Velocity:
		if ( ! pNote->get_note_off() ) {
			s = QString( tr( "Set note velocity" ) )
				.append( QString( ": [%1]")
						 .arg( pNote->get_velocity(), 2, 'f', 2 ) );
			sCaller.append( ":Velocity" );
		}
		break;
		
	case PatternEditor::Mode::Pan:
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
			sCaller.append( ":Pan" );
		}
		break;
		
	case PatternEditor::Mode::LeadLag:
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
		sCaller.append( ":LeadLag" );
		break;

	case PatternEditor::Mode::KeyOctave:
		s = QString( tr( "Set pitch" ) ).append( ": " ).append( tr( "key" ) )
			.append( QString( " [%1], " ).arg( Note::KeyToQString( pNote->get_key() ) ) )
			.append( tr( "octave" ) )
			.append( QString( ": [%1]" ).arg( pNote->get_octave() ) );
		sCaller.append( ":KeyOctave" );
		break;

	case PatternEditor::Mode::Probability:
		if ( ! pNote->get_note_off() ) {
			s = tr( "Set note probability to" )
				.append( QString( ": [%1]" ).arg( pNote->get_probability(), 2, 'f', 2 ) );
		}
		sCaller.append( ":Probability" );
		break;

	case PatternEditor::Mode::Length:
		if ( ! pNote->get_note_off() ) {
			s = tr( "Change note length" )
				.append( QString( ": [%1]" ).arg( pNote->get_probability(), 2, 'f', 2 ) );
		}
		sCaller.append( ":Length" );
		break;

	default:
		ERRORLOG( PatternEditor::modeToQString( mode ) );
	}

	if ( ! s.isEmpty() ) {
		HydrogenApp::get_instance()->showStatusBarMessage( s, sCaller );
	}
}

QPoint PatternEditor::getCursorPosition()
{
	const int nX = PatternEditor::nMargin +
		m_pPatternEditorPanel->getCursorColumn() * m_fGridWidth;
	int nY;
	if ( m_editor == Editor::PianoRoll ) {
		nY = m_nGridHeight * Note::pitchToLine( m_nCursorRow ) + 1;
	}
	else {
		nY = m_nGridHeight * m_pPatternEditorPanel->getSelectedRowDB();
	}

	return QPoint( nX, nY );
}

QRect PatternEditor::getKeyboardCursorRect()
{
	QPoint pos = getCursorPosition();

	float fHalfWidth;
	if ( m_nResolution != MAX_NOTES ) {
		// Corresponds to the distance between grid lines on 1/64 resolution.
		fHalfWidth = m_fGridWidth * 3;
	} else {
		// Corresponds to the distance between grid lines set to resolution
		// "off".
		fHalfWidth = m_fGridWidth;
	}
	if ( m_editor == Editor::DrumPattern ) {
		return QRect( pos.x() - fHalfWidth, pos.y() + 2,
					  fHalfWidth * 2, m_nGridHeight - 3 );
	}
	else if ( m_editor == Editor::PianoRoll ){
		return QRect( pos.x() - fHalfWidth, pos.y()-2,
					  fHalfWidth * 2, m_nGridHeight+3 );
	}
	else {
		return QRect( pos.x() - fHalfWidth, 3, fHalfWidth * 2, height() - 6 );
	}
}

void PatternEditor::clearDraggedNotes() {
	for ( auto& [ _, ppCopiedNote ] : m_draggedNotes ) {
		delete ppCopiedNote;
	}
	m_draggedNotes.clear();
}
