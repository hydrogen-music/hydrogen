/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef SELECTION_H
#define SELECTION_H

#include <QtGui>
#include <QWidget>
#include <QMouseEvent>
#include <QApplication>
#include <QScrollArea>
#include <vector>
#include <set>
#include <QDebug>
#include <cassert>
#include <memory>

#include <core/config.h>
#include <core/Preferences/Preferences.h>
#include "Compatibility/MouseEvent.h"

namespace H2Core {
	class Pattern;
}

//! SelectionWidget defines the interface used by the Selection manager to communicate with a widget
//! implementing selection, and provides for event translation, testing for intersection with selectable
//! objects, keyboard input cursor geometry, and screen refresh. It must be subclassed and
//! implemented by any widget which uses the Selection class.
//!

/** \ingroup docGUI*/
template <class Elem>
class SelectionWidget {
public:

	typedef Elem SelectionIndex;

	//! Find list of elements which intersect a rectangular area. This may be a selection lasso rectangle, or
	//! a single point such as a mouse click. This should concern itself only with the geometry.
	virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) = 0;

		virtual bool canMoveElements() const { return true; }

	//! Calculate screen space occupied by keyboard cursor
	virtual QRect getKeyboardCursorRect() = 0;

	//! Ensure that the Selection contains only valid elements.
	virtual void validateSelection() = 0;

	//! Selection or selection-related visual elements have changed, widget needs to be updated.
	//! At a minimum, the widget's own update() method should be called.
	virtual void updateWidget() = 0;

		/** Retrieves a resolution-dependent margin determining how many pixel a
		 * note is allowed to be away from mouse cursor to still be selected. */
		virtual int getCursorMargin( QInputEvent* pEvent ) const = 0;

		/** Retrieve all elements currently hovered by the mouse pointer. To
		 * ease selection in the pattern editors, a resolution-dependent margin
		 * @a nCursorMargin can be provided. The closest elements within this
		 * margin will be selected. */
		virtual std::vector<SelectionIndex> getElementsAtPoint(
			const QPoint& point, int nCursorMargin,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr ) = 0;

	//! Inform the client that we're deselecting elements.
	virtual bool checkDeselectElements( const std::vector<SelectionIndex>& elements ) {
		return true;
	}


	//! @name User-level mouse events
	//! Inform client of user-level mouse actions.
	//!
	//!   - Clicks are delivered as a single event callback
	//!   - Drags have separate events for start, update and end
	//!
	//! Additionally, dragging a selection is managed by the Selection class and only an 'end' event is
	//! delivered to the client.
	//!
	//! @{
	virtual void mouseClickEvent( QMouseEvent *ev ) = 0;
	virtual void mouseDrawStartEvent( QMouseEvent *ev ) = 0;
	virtual void mouseDrawUpdateEvent( QMouseEvent *ev ) = 0;
	virtual void mouseDrawEndEvent( QMouseEvent *ev ) = 0;
	virtual void selectionMoveUpdateEvent( QMouseEvent *ev ) {};
	virtual void selectionMoveEndEvent( QInputEvent *ev ) = 0;
	virtual void selectionMoveCancelEvent() {};
	//! @}

	//! @name Mouse gesture hooks
	//! Hooks for extra actons (eg. changing the mouse cursor) at the beginning and end of mouse gestures.
	//! @{
	virtual void startMouseLasso( QMouseEvent *ev ) {}
	virtual void startMouseMove( QMouseEvent *ev ) {}
	virtual void endMouseGesture() {}
	//! @}

	//! Find the QScrollArea, if any, which contains the widget. This will be used to scroll the widget during
	//! drag operations.
	virtual QScrollArea *findScrollArea() {
		QWidget *pThisWidget = dynamic_cast< QWidget *>( this );
		if ( pThisWidget ) {
			QWidget *pParent = dynamic_cast< QWidget *>( pThisWidget->parent() );
			if ( pParent ) {
				QScrollArea *pScrollArea = dynamic_cast< QScrollArea *>( pParent->parent() );
				if ( pScrollArea ) {
					return pScrollArea;
				}
			}
		}
		return nullptr;
	}

};


