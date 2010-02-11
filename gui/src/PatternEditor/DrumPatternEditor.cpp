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

#include "DrumPatternEditor.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"

#include <hydrogen/globals.h>
#include <hydrogen/Song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/instrument.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/note.h>
#include <hydrogen/audio_engine.h>

#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
#include "../Skin.h"

#include <math.h>
#include <cassert>
#include <algorithm>

#include <QtGui>

using namespace std;
using namespace H2Core;

DrumPatternEditor::DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel)
 : QWidget( parent )
 , Object( "DrumPatternEditor" )
 , m_nResolution( 8 )
 , m_bUseTriplets( false )
 , m_bRightBtnPressed( false )
 , m_pDraggedNote( NULL )
 , m_pPattern( NULL )
 , m_pPatternEditorPanel( panel )
{
	//setAttribute(Qt::WA_NoBackground);
	setFocusPolicy(Qt::ClickFocus);

	m_nGridWidth = Preferences::get_instance()->getPatternEditorGridWidth();
	m_nGridHeight = Preferences::get_instance()->getPatternEditorGridHeight();

	unsigned nEditorWidth = 20 + m_nGridWidth * ( MAX_NOTES * 4 );
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;

	resize( nEditorWidth, m_nEditorHeight );

	HydrogenApp::get_instance()->addEventListener( this );
	
}



DrumPatternEditor::~DrumPatternEditor()
{
}



void DrumPatternEditor::updateEditor()
{
	Hydrogen* engine = Hydrogen::get_instance();

	// check engine state
	int state = engine->getState();
	if ( (state != STATE_READY) && (state != STATE_PLAYING) ) {
		ERRORLOG( "FIXME: skipping pattern editor update (state shoud be READY or PLAYING)" );
		return;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}


	uint nEditorWidth;
	if ( m_pPattern ) {
		nEditorWidth = 20 + m_nGridWidth * m_pPattern->get_length();
	}
	else {
		nEditorWidth = 20 + m_nGridWidth * MAX_NOTES;
	}
	resize( nEditorWidth, height() );

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
	Song *pSong = Hydrogen::get_instance()->getSong();
	int nInstruments = pSong->get_instrument_list()->get_size();

	int row = (int)( ev->y()  / (float)m_nGridHeight);
	if (row >= nInstruments) {
		return;
	}

	int nColumn = getColumn( ev );

	if ( nColumn >= (int)m_pPattern->get_length() ) {
		update( 0, 0, width(), height() );
		return;
	}
	Instrument *pSelectedInstrument = pSong->get_instrument_list()->get( row );

	if (ev->button() == Qt::LeftButton ) {
		m_bRightBtnPressed = false;
		AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

		bool bNoteAlreadyExist = false;
		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.lower_bound( nColumn ); pos != m_pPattern->note_map.upper_bound( nColumn ); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );
			if ( pNote->get_instrument() == pSelectedInstrument ) {
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
			m_pPattern->note_map.insert( std::make_pair( nPosition, pNote ) );

			// hear note
			Preferences *pref = Preferences::get_instance();
			if ( pref->getHearNewNotes() ) {
				Note *pNote2 = new Note( pSelectedInstrument, 0, fVelocity, fPan_L, fPan_R, nLength, fPitch);
				AudioEngine::get_instance()->get_sampler()->note_on(pNote2);
			}
		}
		pSong->__is_modified = true;
		AudioEngine::get_instance()->unlock(); // unlock the audio engine
	}
	else if (ev->button() == Qt::RightButton ) {
		m_bRightBtnPressed = true;
		m_pDraggedNote = NULL;

		unsigned nRealColumn = 0;
		if( ev->x() > 20 ) {
			nRealColumn = (ev->x() - 20) / static_cast<float>(m_nGridWidth);
		}

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.lower_bound( nColumn ); pos != m_pPattern->note_map.upper_bound( nColumn ); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );

			if ( pNote->get_instrument() == pSelectedInstrument ) {
				m_pDraggedNote = pNote;
				break;
			}
		}
		if ( !m_pDraggedNote ) {
			for ( pos = m_pPattern->note_map.lower_bound( nRealColumn ); pos != m_pPattern->note_map.upper_bound( nRealColumn ); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );

				if ( pNote->get_instrument() == pSelectedInstrument ) {
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
	
				
				m_pPattern->note_map.insert( std::make_pair( nPosition, poffNote ) );
	
				pSong->__is_modified = true;
			}
///
		}
		// potrei essere sulla coda di una nota precedente..
		for ( int nCol = 0; unsigned(nCol) < nRealColumn; ++nCol ) {
			if ( m_pDraggedNote ) break;
			for ( pos = m_pPattern->note_map.lower_bound( nCol ); pos != m_pPattern->note_map.upper_bound( nCol ); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );

				if ( pNote->get_instrument() == pSelectedInstrument
				    && ( (nRealColumn <= pNote->get_position() + pNote->get_length() )
				    && nRealColumn >= pNote->get_position() ) ){
					m_pDraggedNote = pNote;
					break;
				}
			}
		}
		AudioEngine::get_instance()->unlock();
	}

	// update the selected line
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	if (nSelectedInstrument != row) {
		Hydrogen::get_instance()->setSelectedInstrumentNumber( row );
	}
	else {
		update( 0, 0, width(), height() );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
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
		update( 0, 0, width(), height() );
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPanEditor()->updateEditor();
		m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
		m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	}

}



