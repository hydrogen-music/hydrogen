/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef SONG_EDITOR_H
#define SONG_EDITOR_H

#include <vector>

#include <unistd.h>

#include <QtGui>
#include <QtWidgets>
#include <QList>

#include <core/Object.h>
#include "../EventListener.h"
#include "PatternFillDialog.h"
#include "../Selection.h"

class Button;
class ToggleButton;
class SongEditor;
class SongEditorPatternList;
class SongEditorPositionRuler;
class SongEditorPanel;

static const uint SONG_EDITOR_MIN_GRID_WIDTH = 8;
static const uint SONG_EDITOR_MAX_GRID_WIDTH = 16;


//!
//! Song editor
//!
//! The main widget of SongEditorPanel, responsible for altering the sequence of patterns.
//!
//! It supports mouse and keyboard based activation of patterns in timeslots, as well as visual editing of
//! multiple pattern+timeslot cells using a 2-dimensional visual representation, with copy, paste, move,
//! delete, duplicate etc.
//!
class SongEditor : public QWidget, public H2Core::Object, public SelectionWidget<QPoint>
{
    H2_OBJECT
	Q_OBJECT

		struct GridCell {
			bool m_bActive;
			bool m_bDrawnVirtual;
			float m_fWidth;
		};
	
	public:
		SongEditor( QWidget *parent, QScrollArea *pScrollView, SongEditorPanel *pSongEditorPanel );
		~SongEditor();

		void createBackground();
		
		void cleanUp();

		int getGridWidth ();
		void setGridWidth( uint width);
		int getGridHeight () { return m_nGridHeight; }

		int getCursorRow() const;
		int getCursorColumn() const;

		//! Add or delete pattern in the sequence grid.
		void addPattern( int nColumn, int nRow);
		void deletePattern( int nColumn, int nRow );

		//! Modify many pattern cells at once, for use in a single efficient undo/redo action
		void modifyPatternCellsAction( std::vector<QPoint> & addCells, std::vector<QPoint> & deleteCells,
									   std::vector<QPoint> & selectCells );

		void clearThePatternSequenceVector( QString filename );
		void updateEditorandSetTrue();

	public slots:

		void selectAll();
		void selectNone();
		void deleteSelection();
		void copy();
		void paste();
		void cut();

	private:

		Selection<QPoint> m_selection;

		QScrollArea *m_pScrollView;
		SongEditorPanel *m_pSongEditorPanel;

		QMenu *m_pPopupMenu;

		unsigned m_nGridHeight;
		unsigned m_nGridWidth;
		unsigned m_nMaxPatternSequence;
		bool m_bCopyNotMove;

		//! Pattern sequence or selection has changed, so must be redrawn.
		bool m_bSequenceChanged;

		//! In "draw" mode, whether we're activating pattern cells ("drawing") or deactivating ("erasing") is
		//! set at the start of the draw gesture.
		bool m_bDrawingActiveCell;

		//! @name Background pixmap caching
		//!
		//! To make painting the song editor sequence grid more efficient, the drawing uses multiple levels of lazy painting.
		//!   * The grid background pixmap is only updated when the size of the pattern grid changes.
		//!   * The sequence pixmap are only updated when cells are added/removed or selections change
		//!       * the cached grid background pixmap is used when repainting the pattern
		//!   * selections and moving cells are painted on top of the cached sequence pixmap
		//! @{
		QPixmap *m_pBackgroundPixmap;
		QPixmap *m_pSequencePixmap;
		//! @}

		const int m_nMargin = 10;

		//! @name Position of the keyboard input cursor
		//! @{
		int m_nCursorRow;
		int m_nCursorColumn;
		//! @}

		//! @name Conversion between sequence grid coordinates and screen (widget) coordinates.
		//! @{
		QPoint xyToColumnRow( QPoint p );
		QPoint columnRowToXy( QPoint p );
		//! @}

		//! Quantise the selection move offset to the sequence grid
		QPoint movingGridOffset() const;

		//! Mouse position during selection gestures (used to detect crossing cell boundaries)
		QPoint m_previousMousePosition, m_currentMousePosition;

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
		//! @}

		bool togglePatternActive( int nColumn, int nRow );
		void setPatternActive( int nColumn, int nRow, bool value );

		void drawSequence();
  
		void drawPattern( int pos, int number, bool invertColour, double width );

		std::map< QPoint, GridCell > m_gridCells;
		void updateGridCells();
public:

