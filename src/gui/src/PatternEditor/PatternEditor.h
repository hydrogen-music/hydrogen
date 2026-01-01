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
#include "../Widgets/EditorBase.h"
#include "../Widgets/EditorDefs.h"
#include "../Widgets/WidgetWithScalableFont.h"

#include <core/Basics/GridPoint.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Note.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <memory>

#include <QtGui>
#include <QtWidgets>

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
class PatternEditor : public Editor::Base<std::shared_ptr<H2Core::Note>>,
					  public H2Core::Object<PatternEditor>,
					  protected WidgetWithScalableFont<7, 9, 11>
{
	H2_OBJECT(PatternEditor)
	Q_OBJECT

public:
		typedef std::shared_ptr<H2Core::Note> Elem;

		/** Area taken available for an addition sidebar or button */
		static constexpr int nMarginSidebar = 32;
		/** #nMarginSidebar + some additional space to contain a margin and half
		 * of the notes on first grid point. */
		static constexpr int nMargin = nMarginSidebar + 10;

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

		PatternEditor( QWidget *pParent );
		~PatternEditor();

		static void addOrRemoveNoteAction(
			int nPosition,
			H2Core::Instrument::Id id,
			const H2Core::Instrument::Type& sType,
			int nPatternNumber,
			int nOldLength,
			float fOldVelocity,
			float fOldPan,
			float fOldLeadLag,
			H2Core::Note::Key oldKey,
			int nOldOctave,
			float fOldProbability,
			Editor::Action action,
			bool bIsNoteOff,
			bool bIsMappedToDrumkit,
			Editor::ActionModifier modifier
		);

		//! Deselect some notes, and "overwrite" some others.
		void deselectAndOverwriteNotes(
			const std::vector< std::shared_ptr<H2Core::Note> >& selected,
			const std::vector< std::shared_ptr<H2Core::Note> >& overwritten );

		void undoDeselectAndOverwriteNotes(
			const std::vector< std::shared_ptr<H2Core::Note> >& selected,
			const std::vector< std::shared_ptr<H2Core::Note> >& overwritten );

		/** For notes in #PianoRollEditor and the note key version of
		 * #NotePropertiesEditor @a oldKey and @a nOldOctave will be
		 * used to find the actual #H2Core::Note to alter. In the latter
		 * adjusting note/octave can be done too. This is covered using @a
		 * newKey and @a nNewOctave. */
		static void editNotePropertiesAction(
			const Property& property,
			int nPatternNumber,
			int nPosition,
			H2Core::Instrument::Id oldId,
			H2Core::Instrument::Id newId,
			const H2Core::Instrument::Type& sOldType,
			const H2Core::Instrument::Type& sNewType,
			float fVelocity,
			float fPan,
			float fLeadLag,
			float fProbability,
			int nLength,
			H2Core::Note::Key newKey,
			H2Core::Note::Key oldKey,
			int nNewOctave,
			int nOldOctave
		);

		float getGridWidth() const;
		void setGridWidth( float fGridWith );
		unsigned getGridHeight() const;

		bool isSelectionMoving() const;

		//! Merge together the selection groups of two PatternEditor objects to
		//! share a common selection.
		void mergeSelectionGroups( PatternEditor *pPatternEditor ) {
			m_selection.merge( &pPatternEditor->m_selection );
		}

		void setCursorPitch( int nCursorPitch );

		void triggerStatusMessage(
			const std::vector< std::shared_ptr<H2Core::Note> > notes,
			const Property& property, bool bSquash = false );

		/** Caches the AudioEngine::m_nPatternTickPosition in the member
			variable #m_nTick and triggers an update(). */
		void updatePosition( float fTick );

		// Zoom in / out on the time axis
		void zoomIn();
		void zoomLasso( float fOldGridWidth );
		void zoomOut();

		//! Raw Qt mouse events are passed to the Selection.
		virtual void keyPressEvent( QKeyEvent* ev ) override;
		virtual void keyReleaseEvent(QKeyEvent *ev) override;
		virtual void mousePressEvent( QMouseEvent *ev ) override;
		virtual void paintEvent( QPaintEvent* ev ) override;
	
		//! @name SelectionWidget interfaces
		//! @{
		//! Deselecting notes
		bool checkDeselectElements(
			const std::vector< std::shared_ptr<H2Core::Note> >& elements ) override;
		int getCursorMargin( QInputEvent* pEvent ) const override;
		QRect getKeyboardCursorRect() override;
		/** Move or copy notes.
		 *
		 * Moves or copies notes at the end of a Selection move, handling the
		 * behaviours necessary for out-of-range moves or copies.*/
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		//! Ensure that the Selection contains only valid elements.
		virtual void validateSelection() override;
		//! @)

		// @name Editor::Base interfaces
		//! @{
		void handleElements( QInputEvent* ev, Editor::Action action ) override;
		void deleteElements( std::vector< std::shared_ptr<H2Core::Note>> ) override;
		virtual std::vector<SelectionIndex> getElementsAtPoint(
			const QPoint& point,
			Editor::InputSource inputSource,
			int nCursorMargin,
			bool bIncludeHovered,
			std::shared_ptr<H2Core::Pattern> pPattern = nullptr
		) override;
		virtual QPoint elementToPoint( std::shared_ptr<H2Core::Note> pNote
		) const override;
		virtual QPoint gridPointToPoint(
			const H2Core::GridPoint& gridPoint) const override;
		virtual H2Core::GridPoint pointToGridPoint(
			const QPoint& point, bool bHonorQuantization ) const override;
		H2Core::GridPoint movingGridOffset() const override;
		void copy() override;
		void paste() override;
		void ensureCursorIsVisible() override;
		virtual H2Core::GridPoint getCursorPosition() const override;
		void moveCursorLeft( QKeyEvent* ev, Editor::Step step ) override;
		void moveCursorRight( QKeyEvent* ev, Editor::Step step ) override;
		void setCursorTo( std::shared_ptr<H2Core::Note> ) override;
		void setCursorTo( QMouseEvent* ev ) override;
		bool updateKeyboardHoveredElements() override;
		bool updateMouseHoveredElements( QMouseEvent* ev ) override;
		Editor::Input getInput() const override;
		bool syncLasso() override;
		virtual void mouseDrawStart( QMouseEvent* ev ) override;
		virtual void mouseDrawUpdate( QMouseEvent* ev ) override;
		virtual void mouseDrawEnd() override;
		void mouseEditStart( QMouseEvent* ev ) override;
		void mouseEditUpdate( QMouseEvent* ev ) override;
		void mouseEditEnd() override;
		void updateAllComponents( Editor::Update update ) override;
		void updateVisibleComponents( Editor::Update update ) override;
		void updateModifiers( QInputEvent *ev ) override;
		/**
		 * Adjusts #m_nActiveWidth and #m_nEditorWidth to the current
		 * state of the editor.
		 */
		bool updateWidth() override;
		//! @}

public slots:
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

		enum class DragType {
			None,
			Length,
			Property
		};
		static QString DragTypeToQString( DragType dragType );

		/** Scaling factor by which the background colors will be made darker in
		 * case the widget is not in focus. This should help users to determine
		 * which of the editors currently holds focus. */
		static constexpr int nOutOfFocusDim = 110;


		//! Colour to use for rendering and outlining notes
		void applyColor( std::shared_ptr<H2Core::Note> pNote, QPen* pNotePen,
						 QBrush* pNoteBrush, QPen* pNoteTailPen,
						 QBrush* pNoteTailBrush, QPen* pHighlightPen,
						 QBrush* pHighlightBrush, QPen* pMovingPen,
						 QBrush* pMovingBrush, NoteStyle noteStyle ) const;
		//! Calculate colour to use for note representation based on note
		//! velocity.
		static QColor computeNoteColor( float velocity );
		void drawBorders( QPainter& p );
		void drawFocus( QPainter& p );
		//! Draw lines for note grid.
		void drawGridLines( QPainter &p,
							const Qt::PenStyle& style = Qt::SolidLine ) const;
		/** * Draw a note
		 *
		 * @param p Painting device
		 * @param pNote Particular note to draw
		 * @param noteStyle Whether the @a pNote is contained in the pattern
		 *   currently shown in the pattern editor (the one selected in the song
		 *   editor), currently hovered, or selected. */
		void drawNote( QPainter &p, std::shared_ptr<H2Core::Note> pNote,
					   NoteStyle noteStyle ) const;
		/** Update #m_pContentPixmap based on #m_pBackgroundPixmap to show the
		 * latest content of all active pattern. */
		virtual void drawPattern();
		/** If there are multiple notes at the same position and column, the one
		 * with lowest pitch (bottom-most one in PianoRollEditor) will be
		 * rendered up front. If a subset of notes at this point is selected,
		 * the note with lowest pitch within the selection is used. */
		void sortAndDrawNotes( QPainter& p,
							   std::vector< std::shared_ptr<H2Core::Note> > notes,
							   NoteStyle baseStyle );


		/** If the note is left of a NoteOff of the same instrument or of a note
		 * within the same mute group, its sample will only be rendered till
		 * that next note is encountered. We will indicate this behavior by
		 * drawing an effective (more dim) tail of the note. */
		int calculateEffectiveNoteLength( std::shared_ptr<H2Core::Note> pNote ) const;

		/** Checks whether the note would be played back when picked up by the
		 * audio engine. */
		bool checkNotePlayback( std::shared_ptr<H2Core::Note> pNote ) const;

		// How many #m_fGridWidth do make up a quantized grid cell in the
		// current resolution.
		int granularity() const;

		PatternEditorPanel* m_pPatternEditorPanel;
		Property m_property;

		float m_fGridWidth;
		unsigned m_nGridHeight;

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

		int m_nTick;
		QPointF m_drawPreviousPosition;
		H2Core::GridPoint m_drawPreviousGridPoint;
		H2Core::Note::Key m_drawPreviousKey;
		int m_nDrawPreviousOctave;

		// Row the keyboard cursor is residing in.
		//
		// Only in #PianoRollEditor this variable is relevant and updated. In
		// #DrumPatternEditor #PatternEditorPanel::m_nSelectedRowDB is used
		// instead and #NotePropertiesPanel does only contain a single row.
		int m_nCursorPitch;
};

inline float PatternEditor::getGridWidth() const {
	return m_fGridWidth;
}
inline void PatternEditor::setGridWidth( float fGridWidth ) {
	if ( m_fGridWidth != fGridWidth ) {
		m_fGridWidth = fGridWidth;
	}
}
inline unsigned PatternEditor::getGridHeight() const {
	return m_nGridHeight;
}

#endif // PATERN_EDITOR_H
