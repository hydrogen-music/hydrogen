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

#include <QtGui>
#include <QWidget>
#include <QMouseEvent>
#include <QApplication>
#include <vector>
#include <set>
#include <QDebug>


// Selection management for editor widges
//

template<class Widget, class Elem>
class Selection {

private:
	Widget *widget;

	enum MouseState { Up, Down, Dragging } m_mouseState;
	Qt::MouseButton m_mouseButton;

	QMouseEvent *m_pClickEvent;
	QRect m_lasso;
	QPoint m_movingOffset;

	enum SelectionState { Idle, Lasso, Moving } m_selectionState;

	std::set<Elem> m_selectedElements;

public:

	Selection( Widget *w ) {
		widget = w;
		m_mouseState = Up;
		m_pClickEvent = nullptr;
		m_selectionState = Idle;
	}

	void dump() {
		qDebug() << "Selection state: " << ( m_mouseState == Up ? "Up" :
											 m_mouseState == Down ? "Down" :
											 m_mouseState == Dragging ? "Dragging" : "-" )
				 << "\n"
				 << "button: " << m_mouseButton << "\n"
				 << "";
	}


	// ----------------------------------------------------------------------
	// Selection operation interfaces
	bool isMoving() {
		return m_selectionState == Moving;
	}

	QPoint movingOffset() {
		return m_movingOffset;
	}

	bool isSelected( Elem e ) {
		return m_selectedElements.find( e ) != m_selectedElements.end();
	}

	typedef typename std::set<Elem>::iterator iterator;

	iterator begin() { return m_selectedElements.begin(); }
	iterator end() { return m_selectedElements.end(); }

	void removeFromSelection( Elem e ) {
		m_selectedElements.remove( e );
	}

	void addToSelection( Elem e ) {
		m_selectedElements.insert( e );
	}

	void clearSelection() {
		m_selectedElements.clear();
	}

	// ----------------------------------------------------------------------
	// Raw mouse events from Qt. These are handled by a state machine
	// that models the intended user interaction including clicks and
	// drags, with single buttons held down, and translated to
	// meaningful user-level interaction events.

	void mousePressEvent( QMouseEvent *ev ) {

		// macOS ctrl+left-click is reported as a
		// right-click. However, only the 'press' event is reported,
		// there are no move or release events. This is enough for
		// opening context menus, but not enough for drag gestures.
		if ( m_mouseState == Up
			 && ev->button() == Qt::RightButton
			 && ev->buttons() == Qt::LeftButton
			 && ev->modifiers() & Qt::MetaModifier) {
			// Deliver the event as a transient click with no effect on state
			mouseClick( ev );
			return;
		}

		// Check for inconsistent state. If there was a gesture in progress, abandon it.
		if ( !( m_mouseButton & ev->buttons() ) && m_mouseState != Up ) {
			delete m_pClickEvent;
			m_pClickEvent = nullptr;
			m_mouseState = Up;
		}

		// Detect start of click or drag event
		if ( m_mouseState == Up ) {
			m_mouseState = Down;
			m_mouseButton = ev->button();
			assert( m_pClickEvent == nullptr );
			m_pClickEvent = new QMouseEvent(QEvent::MouseButtonPress,
											ev->localPos(), ev->windowPos(), ev->screenPos(),
											m_mouseButton, ev->buttons(), ev->modifiers(),
											Qt::MouseEventSynthesizedByApplication);
			m_pClickEvent->setTimestamp( ev->timestamp() );
		}
	}

	void mouseMoveEvent( QMouseEvent *ev ) {

		if ( m_mouseState == Down ) {
			if ( (ev->pos() - m_pClickEvent->pos() ).manhattanLength() > QApplication::startDragDistance()
				 || (ev->timestamp() - m_pClickEvent->timestamp()) > QApplication::startDragTime() ) {
				// Mouse has moved far enough to consider this a drag rather than a click.
				m_mouseState = Dragging;
				mouseDragStart( m_pClickEvent );
			}
		} else if ( m_mouseState == Dragging ) {
			mouseDragUpdate( ev );
		}
	}

