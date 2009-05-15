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

#include <cassert>

#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/note.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/Pattern.h>
using namespace H2Core;

#include "../HydrogenApp.h"

PianoRollEditor::PianoRollEditor( QWidget *pParent )
 : QWidget( pParent )
 , Object( "PianoRollEditor" )
 , m_nResolution( 8 )
 , m_bUseTriplets( false )
 , m_pPattern( NULL )
{
	INFOLOG( "INIT" );

	m_nRowHeight = 10;
	m_nOctaves = 7;

	setAttribute(Qt::WA_NoBackground);
	setFocusPolicy(Qt::ClickFocus);
	m_nGridWidth = Preferences::getInstance()->getPatternEditorGridWidth();

	m_nEditorWidth = 20 + m_nGridWidth *  (MAX_NOTES * 4);
	m_nEditorHeight = m_nOctaves * 12 * m_nRowHeight;

	m_pBackground = new QPixmap( m_nEditorWidth, m_nEditorHeight );
	m_pTemp = new QPixmap( m_nEditorWidth, m_nEditorHeight );

	resize( m_nEditorWidth, m_nEditorHeight );
	
	createBackground();

	HydrogenApp::getInstance()->addEventListener( this );
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
	uint nEditorWidth;
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


void PianoRollEditor::selectedInstrumentChangedEvent()
{
	updateEditor();
}



void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pTemp, ev->rect() );
}



void PianoRollEditor::createBackground()
{
	INFOLOG( "(re)creating the background" );

	QColor backgroundColor( 250, 250, 250 );
	m_pBackground->fill( backgroundColor );


	QColor octaveColor( 230, 230, 230 );
	QColor octaveAlternateColor( 200, 200, 200 );
	QColor baseOctaveColor( 245, 245, 245 );
	QColor baseNoteColor( 255, 255, 255 );	

	unsigned start_x = 0;
	unsigned end_x = width();

	QPainter p( m_pBackground );

	for ( uint octave = 0; octave < m_nOctaves; ++octave ) {
		unsigned start_y = octave * 12 * m_nRowHeight;

		if ( octave % 2 ) {
			if ( octave == 3 ){
				p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, baseOctaveColor );
			}
			else
			{
				p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, octaveColor );
			}
		}
		else {
			p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, octaveAlternateColor );
			
		}
	}
		p.fillRect( start_x, ( 3 * 12 + 11 )* m_nRowHeight, end_x - start_x, m_nRowHeight,baseNoteColor  );	


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
	for ( int oct = 0; oct < m_nOctaves; oct++ ){
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
		
	draw_grid( p );
}




void PianoRollEditor::draw_grid( QPainter& p )
{
	static const UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
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
					p.setPen( QPen( res_1, 1, Qt::DotLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (m_nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (m_nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (m_nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (m_nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine  ) );
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
					p.setPen( QPen( res_1, 0, Qt::DotLine  ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine  ) );
				}
				p.drawLine(x, 1, x, m_nEditorHeight - 1);
				nCounter++;
			}
		}
	}
}



void PianoRollEditor::selectedPatternChangedEvent()
{
	INFOLOG( "updating m_pPattern pointer" );

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



void PianoRollEditor::drawPattern()
{
	if ( isVisible() == false ) {
		return;
	}


	INFOLOG( "draw pattern" );

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
		ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	if ( nInstrument != Hydrogen::get_instance()->getSelectedInstrumentNumber() ) {
		return;
	}

	uint start_x = 20 + pNote->get_position() * m_nGridWidth;
	uint start_y = height() - m_nRowHeight - ( m_nRowHeight * pNote->m_noteKey.m_key + ( 12 * (pNote->m_noteKey.m_nOctave +3) ) * m_nRowHeight ) + 1;
	uint w = 8;
	uint h = m_nRowHeight - 2;


	// external rect
	pPainter->setBrush(QColor( 100, 160, 233 ));
	pPainter->drawEllipse( start_x -4 , start_y, w, h );

	// internal rect
//	pPainter->fillRect( start_x + 1, start_y + 1, w - 2, h - 2, QColor( 100, 100, 255 ) );
}



void PianoRollEditor::mousePressEvent(QMouseEvent*)
{
	INFOLOG("Mouse press event");
}



void PianoRollEditor::mouseReleaseEvent(QMouseEvent*)
{
	INFOLOG("Mouse release event" );
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