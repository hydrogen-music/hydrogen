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
#include "PianoRollEditor.h"

NotePropertiesRuler::NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode )
 : QWidget( parent )
 , Object( "NotePropertiesRuler" )
 , m_mode( mode )
 , m_pPatternEditorPanel( pPatternEditorPanel )
 , m_pPattern( NULL )
{
	//infoLog("INIT");
	//setAttribute(Qt::WA_NoBackground);

	m_nGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
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
	else if ( m_mode == NOTEKEY ) {
		m_nEditorHeight = 210;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumSize( m_nEditorWidth, m_nEditorHeight );

	m_pBackground = new QPixmap( m_nEditorWidth, m_nEditorHeight );

//m_pBackground->load("/patternEditor/Klaviaturklein.png");
	updateEditor();
	show();

	HydrogenApp::get_instance()->addEventListener( this );
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
	int keyval = val;
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

		if ( m_mode == VELOCITY && !pNote->get_noteoff() ) {
			pNote->set_velocity( val );

			char valueChar[100];
			sprintf( valueChar, "%#.2f",  val);
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
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
					HydrogenApp::get_instance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
				} else if (pNote->get_leadlag() > 0.0) {
					sprintf( valueChar, "%.2f",  ( pNote->get_leadlag() * 5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
					HydrogenApp::get_instance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
				} else {
					HydrogenApp::get_instance()->setStatusBarMessage( QString("Note on beat"), 2000 );
				}

			}
		}

		else if ( m_mode == NOTEKEY ){
			if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				;
			} else {
				//set the note hight
				//QMessageBox::information ( this, "Hydrogen", trUtf8( "val: %1" ).arg(keyval)  );
				if (keyval >= 6 && keyval <= 15 ){//note c
					pNote->m_noteKey.m_key = H2Core::NoteKey::C;
				}
				if (keyval >= 16 && keyval <= 25 ){//note cis / cs
					pNote->m_noteKey.m_key = H2Core::NoteKey::Cs;
				}
				if (keyval >= 26 && keyval <= 35 ){//note d
					pNote->m_noteKey.m_key = H2Core::NoteKey::D;
				}
				if (keyval >= 36 && keyval <= 45 ){//note dis / ef
					pNote->m_noteKey.m_key = H2Core::NoteKey::Ef;
				}
				if (keyval >= 46 && keyval <= 55 ){//note E
					pNote->m_noteKey.m_key = H2Core::NoteKey::E;
				}
				if (keyval >= 56 && keyval <= 65 ){//note f
					pNote->m_noteKey.m_key = H2Core::NoteKey::F;
				}
				if (keyval >= 66 && keyval <= 75 ){//note fis
					pNote->m_noteKey.m_key = H2Core::NoteKey::Fs;
				}
				if (keyval >= 76 && keyval <= 85 ){//note g
					pNote->m_noteKey.m_key = H2Core::NoteKey::G;
				}
				if (keyval >= 86 && keyval <= 95 ){//note gis / af
					pNote->m_noteKey.m_key = H2Core::NoteKey::Af;
				}
				if (keyval >= 96 && keyval <= 105 ){//note a
					pNote->m_noteKey.m_key = H2Core::NoteKey::A;
				}
				if (keyval >= 106 && keyval <= 115 ){//note his / bf
					pNote->m_noteKey.m_key = H2Core::NoteKey::Bf;
				}
				if (keyval >= 116 && keyval <= 125 ){//note h / b
					pNote->m_noteKey.m_key = H2Core::NoteKey::B;
				}
				
				//set the note oktave 
				if (keyval >= 135 && keyval <= 145 ){
					pNote->m_noteKey.m_nOctave = -3;
				}
				else if( keyval >= 146 && keyval <= 155 ){
					pNote->m_noteKey.m_nOctave = -2;
				}
				else if( keyval >= 156 && keyval <= 165 ){
					pNote->m_noteKey.m_nOctave = -1;
				}
				else if( keyval >= 166 && keyval <= 175 ){
					pNote->m_noteKey.m_nOctave = 0;
				}
				else if( keyval >= 176 && keyval <= 185 ){
					pNote->m_noteKey.m_nOctave = 1;
				}
				else if( keyval >= 186 && keyval <= 195 ){
					pNote->m_noteKey.m_nOctave = 2;
				}
				else if( keyval >= 196 && keyval <= 205 ){
					pNote->m_noteKey.m_nOctave = 3;
				}
			}
		}



		pSong->__is_modified = true;
		updateEditor();
		break;
	}
		m_pPatternEditorPanel->getPianoRollEditor()->updateEditor();
		pPatternEditor->updateEditor();
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
		if ( m_mode == VELOCITY && !pNote->get_noteoff() ) {
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
			( HydrogenApp::get_instance() )->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
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
				HydrogenApp::get_instance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
			} else if (pNote->get_leadlag() > 0.0) {
				sprintf( valueChar, "%.2f",  ( pNote->get_leadlag() * 5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
				HydrogenApp::get_instance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
			} else {
				HydrogenApp::get_instance()->setStatusBarMessage( QString("Note on beat"), 2000 );
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

	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();

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
			Note *pposNote = pos->second;
			assert( pposNote );
			uint pos = pposNote->get_position();

			std::multimap <int, Note*>::iterator copos;
			int xoffset = 0;
			for ( copos = m_pPattern->note_map.lower_bound( pos ); copos != m_pPattern->note_map.upper_bound( pos ); ++copos ) {
				Note *pNote = copos->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
					continue;
				}

				uint x_pos = 20 + pos * m_nGridWidth;
	
				uint line_end = height();
	
				uint velocity = (uint)(pNote->get_velocity() * height());
				uint line_start = line_end - velocity;
			
				QColor centerColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
	
				int nLineWidth = 3;
				p.fillRect( x_pos - 1 + xoffset, line_start, nLineWidth,  line_end - line_start , centerColor );
				xoffset++;
			}
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


	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();

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
			Note *pposNote = pos->second;
			assert( pposNote );
			uint pos = pposNote->get_position();

			std::multimap <int, Note*>::iterator copos;
			int xoffset = 0;
			for ( copos = m_pPattern->note_map.lower_bound( pos ); copos != m_pPattern->note_map.upper_bound( pos ); ++copos ) {
				Note *pNote = copos->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
					continue;
				}

				uint x_pos = 20 + pNote->get_position() * m_nGridWidth;

	
				QColor centerColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
	
				if (pNote->get_pan_r() == pNote->get_pan_l()) {
					// pan value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( centerColor ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
				} else {
					int y_start = (int)( pNote->get_pan_l() * height() );
					int y_end = (int)( height() - pNote->get_pan_r() * height() );
	
					int nLineWidth = 3;
					p.fillRect( x_pos - 1 + xoffset, y_start, nLineWidth, y_end - y_start, QColor(  centerColor) );
	
					p.fillRect( x_pos - 1 + xoffset, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor(  centerColor ) );
				}
				xoffset++;
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
 
 
	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	
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

			Note *pposNote = pos->second;
			assert( pposNote );
			uint pos = pposNote->get_position();

			std::multimap <int, Note*>::iterator copos;
			int xoffset = 0;
			for ( copos = m_pPattern->note_map.lower_bound( pos ); copos != m_pPattern->note_map.upper_bound( pos ); ++copos ) {
				Note *pNote = copos->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
					continue;
				}

				uint x_pos = 20 + pNote->get_position() * m_nGridWidth;

				int red1 = (int) (pNote->get_velocity() * 255);
				int green1;
				int blue1;
				blue1 = ( 255 - (int) red1 )* .33;
				green1 =  ( 255 - (int) red1 );
	
				if (pNote->get_leadlag() == 0) {
				
					// leadlag value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( 0 , 0 , 0 ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
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
					p.fillRect( x_pos - 1 + xoffset, y_start, nLineWidth, y_end - y_start, QColor( red, green ,blue ) );
		
					p.fillRect( x_pos - 1 + xoffset, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor( red1, green1 ,blue1 ) );
				}
			xoffset++;
 			}
		}
	}
 
	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}