	void mouseReleaseEvent( QMouseEvent *ev ) {
		if ( m_mouseState != Up && !( ev->buttons() & m_mouseButton) ) {
			if ( m_mouseState == Down ) {
				mouseClick( ev );
			} else if ( m_mouseState == Dragging ) {
				mouseDragEnd( ev );
			}
			m_mouseState = Up;
			delete m_pClickEvent;
			m_pClickEvent = nullptr;

		} else {
			// Other mouse buttons may have been pressed since the
			// last click was started, and may have been released to
			// cause this event.
		}
	}


	// ----------------------------------------------------------------------
	// Paint selection-related things
	void paintSelection( QPainter *painter ) {
		if ( m_selectionState == Lasso ) {
			QPen pen( Qt::white );
			pen.setStyle( Qt::DotLine );
			pen.setWidth(2);
			painter->setPen( pen );
			painter->setBrush( Qt::NoBrush );
			painter->drawRect( m_lasso );
		}
	}


	// ----------------------------------------------------------------------
	// Higher-level mouse events -- clicks and drags
	void mouseClick( QMouseEvent *ev ) {
		if ( ev->modifiers() & Qt::ControlModifier ) {
			// Ctrl+click to add or remove element from selection.
			QRect r = QRect( ev->pos(), ev->pos() );
			std::vector<Elem> elems = widget->elementsIntersecting( r );
			for ( Elem e : elems) {
				if ( m_selectedElements.find( e ) == m_selectedElements.end() ) {
					m_selectedElements.insert( e );
				} else {
					m_selectedElements.erase( e );
				}
			}
			widget->update();
		} else {
			if ( !m_selectedElements.empty() ) {
				// Click without ctrl will clear selection
				// TODO: right-click should not clear selection as we may want context menus
				m_selectedElements.clear();
				widget->update();
			}
			widget->mouseClickEvent( ev );
		}
	}

	void mouseDragStart( QMouseEvent *ev ) {
		QRect r = QRect( m_pClickEvent->pos(), ev->pos() );
		std::vector<Elem> elems = widget->elementsIntersecting( r );

		if ( elems.empty() ) {
			/*  Didn't hit anything. Start new selection drag */
			m_selectionState = Lasso;
			m_lasso.setTopLeft( m_pClickEvent->pos() );
			m_lasso.setBottomRight( ev->pos() );
			widget->update();
		} else {
			/* Move selection */
			m_selectionState = Moving;
			m_movingOffset = ev->pos() - m_pClickEvent->pos();
		}

		widget->mouseDragStartEvent( ev );
	}
	void mouseDragUpdate( QMouseEvent *ev ) {

		if ( m_selectionState == Lasso) {
			m_lasso.setBottomRight( ev->pos() );

			// XXX Quick hack for now: clear and rebuild the entire
			// selection. This might actually be just as quick as
			// checking the difference, depending on how elementsIntersecting is implemented.
			m_selectedElements.clear();
			auto selected = widget->elementsIntersecting( m_lasso );
			for ( auto s : selected ) {
				m_selectedElements.insert( s );
			}

			widget->update();
		} else if ( m_selectionState == Moving ) {
			m_movingOffset = ev->pos() - m_pClickEvent->pos();
			widget->update();
		} else {
			// Pass drag update to widget
			widget->mouseDragUpdateEvent( ev );
		}
	}

	void mouseDragEnd( QMouseEvent *ev ) {
		if ( m_selectionState == Lasso) {
			m_selectionState = Idle;
			widget->update();
		} else if ( m_selectionState == Moving ) {
			m_selectionState = Idle;
			// XXX Call widget to do move
			widget->update();
		} else {
			// Pass drag end to widget
			widget->mouseDragEndEvent( ev );
		}
	}

};

#endif // SELECTION_H
