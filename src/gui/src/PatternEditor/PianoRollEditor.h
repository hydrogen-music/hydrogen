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

#ifndef PIANO_ROLL_EDITOR_H
#define PIANO_ROLL_EDITOR_H

#include <hydrogen/object.h>
#include <hydrogen/basics/note.h>
#include "../EventListener.h"
#include "../Selection.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Pattern;
	class Note;
}

class PatternEditorPanel;

class PianoRollEditor: public QWidget, public EventListener, public H2Core::Object
{
    H2_OBJECT
    Q_OBJECT
	public:
		PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel,
						 QScrollArea *pScrollView );
		~PianoRollEditor();


		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		virtual void patternModifiedEvent();
		//~ Implements EventListener interface
		void setResolution(uint res, bool bUseTriplets);

		void zoom_in();
		void zoom_out();

		void addOrDeleteNoteAction( int nColumn,
									int pressedLine,
									int selectedPatternNumber,
									int selectedinstrument,
									int oldLength,
									float oldVelocity,
									float oldPan_L,
									float oldPan_R,
									float oldLeadLag,
									int oldNoteKeyVal,
									int oldOctaveKeyVal,
									bool noteOff,
									bool isDelete );

		void updateModifiers( QInputEvent *ev );
		QPoint movingGridOffset( );

		void moveNoteAction( int nColumn,
							 H2Core::Note::Octave octave,
							 H2Core::Note::Key key,
							 int nPattern,
							 int nNewColumn,
							 H2Core::Note::Octave newOctave,
							 H2Core::Note::Key newKey,
							 H2Core::Note *pNote);

		void editNotePropertiesAction(   int nColumn,
						int nRealColumn,
						int selectedPatternNumber,
						int selectedInstrumentnumber,
						float velocity,
						float pan_L,
						float pan_R,
						float leadLag,
						int pressedLine );
                void editNoteLengthAction( int nColumn,  int nRealColumn, int length, int selectedPatternNumber, int nSelectedInstrumentnumber, int pressedLine );


		// Selection manager interface
		//! Selections are indexed by Note pointers.

		typedef H2Core::Note* SelectionIndex;
		std::vector<SelectionIndex> elementsIntersecting( QRect r );
		void validateSelection();
		void mouseClickEvent( QMouseEvent *ev );
		void mouseDragStartEvent( QMouseEvent *ev );
		void mouseDragUpdateEvent( QMouseEvent *ev );
		void mouseDragEndEvent( QMouseEvent *ev );
		void selectionMoveEndEvent( QInputEvent *ev );
		QRect getKeyboardCursorRect();


	public slots:
		void updateEditor( bool bPatternOnly = false );

		void selectAll();
		void selectNone();
		void deleteSelection();
		void copy();
		void paste();
		void cut();

	private:

		bool m_bNeedsUpdate;
		bool m_bNeedsBackgroundUpdate;
		bool m_bFineGrained;
		bool m_bCopyNotMove;

		void finishUpdateEditor();

		unsigned m_nRowHeight;
		unsigned m_nOctaves;

		uint m_nResolution;
		bool m_bUseTriplets;

		bool m_bSelectNewNotes;

		H2Core::Pattern *m_pPattern;

		float m_nGridWidth;
		uint m_nEditorWidth;
		uint m_nEditorHeight;
		QPixmap *m_pBackground;
		QPixmap *m_pTemp;
		int m_pOldPoint;

		// Note pitch position of cursor, from 0.
		int m_nCursorNote;
		QPoint cursorPosition();

		PatternEditorPanel *m_pPatternEditorPanel;
		QScrollArea *m_pScrollView;
		H2Core::Note *m_pDraggedNote;

		void createBackground();
		void drawPattern();
		void draw_grid(QPainter& p );
		void drawNote( H2Core::Note *pNote, QPainter *pPainter );

		void addOrRemoveNote( int nColumn, int nRealColumn, int nLine,
							  int nNotekey, int nOctave );

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void keyPressEvent ( QKeyEvent * ev );
		virtual void focusInEvent ( QFocusEvent * ev );
		int getColumn(QMouseEvent *ev);

		Selection<PianoRollEditor, SelectionIndex> m_selection;
		QMenu *m_pPopupMenu;

		int __selectedInstrumentnumber;
		int __selectedPatternNumber;
		int __nRealColumn;
		int __nColumn;
		int __pressedLine;
		int __oldLength;
		
		float __velocity;
		float __oldVelocity;
		float __pan_L;
		float __oldPan_L;
		float __pan_R;
		float __oldPan_R;
		float __leadLag;
		float __oldLeadLag;		
};

#endif

