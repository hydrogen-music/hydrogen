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
class SongEditor : public Editor::Base<std::shared_ptr<GridCell>>
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

		typedef std::shared_ptr<GridCell> Elem;

		SongEditor( QWidget *parent, QScrollArea *pScrollView,
					SongEditorPanel *pSongEditorPanel );
		~SongEditor();

		void addOrRemovePatternCellAction(
			const H2Core::GridPoint& gridPoint, Editor::Action action,
			Editor::ActionModifier modifier );

		void updatePosition( float fTick );

		int getGridWidth();
		void setGridWidth( int width );
		int getGridHeight() { return m_nGridHeight; }

		void clearThePatternSequenceVector( const QString& filename );

		int yScrollTarget( QScrollArea *pScrollArea, int *pnPatternInView );

		//! @name Selection interfaces
		//! @{
		virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;
		virtual QRect getKeyboardCursorRect() override;
		virtual void validateSelection() override {};
		virtual int getCursorMargin( QInputEvent* pEvent ) const override {
			return 0; };
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		//! @}

		//! @name Editor::Base interfaces
		//! @{
		void handleElements( QInputEvent* ev, Editor::Action action ) override;
		void deleteElements( std::vector<std::shared_ptr<GridCell>> vec ) override;
		std::vector< std::shared_ptr<GridCell> > getElementsAtPoint(
			const QPoint& point, int nCursorMargin,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr ) override;
		QPoint elementToPoint( std::shared_ptr<GridCell> pCell ) const override;
		QPoint gridPointToPoint(
			const H2Core::GridPoint& gridPoint) const override;
		H2Core::GridPoint pointToGridPoint(
			const QPoint& point, bool bHonorQuantization ) const override;
		H2Core::GridPoint movingGridOffset() const override;

		void copy() override;
		void paste() override;
		void selectAll() override;
		void ensureCursorIsVisible() override;
		H2Core::GridPoint getCursorPosition() const override;
		void moveCursorDown( QKeyEvent* pEvent, Editor::Step step ) override;
		void moveCursorLeft( QKeyEvent* pEvent, Editor::Step step ) override;
		void moveCursorRight( QKeyEvent* pEvent, Editor::Step step ) override;
		void moveCursorUp( QKeyEvent* pEvent, Editor::Step step ) override;
		void setCursorTo( std::shared_ptr<GridCell> pCell ) override;
		void setCursorTo( QMouseEvent* pEvent ) override;

		void setupPopupMenu() override {};

		bool updateKeyboardHoveredElements() override;
		bool updateMouseHoveredElements( QMouseEvent* pEvent ) override;

		Editor::Input getInput() const override;
		bool syncLasso() override {
			return false;
		}

		void mouseDrawStart( QMouseEvent* pEvent ) override;
		void mouseDrawUpdate( QMouseEvent* pEvent ) override;
		void mouseDrawEnd() override;

		void mouseEditStart( QMouseEvent* pEvent ) override {};
		void mouseEditUpdate( QMouseEvent* pEvent ) override {};
		void mouseEditEnd() override {};

		void updateAllComponents( Editor::Update update ) override;
		void updateVisibleComponents( Editor::Update update ) override;
		bool updateWidth() override;
		void createBackground() override;
		//! @}

	public slots:

		void scrolled( int );

	private:
		enum CellStyle {
			/** Default style. */
			Default = 0x000,
			/** Cell is hovered by mouse or keyboard.*/
			Hovered = 0x001,
			/** Cell is part of the current selection.*/
			Selected = 0x004,
			/** Cell is in a transient state while being moved to another
			 * location. A silhouette will be rendered at the new position. */
			Moved = 0x008,
			/** Cell is activated as part of a virtual pattern/another cell. */
			Virtual = 0x010,
		};

		QScrollArea *			m_pScrollView;
		SongEditorPanel *		m_pSongEditorPanel;

		int 				m_nGridHeight;
		int 				m_nGridWidth;

		//! @name Position of the keyboard input cursor
		H2Core::GridPoint m_cursor;

		//! @}

		virtual void paintEvent(QPaintEvent *ev) override;

		void drawSequence();
  
		void drawPattern( QPainter& painter, std::shared_ptr<GridCell> pCell,
						  CellStyle cellStyle );
		void drawFocus( QPainter& painter );

		std::map<H2Core::GridPoint, std::shared_ptr<GridCell> > m_gridCells;
		void updateGridCells();

		bool updateHoveredCells(
			std::vector< std::shared_ptr<GridCell> > hoveredCells,
			Editor::Hover hover );
		std::vector< std::shared_ptr<GridCell> > m_hoveredCells;
		/** There should be at most one grid cell hovered in the SongEditor. But
		 * by making this member a std::vector we can both deal with no element
		 * hovered neatly as well as keep the code similar to the one in
		 * #PatternEditor. */
		std::vector< std::shared_ptr<GridCell> > m_keyboardHoveredCells;
		std::vector< std::shared_ptr<GridCell> > m_mouseHoveredCells;

		QPointF m_drawPreviousPosition;
		H2Core::GridPoint m_drawPreviousGridPoint;

	/** Cached position of the playhead.*/
	float m_fTick;
};

#endif
