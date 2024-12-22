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
#include "PatternEditorSidebar.h"
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

	m_fGridWidth = pPref->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nActiveWidth = m_nEditorWidth;

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking( true );

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


void PatternEditor::drawNote( QPainter &p, H2Core::Note *pNote,
							  NoteStyle noteStyle ) const
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr || pNote == nullptr ) {
		return;
	}

	// Determine the center of the note symbol.
	int nY;
	if ( m_editor == Editor::DrumPattern ) {
		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
		nY = ( nRow * m_nGridHeight) + (m_nGridHeight / 2) - 3;

	}
	else {
		const auto selectedRow = m_pPatternEditorPanel->getRowDB(
			m_pPatternEditorPanel->getSelectedRowDB() );
		if ( pNote->get_instrument_id() != selectedRow.nInstrumentID ||
			 pNote->getType() != selectedRow.sType ) {
			ERRORLOG( QString( "Provided note [%1] is not part of selected row [%2]" )
					  .arg( pNote->toQString() ).arg( selectedRow.toQString() ) );
			return;
		}

		nY = m_nGridHeight *
			Note::pitchToLine( pNote->get_pitch_from_key_octave() ) + 1;
	}
	const int nX = PatternEditor::nMargin + pNote->get_position() * m_fGridWidth;

	const auto pPref = H2Core::Preferences::get_instance();
	
	const QColor noteColor( pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor );
	const QColor noteInactiveColor( pPref->getTheme().m_color.m_windowTextColor.darker( 150 ) );
	const QColor noteoffColor( pPref->getTheme().m_color.m_patternEditor_noteOffColor );
	const QColor noteoffInactiveColor( pPref->getTheme().m_color.m_windowTextColor );

	p.setRenderHint( QPainter::Antialiasing );

	QColor color = computeNoteColor( pNote->get_velocity() );

	uint w = 8, h =  8;

	if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
		QPen highlightedPen( highlightedNoteColor( noteStyle ) );
		highlightedPen.setWidth( 2 );
		p.setPen( highlightedPen );
		p.setBrush( Qt::NoBrush );
	}

	bool bMoving = noteStyle & NoteStyle::Selected && m_selection.isMoving();
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
		if ( noteStyle & NoteStyle::Background ) {

			if ( nX >= m_nActiveWidth ) {
				notePen.setColor( noteInactiveColor );
			}
			
			noteBrush.setStyle( Qt::Dense4Pattern );
			notePen.setStyle( Qt::DotLine );
		}

		if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
			p.drawEllipse( nX - 4 - 2, nY - 2, w + 4, h + 4 );
		}

		// Draw tail
		if ( pNote->get_length() != LENGTH_ENTIRE_SAMPLE ) {
			float fNotePitch = pNote->get_pitch_from_key_octave();
			float fStep = Note::pitchToFrequency( ( double )fNotePitch );

			// if there is a stop-note to the right of this note, only draw-
			// its length till there.
			int nLength = pNote->get_length();
			const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
			for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
				if ( ppNote != nullptr &&
					 // noteOff note
					 ppNote->get_note_off() &&
					 // located in the same row
					 m_pPatternEditorPanel->findRowDB( ppNote ) == nRow ) {
					const int nNotePos = ppNote->get_position() +
						ppNote->get_lead_lag() *
						AudioEngine::getLeadLagInTicks();

					if ( // left of the NoteOff
						pNote->get_position() < nNotePos &&
						// custom length reaches beyond NoteOff
						pNote->get_position() + pNote->get_length() >
						nNotePos ) {

						// In case there are multiple stop-notes present, take the
						// shortest distance.
						nLength = std::min( nNotePos - pNote->get_position(), nLength );
					}
				}
			}

			width = m_fGridWidth * nLength / fStep;
			width = width - 1;	// lascio un piccolo spazio tra una nota ed un altra

			if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
				p.drawRoundedRect( nX-2, nY, width+4, 3+4, 4, 4 );
			}
			p.setPen( notePen );
			p.setBrush( noteBrush );

			// Since the note body is transparent for an inactive note, we try
			// to start the tail at its boundary. For regular notes we do not
			// care about an overlap, as it ensures that there are no white
			// artifacts between tail and note body regardless of the scale
			// factor.
			int nRectOnsetX = nX;
			int nRectWidth = width;
			if ( noteStyle & NoteStyle::Background ) {
				nRectOnsetX = nRectOnsetX + w/2;
				nRectWidth = nRectWidth - w/2;
			}

			p.drawRect( nRectOnsetX, nY +2, nRectWidth, 3 );
			p.drawLine( nX+width, nY, nX+width, nY + h );
		}

		p.setPen( notePen );
		p.setBrush( noteBrush );
		p.drawEllipse( nX -4 , nY, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + nX -4 -2, movingOffset.y() + nY -2 , w + 4, h + 4 );
			// Moving tail
			if ( pNote->get_length() != LENGTH_ENTIRE_SAMPLE ) {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawRoundedRect( movingOffset.x() + nX-2, movingOffset.y() + nY, width+4, 3+4, 4, 4 );
			}
		}
	}
	else if ( pNote->get_note_off() ) {

		QBrush noteOffBrush( noteoffColor );
		if ( noteStyle & NoteStyle::Background ) {
			noteOffBrush.setStyle( Qt::Dense4Pattern );

			if ( nX >= m_nActiveWidth ) {
				noteOffBrush.setColor( noteoffInactiveColor );
			}
		}

		if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
			p.drawEllipse( nX -4 -2, nY-2, w+4, h+4 );
		}

		p.setPen( Qt::NoPen );
		p.setBrush( noteOffBrush );
		p.drawEllipse( nX -4 , nY, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + nX -4 -2, movingOffset.y() + nY -2, w + 4, h + 4 );
		}
	}
}

