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
#include <cassert>


template <class Elem>
class SelectionWidget {
public:
	// Selection manager interface

	//! Selections are indexed by Note pointers.
	typedef Elem SelectionIndex;

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

	//! Selection or selection-related things have changed, widget needs to be updated.
	virtual void updateWidget() = 0;

	//! Hooks for starting and ending selection mouse gestures
	virtual void startMouseLasso() {}
	virtual void startMouseMove() {}
	virtual void endMouseGesture() {}

};


//! XXX A shared selection group
template <class Elem>
struct SelectionGroup {
	std::set<Elem> m_selectedElements;
	std::set< SelectionWidget<Elem> *> m_selectionWidgets;
};



//! Selection management for editor widgets
//!
//! This template class bundles up the functionality necessary for
//! interactive selections using mouse and keyboard.
//!
//! The Selection class:
//!   - maintains a set of selected elements
//!   - maps low-level Qt mouse events to user-level complete 'click' and 'drag' gestures
//!   - handles some clicks, drags and keyboard events to manage selection
//!   - provides an offset for 'moving' selections, so the client widget can draw moving selections
//!   - paints a selection lasso if needed
//!
//! The client widget must:
//!   - pass mouse and keyboard events to the Selection
//!   - provide query methods for
//!      - elements intersecting a selection area
//!      - providing the location of keyboard input cursor
//!      - receiving user-level mouse gestures
//!      - paint any moving elements
//!      - call paintSelection() to allow the Selection to paint a lasso

template<class Elem>
class Selection {

private:
	SelectionWidget< Elem > *widget;

	enum MouseState { Up, Down, Dragging } m_mouseState;
	Qt::MouseButton m_mouseButton;

	QMouseEvent *m_pClickEvent;
	QRect m_lasso;
	QPoint m_movingOffset;

	enum SelectionState { Idle, MouseLasso, MouseMoving, KeyboardLasso, KeyboardMoving } m_selectionState;

	std::shared_ptr< SelectionGroup< Elem > > m_pSelectionGroup;
	std::set< Elem > m_checkpointSelectedElements;

	QRect m_keyboardCursorStart;

public:

	Selection( SelectionWidget<Elem> *w ) {

		widget = w;
		m_mouseState = Up;
		m_pClickEvent = nullptr;
		m_selectionState = Idle;
		m_pSelectionGroup = std::make_shared< SelectionGroup< Elem > >();
		m_pSelectionGroup->m_selectionWidgets.insert( w );

	}

	void merge( Selection *pOther ) {
		if ( pOther != this ) {
			pOther->m_pSelectionGroup->m_selectionWidgets.insert
				( m_pSelectionGroup->m_selectionWidgets.begin(),
				  m_pSelectionGroup->m_selectionWidgets.end() );
		}
		m_pSelectionGroup = pOther->m_pSelectionGroup;
	}

	void dump() {

		qDebug() << "Mouse state: " << ( m_mouseState == Up ? "Up" :
										 m_mouseState == Down ? "Down" :
										 m_mouseState == Dragging ? "Dragging" : "-" )
				 << "\n"
				 << "button: " << m_mouseButton << "\n"
				 << "Selection state: " << ( m_selectionState == Idle ? "Idle" :
											 m_selectionState == MouseLasso ? "MouseLasso" :
											 m_selectionState == MouseMoving ? "MouseMoving" :
											 m_selectionState == KeyboardLasso ? "KeyboardLasso" :
											 m_selectionState == KeyboardMoving ? "KeyboardMoving" :
											 "-")
				 << "";

	}


	// ----------------------------------------------------------------------
	// Selection operation interfaces
	bool isMoving() const {
		return m_selectionState == MouseMoving || m_selectionState == KeyboardMoving;
	}

	QPoint movingOffset() const {
		return m_movingOffset;
	}

	bool isSelected( Elem e ) {
		return m_pSelectionGroup->m_selectedElements.find( e ) != m_pSelectionGroup->m_selectedElements.end();
	}

	typedef typename std::set<Elem>::iterator iterator;

	iterator begin() { return m_pSelectionGroup->m_selectedElements.begin(); }
	iterator end() { return m_pSelectionGroup->m_selectedElements.end(); }

	void removeFromSelection( Elem e ) {
		m_pSelectionGroup->m_selectedElements.erase( e );
	}

