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

#include "PianoRollEditor.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"
#include "UndoActions.h"
#include <cassert>

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Preferences.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine.h>
#include <core/Helpers/Xml.h>
using namespace H2Core;

#include "../HydrogenApp.h"


const char* PianoRollEditor::__class_name = "PianoRollEditor";

PianoRollEditor::PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel,
								  QScrollArea *pScrollView)
	: QWidget( pParent )
	, Object( __class_name )
	, m_nResolution( 8 )
	, m_bUseTriplets( false )
	, m_pPattern( nullptr )
	, m_pPatternEditorPanel( panel )
	, m_pDraggedNote( nullptr )
	, m_pScrollView( pScrollView )
	, m_selection( this )
{
	INFOLOG( "INIT" );

	m_nRowHeight = 10;
	m_nOctaves = 7;

	setAttribute(Qt::WA_NoBackground);
	setFocusPolicy(Qt::ClickFocus);
	m_nGridWidth = Preferences::get_instance()->getPatternEditorGridWidth();

	m_nEditorWidth = 20 + m_nGridWidth *  (MAX_NOTES * 4);
	m_nEditorHeight = m_nOctaves * 12 * m_nRowHeight;

	m_pBackground = new QPixmap( m_nEditorWidth, m_nEditorHeight );
	m_pTemp = new QPixmap( m_nEditorWidth, m_nEditorHeight );

	m_nCursorPitch = 0;

	resize( m_nEditorWidth, m_nEditorHeight );
	
	createBackground();

	HydrogenApp::get_instance()->addEventListener( this );

	m_bNeedsUpdate = true;
	m_bSelectNewNotes = false;

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, &PianoRollEditor::cut );
	m_pPopupMenu->addAction( tr( "&Copy" ), this, &PianoRollEditor::copy );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, &PianoRollEditor::paste );
	m_pPopupMenu->addAction( tr( "&Delete" ), this, &PianoRollEditor::deleteSelection );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &PianoRollEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &PianoRollEditor::selectNone );

}



PianoRollEditor::~PianoRollEditor()
{
	INFOLOG( "DESTROY" );
}


void PianoRollEditor::setResolution(uint res, bool bUseTriplets)
{
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;
	updateEditor();
}


void PianoRollEditor::updateEditor( bool bPatternOnly )
{
	//	uint nEditorWidth;
	if ( m_pPattern ) {
		m_nEditorWidth = 20 + m_nGridWidth * m_pPattern->get_length();
	}
	else {
		m_nEditorWidth = 20 + m_nGridWidth * MAX_NOTES;
	}
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	PatternList *pPatternList = pHydrogen->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}

	if ( m_bNeedsBackgroundUpdate ) {
		createBackground();
	}
	drawPattern();
	//	ERRORLOG(QString("update editor %1").arg(m_nEditorWidth));
	m_bNeedsUpdate = false;
	m_bNeedsBackgroundUpdate = false;
}



//eventlistener
void PianoRollEditor::patternModifiedEvent()
{
	updateEditor();
}



void PianoRollEditor::selectedInstrumentChangedEvent()
{
	// Update pattern only
	updateEditor( true );
}


void PianoRollEditor::selectedPatternChangedEvent()
{
	//INFOLOG( "updating m_pPattern pointer" );

	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}
	__selectedPatternNumber = nSelectedPatternNumber;
	updateEditor();
}



void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	if ( m_bNeedsUpdate ) {
		finishUpdateEditor();
	}
	painter.drawPixmap( ev->rect(), *m_pTemp, ev->rect() );
	m_selection.paintSelection( &painter );
}