//! Drag scroller object. When attached to a QScrollArea, this will scroll the widget whenever the mouse
//! cursor goes out of bounds.
//!
//! Scrolling is timer-driven to keep a predictable and uniform scroll rate, which increases the further out
//! of bounds the user moves the cursor.
/** \ingroup docGUI*/
class DragScroller : public QObject {
	Q_OBJECT
	QTimer *m_pTimer;
	QScrollArea *m_pScrollArea;
	const int m_nInterval = 20; // ms

public:
	DragScroller( QScrollArea *pScrollArea ) {
		m_pTimer = nullptr;
		m_pScrollArea = pScrollArea;
	}

	~DragScroller() {
		if ( m_pTimer != nullptr) {
			delete m_pTimer;
		}
	}

	void startDrag() {
		if ( m_pTimer == nullptr ) {
			m_pTimer = new QTimer( this );
			m_pTimer->setInterval( m_nInterval );
			connect( m_pTimer, &QTimer::timeout, this, &DragScroller::timeout );
		}
		m_pTimer->start();
	}

	void endDrag() {
		m_pTimer->stop();
	}

public slots:
	void timeout( void ) {
		QWidget *pWidget = m_pScrollArea->widget();
		QPoint pos = pWidget->mapFromGlobal( QCursor::pos() );
		m_pScrollArea->ensureVisible( pos.x(), pos.y(), 1, 1 );
	}
};

//! @name Selection gestures
//!
//! The Selection class implements a few multi-step gestures:
//!    - Dragging a rectangular selection lasso
//!    - Dragging a selection to reposition it
//! Both of these are supported by mouse or by keyboard.
//!
//! @dot
//! digraph "states" {
//!   Idle -> MouseLasso [ label = "startDrag" ];
//!   MouseLasso -> Idle [ label = "endDrag" ];
//!   Idle -> MouseMoving [ label = "startDrag over selected" ];
//!   MouseMoving -> Idle [ label = "endDrag" ];
//!   Idle -> KeyboardLasso [ label = "Select with keyboard (shift)" ];
//!   KeyboardLasso -> Idle [ label = "any other input" ];
//!   KeyboardLasso -> KeyboardMoving [ label = "Return" ];
//!   Idle -> KeyboardMoving [ label = "Return over selected" ];
//!   KeyboardMoving -> Idle [ label = "Return or escape" ];
//! }
//! @enddot
enum SelectionState {
	Idle,
	MouseLasso,
	MouseMoving,
	KeyboardLasso,
	KeyboardMoving
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
//!   - implement the SelectionWidget interface, which Selection uses to:
//!      - find elements intersecting a selection area
//!      - get the location of the keyboard input cursor
//!      - pass on user-level mouse gestures
//!   - paint any moving elements
//!   - call paintSelection() to allow the Selection to paint a lasso
//!
//! Selections can be shared between multiple widgets providing different views of the same underlying model,
//! so long as they share the same element type.

/** \ingroup docGUI*/
template<class Elem>
class Selection {

	//! Group of SelectionWidget objects sharing the same selection set.
	//! @{
	struct SelectionGroup {
		std::set<Elem> m_selectedElements;
		std::set< SelectionWidget<Elem> *> m_selectionWidgets;
	};

	std::shared_ptr< SelectionGroup > m_pSelectionGroup;
	//! @}

private:
	SelectionWidget< Elem > *m_pWidget;

