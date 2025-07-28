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

#include "GridCell.h"
#include "../Widgets/EditorBase.h"
#include "../Widgets/EditorDefs.h"

#include <core/Object.h>

#include <map>
#include <memory>
#include <vector>

#include <QtGui>
#include <QtWidgets>

namespace H2Core {
	class Pattern;
}

class SongEditorPanel;

//!
//! Song editor
//!
//! The main widget of SongEditorPanel, responsible for altering the sequence of
//! patterns.
//!
//! It supports mouse and keyboard based activation of patterns in timeslots, as
//! well as visual editing of multiple pattern+timeslot cells using a
//! 2-dimensional visual representation, with copy, paste, move, delete,
//! duplicate etc.
//!
/** \ingroup docGUI*/
class SongEditor : public Editor::Base<QPoint>
				 , public H2Core::Object<SongEditor>
{
    H2_OBJECT(SongEditor)
	Q_OBJECT

	public:

		static constexpr int nMargin = 10;
		/** Default value of Preferences::m_nSongEditorGridHeight * 5
		 * (patterns)*/
		static constexpr int nMinimumHeight = 90;
		static constexpr int nMinGridWidth = 8;
		static constexpr int nMaxGridWidth = 16;

		typedef QPoint Elem;

		SongEditor( QWidget *parent, QScrollArea *pScrollView,
					SongEditorPanel *pSongEditorPanel );
		~SongEditor();

		static void addOrRemovePatternCellAction(
			const H2Core::GridPoint& gridPoint, Editor::Action action,
			Editor::ActionModifier modifier );

		void updatePosition( float fTick );

		int getGridWidth();
		void setGridWidth( int width );
		int getGridHeight() { return m_nGridHeight; }

		int getCursorRow() const;
		int getCursorColumn() const;

		void clearThePatternSequenceVector( const QString& filename );

		int yScrollTarget( QScrollArea *pScrollArea, int *pnPatternInView );

		//! @name Selection interfaces
		//! @{
		virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;
		virtual QRect getKeyboardCursorRect() override;
		virtual void validateSelection() override {};
		virtual void updateWidget() override;
		virtual int getCursorMargin( QInputEvent* pEvent ) const override {
			return 0; };
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		//! @}

		//! @name Editor::Base interfaces
		//! @{
		void handleElements( QInputEvent* ev, Editor::Action action ) override;
		void deleteElements( std::vector<QPoint> vec ) override;
		virtual std::vector<QPoint> getElementsAtPoint(
			const QPoint& point, int nCursorMargin,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr ) override;
		virtual QPoint elementToPoint( QPoint point ) const override;
		virtual QPoint gridPointToPoint(
			const H2Core::GridPoint& gridPoint) const override;
		virtual H2Core::GridPoint pointToGridPoint(
			const QPoint& point, bool bHonorQuantization ) const override;

		void copy() override;
		void paste() override;
		void selectAll() override;
		void ensureCursorIsVisible() override;
		QPoint getCursorPosition() override;
		void moveCursorDown( QKeyEvent* pEvent, Editor::Step step ) override;
		void moveCursorLeft( QKeyEvent* pEvent, Editor::Step step ) override;
		void moveCursorRight( QKeyEvent* pEvent, Editor::Step step ) override;
		void moveCursorUp( QKeyEvent* pEvent, Editor::Step step ) override;
		void setCursorTo( QPoint point ) override;
		void setCursorTo( QMouseEvent* pEvent ) override;

		void setupPopupMenu() override {};

		bool updateKeyboardHoveredElements() override;
		bool updateMouseHoveredElements( QMouseEvent* pEvent ) override;

		Editor::Input getInput() const override {
			return Editor::Input::Select;
		}

		void mouseDrawStart( QMouseEvent* pEvent ) override;
		void mouseDrawUpdate( QMouseEvent* pEvent ) override;
		void mouseDrawEnd() override;

		void mouseEditStart( QMouseEvent* pEvent ) override {};
		void mouseEditUpdate( QMouseEvent* pEvent ) override {};
		void mouseEditEnd() override {};

		void updateAllComponents( bool bContentOnly ) override;
		void updateVisibleComponents( bool bContentOnly ) override;
		bool updateWidth() override;
		void createBackground() override;
		//! @}

	public slots:

		void scrolled( int );

	private:

		QScrollArea *			m_pScrollView;
		SongEditorPanel *		m_pSongEditorPanel;

		int 				m_nGridHeight;
		int 				m_nGridWidth;

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
		H2Core::GridPoint movingGridOffset() const;

		//! Mouse position during selection gestures (used to detect crossing cell boundaries)
		QPoint m_previousMousePosition, m_currentMousePosition, m_previousGridOffset;

		//! @}

		virtual void paintEvent(QPaintEvent *ev) override;

		void drawSequence();
  
		void drawPattern( int pos, int number, bool invertColour, double width );
		void drawFocus( QPainter& painter );

		std::map<H2Core::GridPoint, GridCell> m_gridCells;
		void updateGridCells();

		bool updateHoveredCells( std::vector<QPoint> hoveredCells,
								 Editor::Hover hover );
		std::vector<QPoint> m_hoveredCells;
		/** There should be at most one grid cell hovered in the SongEditor. But
		 * by making this member a std::vector we can both deal with no element
		 * hovered neatly as well as keep the code similar to the one in
		 * #PatternEditor. */
		std::vector<QPoint> m_keyboardHoveredCells;
		std::vector<QPoint> m_mouseHoveredCells;

		QPointF m_drawPreviousPosition;
		int m_nDrawPreviousColumn;
		int m_nDrawPreviousRow;

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