void PianoRollEditor::createBackground()
{
	//INFOLOG( "(re)creating the background" );

	QColor backgroundColor( 250, 250, 250 );
	m_pBackground->fill( backgroundColor );


	QColor octaveColor( 230, 230, 230 );
	QColor octaveAlternateColor( 200, 200, 200 );
	QColor baseOctaveColor( 245, 245, 245 );
	QColor baseNoteColor( 255, 255, 255 );

	QColor fbk( 160, 160, 160 );

	unsigned start_x = 0;
	unsigned end_x = width();

	QPainter p( m_pBackground );

	for ( uint octave = 0; octave < m_nOctaves; ++octave ) {
		unsigned start_y = octave * 12 * m_nRowHeight;

		if ( octave % 2 ) {


			if ( octave == 3 ){

				//				p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y, end_x - start_x, start_y + 1 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y + 1 * m_nRowHeight, end_x - start_x, start_y + 2 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 2 * m_nRowHeight, end_x - start_x, start_y + 3 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y + 3 * m_nRowHeight, end_x - start_x, start_y + 4 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 4 * m_nRowHeight, end_x - start_x, start_y + 5 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y + 5 * m_nRowHeight, end_x - start_x, start_y + 6 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 6 * m_nRowHeight, end_x - start_x, start_y + 7 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y + 7 * m_nRowHeight, end_x - start_x, start_y + 8 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y + 8 * m_nRowHeight, end_x - start_x, start_y + 9 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 9 * m_nRowHeight, end_x - start_x, start_y + 10 * m_nRowHeight, baseOctaveColor );
				p.fillRect( start_x, start_y + 10 * m_nRowHeight, end_x - start_x, start_y + 11 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 11 * m_nRowHeight, end_x - start_x, start_y + 12 * m_nRowHeight, baseNoteColor );
			}
			else
			{
				//	p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y, end_x - start_x, start_y + 1 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y + 1 * m_nRowHeight, end_x - start_x, start_y + 2 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 2 * m_nRowHeight, end_x - start_x, start_y + 3 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y + 3 * m_nRowHeight, end_x - start_x, start_y + 4 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 4 * m_nRowHeight, end_x - start_x, start_y + 5 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y + 5 * m_nRowHeight, end_x - start_x, start_y + 6 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 6 * m_nRowHeight, end_x - start_x, start_y + 7 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y + 7 * m_nRowHeight, end_x - start_x, start_y + 8 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y + 8 * m_nRowHeight, end_x - start_x, start_y + 9 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 9 * m_nRowHeight, end_x - start_x, start_y + 10 * m_nRowHeight, octaveColor );
				p.fillRect( start_x, start_y + 10 * m_nRowHeight, end_x - start_x, start_y + 11 * m_nRowHeight, fbk );
				p.fillRect( start_x, start_y + 11 * m_nRowHeight, end_x - start_x, start_y + 12 * m_nRowHeight, octaveColor );

			}
		}
		else {
			//			p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y, end_x - start_x, start_y + 1 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y + 1 * m_nRowHeight, end_x - start_x, start_y + 2 * m_nRowHeight, fbk );
			p.fillRect( start_x, start_y + 2 * m_nRowHeight, end_x - start_x, start_y + 3 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y + 3 * m_nRowHeight, end_x - start_x, start_y + 4 * m_nRowHeight, fbk );
			p.fillRect( start_x, start_y + 4 * m_nRowHeight, end_x - start_x, start_y + 5 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y + 5 * m_nRowHeight, end_x - start_x, start_y + 6 * m_nRowHeight, fbk );
			p.fillRect( start_x, start_y + 6 * m_nRowHeight, end_x - start_x, start_y + 7 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y + 7 * m_nRowHeight, end_x - start_x, start_y + 8 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y + 8 * m_nRowHeight, end_x - start_x, start_y + 9 * m_nRowHeight, fbk );
			p.fillRect( start_x, start_y + 9 * m_nRowHeight, end_x - start_x, start_y + 10 * m_nRowHeight, octaveAlternateColor );
			p.fillRect( start_x, start_y + 10 * m_nRowHeight, end_x - start_x, start_y + 11 * m_nRowHeight, fbk );
			p.fillRect( start_x, start_y + 11 * m_nRowHeight, end_x - start_x, start_y + 12 * m_nRowHeight, octaveAlternateColor );
			
		}
	}


	// horiz lines
	for ( uint row = 0; row < ( 12 * m_nOctaves ); ++row ) {
		unsigned y = row * m_nRowHeight;
		p.drawLine( start_x, y,end_x , y );
	}

	//draw text
	QFont font;
	font.setPointSize ( 9 );
	//	font.setWeight( 63 );
	p.setFont( font );
	p.setPen( QColor(10, 10, 10 ) );

	int offset = 0;
	int insertx = 3;
	for ( int oct = 0; oct < (int)m_nOctaves; oct++ ){
		if( oct > 3 ){
			p.drawText( insertx, m_nRowHeight  + offset, "B" );
			p.drawText( insertx, 10 + m_nRowHeight  + offset, "A#" );
			p.drawText( insertx, 20 + m_nRowHeight  + offset, "A" );
			p.drawText( insertx, 30 + m_nRowHeight  + offset, "G#" );
			p.drawText( insertx, 40 + m_nRowHeight  + offset, "G" );
			p.drawText( insertx, 50 + m_nRowHeight  + offset, "F#" );
			p.drawText( insertx, 60 + m_nRowHeight  + offset, "F" );
			p.drawText( insertx, 70 + m_nRowHeight  + offset, "E" );
			p.drawText( insertx, 80 + m_nRowHeight  + offset, "D#" );
			p.drawText( insertx, 90 + m_nRowHeight  + offset, "D" );
			p.drawText( insertx, 100 + m_nRowHeight  + offset, "C#" );
			p.drawText( insertx, 110 + m_nRowHeight  + offset, "C" );
			offset += 12 * m_nRowHeight;
		}else
		{
			p.drawText( insertx, m_nRowHeight  + offset, "b" );
			p.drawText( insertx, 10 + m_nRowHeight  + offset, "a#" );
			p.drawText( insertx, 20 + m_nRowHeight  + offset, "a" );
			p.drawText( insertx, 30 + m_nRowHeight  + offset, "g#" );
			p.drawText( insertx, 40 + m_nRowHeight  + offset, "g" );
			p.drawText( insertx, 50 + m_nRowHeight  + offset, "f#" );
			p.drawText( insertx, 60 + m_nRowHeight  + offset, "f" );
			p.drawText( insertx, 70 + m_nRowHeight  + offset, "e" );
			p.drawText( insertx, 80 + m_nRowHeight  + offset, "d#" );
			p.drawText( insertx, 90 + m_nRowHeight  + offset, "d" );
			p.drawText( insertx, 100 + m_nRowHeight  + offset, "c#" );
			p.drawText( insertx, 110 + m_nRowHeight  + offset, "c" );
			offset += 12 * m_nRowHeight;
		}
	}

	draw_grid( p );
}




void PianoRollEditor::draw_grid( QPainter& p )
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	static const QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	static const QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	static const QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	static const QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	// vertical lines

	int nBase;
	if (m_bUseTriplets) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}

	int n4th = 4 * MAX_NOTES / (nBase * 4);
	int n8th = 4 * MAX_NOTES / (nBase * 8);
	int n16th = 4 * MAX_NOTES / (nBase * 16);
	int n32th = 4 * MAX_NOTES / (nBase * 32);
	int n64th = 4 * MAX_NOTES / (nBase * 64);

	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}
	if (!m_bUseTriplets) {
		for ( int i = 0; i < nNotes + 1; i++ ) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (m_nResolution >= 4) {
					p.setPen( QPen( res_1, 1, Qt::DashLine) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (m_nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DashLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (m_nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DashLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (m_nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DashLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (m_nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DashLine  ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
		}
	}
	else {	// Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * m_nResolution);

		for ( int i = 0; i < nNotes + 1; i++ ) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DashLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DashLine ) );
				}
				p.drawLine(x, 1, x, m_nEditorHeight - 1);
				nCounter++;
			}
		}
	}
}


