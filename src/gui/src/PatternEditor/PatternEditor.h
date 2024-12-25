/*
 * Hydrogen
 * Copyright(c) 2002-2020 by the Hydrogen Team
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef PATERN_EDITOR_H
#define PATERN_EDITOR_H

#include "../EventListener.h"
#include "../Selection.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Note;
	class Instrument;
}

class PatternEditorPanel;

//! Pattern Editor
//!
//! The PatternEditor class is an abstract base class for
//! functionality common to Pattern Editor components
//! (DrumPatternEditor, PianoRollEditor, NotePropertiesRuler).
//!
//! This covers common elements such as some selection handling,
//! timebase functions, and drawing grid lines.
//!
/** \ingroup docGUI*/
class PatternEditor : public QWidget,
					  public EventListener,
					  public H2Core::Object<PatternEditor>,
					  public SelectionWidget<H2Core::Note *>
{
	H2_OBJECT(PatternEditor)
	Q_OBJECT

public:
	enum class Editor {
		DrumPattern = 0,
		PianoRoll = 1,
		NotePropertiesRuler = 2,
		None = 3
	};
		static QString editorToQString( const Editor& editor );
	
	enum class Mode {
		Velocity = 0,
		Pan = 1,
		LeadLag = 2,
		KeyOctave = 3,
		Probability = 4,
		/** For this mode we a dedicated NotePropertiesEditor but solely use it
		 * within undo/redo actions.*/
		Length = 5,
		None = 6
	};
	static QString modeToQString( const Mode& mode );

		/** Specifies which parts of the editor need updating on a paintEvent().
		 * Bigger numerical values imply updating elements with lower ones as
		 * well.*/
		enum class Update {
			/** Just paint transient elements, like hovered notes, cursor, focus
			 * or lasso. */
			None = 0,
			/** Update pattern notes including selection of a cached background
			 * image. */
			Pattern = 1,
			/** Update the background image. */
			Background = 2
		};

	PatternEditor( QWidget *pParent );
	~PatternEditor();

	float getGridWidth() const { return m_fGridWidth; }
	unsigned getGridHeight() const { return m_nGridHeight; }
	//! Zoom in / out on the time axis
	void zoomIn();
	void zoomOut();

	//! Clear the pattern editor selection
	void clearSelection() {
		m_selection.clearSelection();
	}

	/** Move or copy notes.
	 *
	 * Moves or copies notes at the end of a Selection move, handling the
	 * behaviours necessary for out-of-range moves or copies.*/
	virtual void selectionMoveEndEvent( QInputEvent *ev ) override;

	//! Calculate colour to use for note representation based on note velocity. 
	static QColor computeNoteColor( float velocity );


	//! Merge together the selection groups of two PatternEditor objects to share a common selection.
	void mergeSelectionGroups( PatternEditor *pPatternEditor ) {
		m_selection.merge( &pPatternEditor->m_selection );
	}

	//! Ensure that the Selection contains only valid elements.
	virtual void validateSelection() override;

	//! Update the status of modifier keys in response to input events.
	virtual void updateModifiers( QInputEvent *ev );

	//! Update a widget in response to a change in selection
	virtual void updateWidget() override {
		updateEditor( true );
	}

		virtual int getCursorMargin() const override;

	//! Deselecting notes
	virtual bool checkDeselectElements( const std::vector<SelectionIndex>& elements ) override;

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

	//! Do two notes match exactly, from the pattern editor's point of view?
	//! They match if all user-editable properties are the same.
	bool notesMatchExactly( H2Core::Note *pNoteA, H2Core::Note *pNoteB ) const;

	//! Deselect some notes, and "overwrite" some others.
	void deselectAndOverwriteNotes( const std::vector< H2Core::Note *>& selected,
									const std::vector< H2Core::Note *>& overwritten );

	void undoDeselectAndOverwriteNotes( const std::vector< H2Core::Note *>& selected,
										const std::vector< H2Core::Note *>& overwritten );

	//! Raw Qt mouse events are passed to the Selection
	virtual void mousePressEvent( QMouseEvent *ev ) override;
	virtual void mouseMoveEvent( QMouseEvent *ev ) override;
	virtual void mouseReleaseEvent( QMouseEvent *ev ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
	
	virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
	virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
	virtual void mouseDragEndEvent( QMouseEvent *ev ) override;
		virtual QRect getKeyboardCursorRect() override;

	static constexpr int nMargin = 20;

	/** Caches the AudioEngine::m_nPatternTickPosition in the member
		variable #m_nTick and triggers an update(). */
	void updatePosition( float fTick );

		/** Whether new notes added to the editor should be automatically
		 * selected. */
		bool getSelectNewNotes() const;
		void addOrRemoveNote( int nColumn, int nRealColumn, int nRow,
							  int nKey = KEY_MIN,
							  int nOctave = OCTAVE_DEFAULT,
							  bool bDoAdd = true, bool bDoDelete = true,
							  bool bIsNoteOff = false );

	static void addOrRemoveNoteAction( int nColumn,
									   int nInstrumentId,
									   const QString& sType,
									   int nPatternNumber,
									   int nOldLength,
									   float fOldVelocity,
									   float fOldPan,
									   float fOldLeadLag,
									   int nOldKey,
									   int nOldOctave,
									   float fOldProbability,
									   bool bIsDelete,
									   bool bIsMidi,
									   bool bIsNoteOff );

		/** For notes in #PianoRollEditor and the note key version of
		 * #NotePropertiesEditor @a nOldKey and @a nOldOctave will be
		 * used to find the actual #H2Core::Note to alter. In the latter
		 * adjusting note/octave can be done too. This is covered using @a
		 * nNewKey and @a nNewOctave. */
	static void editNotePropertiesAction( const Mode& mode,
										  int nPatternNumber,
										  int nColumn,
										  int nInstrumentId,
										  const QString& sType,
										  float fVelocity,
										  float fPan,
										  float fLeadLag,
										  float fProbability,
										  int nLength,
										  int nNewKey,
										  int nOldKey,
										  int nNewOctave,
										  int nOldOctave );
	static void triggerStatusMessage( H2Core::Note* pNote, const Mode& mode );

	/**
	 * Determines whether to pattern editor should show further
	 * patterns (determined by getPattersToShow()) or just the
	 * currently selected one.
	 */
	static bool isUsingAdditionalPatterns( const std::shared_ptr<H2Core::Pattern> pPattern );

		QPoint getCursorPosition();
		void handleKeyboardCursor( bool bVisible );

		/** Ensure the selection lassos of the other editors match the one of
		 * this instance. */
		bool syncLasso();

protected:

	//! The Selection object.
	Selection< SelectionIndex > m_selection;

public slots:
	virtual void updateEditor( bool bPatternOnly = false ) = 0;
	virtual void selectAll() = 0;
	virtual void selectNone();
	void deleteSelection();
	virtual void copy();
	void paste();
	virtual void cut();
	virtual void alignToGrid();
	virtual void randomizeVelocity();
	virtual void selectAllNotesInRow( int nRow );
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
	void scrolled( int nValue );

protected:
		enum NoteStyle {
			/** Regular note of the current pattern. */
			Foreground = 0x000,
			/** Regular note of another currently playing pattern. The note is
			 * not accessible and can neither be hovered or selected. */
			Background = 0x001,
			/** Note is hovered by mouse.*/
			Hovered = 0x002,
			/** Note is part of the current selection.*/
			Selected = 0x004,
		};

		/** Scaling factor by which the background colors will be made darker in
		 * case the widget is not in focus. This should help users to determine
		 * which of the editors currently holds focus. */
		static constexpr int nOutOfFocusDim = 120;

		/** Distance in pixel the cursor is allowed to be away from a note to
		 * still be associated with it.
		 *
		 * Note that for very small resolutions a smaller margin will be used to
		 * still allow adding notes to adjacent grid cells. */
		static constexpr int nDefaultCursorMargin = 10;

	//! Granularity of grid positioning (in ticks)
	int granularity() const;

	uint m_nEditorHeight;
	uint m_nEditorWidth;

	// width of the editor covered by the current pattern.
	int m_nActiveWidth;

	float m_fGridWidth;
	unsigned m_nGridHeight;

	bool m_bFineGrained;
	bool m_bCopyNotMove;

	bool m_bSelectNewNotes;

		void clearDraggedNotes();
		/** Keeps track of all notes being drag-edited using the right mouse
		 * button. It maps the new, updated version of a note to an copy of
		 * itself still bearing the original values.*/
		std::map<H2Core::Note*, H2Core::Note*> m_draggedNotes;
		/** Column a click-drag event did started in.*/
		int m_nDragStartColumn;
		/** Latest vertical position of a drag event. Adjusted in every drag
		 * update. */
		int m_nDragY;

	PatternEditorPanel* m_pPatternEditorPanel;
	QMenu *m_pPopupMenu;

	QList< QAction * > m_selectionActions;

	void showPopupMenu( const QPoint & pos );

		/** Function in the same vein as getColumn() but calculates both column
		 * and row information from the provided event position. */
		void eventPointToColumnRow( const QPoint& point, int* pColumn,
									int* pRow, int* pRealColumn = nullptr,
									bool bUseFineGrained = false ) const;
	QPoint movingGridOffset() const;

	//! Draw lines for note grid.
	void drawGridLines( QPainter &p, const Qt::PenStyle& style = Qt::SolidLine ) const;

	//! Colour to use for outlining notes
	QColor highlightedNoteColor( NoteStyle noteStyle ) const;

	/**
	 * Draw a note
	 *
	 * @param p Painting device
	 * @param pNote Particular note to draw
	 * @param noteStyle Whether the @a pNote is contained in the pattern
	 *   currently shown in the pattern editor (the one selected in the song
	 *   editor), currently hovered, or selected.
	 */
	void drawNote( QPainter &p, H2Core::Note *pNote, NoteStyle noteStyle ) const;

	/** Updates #m_pBackgroundPixmap to show the latest content. */
	virtual void createBackground();
	void invalidateBackground();
	QPixmap* m_pBackgroundPixmap;
	QPixmap* m_pPatternPixmap;
	bool m_bBackgroundInvalid;

		/** Which parts of the editor to update in the next paint event. */
		Update m_update;

	/**
	 * Adjusts #m_nActiveWidth and #m_nEditorWidth to the current
	 * state of the editor.
	 */
	void updateWidth();

	/** Indicates whether the mouse pointer entered the widget.*/
	bool m_bEntered;
		/** @param bFullUpdate if `false`, just a simple update() of the widget
		 *   will be triggered. If `true`, the background will be updated as
		 *   well. */
		void keyPressEvent ( QKeyEvent *ev, bool bFullUpdate = false );
		virtual void keyReleaseEvent (QKeyEvent *ev) override;
	virtual void enterEvent( QEvent *ev ) override;
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void focusInEvent( QFocusEvent *ev ) override;
	virtual void focusOutEvent( QFocusEvent *ev ) override;

	int m_nTick;

		void setCursorRow( int nCursorRow );
		// Row the keyboard cursor is residing in.
		//
		// Only in #PianoRollEditor this variable is relevant and updated. In
		// #DrumPatternEditor #PatternEditorPanel::m_nSelectedRowDB is used
		// instead and #NotePropertiesPanel does only contain a single row.
		int m_nCursorRow;

	Editor m_editor;
	Mode m_mode;

		/** When left-click dragging a single note/multiple notes at the same
		 * position which are not currently selected, the selection will be
		 * cleared and filled with those notes. Else we would require the user
		 * to lasso-select each single note before being able to move it.
		 *
		 * But we also have to take care of not establishing a selection
		 * prematurely since a click event on the single note would result in
		 * discarding the selection instead of removing the note. We thus use
		 * this member to cache the notes and only select them in case the mouse
		 * will be moved with left button down. */
		std::vector<H2Core::Note*> m_notesToSelectOnMove;

		/** Gets all notes located at @a point (position of e.g. QMouseEvent)*/
		std::vector<H2Core::Note*> getNotesAtPoint( std::shared_ptr<H2Core::Pattern> pPattern,
													const QPoint& point,
													int nCursorMargin,
													bool bExcludeSelected );

		void updateHoveredNotesMouse( const QPoint& point );
		void updateHoveredNotesKeyboard();
};

inline bool PatternEditor::getSelectNewNotes() const {
	return m_bSelectNewNotes;
}

#endif // PATERN_EDITOR_H
