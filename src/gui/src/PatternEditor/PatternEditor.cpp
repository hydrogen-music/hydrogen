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
	m_nEditorWidth = m_nMargin + m_nGridWidth * ( MAX_NOTES * 4 );

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

void PatternEditor::zoomIn()
{
	if (m_nGridWidth >= 3) {
		m_nGridWidth *= 2;
	} else {
		m_nGridWidth *= 1.5;
	}
	updateEditor();
}

void PatternEditor::zoomOut()
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

int PatternEditor::getColumn(QMouseEvent *ev) const
{
	int nWidth = m_nGridWidth * granularity();
	int nColumn = ( ev->x() - m_nMargin + (nWidth / 2) ) / nWidth;
	nColumn = nColumn * granularity();
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


void PatternEditor::updatePatternInfo() {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();

	m_pPattern = nullptr;
	m_nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();

	if ( pSong ) {
		PatternList *pPatternList = pSong->get_pattern_list();
		if ( ( m_nSelectedPatternNumber != -1 ) && ( m_nSelectedPatternNumber < pPatternList->size() ) ) {
			m_pPattern = pPatternList->get( m_nSelectedPatternNumber );
		}
	}
}


QPoint PatternEditor::movingGridOffset( ) const {
	QPoint rawOffset = m_selection.movingOffset();
	// Quantize offset to multiples of m_nGrid{Width,Height}
	int nQuantX = m_nGridWidth, nQuantY = m_nGridHeight;
	float nFactor = 1;
	if ( ! m_bFineGrained ) {
		nFactor = granularity();
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


//! Draw lines for note grid.
void PatternEditor::drawGridLines( QPainter &p, Qt::PenStyle style ) const
{
	static const UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	static const QColor res[5] = {
		QColor( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() ),
		QColor( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() ),
		QColor( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() ),
		QColor( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() ),
		QColor( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() ),
	};

	int nGranularity = granularity() * m_nResolution;
	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}
	int nMaxX = m_nGridWidth * nNotes + m_nMargin;

	if ( !m_bUseTriplets ) {

		// Draw vertical lines. To minimise pen colour changes (and
		// avoid unnecessary division operations), we draw them in
		// multiple passes, of successively finer spacing (and
		// advancing the colour selection at each refinement) until
		// we've drawn enough to satisfy the resolution setting.
		//
		// The drawing sequence looks something like:
		// |       |       |       |         - first pass, all 1/4 notes
		// |   :   |   :   |   :   |   :     - second pass, odd 1/8th notes
		// | . : . | . : . | . : . | . : .   - third pass, odd 1/16th notes

		uint nRes = 4;
		uint nStep = nGranularity / nRes * m_nGridWidth;

		// First, quarter note markers. All the quarter note markers must be drawn.
		if ( m_nResolution >= nRes ) {
			p.setPen( QPen( res[ 0 ], 0, style ) );
			for ( int x = m_nMargin ; x < nMaxX; x += nStep ) {
				p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			}
		}
		nRes *= 2;
		nStep /= 2;

		// For each successive set of finer-spaced lines, the even
		// lines will have already been drawn at the previous coarser
		// pitch, so only the odd numbered lines need to be drawn.
		int nColour = 1;
		while ( m_nResolution >= nRes ) {
			p.setPen( QPen( res[ nColour++ ], 0, style ) );
			for ( int x = m_nMargin + nStep; x < nMaxX; x += nStep * 2) {
				p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			}
			nRes *= 2;
			nStep /= 2;
		}

	} else {

		// Triplet style markers, we only differentiate colours on the
		// first of every triplet.
		uint nStep = granularity() * m_nGridWidth;
		p.setPen(  QPen( res[ 0 ], 0, style ) );
		for ( uint x = m_nMargin; x < nMaxX; x += nStep * 3 ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
		}
		// Second and third marks
		p.setPen(  QPen( res[ 2 ], 0, style ) );
		for ( uint x = m_nMargin + nStep; x < nMaxX; x += nStep * 3 ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + nStep, 1, x + nStep, m_nEditorHeight - 1);
		}
	}

}