void PianoRollEditor::drawPattern()
{
	if ( isVisible() == false ) {
		return;
	}

	//INFOLOG( "draw pattern" );

	validateSelection();

	QPainter p( m_pTemp );
	// copy the background image
	p.drawPixmap( rect(), *m_pBackground, rect() );


	// for each note...
	const Pattern::notes_t* notes = m_pPattern->get_notes();
	FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
		//cout << "note" << endl;
		//cout << "note n: " << it->first << endl;
		Note *note = it->second;
		assert( note );
		drawNote( note, &p );
	}

	// Draw cursor
	if ( hasFocus() && !m_pPatternEditorPanel->cursorHidden() ) {
		QPoint pos = cursorPosition();

		p.setPen( QColor(0,0,0) );
		p.setBrush( Qt::NoBrush );
		p.setRenderHint( QPainter::Antialiasing );
		p.drawRoundedRect( QRect( pos.x() - m_nGridWidth*3, pos.y(),
								  m_nGridWidth*6, m_nRowHeight ), 4, 4 );
	}

}


void PianoRollEditor::drawNote( Note *pNote, QPainter *pPainter )
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor noteColor( pStyle->m_patternEditor_noteColor.getRed(), pStyle->m_patternEditor_noteColor.getGreen(), pStyle->m_patternEditor_noteColor.getBlue() );
	static const QColor noteoffColor( pStyle->m_patternEditor_noteoffColor.getRed(), pStyle->m_patternEditor_noteoffColor.getGreen(), pStyle->m_patternEditor_noteoffColor.getBlue() );

	int nInstrument = -1;
	InstrumentList * pInstrList = Hydrogen::get_instance()->getSong()->get_instrument_list();
	for ( uint nInstr = 0; nInstr < pInstrList->size(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr == pNote->get_instrument() ) {
			nInstrument = nInstr;
			break;
		}
	}
	if ( nInstrument == -1 ) {
		//ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	if ( nInstrument != Hydrogen::get_instance()->getSelectedInstrumentNumber() ) {
		return;
	}

	uint start_x = 20 + pNote->get_position() * m_nGridWidth;
	uint start_y = m_nRowHeight * pitchToLine( pNote->get_notekey_pitch() ) + 1;
	uint w = 8;
	uint h = m_nRowHeight - 2;

	QColor color = m_pPatternEditorPanel->getDrumPatternEditor()->computeNoteColor( pNote->get_velocity() );

	bool bSelected = m_selection.isSelected( pNote );
	QPen selectedPen;
	if ( hasFocus() ) {
		static const QColor selectHilightColor( pStyle->m_selectionHighlightColor.getRed(),
												pStyle->m_selectionHighlightColor.getGreen(),
												pStyle->m_selectionHighlightColor.getBlue() );
		selectedPen = selectHilightColor;
	} else {
		static const QColor selectInactiveColor( pStyle->m_selectionInactiveColor.getRed(),
												 pStyle->m_selectionInactiveColor.getGreen(),
												 pStyle->m_selectionInactiveColor.getBlue() );
		selectedPen = selectInactiveColor;
	}

	if ( bSelected ) {
		selectedPen.setWidth( 2 );
		pPainter->setPen( selectedPen );
		pPainter->setBrush( Qt::NoBrush );
	}

	bool bMoving = bSelected && m_selection.isMoving();
	QPen movingPen( noteColor );
	QPoint movingOffset;

	if ( bMoving ) {
		movingPen.setStyle( Qt::DotLine );
		movingPen.setWidth( 2 );
		QPoint delta = movingGridOffset();
		movingOffset = QPoint( delta.x() * m_nGridWidth,
							   delta.y() * m_nRowHeight );
	}

	pPainter->setRenderHint( QPainter::Antialiasing );

	if ( pNote->get_length() == -1 && pNote->get_note_off() == false ) {
		if ( bSelected ) {
			pPainter->drawEllipse( start_x -4 -2 , start_y -2, w+4, h+4 );
		}
		pPainter->setPen( noteColor );
		pPainter->setBrush( color );
		pPainter->drawEllipse( start_x -4 , start_y, w, h );
		if ( bMoving ) {
			pPainter->setPen( movingPen );
			pPainter->setBrush( Qt::NoBrush );
			pPainter->drawEllipse( start_x -4 -2 + movingOffset.x(), start_y -2 + movingOffset.y(), w+4, h+4 );
		}
	}
	else if ( pNote->get_length() == 1 && pNote->get_note_off() == true ){
		if ( bSelected ) {
			pPainter->drawEllipse( start_x -4 -2 , start_y -2, w+4, h+4 );
		}
		pPainter->setPen( noteoffColor );
		pPainter->setBrush( noteoffColor );
		pPainter->drawEllipse( start_x -4 , start_y, w, h );
		if ( bMoving ) {
			pPainter->setPen( movingPen );
			pPainter->setBrush( Qt::NoBrush );
			pPainter->drawEllipse( start_x -4 -2 + movingOffset.x(), start_y -2 + movingOffset.y(), w+4, h+4 );
		}
	}
	else {
		float fNotePitch = pNote->get_notekey_pitch();
		float fStep = pow( 1.0594630943593, ( double )fNotePitch );

		int nend = m_nGridWidth * pNote->get_length() / fStep;
		nend = nend - 1;	// lascio un piccolo spazio tra una nota ed un altra
		if ( bSelected ) {
			pPainter->drawRoundedRect( start_x-2, start_y-2, nend+4, h+4, 4, 4 );
		}
		pPainter->setPen( noteColor );
		pPainter->setBrush( color );
		pPainter->fillRect( start_x, start_y, nend, h, color );
		pPainter->drawRect( start_x, start_y, nend, h );
		if ( bMoving ) {
			pPainter->setPen( movingPen );
			pPainter->setBrush( Qt::NoBrush );
			pPainter->drawRoundedRect( start_x-2 + movingOffset.x(), start_y -2 + movingOffset.y(), nend+4, h+4, 4, 4 );
		}

	}
}


int PianoRollEditor::getColumn(QMouseEvent *ev)
{
	int nBase;
	if (m_bUseTriplets) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nWidth = (m_nGridWidth * 4 * MAX_NOTES) / (nBase * m_nResolution);

	int x = ev->x();
	int nColumn;
	nColumn = x - 20 + (nWidth / 2);
	nColumn = nColumn / nWidth;
	nColumn = (nColumn * 4 * MAX_NOTES) / (nBase * m_nResolution);
	return nColumn;
}


