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
	void setResolution( uint res, bool bUseTriplets );
	uint getResolution() { return m_nResolution; }
	bool isUsingTriplets() { return m_bUseTriplets;	}

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
	virtual void copy() = 0;
	virtual void paste() = 0;
	virtual void cut() = 0;
	virtual void selectInstrumentNotes( int nInstrument );


protected:

	//! Granularity of grid positioning (in ticks)
	int granularity() const {
		int nBase;
		if (m_bUseTriplets) {
			nBase = 3;
		}
		else {
			nBase = 4;
		}
		return 4 * MAX_NOTES / ( nBase * m_nResolution );
	}

	uint m_nEditorHeight;
	uint m_nEditorWidth;

	float m_nGridWidth;
	unsigned m_nGridHeight;

	int m_nSelectedPatternNumber;
	H2Core::Pattern *m_pPattern;

	const int m_nMargin = 20;

	uint m_nResolution;
	bool m_bUseTriplets;
	bool m_bFineGrained;
	bool m_bCopyNotMove;

	bool m_bSelectNewNotes;
	H2Core::Note *m_pDraggedNote;

	PatternEditorPanel *m_pPatternEditorPanel;
	QMenu *m_pPopupMenu;

	int getColumn( int x ) const;
	QPoint movingGridOffset() const;

	//! Draw lines for note grid.
	void drawGridLines( QPainter &p, Qt::PenStyle style = Qt::SolidLine ) const;

	//! Colour to use for outlining selected notes
	QColor selectedNoteColor( const H2Core::UIStyle *pStyle );

	//! Update current pattern information
	void updatePatternInfo();

};

#endif // PATERN_EDITOR_H