void NotePropertiesRuler::createNoteKeyBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}

	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();

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
			pStyle->m_patternEditor_backgroundColor.getRed() - 100,
			pStyle->m_patternEditor_backgroundColor.getGreen() - 100,
			pStyle->m_patternEditor_backgroundColor.getBlue() - 100
	);

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}
	QPainter p( pixmap );


	p.fillRect( 0, 0, width(), height(), QColor(0,0,0) );
	p.fillRect( 0, 0, 20 + nNotes * m_nGridWidth, height(), backgroundColor );


	p.setPen( horizLinesColor );
	for (unsigned y = 10; y < 80; y = y + 10 ) {
		p.setPen( QPen( res_1, 1, Qt::DashLine ) );
		if (y == 40) p.setPen( QPen( QColor(0,0,0), 1, Qt::SolidLine ) );
		p.drawLine(20, y, 20 + nNotes * m_nGridWidth, y);
/*		if (y == 20 )p.drawText ( 5, y +2 , QString("O")); 
		if (y == 30 )p.drawText ( 6, y +2 , QString("c"));
		if (y == 40 )p.drawText ( 7, y +2 , QString("t"));
		if (y == 50 )p.drawText ( 5, y +2 , QString("a")); 
		if (y == 60 )p.drawText ( 6, y +2 , QString("v"));
		if (y == 70 )p.drawText ( 6, y +2 , QString("e"));
*/
	}

	for (unsigned y = 90; y < 210; y = y + 10 ) {
		p.setPen( QPen( QColor( 255, 255, 255 ), 9, Qt::SolidLine, Qt::FlatCap) );
		if ( y == 100 ||y == 120 ||y == 140 ||y == 170 ||y == 190) 
			p.setPen( QPen( QColor( 0, 0, 0 ), 7, Qt::SolidLine, Qt::FlatCap ) ); 
		p.drawLine(20, y, 20 + nNotes * m_nGridWidth, y);
/*		if (y == 100 )p.drawText ( 5, y +2 , QString("H")); 
		if (y == 110 )p.drawText ( 6, y +2 , QString("a"));
		if (y == 120 )p.drawText ( 7, y +2 , QString("l"));
		if (y == 130 )p.drawText ( 5, y +2 , QString("f"));
		if (y == 150 )p.drawText ( 5, y +2 , QString("s")); 
		if (y == 160 )p.drawText ( 6, y +2 , QString("t"));
		if (y == 170 )p.drawText ( 7, y +2 , QString("e"));
		if (y == 180 )p.drawText ( 5, y +2 , QString("p"));
		if (y == 190 )p.drawText ( 5, y +2 , QString("s"));
*/
	}

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


	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);

