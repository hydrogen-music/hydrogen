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

#include "PianoRollEditor.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "PatternEditorInstrumentList.h"
#include "UndoActions.h"
#include <cassert>

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Xml.h>
using namespace H2Core;

#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"


PianoRollEditor::PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel,
								  QScrollArea *pScrollView)
	: PatternEditor( pParent, panel )
	, m_pScrollView( pScrollView )
{
	m_editor = PatternEditor::Editor::PianoRoll;
	
	m_nGridHeight = 10;
	m_nOctaves = 7;

	setAttribute(Qt::WA_OpaquePaintEvent);

	m_nEditorHeight = m_nOctaves * 12 * m_nGridHeight;

	m_pTemp = new QPixmap( m_nEditorWidth, m_nEditorHeight );

	m_nCursorPitch = 0;

	resize( m_nEditorWidth, m_nEditorHeight );
	
	createBackground();

	HydrogenApp::get_instance()->addEventListener( this );

	m_bNeedsUpdate = true;
	m_bNeedsBackgroundUpdate = true;
	m_bSelectNewNotes = false;
}



PianoRollEditor::~PianoRollEditor()
{
	INFOLOG( "DESTROY" );
	delete m_pTemp;
}


void PianoRollEditor::updateEditor( bool bPatternOnly )
{
	// Ensure that m_pPattern is up to date.
	updatePatternInfo();
	updateWidth();
	
	if ( !bPatternOnly ) {
		m_bNeedsBackgroundUpdate = true;
	}
	if ( !m_bNeedsUpdate ) {
		m_bNeedsUpdate = true;
		update();
	}
}

void PianoRollEditor::finishUpdateEditor()
{
	assert( m_bNeedsUpdate );
	resize( m_nEditorWidth, height() );

	// Ensure that m_pPattern is up to date.
	updatePatternInfo();

	if ( m_bNeedsBackgroundUpdate ) {
		createBackground();
	} else {
		drawPattern();
	}
	
	//	ERRORLOG(QString("update editor %1").arg(m_nEditorWidth));
	m_bNeedsUpdate = false;
	m_bNeedsBackgroundUpdate = false;
}

void PianoRollEditor::selectedInstrumentChangedEvent()
{
	// Update pattern only
	updateEditor( true );
}

void PianoRollEditor::selectedPatternChangedEvent()
{
	updateEditor();
}

void PianoRollEditor::songModeActivationEvent() {
	updateEditor();
}

void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}
	
	auto pPref = Preferences::get_instance();
	
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() || m_bBackgroundInvalid ) {
		createBackground();
	}

	QPainter painter( this );
	if ( m_bNeedsUpdate ) {
		finishUpdateEditor();
	}
	painter.drawPixmap( ev->rect(), *m_pTemp,
						QRectF( pixelRatio * ev->rect().x(),
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
		painter.drawLine( nX, 2, nX, height() - 2 );
	}

	drawFocus( painter );
	
	m_selection.paintSelection( &painter );

	// Draw cursor
	if ( hasFocus() && !HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		QPoint pos = cursorPosition();

		QPen pen( pPref->getColorTheme()->m_cursorColor );
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.setBrush( Qt::NoBrush );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( getKeyboardCursorRect(), 4, 4 );
	}
}

void PianoRollEditor::drawFocus( QPainter& painter ) {

	auto pPref = H2Core::Preferences::get_instance();
	
	if ( ! m_bEntered && ! hasFocus() ) {
		return;
	}
	
	QColor color = pPref->getColorTheme()->m_highlightColor;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		color.setAlpha( 125 );
	}

	int nStartY = HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditorScrollArea()->verticalScrollBar()->value();
	int nStartX = HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditorScrollArea()->horizontalScrollBar()->value();
	int nEndY = nStartY + HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditorScrollArea()->viewport()->size().height();
	int nEndX = std::min( nStartX + HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditorScrollArea()->viewport()->size().width(), width() );

	QPen pen( color );
	pen.setWidth( 4 );
	painter.setPen( pen );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nEndX, nStartY ) );
	painter.drawLine( QPoint( nStartX, nStartY ), QPoint( nStartX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nStartY ), QPoint( nEndX, nEndY ) );
	painter.drawLine( QPoint( nEndX, nEndY ), QPoint( nStartX, nEndY ) );
}

