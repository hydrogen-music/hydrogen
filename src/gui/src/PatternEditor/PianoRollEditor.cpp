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

#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/audio_engine.h>
using namespace H2Core;

#include "../HydrogenApp.h"


const char* PianoRollEditor::__class_name = "PianoRollEditor";

PianoRollEditor::PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel )
	: QWidget( pParent )
	, Object( __class_name )
	, m_nResolution( 8 )
	, m_bRightBtnPressed( false )
	, m_bUseTriplets( false )
	, m_pPattern( NULL )
	, m_pPatternEditorPanel( panel )
	, m_pDraggedNote( NULL )
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

	resize( m_nEditorWidth, m_nEditorHeight );
	
	createBackground();

	HydrogenApp::get_instance()->addEventListener( this );
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


void PianoRollEditor::updateEditor()
{
	//	uint nEditorWidth;
	if ( m_pPattern ) {
		m_nEditorWidth = 20 + m_nGridWidth * m_pPattern->get_length();
	}
	else {
		m_nEditorWidth = 20 + m_nGridWidth * MAX_NOTES;
	}
	resize( m_nEditorWidth, height() );

	// redraw all
	update( 0, 0, width(), height() );

	createBackground();
	drawPattern();
	//	ERRORLOG(QString("update editor %1").arg(m_nEditorWidth));
}



//eventlistener
void PianoRollEditor::patternModifiedEvent()
{
	updateEditor();
}



void PianoRollEditor::selectedInstrumentChangedEvent()
{
	updateEditor();
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
		m_pPattern = NULL;
	}
	__selectedPatternNumber = nSelectedPatternNumber;
	updateEditor();
}



