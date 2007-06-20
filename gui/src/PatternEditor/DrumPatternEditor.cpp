/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "DrumPatternEditor.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"

#include <hydrogen/Globals.h>
#include <hydrogen/Song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/EventQueue.h>
#include <hydrogen/Instrument.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/note.h>
#include <hydrogen/audio_engine.h>

#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
#include "../Skin.h"

#include <cassert>
#include <algorithm>

#include <QRect>
#include <QCursor>
#include <QPixmap>
#include <QPaintEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

using namespace std;
using namespace H2Core;

DrumPatternEditor::DrumPatternEditor(QWidget* parent, PatternEditorPanel *pPanel)
 : QWidget( parent )
 , Object( "DrumPatternEditor" )
 , m_nResolution( 8 )
 , m_bUseTriplets( false )
 , m_pBackground( NULL )
 , m_pTemp( NULL )
 , m_bRightBtnPressed( false )
 , m_pDraggedNote( NULL )
 , m_pPattern( NULL )
 , m_pPatternEditorPanel( pPanel )
{
	//infoLog("INIT");

	setAttribute(Qt::WA_NoBackground);
	setFocusPolicy(Qt::ClickFocus);

	m_nGridWidth = Preferences::getInstance()->getPatternEditorGridWidth();
	m_nGridHeight = Preferences::getInstance()->getPatternEditorGridHeight();

	unsigned nEditorWidth = 20 + m_nGridWidth * ( MAX_NOTES * 4 );
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;

	// Prepare pixmaps
	m_pBackground = new QPixmap( nEditorWidth, m_nEditorHeight );
	m_pTemp = new QPixmap( nEditorWidth, m_nEditorHeight );

	createBackground();

	resize( nEditorWidth, m_nEditorHeight );

	HydrogenApp::getInstance()->addEventListener( this );
}



DrumPatternEditor::~DrumPatternEditor()
{
	//infoLog("DESTROY");
}



void DrumPatternEditor::updateEditor()
{
	//cout << "*** update pattern editor" << endl;

	Hydrogen* engine = Hydrogen::getInstance();

	// check engine state
	int state = engine->getState();
	if ( (state != STATE_READY) && (state != STATE_PLAYING) ) {
		ERRORLOG( "FIXME: skipping pattern editor update (state shoud be READY or PLAYING)" );
		return;
	}

	Hydrogen *pEngine = Hydrogen::getInstance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->getSize() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}


	uint nEditorWidth;
	if ( m_pPattern ) {
		nEditorWidth = 20 + m_nGridWidth * m_pPattern->m_nSize;
	}
	else {
		nEditorWidth = 20 + m_nGridWidth * MAX_NOTES;
	}
	resize( nEditorWidth, height() );


	createBackground();
	drawPattern();

	// redraw all
	update( 0, 0, width(), height() );
}



int DrumPatternEditor::getColumn(QMouseEvent *ev)
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