void PianoRollEditor::addOrRemoveNote( int nColumn, int nRealColumn, int nLine,
									   int nNotekey, int nOctave )
{
	Note::Octave octave = (Note::Octave)nOctave;
	Note::Key notekey = (Note::Key)nNotekey;
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nSelectedInstrumentnumber = pHydrogen->getSelectedInstrumentNumber();
	Song *pSong = pHydrogen->getSong();
	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( nSelectedInstrumentnumber );

	Note* pFoundNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument,
											  notekey, octave );

	int nLength = -1;
	float fVelocity = 0.8f;
	float fPan_L = 0.5f;
	float fPan_R = 0.5f;
	float fLeadLag = 0.0f;

	if ( pFoundNote ) {
		nLength = pFoundNote->get_length();
		fVelocity = pFoundNote->get_velocity();
		fPan_L = pFoundNote->get_pan_l();
		fPan_R = pFoundNote->get_pan_r();
		fLeadLag = pFoundNote->get_lead_lag();
		notekey = pFoundNote->get_key();
		octave = pFoundNote->get_octave();
	}

	if ( pFoundNote == nullptr ) {
		// hear note
		Preferences *pref = Preferences::get_instance();
		if ( pref->getHearNewNotes() ) {
			Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan_L, fPan_R, nLength, 0.0 );
			pNote2->set_key_octave( notekey, octave );
			AudioEngine::get_instance()->get_sampler()->note_on( pNote2 );
		}
	}

	SE_addOrDeleteNotePianoRollAction *action = new SE_addOrDeleteNotePianoRollAction( nColumn,
																					   nLine,
																					   __selectedPatternNumber,
																					   nSelectedInstrumentnumber,
																					   nLength,
																					   fVelocity,
																					   fPan_L,
																					   fPan_R,
																					   fLeadLag,
																					   notekey,
																					   octave,
																					   pFoundNote != nullptr );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );

}


void PianoRollEditor::mousePressEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mousePressEvent( ev );
}

void PianoRollEditor::mouseMoveEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	if ( m_selection.isMoving() ) {
		updateEditor( true );
	}
	m_selection.mouseMoveEvent( ev );
}

void PianoRollEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mouseReleaseEvent( ev );
}

void PianoRollEditor::mouseClickEvent( QMouseEvent *ev ) {
	//ERRORLOG("Mouse press event");
	if ( m_pPattern == nullptr ) {
		return;
	}

	Song *pSong = Hydrogen::get_instance()->getSong();

	int row = ((int) ev->y()) / ((int) m_nEditorHeight);
	if (row >= (int) m_nOctaves * 12 ) {
		return;
	}

	int nColumn = getColumn( ev );

	if ( nColumn >= (int)m_pPattern->get_length() ) {
		update( 0, 0, width(), height() );
		return;
	}
	m_pPatternEditorPanel->setCursorPosition( nColumn );
	m_pPatternEditorPanel->setCursorHidden( true );
	
	int pressedline = ((int) ev->y()) / ((int) m_nRowHeight);

	Instrument *pSelectedInstrument = nullptr;
	int nSelectedInstrumentnumber = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	pSelectedInstrument = pSong->get_instrument_list()->get( nSelectedInstrumentnumber );
	assert(pSelectedInstrument);

	//ERRORLOG(QString("pressedline: %1, column %2, event ev: %3, editorhight %4").arg(pressedline).arg(nColumn).arg(ev->y()).arg(m_nEditorHeight));

	int nPitch = lineToPitch( pressedline );
	Note::Octave pressedoctave = pitchToOctave( nPitch );
	Note::Key pressednotekey = pitchToKey( nPitch );
	m_nCursorPitch = nPitch;
	
	//ERRORLOG(QString("pressedline: %1, octave %2, notekey: %3").arg(pressedline).arg(pressedoctave).arg(pressednotekey));

	if (ev->button() == Qt::LeftButton ) {

		unsigned nRealColumn = 0;
		if( ev->x() > 20 ) {
			nRealColumn = (ev->x() - 20) / static_cast<float>(m_nGridWidth);
		}

		if ( ev->modifiers() & Qt::ShiftModifier ) {
			H2Core::Note *pNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave );
			if ( pNote != nullptr ) {
				SE_addOrDeleteNotePianoRollAction *action = new SE_addOrDeleteNotePianoRollAction( nColumn,
																								   pressedline,
																								   __selectedPatternNumber,
																								   nSelectedInstrumentnumber,
																								   pNote->get_length(),
																								   pNote->get_velocity(),
																								   pNote->get_pan_l(),
																								   pNote->get_pan_r(),
																								   pNote->get_lead_lag(),
																								   pNote->get_key(),
																								   pNote->get_octave(),
																								   pNote != nullptr );
				HydrogenApp::get_instance()->m_pUndoStack->push( action );
			} else {
				SE_addPianoRollNoteOffAction *action = new SE_addPianoRollNoteOffAction( nColumn, pressedline, __selectedPatternNumber, nSelectedInstrumentnumber );
				HydrogenApp::get_instance()->m_pUndoStack->push( action );
			}
			return;
		}

		addOrRemoveNote( nColumn, nRealColumn, pressedline, pressednotekey, pressedoctave );

	} else if ( ev->button() == Qt::RightButton ) {
		// Show context menu
		m_pPopupMenu->popup( ev->globalPos() );

	}

}

