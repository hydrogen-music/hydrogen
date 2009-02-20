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

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/note.h>
using namespace H2Core;

#include <cassert>

#include "../HydrogenApp.h"

#include "NotePropertiesRuler.h"
#include "PatternEditorPanel.h"
#include "DrumPatternEditor.h"


NotePropertiesRuler::NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode )
 : QWidget( parent )
 , Object( "NotePropertiesRuler" )
 , m_mode( mode )
 , m_pPatternEditorPanel( pPatternEditorPanel )
 , m_pPattern( NULL )
{
	//infoLog("INIT");
	//setAttribute(Qt::WA_NoBackground);

	m_nGridWidth = (Preferences::getInstance())->getPatternEditorGridWidth();
	m_nEditorWidth = 20 + m_nGridWidth * ( MAX_NOTES * 4 );

	if (m_mode == VELOCITY ) {
		m_nEditorHeight = 100;
	}
	else if ( m_mode == PAN ) {
		m_nEditorHeight = 100;
	}
	else if ( m_mode == LEADLAG ) {
		m_nEditorHeight = 100;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumSize( m_nEditorWidth, m_nEditorHeight );

	m_pBackground = new QPixmap( m_nEditorWidth, m_nEditorHeight );

	updateEditor();
	show();

	HydrogenApp::getInstance()->addEventListener( this );
}




NotePropertiesRuler::~NotePropertiesRuler()
{
	//infoLog("DESTROY");
}



void NotePropertiesRuler::mousePressEvent(QMouseEvent *ev)
{
//	infoLog( "mousePressEvent()" );
	if (m_pPattern == NULL) return;

	DrumPatternEditor *pPatternEditor = m_pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int width = (m_nGridWidth * 4 *  MAX_NOTES) / ( nBase * pPatternEditor->getResolution());
	int x_pos = ev->x();
	int column;
	column = (x_pos - 20) + (width / 2);
	column = column / width;
	column = (column * 4 * MAX_NOTES) / ( nBase * pPatternEditor->getResolution() );
	float val = height() - ev->y();
	if (val > height()) {
		val = height();
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	val = val / height();

	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = (Hydrogen::get_instance())->getSong();

	std::multimap <int, Note*>::iterator pos;
	for ( pos = m_pPattern->note_map.lower_bound( column ); pos != m_pPattern->note_map.upper_bound( column ); ++pos ) {
		Note *pNote = pos->second;
		assert( pNote );
		assert( (int)pNote->get_position() == column );
		if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
			continue;
		}

		if ( m_mode == VELOCITY ) {
			pNote->set_velocity( val );

			char valueChar[100];
			sprintf( valueChar, "%#.2f",  val);
			HydrogenApp::getInstance()->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
		}
		else if ( m_mode == PAN ){
			float pan_L, pan_R;
			if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				val = 0.5;
			}
			if ( val > 0.5 ) {
				pan_L = 1.0 - val;
				pan_R = 0.5;
			}
			else {
				pan_L = 0.5;
				pan_R = val;
			}

			pNote->set_pan_l( pan_L );
			pNote->set_pan_r( pan_R );
		}
		else if ( m_mode == LEADLAG ){
			if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				pNote->set_leadlag(0.0);
			} else {
				pNote->set_leadlag((val * -2.0) + 1.0);
				char valueChar[100];
				if (pNote->get_leadlag() < 0.0) {
					sprintf( valueChar, "%.2f",  ( pNote->get_leadlag() * -5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
					HydrogenApp::getInstance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
				} else if (pNote->get_leadlag() > 0.0) {
					sprintf( valueChar, "%.2f",  ( pNote->get_leadlag() * 5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
					HydrogenApp::getInstance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
				} else {
					HydrogenApp::getInstance()->setStatusBarMessage( QString("Note on beat"), 2000 );
				}

			}
		}
		pSong->__is_modified = true;
		updateEditor();
		break;
	}
}

void NotePropertiesRuler::wheelEvent(QWheelEvent *ev)
{
//      infoLog( "mousePressEvent()" );
	if (m_pPattern == NULL) return;

	float delta;
	if (ev->modifiers() == Qt::ControlModifier) {
		delta = 0.01; // fine control
	} else { 
		delta = 0.05; // course control
	}
		
	if ( ev->delta() < 0 ) {
		delta = (delta * -1.0);
	}

	DrumPatternEditor *pPatternEditor = m_pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int width = (m_nGridWidth * 4 *  MAX_NOTES) / ( nBase * pPatternEditor->getResolution());
	int x_pos = ev->x();
	int column;
	column = (x_pos - 20) + (width / 2);
	column = column / width;
	column = (column * 4 * MAX_NOTES) / ( nBase * pPatternEditor->getResolution() );

	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = (Hydrogen::get_instance())->getSong();

	std::multimap <int, Note*>::iterator pos;
	for ( pos = m_pPattern->note_map.lower_bound( column ); pos != m_pPattern->note_map.upper_bound( column ); ++pos ) {
		Note *pNote = pos->second;
		assert( pNote );
		assert( (int)pNote->get_position() == column );
		if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
			continue;
		}
		if ( m_mode == VELOCITY ) {
			float val = pNote->get_velocity() + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}

			pNote->set_velocity(val);

			char valueChar[100];
			sprintf( valueChar, "%#.2f",  val);
			( HydrogenApp::getInstance() )->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
		}
		else if ( m_mode == PAN ){
			float pan_L, pan_R;

			float val = (pNote->get_pan_r() - pNote->get_pan_l() + 0.5) + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}
			if ( val > 0.5 ) {
				pan_L = 1.0 - val;
				pan_R = 0.5;
			}
			else {
				pan_L = 0.5;
				pan_R = val;
			}

			pNote->set_pan_l(pan_L);
			pNote->set_pan_r(pan_R);
		}
		else if ( m_mode == LEADLAG ){
			float val = (pNote->get_leadlag() - 1.0)/-2.0 + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}
			pNote->set_leadlag((val * -2.0) + 1.0);
			char valueChar[100];
			if (pNote->get_leadlag() < 0.0) {
				sprintf( valueChar, "%.2f",  ( pNote->get_leadlag() * -5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
				HydrogenApp::getInstance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
			} else if (pNote->get_leadlag() > 0.0) {
				sprintf( valueChar, "%.2f",  ( pNote->get_leadlag() * 5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
				HydrogenApp::getInstance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
			} else {
				HydrogenApp::getInstance()->setStatusBarMessage( QString("Note on beat"), 2000 );
			}
		}

		pSong->__is_modified = true;
		updateEditor();
		break;
	}
}


 void NotePropertiesRuler::mouseMoveEvent( QMouseEvent *ev )
{
//	infoLog( "mouse move" );
	mousePressEvent( ev );
}



void NotePropertiesRuler::paintEvent( QPaintEvent *ev)
{
	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pBackground, ev->rect() );
}



void NotePropertiesRuler::createVelocityBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}

	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();

	H2RGBColor valueColor(
			(int)( pStyle->m_patternEditor_backgroundColor.getRed() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getGreen() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getBlue() * ( 1 - 0.3 ) )
	);

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	QColor horizLinesColor(
			pStyle->m_patternEditor_backgroundColor.getRed() - 20,
			pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
			pStyle->m_patternEditor_backgroundColor.getBlue() - 20
	);

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}


	QPainter p( pixmap );

	p.fillRect( 0, 0, width(), height(), QColor(0,0,0) );
	p.fillRect( 0, 0, 20 + nNotes * m_nGridWidth, height(), backgroundColor );


	// vertical lines

	DrumPatternEditor *pPatternEditor = m_pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
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
	int nResolution = pPatternEditor->getResolution();


	if ( !pPatternEditor->isUsingTriplets() ) {

		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (nResolution >= 4) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
		}
	}
	else {	// Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * nResolution);

		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
				}
				p.drawLine(x, 0, x, m_nEditorHeight);
				nCounter++;
			}
		}
	}

	p.setPen( horizLinesColor );
	for (unsigned y = 0; y < m_nEditorHeight; y = y + (m_nEditorHeight / 10)) {
		p.drawLine(20, y, 20 + nNotes * m_nGridWidth, y);
	}

	// draw velocity lines
	if (m_pPattern != NULL) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();
		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.begin(); pos != m_pPattern->note_map.end(); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
				continue;
			}

			uint pos = pNote->get_position();
			uint x_pos = 20 + pos * m_nGridWidth;

			uint line_end = height();

			uint velocity = (uint)(pNote->get_velocity() * height());
			uint line_start = line_end - velocity;

			QColor sideColor(
				(int)( valueColor.getRed() * ( 1 - pNote->get_velocity() ) ),
				(int)( valueColor.getGreen() * ( 1 - pNote->get_velocity() ) ),
				(int)( valueColor.getBlue() * ( 1 - pNote->get_velocity() ) )
			);
			p.fillRect( (int)( x_pos - m_nGridWidth / 2.0 ) + 1, line_start, m_nGridWidth, line_end - line_start, sideColor );

			QColor centerColor(
				(int)( valueColor.getRed() * ( 1 - pNote->get_velocity() ) ),
				(int)( valueColor.getGreen() * ( 1 - pNote->get_velocity() ) ),
				(int)( valueColor.getBlue() * ( 1 - pNote->get_velocity() ) )
			);
			int nLineWidth = (int)( m_nGridWidth / 2.0 );
			int nSpace = (int)( ( m_nGridWidth -nLineWidth ) / 2.0 );
			p.fillRect( (int)( x_pos - nSpace ) + 1, line_start, nLineWidth, line_end - line_start, centerColor );
		}
	}
	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}