void DrumPatternEditor::mousePressEvent(QMouseEvent *ev)
{
	if ( m_pPattern == NULL ) {
		return;
	}
	Song *pSong = Hydrogen::getInstance()->getSong();
	int nInstruments = pSong->getInstrumentList()->getSize();

//	int row = (int)( (float)( height() - ev->y() )  / (float)m_nGridHeight);
	int row = (int)( ev->y()  / (float)m_nGridHeight);
	if (row >= nInstruments) {
		return;
	}

	int nColumn = getColumn( ev );

	if ( nColumn >= (int)m_pPattern->m_nSize ) {
		update( 0, 0, width(), height() );
		return;
	}
	Instrument *pSelectedInstrument = pSong->getInstrumentList()->get( row );

	if (ev->button() == Qt::LeftButton ) {
		m_bRightBtnPressed = false;
		AudioEngine::getInstance()->lock( "DrumPatternEditor::mousePressEvent" );	// lock the audio engine

		bool bNoteAlreadyExist = false;
		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->m_noteMap.lower_bound( nColumn ); pos != m_pPattern->m_noteMap.upper_bound( nColumn ); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );
			if ( pNote->getInstrument() == pSelectedInstrument ) {
				// the note exists...remove it!
				bNoteAlreadyExist = true;
				delete pNote;
				m_pPattern->m_noteMap.erase( pos );
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
			Note *pNote = new Note( pSelectedInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch);
			m_pPattern->m_noteMap.insert( std::make_pair( nPosition, pNote ) );

			// hear note
			Preferences *pref = Preferences::getInstance();
			if ( pref->getHearNewNotes() ) {
				Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan_L, fPan_R, nLength, fPitch);
				AudioEngine::getInstance()->getSampler()->note_on(pNote2);
			}
		}
		pSong->m_bIsModified = true;
		AudioEngine::getInstance()->unlock(); // unlock the audio engine
	}
	else if (ev->button() == Qt::RightButton ) {
		m_bRightBtnPressed = true;
		m_pDraggedNote = NULL;

		int nRealColumn = (ev->x() - 20) /  (int)m_nGridWidth;

		AudioEngine::getInstance()->lock( "DrumPatternEditor::mousePressEvent" );

		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->m_noteMap.lower_bound( nColumn ); pos != m_pPattern->m_noteMap.upper_bound( nColumn ); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );

			if ( pNote->getInstrument() == pSelectedInstrument ) {
				//INFOLOG( "TROVATA NOTA!" );
				m_pDraggedNote = pNote;
				break;
			}
		}
		if ( !m_pDraggedNote ) {
			for ( pos = m_pPattern->m_noteMap.lower_bound( nRealColumn ); pos != m_pPattern->m_noteMap.upper_bound( nRealColumn ); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );

				if ( pNote->getInstrument() == pSelectedInstrument ) {
					//INFOLOG( "TROVATA NOTA!" );
					m_pDraggedNote = pNote;
					break;
				}
			}
		}
		// potrei essere sulla coda di una nota precedente..
		for ( int nCol = 0; nCol < nRealColumn; ++nCol ) {
			if ( m_pDraggedNote ) break;
			for ( pos = m_pPattern->m_noteMap.lower_bound( nCol ); pos != m_pPattern->m_noteMap.upper_bound( nCol ); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );

				if ( pNote->getInstrument() == pSelectedInstrument ) {
					//INFOLOG( "TROVATA NOTA sulla coda...!" );
					m_pDraggedNote = pNote;
					break;
				}
			}
		}
		AudioEngine::getInstance()->unlock();
	}

	// update the selected line
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	if (nSelectedInstrument != row) {
	  //m_pPatternEditorPanel->setSelectedInstrument(row);
	  Hydrogen::getInstance()->setSelectedInstrumentNumber( row );


	}
	else {
		//createBackground();
		drawPattern();
		update( 0, 0, width(), height() );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
	}
}



void DrumPatternEditor::mouseReleaseEvent(QMouseEvent *ev)
{
	UNUSED( ev );
	setCursor( QCursor( Qt::ArrowCursor ) );

	if (m_pPattern == NULL) {
		return;
	}
}



void DrumPatternEditor::mouseMoveEvent(QMouseEvent *ev)
{
	if (m_pPattern == NULL) {
		return;
	}

	int row = MAX_INSTRUMENTS - 1 - (ev->y()  / (int)m_nGridHeight);
	if (row >= MAX_INSTRUMENTS) {
		return;
	}

	if (m_bRightBtnPressed && m_pDraggedNote ) {
		int nTickColumn = getColumn( ev );

		AudioEngine::getInstance()->lock("DrumPatternEditor::mouseMoveEvent");	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->m_nPosition;

		if (nLen <= 0) {
			nLen = -1;
		}
		m_pDraggedNote->m_nLength = nLen;

		Hydrogen::getInstance()->getSong()->m_bIsModified = true;
		AudioEngine::getInstance()->unlock(); // unlock the audio engine

		drawPattern();
		update( 0, 0, width(), height() );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
	}

}

void DrumPatternEditor::keyPressEvent (QKeyEvent *ev)
{
	ev->ignore();
}