void DrumPatternEditor::keyPressEvent (QKeyEvent *ev)
{
	ev->ignore();
}



///
/// Draws a pattern
///
void DrumPatternEditor::__draw_pattern(QPainter& painter)
{
	const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	const QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );

	__create_background( painter );

	if (m_pPattern == NULL) {
		return;
	}

	int nNotes = m_pPattern->get_length();
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = Hydrogen::get_instance()->getSong();

	InstrumentList * pInstrList = pSong->get_instrument_list();

	
	if ( m_nEditorHeight != (int)( m_nGridHeight * pInstrList->get_size() ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * pInstrList->get_size();
		resize( width(), m_nEditorHeight );
	}

	for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
		uint y = m_nGridHeight * nInstr;
		if ( nInstr == (uint)nSelectedInstrument ) {	// selected instrument
			painter.fillRect( 0, y + 1, ( 20 + nNotes * m_nGridWidth ), m_nGridHeight - 1, selectedRowColor );
		}
	}


	// draw the grid
	__draw_grid( painter );
	

	/*
		BUGFIX
		
		if m_pPattern is not renewed every time we draw a note, 
		hydrogen will crash after you save a song and create a new one. 
		-smoors
	*/
	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}
	// ~ FIX



	if( m_pPattern->note_map.size() == 0) return;

	std::multimap <int, Note*>::iterator pos;
	for ( pos = m_pPattern->note_map.begin(); pos != m_pPattern->note_map.end(); pos++ ) {
		Note *note = pos->second;
		assert( note );
		__draw_note( note, painter );
	}
}



QColor DrumPatternEditor::computeNoteColor( float velocity ){
    int red;
    int green;
    int blue;


    /*
	The note gets painted black if it has the default velocity (0.8).
	The color changes if you alter the velocity..
    */

    //qDebug() << "x: " << x;
    //qDebug() << "x2: " << x*x;


    if( velocity < 0.8){
	red = fabs(-( velocity - 0.8))*255;
	green =  fabs(-( velocity - 0.8))*255;
	blue =  green * 1.25;
    } else {
	green = blue = 0;
	red = (velocity-0.8)*5*255;
    }

    //qDebug() << "R " << red << "G " << green << "blue " << blue;
    return QColor( red, green, blue );
}



///
/// Draws a note
///
void DrumPatternEditor::__draw_note( Note *note, QPainter& p )
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor noteColor( pStyle->m_patternEditor_noteColor.getRed(), pStyle->m_patternEditor_noteColor.getGreen(), pStyle->m_patternEditor_noteColor.getBlue() );
	static const QColor noteoffColor( pStyle->m_patternEditor_noteoffColor.getRed(), pStyle->m_patternEditor_noteoffColor.getGreen(), pStyle->m_patternEditor_noteoffColor.getBlue() );

	p.setRenderHint( QPainter::Antialiasing );

	int nInstrument = -1;
	InstrumentList * pInstrList = Hydrogen::get_instance()->getSong()->get_instrument_list();
	for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr == note->get_instrument() ) {
 			nInstrument = nInstr;
			break;
		}
	}
	if ( nInstrument == -1 ) {
		ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	uint pos = note->get_position();

	p.setPen( noteColor );


	QColor color = computeNoteColor( note->get_velocity() );

	uint w = 8;
	uint h =  m_nGridHeight / 3;

	if ( note->get_length() == -1 && note->get_noteoff() == false ) {	// trigger note
		uint x_pos = 20 + (pos * m_nGridWidth);// - m_nGridWidth / 2.0;
		uint y_pos = ( nInstrument * m_nGridHeight) + (m_nGridHeight / 2) - 3;
		p.setBrush( color );
		p.drawEllipse( x_pos -4 , y_pos, w, h );


	}
	else if ( note->get_length() == 1 && note->get_noteoff() == true ){
		p.setPen( noteoffColor );
		uint x_pos = 20 + ( pos * m_nGridWidth );// - m_nGridWidth / 2.0;

		uint y_pos = ( nInstrument * m_nGridHeight ) + (m_nGridHeight / 2) - 3;
		p.setBrush(QColor( noteoffColor));
		p.drawEllipse( x_pos -4 , y_pos, w, h );



	}		
	else {
		float fNotePitch = note->m_noteKey.m_nOctave * 12 + note->m_noteKey.m_key;
		float fStep = pow( 1.0594630943593, ( double )fNotePitch );

		uint x = 20 + (pos * m_nGridWidth);
		int w = m_nGridWidth * note->get_length() / fStep;
		w = w - 1;	// lascio un piccolo spazio tra una nota ed un altra

		int y = (int) ( ( nInstrument ) * m_nGridHeight  + (m_nGridHeight / 100.0 * 30.0) );
		int h = (int) (m_nGridHeight - ((m_nGridHeight / 100.0 * 30.0) * 2.0) );
		p.setBrush( color );
		p.fillRect( x, y + 1, w, h + 1, color );	/// \todo: definire questo colore nelle preferenze
		p.drawRect( x, y + 1, w, h + 1 );
	}
}




