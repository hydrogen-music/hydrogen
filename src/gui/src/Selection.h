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

#ifndef SELECTION_H
#define SELECTION_H

#include <QWidget>
#include <QMouseEvent>
#include <QApplication>
#include <map>


// Selection management for editor widges
//

template<class Widget, class Location, class Elem>
class Selection {

private:
	Widget *widget;

	// Selection state
	enum SelectionState { NoSelection,
				 DraggingLasso,
				 Selected,
				 DraggingSelection
	} m_selectionState;

	enum MouseState { Up, Down, Dragging } m_mouseState;
	Qt::MouseButton m_mouseButton;

	QPoint m_clickPos;
	QEvent *m_pClickEvent;

	// Lasso: screen coords or grid coords?
	// Moving selection
	// Selected cells
	
public:
	Selection( Widget *w ) {
		widget = w;
		m_mouseState = Up;
		m_selectionState = NoSelection;
		m_pClickEvent = nullptr;
	}


	// ----------------------------------------------------------------------
	// Raw mouse events from Qt. These are handled by a state machine
	// that models the intended user interaction including clicks and
	// drags, with single buttons held down, and translated to
	// meaningful user-level interaction events.
	
	bool mousePressEvent( QMouseEvent *ev ) {
		if ( m_mouseState == Up ) {
			m_mouseState = Down;
			m_clickPos = ev->pos();
			m_mouseButton = ev->button();
			assert( m_pClickEvent = nullptr );
			m_pClickEvent = new QMouseEvent(QEvent::MouseButtonPress,
											ev->localPos(), ev->windowPos(), ev->screenPos(),
											m_mouseButton, ev->buttons(), ev->modifiers(),
											Qt::MouseEventSynthesizedByApplication);
		}
	}

	bool mouseMoveEvent( QMouseEvent *ev ) {
		if ( m_mouseState == Down ) {
			if ( (ev->pos() - m_clickPos).manhattanLength()
				 > QApplication::startDragDistance() ) {
				// Begin drag
				m_mouseState = Dragging;
				mouseDragStart( ev );
			}
		} else if ( m_mouseState == Dragging ) {
			mouseDragUpdate( ev );
		}
	}

	bool mouseReleaseEvent( QMouseEvent *ev ) {
		if ( ev->button() == m_mouseButton ) {
			if ( m_mouseState == Down ) {
				mouseClick( ev );
			} else if ( m_mouseState == Dragging ) {
				mouseDragEnd( ev );
			}
			m_mouseState = Up;
			delete m_pClickEvent;
			m_pClickEvent = nullptr;
		} else {
			// Other mouse buttons may have been pressed either before
			// the last click was started, or after the previous click
			// was begun. Ignore these.
		}
	}

	// ----------------------------------------------------------------------
	// Derived mouse events
	void mouseClick( QMouseEvent *ev ) {
	}

	void mouseDragStart( QMouseEvent *ev ) {
	}
	void mouseDragUpdate( QMouseEvent *ev ) {
	}
	void mouseDragEnd( QMouseEvent *ev ) {
	}

	

	// maps to
	// mouseClick
	// mouseStartDrag
	// mouseDragging
	// mouseEndDrag


};

#endif // SELECTION_H