void PianoRollEditor::createBackground()
{
	auto pPref = H2Core::Preferences::get_instance();
	
	const QColor backgroundColor = pPref->getColorTheme()->m_patternEditor_backgroundColor;
	const QColor backgroundInactiveColor = pPref->getColorTheme()->m_windowColor;
	const QColor alternateRowColor = pPref->getColorTheme()->m_patternEditor_alternateRowColor;
	const QColor octaveColor = pPref->getColorTheme()->m_patternEditor_octaveRowColor;
	// The line corresponding to the default pitch set to new notes
	// will be highlighted.
	const QColor baseNoteColor = octaveColor.lighter( 119 );
	const QColor lineColor( pPref->getColorTheme()->m_patternEditor_lineColor );
	const QColor lineInactiveColor( pPref->getColorTheme()->m_windowTextColor.darker( 170 ) );

	unsigned start_x = 0;
	unsigned end_x = m_nActiveWidth;

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		delete m_pTemp;
		m_pTemp = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pTemp->setDevicePixelRatio( pixelRatio );
	}

	m_pBackgroundPixmap->fill( backgroundInactiveColor );

	QPainter p( m_pBackgroundPixmap );

	for ( uint ooctave = 0; ooctave < m_nOctaves; ++ooctave ) {
		unsigned start_y = ooctave * 12 * m_nGridHeight;

		for ( int ii = 0; ii < 12; ++ii ) {
			if ( ii == 0 || ii == 2 || ii == 4 || ii == 6 || ii == 7 ||
				 ii == 9 || ii == 11 ) {
				if ( ooctave % 2 != 0 ) {
					p.fillRect( start_x, start_y + ii * m_nGridHeight,
								end_x - start_x, start_y + ( ii + 1 ) * m_nGridHeight,
								octaveColor );
				} else {
					p.fillRect( start_x, start_y + ii * m_nGridHeight,
								end_x - start_x, start_y + ( ii + 1 ) * m_nGridHeight,
								backgroundColor );
				}
			} else {
				p.fillRect( start_x, start_y + ii * m_nGridHeight,
							end_x - start_x, start_y + ( ii + 1 ) * m_nGridHeight,
							alternateRowColor );
			}
		}

		// Highlight base note pitch
		if ( ooctave == 3 ) {
			p.fillRect( start_x, start_y + 11 * m_nGridHeight,
						end_x - start_x, start_y + 12 * m_nGridHeight,
						baseNoteColor );
		}
	}


	// horiz lines
	p.setPen( lineColor );
	for ( uint row = 0; row < ( 12 * m_nOctaves ); ++row ) {
		unsigned y = row * m_nGridHeight;
		p.drawLine( start_x, y, end_x, y );
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( lineInactiveColor );
		for ( uint row = 0; row < ( 12 * m_nOctaves ); ++row ) {
			unsigned y = row * m_nGridHeight;
			p.drawLine( m_nActiveWidth, y, m_nEditorWidth, y );
		}
	}

	//draw text
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	p.setFont( font );
	p.setPen( pPref->getColorTheme()->m_patternEditor_textColor );

	int offset = 0;
	int insertx = 3;
	for ( int oct = 0; oct < (int)m_nOctaves; oct++ ){
		if( oct > 3 ){
			p.drawText( insertx, m_nGridHeight  + offset, "B" );
			p.drawText( insertx, 10 + m_nGridHeight  + offset, "A#" );
			p.drawText( insertx, 20 + m_nGridHeight  + offset, "A" );
			p.drawText( insertx, 30 + m_nGridHeight  + offset, "G#" );
			p.drawText( insertx, 40 + m_nGridHeight  + offset, "G" );
			p.drawText( insertx, 50 + m_nGridHeight  + offset, "F#" );
			p.drawText( insertx, 60 + m_nGridHeight  + offset, "F" );
			p.drawText( insertx, 70 + m_nGridHeight  + offset, "E" );
			p.drawText( insertx, 80 + m_nGridHeight  + offset, "D#" );
			p.drawText( insertx, 90 + m_nGridHeight  + offset, "D" );
			p.drawText( insertx, 100 + m_nGridHeight  + offset, "C#" );
			p.drawText( insertx, 110 + m_nGridHeight  + offset, "C" );
			offset += 12 * m_nGridHeight;
		}else
		{
			p.drawText( insertx, m_nGridHeight  + offset, "b" );
			p.drawText( insertx, 10 + m_nGridHeight  + offset, "a#" );
			p.drawText( insertx, 20 + m_nGridHeight  + offset, "a" );
			p.drawText( insertx, 30 + m_nGridHeight  + offset, "g#" );
			p.drawText( insertx, 40 + m_nGridHeight  + offset, "g" );
			p.drawText( insertx, 50 + m_nGridHeight  + offset, "f#" );
			p.drawText( insertx, 60 + m_nGridHeight  + offset, "f" );
			p.drawText( insertx, 70 + m_nGridHeight  + offset, "e" );
			p.drawText( insertx, 80 + m_nGridHeight  + offset, "d#" );
			p.drawText( insertx, 90 + m_nGridHeight  + offset, "d" );
			p.drawText( insertx, 100 + m_nGridHeight  + offset, "c#" );
			p.drawText( insertx, 110 + m_nGridHeight  + offset, "c" );
			offset += 12 * m_nGridHeight;
		}
	}

	drawGridLines( p, Qt::DashLine );
	drawPattern();
	
	p.setPen( QPen( lineColor, 2, Qt::SolidLine ) );
	p.drawLine( m_nEditorWidth, 0, m_nEditorWidth, m_nEditorHeight );

	m_bBackgroundInvalid = false;
}


