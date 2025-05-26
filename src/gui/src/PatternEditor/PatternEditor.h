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

#ifndef PATERN_EDITOR_H
#define PATERN_EDITOR_H

#include "../Selection.h"
#include "../Widgets/BaseEditor.h"
#include "../Widgets/WidgetWithScalableFont.h"

#include <core/Basics/Note.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <memory>

#include <QtGui>
#include <QtWidgets>

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
class PatternEditor : public BaseEditor<std::shared_ptr<H2Core::Note>>,
					  public H2Core::Object<PatternEditor>,
					  protected WidgetWithScalableFont<7, 9, 11>
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
	
	enum class Property {
		Velocity = 0,
		Pan = 1,
		LeadLag = 2,
		KeyOctave = 3,
		Probability = 4,
		/** For this property there is no dedicated NotePropertiesEditor
		 * instance but we solely use it within undo/redo actions.*/
		Length = 5,
		/** For this property there is no dedicated NotePropertiesEditor
		 * instance but we solely use it within undo/redo actions.*/
		Type = 6,
		/** For this property there is no dedicated NotePropertiesEditor
		 * instance but we solely use it within undo/redo actions.*/
		InstrumentId = 7,
		None = 8
	};
	static QString propertyToQString( const Property& property );

	PatternEditor( QWidget *pParent, BaseEditor::EditorType editorType );
	~PatternEditor();

	float getGridWidth() const { return m_fGridWidth; }
	unsigned getGridHeight() const { return m_nGridHeight; }
	//! Zoom in / out on the time axis
	void zoomIn();
	void zoomOut();
		void zoomLasso( float fOldGridWidth );

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
	virtual void updateModifiers( QInputEvent *ev ) override;

		virtual std::vector<SelectionIndex> getElementsAtPoint(
			const QPoint& point, int nCursorMargin,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr ) override;

		virtual int getCursorMargin( QInputEvent* pEvent ) const override;

	//! Deselecting notes
	virtual bool checkDeselectElements( const std::vector<SelectionIndex>& elements ) override;

	//! Deselect some notes, and "overwrite" some others.
	void deselectAndOverwriteNotes( const std::vector< std::shared_ptr<H2Core::Note> >& selected,
									const std::vector< std::shared_ptr<H2Core::Note> >& overwritten );

	void undoDeselectAndOverwriteNotes( const std::vector< std::shared_ptr<H2Core::Note> >& selected,
										const std::vector< std::shared_ptr<H2Core::Note> >& overwritten );

	//! Raw Qt mouse events are passed to the Selection
	virtual void mousePressEvent( QMouseEvent *ev ) override;
	virtual void mouseMoveEvent( QMouseEvent *ev ) override;
	virtual void mouseReleaseEvent( QMouseEvent *ev ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
	
	virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
	virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
	virtual void mouseDragEndEvent( QMouseEvent *ev ) override;
		virtual QRect getKeyboardCursorRect() override;

		/** Area taken available for an addition sidebar or button */
		static constexpr int nMarginSidebar = 32;
		/** #nMarginSidebar + some additional space to contain a margin and half
		 * of the notes on first grid point. */
		static constexpr int nMargin = nMarginSidebar + 10;

	/** Caches the AudioEngine::m_nPatternTickPosition in the member
		variable #m_nTick and triggers an update(). */
	void updatePosition( float fTick );

		/** Additional action to perform on the first redo() call of
		 * #SE_addOrRemoveNotes. */
		enum AddNoteAction {
			None = 0x000,
			/** Add the new note to the current selection. */
			AddToSelection = 0x001,
			/** Move cursor to focus newly added note. */
			MoveCursorTo = 0x002,
			/** Play back the new note in case hear notes is enabled. */
			Playback = 0x004
		};

		static void addOrRemoveNoteAction( int nPosition,
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
										   bool bIsNoteOff,
										   bool bIsMappedToDrumkit,
										   AddNoteAction action );

		/** For notes in #PianoRollEditor and the note key version of
		 * #NotePropertiesEditor @a nOldKey and @a nOldOctave will be
		 * used to find the actual #H2Core::Note to alter. In the latter
		 * adjusting note/octave can be done too. This is covered using @a
		 * nNewKey and @a nNewOctave. */
	static void editNotePropertiesAction( const Property& property,
										  int nPatternNumber,
										  int nPosition,
										  int nOldInstrumentId,
										  int nNewInstrumentId,
										  const QString& sOldType,
										  const QString& sNewType,
										  float fVelocity,
										  float fPan,
										  float fLeadLag,
										  float fProbability,
										  int nLength,
										  int nNewKey,
										  int nOldKey,
										  int nNewOctave,
										  int nOldOctave );
		void triggerStatusMessage(
			const std::vector< std::shared_ptr<H2Core::Note> > notes,
			const Property& property, bool bSquash = false );

		QPoint getCursorPosition();

		/** Ensure the selection lassos of the other editors match the one of
		 * this instance. */
		bool syncLasso();
		bool isSelectionMoving() const;

		QPoint movingGridOffset() const;

		void setCursorPitch( int nCursorPitch );

public slots:
	virtual void selectAll() = 0;
	virtual void selectNone();
	void deleteSelection( bool bHandleSetupTeardown = true );
	virtual void copy( bool bHandleSetupTeardown = true );
	void paste();
	virtual void cut();
	virtual void alignToGrid();
	virtual void randomizeVelocity();
	void selectAllNotesInRow( int nRow, int nPitch = PITCH_INVALID );
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
			/** Note is in a transient state while being moved to another
			 * location. A silhouette will be rendered at the new position. */
			Moved = 0x008,
			/** Note won't be played back by the audio engine. */
			NoPlayback = 0x010,
			/** Note does not have a user defined length but one introduced just
			 * for visualization purposes by a neighbouring note off note or one
			 * of the same mute group. */
			EffectiveLength = 0x020,
		};

		/** Scaling factor by which the background colors will be made darker in
		 * case the widget is not in focus. This should help users to determine
		 * which of the editors currently holds focus. */
		static constexpr int nOutOfFocusDim = 110;

		void ensureCursorIsVisible() override;
		void updateKeyboardHoveredElements() override;
		void updateMouseHoveredElements( QMouseEvent* ev ) override;
		void updateAllComponents( bool bContentOnly ) override;
		void updateVisibleComponents( bool bContentOnly ) override;

	//! Granularity of grid positioning (in ticks)
	int granularity() const;

	float m_fGridWidth;
	unsigned m_nGridHeight;

		enum class DragType {
			None,
			Length,
			Property
		};
		static QString DragTypeToQString( DragType dragType );
		/** Specifies whether the user interaction is altering the length
		 * (horizontal) or the currently selected property (vertical) of a
		 * note. */
		DragType m_dragType;

		/** Keeps track of all notes being drag-edited using the right mouse
		 * button. It maps the new, updated version of a note to an copy of
		 * itself still bearing the original values.*/
		std::map< std::shared_ptr<H2Core::Note>,
			std::shared_ptr<H2Core::Note> > m_draggedNotes;
		/** Column a click-drag event did started in.*/
		int m_nDragStartColumn;
		/** Latest vertical position of a drag event. Adjusted in every drag
		 * update. */
		int m_nDragY;
		QPoint m_dragStart;

	PatternEditorPanel* m_pPatternEditorPanel;
	QMenu *m_pPopupMenu;

	QList< QAction * > m_selectionActions;

	void showPopupMenu( QMouseEvent* pEvent );

		/** Function in the same vein as getColumn() but calculates both column
		 * and row information from the provided event position. */
		void eventPointToColumnRow( const QPoint& point, int* pColumn,
									int* pRow, int* pRealColumn = nullptr,
									bool bUseFineGrained = false ) const;

	//! Draw lines for note grid.
	void drawGridLines( QPainter &p, const Qt::PenStyle& style = Qt::SolidLine ) const;

	//! Colour to use for rendering and outlining notes
	void applyColor( std::shared_ptr<H2Core::Note> pNote, QPen* pNotePen,
					 QBrush* pNoteBrush, QPen* pNoteTailPen,
					 QBrush* pNoteTailBrush, QPen* pHighlightPen,
					 QBrush* pHighlightBrush, QPen* pMovingPen,
					 QBrush* pMovingBrush, NoteStyle noteStyle ) const;

		/** If there are multiple notes at the same position and column, the one
		 * with lowest pitch (bottom-most one in PianoRollEditor) will be
		 * rendered up front. If a subset of notes at this point is selected,
		 * the note with lowest pitch within the selection is used. */
		void sortAndDrawNotes( QPainter& p,
							   std::vector< std::shared_ptr<H2Core::Note> > notes,
							   NoteStyle baseStyle );
	/**
	 * Draw a note
	 *
	 * @param p Painting device
	 * @param pNote Particular note to draw
	 * @param noteStyle Whether the @a pNote is contained in the pattern
	 *   currently shown in the pattern editor (the one selected in the song
	 *   editor), currently hovered, or selected.
	 */
	void drawNote( QPainter &p, std::shared_ptr<H2Core::Note> pNote,
				   NoteStyle noteStyle ) const;

		/** Update #m_pContentPixmap based on #m_pBackgroundPixmap to show the
		 * latest content of all active pattern. */
		virtual void drawPattern();
		void drawFocus( QPainter& p );
		void drawBorders( QPainter& p );

	/**
	 * Adjusts #m_nActiveWidth and #m_nEditorWidth to the current
	 * state of the editor.
	 */
	virtual bool updateWidth() override;

	/** Indicates whether the mouse pointer entered the widget.*/
	bool m_bEntered;
		/** @param bFullUpdate if `false`, just a simple update() of the widget
		 *   will be triggered. If `true`, the background will be updated as
		 *   well. */
		void keyPressEvent ( QKeyEvent *ev, bool bFullUpdate = false );
		virtual void keyReleaseEvent (QKeyEvent *ev) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void focusInEvent( QFocusEvent *ev ) override;
	virtual void focusOutEvent( QFocusEvent *ev ) override;
		virtual void paintEvent( QPaintEvent* ev ) override;

	int m_nTick;

		// Row the keyboard cursor is residing in.
		//
		// Only in #PianoRollEditor this variable is relevant and updated. In
		// #DrumPatternEditor #PatternEditorPanel::m_nSelectedRowDB is used
		// instead and #NotePropertiesPanel does only contain a single row.
		int m_nCursorPitch;

	Editor m_editor;
	Property m_property;


		void updateHoveredNotesMouse( QMouseEvent* pEvent,
									  bool bUpdateEditors = true );
		void updateHoveredNotesKeyboard( bool bUpdateEditors = true );

		/** Checks whether the note would be played back when picked up by the
		 * audio engine. */
		bool checkNotePlayback( std::shared_ptr<H2Core::Note> pNote ) const;

		/** If the note is left of a NoteOff of the same instrument or of a note
		 * within the same mute group, its sample will only be rendered till
		 * that next note is encountered. We will indicate this behavior by
		 * drawing an effective (more dim) tail of the note. */
		int calculateEffectiveNoteLength( std::shared_ptr<H2Core::Note> pNote ) const;
};

#endif // PATERN_EDITOR_H