	//! @name Mouse state model
	//!
	//! The state of the mouse is modeled as a simple state machine to distinguish user-level mouse
	//! actions (click, drag) using the information available in the lower-level Qt events (mouse press,
	//! release and move). This also filters out the potential ordering effects of non-standard user
	//! behaviour, such as pressing multiple mouse buttons at once.
	//!
	//! The Qt standard drag distance and time are used to identify the transition from a click to a drag.
	//!
	//! @dot
	//! digraph "states" {
	//!   Up -> Down [ label="mousePressEvent" ];
	//!   Down -> Up [ label="mouseReleaseEvent" ];
	//!   Down -> Drag [ label="mouseMoveEvent by distance > startDragDistance() or after startDragTime()" ];
	//!   Drag -> Up [ label="mouseReleaseEvent" ];
	//! }
	//! @enddot
	//! @{
	enum MouseState { Up, Down, Dragging } m_mouseState;
	Qt::MouseButton m_mouseButton; //!< Mouse button that began the gesture
	QMouseEvent *m_pClickEvent;	   //!< Mouse event to deliver as 'click' or 'drag' events
	//! @}

	//! @{
		SelectionState m_selectionState;

	QRect m_lasso;				//!< Dimensions of a current selection lasso
	QPoint m_movingOffset;		//!< Offset that a selection has been moved by
	QRect m_keyboardCursorStart; //!< Keyboard cursor position at the start of a keyboard gesture
	//! @}

	//! For gestures modifying a selection, store the initial selection set as a checkpoint to restore when
	//! rebuilding the selection with the current lasso area.
	std::set< Elem > m_checkpointSelectedElements;

	//! Scroller to use while dragging selections
	DragScroller *m_pDragScroller;


public:

	Selection( SelectionWidget<Elem> *w ) {

		m_pWidget = w;
		m_mouseButton = Qt::NoButton;
		m_mouseState = Up;
		m_pClickEvent = nullptr;
		m_selectionState = Idle;
		m_pSelectionGroup = std::make_shared< SelectionGroup >();
		m_pSelectionGroup->m_selectionWidgets.insert( w );
		m_pDragScroller = nullptr;

	}

	//! Merge the selection groups of two Selection widgets. After this is called, the set of selected items
	//! will be shared between them, and any changes made by one widget will cause the other to be notified
	//! and potentially refreshed.
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


	//! Is there an ongoing (and incomplete) selection movement gesture?
	bool isMoving() const {
		return m_selectionState == MouseMoving || m_selectionState == KeyboardMoving;
	}

	//! Is there a mouse gesture in progress?
	bool isMouseGesture() const {
		return m_selectionState == MouseMoving || m_selectionState == MouseLasso;
	}

	//! During a selection "move" gesture, return the current movement position relative to the start
	//! position, in screen coordinates.
	QPoint movingOffset() const {
		return m_movingOffset;
	}

	//! Is there an ongoing lasso gesture?
	bool isLasso() const {
		return m_selectionState == MouseLasso || m_selectionState == KeyboardLasso;
	}

	//! Is an element in the set of currently selected elements? 
	bool isSelected( const Elem& e ) const {
		return m_pSelectionGroup->m_selectedElements.find( e ) != m_pSelectionGroup->m_selectedElements.end();
	}

	bool isEmpty() const {
		return m_pSelectionGroup->m_selectedElements.empty();
	}

	void removeFromSelection( Elem& e, bool bCheck = true ) {
		std::vector<Elem> v { e };
		if ( !bCheck || m_pWidget->checkDeselectElements( v ) ) {
			m_pSelectionGroup->m_selectedElements.erase( e );
		}
	}

	void addToSelection( const Elem& e ) {
		m_pSelectionGroup->m_selectedElements.insert( e );
	}

	void clearSelection( bool bCheck = true ) {
		std::vector<Elem> v ( m_pSelectionGroup->m_selectedElements.begin(),
							  m_pSelectionGroup->m_selectedElements.end() );
		if ( !bCheck || m_pWidget->checkDeselectElements( v ) ) {
			m_pSelectionGroup->m_selectedElements.clear();
		}
	}