void PianoRollEditor::mouseDragStartEvent( QMouseEvent *ev )
{
	m_pDraggedNote = nullptr;
	Hydrogen *pH2 = Hydrogen::get_instance();
	int nColumn = getColumn( ev );
	Song *pSong = pH2->getSong();
	int nSelectedInstrumentnumber = pH2->getSelectedInstrumentNumber();
	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( nSelectedInstrumentnumber );
	m_pPatternEditorPanel->setCursorPosition( nColumn );
	m_pPatternEditorPanel->setCursorHidden( true );

	int pressedline = ((int) ev->y()) / ((int) m_nRowHeight);

	//ERRORLOG(QString("pressedline: %1, column %2, event ev: %3, editorhight %4").arg(pressedline).arg(nColumn).arg(ev->y()).arg(m_nEditorHeight));

	Note::Octave pressedoctave = pitchToOctave( lineToPitch( pressedline ) );
	Note::Key pressednotekey = pitchToKey( lineToPitch( pressedline ) );
	m_nCursorPitch = lineToPitch( pressedline );

	if (ev->button() == Qt::RightButton ) {
		m_pOldPoint = ev->y();

		unsigned nRealColumn = 0;
		if( ev->x() > 20 ) {
			nRealColumn = (ev->x() - 20) / static_cast<float>(m_nGridWidth);
		}


		//		AudioEngine::get_instance()->lock( RIGHT_HERE );

		m_pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave, false );

		//needed for undo note length
		__nRealColumn = nRealColumn;
		__nColumn = nColumn;
		__pressedLine = pressedline;
		__selectedInstrumentnumber = nSelectedInstrumentnumber;
		if( m_pDraggedNote ){
			__oldLength = m_pDraggedNote->get_length();
			//needed to undo note properties
			__oldVelocity = m_pDraggedNote->get_velocity();
			__oldPan_L = m_pDraggedNote->get_pan_l();
			__oldPan_R = m_pDraggedNote->get_pan_r();
			__oldLeadLag = m_pDraggedNote->get_lead_lag();

			__velocity = __oldVelocity;
			__pan_L = __oldPan_L;
			__pan_R = __oldPan_R;
			__leadLag = __oldLeadLag;
		}else
		{
			__oldLength = -1;
		}
		//		AudioEngine::get_instance()->unlock();
	}
}


void PianoRollEditor::addOrDeleteNoteAction( int nColumn,
											 int pressedLine,
											 int selectedPatternNumber,
											 int selectedinstrument,
											 int oldLength,
											 float oldVelocity,
											 float oldPan_L,
											 float oldPan_R,
											 float oldLeadLag,
											 int oldNoteKeyVal,
											 int oldOctaveKeyVal,
											 bool noteOff,
											 bool isDelete )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();

	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( selectedinstrument );
	assert(pSelectedInstrument);

	Pattern *pPattern = nullptr;
	if ( ( selectedPatternNumber != -1 ) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}

	Note::Octave pressedoctave = pitchToOctave( lineToPitch( pressedLine ) );
	Note::Key pressednotekey = pitchToKey( lineToPitch( pressedLine ) );

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

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
		float fPan_L = oldPan_L;
		float fPan_R = oldPan_R;
		int nLength = oldLength;
		float fPitch = 0.0f;

		if ( noteOff ) {
			fVelocity = 0.0f;
			fPan_L = 0.5f;
			fPan_R = 0.5f;
			nLength = 1;
			fPitch = 0.0f;
		}

		if( pPattern ) {
			Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch );
			pNote->set_note_off( noteOff );
			if(! noteOff) pNote->set_lead_lag( oldLeadLag );
			pNote->set_key_octave( pressednotekey, pressedoctave );
			pPattern->insert_note( pNote );
			if ( m_bSelectNewNotes ) {
				m_selection.addToSelection( pNote );
			}
		}
	}
	pSong->set_is_modified( true );
	AudioEngine::get_instance()->unlock(); // unlock the audio engine

	updateEditor( true );
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}