void NotePropertiesRuler::createPanBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}


	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();

	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );

	//QColor backgroundColor( 255, 255, 255 );
	QColor blackKeysColor( 240, 240, 240 );
	QColor horizLinesColor(
			pStyle->m_patternEditor_backgroundColor.getRed() - 20,
			pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
			pStyle->m_patternEditor_backgroundColor.getBlue() - 20
	);
	H2RGBColor valueColor(
			(int)( pStyle->m_patternEditor_backgroundColor.getRed() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getGreen() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getBlue() * ( 1 - 0.3 ) )
	);

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	QPainter p( pixmap );

	p.fillRect( 0, 0, width(), height(), QColor(0, 0, 0) );

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}
	p.fillRect( 0, 0, 20 + nNotes * m_nGridWidth, height(), backgroundColor );


	// central line
	p.setPen( horizLinesColor );
	p.drawLine(0, height() / 2.0, m_nEditorWidth, height() / 2.0);



	// vertical lines
	DrumPatternEditor *pPatternEditor = m_pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
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

	int nResolution = pPatternEditor->getResolution();

	if ( !pPatternEditor->isUsingTriplets() ) {

		for (uint i = 0; i < nNotes +1 ; i++) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (nResolution >= 4) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
		}
	}
	else {	// Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * nResolution);

		for (uint i = 0; i < nNotes +1; i++) {
			uint x = 20 + i * m_nGridWidth;

			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
				}
				p.drawLine(x, 0, x, m_nEditorHeight);
				nCounter++;
			}
		}
	}

	if ( m_pPattern ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();

		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.begin(); pos != m_pPattern->note_map.end(); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
				continue;
			}
			uint x_pos = 20 + pNote->get_position() * m_nGridWidth;

			if (pNote->get_pan_r() == pNote->get_pan_l()) {
				// pan value is centered - draw circle
				int y_pos = (int)( height() * 0.5 );
				p.setBrush(QColor( 0, 0, 0 ));
				p.drawEllipse( x_pos-4, y_pos-4, 8, 8);
			} else {
				int y_start = (int)( pNote->get_pan_l() * height() );
				int y_end = (int)( height() - pNote->get_pan_r() * height() );

				int nLineWidth = 3;
				p.fillRect( x_pos - 1, y_start, nLineWidth, y_end - y_start, QColor( 0, 0 ,0 ) );

				p.fillRect( x_pos - 1, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor( 0, 0 ,0 ) );
			}
		}
	}

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}

