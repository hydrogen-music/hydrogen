/*
 * Hydrogen
 * Copyright(c) 2002-2020 by the Hydrogen Team
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

#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include "EditorDefs.h"
#include "../HydrogenApp.h"
#include "../Selection.h"

#include <memory>

#include <QtGui>
#include <QtWidgets>

namespace H2Core {
	class Pattern;
}

namespace Editor {

/** Base Editor
*
* The Base class is an abstract base class for functionality common to all our
* Editors (#DrumPatternEditor, #PianoRollEditor, #NotePropertiesRuler,
* #SongEditor, #AutomationPathView, #TargetWaveDisplay).
*
* This covers common elements such as
* - Keyboard arrow movement (plus modifiers)
* - Selection via keyboard and mouse
* - Highlighting of selected elements
* - Lasso rendering
* - Hovering of elements via keyboard and mouse as well as a shared cursor
*   margins.
* - Mouse and keyboard based moving of selected elements
* - Left-click moving of single elements (selected or not)
* - Right-click popup menu (if applicable)
* - Right-click drawing lines.
*
* \ingroup docGUI */
template<class Elem>
class Base : public SelectionWidget<Elem>, public QWidget
{
	public:

		Base( QWidget* pParent )
			: QWidget( pParent )
			, m_type( Type::Grid )
			, m_selection( this )
			, m_nActiveWidth( width() )
			, m_nEditorHeight( height() )
			, m_nEditorWidth( width() )
			, m_bCopyNotMove( false )
			, m_bEntered( false )
			, m_instance( Instance::None )
			, m_update( Update::Background )
			, m_pPopupMenu( new QMenu( this ) )
		{
			qreal pixelRatio = devicePixelRatio();
			m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
											   m_nEditorHeight * pixelRatio );
			m_pContentPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
											m_nEditorHeight * pixelRatio );
			m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
			m_pContentPixmap->setDevicePixelRatio( pixelRatio );
		}
		virtual ~Base() {
			if ( m_pContentPixmap != nullptr ) {
				delete m_pContentPixmap;
			}
			if ( m_pBackgroundPixmap != nullptr ) {
				delete m_pBackgroundPixmap;
			}
		}

		virtual void ensureCursorIsVisible() {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void handleElements( QInputEvent* ev, Editor::Action action ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void deleteElements( std::vector<Elem> ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void copy() {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void paste() {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void moveCursorDown( QKeyEvent* ev, Editor::Step step ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void moveCursorLeft( QKeyEvent* ev, Editor::Step step ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void moveCursorRight( QKeyEvent* ev, Editor::Step step ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void moveCursorUp( QKeyEvent* ev, Editor::Step step ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void selectAll() {
			___ERRORLOG( "To be implemented by parent" );
		}
		/** Since the cursor might be shared amongst various components of the
		 * editor, we do not store it in here. */
 		virtual QPoint getCursorPosition() {
			___ERRORLOG( "To be implemented by parent" );
			return QPoint( 0, 0 );
		}
		virtual void setCursorTo( Elem ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void setCursorTo( QMouseEvent* ev ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		/** Can be called to e.g. disable some options based on the selected
		 * elements. */
		virtual void setupPopupMenu() {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void updateKeyboardHoveredElements() {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void updateMouseHoveredElements( QMouseEvent* ev ) {
			___ERRORLOG( "To be implemented by parent" );
		}

		virtual void propertyDrawStart( QMouseEvent* ev ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void propertyDrawUpdate( QMouseEvent* ev ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		virtual void propertyDrawEnd() {
			___ERRORLOG( "To be implemented by parent" );
		}

		/** In contrast to Selection::updateWidget() this method indicates a
		 * state change in the overall editor and triggers an update of all its
		 * visible components, e.g. including its ruler and sidebar. */
		virtual void updateVisibleComponents( bool bContentOnly ) {
			___ERRORLOG( "To be implemented by parent" );
		}
		/** Updates all widgets dependent on this one. */
		virtual void updateAllComponents( bool bContentOnly ) {
			___ERRORLOG( "To be implemented by parent" );
		}

		/** Adjusts #m_nActiveWidth and #m_nEditorWidth to the current
		 * state of the editor.
		 *
		 * @returns true in case the width of the widget did change. */
		virtual bool updateWidth() {
			___ERRORLOG( "To be implemented by parent" );
			return false;
		}

		/** Updates #m_pBackgroundPixmap. */
		virtual void createBackground() {
			___ERRORLOG( "To be implemented by parent" );
		}

		//! Clear the pattern editor selection
		void clearSelection() {
			m_selection.clearSelection();
		}

		void cut() {
			copy();
			deleteSelection();
		}

		void deleteSelection() {
			// Delete selected elements.
			std::vector<Elem> elementsToDelete;
			for ( const auto& eelem : m_selection ) {
				elementsToDelete.push_back( eelem );
			}
			deleteElements( elementsToDelete );
		}

// 		/** Move or copy notes.
// 		 *
// 		 * Moves or copies notes at the end of a Selection move, handling the
// 		 * behaviours necessary for out-of-range moves or copies.*/
// 		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;

// 		//! Ensure that the Selection contains only valid elements.
// 		virtual void validateSelection() override;

		//! Update a widget in response to a change in selection
		virtual void updateWidget() override {
			updateEditor( true );
		}

		virtual void updateEditor( bool bContentOnly = true ) {
			if ( updateWidth() ) {
				m_update = Update::Background;
			}
			else if ( bContentOnly && m_update != Update::Background ) {
				// Background takes priority over Pattern.
				m_update = Update::Content;
			}
			else {
				m_update = Update::Background;
			}

			// redraw
			update();
		}

 		//! Update the status of modifier keys in response to input events.
 		virtual void updateModifiers( QInputEvent *ev ) {
			// Key: Ctrl + drag: copy notes rather than moving
			m_bCopyNotMove = ev->modifiers() & Qt::ControlModifier;

			if ( QKeyEvent* pKeyEvent = dynamic_cast<QKeyEvent*>( ev ) ) {
				// Keyboard events for press and release of modifier keys don't
				// have those keys in the modifiers set, so explicitly update
				// these.
				if ( pKeyEvent->key() == Qt::Key_Control ) {
					m_bCopyNotMove = ev->type() == QEvent::KeyPress;
				}
			}

			if ( m_selection.isMouseGesture() && m_selection.isMoving() ) {
				// If a selection is currently being moved, change the cursor
				// appropriately. Selection will change it back after the move
				// is complete (or abandoned)
				if ( m_bCopyNotMove && cursor().shape() != Qt::DragCopyCursor ) {
					setCursor( QCursor( Qt::DragCopyCursor ) );
				}
				else if ( ! m_bCopyNotMove && cursor().shape() != Qt::DragMoveCursor ) {
					setCursor( QCursor( Qt::DragMoveCursor ) );
				}
			}
		}

		void updatePixmapSize() {
			// Resize pixmap if pixel ratio has changed
			qreal pixelRatio = devicePixelRatio();
			if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
				 m_pBackgroundPixmap->height() != m_nEditorHeight ||
				 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
				delete m_pBackgroundPixmap;
				m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
												   m_nEditorHeight * pixelRatio );
				m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
				delete m_pContentPixmap;
				m_pContentPixmap = new QPixmap( m_nEditorWidth  * pixelRatio,
												m_nEditorHeight * pixelRatio );
				m_pContentPixmap->setDevicePixelRatio( pixelRatio );
			}
		}

 		virtual int getCursorMargin( QInputEvent* pEvent ) const override {
			return Editor::nDefaultCursorMargin;
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

		virtual std::vector<Elem> getElementsAtPoint(
			const QPoint& point, int nCursorMargin,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr )
		{
			___ERRORLOG( "To be implemented by parent" );
			return std::vector<Elem>();
		}

 		virtual void mousePressEvent( QMouseEvent *ev ) override {
			auto pEv = static_cast<MouseEvent*>( ev );

			updateModifiers( ev );

			m_elementsToSelectForPopup.clear();
			m_elementsHoveredForPopup.clear();
			m_elementsHoveredOnDragStart.clear();
			m_elementsToSelect.clear();

			if ( ( ev->buttons() == Qt::LeftButton ||
				   ev->buttons() == Qt::RightButton ) &&
				 ! ( ev->modifiers() & Qt::ControlModifier ) ) {

				// When interacting with note(s) not already in a selection, we
				// will discard the current selection and add these notes under
				// point to a transient one.
				const auto notesUnderPoint = getElementsAtPoint(
					pEv->position().toPoint(), getCursorMargin( ev ) );

				bool bSelectionHovered = false;
				for ( const auto& ppNote : notesUnderPoint ) {
					if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
						bSelectionHovered = true;
						break;
					}
				}

				// We honor the current selection.
				if ( bSelectionHovered ) {
					for ( const auto& ppNote : notesUnderPoint ) {
						if ( ppNote != nullptr && m_selection.isSelected( ppNote ) ) {
							m_elementsHoveredOnDragStart.push_back( ppNote );
						}
					}
				}
				else {
					m_elementsToSelect = notesUnderPoint;
					m_elementsHoveredOnDragStart = notesUnderPoint;
				}

				if ( ev->button() == Qt::RightButton ) {
					m_elementsToSelectForPopup = m_elementsToSelect;
					m_elementsHoveredForPopup = m_elementsHoveredOnDragStart;
				}

				// Property drawing in the ruler must not select notes.
				if ( ev->button() == Qt::RightButton ) {
					m_elementsToSelect.clear();
				}
			}

			// propagate event to selection. This could very well cancel a lasso
			// created via keyboard events.
			m_selection.mousePressEvent( ev );

			// Hide cursor in case this behavior was selected in the
			// Preferences.
			handleKeyboardCursor( false );
		}

 		virtual void mouseMoveEvent( QMouseEvent *ev ) override {
			if ( m_elementsToSelect.size() > 0 ) {
				if ( ev->buttons() == Qt::LeftButton ||
					 ev->buttons() == Qt::RightButton ) {
					m_selection.clearSelection();
					for ( const auto& ppNote : m_elementsToSelect ) {
						m_selection.addToSelection( ppNote );
					}
				}
				else {
					m_elementsToSelect.clear();
				}
			}

			updateModifiers( ev );

			// Check which note is hovered.
			updateMouseHoveredElements( ev );

			if ( ev->buttons() != Qt::NoButton ) {
				m_selection.mouseMoveEvent( ev );
				if ( m_selection.isMoving() ) {
					updateVisibleComponents( true );
				}
			}
		}
 		virtual void mouseReleaseEvent( QMouseEvent *ev ) override {
			// Don't call updateModifiers( ev ) in here because we want to apply
			// the state of the modifiers used during the last update/rendering.
			// Else the user might position a note carefully and it jumps to
			// different place because she released the Alt modifier slightly
			// earlier than the mouse button.

			m_selection.mouseReleaseEvent( ev );

			m_elementsHoveredOnDragStart.clear();

			if ( ev->button() == Qt::LeftButton && m_elementsToSelect.size() > 0 ) {
				// We used a transient selection of note(s) at a single
				// position.
				m_selection.clearSelection();
				m_elementsToSelect.clear();
				updateVisibleComponents( true );
			}
		}

 		virtual void mouseClickEvent( QMouseEvent *ev ) override {
			auto pEv = static_cast<MouseEvent*>( ev );

			updateModifiers( ev );

			// main button action
			if ( ev->button() == Qt::LeftButton &&
				 m_instance != Instance::NotePropertiesRuler ) {

				setCursorTo( ev );

				// Check whether an existing note or an empty grid cell was
				// clicked.
				handleElements( ev, Editor::Action::ToggleElements );

				m_selection.clearSelection();
				updateMouseHoveredElements( ev );
			}
			else if ( ev->button() == Qt::RightButton ) {
				if ( m_elementsHoveredForPopup.size() > 0 ) {
					setCursorTo( m_elementsHoveredForPopup[ 0 ] );
				}
				else {
					setCursorTo( ev );
				}
				showPopupMenu( ev );
			}

			update();
		}

		virtual void mouseDrawStartEvent( QMouseEvent *ev ) override {
			propertyDrawStart( ev );
			propertyDrawUpdate( ev );
		}

		virtual void mouseDrawUpdateEvent( QMouseEvent *ev ) override {
			propertyDrawUpdate( ev );
		}

		virtual void mouseDrawEndEvent( QMouseEvent *ev ) override {
			propertyDrawEnd();
		}

 		void handleKeyboardCursor( bool bVisible ) {
			auto pHydrogenApp = HydrogenApp::get_instance();
			const bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();

			pHydrogenApp->setHideKeyboardCursor( ! bVisible );

			// Only update on state changes
			if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
				updateKeyboardHoveredElements();
				if ( bVisible ) {
					m_selection.updateKeyboardCursorPosition();
					ensureCursorIsVisible();

					if ( m_selection.isLasso() && m_update !=
						 Editor::Update::Background ) {
						// Since the event was used to alter the note selection,
						// we need to repainting all note symbols (including
						// whether or not they are selected).
						m_update = Editor::Update::Content;
					}
				}
				updateAllComponents( true );
			}
		}
// 		bool isSelectionMoving() const;

// 		QPoint movingGridOffset() const;

// public slots:
 		/** When right-click opening a popup menu, ensure the clicked note is
 		 * selected. Note however that this is just temporary while the popup is
 		 * shown. Selection for the popup actions themselves is done by
 		 * popupSetup(). */
		void popupMenuAboutToShow() {
			if ( m_elementsToSelectForPopup.size() > 0 ) {
				m_selection.clearSelection();

				for ( const auto& ppNote : m_elementsToSelectForPopup ) {
					m_selection.addToSelection( ppNote );
				}
				updateVisibleComponents( true );
			}
		}

		void popupMenuAboutToHide() {
			if ( m_elementsToSelectForPopup.size() > 0 ) {
				m_selection.clearSelection();
				updateVisibleComponents( false );
			}
		}

	public:

		/** Which parts of the editor to update in the next paint event. */
		Update m_update;
		Instance m_instance;
		Type m_type;

		//! The Selection object.
		Selection<Elem> m_selection;

		QPixmap* m_pBackgroundPixmap;
		QPixmap* m_pContentPixmap;

		// width of the editor covered by the current pattern.
		int m_nActiveWidth;
		int m_nEditorHeight;
		int m_nEditorWidth;

		bool m_bCopyNotMove;
		/** Indicates whether the mouse pointer entered the widget.*/
		bool m_bEntered;

// 		enum class DragType {
// 			None,
// 			Length,
// 			Property
// 		};
// 		static QString DragTypeToQString( DragType dragType );
// 		/** Specifies whether the user interaction is altering the length
// 		 * (horizontal) or the currently selected property (vertical) of a
// 		 * note. */
// 		DragType m_dragType;

// 		/** Keeps track of all notes being drag-edited using the right mouse
// 		 * button. It maps the new, updated version of a note to an copy of
// 		 * itself still bearing the original values.*/
// 		std::map< std::shared_ptr<H2Core::Note>,
// 			std::shared_ptr<H2Core::Note> > m_draggedNotes;
// 		/** Column a click-drag event did started in.*/
// 		int m_nDragStartColumn;
// 		/** Latest vertical position of a drag event. Adjusted in every drag
// 		 * update. */
// 		int m_nDragY;
// 		QPoint m_dragStart;
// 		/** When drag editing note properties using right-click drag in
// 		 * #DrumPatternEditor and #PianoRollEditor, we display a status message
// 		 * indicating the value change. But when dragging a selection of notes
// 		 * or multiple notes at point, it is not obvious which information to
// 		 * display. We show all values changes of notes at the initial mouse
// 		 * cursor position. */
// 		std::vector< std::shared_ptr<H2Core::Note> > m_elementsHoveredOnDragStart;

		QMenu *m_pPopupMenu;

		void showPopupMenu( QMouseEvent* pEvent ){
			setupPopupMenu();

			auto pEv = static_cast<MouseEvent*>( pEvent );
			m_pPopupMenu->popup( pEv->globalPosition().toPoint() );
		}

// 		/** Function in the same vein as getColumn() but calculates both column
// 		 * and row information from the provided event position. */
// 		void eventPointToColumnRow( const QPoint& point, int* pColumn,
// 									int* pRow, int* pRealColumn = nullptr,
// 									bool bUseFineGrained = false ) const;


 		virtual void keyPressEvent( QKeyEvent* ev ) override {

			auto pHydrogenApp = HydrogenApp::get_instance();
			const bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();

			// Checks whether the elements at point are part of the current
			// selection. If not, the latter is cleared and elements at
			// point/cursor will be selected instead.
			auto selectElementsAtPoint = [&]() {
				const auto elementsUnderPoint = getElementsAtPoint(
					getCursorPosition(), 0 );
				if ( elementsUnderPoint.size() == 0 ) {
					return false;
				}

				bool bElementsSelected = false;
				if ( ! m_selection.isEmpty() ) {
					for ( const auto& ppElement : elementsUnderPoint ) {
						if ( m_selection.isSelected( ppElement ) ) {
							bElementsSelected = true;
							break;
						}
					}
				}

				if ( ! bElementsSelected ) {
					m_selection.clearSelection();
					for ( const auto& ppElement : elementsUnderPoint ) {
						m_selection.addToSelection( ppElement );
					}
					return true;
				}

				return false;
			};

			bool bUnhideCursor = ev->key() != Qt::Key_Delete;

			auto pCleanedEvent = new QKeyEvent(
				QEvent::KeyPress, ev->key(), Qt::NoModifier, ev->text() );

			const bool bIsSelectionKey = m_selection.keyPressEvent( ev );
			updateModifiers( ev );

			if ( bIsSelectionKey ) {
				// Key was claimed by Selection
			}
			else if ( ev->matches( QKeySequence::MoveToNextLine ) ||
					  ev->matches( QKeySequence::SelectNextLine ) ) {
				moveCursorDown( ev, Editor::Step::Character );
			}
			else if ( ev->key() == Qt::Key_Down &&
					  ev->modifiers() & Qt::AltModifier ) {
				moveCursorDown( ev, Editor::Step::Tiny );
			}
			else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) ||
					  ev->matches( QKeySequence::SelectEndOfBlock ) ) {
				moveCursorDown( ev, Editor::Step::Word );
			}
			else if ( ev->matches( QKeySequence::MoveToNextPage ) ||
					  ev->matches( QKeySequence::SelectNextPage ) ) {
				moveCursorDown( ev, Editor::Step::Page );
			}
			else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ||
					  ev->matches( QKeySequence::SelectEndOfDocument ) ) {
				moveCursorDown( ev, Editor::Step::Document );
			}
			else if ( ev->matches( QKeySequence::MoveToPreviousLine ) ||
					  ev->matches( QKeySequence::SelectPreviousLine ) ) {
				moveCursorUp( ev, Editor::Step::Character );
			}
			else if ( ev->key() == Qt::Key_Up &&
					  ev->modifiers() & Qt::AltModifier ) {
				moveCursorUp( ev, Editor::Step::Tiny );
			}
			else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) ||
					  ev->matches( QKeySequence::SelectStartOfBlock ) ) {
				moveCursorUp( ev, Editor::Step::Word );
			}
			else if ( ev->matches( QKeySequence::MoveToPreviousPage ) ||
					  ev->matches( QKeySequence::SelectPreviousPage ) ) {
				moveCursorUp( ev, Editor::Step::Page );
			}
			else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ||
					  ev->matches( QKeySequence::SelectStartOfDocument ) ) {
				moveCursorUp( ev, Editor::Step::Document );
			}
			else if ( ev->matches( QKeySequence::MoveToNextChar ) ||
					  ev->matches( QKeySequence::SelectNextChar ) ||
					  ( ev->modifiers() & Qt::AltModifier && (
						  pCleanedEvent->matches( QKeySequence::MoveToNextChar ) ||
						  pCleanedEvent->matches( QKeySequence::SelectNextChar ) ) ) ) {
				// ->
				moveCursorRight( ev, Editor::Step::Character );
			}
			else if ( ev->matches( QKeySequence::MoveToNextWord ) ||
					  ev->matches( QKeySequence::SelectNextWord ) ) {
				// -->
				moveCursorRight( ev, Editor::Step::Word );
			}
			else if ( ev->matches( QKeySequence::MoveToEndOfLine ) ||
					  ev->matches( QKeySequence::SelectEndOfLine ) ) {
				// -->|
				moveCursorRight( ev, Editor::Step::Document );
			}
			else if ( ev->matches( QKeySequence::MoveToPreviousChar ) ||
					  ev->matches( QKeySequence::SelectPreviousChar ) ||
					  ( ev->modifiers() & Qt::AltModifier && (
						  pCleanedEvent->matches( QKeySequence::MoveToPreviousChar ) ||
						  pCleanedEvent->matches( QKeySequence::SelectPreviousChar ) ) ) ) {
				// <-
				moveCursorLeft( ev, Editor::Step::Character );
			}
			else if ( ev->matches( QKeySequence::MoveToPreviousWord ) ||
					  ev->matches( QKeySequence::SelectPreviousWord ) ) {
				// <--
				moveCursorLeft( ev, Editor::Step::Word );
			}
			else if ( ev->matches( QKeySequence::MoveToStartOfLine ) ||
					  ev->matches( QKeySequence::SelectStartOfLine ) ) {
				// |<--
				moveCursorLeft( ev, Editor::Step::Document );
			}
			else if ( ev->matches( QKeySequence::SelectAll ) ) {
				// Key: Ctrl + A: Select all
				bUnhideCursor = false;
				selectAll();
			}
			else if ( ev->matches( QKeySequence::Deselect ) ) {
				// Key: Shift + Ctrl + A: clear selection
				bUnhideCursor = false;
				m_selection.clearSelection();
			}
			else if ( ev->matches( QKeySequence::Copy ) ) {
				bUnhideCursor = false;
				const bool bTransientSelection = selectElementsAtPoint();

				copy();

				if ( bTransientSelection ) {
					m_selection.clearSelection();
				}
			}
			else if ( ev->matches( QKeySequence::Paste ) ) {
				bUnhideCursor = false;
				paste();
			}
			else if ( ev->matches( QKeySequence::Cut ) ) {
				bUnhideCursor = false;
				const bool bTransientSelection = selectElementsAtPoint();

				cut();

				if ( bTransientSelection ) {
					m_selection.clearSelection();
				}
			}
			else if ( ev->key() == Qt::Key_Enter ||
					  ev->key() == Qt::Key_Return ) {
				// Key: Enter / Return: add or remove note at current
				// position
				m_selection.clearSelection();
				handleElements( ev, Editor::Action::ToggleElements );
			}
			else if ( ev->key() == Qt::Key_Delete ) {
				// Key: Delete / Backspace: delete selected notes, or note
				// under keyboard cursor
				if ( m_selection.begin() != m_selection.end() ) {
					deleteSelection();
				}
				else {
					handleElements( ev, Editor::Action::DeleteElements );
				}
			}
			else {
				ev->ignore();
				return;
			}

			updateKeyboardHoveredElements();

			if ( bUnhideCursor ) {
				handleKeyboardCursor( bUnhideCursor );
			}

			updateVisibleComponents( true );

			if ( ! ev->isAccepted() ) {
				ev->accept();
			}
		}

#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent* ev ) override {
#else
		virtual void enterEvent( QEvent* ev ) override {
#endif
			UNUSED( ev );
			m_bEntered = true;

			// Update focus, hovered notes and selection color.
			updateVisibleComponents( true );
		}

		virtual void leaveEvent( QEvent* ev ) override {
			UNUSED( ev );
			m_bEntered = false;

			updateMouseHoveredElements( nullptr );

			// Ending the enclosing undo context. This is key to enable the
			// Undo/Redo buttons in the main menu again and it feels like a good
			// rule of thumb to consider an action done whenever the user moves
			// mouse or cursor away from the widget.
			HydrogenApp::get_instance()->endUndoContext();

			// Update focus, hovered notes and selection color.
			updateVisibleComponents( true );
		}

		virtual void focusInEvent( QFocusEvent* ev ) override {
			UNUSED( ev );
			if ( ev->reason() == Qt::TabFocusReason ||
				 ev->reason() == Qt::BacktabFocusReason ) {
				handleKeyboardCursor( true );
			}

			// Update hovered notes, cursor, background color, selection
			// color...
			updateAllComponents( false );
		}

		virtual void focusOutEvent( QFocusEvent *ev ) override {
			UNUSED( ev );
			// Update hovered notes, cursor, background color, selection
			// color...
			updateAllComponents( false );
		}

		void popupSetup() {
			if ( m_elementsToSelectForPopup.size() > 0 ) {
				m_selection.clearSelection();

				for ( const auto& ppNote : m_elementsToSelectForPopup ) {
					m_selection.addToSelection( ppNote );
				}
			}
		}

		void popupTeardown() {
			if ( m_elementsToSelectForPopup.size() > 0 ) {
				m_elementsToSelectForPopup.clear();
				m_selection.clearSelection();
			}

			// The popup might have caused the cursor to move out of this widget
			// and the latter will loose focus once the popup is torn down. We
			// have to ensure not to display some glitchy notes previously
			// hovered by mouse which are not present anymore (e.g. since they
			// were aligned to a different position).
			updateMouseHoveredElements( nullptr );
		}

		/** When left-click dragging or applying actions using right-click popup
		 * menu on a single note/multiple notes at the same position which are
		 * not currently selected, the selection will be cleared and filled with
		 * those notes. Else we would require the user to lasso-select each
		 * single note before being able to move it.
		 *
		 * But we also have to take care of not establishing a selection
		 * prematurely since a click event on the single note would result in
		 * discarding the selection instead of removing the note. We thus use
		 * this member to cache the notes and only select them in case the mouse
		 * will be moved with left button down or right button is released
		 * without move (click). */
		std::vector<Elem> m_elementsToSelect;

		/** When drag editing note properties using right-click drag in
		 * #DrumPatternEditor and #PianoRollEditor, we display a status message
		 * indicating the value change. But when dragging a selection of notes
		 * or multiple notes at point, it is not obvious which information to
		 * display. We show all values changes of notes at the initial mouse
		 * cursor position. */
		std::vector<Elem> m_elementsHoveredOnDragStart;

		/** Right-clicking a (hovered) note should make the popup menu act on
		 * this note as well using a transient selection. However, as it is only
		 * transient, we have to take care of clearing it later on. This "later"
		 * is a little difficult. QMenu::aboutToHide() is called _before_ the
		 * action trigger in the menu and thus too early for us to be used.
		 * Clearing the selection on QMenu::triggered clears the selection
		 * _after_ an action was triggered via the menu. But it does not in case
		 * the menu was cancelled - e.g. just right-clicking a second time. The
		 * later was implemented during development and felt quite weird.
		 *
		 * Instead, we use this member to cache notes which would be part of the
		 * transient selection and both do select them and clear the selection
		 * in all associated action slots/methods. */
		std::vector<Elem> m_elementsToSelectForPopup;

 		std::vector<Elem> m_elementsHoveredForPopup;

// 		void updateHoveredNotesMouse( QMouseEvent* pEvent,
// 									  bool bUpdateEditors = true );
// 		void updateHoveredNotesKeyboard( bool bUpdateEditors = true );

};

} // namespace Base

#endif // EDITOR_BASE_H
