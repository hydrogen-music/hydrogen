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

#ifndef SONG_EDITOR_H
#define SONG_EDITOR_H

#include <map>
#include <memory>
#include <vector>

#include <QtGui>
#include <QtWidgets>
#include <QList>

#include <core/Object.h>
#include "../Selection.h"

namespace H2Core {
	class Pattern;
}

class SongEditorPanel;

//!
//! Song editor
//!
//! The main widget of SongEditorPanel, responsible for altering the sequence of patterns.
//!
//! It supports mouse and keyboard based activation of patterns in timeslots, as well as visual editing of
//! multiple pattern+timeslot cells using a 2-dimensional visual representation, with copy, paste, move,
//! delete, duplicate etc.
//!
/** \ingroup docGUI*/
class SongEditor : public QWidget
				 , public H2Core::Object<SongEditor>
				 , public SelectionWidget<QPoint>
{
    H2_OBJECT(SongEditor)
	Q_OBJECT

		struct GridCell {
			bool m_bActive;
			bool m_bDrawnVirtual;
			float m_fWidth;
		};
	
	public:
		SongEditor( QWidget *parent, QScrollArea *pScrollView,
					SongEditorPanel *pSongEditorPanel );
		~SongEditor();

		void updateEditor( bool bSequenceOnly = false );
		void updatePosition( float fTick );

		int getGridWidth();
		void setGridWidth( int width );
		int getGridHeight() { return m_nGridHeight; }

		int getCursorRow() const;
		int getCursorColumn() const;

		//! Modify many pattern cells at once, for use in a single efficient undo/redo action
		void modifyPatternCellsAction( const std::vector<QPoint>& addCells,
									   const std::vector<QPoint>& deleteCells,
									   const std::vector<QPoint>& selectCells );

		void clearThePatternSequenceVector( const QString& filename );

		int yScrollTarget( QScrollArea *pScrollArea, int *pnPatternInView );

	static constexpr int nMargin = 10;
		/** Default value of Preferences::m_nSongEditorGridHeight * 5
		 * (patterns)*/
		static constexpr int m_nMinimumHeight = 90;
		static constexpr int nMinGridWidth = 8;
		static constexpr int nMaxGridWidth = 16;

		//! @name Selection interfaces
		//! see Selection.h for details.
		//! @{
		virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;
		virtual QRect getKeyboardCursorRect() override;
		virtual void validateSelection() override {};
		virtual void updateWidget() override;
		virtual int getCursorMargin( QInputEvent* pEvent ) const override { return 0; };
		virtual std::vector<SelectionIndex> getElementsAtPoint(
			const QPoint& point, int nCursorMargin,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
		virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
		virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
		virtual void mouseDragEndEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		virtual void updateModifiers( QInputEvent *ev );
		virtual bool canDragElements() override {
			return false;
		}
		//! @}

	public slots:

		void selectAll();
		void selectNone();
		void deleteSelection();
		void copy();
		void paste();
		void cut();
		void scrolled( int );

	private:
		void createBackground();

		Selection<QPoint> m_selection;

		QScrollArea *			m_pScrollView;
		SongEditorPanel *		m_pSongEditorPanel;

		int 				m_nGridHeight;
		int 				m_nGridWidth;
		bool					m_bCopyNotMove;

		//! In "draw" mode, whether we're activating pattern cells ("drawing") or deactivating ("erasing") is
		//! set at the start of the draw gesture.
		bool 					m_bDrawingActiveCell;

		//! Pattern sequence or selection has changed, so must be redrawn.
		bool 					m_bSequenceChanged;

		QMenu *					m_pPopupMenu;

		bool m_bBackgroundInvalid;


		//! @name Background pixmap caching
		//!
		//! To make painting the song editor sequence grid more efficient, the drawing uses multiple levels of lazy painting.
		//!   * The grid background pixmap is only updated when the size of the pattern grid changes.
		//!   * The sequence pixmap are only updated when cells are added/removed or selections change
		//!       * the cached grid background pixmap is used when repainting the pattern
		//!   * selections and moving cells are painted on top of the cached sequence pixmap
		//! @{
		QPixmap *				m_pBackgroundPixmap;
		QPixmap *				m_pSequencePixmap;
		//! @}

		//! @name Position of the keyboard input cursor
		//! @{
		int m_nCursorRow;
		int m_nCursorColumn;
		//! @}

		//! @name Conversion between sequence grid coordinates and screen (widget) coordinates.
		//! @{
		QPoint xyToColumnRow( const QPoint& p ) const;
		QPoint columnRowToXy( const QPoint& p ) const;
		//! @}

		//! Quantise the selection move offset to the sequence grid
		QPoint movingGridOffset() const;

		//! Mouse position during selection gestures (used to detect crossing cell boundaries)
		QPoint m_previousMousePosition, m_currentMousePosition, m_previousGridOffset;

		//! @name Change the mouse cursor during mouse gestures
		//! @{
		virtual void startMouseLasso( QMouseEvent *ev ) override {
			m_bSequenceChanged = true;
			setCursor( Qt::CrossCursor );
		}

		virtual void endMouseGesture() override {
			unsetCursor();
		}
		//! @}

		//! @name System events
		//! @{
		virtual void mousePressEvent(QMouseEvent *ev) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;
		virtual void mouseMoveEvent(QMouseEvent *ev) override;
		virtual void keyPressEvent (QKeyEvent *ev) override;
		virtual void keyReleaseEvent (QKeyEvent *ev) override;
		virtual void paintEvent(QPaintEvent *ev) override;
		virtual void focusInEvent( QFocusEvent *ev ) override;
	virtual void focusOutEvent( QFocusEvent *ev ) override;
		virtual void enterEvent( QEnterEvent *ev ) override;
		virtual void leaveEvent( QEvent *ev ) override;
		//! @}

    	void togglePatternActive( int nColumn, int nRow );
		void setPatternActive( int nColumn, int nRow, bool bActivate );

		void drawSequence();
  
		void drawPattern( int pos, int number, bool invertColour, double width );
		void drawFocus( QPainter& painter );

		std::map< QPoint, GridCell > m_gridCells;
		void updateGridCells();
		bool m_bEntered;

	/** Cached position of the playhead.*/
	float m_fTick;
};

inline int SongEditor::getCursorRow() const {
	return m_nCursorRow;
}

inline int SongEditor::getCursorColumn() const {
	return m_nCursorColumn;
}

#endif
