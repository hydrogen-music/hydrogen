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
#include "PianoRollEditor.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
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
	, m_property( Property::None )
	, m_nCursorPitch( 0 )
	, m_nDragStartColumn( 0 )
	, m_nDragY( 0 )
	, m_update( Update::Background )
	, m_bPropertyDragActive( false )
{
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	const auto pPref = H2Core::Preferences::get_instance();

	m_fGridWidth = pPref->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nActiveWidth = m_nEditorWidth;

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking( true );

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

	updateWidth();

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
									   height() * pixelRatio );
	m_pPatternPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
									height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	m_pPatternPixmap->setDevicePixelRatio( pixelRatio );
}

PatternEditor::~PatternEditor()
{
	m_draggedNotes.clear();

	if ( m_pPatternPixmap != nullptr ) {
		delete m_pPatternPixmap;
	}
	if ( m_pBackgroundPixmap != nullptr ) {
		delete m_pBackgroundPixmap;
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

void PatternEditor::zoomLasso( float fOldGridWidth ) {
	if ( m_selection.isLasso() ) {
		const float fScale = m_fGridWidth / fOldGridWidth;
		m_selection.scaleLasso( fScale, PatternEditor::nMargin );
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


void PatternEditor::drawNote( QPainter &p, std::shared_ptr<H2Core::Note> pNote,
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
		if ( pNote->getInstrumentId() != selectedRow.nInstrumentID ||
			 pNote->getType() != selectedRow.sType ) {
			ERRORLOG( QString( "Provided note [%1] is not part of selected row [%2]" )
					  .arg( pNote->toQString() ).arg( selectedRow.toQString() ) );
			return;
		}

		nY = m_nGridHeight *
			Note::pitchToLine( pNote->getPitchFromKeyOctave() ) +
			(m_nGridHeight / 2) - 3;
	}
	const int nX = PatternEditor::nMargin + pNote->getPosition() * m_fGridWidth;

	const auto pPref = H2Core::Preferences::get_instance();
	
	const QColor noteColor( pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor );
	const QColor noteInactiveColor( pPref->getTheme().m_color.m_windowTextColor.darker( 150 ) );
	const QColor noteoffColor( pPref->getTheme().m_color.m_patternEditor_noteOffColor );
	const QColor noteoffInactiveColor( pPref->getTheme().m_color.m_windowTextColor );

	p.setRenderHint( QPainter::Antialiasing );

	QColor color = computeNoteColor( pNote->getVelocity() );

	uint w = 8, h =  8;

	QPen highlightPen;
	QBrush highlightBrush;
	applyHighlightColor( &highlightPen, &highlightBrush, noteStyle );

	QPen movingPen( noteColor );
	QPoint movingOffset;
	if ( noteStyle & NoteStyle::Moved ) {
		movingPen.setStyle( Qt::DotLine );
		movingPen.setColor( highlightPen.color() );
		movingPen.setWidth( 2 );
		QPoint delta = movingGridOffset();
		movingOffset = QPoint( delta.x() * m_fGridWidth,
							   delta.y() * m_nGridHeight );
	}

	if ( pNote->getNoteOff() == false ) {	// trigger note
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
			p.setPen( highlightPen );
			p.setBrush( highlightBrush );
			p.drawEllipse( nX - 4 - 3, nY - 3, w + 6, h + 6 );
			p.setBrush( Qt::NoBrush );
		}

		// Draw tail
		if ( pNote->getLength() != LENGTH_ENTIRE_SAMPLE ) {
			float fNotePitch = pNote->getPitchFromKeyOctave();
			float fStep = Note::pitchToFrequency( ( double )fNotePitch );

			// if there is a stop-note to the right of this note, only draw-
			// its length till there.
			int nLength = pNote->getLength();
			const int nRow = m_pPatternEditorPanel->findRowDB( pNote );
			for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
				if ( ppNote != nullptr &&
					 // noteOff note
					 ppNote->getNoteOff() &&
					 // located in the same row
					 m_pPatternEditorPanel->findRowDB( ppNote ) == nRow ) {
					const int nNotePos = ppNote->getPosition() +
						ppNote->getLeadLag() *
						AudioEngine::getLeadLagInTicks();

					if ( // left of the NoteOff
						pNote->getPosition() < nNotePos &&
						// custom length reaches beyond NoteOff
						pNote->getPosition() + pNote->getLength() >
						nNotePos ) {

						// In case there are multiple stop-notes present, take the
						// shortest distance.
						nLength = std::min( nNotePos - pNote->getPosition(), nLength );
					}
				}
			}

			width = m_fGridWidth * nLength / fStep;
			width = width - 1;	// lascio un piccolo spazio tra una nota ed un altra

			if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
				p.setPen( highlightPen );
				p.setBrush( highlightBrush );
				// Tail highlight
				p.drawRect( nX - 3, nY - 1, width + 6, 3 + 6 );
				p.drawEllipse( nX - 4 - 3, nY - 3, w + 6, h + 6 );
				p.fillRect( nX - 4, nY, width, 3 + 4, highlightBrush );
			}
			if ( ! ( noteStyle & NoteStyle::Moved ) ) {
				p.setPen( notePen );
				p.setBrush( noteBrush );

				// Since the note body is transparent for an inactive note, we
				// try to start the tail at its boundary. For regular notes we
				// do not care about an overlap, as it ensures that there are no
				// white artifacts between tail and note body regardless of the
				// scale factor.
				int nRectOnsetX = nX;
				int nRectWidth = width;
				if ( noteStyle & NoteStyle::Background ) {
					nRectOnsetX = nRectOnsetX + w/2;
					nRectWidth = nRectWidth - w/2;
				}

				p.drawRect( nRectOnsetX, nY +2, nRectWidth, 3 );
				p.drawLine( nX+width, nY, nX+width, nY + h );
			}
		}

		// Draw note
		if ( ! ( noteStyle & NoteStyle::Moved ) ) {
			p.setPen( notePen );
			p.setBrush( noteBrush );
			p.drawEllipse( nX -4 , nY, w, h );
		}
		else {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + nX -4 -2, movingOffset.y() + nY -2 , w + 4, h + 4 );

			// Moving tail
			if ( pNote->getLength() != LENGTH_ENTIRE_SAMPLE ) {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawRoundedRect( movingOffset.x() + nX-2, movingOffset.y() + nY, width+4, 3+4, 4, 4 );
			}
		}
	}
	else if ( pNote->getNoteOff() ) {

		QBrush noteOffBrush( noteoffColor );
		if ( noteStyle & NoteStyle::Background ) {
			noteOffBrush.setStyle( Qt::Dense4Pattern );

			if ( nX >= m_nActiveWidth ) {
				noteOffBrush.setColor( noteoffInactiveColor );
			}
		}

		if ( noteStyle & ( NoteStyle::Selected | NoteStyle::Hovered ) ) {
			p.setPen( highlightPen );
			p.setBrush( highlightBrush );
			p.drawEllipse( nX - 4 - 3, nY - 3, w + 6, h + 6 );
			p.setBrush( Qt::NoBrush );
		}

		if ( ! ( noteStyle & NoteStyle::Moved ) ) {
			p.setPen( Qt::NoPen );
			p.setBrush( noteOffBrush );
			p.drawEllipse( nX -4 , nY, w, h );
		}
		else {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + nX -4 -2, movingOffset.y() + nY -2,
						   w + 4, h + 4 );
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

void PatternEditor::updateEditor( bool bPatternOnly )
{
	if ( updateWidth() ) {
		m_update = Update::Background;
	}
	else if ( bPatternOnly && m_update != Update::Background ) {
		// Background takes priority over Pattern.
		m_update = Update::Pattern;
	}
	else {
		m_update = Update::Background;
	}

	update();
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

	auto pHydrogenApp = HydrogenApp::get_instance();
	QClipboard *clipboard = QApplication::clipboard();
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
				nDeltaPitch = m_nCursorPitch -
					positionNode.read_int( "maxPitch", m_nCursorPitch );
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
					nInstrumentId = pNote->getInstrumentId();
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
				const int nPitch = pNote->getPitchFromKeyOctave() + nDeltaPitch;
				if ( nPitch < KEYS_PER_OCTAVE * OCTAVE_MIN ||
					 nPitch >= KEYS_PER_OCTAVE * ( OCTAVE_MAX + 1 ) ) {
					continue;
				}

				nKey = Note::pitchToKey( nPitch );
				nOctave = Note::pitchToOctave( nPitch );
			}
			else {
				nKey = pNote->getKey();
				nOctave = pNote->getOctave();
			}

			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemoveNoteAction(
					nPos,
					nInstrumentId,
					sType,
					m_pPatternEditorPanel->getPatternNumber(),
					pNote->getLength(),
					pNote->getVelocity(),
					pNote->getPan(),
					pNote->getLeadLag(),
					nKey,
					nOctave,
					pNote->getProbability(),
					/* bIsDelete */ false,
					/* bIsMidi */ false,
					/* bIsNoteOff */ pNote->getNoteOff() ) );
		}
		pHydrogenApp->endUndoMacro();
	}

	if ( bAppendedToDB ) {
		// We added a note to the pattern currently not represented by the DB.
		// We have to force its update in order to avoid inconsistencies.
		const int nOldSize = m_pPatternEditorPanel->getRowNumberDB();
		m_pPatternEditorPanel->updateDB();
		m_pPatternEditorPanel->updateEditors( true );
		m_pPatternEditorPanel->resizeEvent( nullptr );

		// Select the append line
		m_pPatternEditorPanel->setSelectedRowDB( nOldSize );
	}

	m_bSelectNewNotes = false;
}