void PianoRollEditor::drawPattern()
{
	//INFOLOG( "draw pattern" );

	validateSelection();

	qreal pixelRatio = devicePixelRatio();
	
	QPainter p( m_pTemp );
	// copy the background image
	p.drawPixmap( rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * rect().x(),
								pixelRatio * rect().y(),
								pixelRatio * rect().width(),
								pixelRatio * rect().height() ) );

	// for each note...
	for ( Pattern *pPattern : getPatternsToShow() ) {
		bool bIsForeground = ( pPattern == m_pPattern );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_LENGTH( notes, it, pPattern ) {
			Note *note = it->second;
			assert( note );
			drawNote( note, &p, bIsForeground );
		}
	}

}


void PianoRollEditor::drawNote( Note *pNote, QPainter *pPainter, bool bIsForeground )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( pNote->get_instrument() == pHydrogen->getSelectedInstrument() ) {
		QPoint pos ( PatternEditor::nMargin + pNote->get_position() * m_fGridWidth,
					 m_nGridHeight * pitchToLine( pNote->get_notekey_pitch() ) + 1);
		drawNoteSymbol( *pPainter, pos, pNote, bIsForeground );
	}
}


void PianoRollEditor::addOrRemoveNote( int nColumn, int nRealColumn, int nLine,
									   int nNotekey, int nOctave,
									   bool bDoAdd, bool bDoDelete )
{
	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}
	
	Note::Octave octave = (Note::Octave)nOctave;
	Note::Key notekey = (Note::Key)nNotekey;
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nSelectedInstrumentnumber = pHydrogen->getSelectedInstrumentNumber();
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		DEBUGLOG( "No instrument selected" );
		return;
	}

	Note* pOldNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument,
											  notekey, octave );

	int nLength = -1;
	float fVelocity = 0.8f;
	float fPan = 0.0f;
	float fLeadLag = 0.0f;
	float fProbability = 1.0f;

	if ( pOldNote && !bDoDelete ) {
		// Found an old note, but we don't want to delete, so just return.
		return;
	} else if ( !pOldNote && !bDoAdd ) {
		// No note there, but we don't want to add a new one, so return.
		return;
	}

	if ( pOldNote ) {
		nLength = pOldNote->get_length();
		fVelocity = pOldNote->get_velocity();
		fPan = pOldNote->getPan();
		fLeadLag = pOldNote->get_lead_lag();
		notekey = pOldNote->get_key();
		octave = pOldNote->get_octave();
		fProbability = pOldNote->get_probability();
	}

	if ( pOldNote == nullptr ) {
		// hear note
		Preferences *pref = Preferences::get_instance();
		if ( pref->getHearNewNotes() && pSelectedInstrument->hasSamples() ) {
			Note *pNote2 = new Note( pSelectedInstrument );
			pNote2->set_key_octave( notekey, octave );
			m_pAudioEngine->getSampler()->noteOn( pNote2 );
		}
	}

	SE_addOrDeleteNotePianoRollAction *action = new SE_addOrDeleteNotePianoRollAction( nColumn,
																					   nLine,
																					   m_nSelectedPatternNumber,
																					   nSelectedInstrumentnumber,
																					   nLength,
																					   fVelocity,
																					   fPan,
																					   fLeadLag,
																					   notekey,
																					   octave,
																					   fProbability,
																					   pOldNote != nullptr );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );

}