///
/// Draws a pattern
///
void DrumPatternEditor::drawPattern()
{
	//cout << "draw pattern" << endl;
	static const UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	static const QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );

	QPainter painter( m_pTemp );

	// copy the background image
	painter.drawPixmap( rect(), *m_pBackground, rect() );

	QPainter* p = &painter;


	if (m_pPattern == NULL) {
		return;
	}

	int nNotes = m_pPattern->m_nSize;
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	Song *pSong = Hydrogen::getInstance()->getSong();

	InstrumentList * pInstrList = pSong->getInstrumentList();

	if ( m_nEditorHeight != (int)( m_nGridHeight * pInstrList->getSize() ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * pInstrList->getSize();
		resize( width(), m_nEditorHeight );
		createBackground();
	}


	for ( uint nInstr = 0; nInstr < pInstrList->getSize(); ++nInstr ) {
	  //Instrument *pInstr = pSong->getInstrumentList()->get( nInstr );
		uint y = m_nGridHeight * nInstr;
		if ( nInstr == (uint)nSelectedInstrument ) {	// selected instrument
			p->fillRect( 0, y + 1, (20 + nNotes * m_nGridWidth), m_nGridHeight - 1, selectedRowColor );
		}
		//if ( pInstr->m_bIsMuted ) {
		//	p->fillRect( 0, y + 1, ( 20 + nNotes * m_nGridWidth), m_nGridHeight - 1, muteRowColor );
		//}
	}


	// draw the grid
	drawGrid(  p );

	std::multimap <int, Note*>::iterator pos;
	for ( pos = m_pPattern->m_noteMap.begin(); pos != m_pPattern->m_noteMap.end(); pos++ ) {
		Note *note = pos->second;
		assert( note );
		drawNote( note, p );
	}
}


///
/// Draws a note
///
void DrumPatternEditor::drawNote( Note *note, QPainter* p )
{
	static const UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	static const QColor noteColor( pStyle->m_patternEditor_noteColor.getRed(), pStyle->m_patternEditor_noteColor.getGreen(), pStyle->m_patternEditor_noteColor.getBlue() );

	p->setRenderHint( QPainter::Antialiasing );

	int nInstrument = -1;
	InstrumentList * pInstrList = Hydrogen::getInstance()->getSong()->getInstrumentList();
	for ( uint nInstr = 0; nInstr < pInstrList->getSize(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr == note->getInstrument() ) {
 			nInstrument = nInstr;
			break;
		}
	}
	if ( nInstrument == -1 ) {
		ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	uint pos = note->m_nPosition;

	p->setPen( noteColor );

	if ( note->m_nLength == -1 ) {	// trigger note
		uint x_pos = 20 + (pos * m_nGridWidth);// - m_nGridWidth / 2.0;

		uint y_pos = ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3;

		// draw the "dot"
		p->drawLine(x_pos, y_pos, x_pos + 3, y_pos + 3);		// A
		p->drawLine(x_pos, y_pos, x_pos - 3, y_pos + 3);		// B
		p->drawLine(x_pos, y_pos + 6, x_pos + 3, y_pos + 3);	// C
		p->drawLine(x_pos - 3, y_pos + 3, x_pos, y_pos + 6);	// D

		p->drawLine(x_pos, y_pos + 1, x_pos + 2, y_pos + 3);
		p->drawLine(x_pos, y_pos + 1, x_pos - 2, y_pos + 3);
		p->drawLine(x_pos, y_pos + 5, x_pos + 2, y_pos + 3);
		p->drawLine(x_pos - 2, y_pos + 3, x_pos, y_pos + 5);

		p->drawLine(x_pos, y_pos + 2, x_pos + 1, y_pos + 3);
		p->drawLine(x_pos, y_pos + 2, x_pos - 1, y_pos + 3);
		p->drawLine(x_pos, y_pos + 4, x_pos + 1, y_pos + 3);
		p->drawLine(x_pos - 1, y_pos + 3, x_pos, y_pos + 4);
	}
	else {
		uint x = 20 + (pos * m_nGridWidth);
		int w = m_nGridWidth * note->m_nLength;
		w = w - 1;	// lascio un piccolo spazio tra una nota ed un altra

		int y = (int) ( ( nInstrument ) * m_nGridHeight  + (m_nGridHeight / 100.0 * 30.0) );
		int h = (int) (m_nGridHeight - ((m_nGridHeight / 100.0 * 30.0) * 2.0) );

		p->fillRect( x, y + 1, w, h + 1, QColor(100, 100, 200) );	/// \todo: definire questo colore nelle preferenze
		p->drawRect( x, y + 1, w, h + 1 );
	}
}




void DrumPatternEditor::drawGrid( QPainter* p )
{
	//cout << "draw grid" << endl;
	static const UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	static const QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	static const QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	static const QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	static const QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	static const QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	// vertical lines
	p->setPen( QPen( res_1, 0, Qt::DotLine ) );

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
		nNotes = m_pPattern->m_nSize;
	}
	if (!m_bUseTriplets) {
		for ( int i = 0; i < nNotes + 1; i++ ) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (m_nResolution >= 4) {
					p->setPen( QPen( res_1, 0 ) );
					p->drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (m_nResolution >= 8) {
					p->setPen( QPen( res_2, 0 ) );
					p->drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (m_nResolution >= 16) {
					p->setPen( QPen( res_3, 0 ) );
					p->drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (m_nResolution >= 32) {
					p->setPen( QPen( res_4, 0 ) );
					p->drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (m_nResolution >= 64) {
					p->setPen( QPen( res_5, 0 ) );
					p->drawLine(x, 1, x, m_nEditorHeight - 1);
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
					p->setPen( QPen( res_1, 0 ) );
				}
				else {
					p->setPen( QPen( res_3, 0 ) );
				}
				p->drawLine(x, 1, x, m_nEditorHeight - 1);
				nCounter++;
			}
		}
	}


	// fill the first half of the rect with a solid color
	static const QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	static const QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	Song *pSong = Hydrogen::getInstance()->getSong();
	int nInstruments = pSong->getInstrumentList()->getSize();
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + 1;
		if ( i == nSelectedInstrument ) {
			p->fillRect( 0, y, (20 + nNotes * m_nGridWidth), m_nGridHeight * 0.7, selectedRowColor );
		}
		else {
			p->fillRect( 0, y, (20 + nNotes * m_nGridWidth), m_nGridHeight * 0.7, backgroundColor );
		}
	}

}


void DrumPatternEditor::createBackground()
{
	//cout << "recreate background" << endl;

	static const UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	static const QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	static const QColor alternateRowColor( pStyle->m_patternEditor_alternateRowColor.getRed(), pStyle->m_patternEditor_alternateRowColor.getGreen(), pStyle->m_patternEditor_alternateRowColor.getBlue() );
	static const QColor lineColor( pStyle->m_patternEditor_lineColor.getRed(), pStyle->m_patternEditor_lineColor.getGreen(), pStyle->m_patternEditor_lineColor.getBlue() );


	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->m_nSize;
	}


	Song *pSong = Hydrogen::getInstance()->getSong();
	int nInstruments = pSong->getInstrumentList()->getSize();

	if ( m_nEditorHeight != (int)( m_nGridHeight * nInstruments ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * nInstruments;
		resize( width(), m_nEditorHeight );
	}



	//	m_pBackground->fill( QColor( 0,0,0 ) );
	QPainter p( m_pBackground );

	p.fillRect(0, 0, 20 + nNotes * m_nGridWidth, height(), backgroundColor);
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i;
		if ( ( i % 2) != 0) {
			p.fillRect( 0, y, (20 + nNotes * m_nGridWidth), m_nGridHeight, alternateRowColor );
		}

	}

	// horizontal lines
	p.setPen( lineColor );
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + m_nGridHeight;
		p.drawLine( 0, y, (20 + nNotes * m_nGridWidth), y);
	}


	p.drawLine( 0, m_nEditorHeight, (20 + nNotes * m_nGridWidth), m_nEditorHeight );
}