void NotePropertiesRuler::createLeadLagBackground(QPixmap *pixmap) 
{
	if ( !isVisible() ) {   
		return;
	}
 
 
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	
	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	QColor blackKeysColor( 240, 240, 240 );
	QColor horizLinesColor(
			pStyle->m_patternEditor_backgroundColor.getRed() - 20,
			pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
			pStyle->m_patternEditor_backgroundColor.getBlue() - 20
	);
	H2RGBColor valueColor(
			(int)( pStyle->m_patternEditor_backgroundColor.getRed() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getGreen() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getBlue() * ( 1 - 0.3 ) )
	);
 
	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );
 
	QPainter p( pixmap );
 
	p.fillRect( 0, 0, width(), height(), QColor(0, 0, 0) );
 
	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}
	p.fillRect( 0, 0, 20 + nNotes * m_nGridWidth, height(), backgroundColor );
 
 
	// central line
	p.setPen( horizLinesColor );
	p.drawLine(0, height() / 2.0, m_nEditorWidth, height() / 2.0);
 
 
 
	// vertical lines
	DrumPatternEditor *pPatternEditor = m_pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
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
 
	int nResolution = pPatternEditor->getResolution();
 
	if ( !pPatternEditor->isUsingTriplets() ) {
 
		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 20 + i * m_nGridWidth;
 
			if ( (i % n4th) == 0 ) {
				if (nResolution >= 4) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
		}
	}
	else {  // Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * nResolution);
 
		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 20 + i * m_nGridWidth;
 
			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
				}
				p.drawLine(x, 0, x, m_nEditorHeight);
				nCounter++;
			}
		}
	}
 
	if ( m_pPattern ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();
 
		std::multimap <int, Note*>::iterator pos;
		for ( pos = m_pPattern->note_map.begin(); pos != m_pPattern->note_map.end(); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
				continue;
			}
			uint x_pos = 20 + pNote->get_position() * m_nGridWidth;

			if (pNote->get_leadlag() == 0) {
				// leadlag value is centered - draw circle
				int y_pos = (int)( height() * 0.5 );
				p.setBrush(QColor( 0, 0, 0 ));
				p.drawEllipse( x_pos-4, y_pos-4, 8, 8);
			} else {
				int y_start = (int)( height() * 0.5 );
				int y_end = y_start + ((pNote->get_leadlag()/2) * height());
	 
				int nLineWidth = 3;
				int red;
				int green;
				int blue = (int) (pNote->get_leadlag() * 255);
				if (blue < 0)  {
					red = blue *-1;
					blue = (int) red * .33;
					green = (int) red * .33;
				} else {
					red = (int) blue * .33;
					green = (int) blue * .33;
				}
				p.fillRect( x_pos - 1, y_start, nLineWidth, y_end - y_start, QColor( red, green ,blue ) );
	 
				p.fillRect( x_pos - 1, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor( red, green ,blue ) );
			}
 
		}
	}
 
	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}