void PianoRollEditor::mouseClickEvent( QMouseEvent *ev ) {

	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	auto pHydrogenApp = HydrogenApp::get_instance();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

	int nPressedLine = ((int) pEv->position().y()) / ((int) m_nGridHeight);
	if ( nPressedLine >= (int) m_nOctaves * 12 ) {
		return;
	}

	int nColumn = getColumn( pEv->position().x(), /* bUseFineGrained=*/ true );

	if ( nColumn >= (int)m_pPattern->get_length() ) {
		update( 0, 0, width(), height() );
		return;
	}
	m_pPatternEditorPanel->setCursorPosition( nColumn );

	auto pSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	int nSelectedInstrumentnumber = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	int nPitch = lineToPitch( nPressedLine );
	Note::Octave pressedoctave = Note::pitchToOctave( nPitch );
	Note::Key pressednotekey = Note::pitchToKey( nPitch );
	m_nCursorPitch = nPitch;

	if (ev->button() == Qt::LeftButton ) {

		unsigned nRealColumn = 0;
		if( pEv->position().x() > PatternEditor::nMargin ) {
			nRealColumn = (pEv->position().x() - PatternEditor::nMargin) / static_cast<float>(m_fGridWidth);
		}

		if ( ev->modifiers() & Qt::ShiftModifier ) {
			H2Core::Note *pNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave );
			if ( pNote != nullptr ) {
				SE_addOrDeleteNotePianoRollAction *action = new SE_addOrDeleteNotePianoRollAction( nColumn,
																								   nPressedLine,
																								   m_nSelectedPatternNumber,
																								   nSelectedInstrumentnumber,
																								   pNote->get_length(),
																								   pNote->get_velocity(),
																								   pNote->getPan(),
																								   pNote->get_lead_lag(),
																								   pNote->get_key(),
																								   pNote->get_octave(),
																								   pNote->get_probability(),
																								   pNote != nullptr );
				pHydrogenApp->m_pUndoStack->push( action );
			} else {
				SE_addPianoRollNoteOffAction *action = new SE_addPianoRollNoteOffAction( nColumn, nPressedLine, m_nSelectedPatternNumber, nSelectedInstrumentnumber );
				pHydrogenApp->m_pUndoStack->push( action );
			}
			return;
		}

		addOrRemoveNote( nColumn, nRealColumn, nPressedLine, pressednotekey, pressedoctave );

	} else if ( ev->button() == Qt::RightButton ) {
		// Show context menu
		m_pPopupMenu->popup( pEv->globalPosition().toPoint() );

	}

}

void PianoRollEditor::mousePressEvent( QMouseEvent* ev ) {
	auto pEv = static_cast<MouseEvent*>( ev );

	if ( pEv->position().x() > m_nActiveWidth ) {
		return;
	}

	PatternEditor::mousePressEvent( ev );
	
	auto pHydrogenApp = HydrogenApp::get_instance();

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	pHydrogenApp->setHideKeyboardCursor( true );

	// Cursor just got hidden.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		update();
	}

	// Update cursor position
	if ( ! pHydrogenApp->hideKeyboardCursor() ) {
		int nPressedLine = ((int) pEv->position().y()) / ((int) m_nGridHeight);
		if ( nPressedLine >= (int) m_nOctaves * 12 ) {
			return;
		}
		m_nCursorPitch = lineToPitch( nPressedLine );	

		int nColumn = getColumn( pEv->position().x(), /* bUseFineGrained=*/ true );
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

void PianoRollEditor::mouseDragStartEvent( QMouseEvent *ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	// Handles cursor repositioning and hiding and stores general
	// properties.
	PatternEditor::mouseDragStartEvent( ev );
	
	m_pDraggedNote = nullptr;
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nColumn = getColumn( pEv->position().x() );
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		DEBUGLOG( "No instrument selected" );
		return;
	}

	int nRow = std::floor(static_cast<float>(pEv->position().y()) /
						  static_cast<float>(m_nGridHeight));

	Note::Octave pressedOctave = Note::pitchToOctave( lineToPitch( nRow ) );
	Note::Key pressedNoteKey = Note::pitchToKey( lineToPitch( nRow ) );
	m_nCursorPitch = lineToPitch( nRow );

	if (ev->button() == Qt::RightButton ) {

		int nRealColumn = 0;
		if( pEv->position().x() > PatternEditor::nMargin ) {
			nRealColumn =
				static_cast<int>(std::floor(
					static_cast<float>((pEv->position().x() - PatternEditor::nMargin)) /
					m_fGridWidth));
		}

		m_pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn,
												pSelectedInstrument,
												pressedNoteKey,
												pressedOctave, false );

		// Store note-specific properties.
		storeNoteProperties( m_pDraggedNote );
		
		m_nRow = nRow;
	}
}