void PianoRollEditor::updateModifiers( QInputEvent *ev ) {
	// Key: Alt + drag: move notes with fine-grained positioning
	m_bFineGrained = ev->modifiers() & Qt::AltModifier;
	// Key: Ctrl + drag: copy notes rather than moving
	m_bCopyNotMove = ev->modifiers() & Qt::ControlModifier;

	if ( m_selection.isMoving() ) {
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


QPoint PianoRollEditor::movingGridOffset( ) {
	QPoint rawOffset = m_selection.movingOffset();
	// Quantize offset to multiples of m_nGrid{Width,Height}
	int nQuantX = m_nGridWidth, nQuantY = m_nRowHeight;
	float nFactor = 1;
	if ( ! m_bFineGrained ) {
		int nBase = m_bUseTriplets ? 3 : 4;
		nFactor = (4 * MAX_NOTES) / (nBase * m_nResolution);
		nQuantX = m_nGridWidth * nFactor;
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
	Song *pSong = pHydrogen->getSong();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	PatternList *pPatternList = pSong->get_pattern_list();
	Note *pFoundNote = nullptr;

	if ( nPattern < 0 || nPattern > pPatternList->size() ) {
		ERRORLOG( "Invalid pattern number" );
		AudioEngine::get_instance()->unlock();
		return;
	}

	Pattern *pPattern = pPatternList->get( nPattern );

	FOREACH_NOTE_IT_BOUND((Pattern::notes_t *)pPattern->get_notes(), it, nColumn) {
		Note *pCandidateNote = it->second;
		if ( pCandidateNote->get_instrument() == pNote->get_instrument()
			 && pCandidateNote->get_octave() == octave
			 && pCandidateNote->get_key() == key
			 && pCandidateNote->get_velocity() == pNote->get_velocity()
			 && pCandidateNote->get_lead_lag() == pNote->get_lead_lag()
			 && pCandidateNote->get_pan_r() == pNote->get_pan_r()
			 && pCandidateNote->get_pan_l() == pNote->get_pan_r()
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
		AudioEngine::get_instance()->unlock();
		return;
	}

	// Remove and insert at new position
	pPattern->remove_note( pFoundNote );
	pFoundNote->set_position( nNewColumn );
	pPattern->insert_note( pFoundNote );
	pFoundNote->set_key_octave( newKey, newOctave );

	AudioEngine::get_instance()->unlock();

	updateEditor( true );
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}




void PianoRollEditor::mouseDragUpdateEvent( QMouseEvent *ev )
{
	if (m_pPattern == nullptr) {
		return;
	}

	int row = ((int) ev->y()) / ((int) m_nEditorHeight);
	if (row >= (int) m_nOctaves * 12 ) {
		return;
	}

	if ( m_pDraggedNote ) {
		if ( m_pDraggedNote->get_note_off() ) return;
		int nTickColumn = getColumn( ev );

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->get_position();

		if (nLen <= 0) {
			nLen = -1;
		}

		float fNotePitch = m_pDraggedNote->get_notekey_pitch();
		float fStep = 0;
		if(nLen > -1){
			fStep = pow( 1.0594630943593, ( double )fNotePitch );
		} else {
			fStep = 1.0;
		}
		m_pDraggedNote->set_length( nLen * fStep);

		Hydrogen::get_instance()->getSong()->set_is_modified( true );
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		updateEditor( true );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	}

	int selectedProperty = m_pPatternEditorPanel->getPropertiesComboValue();

	//edit velocity
	if ( m_pDraggedNote && selectedProperty == 0 ) { // Velocity
		if ( m_pDraggedNote->get_note_off() ) return;

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

		float val = m_pDraggedNote->get_velocity();

		
		float ymove = m_pOldPoint - ev->y();
		val = val  +  (ymove / 100);
		if (val > 1) {
			val = 1;
		}
		else if (val < 0.0) {
			val = 0.0;
		}

		m_pDraggedNote->set_velocity( val );

		__velocity = val;

		Hydrogen::get_instance()->getSong()->set_is_modified( true );
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		updateEditor( true );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
		m_pOldPoint = ev->y();
	}

	//edit pan
	if ( m_pDraggedNote && selectedProperty == 1 ) { // Pan
		if ( m_pDraggedNote->get_note_off() ) return;

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

		float pan_L, pan_R;
		
		float val = (m_pDraggedNote->get_pan_r() - m_pDraggedNote->get_pan_l() + 0.5);

		float ymove = m_pOldPoint - ev->y();
		val = val  +  (ymove / 100);


		if ( val > 0.5 ) {
			pan_L = 1.0 - val;
			pan_R = 0.5;
		}
		else {
			pan_L = 0.5;
			pan_R = val;
		}

		m_pDraggedNote->set_pan_l( pan_L );
		m_pDraggedNote->set_pan_r( pan_R );

		__pan_L = pan_L;
		__pan_R = pan_R;

		Hydrogen::get_instance()->getSong()->set_is_modified( true );
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		updateEditor( true );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
		m_pOldPoint = ev->y();
	}

	//edit lead lag
	if ( m_pDraggedNote && selectedProperty ==  2 ) { // Lead and Lag
		if ( m_pDraggedNote->get_note_off() ) return;

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

		
		float val = ( m_pDraggedNote->get_lead_lag() - 1.0 ) / -2.0 ;

		float ymove = m_pOldPoint - ev->y();
		val = val  +  (ymove / 100);

		if (val > 1.0) {
			val = 1.0;
		}
		else if (val < 0.0) {
			val = 0.0;
		}

		m_pDraggedNote->set_lead_lag((val * -2.0) + 1.0);

		__leadLag = (val * -2.0) + 1.0;

		char valueChar[100];
		if ( m_pDraggedNote->get_lead_lag() < 0.0 ) {
			sprintf( valueChar, "%.2f",  ( m_pDraggedNote->get_lead_lag() * -5 ) ); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
		} else if ( m_pDraggedNote->get_lead_lag() > 0.0 ) {
			sprintf( valueChar, "%.2f",  ( m_pDraggedNote->get_lead_lag() * 5 ) ); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
		} else {
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Note on beat"), 2000 );
		}

		Hydrogen::get_instance()->getSong()->set_is_modified( true );
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		updateEditor( true );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
		m_pOldPoint = ev->y();
	}

}


void PianoRollEditor::mouseDragEndEvent( QMouseEvent *ev )
{
	//INFOLOG("Mouse release event" );
	if (m_pPattern == nullptr) {
		return;
	}

	if ( m_pDraggedNote ) {
		if ( m_pDraggedNote->get_note_off() ) return;



		if( m_pDraggedNote->get_length() != __oldLength )
		{
			SE_editPianoRollNoteLengthAction *action = new SE_editPianoRollNoteLengthAction( m_pDraggedNote->get_position(),  m_pDraggedNote->get_position(), m_pDraggedNote->get_length(),__oldLength, __selectedPatternNumber, __selectedInstrumentnumber, __pressedLine );
			HydrogenApp::get_instance()->m_pUndoStack->push( action );
		}


		if( __velocity == __oldVelocity &&  __oldLeadLag == __leadLag && __oldPan_L == __pan_L && __oldPan_R == __pan_R ) return;
		SE_editNotePropertiesPianoRollAction *action = new SE_editNotePropertiesPianoRollAction( m_pDraggedNote->get_position(),
																								 m_pDraggedNote->get_position(),
																								 __selectedPatternNumber,
																								 __selectedInstrumentnumber,
																								 __velocity,
																								 __oldVelocity,
																								 __pan_L,
																								 __oldPan_L,
																								 __pan_R,
																								 __oldPan_R,
																								 __leadLag,
																								 __oldLeadLag,
																								 __pressedLine );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
}

QPoint PianoRollEditor::cursorPosition()
{
	uint x = 20 + m_pPatternEditorPanel->getCursorPosition() * m_nGridWidth;
	uint y = m_nRowHeight * pitchToLine( m_nCursorPitch ) + 1;
	return QPoint(x, y);
}

void PianoRollEditor::selectAll()
{
	m_selection.clearSelection();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	Pattern *pPattern = pSong->get_pattern_list()->get( pHydrogen->getSelectedPatternNumber() );
	Instrument *pInstrument =  pSong->get_instrument_list()->get( pHydrogen->getSelectedInstrumentNumber() );
	FOREACH_NOTE_CST_IT_BEGIN_END( pPattern->get_notes(), it )
	{
		if ( it->second->get_instrument() == pInstrument ) {
			m_selection.addToSelection( it->second );
		}
	}
	updateEditor( true );
}

void PianoRollEditor::selectNone()
{
	m_selection.clearSelection();
	updateEditor( true );
}

void PianoRollEditor::selectInstrumentNotes( int nInstrument )
{
	InstrumentList *pInstrumentList = Hydrogen::get_instance()->getSong()->get_instrument_list();
	Instrument *pInstrument = pInstrumentList->get( nInstrument );

	m_selection.clearSelection();
	FOREACH_NOTE_CST_IT_BEGIN_END(m_pPattern->get_notes(), it) {
		if ( it->second->get_instrument() == pInstrument ) {
			m_selection.addToSelection( it->second );
		}
	}
	update();
}

void PianoRollEditor::deleteSelection()
{
	if ( m_selection.begin() != m_selection.end() ) {
		// Delete a selection.
		int nSelectedInstrumentnumber = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
		pUndo->beginMacro("delete notes");
		validateSelection();
		for ( Note *pNote : m_selection ) {
			if ( m_selection.isSelected( pNote ) ) {
				int nLine = pitchToLine( pNote->get_notekey_pitch() );
				pUndo->push( new SE_addOrDeleteNotePianoRollAction( pNote->get_position(),
																	nLine,
																	__selectedPatternNumber,
																	nSelectedInstrumentnumber,
																	pNote->get_length(),
																	pNote->get_velocity(),
																	pNote->get_pan_l(),
																	pNote->get_pan_r(),
																	pNote->get_lead_lag(),
																	pNote->get_key(),
																	pNote->get_octave(),
																	true ) );
			}
		}
		pUndo->endMacro();
		m_selection.clearSelection();
	}
}

///
/// Copy selection to clipboard in XML
///
void PianoRollEditor::copy()
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "noteSelection" );
	XMLNode positionNode = root.createNode( "sourcePosition" );
	XMLNode noteList = root.createNode( "noteList" );

	positionNode.write_int( "position", m_pPatternEditorPanel->getCursorPosition() );
	positionNode.write_int( "pitch", m_nCursorPitch );
	positionNode.write_int( "instrument", Hydrogen::get_instance()->getSelectedInstrumentNumber() );

	for ( Note *pNote : m_selection ) {
		XMLNode note_node = noteList.createNode( "note" );
		pNote->save_to( &note_node );
	}

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText( doc.toString() );
}

void PianoRollEditor::cut()
{
	copy();
	deleteSelection();
}


///
/// Paste selection
///
/// Selection is XML containing notes, contained in a root 'note_selection' element.
///
void PianoRollEditor::paste()
{
	QClipboard *clipboard = QApplication::clipboard();
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;
	InstrumentList *pInstrList = Hydrogen::get_instance()->getSong()->get_instrument_list();
	int nInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	XMLNode noteList;
	int nDeltaPos = 0, nDeltaPitch = 0;


	XMLDoc doc;
	if ( ! doc.setContent( clipboard->text() ) ) {
		// Pasted something that's not valid XML.
		return;
	}

	XMLNode selection = doc.firstChildElement( "noteSelection" ); qDebug() << "noteSelection:";
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

			nDeltaPos = nCurrentPos - positionNode.read_int( "position", nCurrentPos );
			nDeltaPitch = m_nCursorPitch - positionNode.read_int( "pitch", m_nCursorPitch );
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
																	__selectedPatternNumber,
																	nInstrument,
																	pNote->get_length(),
																	pNote->get_velocity(),
																	pNote->get_pan_l(),
																	pNote->get_pan_r(),
																	pNote->get_lead_lag(),
																	0,
																	0,
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
	m_pPatternEditorPanel->setCursorHidden( false );
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Selection key, nothing more to do (other than update editor)
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
		if ( m_nCursorPitch > octaveKeyToPitch( (Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN ) ) {
			m_nCursorPitch --;
		}

	} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) || ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		m_nCursorPitch = octaveKeyToPitch( (Note::Octave)OCTAVE_MIN, (Note::Key)KEY_MIN );

	} else if ( ev->matches( QKeySequence::MoveToPreviousLine ) || ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( m_nCursorPitch < octaveKeyToPitch( (Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX ) ) {
			m_nCursorPitch ++;
		}

	} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) || ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		m_nCursorPitch = octaveKeyToPitch( (Note::Octave)OCTAVE_MAX, (Note::Key)KEY_MAX );

	} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter/Return : Place or remove note at current position
		int pressedline = pitchToLine( m_nCursorPitch );
		int nPitch = lineToPitch( pressedline );
		addOrRemoveNote( m_pPatternEditorPanel->getCursorPosition(), -1, pressedline,
						 pitchToKey( nPitch ), pitchToOctave( nPitch ) );

	} else if ( ev->matches( QKeySequence::SelectAll ) ) {
		// Key: Ctrl + A: Select all
		selectAll();

	} else if ( ev->matches( QKeySequence::Deselect ) ) {
		// Key: Shift + Ctrl + A: clear selection
		selectNone();

	} else if ( ev->key() == Qt::Key_Delete || ev->key() == Qt::Key_Backspace ) {
		// Key: Delete: delete selection
		deleteSelection();

	} else if ( ev->matches( QKeySequence::Copy ) ) {
		copy();

	} else if ( ev->matches( QKeySequence::Paste ) ) {
		paste();

	} else if ( ev->matches( QKeySequence::Cut ) ) {
		cut();

	} else {
		m_pPatternEditorPanel->setCursorHidden( true );
		ev->ignore();
		return;
	}

	// Update editor
	QPoint pos = cursorPosition();
	m_pScrollView->ensureVisible( pos.x(), pos.y() );
	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	updateEditor( true );
	ev->accept();
}