void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pTemp, ev->rect() );
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
	uint start_y = height() - m_nRowHeight - ( m_nRowHeight * pNote->get_key() + ( 12 * (pNote->get_octave() +3) ) * m_nRowHeight ) + 1;
	uint w = 8;
	uint h = m_nRowHeight - 2;

	QColor color = m_pPatternEditorPanel->getDrumPatternEditor()->computeNoteColor( pNote->get_velocity() );

	if ( pNote->get_length() == -1 && pNote->get_note_off() == false ) {
		pPainter->setBrush( color );
		pPainter->drawEllipse( start_x -4 , start_y, w, h );
	}
	else if ( pNote->get_length() == 1 && pNote->get_note_off() == true ){
		pPainter->setBrush(QColor( noteoffColor ));
		pPainter->drawEllipse( start_x -4 , start_y, w, h );
	}
	else {
		float fNotePitch = pNote->get_octave() * 12 + pNote->get_key();
		float fStep = pow( 1.0594630943593, ( double )fNotePitch );

		int nend = m_nGridWidth * pNote->get_length() / fStep;
		nend = nend - 1;	// lascio un piccolo spazio tra una nota ed un altra

		pPainter->setBrush( color );
		pPainter->fillRect( start_x, start_y, nend, h, color );
		pPainter->drawRect( start_x, start_y, nend, h );
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


void PianoRollEditor::mousePressEvent(QMouseEvent *ev)
{
	
	//ERRORLOG("Mouse press event");
	if ( m_pPattern == NULL ) {
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
	
	int pressedline = ((int) ev->y()) / ((int) m_nRowHeight);

	Instrument *pSelectedInstrument = NULL;
	int nSelectedInstrumentnumber = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	pSelectedInstrument = pSong->get_instrument_list()->get( nSelectedInstrumentnumber );
	assert(pSelectedInstrument);

	//ERRORLOG(QString("pressedline: %1, column %2, event ev: %3, editorhight %4").arg(pressedline).arg(nColumn).arg(ev->y()).arg(m_nEditorHeight));

	Note::Octave pressedoctave = (Note::Octave)(3 - (pressedline / 12 ));
	Note::Key pressednotekey;
	if ( pressedline < 12 ){
		pressednotekey = (Note::Key)(11 - pressedline);
	}
	else
	{
		pressednotekey = (Note::Key)(11 - pressedline % 12);
	}

	
	//ERRORLOG(QString("pressedline: %1, octave %2, notekey: %3").arg(pressedline).arg(pressedoctave).arg(pressednotekey));

	if (ev->button() == Qt::LeftButton ) {

		if ( ev->modifiers() & Qt::ShiftModifier ){

			SE_addPianoRollNoteOffAction *action = new SE_addPianoRollNoteOffAction( nColumn, pressedline, __selectedPatternNumber, nSelectedInstrumentnumber );
			HydrogenApp::get_instance()->m_undoStack->push( action );
			return;
		}

		unsigned nRealColumn = 0;
		if( ev->x() > 20 ) {
			nRealColumn = (ev->x() - 20) / static_cast<float>(m_nGridWidth);
		}

		H2Core::Note* pDraggedNote = 0;
		pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave );

		int oldLength = -1;
		float oldVelocity = 0.8f;
		float oldPan_L = 0.5f;
		float oldPan_R = 0.5f;
		float oldLeadLag = 0.0f;
		int oldNoteKeyVal = 0;
		int oldOctaveKeyVal = 0;

		if( pDraggedNote ){
			oldLength = pDraggedNote->get_length();
			oldVelocity = pDraggedNote->get_velocity();
			oldPan_L = pDraggedNote->get_pan_l();
			oldPan_R = pDraggedNote->get_pan_r();
			oldLeadLag = pDraggedNote->get_lead_lag();
			oldNoteKeyVal = pDraggedNote->get_key();
			oldOctaveKeyVal = pDraggedNote->get_octave();
		}

		SE_addNotePianoRollAction *action = new SE_addNotePianoRollAction( nColumn,
																		   pressedline,
																		   __selectedPatternNumber,
																		   nSelectedInstrumentnumber,
																		   oldLength,
																		   oldVelocity,
																		   oldPan_L,
																		   oldPan_R,
																		   oldLeadLag,
																		   oldNoteKeyVal,
																		   oldOctaveKeyVal );
		HydrogenApp::get_instance()->m_undoStack->push( action );

	}

	else if (ev->button() == Qt::RightButton ) {
		m_bRightBtnPressed = true;
		m_pDraggedNote = NULL;
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
											 bool noteOff   )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	H2Core::Pattern *pPattern;

	Instrument *pSelectedInstrument = NULL;
	pSelectedInstrument = pSong->get_instrument_list()->get( selectedinstrument );
	assert(pSelectedInstrument);

	if ( ( selectedPatternNumber != -1 ) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}
	else {
		pPattern = NULL;
	}

	Note::Octave pressedoctave = (Note::Octave)(3 - (pressedLine / 12 ));
	Note::Key pressednotekey;
	if ( pressedLine < 12 ){
		pressednotekey = (Note::Key)(11 - pressedLine);
	}
	else
	{
		pressednotekey = (Note::Key)(11 - pressedLine % 12);
	}

	m_bRightBtnPressed = false;

	bool bNoteAlreadyExist = false;
	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine
	Note* note = m_pPattern->find_note( nColumn, -1, pSelectedInstrument, pressednotekey, pressedoctave );
	if( note ) {
		// the note exists...remove it!
		bNoteAlreadyExist = true;
		m_pPattern->remove_note( note );
		delete note;
	}

	if ( bNoteAlreadyExist == false ) {
		// create the new note
		unsigned nPosition = nColumn;
		float fVelocity = oldVelocity;
		float fPan_L = oldPan_L;
		float fPan_R = oldPan_R;
		int nLength = oldLength;
		float fPitch = 0.0f;

		if(noteOff)
		{
			fVelocity = 0.0f;
			fPan_L = 0.5f;
			fPan_R = 0.5f;
			nLength = 1;
			fPitch = 0.0f;
		}


		Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch );
		pNote->set_note_off( noteOff );
		if(! noteOff) pNote->set_lead_lag( oldLeadLag );
		pNote->set_key_octave( pressednotekey, pressedoctave );
		pPattern->insert_note( pNote );
		// hear note
		Preferences *pref = Preferences::get_instance();
		if ( pref->getHearNewNotes() && !noteOff ) {
			Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan_L, fPan_R, nLength, fPitch);
			pNote2->set_key_octave( pressednotekey, pressedoctave );
			AudioEngine::get_instance()->get_sampler()->note_on(pNote2);
		}
	}
	pSong->set_is_modified( true );
	AudioEngine::get_instance()->unlock(); // unlock the audio engine

	updateEditor();
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}