void PianoRollEditor::mouseDragUpdateEvent( QMouseEvent *ev )
{
	auto pEv = static_cast<MouseEvent*>( ev );

	int nRow = std::floor(static_cast<float>(pEv->position().y()) /
						  static_cast<float>(m_nGridHeight));
	if ( nRow >= (int) m_nOctaves * 12 ) {
		return;
	}

	PatternEditor::mouseDragUpdateEvent( ev );
}

void PianoRollEditor::addOrDeleteNoteAction( int nColumn,
											 int pressedLine,
											 int selectedPatternNumber,
											 int selectedinstrument,
											 int oldLength,
											 float oldVelocity,
											 float fOldPan,
											 float oldLeadLag,
											 int oldNoteKeyVal,
											 int oldOctaveKeyVal,
											 float fProbability,
											 bool noteOff,
											 bool isDelete )
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();

	auto pSelectedInstrument = pSong->getInstrumentList()->get( selectedinstrument );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Instrument [%1] could not be found" )
				  .arg( selectedinstrument ) );
		return;
	}

	Pattern *pPattern = nullptr;
	if ( ( selectedPatternNumber != -1 ) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}

	Note::Octave pressedoctave = Note::pitchToOctave( lineToPitch( pressedLine ) );
	Note::Key pressednotekey = Note::pitchToKey( lineToPitch( pressedLine ) );

	m_pAudioEngine->lock( RIGHT_HERE );	// lock the audio engine

	if ( isDelete ) {
		Note* note = m_pPattern->find_note( nColumn, -1, pSelectedInstrument, pressednotekey, pressedoctave );
		if ( note ) {
			// the note exists...remove it!
			m_pPattern->remove_note( note );
			delete note;
		} else {
			ERRORLOG( "Could not find note to delete" );
		}
	} else {
		// create the new note
		unsigned nPosition = nColumn;
		float fVelocity = oldVelocity;
		float fPan = fOldPan;
		int nLength = oldLength;

		if ( noteOff ) {
			fVelocity = 0.0f;
			fPan = 0.f;
			nLength = 1;
		}
		
		if ( pPattern != nullptr ) {
			Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan, nLength );
			pNote->set_note_off( noteOff );
			if( ! noteOff ) {
				pNote->set_lead_lag( oldLeadLag );
			}
			pNote->set_key_octave( pressednotekey, pressedoctave );
			pNote->set_probability( fProbability );
			pPattern->insert_note( pNote );
			if ( m_bSelectNewNotes ) {
				m_selection.addToSelection( pNote );
			}
		}
	}
	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock(); // unlock the audio engine

	m_pPatternEditorPanel->updateEditors( true );
}


