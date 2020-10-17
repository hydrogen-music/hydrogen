/*
 * Hydrogen
 * Copyright(c) 2002-2008 by the Hydrogen Team
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

#include "PatternEditor.h"

#include <hydrogen/globals.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/helpers/xml.h>

#include "../HydrogenApp.h"
#include "../EventListener.h"
#include "PatternEditorPanel.h"


using namespace std;
using namespace H2Core;


const char* PatternEditor::__class_name = "PatternEditor";


PatternEditor::PatternEditor( QWidget *pParent, const char *sClassName,
							  PatternEditorPanel *panel )
	: Object ( sClassName ), QWidget( pParent ), m_selection( this ) {
	m_nResolution = 8;
	m_bUseTriplets = false;
	m_pDraggedNote = nullptr;
	m_pPatternEditorPanel = panel;
	m_pPattern = nullptr;
	m_bSelectNewNotes = false;
	m_bFineGrained = false;
	m_bCopyNotMove = false;

	m_nGridWidth = Preferences::get_instance()->getPatternEditorGridWidth();
	m_nEditorWidth = 20 + m_nGridWidth * ( MAX_NOTES * 4 );

	setFocusPolicy(Qt::StrongFocus);

	HydrogenApp::get_instance()->addEventListener( this );

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, &PatternEditor::cut );
	m_pPopupMenu->addAction( tr( "&Copy" ), this, &PatternEditor::copy );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, &PatternEditor::paste );
	m_pPopupMenu->addAction( tr( "&Delete" ), this, &PatternEditor::deleteSelection );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &PatternEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &PatternEditor::selectNone );


}


void PatternEditor::setResolution(uint res, bool bUseTriplets)
{
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;

	// redraw all
	update( 0, 0, width(), height() );
	m_pPatternEditorPanel->updateEditors();
}

void PatternEditor::zoom_in()
{
	if (m_nGridWidth >= 3) {
		m_nGridWidth *= 2;
	} else {
		m_nGridWidth *= 1.5;
	}
	updateEditor();
}

void PatternEditor::zoom_out()
{
	if ( m_nGridWidth > 1.5 ) {
		if (m_nGridWidth > 3) {
			m_nGridWidth /= 2;
		} else {
			m_nGridWidth /= 1.5;
		}
		updateEditor();
	}
}

QColor PatternEditor::computeNoteColor( float velocity ){
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

int PatternEditor::getColumn(QMouseEvent *ev)
{
	int nBase;
	if (m_bUseTriplets) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	float nWidth = (m_nGridWidth * 4 * MAX_NOTES) / (nBase * m_nResolution);

	int x = ev->x();
	int nColumn;
	nColumn = x - 20 + (nWidth / 2);
	nColumn = nColumn / nWidth;
	nColumn = (nColumn * 4 * MAX_NOTES) / (nBase * m_nResolution);
	return nColumn;
}

void PatternEditor::selectNone()
{
	m_selection.clearSelection();
	update();
}

void PatternEditor::selectInstrumentNotes( int nInstrument )
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

void PatternEditor::mousePressEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mousePressEvent( ev );
}

void PatternEditor::mouseMoveEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	if ( m_selection.isMoving() ) {
		updateEditor( true );
	}
	m_selection.mouseMoveEvent( ev );
}

void PatternEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mouseReleaseEvent( ev );
}

void PatternEditor::updateModifiers( QInputEvent *ev ) {
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

QPoint PatternEditor::movingGridOffset( ) {
	QPoint rawOffset = m_selection.movingOffset();
	// Quantize offset to multiples of m_nGrid{Width,Height}
	int nQuantX = m_nGridWidth, nQuantY = m_nGridHeight;
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
