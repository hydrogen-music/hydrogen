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

#include <hydrogen/object.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Note;
	class Pattern;
	class Instrument;
}

class PatternEditorPanel;

//! Abstract pattern editor common functionality
//!
//! The PatternEditor class is an abstract base class for functionality common to
//! Pattern Editor components, specifically the DrumPatternEditor and PianoRollEditor.
//!
class PatternEditor : public QWidget, public EventListener, public H2Core::Object
{
	H2_OBJECT
		Q_OBJECT

public:
	PatternEditor( QWidget *pParent, const char *sClassName,
				   PatternEditorPanel *panel );

	void setResolution( uint res, bool bUseTriplets );
	uint getResolution() { return m_nResolution; }
	bool isUsingTriplets() { return m_bUseTriplets;	}

	//! Zoom in / out on the time axis
	void zoom_in();
	void zoom_out();

	static QColor computeNoteColor( float velocity );

	// Selection manager interface

	//! Selections are indexed by Note pointers.
	typedef H2Core::Note* SelectionIndex;

	//! Find list of elements which intersect a selection drag rectangle
	virtual std::vector<SelectionIndex> elementsIntersecting( QRect r ) = 0;

	//! Ensure that the Selection contains only valid elements
	virtual void validateSelection() = 0;

	//! Selection manager interface:
	//! called by Selection when click detected
	virtual void mouseClickEvent( QMouseEvent *ev ) = 0;

	//! Called by Selection when drag started
	virtual void mouseDragStartEvent( QMouseEvent *ev ) = 0;

	//! Called by Selection when drag position changes
	virtual void mouseDragUpdateEvent( QMouseEvent *ev ) = 0;

	//! Called by Selection when drag ends
	virtual void mouseDragEndEvent( QMouseEvent *ev ) = 0;

	//! Called by Selection when a move drag is completed.
	virtual void selectionMoveEndEvent( QInputEvent *ev ) = 0;

	//! Calculate screen position of keyboard input cursor
	virtual QRect getKeyboardCursorRect() = 0;

	//! Raw Qt mouse events are passed to the Selection
	virtual void mousePressEvent( QMouseEvent *ev );
	virtual void mouseMoveEvent( QMouseEvent *ev );
	virtual void mouseReleaseEvent( QMouseEvent *ev );

	//! Update the status of modifier keys in response to input events
	virtual void updateModifiers( QInputEvent *ev );

protected:
	Selection< PatternEditor, SelectionIndex > m_selection;

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


	//! Granularity of grid positioning
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

	unsigned m_nGridWidth;
	unsigned m_nGridHeight;

	const unsigned nMargin = 20;

	uint m_nResolution;
	bool m_bUseTriplets;
	bool m_bFineGrained;
	bool m_bCopyNotMove;

	bool m_bSelectNewNotes;
	H2Core::Note *m_pDraggedNote;

	H2Core::Pattern *m_pPattern;
	PatternEditorPanel *m_pPatternEditorPanel;
	QMenu *m_pPopupMenu;

	int getColumn(QMouseEvent *ev) const;
	QPoint movingGridOffset() const;

	// Draw lines for note grid.
	void drawGridLines( QPainter &p, Qt::PenStyle style = Qt::SolidLine ) const;

};

#endif // PATERN_EDITOR_H