// Find a note that matches pNote, and move it from (nColumn, nRow) to (nNewColumn, nNewRow)
void PianoRollEditor::moveNoteAction( int nColumn,
									  Note::Octave octave,
									  Note::Key key,
									  int nPattern,
									  int nNewColumn,
									  Note::Octave newOctave,
									  Note::Key newKey,
									  Note *pNote)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	m_pAudioEngine->lock( RIGHT_HERE );
	PatternList *pPatternList = pSong->getPatternList();
	Note *pFoundNote = nullptr;

	if ( nPattern < 0 || nPattern > pPatternList->size() ) {
		ERRORLOG( "Invalid pattern number" );
		m_pAudioEngine->unlock();
		return;
	}

	Pattern *pPattern = pPatternList->get( nPattern );

	FOREACH_NOTE_IT_BOUND_END((Pattern::notes_t *)pPattern->get_notes(), it, nColumn) {
		Note *pCandidateNote = it->second;
		if ( pCandidateNote->get_instrument() == pNote->get_instrument()
			 && pCandidateNote->get_octave() == octave
			 && pCandidateNote->get_key() == key
			 && pCandidateNote->get_velocity() == pNote->get_velocity()
			 && pCandidateNote->get_lead_lag() == pNote->get_lead_lag()
			 && pCandidateNote->getPan() == pNote->getPan()
			 && pCandidateNote->get_note_off() == pNote->get_note_off() ) {
			pFoundNote = pCandidateNote;
			if ( m_selection.isSelected( pFoundNote ) ) {
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

	// Remove and insert at new position
	pPattern->remove_note( pFoundNote );
	pFoundNote->set_position( nNewColumn );
	pPattern->insert_note( pFoundNote );
	pFoundNote->set_key_octave( newKey, newOctave );

	pHydrogen->setIsModified( true );
	m_pAudioEngine->unlock();

	m_pPatternEditorPanel->updateEditors( true );
}


QPoint PianoRollEditor::cursorPosition()
{
	uint x = PatternEditor::nMargin + m_pPatternEditorPanel->getCursorPosition() * m_fGridWidth;
	uint y = m_nGridHeight * pitchToLine( m_nCursorPitch ) + 1;
	return QPoint(x, y);
}

void PianoRollEditor::selectAll()
{
	selectInstrumentNotes( Hydrogen::get_instance()->getSelectedInstrumentNumber() );
}


void PianoRollEditor::deleteSelection()
{
	if ( m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	if ( m_selection.begin() != m_selection.end() ) {
		// Delete a selection.
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		int nSelectedInstrumentNumber = pHydrogen->getSelectedInstrumentNumber();
		auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
		if ( pSelectedInstrument == nullptr ) {
			DEBUGLOG( "No instrument selected" );
			return;
		}
		QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
		validateSelection();
		std::list< QUndoCommand * > actions;
		for ( Note *pNote : m_selection ) {
			if ( m_selection.isSelected( pNote ) ) {
				if ( pNote->get_instrument() == pSelectedInstrument ) {
					int nLine = pitchToLine( pNote->get_notekey_pitch() );
					actions.push_back( new SE_addOrDeleteNotePianoRollAction( pNote->get_position(),
																			  nLine,
																			  m_nSelectedPatternNumber,
																			  nSelectedInstrumentNumber,
																			  pNote->get_length(),
																			  pNote->get_velocity(),
																			  pNote->getPan(),
																			  pNote->get_lead_lag(),
																			  pNote->get_key(),
																			  pNote->get_octave(),
																			  pNote->get_probability(),
																			  true ) );
				}
			}
		}
		m_selection.clearSelection();

		pUndo->beginMacro("delete notes");
		for ( QUndoCommand * pAction : actions ) {
			pUndo->push( pAction );
		}
		pUndo->endMacro();
	}
}


///
/// Paste selection
///
/// Selection is XML containing notes, contained in a root 'note_selection' element.
///
void PianoRollEditor::paste()
{
	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		// No pattern selected.
		return;
	}

	QClipboard *clipboard = QApplication::clipboard();
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	auto pInstrList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	int nInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	XMLNode noteList;
	int nDeltaPos = 0, nDeltaPitch = 0;


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
		if ( !positionNode.isNull() ) {
			int nCurrentPos = m_pPatternEditorPanel->getCursorPosition();

			nDeltaPos = nCurrentPos -
				positionNode.read_int( "minColumn", nCurrentPos );
			nDeltaPitch = m_nCursorPitch -
				positionNode.read_int( "maxPitch", m_nCursorPitch );
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
			int nPitch = pNote->get_notekey_pitch() + nDeltaPitch;

			if ( nPos >= 0 && nPos < m_pPattern->get_length() && nPitch >= 12 * OCTAVE_MIN && nPitch < 12 * (OCTAVE_MAX+1) ) {
				int nLine = pitchToLine( nPitch );
				pUndo->push( new SE_addOrDeleteNotePianoRollAction( nPos,
																	nLine,
																	m_nSelectedPatternNumber,
																	nInstrument,
																	pNote->get_length(),
																	pNote->get_velocity(),
																	pNote->getPan(),
																	pNote->get_lead_lag(),
																	0,
																	0,
																	pNote->get_probability(),
																	false ) );
			}
			delete pNote;
		}
		pUndo->endMacro();
	}

	m_bSelectNewNotes = false;
}


void PianoRollEditor::keyPressEvent( QKeyEvent * ev )
{
	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
		
	const int nBlockSize = 5, nWordSize = 5;
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bUnhideCursor = true;
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Selection key, nothing more to do (other than update editor)
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
		if ( m_nCursorPitch > Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN ) ) {
			m_nCursorPitch --;
		}

	} else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) || ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		m_nCursorPitch = std::max( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN ),
								   m_nCursorPitch - nBlockSize );

	} else if ( ev->matches( QKeySequence::MoveToNextPage ) || ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down -- move down by a whole octave
		int nMinPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN );
		m_nCursorPitch -= 12;
		if ( m_nCursorPitch < nMinPitch ) {
			m_nCursorPitch = nMinPitch;
		}

	} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) || ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		m_nCursorPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN );

	} else if ( ev->matches( QKeySequence::MoveToPreviousLine ) || ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( m_nCursorPitch < Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX ) ) {
			m_nCursorPitch ++;
		}

	} else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) || ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		m_nCursorPitch = std::min( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX ),
								   m_nCursorPitch + nBlockSize );

	} else if ( ev->matches( QKeySequence::MoveToPreviousPage ) || ev->matches( QKeySequence::SelectPreviousPage ) ) {
		int nMaxPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX );
		m_nCursorPitch += 12;
		if ( m_nCursorPitch >= nMaxPitch ) {
			m_nCursorPitch = nMaxPitch;
		}

	} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) || ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		m_nCursorPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX );

	} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter/Return : Place or remove note at current position
		int pressedline = pitchToLine( m_nCursorPitch );
		int nPitch = lineToPitch( pressedline );
		addOrRemoveNote( m_pPatternEditorPanel->getCursorPosition(), -1, pressedline,
						 Note::pitchToKey( nPitch ), Note::pitchToOctave( nPitch ) );

	} else if ( ev->matches( QKeySequence::SelectAll ) ) {
		// Key: Ctrl + A: Select all
		bUnhideCursor = false;
		selectAll();

	} else if ( ev->matches( QKeySequence::Deselect ) ) {
		// Key: Shift + Ctrl + A: clear selection
		bUnhideCursor = false;
		selectNone();

	} else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete: delete selection or note at keyboard cursor
		bUnhideCursor = false;
		if ( m_selection.begin() != m_selection.end() ) {
			deleteSelection();
		} else {
			// Delete a note under the keyboard cursor
			int pressedline = pitchToLine( m_nCursorPitch );
			int nPitch = lineToPitch( pressedline );
			addOrRemoveNote( m_pPatternEditorPanel->getCursorPosition(), -1, pressedline,
							 Note::pitchToKey( nPitch ), Note::pitchToOctave( nPitch ),
							 /*bDoAdd=*/false, /*bDoDelete=*/true );
		}

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
			m_pPatternEditorPanel->getPatternEditorRuler()->update();
			update();
		}
		return;
	}

	// Update editor
	QPoint pos = cursorPosition();
	if ( bUnhideCursor ) {
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
	}
	m_pScrollView->ensureVisible( pos.x(), pos.y() );
	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );

	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}
	
	updateEditor( true );
	ev->accept();
}