	void addToSelection( Elem e ) {
		m_pSelectionGroup->m_selectedElements.insert( e );
	}

	void clearSelection() {
		m_pSelectionGroup->m_selectedElements.clear();
	}

	bool checkSelectionChanged() {
		if ( m_checkpointSelectedElements != m_pSelectionGroup->m_selectedElements ) {
			m_checkpointSelectedElements = m_pSelectionGroup->m_selectedElements;
			return true;
		}
		return false;
	}

	void updateWidgetGroup() {
		for ( auto pW : m_pSelectionGroup->m_selectionWidgets ) {
			pW->updateWidget();
		}
	}

	// ---------------------------------------------------------------------
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
	// Paint selection-related elements (ie lasso)

	void paintSelection( QPainter *painter ) {
		if ( m_selectionState == MouseLasso || m_selectionState == KeyboardLasso ) {
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
				if ( m_pSelectionGroup->m_selectedElements.find( e )
					 == m_pSelectionGroup->m_selectedElements.end() ) {
					m_pSelectionGroup->m_selectedElements.insert( e );
				} else {
					m_pSelectionGroup->m_selectedElements.erase( e );
				}
			}
			updateWidgetGroup();
		} else {
			if ( ev->button() != Qt::RightButton && !m_pSelectionGroup->m_selectedElements.empty() ) {
				// Click without control or right button, and
				// non-empty selection, will just clear selection
				m_pSelectionGroup->m_selectedElements.clear();
				updateWidgetGroup();
			} else {
				widget->mouseClickEvent( ev );
			}
		}
	}

	void mouseDragStart( QMouseEvent *ev ) {
		if ( ev->button() == Qt::LeftButton) {
			QRect r = QRect( m_pClickEvent->pos(), ev->pos() );
			std::vector<Elem> elems = widget->elementsIntersecting( r );

			if ( elems.empty() ) {
				//  Didn't hit anything. Start new selection drag.
				m_checkpointSelectedElements = m_pSelectionGroup->m_selectedElements;
				m_selectionState = MouseLasso;
				m_lasso.setTopLeft( m_pClickEvent->pos() );
				m_lasso.setBottomRight( ev->pos() );
				widget->startMouseLasso();
				widget->updateWidget();

			} else {
				/* Did the user start dragging a selected element, on an unselected element?
				 */
				bool bHitselected = false;
				for ( Elem elem : elems ) {
					if ( isSelected( elem ) ) {
						bHitselected = true;
						break;
					}
				}
				/* Move selection */
				if ( bHitselected ) {
					m_selectionState = MouseMoving;
					widget->startMouseMove();
					m_movingOffset = ev->pos() - m_pClickEvent->pos();
				}
			}

		}
		widget->mouseDragStartEvent( ev );
	}

	void mouseDragUpdate( QMouseEvent *ev ) {
		if ( m_selectionState == MouseLasso) {
			m_lasso.setBottomRight( ev->pos() );

			// Clear and rebuild selection.
			m_pSelectionGroup->m_selectedElements.clear();
			if ( ev->modifiers() & Qt::ControlModifier ) {
				m_pSelectionGroup->m_selectedElements = m_checkpointSelectedElements;
			}
			auto selected = widget->elementsIntersecting( m_lasso );
			for ( auto s : selected ) {
				m_pSelectionGroup->m_selectedElements.insert( s );
			}
			updateWidgetGroup();

		} else if ( m_selectionState == MouseMoving ) {
			m_movingOffset = ev->pos() - m_pClickEvent->pos();
			updateWidgetGroup();

		} else {
			// Pass drag update to widget
			widget->mouseDragUpdateEvent( ev );
		}
	}

	void mouseDragEnd( QMouseEvent *ev ) {
		if ( m_selectionState == MouseLasso) {
			m_checkpointSelectedElements.clear();
			m_selectionState = Idle;
			widget->endMouseGesture();
			updateWidgetGroup();

		} else if ( m_selectionState == MouseMoving ) {
			m_selectionState = Idle;
			widget->endMouseGesture();
			widget->selectionMoveEndEvent( ev );
			updateWidgetGroup();

		} else {
			// Pass drag end to widget
			widget->mouseDragEndEvent( ev );
		}
	}


	// ----------------------------------------------------------------------
	// Keyboard interactions

	//! Key press event filter.

	//! Called by the client Widget to allow Selection to take some
	//! action on key presses. Must be called before the client Widget
	//! decides to take action on the key event.
	//! \returns true to indicate that the Selection claims the
	//!          keypress, false otherwise.

	bool keyPressEvent( QKeyEvent *ev ) {

		if ( ev->matches( QKeySequence::SelectNextChar )
			 || ev->matches( QKeySequence::SelectPreviousChar )
			 || ev->matches( QKeySequence::SelectNextLine )
			 || ev->matches( QKeySequence::SelectPreviousLine )
			 || ev->matches( QKeySequence::SelectStartOfLine)
			 || ev->matches( QKeySequence::SelectEndOfLine )
			 || ev->matches( QKeySequence::SelectStartOfDocument )
			 || ev->matches( QKeySequence::SelectEndOfDocument ) ) {
			// Selection keys will start or continue a selection lasso

			if ( m_selectionState == KeyboardLasso ) {
				// Already in keyboard lasso state, just moving the
				// current selection. Wait for Widget to update
				// keyboard cursor position before updating lasso
				// dimensions.
			} else {
				// Begin keyboard cursor lasso.
				m_selectionState = KeyboardLasso;
				m_keyboardCursorStart = widget->getKeyboardCursorRect();
				m_lasso = m_keyboardCursorStart;
			}

		} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {

			// Key: Enter/Return: start or end a move or copy
			if ( m_selectionState == Idle ) {

				bool bHitselected = false;
				for ( Elem e : widget->elementsIntersecting( widget->getKeyboardCursorRect() ) ) {
					if ( m_pSelectionGroup->m_selectedElements.find( e )
						 != m_pSelectionGroup->m_selectedElements.end() ) {
						bHitselected = true;
						break;
					}
				}
				if ( bHitselected ) {
					// Hit "Enter" over a selected element. Begin move.
					m_keyboardCursorStart = widget->getKeyboardCursorRect();
					m_selectionState = KeyboardMoving;
					updateWidgetGroup();
					return true;
				}

			} else if ( m_selectionState == KeyboardLasso ) {
				// If we hit 'Enter' from lasso mode, go directly to move
				m_keyboardCursorStart = widget->getKeyboardCursorRect();
				m_selectionState = KeyboardMoving;
				updateWidgetGroup();
				return true;

			} else if ( m_selectionState == KeyboardMoving ) {
				// End keyboard move
				m_selectionState = Idle;
				widget->selectionMoveEndEvent( ev );
				return true;

			} else if ( m_selectionState == KeyboardLasso ) {
				// end keyboard lasso
				m_selectionState = Idle;
				return true;
			}

		} else if ( ev->key() == Qt::Key_Escape ) {

			// Key: Escape: cancel any lasso or move/copy in progress; or clear any selection.
			if ( m_selectionState == Idle ) {
				if ( !m_pSelectionGroup->m_selectedElements.empty() ) {
					m_pSelectionGroup->m_selectedElements.clear();
					updateWidgetGroup();
					return true;
				}
			} else {
				if ( m_selectionState == MouseMoving || m_selectionState == MouseLasso ) {
					widget->endMouseGesture();
				}
				m_selectionState = Idle;
				updateWidgetGroup();
				return true;
			}

		} else {
			// Other keys should probably also cancel lasso, but not move?
			if ( m_selectionState == KeyboardLasso ) {
				m_selectionState = Idle;
				updateWidgetGroup();
			}

		}
		return false;
	}


	//! Update the keyboard cursor.
	//! Called by the client widget to tell the Selection the current
	//! location of the keyboard input cursor.
	void updateKeyboardCursorPosition( QRect cursor ) {
		if ( m_selectionState == KeyboardLasso ) {
			m_lasso = m_keyboardCursorStart.united( widget->getKeyboardCursorRect() );

			// Clear and rebuild selection
			m_pSelectionGroup->m_selectedElements.clear();
			auto selected = widget->elementsIntersecting( m_lasso );
			for ( auto s : selected ) {
				m_pSelectionGroup->m_selectedElements.insert( s );
			}

		} else if ( m_selectionState == KeyboardMoving ) {
			QRect cursorPosition = widget->getKeyboardCursorRect();
			m_movingOffset = cursorPosition.topLeft() - m_keyboardCursorStart.topLeft();

		}
	}

};

#endif // SELECTION_H
