/*
 * Hydrogen
 * Copyright(c) 2002-2020 by the Hydrogen Team
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

#ifndef PATERN_EDITOR_H
#define PATERN_EDITOR_H

#include "../EventListener.h"
#include "../Selection.h"

#include <core/Object.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Note;
	class Pattern;
	class Instrument;
	class UIStyle;
}

class PatternEditorPanel;

//! Pattern Editor
//!
//! The PatternEditor class is an abstract base class for
//! functionality common to Pattern Editor components
//! (DrumPatternEditor, PianoRollEditor, NotePropertiesRuler).
//!
//! This covers common elements such as some selection handling,
//! timebase functions, and drawing grid lines.
//!
class PatternEditor : public QWidget,
					  public EventListener,
					  public H2Core::Object,
					  public SelectionWidget<H2Core::Note *>
{
	H2_OBJECT
	Q_OBJECT

public:
	PatternEditor( QWidget *pParent, const char *sClassName,
				   PatternEditorPanel *panel );


	//! Set the editor grid resolution, dividing a whole note into `res` subdivisions. 
	void setResolution( uint res );
	uint getResolution() const { return m_nResolution; }
	
	//void setTupletNumerator( int n ) { m_nTupletNumerator = n; }
	int	getTupletNumerator() const { return m_nTupletNumerator; }

	//void setTupletDenominator( int n ) { m_nTupletDenominator = n; }
	int	getTupletDenominator() const { return m_nTupletDenominator; }
	
	void setTupletRatio( int nTupletNumerator, int nTupletDenominator );

	float getGridWidth() const { return m_nGridWidth; }
	unsigned getGridHeight() const { return m_nGridHeight; }
	
	void setTupletResolution( int nRes, int nTupletNum, int nTupletDen) { //TODO needed?
		m_nResolution = nRes;
		m_nTupletNumerator = nTupletNum;
		m_nTupletDenominator = nTupletDen;
	 }

	//! Zoom in / out on the time axis
	void zoomIn();
	void zoomOut();

	//! Calculate colour to use for note representation based on note velocity. 
	static QColor computeNoteColor( float velocity );


	//! Merge together the selection groups of two PatternEditor objects to share a common selection.
	void mergeSelectionGroups( PatternEditor *pPatternEditor ) {
		m_selection.merge( &pPatternEditor->m_selection );
	}

	//! Ensure that the Selection contains only valid elements.
	virtual void validateSelection() override;

	//! Update the status of modifier keys in response to input events.
	virtual void updateModifiers( QInputEvent *ev );

	//! Update a widget in response to a change in selection
	virtual void updateWidget() override {
		updateEditor( true );
	}

	//! Change the mouse cursor during mouse gestures
	virtual void startMouseLasso( QMouseEvent *ev ) override {
		setCursor( Qt::CrossCursor );
	}

	virtual void startMouseMove( QMouseEvent *ev ) override {
		setCursor( Qt::DragMoveCursor );
	}

	virtual void endMouseGesture() override {
		unsetCursor();
	}

	//! Raw Qt mouse events are passed to the Selection
	virtual void mousePressEvent( QMouseEvent *ev ) override;
	virtual void mouseMoveEvent( QMouseEvent *ev ) override;
	virtual void mouseReleaseEvent( QMouseEvent *ev ) override;

protected:

	//! The Selection object.
	Selection< SelectionIndex > m_selection;

public slots:
	virtual void updateEditor( bool bPatternOnly = false ) = 0;
	virtual void selectAll() = 0;
	virtual void selectNone();
	virtual void deleteSelection() = 0;
	virtual void copy();
	virtual void paste() = 0;
	virtual void cut();
	virtual void selectInstrumentNotes( int nInstrument );


protected:

	//! Granularity of grid positioning ( = distance between grid marks, in ticks unit)
	float granularity() const { // float for tuplets
		//return (float) 4. * MAX_NOTES / ( m_nTupletNumerator * m_nResolution );
		return (float) MAX_NOTES * m_nTupletDenominator / ( m_nTupletNumerator * m_nResolution );
	}

	uint m_nEditorHeight;
	uint m_nEditorWidth;

	float m_nGridWidth;
	unsigned m_nGridHeight;

	int m_nSelectedPatternNumber;
	H2Core::Pattern *m_pPattern;

	const int m_nMargin = 20;

	uint m_nResolution;
	/** 
	* A tuplet is explicitly specified by a rational number, i.e. the fraction = m_nTupletNumerator / m_nTupletDenominator
	* in fact this fraction divides the note value returning its resultant length (in whole note units).
	* example: standard triplets 3:2 = 3/2,
	*	 in fact a single eight note under a triplet has length = 1/8 * 2/3 of whole note = 1/12 of whole note;
	* other examples: standard quintuplets: 5:4 = 5/4;
	* weird (wrongly written?) quintuplets: 5:2;
	* std quartuplets: 4:3;
	* a difficult tuplet: 5:3.
	* Note: when the TupletDenominator is hidden, a power of 2 is usually assumed (the biggest but not bigger than TupletNumerator)
	* except for quartuplets or 2-tuplets (in those cases there isn't a more used assumed TupletDenominator).
	*/
	int m_nTupletNumerator;
	int m_nTupletDenominator;
	bool m_bFineGrained;
	bool m_bCopyNotMove;

	bool m_bSelectNewNotes;
	H2Core::Note *m_pDraggedNote;

	PatternEditorPanel *m_pPatternEditorPanel;
	QMenu *m_pPopupMenu;

	int getColumn( int x ) const;
	float getFloatColumn( int x ) const;
	QPoint movingGridOffset() const;

	//! Draw lines for note grid.
	void drawGridLines( QPainter &p, Qt::PenStyle style = Qt::SolidLine ) const;

	//! Colour to use for outlining selected notes
	QColor selectedNoteColor( const H2Core::UIStyle *pStyle );

	//! Update current pattern information
	void updatePatternInfo();

};

#endif // PATERN_EDITOR_H