void NotePropertiesRuler::updateEditor()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}


	// update editor width
	int editorWidth;
	if ( m_pPattern ) {
		editorWidth = 20 + m_pPattern->get_length() * m_nGridWidth;
	}
	else {
		editorWidth =  20 + MAX_NOTES * m_nGridWidth;
	}
	resize( editorWidth, height() );
		
	delete m_pBackground;
	m_pBackground = new QPixmap( editorWidth, m_nEditorHeight );

	if ( m_mode == VELOCITY ) {
		createVelocityBackground( m_pBackground );
	}
	else if ( m_mode == PAN ) {
		createPanBackground( m_pBackground );
	}
	else if ( m_mode == LEADLAG ) {
		createLeadLagBackground( m_pBackground );
	}

	// redraw all
	update();
}



void NotePropertiesRuler::zoomIn()
{
	if (m_nGridWidth >= 3){
		m_nGridWidth *= 2;
	}else
	{
		m_nGridWidth *= 1.5;
	}
	updateEditor();
}



void NotePropertiesRuler::zoomOut()
{
	if ( m_nGridWidth > 1.5 ) {
		if (m_nGridWidth > 3){
			m_nGridWidth /=  2;
		}else
		{
			m_nGridWidth /= 1.5;
		}
	updateEditor();
	}
}



void NotePropertiesRuler::selectedPatternChangedEvent()
{
	updateEditor();
}



void NotePropertiesRuler::selectedInstrumentChangedEvent()
{
	updateEditor();
}