void DrumPatternEditor::paintEvent( QPaintEvent *ev )
{
	//INFOLOG( "paint" );
	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pTemp, ev->rect() );
}






void DrumPatternEditor::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
	updateEditor();
}



void DrumPatternEditor::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
//	updateStart(false);
}



void DrumPatternEditor::setResolution(uint res, bool bUseTriplets)
{
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;

	// redraw all
	createBackground();
	drawPattern();
	update( 0, 0, width(), height() );
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	//m_pPatternEditorPanel->getPitchEditor()->updateEditor();
	/// \todo [DrumPatternEditor::setResolution] aggiornare la risoluzione del Ruler in alto."
}


void DrumPatternEditor::zoomIn()
{
	m_nGridWidth = m_nGridWidth * 2;
	updateEditor();
}



void DrumPatternEditor::zoomOut()
{
	m_nGridWidth = m_nGridWidth / 2;
	if (m_nGridWidth < 3) {
		m_nGridWidth = 3;
	}
	updateEditor();
}


void DrumPatternEditor::selectedInstrumentChangedEvent()
{
	//cout << "instrument changed" << endl;
	drawPattern();
	update( 0, 0, width(), height() );
}


/// This method is called from another thread (audio engine)
void DrumPatternEditor::patternModifiedEvent()
{
	//cout << "pattern modified" << endl;
	drawPattern();
	update( 0, 0, width(), height() );
}


void DrumPatternEditor::patternChangedEvent()
{
	//cout << "pattern changed EVENT" << endl;
	//updateEditor();
}

void DrumPatternEditor::selectedPatternChangedEvent()
{
	//cout << "selected pattern changed EVENT" << endl;
	updateEditor();
}