void PianoRollEditor::mouseMoveEvent(QMouseEvent *ev)
{
	if (m_pPattern == NULL) {
		return;
	}

	int row = ((int) ev->y()) / ((int) m_nEditorHeight);
	if (row >= (int) m_nOctaves * 12 ) {
		return;
	}

	if (m_bRightBtnPressed && m_pDraggedNote ) {
		if ( m_pDraggedNote->get_note_off() ) return;
		int nTickColumn = getColumn( ev );

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->get_position();

		if (nLen <= 0) {
			nLen = -1;
		}

		float fNotePitch = m_pDraggedNote->get_octave() * 12 + m_pDraggedNote->get_key();
		float fStep = 0;
		if(nLen > -1){
			fStep = pow( 1.0594630943593, ( double )fNotePitch );
		} else {
			fStep = 1.0;
		}
		m_pDraggedNote->set_length( nLen * fStep);

		Hydrogen::get_instance()->getSong()->set_is_modified( true );
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		//__draw_pattern();
		updateEditor();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	}

	int selectedProperty = m_pPatternEditorPanel->getPropertiesComboValue();

	//edit velocity
	if (m_bRightBtnPressed && m_pDraggedNote && selectedProperty == 0 ) { // Velocity
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

		//__draw_pattern();
		updateEditor();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
		m_pOldPoint = ev->y();
	}

	//edit pan
	if (m_bRightBtnPressed && m_pDraggedNote && selectedProperty == 1 ) { // Pan
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

		//__draw_pattern();
		updateEditor();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
		m_pOldPoint = ev->y();
	}

	//edit lead lag
	if (m_bRightBtnPressed && m_pDraggedNote && selectedProperty ==  2 ) { // Lead and Lag
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

		//__draw_pattern();
		updateEditor();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
		m_pOldPoint = ev->y();
	}

}


void PianoRollEditor::mouseReleaseEvent(QMouseEvent *ev)
{
	//INFOLOG("Mouse release event" );
	if (m_pPattern == NULL) {
		return;
	}

	if ( m_bRightBtnPressed && m_pDraggedNote ) {
		if ( m_pDraggedNote->get_note_off() ) return;



		if( m_pDraggedNote->get_length() != __oldLength )
		{
			SE_editPianoRollNoteLengthAction *action = new SE_editPianoRollNoteLengthAction( m_pDraggedNote->get_position(),  m_pDraggedNote->get_position(), m_pDraggedNote->get_length(),__oldLength, __selectedPatternNumber, __selectedInstrumentnumber, __pressedLine );
			HydrogenApp::get_instance()->m_undoStack->push( action );
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
		HydrogenApp::get_instance()->m_undoStack->push( action );
	}
}


void PianoRollEditor::editNoteLengthAction( int nColumn,  int nRealColumn,  int length, int selectedPatternNumber, int nSelectedInstrumentnumber, int pressedline)
{

	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();

	H2Core::Pattern *pPattern;
	if ( (selectedPatternNumber != -1) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}
	else {
		pPattern = NULL;
	}

	Song *pSong = pEngine->getSong();
	int nInstruments = pSong->get_instrument_list()->size();

	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( nSelectedInstrumentnumber );


	Note::Octave pressedoctave = (Note::Octave)(3 - (pressedline / 12 ));
	Note::Key pressednotekey;
	if ( pressedline < 12 ){
		pressednotekey = (Note::Key)(11 - pressedline);
	}
	else
	{
		pressednotekey = (Note::Key)(11 - pressedline % 12);
	}

	Note* pDraggedNote = 0;
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave, false );
	if ( pDraggedNote ){
		pDraggedNote->set_length( length );
	}
	AudioEngine::get_instance()->unlock();
	updateEditor();
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
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();

	H2Core::Pattern *pPattern;
	if ( (selectedPatternNumber != -1) && ( (uint)selectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( selectedPatternNumber );
	}
	else {
		pPattern = NULL;
	}


	Note::Octave pressedoctave = (Note::Octave)(3 - (pressedline / 12 ));
	Note::Key pressednotekey;
	if ( pressedline < 12 ){
		pressednotekey = (Note::Key)(11 - pressedline);
	}
	else
	{
		pressednotekey = (Note::Key)(11 - pressedline % 12);
	}

	Song *pSong = pEngine->getSong();
	int nInstruments = pSong->get_instrument_list()->size();

	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( selectedInstrumentnumber );

	Note* pDraggedNote = 0;
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	pDraggedNote = m_pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, pressednotekey, pressedoctave, false );
	if ( pDraggedNote ){
		pDraggedNote->set_velocity( velocity );
		pDraggedNote->set_pan_l( pan_L );
		pDraggedNote->set_pan_r( pan_R );
		pDraggedNote->set_lead_lag( leadLag );
	}
	AudioEngine::get_instance()->unlock();
	updateEditor();
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