void PatternEditor::selectAllNotesInRow( int nRow, int nPitch )
{
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
			if ( ppNote != nullptr &&
				 ppNote->getInstrumentId() == row.nInstrumentID &&
				 ppNote->getType() == row.sType &&
				 ppNote->getKey() == key && ppNote->getOctave() == octave ) {
				m_selection.addToSelection( ppNote );
			}
		}
	}
	else {
		for ( const auto& [ _, ppNote ] : *pPattern->getNotes() ) {
			if ( ppNote != nullptr &&
				 ppNote->getInstrumentId() == row.nInstrumentID &&
				 ppNote->getType() == row.sType ) {
				m_selection.addToSelection( ppNote );
			}
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

	auto pHydrogenApp = HydrogenApp::get_instance();

	// Move the notes
	pHydrogenApp->beginUndoMacro( tr( "Align notes to grid" ) );

	for ( const auto& ppNote : m_selection ) {
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
		const int nInstrumentId = ppNote->getInstrumentId();
		const QString sType = ppNote->getType();
		const int nLength = ppNote->getLength();
		const float fVelocity = ppNote->getVelocity();
		const float fPan = ppNote->getPan();
		const float fLeadLag = ppNote->getLeadLag();
		const int nKey = ppNote->getKey();
		const int nOctave = ppNote->getOctave();
		const float fProbability = ppNote->getProbability();
		const bool bNoteOff = ppNote->getNoteOff();

		// Move note -> delete at source position
		pHydrogenApp->pushUndoCommand( new SE_addOrRemoveNoteAction(
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
		pHydrogenApp->pushUndoCommand( new SE_addOrRemoveNoteAction(
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

	pHydrogenApp->endUndoMacro();
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
}

void PatternEditor::mousePressEvent( QMouseEvent *ev ) {
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	if ( ev->x() > m_nActiveWidth || ev->x() <= PatternEditor::nMarginSidebar ) {
		if ( ! m_selection.isEmpty() ) {
			m_selection.clearSelection();
			m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
			m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor( true );
		}
		return;
	}

	updateModifiers( ev );

	if ( ( ev->buttons() == Qt::LeftButton || ev->buttons() == Qt::RightButton ) &&
		 ! ( ev->modifiers() & Qt::ControlModifier ) ) {
		m_notesToSelectOnMove.clear();

		// When interacting with note(s) not already in a selection, we will
		// discard the current selection and add these notes under point to a
		// transient one.
		const auto notesUnderPoint = getElementsAtPoint(
			ev->pos(), getCursorMargin( ev ), pPattern );

		bool bSelectionHovered = false;
		for ( const auto& ppNote : notesUnderPoint ) {
			if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
				bSelectionHovered = true;
				break;
			}
		}

		// We honor the current selection.
		if ( bSelectionHovered ) {
			for ( const auto& ppNote : notesUnderPoint ) {
				if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
					m_notesHoveredOnDragStart.push_back( ppNote );
				}
			}
		}
		else {
			m_notesToSelectOnMove = notesUnderPoint;
			m_notesHoveredOnDragStart = notesUnderPoint;
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
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
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
		setCursorPitch( Note::lineToPitch( nRow ) );

		// Use the row of the DrumPatternEditor/DB for further note
		// interactions.
		nRow = m_pPatternEditorPanel->getSelectedRowDB();
	}

	bool bClickedOnGrid = false;

	// main button action
	if ( ev->button() == Qt::LeftButton &&
		 m_editor != Editor::NotePropertiesRuler ) {

		// Check whether an existing note or an empty grid cell was clicked.
		const auto notesAtPoint = getElementsAtPoint(
			ev->pos(), getCursorMargin( ev ), pPattern );
		if ( notesAtPoint.size() == 0 ) {
			// Empty grid cell
			bClickedOnGrid = true;

			// By pressing the Alt button the user can bypass quantization of
			// new note to the grid.
			const int nTargetColumn = ev->modifiers() & Qt::AltModifier ?
				nRealColumn : nColumn;

			int nKey = KEY_MIN;
			int nOctave = OCTAVE_DEFAULT;
			if ( m_editor == Editor::PianoRoll ) {
				nOctave = Note::pitchToOctave( m_nCursorPitch );
				nKey = Note::pitchToKey( m_nCursorPitch );
			}

			m_pPatternEditorPanel->addOrRemoveNotes(
				nTargetColumn, nRow, nKey, nOctave,
				/* bDoAdd */true, /* bDoDelete */false,
				/* bIsNoteOff */ ev->modifiers() & Qt::ShiftModifier );
		}
		else {
			// Note(s) clicked. Delete them.
			pHydrogenApp->beginUndoMacro(
				pCommonStrings->getActionDeleteNotes() );
			for ( const auto& ppNote : notesAtPoint ) {
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
						/* bIsDelete */ true,
						/* bIsMidi */ false,
						/* bIsNoteOff */ ppNote->getNoteOff() ) );
}
			pHydrogenApp->endUndoMacro();
		}
		m_selection.clearSelection();
		updateHoveredNotesMouse( ev );

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
	updateHoveredNotesMouse( ev );

	if ( ev->buttons() != Qt::NoButton ) {
		updateModifiers( ev );
		m_selection.mouseMoveEvent( ev );
		if ( m_selection.isMoving() ) {
			m_pPatternEditorPanel->getVisibleEditor()->update();
			m_pPatternEditorPanel->getVisiblePropertiesRuler()->update();
			}
		else if ( syncLasso() ) {
			m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
			m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor( true );
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

	m_notesHoveredOnDragStart.clear();

	if ( m_notesToSelectOnMove.size() > 0 ) {
		// We used a transient selection of note(s) at a single position.
		m_selection.clearSelection();
		m_notesToSelectOnMove.clear();
		bUpdate = true;
	}

	if ( bUpdate ) {
		m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor( true );
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

int PatternEditor::getCursorMargin( QInputEvent* pEvent ) const {
	const int nResolution = m_pPatternEditorPanel->getResolution();

	// The Alt modifier is used for more fine grained control throughout
	// Hydrogen and will diminish the cursor margin.
	if ( pEvent->modifiers() & Qt::AltModifier ) {
		return 0;
	}

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
	std::set< std::shared_ptr<Note> > duplicates;
	for ( const auto& ppNote : elements ) {
		if ( duplicates.find( ppNote ) != duplicates.end() ) {
			// Already marked ppNote as a duplicate of some other ppNote. Skip it.
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND_END( pPattern->getNotes(), it, ppNote->getPosition() ) {
			// Duplicate note of a selected note is anything occupying the same position. Multiple notes
			// sharing the same location might be selected; we count these as duplicates too. They will appear
			// in both the duplicates and selection lists.
			if ( it->second != ppNote && ppNote->match( it->second ) ) {
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


void PatternEditor::deselectAndOverwriteNotes( const std::vector< std::shared_ptr<H2Core::Note> >& selected,
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


void PatternEditor::undoDeselectAndOverwriteNotes( const std::vector< std::shared_ptr<H2Core::Note> >& selected,
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
	m_pPatternEditorPanel->updateEditors( true );
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


void PatternEditor::applyHighlightColor( QPen* pPen, QBrush* pBrush,
											 NoteStyle noteStyle ) const {
	
	const auto pPref = H2Core::Preferences::get_instance();

	QColor color;
	if ( m_pPatternEditorPanel->getEntered() ||
		 m_pPatternEditorPanel->hasPatternEditorFocus() ) {
		color = pPref->getTheme().m_color.m_selectionHighlightColor;
	} else {
		color = pPref->getTheme().m_color.m_selectionInactiveColor;
	}

	int nFactor = 100;
	if ( noteStyle & NoteStyle::Selected && noteStyle & NoteStyle::Hovered ) {
		nFactor = 107;
	}
	else if ( noteStyle & NoteStyle::Hovered ) {
		nFactor = 125;
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

	if ( pBrush != nullptr ) {
		pBrush->setColor( color );
		pBrush->setStyle( Qt::SolidPattern );
	}

	if ( pPen != nullptr ) {
		int nHue, nSaturation, nValue;
		color.getHsv( &nHue, &nSaturation, &nValue );
		if ( nValue >= 130 ) {
			pPen->setColor( Qt::black );
		} else {
			pPen->setColor( Qt::white );
		}
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

void PatternEditor::deleteSelection()
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	if ( m_selection.begin() != m_selection.end() ) {
		// Selection exists, delete it.

		auto pHydrogenApp = HydrogenApp::get_instance();

		validateSelection();

		// Construct list of UndoActions to perform before performing any of them, as the
		// addOrDeleteNoteAction may delete duplicate notes in undefined order.
		std::list<QUndoCommand*> actions;
		for ( const auto pNote : m_selection ) {
			if ( pNote != nullptr && m_selection.isSelected( pNote ) ) {
				actions.push_back( new SE_addOrRemoveNoteAction(
									   pNote->getPosition(),
									   pNote->getInstrumentId(),
									   pNote->getType(),
									   m_pPatternEditorPanel->getPatternNumber(),
									   pNote->getLength(),
									   pNote->getVelocity(),
									   pNote->getPan(),
									   pNote->getLeadLag(),
									   pNote->getKey(),
									   pNote->getOctave(),
									   pNote->getProbability(),
									   true, // bIsDelete
									   false, // bIsMidi
									   pNote->getNoteOff() ) );
			}
		}
		m_selection.clearSelection();

		if ( actions.size() > 0 ) {
			pHydrogenApp->beginUndoMacro(
				HydrogenApp::get_instance()->getCommonStrings()
				->getActionDeleteNotes() );
			for ( QUndoCommand *pAction : actions ) {
				pHydrogenApp->pushUndoCommand( pAction );
			}
			pHydrogenApp->endUndoMacro();
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

	m_bSelectNewNotes = true;

	for ( auto pNote : selectedNotes ) {
		if ( pNote == nullptr ) {
			continue;
		}
		const int nPosition = pNote->getPosition();
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

		int nNewKey = pNote->getKey();
		int nNewOctave = pNote->getOctave();
		int nNewPitch = pNote->getPitchFromKeyOctave();
		if ( m_editor == Editor::PianoRoll && offset.y() != 0 ) {
			nNewPitch -= offset.y();
			nNewKey = Note::pitchToKey( nNewPitch );
			nNewOctave = Note::pitchToOctave( nNewPitch );
		}

		// For NotePropertiesRuler there is no vertical displacement.

		bool bNoteInRange = nNewPosition >= 0 &&
			nNewPosition <= pPattern->getLength();
		if ( m_editor == Editor::DrumPattern ) {
			bNoteInRange = bNoteInRange && nNewRow >= 0 &&
				nNewRow <= m_pPatternEditorPanel->getRowNumberDB();
		}
		else if ( m_editor == Editor::PianoRoll ){
			bNoteInRange = bNoteInRange && nNewOctave >= OCTAVE_MIN &&
				nNewOctave <= OCTAVE_MAX;
		}

		// Cache note properties since a potential first note deletion will also
		// call the note's destructor.
		const int nLength = pNote->getLength();
		const float fVelocity = pNote->getVelocity();
		const float fPan = pNote->getPan();
		const float fLeadLag = pNote->getLeadLag();
		const int nKey = pNote->getKey();
		const int nOctave = pNote->getOctave();
		const float fProbability = pNote->getProbability();
		const bool bNoteOff = pNote->getNoteOff();

		// We'll either select the new, duplicated note or the new, moved
		// replacement of the note.
		m_selection.removeFromSelection( pNote, false );

		if ( ! m_bCopyNotMove ) {
			// Note is moved either out of range or to a new position. Delete
			// the note at the source position.
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemoveNoteAction(
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
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemoveNoteAction(
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

	// Selecting the clicked row
	auto pMouseEvent = dynamic_cast<QMouseEvent*>(ev);
	if ( pMouseEvent != nullptr ) {
		int nRow;
		eventPointToColumnRow( pMouseEvent->pos(), nullptr, &nRow, nullptr );

		if ( m_editor == Editor::DrumPattern ) {
			m_pPatternEditorPanel->setSelectedRowDB( nRow );
		}
		else if ( m_editor == Editor::PianoRoll ) {
			setCursorPitch( Note::lineToPitch( nRow ) );
		}

		// Handling the cursor column is a lot more difficult. We would need to
		// take into account whether the dragged note was positioned off grid
		// and whether Alt is pressed. Just looking at the notes under point
		// does not cut it either since - without Alt - we just quantize the
		// resulting position to the next grid point. Thus, we don't update
		// cursor column on mouse event.
	}

	m_bSelectNewNotes = false;
	pHydrogenApp->endUndoMacro();
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

void PatternEditor::keyPressEvent( QKeyEvent *ev, bool bFullUpdate )
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
		bFullUpdate = pVisibleEditor->syncLasso() || bFullUpdate;
	}
	else {
		m_selection.updateKeyboardCursorPosition();
		bFullUpdate = syncLasso() || bFullUpdate;
	}
	updateHoveredNotesKeyboard();

	if ( bUnhideCursor ) {
		handleKeyboardCursor( bUnhideCursor );
	}

	if ( bFullUpdate ) {
		// Notes have might become selected. We have to update the background as
		// well.
		m_pPatternEditorPanel->getVisibleEditor()->updateEditor( true );
		m_pPatternEditorPanel->getVisiblePropertiesRuler()->updateEditor( true );
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
			m_pPatternEditorPanel->ensureVisible();

			if ( m_selection.isLasso() ) {
				// Since the event was used to alter the note selection, we need
				// to repainting all note symbols (including whether or not they
				// are selected).
				m_update = Update::Pattern;
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
		std::map< std::shared_ptr<Pattern>,
				  std::vector< std::shared_ptr<Note> > > empty;
		// Takes care of the update.
		m_pPatternEditorPanel->setHoveredNotesMouse( empty );
	}

	// Ending the enclosing undo context. This is key to enable the Undo/Redo
	// buttons in the main menu again and it feels like a good rule of thumb to
	// consider an action done whenever the user moves mouse or cursor away from
	// the widget.
	HydrogenApp::get_instance()->endUndoContext();

	// update focus and hovered notes
	update();
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

		if ( m_editor == Editor::NotePropertiesRuler ) {
			m_pPatternEditorPanel->getVisibleEditor()->update();
		}
		else {
			m_pPatternEditorPanel->getVisiblePropertiesRuler()->update();
		}
	}
	
	// Update to remove the focus border highlight
	updateEditor();
}

void PatternEditor::paintEvent( QPaintEvent* ev )
{
	if (!isVisible()) {
		return;
	}

	const auto pPref = Preferences::get_instance();

	const qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_update == Update::Background ) {
		createBackground();
	}

	if ( m_update == Update::Background || m_update == Update::Pattern ) {
		drawPattern();
		m_update = Update::None;
	}

	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pPatternPixmap,
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
		 m_pPatternEditorPanel->hasPatternEditorFocus() ) {
		QColor cursorColor( pPref->getTheme().m_color.m_cursorColor );
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

void PatternEditor::drawPattern()
{
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
	const auto pPref = H2Core::Preferences::get_instance();
	const QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
					  getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	const QColor textColor(
		pPref->getTheme().m_color.m_patternEditor_noteVelocityDefaultColor );
	QColor textBackgroundColor( textColor );
	textBackgroundColor.setAlpha( 150 );

	validateSelection();

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// If there are multiple notes at the same position and column, the one with
	// lowest pitch (bottom-most one in PianoRollEditor) will be rendered up
	// front. If a subset of notes at this point is selected, the note with
	// lowest pitch within the selection is used.
	auto sortAndDrawNotes = [&]( QPainter& p,
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
	};

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
				// Notes are located beyond the active length of the editor and
				// aren't visible even when drawn.
				break;
			}
			if ( ppNote == nullptr ||
				 ( m_editor == Editor::PianoRoll &&
				   ( ppNote->getInstrumentId() != selectedRow.nInstrumentID ||
					 ppNote->getType() != selectedRow.sType ) ) ) {
				continue;
			}

			int nRow = -1;
			nRow = m_pPatternEditorPanel->findRowDB( ppNote );
			auto row = m_pPatternEditorPanel->getRowDB( nRow );
			if ( nRow == -1 ||
				 ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) ) {
				ERRORLOG( QString( "Note [%1] not associated with DB" )
						  .arg( ppNote->toQString() ) );
				m_pPatternEditorPanel->printDB();
				continue;
			}

			if ( m_editor == Editor::PianoRoll ) {
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
			const int x = PatternEditor::nMargin +
				( nnColumn * m_fGridWidth );
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

void PatternEditor::drawFocus( QPainter& p ) {

	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}

	const auto pPref = H2Core::Preferences::get_instance();

	QColor color = pPref->getTheme().m_color.m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't clicked it yet,
	// the highlight will be done more transparent to indicate that keyboard
	// inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	const QScrollArea* pScrollArea;

	if ( m_editor == Editor::DrumPattern ) {
		pScrollArea = m_pPatternEditorPanel->getDrumPatternEditorScrollArea();
	}
	else if ( m_editor == Editor::PianoRoll ) {
		pScrollArea = m_pPatternEditorPanel->getPianoRollEditorScrollArea();
	}
	else if ( m_editor == Editor::NotePropertiesRuler ) {
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
	const int nStartX = pScrollArea->horizontalScrollBar()->value();
	int nEndY = nStartY + pScrollArea->viewport()->size().height();
	if ( m_editor == Editor::DrumPattern ) {
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

void PatternEditor::drawBorders( QPainter& p ) {
	const auto pPref = H2Core::Preferences::get_instance();

	const QColor borderColor(
		pPref->getTheme().m_color.m_patternEditor_lineColor );
	const QColor borderInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

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

void PatternEditor::createBackground() {
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
						  pHydrogen->getAudioEngine()->getPlayingPatterns()->longest_pattern_length( false ) + 1,
						  static_cast<float>(nActiveWidth) );
		}
		else {
			nEditorWidth = nActiveWidth;
		}
	}
	else {
		nEditorWidth = PatternEditor::nMargin + MAX_NOTES * m_fGridWidth;
		nActiveWidth = nEditorWidth;
	}

	if ( m_nEditorWidth != nEditorWidth || m_nActiveWidth != nActiveWidth ) {
		m_nEditorWidth = nEditorWidth;
		m_nActiveWidth = nActiveWidth;

		resize( m_nEditorWidth, m_nEditorHeight );

		return true;
	}

	return false;
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

	m_property = m_pPatternEditorPanel->getSelectedNoteProperty();

	if ( ev->button() == Qt::RightButton ) {
		// Adjusting note properties.
		m_bPropertyDragActive = true;

		const auto notesAtPoint = getElementsAtPoint(
			ev->pos(), getCursorMargin( ev ), pPattern );
		if ( notesAtPoint.size() == 0 ) {
			return;
		}

		m_draggedNotes.clear();
		// Either all or none of the notes at point should be selected. It is
		// safe to just check the first one.
		if ( m_selection.isSelected( notesAtPoint[ 0 ] ) ) {
			// The clicked note is part of the current selection. All selected
			// notes will be edited.
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
				// NoteOff notes can have both a custom lead/lag and
				// probability. But all other properties won't take effect.
				if ( ! ( ppNote->getNoteOff() &&
						( m_property != Property::LeadLag &&
						  m_property != Property::Probability ) ) ) {
					m_draggedNotes[ ppNote ] = std::make_shared<Note>( ppNote );
				}
			}
		}
		// All notes at located at the same point.
		m_nDragStartColumn = notesAtPoint[ 0 ]->getPosition();
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
		float fNotePitch = ppNote->getPitchFromKeyOctave();
		float fStep = 0;
		if ( nLen > -1 ){
			fStep = Note::pitchToFrequency( ( double )fNotePitch );
		} else {
			fStep=  1.0;
		}
		ppNote->setLength( nLen * fStep );


		// edit note property. We do not support the note key property.
		if ( m_property != Property::KeyOctave ) {
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
		
			float fMoveY = m_nDragY - ev->y();
			fValue = fValue  + (fMoveY / 100);
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

		}
	}
	m_nDragY = ev->y();

	if ( m_property != Property::KeyOctave ) {
		triggerStatusMessage( m_notesHoveredOnDragStart, m_property );
	}

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

	m_bPropertyDragActive = false;

	if ( m_draggedNotes.size() == 0 ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();

	bool bMacroStarted = false;
	if ( m_draggedNotes.size() > 1 ) {
		pHydrogenApp->beginUndoMacro( tr( "edit note properties by dragging" ) );
		bMacroStarted = true;
	}
	else {
		// Just a single note was edited.
		for ( const auto& [ ppUpdatedNote, ppOriginalNote ] : m_draggedNotes ) {
			if ( ( ppUpdatedNote->getVelocity() !=
				   ppOriginalNote->getVelocity() ||
				   ppUpdatedNote->getPan() !=
				   ppOriginalNote->getPan() ||
				   ppUpdatedNote->getLeadLag() !=
				   ppOriginalNote->getLeadLag() ||
				   ppUpdatedNote->getProbability() !=
				   ppOriginalNote->getProbability() ) &&
				 ppUpdatedNote->getLength() != ppOriginalNote->getLength() ) {
				// Both length and another property have been edited.
				pHydrogenApp->beginUndoMacro(
					tr( "edit note properties by dragging" ) );
				bMacroStarted = true;
			}
		}
	}

	auto editNoteProperty = [=]( PatternEditor::Property property,
								 std::shared_ptr<Note> pNewNote,
								 std::shared_ptr<Note> pOldNote ) {
		pHydrogenApp->pushUndoCommand(
			new SE_editNotePropertiesAction(
				property,
				m_pPatternEditorPanel->getPatternNumber(),
				pNewNote->getPosition(),
				pNewNote->getInstrumentId(),
				pNewNote->getInstrumentId(),
				pNewNote->getType(),
				pNewNote->getType(),
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
				pNewNote->getKey(),
				pOldNote->getKey(),
				pNewNote->getOctave(),
				pOldNote->getOctave() ) );
	};

	std::vector< std::shared_ptr<Note> > notesStatusLength, notesStatusProp;

	for ( const auto& [ ppUpdatedNote, ppOriginalNote ] : m_draggedNotes ) {
		if ( ppUpdatedNote == nullptr || ppOriginalNote == nullptr ) {
			continue;
		}

		if ( ppUpdatedNote->getLength() != ppOriginalNote->getLength() ) {
			editNoteProperty( Property::Length, ppUpdatedNote, ppOriginalNote );

			// We only trigger status messages for notes hovered by the user.
			for ( const auto ppNote : m_notesHoveredOnDragStart ) {
				if ( ppNote == ppOriginalNote ) {
					notesStatusLength.push_back( ppUpdatedNote );
				}
			}
		}

		if ( ppUpdatedNote->getVelocity() != ppOriginalNote->getVelocity() ||
			 ppUpdatedNote->getPan() != ppOriginalNote->getPan() ||
			 ppUpdatedNote->getLeadLag() != ppOriginalNote->getLeadLag() ||
			 ppUpdatedNote->getProbability() !=
			 ppOriginalNote->getProbability() ) {
			editNoteProperty( m_property, ppUpdatedNote, ppOriginalNote );

			// We only trigger status messages for notes hovered by the user.
			for ( const auto ppNote : m_notesHoveredOnDragStart ) {
				if ( ppNote == ppOriginalNote ) {
					notesStatusProp.push_back( ppUpdatedNote );
				}
			}
		}
	}

	if ( notesStatusLength.size() > 0 ) {
		triggerStatusMessage( notesStatusLength, Property::Length );
	}
	if ( notesStatusProp.size() > 0 ) {
		triggerStatusMessage( notesStatusProp, m_property );
	}

	if ( bMacroStarted ) {
		pHydrogenApp->endUndoMacro();
	}

	m_draggedNotes.clear();
}

void PatternEditor::editNotePropertiesAction( const Property& property,
											  int nPatternNumber,
											  int nPosition,
											  int nOldInstrumentId,
											  int nNewInstrumentId,
											  const QString& sOldType,
											  const QString& sNewType,
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
	auto pNote = pPattern->findNote(
		nPosition, nOldInstrumentId, sOldType, static_cast<Note::Key>(nOldKey),
		static_cast<Note::Octave>(nOldOctave) );
	if ( pNote == nullptr && property == Property::Type ) {
		// Maybe the type of an unmapped note was set to one already present in
		// the drumkit. In this case the instrument id of the note is remapped
		// and might not correspond to the value used to create the undo/redo
		// action.
		bool bOk;
		const int nKitId =
			pSong->getDrumkit()->toDrumkitMap()->getId( sOldType, &bOk );
		if ( bOk ) {
			pNote = pPattern->findNote(
				nPosition, nKitId, sOldType, static_cast<Note::Key>(nOldKey),
				static_cast<Note::Octave>(nOldOctave) );
		}
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
			if ( pNote->getKey() != nNewKey ||
				 pNote->getOctave() != nNewOctave ) {
				pNote->setKeyOctave( static_cast<Note::Key>(nNewKey),
									 static_cast<Note::Octave>(nNewOctave) );
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
				 pNote->getInstrumentId() != nNewInstrumentId ) {
				pNote->setInstrumentId( nNewInstrumentId );
				pNote->setType( sNewType );

				pNote->mapTo( pSong->getDrumkit() );

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

		if ( property == Property::Type ) {
			pPatternEditorPanel->updateDB();
			pPatternEditorPanel->updateEditors();
			pPatternEditorPanel->resizeEvent( nullptr );
		}
		else {
			pPatternEditorPanel->updateEditors( true );
		}
	}
}

void PatternEditor::addOrRemoveNoteAction( int nPosition,
										   int nInstrumentId,
										   const QString& sType,
										   int nPatternNumber,
										   int nOldLength,
										   float fOldVelocity,
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
	auto pVisibleEditor = pPatternEditorPanel->getVisibleEditor();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );	// lock the audio engine

	if ( bIsDelete ) {
		// Find and delete an existing (matching) note.

		// In case there are multiple notes at this position, use all provided
		// properties to find right one.
		std::vector< std::shared_ptr<Note> > notesFound;
		const auto pNotes = pPattern->getNotes();
		for ( auto it = pNotes->lower_bound( nPosition );
			  it != pNotes->end() && it->first <= nPosition; ++it ) {
			auto ppNote = it->second;
			if ( ppNote != nullptr &&
				 ppNote->getInstrumentId() == nInstrumentId &&
				 ppNote->getType() == sType &&
				 ppNote->getKey() == static_cast<Note::Key>(nOldKey) &&
				 ppNote->getOctave() == static_cast<Note::Octave>(nOldOctave) ) {
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
		if ( nInstrumentId != EMPTY_INSTR_ID ) {
			// Can still be nullptr for notes in unmapped id-only rows.
			pInstrument =
				pSong->getDrumkit()->getInstruments()->find( nInstrumentId );
		}

		auto pNote = std::make_shared<Note>( pInstrument, nPosition, fVelocity,
											 fPan, nLength );
		pNote->setInstrumentId( nInstrumentId );
		pNote->setType( sType );
		pNote->setNoteOff( bIsNoteOff );
		pNote->setLeadLag( fOldLeadLag );
		pNote->setProbability( fOldProbability );
		pNote->setKeyOctave( static_cast<Note::Key>(nOldKey),
							   static_cast<Note::Octave>(nOldOctave) );
		pPattern->insertNote( pNote );

		if ( pVisibleEditor->getSelectNewNotes() ) {
			pVisibleEditor->m_selection.addToSelection( pNote );
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
	default:
		s = QString( "Unknown property [%1]" ).arg( static_cast<int>(property) ) ;
		break;
	}

	return s;
}

QString PatternEditor::updateToQString( const Update& update ) {
	switch ( update ) {
	case PatternEditor::Update::Background:
		return "Background";
	case PatternEditor::Update::Pattern:
		return "Pattern";
	case PatternEditor::Update::None:
		return "None";
	default:
		return QString( "Unknown update [%1]" ).arg( static_cast<int>(update) ) ;
	}
}

void PatternEditor::triggerStatusMessage(
	const std::vector< std::shared_ptr<Note> > notes, const Property& property ) {
	QString sCaller( _class_name() );
	QString sUnit( tr( "ticks" ) );

	// Aggregate all values of the provided notes
	QStringList values;
	float fValue;
	for ( const auto& ppNote : notes ) {
		if ( ppNote == nullptr ) {
			continue;
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
					.arg( ppNote->getOctave() );
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

QPoint PatternEditor::getCursorPosition()
{
	const int nX = PatternEditor::nMargin +
		m_pPatternEditorPanel->getCursorColumn() * m_fGridWidth;
	int nY;
	if ( m_editor == Editor::PianoRoll ) {
		nY = m_nGridHeight * Note::pitchToLine( m_nCursorPitch ) + 1;
	}
	else {
		nY = m_nGridHeight * m_pPatternEditorPanel->getSelectedRowDB();
	}

	return QPoint( nX, nY );
}

void PatternEditor::setCursorPitch( int nCursorPitch ) {
	const int nMinPitch = Note::octaveKeyToPitch(
		(Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN );
	const int nMaxPitch = Note::octaveKeyToPitch(
		(Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX );

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
	if ( m_editor == Editor::PianoRoll ) {
		m_update = Update::Background;
		update();
	}

	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		m_pPatternEditorPanel->ensureVisible();
		m_pPatternEditorPanel->getSidebar()->updateEditor();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
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

std::vector< std::shared_ptr<Note> > PatternEditor::getElementsAtPoint(
	const QPoint& point, int nCursorMargin,
	std::shared_ptr<H2Core::Pattern> pPattern )
{
	std::vector< std::shared_ptr<Note> > notesUnderPoint;
	if ( pPattern == nullptr ) {
		pPattern = m_pPatternEditorPanel->getPattern();
		if ( pPattern == nullptr ) {
			return std::move( notesUnderPoint );
		}
	}

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
			 ppNote->getInstrumentId() == row.nInstrumentID &&
			 ppNote->getType() == row.sType ) {

			const int nDistance =
				std::abs( ppNote->getPosition() - nRealColumn );

			if ( nDistance < nLastDistance ) {
				// This note is nearer than (potential) previous ones.
				notesUnderPoint.clear();
				nLastDistance = nDistance;
				nLastPosition = ppNote->getPosition();
			}

			if ( nDistance <= nLastDistance &&
				 ppNote->getPosition() == nLastPosition ) {
				// In case of the PianoRoll editor we do have to additionally
				// differentiate between different pitches.
				if ( m_editor != Editor::PianoRoll ||
					 ( m_editor == Editor::PianoRoll &&
					   ppNote->getKey() ==
					   Note::pitchToKey( Note::lineToPitch( nRow ) ) &&
					   ppNote->getOctave() ==
					   Note::pitchToOctave( Note::lineToPitch( nRow ) ) ) ) {
					notesUnderPoint.push_back( ppNote );
				}
			}
		}
	}

	// Within the ruler all selected notes are along notes of the selected row.
	// These notes can be interacted with (property change, deselect etc.).
	if ( m_editor == Editor::NotePropertiesRuler ) {
		// Ensure we do not add the same note twice.
		std::vector< std::shared_ptr<Note> > furtherNotes;
		bool bFound = false;
		for ( const auto& ppNote : m_selection ) {
			bFound = false;
			for ( const auto& ppCurrentNote : notesUnderPoint )
				if ( ppCurrentNote == ppNote ) {
					bFound = true;
					break;
				}
			if ( ! bFound && ppNote != nullptr ) {
				furtherNotes.push_back( ppNote );
			}
		}

		for ( const auto& ppNote : furtherNotes ) {
			const int nDistance = std::abs( ppNote->getPosition() - nRealColumn );

			if ( nDistance < nLastDistance ) {
				// This note is nearer than (potential) previous ones.
				notesUnderPoint.clear();
				nLastDistance = nDistance;
				nLastPosition = ppNote->getPosition();
			}

			if ( nDistance <= nLastDistance &&
				 ppNote->getPosition() == nLastPosition ) {
				notesUnderPoint.push_back( ppNote );
			}
		}
	}

	return std::move( notesUnderPoint );
}

void PatternEditor::updateHoveredNotesMouse( QMouseEvent* pEvent ) {
	const int nCursorMargin = getCursorMargin( pEvent );

	int nRealColumn;
	eventPointToColumnRow( pEvent->pos(), nullptr, nullptr, &nRealColumn );
	int nRealColumnUpper;
	eventPointToColumnRow( pEvent->pos() + QPoint( nCursorMargin, 0 ),
						   nullptr, nullptr, &nRealColumnUpper );

	// getElementsAtPoint is generous in finding notes by taking a margin around
	// the cursor into account as well. We have to ensure we only use to closest
	// notes reported.
	int nLastDistance = nRealColumnUpper - nRealColumn + 1;

	// In addition, we have to ensure to only provide notes from a single
	// position. In case the cursor is placed exactly in the middle of two
	// notes, the left one wins.
	int nLastPosition = -1;

	std::map< std::shared_ptr<Pattern>,
			  std::vector< std::shared_ptr<Note> > > hovered;
	// We do not highlight hovered notes during a property drag. Else, the
	// hovered ones would appear in front of the dragged one in the ruler,
	// hiding the newly adjusted value.
	if ( ! m_bPropertyDragActive &&
		 pEvent->x() > PatternEditor::nMarginSidebar ) {
		for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
			const auto hoveredNotes = getElementsAtPoint(
				pEvent->pos(), nCursorMargin, ppPattern );
			if ( hoveredNotes.size() > 0 ) {
				const int nDistance =
					std::abs( hoveredNotes[ 0 ]->getPosition() - nRealColumn );
				if ( nDistance < nLastDistance ) {
					// This batch of notes is nearer than (potential) previous ones.
					hovered.clear();
					nLastDistance = nDistance;
					nLastPosition = hoveredNotes[ 0 ]->getPosition();
				}

				if ( hoveredNotes[ 0 ]->getPosition() == nLastPosition ) {
					hovered[ ppPattern ] = hoveredNotes;
				}
			}
		}
	}
	m_pPatternEditorPanel->setHoveredNotesMouse( hovered );
}

void PatternEditor::updateHoveredNotesKeyboard() {
	const auto point = getCursorPosition();

	std::map< std::shared_ptr<Pattern>,
			  std::vector< std::shared_ptr<Note> > > hovered;
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// cursor visible
		for ( const auto& ppPattern : m_pPatternEditorPanel->getPatternsToShow() ) {
			const auto hoveredNotes = getElementsAtPoint( point, 0, ppPattern );
			if ( hoveredNotes.size() > 0 ) {
				hovered[ ppPattern ] = hoveredNotes;
			}
		}
	}
	m_pPatternEditorPanel->setHoveredNotesKeyboard( hovered );
}

bool PatternEditor::syncLasso() {

	const int nMargin = 5;

	bool bUpdate = false;
	if ( m_editor == Editor::NotePropertiesRuler ) {
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
				if ( ppNote != nullptr && ppNote->getType() == row.sType &&
					 ppNote->getInstrumentId() == row.nInstrumentID ) {
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

bool PatternEditor::isSelectionMoving() const {
	return m_selection.isMoving();
}