//paint the oktave	
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
			
			//check the note type
			if ( !pNote->get_noteoff() ) {	
				uint x_pos = 20 + pNote->get_position() * m_nGridWidth;
	
				int oktave = 0;
				if ( pNote ) oktave = pNote->m_noteKey.m_nOctave;
	
				if (pNote->m_noteKey.m_nOctave == -3){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 70-3, 6, 6);
				}
				if (pNote->m_noteKey.m_nOctave == -2){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 60-3, 6, 6);
				}
				if (pNote->m_noteKey.m_nOctave == -1){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 50-3, 6, 6);
				}
				if (pNote->m_noteKey.m_nOctave == 0){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 40-3, 6, 6);
				}
				if (pNote->m_noteKey.m_nOctave == 1){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 30-3, 6, 6);
				}
				if (pNote->m_noteKey.m_nOctave == 2){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 20-3, 6, 6);
				}
				if (pNote->m_noteKey.m_nOctave == 3){
					p.setBrush(QColor( 99, 160, 233 ));
					p.drawEllipse( x_pos-3, 10-3, 6, 6);
				}
			}
		}
	}

//paint the note 
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

			if ( !pNote->get_noteoff() ) {
				uint x_pos = 20 + pNote->get_position() * m_nGridWidth;
	
				int oktave = 0;
				if ( pNote ) oktave = pNote->m_noteKey.m_nOctave;
	
				if (pNote->m_noteKey.m_key == 0 ){//note c
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 200-4, 8, 8);
				}
				if (pNote->m_noteKey.m_key == 1 ){//note cis
					p.setBrush(QColor( 255, 255, 255  ));
					p.drawEllipse( x_pos-3, 190-3, 6, 6);
				}
				if (pNote->m_noteKey.m_key == 2 ){//note d
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 180-4, 8, 8);
				}
				if (pNote->m_noteKey.m_key == 3 ){//note dis
					p.setBrush(QColor( 255, 255, 255));
					p.drawEllipse( x_pos-3, 170-3, 6, 6);
				}
				if (pNote->m_noteKey.m_key == 4 ){//note e
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 160-4, 8, 8);
				}
				if (pNote->m_noteKey.m_key == 5 ){//note f
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 150-4, 8, 8);
				}
				if (pNote->m_noteKey.m_key == 6 ){//note fis
					p.setBrush(QColor( 255, 255, 255));
					p.drawEllipse( x_pos-3, 140-3, 6, 6);
				}
				if (pNote->m_noteKey.m_key == 7 ){//note g
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 130-4, 8, 8);
				}
				if (pNote->m_noteKey.m_key == 8 ){//note gis
					p.setBrush(QColor( 255, 255, 255));
					p.drawEllipse( x_pos-3, 120-3, 6, 6);
				}
				if (pNote->m_noteKey.m_key == 9 ){//note a
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 110-4, 8, 8);
				}
				if (pNote->m_noteKey.m_key == 10 ){//note ais
					p.setBrush(QColor( 255, 255, 255));
					p.drawEllipse( x_pos-3, 100-3, 6, 6);
				}
				if (pNote->m_noteKey.m_key == 11 ){//note h
					p.setBrush(QColor( 0, 0, 0));
					p.drawEllipse( x_pos-4, 90-4, 8, 8);
				}
			}
		}
	}	
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
	else if ( m_mode == NOTEKEY ) {
		createNoteKeyBackground( m_pBackground );
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