// Selection manager interface
void PianoRollEditor::selectionMoveEndEvent( QInputEvent *ev )
{
	updateModifiers( ev );

	QPoint offset = movingGridOffset();
	if ( offset.x() == 0 && offset.y() == 0 ) {
		// Move with no effect.
		return;
	}

	validateSelection();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	int nSelectedInstrumentNumber = pHydrogen->getSelectedInstrumentNumber();

	if ( m_pPattern == nullptr || nSelectedPatternNumber == -1 ) {
		// No pattern selected. Nothing to be selected.
		return;
	}

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
		int nPosition = pNote->get_position();
		int nNewPosition = nPosition + offset.x();

		Note::Octave octave = pNote->get_octave();
		Note::Key key = pNote->get_key();
		// Transpose note
		int nNewPitch = pNote->get_notekey_pitch() - offset.y();
		int nLine = pitchToLine( nNewPitch );
		Note::Octave newOctave = Note::pitchToOctave( nNewPitch );
		Note::Key newKey = Note::pitchToKey( nNewPitch );
		bool bNoteInRange = ( newOctave >= OCTAVE_MIN && newOctave <= OCTAVE_MAX && nNewPosition >= 0
							  && nNewPosition < m_pPattern->get_length() );

		if ( m_bCopyNotMove ) {
			if ( bNoteInRange ) {
				pUndo->push( new SE_addOrDeleteNotePianoRollAction( nNewPosition,
																	nLine,
																	nSelectedPatternNumber,
																	nSelectedInstrumentNumber,
																	pNote->get_length(),
																	pNote->get_velocity(),
																	pNote->getPan(),
																	pNote->get_lead_lag(),
																	newKey,
																	newOctave,
																	pNote->get_probability(),
																	false ) );
			}
		} else {
			if ( bNoteInRange ) {
				pUndo->push( new SE_moveNotePianoRollAction( nPosition, octave, key, nSelectedPatternNumber, nNewPosition, newOctave, newKey, pNote ) );
			} else {
				pUndo->push( new SE_addOrDeleteNotePianoRollAction( pNote->get_position(),
																	nLine - offset.y(),
																	nSelectedPatternNumber,
																	nSelectedInstrumentNumber,
																	pNote->get_length(),
																	pNote->get_velocity(),
																	pNote->getPan(),
																	pNote->get_lead_lag(),
																	key,
																	octave,
																	pNote->get_probability(),
																	true ) );
			}
		}
	}

	m_bSelectNewNotes = false;
	pUndo->endMacro();
}