void PatternEditor::eventPointToColumnRow( const QPoint& point, int* pColumn,
										   int* pRow, int* pRealColumn,
										   bool bUseFineGrained ) const {
	if ( pRow != nullptr ) {
		*pRow = static_cast<int>(
			std::floor( static_cast<float>(point.y()) /
						static_cast<float>(m_nGridHeight) ) );
	}

	if ( pColumn != nullptr ) {
		int nGranularity = 1;
		if ( !( bUseFineGrained && m_bFineGrained ) ) {
			nGranularity = granularity();
		}
		const int nWidth = m_fGridWidth * nGranularity;
		int nColumn = ( point.x() - PatternEditor::nMargin + (nWidth / 2) ) /
			nWidth;
		*pColumn = std::max( 0, nColumn * nGranularity );
	}

	if ( pRealColumn != nullptr ) {
		if ( point.x() > PatternEditor::nMargin ) {
			*pRealColumn = static_cast<int>(
				std::floor( ( point.x() -
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
	if ( m_editor == Editor::DrumPattern || m_editor == Editor::PianoRoll ) {
		// Enable or disable menu actions that only operate on selections.
		const bool bEmpty = m_selection.isEmpty();
		for ( auto & action : m_selectionActions ) {
			action->setEnabled( !bEmpty );
		}
	}

	m_pPopupMenu->popup( pos );
}

///
/// Copy selection to clipboard in XML
///
void PatternEditor::copy()
{
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
		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
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
	handleKeyboardCursor( true );
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
	bool bAppendedToDB = false;

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

			const int nPos = pNote->get_position() + nDeltaPos;
			if ( nPos < 0 || nPos >= pPattern->getLength() ) {
				delete pNote;
				continue;
			}

			int nInstrumentId;
			QString sType;
			if ( m_editor == Editor::DrumPattern ) {
				const auto nNoteRow =
					m_pPatternEditorPanel->findRowDB( pNote, true );
				if ( nNoteRow != -1 ) {
					// Note belongs to a row already present in the DB.
					const int nRow = nNoteRow + nDeltaRow;
					if ( nRow < 0 ||
						 nRow >= m_pPatternEditorPanel->getRowNumberDB() ) {
						delete pNote;
						continue;
					}
					const auto row = m_pPatternEditorPanel->getRowDB( nRow );
					nInstrumentId = row.nInstrumentID;
					sType = row.sType;
				}
				else {
					// Note can not be represented in the current DB. This means
					// it might be a type-only one copied from a a different
					// pattern. We will append it to the DB.
					nInstrumentId = pNote->get_instrument_id();
					sType = pNote->getType();
					bAppendedToDB = true;
				}
			}
			else {
				const auto row = m_pPatternEditorPanel->getRowDB( nSelectedRow );
				nInstrumentId = row.nInstrumentID;
				sType = row.sType;
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
							 nInstrumentId,
							 sType,
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

	if ( bAppendedToDB ) {
		// We added a note to the pattern currently not represented by the DB.
		// We have to force its update in order to avoid inconsistencies.
		const int nOldSize = m_pPatternEditorPanel->getRowNumberDB();
		m_pPatternEditorPanel->updateDB();
		m_pPatternEditorPanel->updateEditors();
		m_pPatternEditorPanel->resizeEvent( nullptr );

		// Select the append line
		m_pPatternEditorPanel->setSelectedRowDB( nOldSize );
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

	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

	// Move the notes
	pUndo->beginMacro( tr( "Align notes to grid" ) );

	for ( Note *pNote : m_selection ) {
		if ( pNote == nullptr ) {
			continue;
		}

		const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
		const auto row = m_pPatternEditorPanel->getRowDB( nRow );
		const int nPosition = pNote->get_position();
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
		const int nInstrumentId = pNote->get_instrument_id();
		const QString sType = pNote->getType();
		const int nLength = pNote->get_length();
		const float fVelocity = pNote->get_velocity();
		const float fPan = pNote->getPan();
		const float fLeadLag = pNote->get_lead_lag();
		const int nKey = pNote->get_key();
		const int nOctave = pNote->get_octave();
		const float fProbability = pNote->get_probability();
		const bool bNoteOff = pNote->get_note_off();

		// Move note -> delete at source position
		pUndo->push( new SE_addOrRemoveNoteAction(
						 nPosition,
						 nInstrumentId,
						 sType,
						 m_pPatternEditorPanel->getPatternNumber(),
						 nLength,
						 fVelocity,
						 fPan,
						 fLeadLag,
						 nKey,
						 nOctave,
						 fProbability,
						 /* bIsDelete */ true,
						 /* bIsMidi */ false,
						 bNoteOff ) );

		// Add at target position
		pUndo->push( new SE_addOrRemoveNoteAction(
						 nNewPosition,
						 nInstrumentId,
						 sType,
						 m_pPatternEditorPanel->getPatternNumber(),
						 nLength,
						 fVelocity,
						 fPan,
						 fLeadLag,
						 nKey,
						 nOctave,
						 fProbability,
						 /* bIsDelete */ false,
						 /* bIsMidi */ false,
						 bNoteOff ) );
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
		float fVal = ( rand() % 100 ) / 100.0;
		fVal = std::clamp( pNote->get_velocity() + ( ( fVal - 0.50 ) / 2 ),
						   0.0, 1.0 );
		pUndo->push( new SE_editNotePropertiesAction(
						 PatternEditor::Mode::Velocity,
						 m_pPatternEditorPanel->getPatternNumber(),
						 pNote->get_position(),
						 pNote->get_instrument_id(),
						 pNote->getType(),
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

void PatternEditor::mousePressEvent( QMouseEvent *ev ) {
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	if ( ev->x() > m_nActiveWidth ) {
		if ( ! m_selection.isEmpty() ) {
			m_selection.clearSelection();
			m_pPatternEditorPanel->getVisibleEditor()->updateEditor();
			m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor();
		}
		return;
	}

	updateModifiers( ev );

	if ( ev->buttons() == Qt::LeftButton || ev->buttons() == Qt::RightButton ) {
		m_notesToSelectOnMove.clear();

		// When interacting with note(s) not already in a selection, we will
		// discard the current selection and add these notes under point to a
		// transient one.
		const auto notesUnderPoint = getNotesAtPoint( pPattern, ev->pos(), true );
		for ( const auto& ppNote : notesUnderPoint ) {
			m_notesToSelectOnMove.push_back( ppNote );
		}
	}

	// propagate event to selection. This could very well cancel a lasso created
	// via keyboard events.
	m_selection.mousePressEvent( ev );

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	handleKeyboardCursor( false );
}

void PatternEditor::mouseClickEvent( QMouseEvent *ev )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	int nRow, nColumn, nRealColumn;
	eventPointToColumnRow( ev->pos(), &nColumn, &nRow, &nRealColumn,
						   /* fineGrained */true );

	// Select the corresponding row
	if ( m_editor == Editor::DrumPattern ) {
		const auto row = m_pPatternEditorPanel->getRowDB( nRow );
		if ( row.nInstrumentID != EMPTY_INSTR_ID || ! row.sType.isEmpty() ) {
			m_pPatternEditorPanel->setSelectedRowDB( nRow );
		}
	}
	else if ( m_editor == Editor::PianoRoll ) {
		// Update the row of the piano roll itself.
		setCursorRow( Note::lineToPitch( nRow ) );

		// Use the row of the DrumPatternEditor/DB for further note
		// interactions.
		nRow = m_pPatternEditorPanel->getSelectedRowDB();
	}

	bool bClickedOnGrid = false;

	// main button action
	if ( ev->button() == Qt::LeftButton &&
		 m_editor != Editor::NotePropertiesRuler ) {

		// Check whether an existing note or an empty grid cell was clicked.
		const auto notesAtPoint = getNotesAtPoint( pPattern, ev->pos(), false );
		if ( notesAtPoint.size() == 0 ) {
			// Empty grid cell
			bClickedOnGrid = true;

			// By pressing the Alt button the user can bypass quantization of
			// new note to the grid.
			const int nTargetColumn = ev->modifiers() & Qt::AltModifier ?
				nRealColumn : nColumn;

			// Pressing Shift causes the added note to be of NoteOff type.
			if ( m_editor == Editor::DrumPattern ) {
				addOrRemoveNote(
					nTargetColumn, nRealColumn, nRow, KEY_MIN, OCTAVE_DEFAULT,
					/* bDoAdd */true, /* bDoDelete */false,
					/* bIsNoteOff */ev->modifiers() & Qt::ShiftModifier );
			}
			else if ( m_editor == Editor::PianoRoll ) {
				const Note::Octave octave = Note::pitchToOctave( m_nCursorRow );
				const Note::Key noteKey = Note::pitchToKey( m_nCursorRow );
				addOrRemoveNote(
					nTargetColumn, nRealColumn, nRow, noteKey, octave,
					/* bDoAdd */true, /* bDoDelete */false,
					/* bIsNoteOff */ ev->modifiers() & Qt::ShiftModifier );
			}
		}
		else {
			// Note(s) clicked. Delete them.
			auto pUndo = HydrogenApp::get_instance()->m_pUndoStack;
			pUndo->beginMacro( HydrogenApp::get_instance()->getCommonStrings()
							   ->getActionDeleteNotes() );
			for ( const auto& ppNote : notesAtPoint ) {
				pUndo->push( new SE_addOrRemoveNoteAction(
								 ppNote->get_position(),
								 ppNote->get_instrument_id(),
								 ppNote->getType(),
								 m_pPatternEditorPanel->getPatternNumber(),
								 ppNote->get_length(),
								 ppNote->get_velocity(),
								 ppNote->getPan(),
								 ppNote->get_lead_lag(),
								 ppNote->get_key(),
								 ppNote->get_octave(),
								 ppNote->get_probability(),
								 /* bIsDelete */ true,
								 /* bIsMidi */ false,
								 /* bIsNoteOff */ ppNote->get_note_off() ) );
}
			pUndo->endMacro();
		}
		m_selection.clearSelection();
		updateHoveredNotesMouse( ev->pos() );

	}
	else if ( ev->button() == Qt::RightButton ) {
		showPopupMenu( ev->globalPos() );
	}

	// Update cursor position
	if ( bClickedOnGrid && m_editor != Editor::NotePropertiesRuler ) {
		m_pPatternEditorPanel->setCursorColumn( nColumn );
	}

	update();
}

void PatternEditor::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_pPatternEditorPanel->getPattern() == nullptr ) {
		return;
	}

	if ( m_notesToSelectOnMove.size() > 0 ) {
		if ( ev->buttons() == Qt::LeftButton ||
			 ev->buttons() == Qt::RightButton ) {
			m_selection.clearSelection();
			for ( const auto& ppNote : m_notesToSelectOnMove ) {
				m_selection.addToSelection( ppNote );
			}
		}
		else {
			m_notesToSelectOnMove.clear();
		}
	}

	// Check which note is hovered.
	updateHoveredNotesMouse( ev->pos() );

	if ( ev->buttons() != Qt::NoButton ) {
		updateModifiers( ev );
		m_selection.mouseMoveEvent( ev );
		if ( syncLasso() || m_selection.isMoving() ) {
			m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
			m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor();
		}
	}
}

void PatternEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	updateModifiers( ev );

	bool bUpdate = false;

	// In case we just cancelled a lasso, we have to tell the other editors.
	const bool oldState = m_selection.getSelectionState();
	m_selection.mouseReleaseEvent( ev );
	if ( oldState != m_selection.getSelectionState() ) {
		syncLasso();
		bUpdate = true;
	}

	if ( m_notesToSelectOnMove.size() > 0 ) {
		// We used a transient selection of note(s) at a single position.
		m_selection.clearSelection();
		m_notesToSelectOnMove.clear();
		bUpdate = true;
	}

	if ( bUpdate ) {
		m_pPatternEditorPanel->getVisibleEditor()->updateEditor();
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor();
	}
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

int PatternEditor::getCursorMargin() const {
	const int nResolution = m_pPatternEditorPanel->getResolution();
	if ( nResolution < 32 ) {
		return PatternEditor::nDefaultCursorMargin;
	}
	else if ( nResolution < MAX_NOTES ) {
		return PatternEditor::nDefaultCursorMargin / 2;
	}
	else {
		return 0;
	}
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

	const auto modifiers = m_selection.getModifiers();

	// Quantization in y direction is mandatory. A note can not be placed
	// between lines.
	int nQuantY = m_nGridHeight;
	int nBiasY = nQuantY / 2;
	if ( rawOffset.y() < 0 ) {
		nBiasY = -nBiasY;
	}
	const int nOffsetY = (rawOffset.y() + nBiasY) / nQuantY;

	int nOffsetX;
	if ( modifiers & Qt::AltModifier ) {
		// No quantization
		nOffsetX = static_cast<int>(
			std::floor( static_cast<float>(rawOffset.x()) / m_fGridWidth ) );
	}
	else {
		// Quantize offset to multiples of m_nGrid{Width,Height}
		int nQuantX = m_fGridWidth;
		float nFactor = 1;
		if ( ! m_bFineGrained ) {
			nFactor = granularity();
			nQuantX = m_fGridWidth * nFactor;
		}
		int nBiasX = nQuantX / 2;
		if ( rawOffset.x() < 0 ) {
			nBiasX = -nBiasX;
		}
		nOffsetX = nFactor * static_cast<int>((rawOffset.x() + nBiasX) / nQuantX);
	}


	return QPoint( nOffsetX, nOffsetY);
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

	if ( ! m_pPatternEditorPanel->isUsingTriplets() ) {

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
		const int nResolution = m_pPatternEditorPanel->getResolution();

		// For each successive set of finer-spaced lines, the even
		// lines will have already been drawn at the previous coarser
		// pitch, so only the odd numbered lines need to be drawn.
		int nColour = 1;
		for ( int nnRes : availableResolutions ) {
			if ( nnRes > nResolution ) {
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


QColor PatternEditor::highlightedNoteColor( NoteStyle noteStyle ) const {
	
	const auto pPref = H2Core::Preferences::get_instance();

	QColor color = pPref->getTheme().m_color.m_selectionHighlightColor;

	int nFactor = 100;
	if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
		nFactor = 120;
	}
	else if ( noteStyle & NoteStyle::Hovered ) {
		nFactor = 140;
	}

	if ( noteStyle & NoteStyle::Hovered ) {
		// Depending on the selection color, we make it either darker or
		// lighter.
		int nHue, nSaturation, nValue;
		color.getHsv( &nHue, &nSaturation, &nValue );
		if ( nValue >= 130 ) {
			color = color.darker( nFactor );
		} else {
			color = color.lighter( nFactor );
		}
	}

	return std::move( color );
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
									   pNote->get_instrument_id(),
									   pNote->getType(),
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
			pUndo->beginMacro( HydrogenApp::get_instance()->getCommonStrings()
							   ->getActionDeleteNotes() );
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
		const auto row = m_pPatternEditorPanel->getRowDB( nRow );
		const auto newRow = m_pPatternEditorPanel->getRowDB( nNewRow );

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

		// Cache note properties since a potential first note deletion will also
		// call the note's destructor.
		const int nLength = pNote->get_length();
		const float fVelocity = pNote->get_velocity();
		const float fPan = pNote->getPan();
		const float fLeadLag = pNote->get_lead_lag();
		const int nKey = pNote->get_key();
		const int nOctave = pNote->get_octave();
		const float fProbability = pNote->get_probability();
		const bool bNoteOff = pNote->get_note_off();

		if ( ! m_bCopyNotMove ) {
			// Note is moved either out of range or to a new position. Delete
			// the note at the source position.
			pUndo->push( new SE_addOrRemoveNoteAction(
							 nPosition,
							 row.nInstrumentID,
							 row.sType,
							 m_pPatternEditorPanel->getPatternNumber(),
							 nLength,
							 fVelocity,
							 fPan,
							 fLeadLag,
							 nKey,
							 nOctave,
							 fProbability,
							 /* bIsDelete */ true,
							 /* bIsMidi */ false,
							 bNoteOff ) );
		}

		if ( bNoteInRange ) {
			// Create a new note at the target position
			pUndo->push( new SE_addOrRemoveNoteAction(
							 nNewPosition,
							 newRow.nInstrumentID,
							 newRow.sType,
							 m_pPatternEditorPanel->getPatternNumber(),
							 nLength,
							 fVelocity,
							 fPan,
							 fLeadLag,
							 nNewKey,
							 nNewOctave,
							 fProbability,
							 /* bIsDelete */ false,
							 /* bIsMidi */ false,
							 bNoteOff ) );
		}
	}

	m_bSelectNewNotes = false;
	pUndo->endMacro();
}

void PatternEditor::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

int PatternEditor::granularity() const {
	int nBase;
	if ( m_pPatternEditorPanel->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	return 4 * MAX_NOTES / ( nBase * m_pPatternEditorPanel->getResolution() );
}

void PatternEditor::keyPressEvent( QKeyEvent *ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();

	const int nWordSize = 5;

	bool bUnhideCursor = ev->key() != Qt::Key_Delete;

	auto pCleanedEvent = new QKeyEvent(
		QEvent::KeyPress, ev->key(), Qt::NoModifier, ev->text() );

	// Check whether the event was already handled by a method of a child class.
	if ( ! ev->isAccepted() ) {
		updateModifiers( ev );

		if ( ev->matches( QKeySequence::MoveToNextChar ) ||
			 ev->matches( QKeySequence::SelectNextChar ) ||
			 ( ev->modifiers() & Qt::AltModifier && (
				 pCleanedEvent->matches( QKeySequence::MoveToNextChar ) ||
				 pCleanedEvent->matches( QKeySequence::SelectNextChar ) ) ) ) {
			// ->
			m_pPatternEditorPanel->moveCursorRight( ev );
		}
		else if ( ev->matches( QKeySequence::MoveToNextWord ) ||
				  ev->matches( QKeySequence::SelectNextWord ) ) {
			// -->
			m_pPatternEditorPanel->moveCursorRight( ev, nWordSize );
		}
		else if ( ev->matches( QKeySequence::MoveToEndOfLine ) ||
				  ev->matches( QKeySequence::SelectEndOfLine ) ) {
			// -->|
			m_pPatternEditorPanel->setCursorColumn( pPattern->getLength() );
		}
		else if ( ev->matches( QKeySequence::MoveToPreviousChar ) ||
				  ev->matches( QKeySequence::SelectPreviousChar ) ||
				  ( ev->modifiers() & Qt::AltModifier && (
					  pCleanedEvent->matches( QKeySequence::MoveToPreviousChar ) ||
					  pCleanedEvent->matches( QKeySequence::SelectPreviousChar ) ) ) ) {
			// <-
			m_pPatternEditorPanel->moveCursorLeft( ev );
		}
		else if ( ev->matches( QKeySequence::MoveToPreviousWord ) ||
				  ev->matches( QKeySequence::SelectPreviousWord ) ) {
			// <--
			m_pPatternEditorPanel->moveCursorLeft( ev, nWordSize );
		}
		else if ( ev->matches( QKeySequence::MoveToStartOfLine ) ||
				  ev->matches( QKeySequence::SelectStartOfLine ) ) {
			// |<--
			m_pPatternEditorPanel->setCursorColumn( 0 );
		}
		else if ( ev->matches( QKeySequence::SelectAll ) ) {
			// Key: Ctrl + A: Select all
			bUnhideCursor = false;
			selectAll();
		}
		else if ( ev->matches( QKeySequence::Deselect ) ) {
			// Key: Shift + Ctrl + A: clear selection
			bUnhideCursor = false;
			selectNone();
		}
		else if ( ev->matches( QKeySequence::Copy ) ) {
			bUnhideCursor = false;
			copy();
		}
		else if ( ev->matches( QKeySequence::Paste ) ) {
			bUnhideCursor = false;
			paste();
		}
		else if ( ev->matches( QKeySequence::Cut ) ) {
			bUnhideCursor = false;
			cut();
		}
		else {
			ev->ignore();
			return;
		}
	}

	// synchronize lassos
	bool bFullUpdate = false;
	auto pVisibleEditor = m_pPatternEditorPanel->getVisibleEditor();
	// In case we use keyboard events to _continue_ an existing lasso in
	// NotePropertiesRuler started in DrumPatternEditor (followed by moving
	// focus to NPR using tab key), DrumPatternEditor has to be used to update
	// the shared set of selected notes. Else, only notes of the current row
	// will be added after an update.
	if ( m_editor == Editor::NotePropertiesRuler &&
		 pVisibleEditor->m_selection.isLasso() && m_selection.isLasso() &&
		 dynamic_cast<DrumPatternEditor*>(pVisibleEditor) != nullptr ) {
		pVisibleEditor->m_selection.updateKeyboardCursorPosition();
		bFullUpdate = pVisibleEditor->syncLasso();
	}
	else {
		m_selection.updateKeyboardCursorPosition();
		bFullUpdate = syncLasso();
	}
	updateHoveredNotesKeyboard();

	if ( bUnhideCursor ) {
		handleKeyboardCursor( bUnhideCursor );
	}

	if ( bFullUpdate ) {
		// Notes have might become selected. We have to update the background as
		// well.
		m_pPatternEditorPanel->getVisibleEditor()->updateEditor();
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor();
	}
	else {
		m_pPatternEditorPanel->getVisibleEditor()->update();
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->update();
	}

	if ( ! ev->isAccepted() ) {
		ev->accept();
	}
}

void PatternEditor::handleKeyboardCursor( bool bVisible ) {
	auto pHydrogenApp = HydrogenApp::get_instance();
	const bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();

	pHydrogenApp->setHideKeyboardCursor( ! bVisible );

	// Only update on state changes
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		updateHoveredNotesKeyboard();
		if ( bVisible ) {
			m_selection.updateKeyboardCursorPosition();
			m_pPatternEditorPanel->ensureCursorVisible();

			if ( m_selection.isLasso() ) {
				// Since event was used to alter the note selection, we
				// invalidate background and force a repainting of all note
				// symbols (including whether or not they are selected).
				invalidateBackground();
			}
		}

		m_pPatternEditorPanel->getSidebar()->updateEditor();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getVisibleEditor()->update();
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->update();
	}
}

void PatternEditor::keyReleaseEvent( QKeyEvent *ev ) {
	updateModifiers( ev );
}

void PatternEditor::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void PatternEditor::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;

	if ( m_pPatternEditorPanel->getHoveredNotes().size() > 0 ) {
		std::map<std::shared_ptr<Pattern>, std::vector<Note*>> empty;
		// Takes care of the update.
		m_pPatternEditorPanel->setHoveredNotesMouse( empty );
	}
	else {
		update();
	}
}

void PatternEditor::focusInEvent( QFocusEvent *ev ) {
	UNUSED( ev );
	if ( ev->reason() == Qt::TabFocusReason ||
		 ev->reason() == Qt::BacktabFocusReason ) {
		handleKeyboardCursor( true );
	}
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getSidebar()->update();
	}

	// Update to show the focus border highlight
	updateEditor();
}

void PatternEditor::focusOutEvent( QFocusEvent *ev ) {
	UNUSED( ev );
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getSidebar()->update();
	}
	
	// Update to remove the focus border highlight
	updateEditor();
}

void PatternEditor::invalidateBackground() {
	m_bBackgroundInvalid = true;
}

void PatternEditor::createBackground() {
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

	m_mode = m_pPatternEditorPanel->getNotePropertiesMode();

	if ( ev->button() == Qt::RightButton ) {
		// Adjusting note properties.

		const auto notesAtPoint = getNotesAtPoint( pPattern, ev->pos(), false );
		if ( notesAtPoint.size() == 0 ) {
			return;
		}

		clearDraggedNotes();
		// Either all or none of the notes at point should be selected. It is
		// safe to just check the first one.
		if ( m_selection.isSelected( notesAtPoint[ 0 ] ) ) {
			// The clicked note is part of the current selection. All selected
			// notes will be edited.
			for ( const auto& ppNote : m_selection ) {
				if ( ppNote != nullptr &&
					 ! ( ppNote->get_note_off() &&
						 ( m_mode != Mode::LeadLag &&
						   m_mode != Mode::Probability ) ) ) {
					m_draggedNotes[ ppNote ] = new Note( ppNote );
				}
			}
		}
		else {
			for ( const auto& ppNote : notesAtPoint ) {
				// NoteOff notes can have both a custom lead/lag and
				// probability. But all other properties won't take effect.
				if ( ! ( ppNote->get_note_off() &&
						( m_mode != Mode::LeadLag &&
						  m_mode != Mode::Probability ) ) ) {
					m_draggedNotes[ ppNote ] = new Note( ppNote );
				}
			}
		}
		// All notes at located at the same point.
		m_nDragStartColumn = notesAtPoint[ 0 ]->get_position();
		m_nDragY = ev->y();
	}
}

void PatternEditor::mouseDragUpdateEvent( QMouseEvent *ev) {
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr || m_draggedNotes.size() == 0 ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	int nColumn, nRealColumn;
	eventPointToColumnRow( ev->pos(), &nColumn, nullptr, &nRealColumn );

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	int nTargetColumn;
	if ( ev->modifiers() == Qt::ControlModifier ||
		 ev->modifiers() == Qt::AltModifier ) {
		// fine control
		nTargetColumn = nRealColumn;
	} else {
		nTargetColumn = nColumn;
	}

	int nLen = nTargetColumn - m_nDragStartColumn;

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
							  pNewNote->get_instrument_id(),
							  pNewNote->getType(),
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
											  int nInstrumentId,
											  const QString& sType,
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
	if ( pSong == nullptr ) {
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
	auto pNote = pPattern->findNote(
		nColumn, nColumn, nInstrumentId, sType, static_cast<Note::Key>(nOldKey),
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

	if ( nColumn >= pPattern->getLength() ) {
		// Note would be beyond the active region of the current pattern.
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
			row.nInstrumentID,
			row.sType,
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
										   int nInstrumentId,
										   const QString& sType,
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

	if ( nInstrumentId == EMPTY_INSTR_ID && sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row" ) );
		return;
	}

	auto pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );	// lock the audio engine

	if ( bIsDelete ) {
		// Find and delete an existing (matching) note.
		auto pNote = pPattern->findNote(
			nColumn, -1, nInstrumentId, sType, static_cast<Note::Key>(nOldKey),
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
		}

		std::shared_ptr<Instrument> pInstrument = nullptr;
		if ( nInstrumentId != EMPTY_INSTR_ID ) {
			pInstrument =
				pSong->getDrumkit()->getInstruments()->find( nInstrumentId );
			if ( pInstrument == nullptr ) {
				ERRORLOG( QString( "Instrument [%1] could not be found" )
						  .arg( nInstrumentId ) );
				pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
				return;
			}
		}

		auto pNote = new Note( pInstrument, nPosition, fVelocity, fPan, nLength );
		pNote->set_instrument_id( nInstrumentId );
		pNote->setType( sType );
		pNote->set_note_off( bIsNoteOff );
		pNote->set_lead_lag( fOldLeadLag );
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

QString PatternEditor::editorToQString( const Editor& editor ) {
	switch ( editor ) {
	case PatternEditor::Editor::DrumPattern:
		return "DrumPattern";
	case PatternEditor::Editor::PianoRoll:
		return "PianoRoll";
	case PatternEditor::Editor::NotePropertiesRuler:
		return "NotePropertiesRuler";
	case PatternEditor::Editor::None:
	default:
		return QString( "Unknown editor [%1]" ).arg( static_cast<int>(editor) ) ;
	}
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
		if ( ! pNote->get_note_off() ) {
			s = QString( tr( "Set pitch" ) ).append( ": " ).append( tr( "key" ) )
				.append( QString( " [%1], " ).arg( Note::KeyToQString( pNote->get_key() ) ) )
				.append( tr( "octave" ) )
				.append( QString( ": [%1]" ).arg( pNote->get_octave() ) );
			sCaller.append( ":KeyOctave" );
		}
		break;

	case PatternEditor::Mode::Probability:
		s = tr( "Set note probability to" )
			.append( QString( ": [%1]" ).arg( pNote->get_probability(), 2, 'f', 2 ) );
		sCaller.append( ":Probability" );
		break;

	case PatternEditor::Mode::Length:
		if ( ! pNote->get_note_off() ) {
			s = tr( "Change note length" )
				.append( QString( ": [%1]" ).arg( pNote->get_probability(), 2, 'f', 2 ) );
		sCaller.append( ":Length" );
		}
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

void PatternEditor::setCursorRow( int nCursorRow ) {
	const int nMinPitch = Note::octaveKeyToPitch(
		(Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN );
	const int nMaxPitch = Note::octaveKeyToPitch(
		(Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX );

	if ( nCursorRow < nMinPitch ) {
		nCursorRow = nMinPitch;
	}
	else if ( nCursorRow >= nMaxPitch ) {
		nCursorRow = nMaxPitch;
	}

	if ( nCursorRow == m_nCursorRow ) {
		return;
	}

	m_nCursorRow = nCursorRow;

	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		m_pPatternEditorPanel->ensureCursorVisible();
		m_pPatternEditorPanel->getSidebar()->updateEditor();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getVisibleEditor()->update();
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->update();
	}
}

QRect PatternEditor::getKeyboardCursorRect()
{
	QPoint pos = getCursorPosition();

	float fHalfWidth;
	if ( m_pPatternEditorPanel->getResolution() != MAX_NOTES ) {
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

std::vector<Note*> PatternEditor::getNotesAtPoint( std::shared_ptr<H2Core::Pattern> pPattern,
												   const QPoint& point,
												   bool bExcludeSelected ) {
	std::vector<Note*> notesUnderPoint;
	if ( pPattern == nullptr ) {
		return std::move( notesUnderPoint );
	}

	const int nCursorMargin = getCursorMargin();

	int nRow, nRealColumn;
	eventPointToColumnRow( point, nullptr, &nRow, &nRealColumn );

	int nRealColumnLower, nRealColumnUpper;
	eventPointToColumnRow( point - QPoint( nCursorMargin, 0 ),
						   nullptr, nullptr, &nRealColumnLower );
	eventPointToColumnRow( point + QPoint( nCursorMargin, 0 ),
						   nullptr, nullptr, &nRealColumnUpper );


	// Assemble all notes to be edited.
	DrumPatternRow row;
	if ( m_editor == Editor::DrumPattern ) {
		row = m_pPatternEditorPanel->getRowDB( nRow );
	}
	else {
		row = m_pPatternEditorPanel->getRowDB(
			m_pPatternEditorPanel->getSelectedRowDB() );
	}

	// Prior to version 2.0 notes where selected by clicking its grid cell,
	// while this caused only notes on the current grid to be accessible it also
	// made them quite easy select. Just using the position of the mouse cursor
	// would feel like a regression, as it would be way harded to hit the notes.
	// Instead, we introduce a certain rectangle (manhattan distance) around the
	// cursor which can select notes but only return those nearest to the
	// center.
	int nLastDistance = nRealColumnUpper - nRealColumn + 1;

	// We have to ensure to only provide notes from a single position. In case
	// the cursor is placed exactly in the middle of two notes, the left one
	// wins.
	int nLastPosition = -1;

	const auto notes = pPattern->getNotes();
	for ( auto it = notes->lower_bound( nRealColumnLower );
		  it != notes->end() && it->first <= nRealColumnUpper; ++it ) {
		const auto ppNote = it->second;
		if ( ppNote != nullptr &&
			 ( ! bExcludeSelected ||
			   bExcludeSelected && ! m_selection.isSelected( ppNote ) ) &&
			 ppNote->get_instrument_id() == row.nInstrumentID &&
			 ppNote->getType() == row.sType ) {

			const int nDistance =
				std::abs( ppNote->get_position() - nRealColumn );

			if ( nDistance < nLastDistance ) {
				// This note is nearer than (potential) previous ones.
				notesUnderPoint.clear();
				nLastDistance = nDistance;
				nLastPosition = ppNote->get_position();
			}

			if ( nDistance <= nLastDistance &&
				 ppNote->get_position() == nLastPosition ) {
				// In case of the PianoRoll editor we do have to additionally
				// differentiate between different pitches.
				if ( m_editor != Editor::PianoRoll ||
					 ( m_editor == Editor::PianoRoll &&
					   ppNote->get_key() ==
					   Note::pitchToKey( Note::lineToPitch( nRow ) ) &&
					   ppNote->get_octave() ==
					   Note::pitchToOctave( Note::lineToPitch( nRow ) ) ) ) {
					notesUnderPoint.push_back( ppNote );
				}
			}
		}
	}

	return std::move( notesUnderPoint );
}

void PatternEditor::updateHoveredNotesMouse( const QPoint& point ) {
	int nRealColumn;
	eventPointToColumnRow( point, nullptr, nullptr, &nRealColumn );
	int nRealColumnUpper;
	eventPointToColumnRow( point + QPoint( getCursorMargin(), 0 ),
						   nullptr, nullptr, &nRealColumnUpper );

	// getNotesAtPoint is generous in finding notes by taking a margin around
	// the cursor into account as well. We have to ensure we only use to closest
	// notes reported.
	int nLastDistance = nRealColumnUpper - nRealColumn + 1;

	// In addition, we have to ensure to only provide notes from a single
	// position. In case the cursor is placed exactly in the middle of two
	// notes, the left one wins.
	int nLastPosition = -1;

	std::map<std::shared_ptr<Pattern>, std::vector<Note*>> hovered;
	for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
		const auto hoveredNotes = getNotesAtPoint( ppPattern, point, false );
		if ( hoveredNotes.size() > 0 ) {
			const int nDistance =
				std::abs( hoveredNotes[ 0 ]->get_position() - nRealColumn );
			if ( nDistance < nLastDistance ) {
				// This batch of notes is nearer than (potential) previous ones.
				hovered.clear();
				nLastDistance = nDistance;
				nLastPosition = hoveredNotes[ 0 ]->get_position();
			}

			if ( hoveredNotes[ 0 ]->get_position() == nLastPosition ) {
				hovered[ ppPattern ] = hoveredNotes;
			}
		}
	}
	m_pPatternEditorPanel->setHoveredNotesMouse( hovered );
}

void PatternEditor::updateHoveredNotesKeyboard() {
	const auto point = getCursorPosition();

	std::map<std::shared_ptr<Pattern>, std::vector<Note*>> hovered;
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// cursor visible
		for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
			const auto hoveredNotes = getNotesAtPoint( ppPattern, point, false );
			if ( hoveredNotes.size() > 0 ) {
				hovered[ ppPattern ] = hoveredNotes;
			}
		}
	}
	m_pPatternEditorPanel->setHoveredNotesKeyboard( hovered );
}

bool PatternEditor::syncLasso() {
	bool bUpdate = false;
	if ( m_editor == Editor::NotePropertiesRuler ) {
		auto pVisibleEditor = m_pPatternEditorPanel->getVisibleEditor();

		QRect cursor, prevLasso;
		QRect cursorStart = m_selection.getKeyboardCursorStart();
		QRect lasso = m_selection.getLasso();
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

			for ( const auto& ppNote : m_selection ) {
				if ( ppNote != nullptr ) {
					const QPoint np = pPianoRoll->noteToPoint( ppNote );
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