	//! @name Selection iteration
	//!
	//! Shorthand iteration is provided so that ranged for loops can be used for convenience:
	//! ```
	//!   Selection<items> selection;
	//!   ...
	//!   for ( auto i : selection ) {
	//!     ...
	//! ```
	//! @{
	typedef typename std::set<Elem>::iterator iterator;

	iterator begin() { return m_pSelectionGroup->m_selectedElements.begin(); }
	iterator end() { return m_pSelectionGroup->m_selectedElements.end(); }
	//! @}

	//! Update any widgets in this selection group.
	void updateWidgetGroup() {
		for ( auto pW : m_pSelectionGroup->m_selectionWidgets ) {
			pW->updateWidget();
		}
	}

	//! Cancel any selection gesture (lasso, move, with keyboard or mouse) in progress.
	void cancelGesture() {
		if ( m_mouseState == Dragging ) {
			mouseDragEnd();
		}
		m_mouseState = Up;
		if ( m_pClickEvent ) {
			delete m_pClickEvent;
			m_pClickEvent = nullptr;
		}
		if ( m_selectionState == MouseMoving || m_selectionState == MouseLasso ) {
			m_pWidget->endMouseGesture();
			m_pWidget->selectionMoveCancelEvent();
		}
		m_selectionState = Idle;
		updateWidgetGroup();
	}

	// -------------------------------------------------------------------------------------------------------
	//! @name Raw mouse events
	//!
	//! Raw mouse events from Qt. These are handled by a state machine that models the intended user
	//! interaction including clicks and drags, with single buttons held down, and translated to meaningful
	//! user-level interaction events.
	//!
	//! These are named identically for the corresponding QWidget event handlers and should be called from
	//! those handlers of the SelectionWidget.
	//! @{

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