std::vector<PianoRollEditor::SelectionIndex> PianoRollEditor::elementsIntersecting( QRect r ) 
{
	std::vector<SelectionIndex> result;
	if ( m_pPattern == nullptr ) {
		return std::move( result );
	}
	
	int w = 8;
	int h = m_nGridHeight - 2;
	auto pSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		DEBUGLOG( "No instrument selected" );
		return std::move( result );
	}

	r = r.normalized();
	if ( r.top() == r.bottom() && r.left() == r.right() ) {
		r += QMargins( 2, 2, 2, 2 );
	}

	// Calculate the first and last position values that this rect will intersect with
	int x_min = (r.left() - w - PatternEditor::nMargin) / m_fGridWidth;
	int x_max = (r.right() + w - PatternEditor::nMargin) / m_fGridWidth;

	const Pattern::notes_t* pNotes = m_pPattern->get_notes();

	for ( auto it = pNotes->lower_bound( x_min ); it != pNotes->end() && it->first <= x_max; ++it ) {
		Note *pNote = it->second;
		if ( pNote->get_instrument() == pSelectedInstrument ) {
			uint start_x = PatternEditor::nMargin + pNote->get_position() * m_fGridWidth;
			uint start_y = m_nGridHeight * pitchToLine( pNote->get_notekey_pitch() ) + 1;

			if ( r.intersects( QRect( start_x -4 , start_y, w, h ) ) ) {
				result.push_back( pNote );
			}
		}
	}
	updateEditor( true );
	return std::move( result );
}

///
/// Position of keyboard input cursor on screen
///
QRect PianoRollEditor::getKeyboardCursorRect()
{
	const QPoint pos = cursorPosition();
	float fHalfWidth;
	if ( m_nResolution != MAX_NOTES ) {
		// Corresponds to the distance between grid lines on 1/64 resolution.
		fHalfWidth = m_fGridWidth * 3;
	} else {
		// Corresponds to the distance between grid lines set to resolution
		// "off".
		fHalfWidth = m_fGridWidth;
	}
	return QRect( pos.x() - fHalfWidth, pos.y()-2,
				  fHalfWidth * 2, m_nGridHeight+3 );
}

void PianoRollEditor::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		invalidateBackground();
		update();
	}
}