void DrumPatternEditor::__draw_grid( QPainter& p )
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	static const QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	static const QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	static const QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	static const QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	// vertical lines
	p.setPen( QPen( res_1, 0, Qt::DotLine ) );

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
					p.setPen( QPen( res_1, 0 ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (m_nResolution >= 8) {
					p.setPen( QPen( res_2, 0 ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (m_nResolution >= 16) {
					p.setPen( QPen( res_3, 0 ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (m_nResolution >= 32) {
					p.setPen( QPen( res_4, 0 ) );
					p.drawLine(x, 1, x, m_nEditorHeight - 1);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (m_nResolution >= 64) {
					p.setPen( QPen( res_5, 0 ) );
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
					p.setPen( QPen( res_1, 0 ) );
				}
				else {
					p.setPen( QPen( res_3, 0 ) );
				}
				p.drawLine(x, 1, x, m_nEditorHeight - 1);
				nCounter++;
			}
		}
	}


	// fill the first half of the rect with a solid color
	static const QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	static const QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = Hydrogen::get_instance()->getSong();
	int nInstruments = pSong->get_instrument_list()->get_size();
	for ( uint i = 0; i < (uint)nInstruments; i++ ) {
		uint y = m_nGridHeight * i + 1;
		if ( i == (uint)nSelectedInstrument ) {
			p.fillRect( 0, y, (20 + nNotes * m_nGridWidth), (int)( m_nGridHeight * 0.7 ), selectedRowColor );
		}
		else {
			p.fillRect( 0, y, (20 + nNotes * m_nGridWidth), (int)( m_nGridHeight * 0.7 ), backgroundColor );
		}
	}

}


void DrumPatternEditor::__create_background( QPainter& p)
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	static const QColor alternateRowColor( pStyle->m_patternEditor_alternateRowColor.getRed(), pStyle->m_patternEditor_alternateRowColor.getGreen(), pStyle->m_patternEditor_alternateRowColor.getBlue() );
	static const QColor lineColor( pStyle->m_patternEditor_lineColor.getRed(), pStyle->m_patternEditor_lineColor.getGreen(), pStyle->m_patternEditor_lineColor.getBlue() );

	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}

	Song *pSong = Hydrogen::get_instance()->getSong();
	int nInstruments = pSong->get_instrument_list()->get_size();

	if ( m_nEditorHeight != (int)( m_nGridHeight * nInstruments ) ) {
		// the number of instruments is changed...recreate all
		m_nEditorHeight = m_nGridHeight * nInstruments;
		resize( width(), m_nEditorHeight );
	}

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



void DrumPatternEditor::paintEvent( QPaintEvent* /*ev*/ )
{
	//INFOLOG( "paint" );
	//QWidget::paintEvent(ev);
	
	QPainter painter( this );
	__draw_pattern( painter );
}






void DrumPatternEditor::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
	updateEditor();
}



void DrumPatternEditor::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
}



void DrumPatternEditor::setResolution(uint res, bool bUseTriplets)
{
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;

	// redraw all
	update( 0, 0, width(), height() );
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPanEditor()->updateEditor();
	m_pPatternEditorPanel->getLeadLagEditor()->updateEditor();
	m_pPatternEditorPanel->getNoteKeyEditor()->updateEditor();
	/// \todo [DrumPatternEditor::setResolution] aggiornare la risoluzione del Ruler in alto."
}



void DrumPatternEditor::zoom_in()
{
	if (m_nGridWidth >= 3){
		m_nGridWidth *= 2;
	}else
	{
		m_nGridWidth *= 1.5;
	}
	updateEditor();
}



void DrumPatternEditor::zoom_out()
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

void DrumPatternEditor::selectedInstrumentChangedEvent()
{
	update( 0, 0, width(), height() );
}


/// This method is called from another thread (audio engine)
void DrumPatternEditor::patternModifiedEvent()
{
	update( 0, 0, width(), height() );
}


void DrumPatternEditor::patternChangedEvent()
{
	updateEditor();
}


void DrumPatternEditor::selectedPatternChangedEvent()
{
	//cout << "selected pattern changed EVENT" << endl;
	updateEditor();
}