			auto pEv = static_cast<MouseEvent*>( ev );

#ifdef H2CORE_HAVE_QT6
			m_pClickEvent = new QMouseEvent(
				QEvent::MouseButtonPress,
				pEv->position(), pEv->scenePosition(), pEv->globalPosition(),
				m_mouseButton, ev->buttons(), ev->modifiers(),
				Qt::MouseEventSynthesizedByApplication);
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
			m_pClickEvent = new QMouseEvent(
				QEvent::MouseButtonPress,
				ev->localPos(), ev->windowPos(), ev->screenPos(),
				m_mouseButton, ev->buttons(), ev->modifiers(),
				Qt::MouseEventSynthesizedByApplication);
#else
			m_pClickEvent = new QMouseEvent(
				QEvent::MouseButtonPress,
				ev->localPos(), ev->windowPos(), ev->screenPos(),
				m_mouseButton, ev->buttons(), ev->modifiers());
#endif
			m_pClickEvent->setTimestamp( ev->timestamp() );
		}
	}

	void mouseMoveEvent( QMouseEvent *ev ) {

		auto pEv = static_cast<MouseEvent*>( ev );
		auto pClickEv = static_cast<MouseEvent*>( m_pClickEvent );

		if ( m_mouseState == Down ) {
			if ( (pEv->position() - pClickEv->position() ).manhattanLength() > QApplication::startDragDistance()
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

		if ( m_selectionState == SelectionState::KeyboardLasso ||
			 m_selectionState == SelectionState::KeyboardMoving ) {
			// There is still a former keyboard lasso active. Cancel it.
			m_selectionState = SelectionState::Idle;
		}

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

	//! @}


	//! Paint selection-related elements (ie lasso)
	void paintSelection( QPainter *painter ) {
		if ( m_selectionState == MouseLasso || m_selectionState == KeyboardLasso ) {
			QPen pen( H2Core::Preferences::get_instance()->getTheme().m_color.
					  m_selectionHighlightColor );
			pen.setStyle( Qt::DotLine );
			pen.setWidth(2);
			painter->setPen( pen );
			painter->setBrush( Qt::NoBrush );

			painter->drawRect( m_lasso );
		}
	}


	// -------------------------------------------------------------------------------------------------------
	//! @name Higher-level mouse events
	//!
	//! These events are synthesised by the MouseState state machine, and are either processed by the
	//! SelectionState state machine, or passed to the corresponding event handlers of the SelectionWidget.
	//! @{

	void mouseClick( QMouseEvent *ev ) {
		auto pEv = static_cast<MouseEvent*>( ev );
		auto pClickEv = static_cast<MouseEvent*>( m_pClickEvent );

		if ( ev->modifiers() & Qt::ControlModifier ) {
			// Ctrl+click to add or remove element from selection.
			std::vector<Elem> elems = m_pWidget->getElementsAtPoint(
				pClickEv->position().toPoint(), m_pWidget->getCursorMargin( ev ) );
			for ( Elem e : elems) {
				if ( m_pSelectionGroup->m_selectedElements.find( e )
					 == m_pSelectionGroup->m_selectedElements.end() ) {
					addToSelection( e );
				} else {
					removeFromSelection( e );
				}
			}
			updateWidgetGroup();
		} else {
			if ( ev->button() != Qt::RightButton && !m_pSelectionGroup->m_selectedElements.empty() ) {
				// Click without control or right button, and
				// non-empty selection, will just clear selection
				clearSelection();
				updateWidgetGroup();
			} else if ( ev->button() == Qt::RightButton && m_pSelectionGroup->m_selectedElements.empty() ) {
				// Right-clicking with an empty selection will first attempt to select anything at the click
				// position before passing the click through to the client.
				QRect r = QRect( pEv->position().toPoint(),
								 pEv->position().toPoint() );
				std::vector<Elem> elems = m_pWidget->elementsIntersecting( r );
				for ( Elem e : elems) {
					addToSelection( e );
				}
				m_pWidget->mouseClickEvent( ev );
			} else {
				m_pWidget->mouseClickEvent( ev );
			}
		}
	}

	void mouseDragStart( QMouseEvent *ev ) {

		if ( m_pDragScroller == nullptr ) {
			m_pDragScroller = new DragScroller( m_pWidget->findScrollArea() );
		}
		m_pDragScroller->startDrag();

		auto pEv = static_cast<MouseEvent*>( ev );
		auto pClickEv = static_cast<MouseEvent*>( m_pClickEvent );

		if ( ev->button() == Qt::LeftButton) {
			std::vector<Elem> elems = m_pWidget->getElementsAtPoint(
				pClickEv->position().toPoint(), m_pWidget->getCursorMargin( ev ) );

			/* Did the user start dragging a selected element, or an unselected element?
			 */
			bool bHitSelected = false, bHitAny = false;
			for ( Elem elem : elems ) {
				bHitAny = true;
				if ( isSelected( elem ) ) {
					bHitSelected = true;
					break;
				}
			}

			if ( bHitSelected ) {
				/* Move selection */
				if ( bHitSelected ) {
					m_selectionState = MouseMoving;
					m_movingOffset = pEv->position().toPoint() -
						pClickEv->position().toPoint();
					m_pWidget->startMouseMove( ev );
				}
			} else if ( bHitAny ) {
				// Allow mouseDrawStartEvent to handle anything

			} else {
				//  Didn't hit anything. Start new selection drag.
				m_checkpointSelectedElements = m_pSelectionGroup->m_selectedElements;
				m_selectionState = MouseLasso;
				m_lasso.setTopLeft( pClickEv->position().toPoint() );
				m_lasso.setBottomRight( pEv->position().toPoint() );
				m_pWidget->startMouseLasso( ev );
				m_pWidget->updateWidget();

			}

		}

		if ( m_selectionState == Idle ) {
			m_pWidget->mouseDrawStartEvent( ev );
		}
	}

	void mouseDragUpdate( QMouseEvent *ev ) {

		auto pEv = static_cast<MouseEvent*>( ev );
		auto pClickEv = static_cast<MouseEvent*>( m_pClickEvent );

		if ( m_selectionState == MouseLasso) {
			m_lasso.setBottomRight( pEv->position().toPoint() );

			if ( ev->modifiers() & Qt::ControlModifier ) {
				// Start with previously selected elements
				m_pSelectionGroup->m_selectedElements = m_checkpointSelectedElements;
			} else {
				// Clear and rebuild selection
				m_checkpointSelectedElements.clear();
				clearSelection();
			}
			auto selected = m_pWidget->elementsIntersecting( m_lasso );
			for ( auto s : selected ) {
				if ( m_checkpointSelectedElements.find( s ) == m_checkpointSelectedElements.end() ) {
					addToSelection( s );
				}
			}
			updateWidgetGroup();

		}
		else if ( m_selectionState == MouseMoving ) {
			m_movingOffset = pEv->position().toPoint() -
				pClickEv->position().toPoint();
			m_pWidget->selectionMoveUpdateEvent( ev );
		}
		else {
			// Pass drag update to widget
			m_pWidget->mouseDrawUpdateEvent( ev );
		}
	}

	void mouseDragEnd( QMouseEvent *ev = nullptr ) {

		m_pDragScroller->endDrag();

		if ( m_selectionState == MouseLasso) {
			m_checkpointSelectedElements.clear();
			m_selectionState = Idle;
			m_pWidget->endMouseGesture();
			updateWidgetGroup();

		} else if ( m_selectionState == MouseMoving ) {
			m_selectionState = Idle;
			m_pWidget->endMouseGesture();
			m_pWidget->selectionMoveEndEvent( ev );
			updateWidgetGroup();

		} else {
			// Pass drag end to widget
			m_pWidget->mouseDrawEndEvent( ev );
		}
	}
	//! @}



	// -------------------------------------------------------------------------------------------------------
	//! @name Keyboard interactions
	//! @{

	//! Key press event filter.
	//!
	//! Called by the client Widget to allow Selection to take some
	//! action on key presses. Must be called before the client Widget
	//! decides to take action on the key event.
	//! \returns true to indicate that the Selection claims the
	//!          keypress, false otherwise.

	bool keyPressEvent( QKeyEvent *ev ) {

		// Pressing the Alt modifier won't result in the usual move operations
		// but is consistent with a more fine-grained editing already used in
		// NotePropertiesRuler.
		auto pCleanedEvent = new QKeyEvent(
			QEvent::KeyPress, ev->key(), Qt::NoModifier, ev->text() );
		if ( ev->modifiers() & Qt::AltModifier && (
				 pCleanedEvent->matches( QKeySequence::MoveToNextChar ) ||
				 pCleanedEvent->matches( QKeySequence::SelectNextChar ) ||
				 pCleanedEvent->matches( QKeySequence::MoveToPreviousChar ) ||
				 pCleanedEvent->matches( QKeySequence::SelectPreviousChar ) ) ) {
		}

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
				m_keyboardCursorStart = m_pWidget->getKeyboardCursorRect();
				m_lasso = m_keyboardCursorStart;
			}

		} else if ( ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) &&
					m_pWidget->canMoveElements() ) {

			// Key: Enter/Return: start or end a move or copy
			if ( m_selectionState == Idle ) {

				bool bHitselected = false;
				for ( Elem e : m_pWidget->elementsIntersecting( m_pWidget->getKeyboardCursorRect() ) ) {
					if ( m_pSelectionGroup->m_selectedElements.find( e )
						 != m_pSelectionGroup->m_selectedElements.end() ) {
						bHitselected = true;
						break;
					}
				}
				if ( bHitselected ) {
					// Hit "Enter" over a selected element. Begin move.
					m_keyboardCursorStart = m_pWidget->getKeyboardCursorRect();
					m_selectionState = KeyboardMoving;
					updateWidgetGroup();
					return true;
				}

			} else if ( m_selectionState == KeyboardLasso ) {
				// If we hit 'Enter' from lasso mode, go directly to move
				m_keyboardCursorStart = m_pWidget->getKeyboardCursorRect();
				m_selectionState = KeyboardMoving;
				updateWidgetGroup();
				return true;

			} else if ( m_selectionState == KeyboardMoving ) {
				// End keyboard move
				m_selectionState = Idle;
				m_pWidget->selectionMoveEndEvent( ev );
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
					clearSelection();
					updateWidgetGroup();
					return true;
				}
			} else {
				if ( m_selectionState == MouseMoving || m_selectionState == MouseLasso ) {
					m_pWidget->endMouseGesture();
					m_pWidget->selectionMoveCancelEvent();
				}
				m_selectionState = Idle;
				updateWidgetGroup();
				return true;
			}

		} else {
			// Other keys should probably also cancel lasso, but not move?
			if ( m_selectionState == KeyboardLasso &&
				 ev->modifiers() != Qt::ShiftModifier ) {

				if ( m_selectionState != Idle ) {
					m_selectionState = Idle;
					updateWidgetGroup();
					// Cancelling lasso should not eat event.
					return false;
				}
			}
		}
		return false;
	}

		//! A means to synchronize the lassos of different widgets.
		bool syncLasso( SelectionState state, const QRect& cursorStart,
						const QRect& lasso ) {
			bool bUpdate = false;

			if ( m_selectionState != state ) {
				m_selectionState = state;
				bUpdate = true;
			}

			if ( m_keyboardCursorStart != cursorStart ) {
				m_keyboardCursorStart = cursorStart;
				if ( m_selectionState == KeyboardMoving ) {
					m_movingOffset = m_pWidget->getKeyboardCursorRect().topLeft() -
						cursorStart.topLeft();
				}
				bUpdate = true;
			}

			if ( m_lasso != lasso ) {
				m_lasso = lasso;
				bUpdate = true;
			}

			return bUpdate;
		}

		const QRect& getKeyboardCursorStart() const {
			return m_keyboardCursorStart;
		}
		const QRect& getLasso() const {
			return m_lasso;
		}
		const SelectionState& getSelectionState() const {
			return m_selectionState;
		}

		/** Scale lasso when zooming in or out. @a nOffset can compensate for a
		 * margin not affected by @a fScale. */
		void scaleLasso( float fScale, int nOffset ) {
			m_lasso.setRect( fScale * ( m_lasso.x() - nOffset ) + nOffset,
							 m_lasso.y(),
							 m_lasso.width() * fScale, m_lasso.height() );

			m_keyboardCursorStart.setRect(
				fScale * ( m_keyboardCursorStart.x() - nOffset ) + nOffset,
				m_keyboardCursorStart.y(),
				m_keyboardCursorStart.width() * fScale,
				m_keyboardCursorStart.height() );
		}

	//! Update the keyboard cursor.
	//! Called by the client widget to tell the Selection the current
	//! location of the keyboard input cursor.
	void updateKeyboardCursorPosition() {
		if ( m_selectionState == KeyboardLasso ) {
			m_lasso = m_keyboardCursorStart.united( m_pWidget->getKeyboardCursorRect() );

			// Clear and rebuild selection
			clearSelection();
			auto selected = m_pWidget->elementsIntersecting( m_lasso );
			for ( auto s : selected ) {
				m_pSelectionGroup->m_selectedElements.insert( s );
			}

		} else if ( m_selectionState == KeyboardMoving ) {
			QRect cursorPosition = m_pWidget->getKeyboardCursorRect();
			m_movingOffset = cursorPosition.topLeft() - m_keyboardCursorStart.topLeft();

		}
	}

	//! @}

};

// Implement comparison between QPoints needed for std::set. This is required
// for the #SongEditor version of the selection.
inline int operator<( QPoint a, QPoint b ) {
	int nAx = a.x(), nBx = b.x();
	if ( nAx != nBx ) {
		return nAx < nBx;
	} else {
		int nAy = a.y(), nBy = b.y();
		return nAy < nBy;
	}
}

#endif // SELECTION_H