void PianoRollEditor::focusInEvent( QFocusEvent * ev )
{
	UNUSED( ev );
	if ( ev->reason() == Qt::TabFocusReason || ev->reason() == Qt::BacktabFocusReason ) {
		m_pPatternEditorPanel->setCursorHidden( false );
		m_pPatternEditorPanel->ensureCursorVisible();
	}
	updateEditor( true );
}


void PianoRollEditor::editNoteLengthAction( int nColumn,  int nRealColumn,  int length, int selectedPatternNumber, int nSelectedInstrumentnumber, int pressedline)
{

	Hydrogen *pEngine = Hydrogen::get_instance();

	Song *pSong = pEngine->getSong();
	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( nSelectedInstrumentnumber );


	Note::Octave pressedoctave = pitchToOctave( lineToPitch( pressedline ) );
	Note::Key pressednotekey = pitchToKey( lineToPitch( pressedline ) );

	Note* pDraggedNote = nullptr;
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave, false );
	if ( pDraggedNote ){
		pDraggedNote->set_length( length );
	}
	AudioEngine::get_instance()->unlock();
	updateEditor( true );
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}



void PianoRollEditor::editNotePropertiesAction( int nColumn,
						int nRealColumn,
						int selectedPatternNumber,
						int selectedInstrumentnumber,
						float velocity,
						float pan_L,
						float pan_R,
						float leadLag,
						int pressedline )
{

	Hydrogen *pEngine = Hydrogen::get_instance();

	Note::Octave pressedoctave = pitchToOctave( lineToPitch( pressedline ) );
	Note::Key pressednotekey = pitchToKey( lineToPitch( pressedline ) );

	Song *pSong = pEngine->getSong();

	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( selectedInstrumentnumber );

	Note* pDraggedNote = nullptr;
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave, false );
	if ( pDraggedNote ){
		pDraggedNote->set_velocity( velocity );
		pDraggedNote->set_pan_l( pan_L );
		pDraggedNote->set_pan_r( pan_R );
		pDraggedNote->set_lead_lag( leadLag );
	}
	AudioEngine::get_instance()->unlock();
	updateEditor( true );
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}



