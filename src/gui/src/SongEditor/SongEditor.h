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

#include <vector>
#include <memory>

#include <unistd.h>

#include <QtGui>
#include <QtWidgets>
#include <QList>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Timeline.h>
#include "../EventListener.h"
#include "PatternFillDialog.h"
#include "../Selection.h"
#include "../Widgets/WidgetWithScalableFont.h"
#include "../Widgets/WidgetWithHighlightedList.h"

namespace H2Core {
	class Hydrogen;
	class AudioEngine;
}

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
/** \ingroup docGUI*/
class SongEditor : public QWidget
				 , public H2Core::Object<SongEditor>
				 , public SelectionWidget<QPoint>
				 , public EventListener
{
    H2_OBJECT(SongEditor)
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
		void invalidateBackground();
		void updatePosition( float fTick );

		int getGridWidth ();
		void setGridWidth( uint width);
		int getGridHeight () { return m_nGridHeight; }

		int getCursorRow() const;
		int getCursorColumn() const;

		//! Modify many pattern cells at once, for use in a single efficient undo/redo action
		void modifyPatternCellsAction( std::vector<QPoint> & addCells, std::vector<QPoint> & deleteCells,
									   std::vector<QPoint> & selectCells );

		void clearThePatternSequenceVector( QString filename );
		void updateEditorandSetTrue();

		int yScrollTarget( QScrollArea *pScrollArea, int *pnPatternInView );

	static constexpr int nMargin = 10;
		/** Default value of Preferences::m_nSongEditorGridHeight * 5
		 * (patterns)*/
		static constexpr int m_nMinimumHeight = 90;

	public slots:

		void selectAll();
		void selectNone();
		void deleteSelection();
		void copy();
		void paste();
		void cut();
		void onPreferencesChanged( H2Core::Preferences::Changes changes );
		void scrolled( int );

	private:

		Selection<QPoint> m_selection;

		QScrollArea *			m_pScrollView;
		SongEditorPanel *		m_pSongEditorPanel;

		H2Core::Hydrogen* 		m_pHydrogen;
		H2Core::AudioEngine* 	m_pAudioEngine;

		unsigned 				m_nGridHeight;
		unsigned 				m_nGridWidth;
		bool					m_bIsMoving;
		bool					m_bCopyNotMove;

		int m_nMaxPatternColors;


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
		QPoint xyToColumnRow( QPoint p );
		QPoint columnRowToXy( QPoint p );
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
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
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

	virtual void patternModifiedEvent() override;
	virtual void relocationEvent() override;
	virtual void patternEditorLockedEvent() override;
	
	/** Cached position of the playhead.*/
	float m_fTick;
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
/** \ingroup docGUI*/
class SongEditorPatternList :  public QWidget
							, protected WidgetWithScalableFont<8, 10, 12>
							, public WidgetWithHighlightedList
							, public H2Core::Object<SongEditorPatternList>
							, public EventListener
{
    H2_OBJECT(SongEditorPatternList)
	Q_OBJECT

	public:
	
		explicit SongEditorPatternList( QWidget *parent );
		~SongEditorPatternList();
	
		SongEditorPatternList(const SongEditorPatternList&) = delete;
		SongEditorPatternList& operator=( const SongEditorPatternList& rhs ) = delete;

		void updateEditor();
		void createBackground();
		void invalidateBackground();
		void movePatternLine( int, int );
		void acceptPatternPropertiesDialogSettings( QString newPatternName, QString newPatternInfo, QString newPatternCategory, int patternNr );
		void revertPatternPropertiesDialogSettings(QString oldPatternName, QString oldPatternInfo, QString oldPatternCategory, int patternNr);
		void fillRangeWithPattern(FillRange* r, int nPattern);
		int getGridHeight() { return m_nGridHeight; }
	
	virtual void patternModifiedEvent() override;
	virtual void playingPatternsChangedEvent() override;
	virtual void songModeActivationEvent() override;
	virtual void stackedModeActivationEvent( int nValue ) override;
	virtual void selectedPatternChangedEvent() override;
	virtual void nextPatternsChangedEvent() override;
	virtual void relocationEvent() override;
	virtual void patternEditorLockedEvent() override;

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
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private:
		H2Core::Hydrogen* 		m_pHydrogen;
		H2Core::AudioEngine* 	m_pAudioEngine;
		uint 				m_nGridHeight;
		uint 				m_nWidth;
		static constexpr uint 	m_nInitialHeight = 10;

		QPixmap *			m_pBackgroundPixmap;
		bool m_bBackgroundInvalid;
							
		QPixmap				m_labelBackgroundLight;
		QPixmap				m_labelBackgroundDark;
		QPixmap				m_labelBackgroundSelected;
		QPixmap				m_playingPattern_on_Pixmap;
		QPixmap				m_playingPattern_off_Pixmap;
		QPixmap				m_playingPattern_empty_Pixmap;
							
		QMenu *				m_pPatternPopup;
		QLineEdit *			m_pLineEdit;
		H2Core::Pattern *	m_pPatternBeingEdited;

		DragScroller *		m_pDragScroller;
		
		void inlineEditPatternName( int row );

		virtual void mousePressEvent( QMouseEvent *ev ) override;
		virtual void mouseDoubleClickEvent( QMouseEvent *ev ) override;
		virtual void paintEvent( QPaintEvent *ev ) override;
		virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void leaveEvent( QEvent *ev );

	QPoint __drag_start_position;

		void togglePattern( int );


	void setRowSelection( RowSelection rowSelection );
	/**
	 * Specifies the row the mouse cursor is currently hovered
	 * over. -1 for no cursor.
	 */
	int m_nRowHovered;

	QTimer* m_pHighlightLockedTimer;
};


// class SongEditorPatternListener : public EventListener {
//
// }
//

/** \ingroup docGUI*/
class SongEditorPositionRuler :  public QWidget, protected WidgetWithScalableFont<8, 10, 12>, public EventListener, public H2Core::Object<SongEditorPositionRuler>
{
    H2_OBJECT(SongEditorPositionRuler)
	Q_OBJECT

	public:
		explicit SongEditorPositionRuler( QWidget *parent );
		~SongEditorPositionRuler();	

		uint getGridWidth();
		void setGridWidth (uint width);
	virtual void tempoChangedEvent( int ) override;
	virtual void playingPatternsChangedEvent() override;
	virtual void songModeActivationEvent() override;
	virtual void relocationEvent() override;
	virtual void songSizeChangedEvent() override;
	virtual void patternModifiedEvent() override;
	virtual void updateSongEvent( int ) override;
	
	virtual void timelineActivationEvent() override;
	virtual void timelineUpdateEvent( int nValue ) override;

	static int tickToColumn( float fTick, uint nGridWidth );
		static constexpr int m_nMinimumHeight = 50;

	public slots:
		void updatePosition();
		void showTagWidget( int nColumn );
		void showBpmWidget( int nColumn );
		void onPreferencesChanged( H2Core::Preferences::Changes changes );
		void createBackground();
		void invalidateBackground();
		virtual void jackTimebaseStateChangedEvent( int nState ) override;

	private:
		H2Core::Hydrogen* 		m_pHydrogen;
		H2Core::AudioEngine* 	m_pAudioEngine;
		QTimer *			m_pTimer;
		uint				m_nGridWidth;

	int m_nActiveBpmWidgetColumn;
	int m_nHoveredColumn;

	/** Indicated the part of the widget the cursor is hovering over.*/
	enum class HoveredRow {
		/** Cursor is not hovering the widget.*/
		None,
		/** Upper half until the lower end of the tempo marker
			text. */ 
		TempoMarker,
		/** Still part of the upper half, but only the last
			#m_nTagHeight pixels.*/
		Tag,
		/** Lower half*/
		Ruler
	};
	HoveredRow m_hoveredRow;

	/** Cached position of the playhead.*/
	float m_fTick;
	/** Cached and coarsed-grained position of the playhead.*/
	int m_nColumn;

	int m_nTagHeight;

	/** Area covering the length of the song columns.*/
	int m_nActiveColumns;

		QPixmap *			m_pBackgroundPixmap;
		bool 				m_bBackgroundInvalid;
		bool				m_bRightBtnPressed;
		
		virtual void mouseMoveEvent(QMouseEvent *ev) override;
		virtual void mousePressEvent( QMouseEvent *ev ) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;
		virtual void paintEvent( QPaintEvent *ev ) override;

	virtual void leaveEvent( QEvent* ev ) override;
	virtual bool event( QEvent* ev ) override;

	/** Calculates the position in pixel required to the painter for a
	 * particular @a nColumn of the grid.
	 *
	 * TODO: There needs to be some refactoring / common basis for
	 * song (and pattern) editor classes.
 	 */
	int columnToX( int nColumn ) const;
	int xToColumn( int nX ) const;

	void showToolTip( const QPoint& pos, const QPoint& globalPos );

	void drawTempoMarker( std::shared_ptr<const H2Core::Timeline::TempoMarker> pTempoMarker,
						  bool bEmphasize, QPainter& painter );
	/**
	 * @param pTempoMarker Associated #TempoMarker
	 * @param bEmphasize Whether the text of @a pTempoMarker will be
	 *   printed with bold font.
	 */
	QRect calcTempoMarkerRect( std::shared_ptr<const H2Core::Timeline::TempoMarker> pTempoMarker,
		bool bEmphasize = false ) const;

};

#endif
