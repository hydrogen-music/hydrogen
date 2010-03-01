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
#include "DrumPatternEditor.h"
#include <cassert>

#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/note.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/audio_engine.h>
using namespace H2Core;

#include "../HydrogenApp.h"

PianoRollEditor::PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel )
 : QWidget( pParent )
 , Object( "PianoRollEditor" )
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
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}
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
	std::multimap <int, Note*>::iterator pos;
	for ( pos = m_pPattern->note_map.begin(); pos != m_pPattern->note_map.end(); pos++ ) {
		//cout << "note" << endl;
		//cout << "note n: " << pos->first << endl;
		Note *note = pos->second;
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
	for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
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
	uint start_y = height() - m_nRowHeight - ( m_nRowHeight * pNote->m_noteKey.m_key + ( 12 * (pNote->m_noteKey.m_nOctave +3) ) * m_nRowHeight ) + 1;
	uint w = 8;
	uint h = m_nRowHeight - 2;

	QColor color = m_pPatternEditorPanel->getDrumPatternEditor()->computeNoteColor( pNote->get_velocity() );

	if ( pNote->get_length() == -1 && pNote->get_noteoff() == false ) {
		pPainter->setBrush( color );
		pPainter->drawEllipse( start_x -4 , start_y, w, h );
	}
	else if ( pNote->get_length() == 1 && pNote->get_noteoff() == true ){
		pPainter->setBrush(QColor( noteoffColor ));
		pPainter->drawEllipse( start_x -4 , start_y, w, h );
	}
	else {
		float fNotePitch = pNote->m_noteKey.m_nOctave * 12 + pNote->m_noteKey.m_key;
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

	int pressedoctave = 3 - (pressedline / 12 );
	int pressednotekey = 0;
	if ( pressedline < 12 ){
		pressednotekey = 11 - pressedline;
	}
	else
	{
		pressednotekey = 11 - pressedline % 12;
	}

	
	//ERRORLOG(QString("pressedline: %1, octave %2, notekey: %3").arg(pressedline).arg(pressedoctave).arg(pressednotekey));

	if (ev->button() == Qt::LeftButton ) {
		m_bRightBtnPressed = false;
		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

		bool bNoteAlreadyExist = false;
		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.lower_bound( nColumn ); pos != m_pPattern->note_map.upper_bound( nColumn ); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );
			if ( pNote->m_noteKey.m_nOctave ==  pressedoctave && pNote->m_noteKey.m_key  ==  pressednotekey && pNote->get_instrument() == pSelectedInstrument) {
				// the note exists...remove it!
				bNoteAlreadyExist = true;
				delete pNote;
				m_pPattern->note_map.erase( pos );
				break;
			}
		}

		if ( bNoteAlreadyExist == false ) {
			// create the new note
			const unsigned nPosition = nColumn;
			const float fVelocity = 0.8f;
			const float fPan_L = 0.5f;
			const float fPan_R = 0.5f;
			const int nLength = -1;
			const float fPitch = 0.0f;
			Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch );
			pNote->set_noteoff( false );

			if ( pressednotekey == 0 )//note c
				pNote->m_noteKey.m_key = H2Core::NoteKey::C;
			if ( pressednotekey == 1 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::Cs;
			if ( pressednotekey == 2 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::D;
			if ( pressednotekey == 3 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::Ef;
			if ( pressednotekey == 4 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::E;
			if ( pressednotekey == 5 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::F;
			if ( pressednotekey == 6 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::Fs;
			if ( pressednotekey == 7 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::G;
			if ( pressednotekey == 8 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::Af;
			if ( pressednotekey == 9 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::A;
			if ( pressednotekey == 10 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::Bf;
			if ( pressednotekey == 11 )
				pNote->m_noteKey.m_key = H2Core::NoteKey::B;
			
			pNote->m_noteKey.m_nOctave = pressedoctave;
			m_pPattern->note_map.insert( std::make_pair( nPosition, pNote ) );

			// hear note
			Preferences *pref = Preferences::get_instance();
			if ( pref->getHearNewNotes() ) {
				Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan_L, fPan_R, nLength, fPitch);
				if ( pressednotekey == 0 )//note c
					pNote2->m_noteKey.m_key = H2Core::NoteKey::C;
				if ( pressednotekey == 1 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::Cs;
				if ( pressednotekey == 2 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::D;
				if ( pressednotekey == 3 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::Ef;
				if ( pressednotekey == 4 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::E;
				if ( pressednotekey == 5 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::F;
				if ( pressednotekey == 6 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::Fs;
				if ( pressednotekey == 7 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::G;
				if ( pressednotekey == 8 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::Af;
				if ( pressednotekey == 9 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::A;
				if ( pressednotekey == 10 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::Bf;
				if ( pressednotekey == 11 )
					pNote2->m_noteKey.m_key = H2Core::NoteKey::B;
				
				pNote2->m_noteKey.m_nOctave = pressedoctave;
				AudioEngine::get_instance()->get_sampler()->note_on(pNote2);
			}
		}
		pSong->__is_modified = true;
		AudioEngine::get_instance()->unlock(); // unlock the audio engine
	}

	else if (ev->button() == Qt::RightButton ) {
		m_bRightBtnPressed = true;
		m_pDraggedNote = NULL;
		m_pOldPoint = ev->y();

		unsigned nRealColumn = 0;
		if( ev->x() > 20 ) {
			nRealColumn = (ev->x() - 20) / static_cast<float>(m_nGridWidth);
		}

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.lower_bound( nColumn ); pos != m_pPattern->note_map.upper_bound( nColumn ); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );

			if ( pNote->m_noteKey.m_nOctave ==  pressedoctave && pNote->m_noteKey.m_key  ==  pressednotekey && pNote->get_instrument() == pSelectedInstrument) {
				m_pDraggedNote = pNote;
				break;
			}
		}
		if ( !m_pDraggedNote ) {
			for ( pos = m_pPattern->note_map.lower_bound( nRealColumn ); pos != m_pPattern->note_map.upper_bound( nRealColumn ); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );

				if ( pNote->m_noteKey.m_nOctave ==  pressedoctave && pNote->m_noteKey.m_key  ==  pressednotekey && pNote->get_instrument() == pSelectedInstrument) {
					m_pDraggedNote = pNote;
					break;
				}
			}	
///
		//	__rightclickedpattereditor
		//	0 = note length
		//	1 = note off"
		//	2 = edit velocity
		//	3 = edit pan
		//	4 = edit lead lag

			if ( Preferences::get_instance()->__rightclickedpattereditor == 1){
				// create the new note
				const unsigned nPosition = nColumn;
				const float fVelocity = 0.0f;
				const float fPan_L = 0.5f;
				const float fPan_R = 0.5f;
				const int nLength = 1;
				const float fPitch = 0.0f;
				Note *poffNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch);
				poffNote->set_noteoff( true );
				if ( pressednotekey == 0 )//note c
					poffNote->m_noteKey.m_key = H2Core::NoteKey::C;
				if ( pressednotekey == 1 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::Cs;
				if ( pressednotekey == 2 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::D;
				if ( pressednotekey == 3 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::Ef;
				if ( pressednotekey == 4 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::E;
				if ( pressednotekey == 5 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::F;
				if ( pressednotekey == 6 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::Fs;
				if ( pressednotekey == 7 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::G;
				if ( pressednotekey == 8 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::Af;
				if ( pressednotekey == 9 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::A;
				if ( pressednotekey == 10 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::Bf;
				if ( pressednotekey == 11 )
					poffNote->m_noteKey.m_key = H2Core::NoteKey::B;
				
				poffNote->m_noteKey.m_nOctave = pressedoctave;
				
				m_pPattern->note_map.insert( std::make_pair( nPosition, poffNote ) );
	
				pSong->__is_modified = true;
			}

		}

		for ( int nCol = 0; unsigned(nCol) < nRealColumn; ++nCol ) {
			if ( m_pDraggedNote ) break;
			for ( pos = m_pPattern->note_map.lower_bound( nCol ); pos != m_pPattern->note_map.upper_bound( nCol ); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );

				if ( ( pNote->m_noteKey.m_nOctave ==  pressedoctave && pNote->m_noteKey.m_key  ==  pressednotekey && pNote->get_instrument() == pSelectedInstrument )
				    && ( (nRealColumn <= pNote->get_position() + pNote->get_length() )
				    && nRealColumn >= pNote->get_position() ) ){
					m_pDraggedNote = pNote;
					break;
				}
			}
		}
		AudioEngine::get_instance()->unlock();
	}

//	update( 0, 0, width(), height() );
	updateEditor();
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	
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

	//	__rightclickedpattereditor
	//	0 = note length
	//	1 = note off"
	//	2 = edit velocity
	//	3 = edit pan
	//	4 = edit lead lag

	if (m_bRightBtnPressed && m_pDraggedNote && ( Preferences::get_instance()->__rightclickedpattereditor == 0 ) ) {
		if ( m_pDraggedNote->get_noteoff() ) return;
		int nTickColumn = getColumn( ev );

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->get_position();

		if (nLen <= 0) {
			nLen = -1;
		}

		float fNotePitch = m_pDraggedNote->m_noteKey.m_nOctave * 12 + m_pDraggedNote->m_noteKey.m_key;
		float fStep = 0;
		if(nLen > -1){
			fStep = pow( 1.0594630943593, ( double )fNotePitch );
		}else
		{
			fStep = 1.0; 
		}
		m_pDraggedNote->set_length( nLen * fStep);

		Hydrogen::get_instance()->getSong()->__is_modified = true;
		AudioEngine::get_instance()->unlock(); // unlock the audio engine

		//__draw_pattern();
		updateEditor();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	}

	//edit velocity
	if (m_bRightBtnPressed && m_pDraggedNote && ( Preferences::get_instance()->__rightclickedpattereditor == 2 ) ) {
		if ( m_pDraggedNote->get_noteoff() ) return;

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

		Hydrogen::get_instance()->getSong()->__is_modified = true;
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
	if (m_bRightBtnPressed && m_pDraggedNote && ( Preferences::get_instance()->__rightclickedpattereditor == 3 ) ) {
		if ( m_pDraggedNote->get_noteoff() ) return;

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

		Hydrogen::get_instance()->getSong()->__is_modified = true;
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
	if (m_bRightBtnPressed && m_pDraggedNote && ( Preferences::get_instance()->__rightclickedpattereditor == 4 ) ) {
		if ( m_pDraggedNote->get_noteoff() ) return;

		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

		
		float val = ( m_pDraggedNote->get_leadlag() - 1.0 ) / -2.0 ;
	
		float ymove = m_pOldPoint - ev->y();
		val = val  +  (ymove / 100);

		if (val > 1.0) {
			val = 1.0;
		}
		else if (val < 0.0) {
			val = 0.0;
		}

		m_pDraggedNote->set_leadlag((val * -2.0) + 1.0);
		char valueChar[100];
		if ( m_pDraggedNote->get_leadlag() < 0.0 ) {
			sprintf( valueChar, "%.2f",  ( m_pDraggedNote->get_leadlag() * -5 ) ); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
		} else if ( m_pDraggedNote->get_leadlag() > 0.0 ) {
			sprintf( valueChar, "%.2f",  ( m_pDraggedNote->get_leadlag() * 5 ) ); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
		} else {
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Note on beat"), 2000 );
		}

		Hydrogen::get_instance()->getSong()->__is_modified = true;
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