void PianoRollEditor::zoom_in()
{
	if (m_nGridWidth >= 3){
		m_nGridWidth *= 2;
	}else
	{
		m_nGridWidth *= 1.5;
	}
	updateEditor();
}



void PianoRollEditor::zoom_out()
{
	if ( m_nGridWidth > 1.5 ) {
		if (m_nGridWidth > 3){
			m_nGridWidth /= 2;
		}else
		{
			m_nGridWidth /= 1.5;
		}
		updateEditor();
	}
}


// Selection manager interface
void PianoRollEditor::selectionMoveEndEvent( QInputEvent *ev ) {
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
		Note::Octave newOctave = pitchToOctave( nNewPitch );
		Note::Key newKey = pitchToKey( nNewPitch );
		bool bNoteInRange = ( newOctave >= OCTAVE_MIN && newOctave <= OCTAVE_MAX && nNewPosition >= 0
							  && nNewPosition < m_pPattern->get_length() );

		if ( m_bCopyNotMove ) {
			if ( bNoteInRange ) {
				pUndo->push( new SE_addOrDeleteNotePianoRollAction( nNewPosition, nLine, nSelectedPatternNumber,
																	nSelectedInstrumentNumber, pNote->get_length(), pNote->get_velocity(),
																	pNote->get_pan_l(), pNote->get_pan_r(),
																	pNote->get_lead_lag(), newKey, newOctave, false ) );
			}
		} else {
			if ( bNoteInRange ) {
				pUndo->push( new SE_moveNotePianoRollAction( nPosition, octave, key, nSelectedPatternNumber, nNewPosition, newOctave, newKey, pNote ) );
			} else {
				pUndo->push( new SE_addOrDeleteNotePianoRollAction( pNote->get_position(), nLine - offset.y(),  nSelectedPatternNumber,
																	nSelectedInstrumentNumber, pNote->get_length(), pNote->get_velocity(),
																	pNote->get_pan_l(), pNote->get_pan_r(),
																	pNote->get_lead_lag(), key, octave, true ) );
			}
		}
	}

	m_bSelectNewNotes = false;
	pUndo->endMacro();
}

std::vector<PianoRollEditor::SelectionIndex> PianoRollEditor::elementsIntersecting( QRect r ) {

	int w = 8;
	int h = m_nRowHeight - 2;
	int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Instrument *pInstr = Hydrogen::get_instance()->getSong()->get_instrument_list()->get( nInstr );

	r = r.normalized();
	if ( r.top() == r.bottom() && r.left() == r.right() ) {
		r += QMargins( 2, 2, 2, 2 );
	}

	// Calculate the first and last position values that this rect will intersect with
	int x_min = (r.left() - w - 20) / m_nGridWidth;
	int x_max = (r.right() + w - 20) / m_nGridWidth;

	const Pattern::notes_t* pNotes = m_pPattern->get_notes();
	std::vector<SelectionIndex> result;

	for ( auto it = pNotes->lower_bound( x_min ); it != pNotes->end() && it->first <= x_max; ++it ) {
		Note *pNote = it->second;
		if ( pNote->get_instrument() == pInstr ) {
			uint start_x = 20 + pNote->get_position() * m_nGridWidth;
			uint start_y = m_nRowHeight * pitchToLine( pNote->get_notekey_pitch() ) + 1;

			if ( r.intersects( QRect( start_x -4 , start_y, w, h ) ) ) {
				result.push_back( pNote );
			}
		}
	}
	updateEditor( true );
	return std::move( result );
}

///
/// Ensure selection only refers to valid notes, and does not contain any stale references to deleted notes.
///
void PianoRollEditor::validateSelection()
{
	// Rebuild selection from valid notes.
	std::set<Note *> valid;
	FOREACH_NOTE_CST_IT_BEGIN_END(m_pPattern->get_notes(), it) {
		if ( m_selection.isSelected( it->second ) ) {
			valid.insert( it->second );
		}
	}
	m_selection.clearSelection();
	for (auto i : valid ) {
		m_selection.addToSelection( i );
	}
}

///
/// Position of keyboard input cursor on screen
///
QRect PianoRollEditor::getKeyboardCursorRect() {
	QPoint pos = cursorPosition();
	return QRect( pos.x() - m_nGridWidth*3, pos.y(),
				  m_nGridWidth*6, m_nRowHeight );
}