		//! @name Selection interfaces
		//! see Selection.h for details.
		//! @{
		virtual std::vector<SelectionIndex> elementsIntersecting( QRect r ) override;
		virtual QRect getKeyboardCursorRect() override;
		virtual void validateSelection() override {};
		virtual void updateWidget() override;
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

};

inline int SongEditor::getCursorRow() const {
	return m_nCursorRow;
}

inline int SongEditor::getCursorColumn() const {
	return m_nCursorColumn;
}


///
/// Song editor pattern list
///
class SongEditorPatternList : public QWidget, public H2Core::Object, public EventListener
{
    H2_OBJECT
	Q_OBJECT

	public:
		explicit SongEditorPatternList( QWidget *parent );
		~SongEditorPatternList();
	
		SongEditorPatternList(const SongEditorPatternList&) = delete;
		SongEditorPatternList& operator=( const SongEditorPatternList& rhs ) = delete;

		void updateEditor();
		void createBackground();
		void movePatternLine( int, int );
		void deletePatternFromList( QString patternFilename, QString sequenceFileName, int patternPosition );
		void restoreDeletedPatternsFromList( QString patternFilename, QString sequenceFileName, int patternPosition );
		void acceptPatternPropertiesDialogSettings( QString newPatternName, QString newPatternInfo, QString newPatternCategory, int patternNr );
		void revertPatternPropertiesDialogSettings(QString oldPatternName, QString oldPatternInfo, QString oldPatternCategory, int patternNr);
		void loadPatternAction( QString filename, int position);
		void fillRangeWithPattern(FillRange* r, int nPattern);
		void patternPopup_duplicateAction( QString patternFilename, int patternposition );
		int getGridHeight() { return m_nGridHeight; }

	public slots:
		void patternPopup_edit();
		void patternPopup_save();
		void patternPopup_export();
		void patternPopup_load();
		void patternPopup_properties();
		void patternPopup_delete();
		void patternPopup_duplicate();
		void patternPopup_fill();
		void patternPopup_virtualPattern();
		void inlineEditingFinished();
		void inlineEditingEntered();
		virtual void dragEnterEvent(QDragEnterEvent *event) override;
		virtual void dropEvent(QDropEvent *event) override;
		virtual void timelineUpdateEvent( int nValue ) override;

	private:
		uint m_nGridHeight;
		uint m_nWidth;
		static const uint m_nInitialHeight = 10;

		QPixmap *			m_pBackgroundPixmap;
							
		QPixmap				m_labelBackgroundLight;
		QPixmap				m_labelBackgroundDark;
		QPixmap				m_labelBackgroundSelected;
		QPixmap				m_playingPattern_on_Pixmap;
		QPixmap				m_playingPattern_off_Pixmap;
							
		QMenu *				m_pPatternPopup;
		QLineEdit *			m_pLineEdit;
		H2Core::Pattern *	m_pPatternBeingEdited;

		DragScroller *		m_pDragScroller;
		
		void inlineEditPatternName( int row );

		virtual void mousePressEvent( QMouseEvent *ev ) override;
		virtual void mouseDoubleClickEvent( QMouseEvent *ev ) override;
		virtual void paintEvent( QPaintEvent *ev ) override;

		void togglePattern( int );

		virtual void patternChangedEvent() override;
		void mouseMoveEvent(QMouseEvent *event) override;
		QPoint __drag_start_position;

};


// class SongEditorPatternListener : public EventListener {
//
// }
//

class SongEditorPositionRuler : public QWidget, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

	public:
		explicit SongEditorPositionRuler( QWidget *parent );
		~SongEditorPositionRuler();	

		void createBackground();

		uint getGridWidth();
		void setGridWidth (uint width);
		void editTimeLineAction( int newPosition, float newBpm );
		void deleteTimeLinePosition( int position );
		void editTagAction( QString text, int position, QString textToReplace );
		void deleteTagAction( QString text, int position );

	public slots:
		void updatePosition();

	private:
		QTimer *			m_pTimer;
		uint				m_nGridWidth;
		uint				m_nMaxPatternSequence;
		uint				m_nInitialWidth;
		static const uint	m_nHeight = 50;
		const int m_nMargin = 10;

		QPixmap *			m_pBackgroundPixmap;
		QPixmap				m_tickPositionPixmap;
		bool				m_bRightBtnPressed;
		
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mousePressEvent( QMouseEvent *ev );
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void paintEvent( QPaintEvent *ev );

};


#endif
