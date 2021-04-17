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

#include <core/Object.h>
#include <core/Basics/Note.h>
#include "../EventListener.h"
#include "../Selection.h"
#include "PatternEditor.h"

#include <QtGui>
#include <QtWidgets>

class PianoRollEditor: public PatternEditor
{
    H2_OBJECT
    Q_OBJECT
	public:
		PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel,
						 QScrollArea *pScrollView );
		~PianoRollEditor();


		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
		virtual void patternModifiedEvent() override;
		//~ Implements EventListener interface

		// Pitch / line conversions
		int lineToPitch( int nLine ) {
			return 12 * (OCTAVE_MIN+m_nOctaves) - 1 - nLine;
		}
		int pitchToLine( int nPitch ) {
			return 12 * (OCTAVE_MIN+m_nOctaves) - 1 - nPitch;
		}

		H2Core::Note::Octave pitchToOctave( int nPitch ) {
			if ( nPitch >= 0 ) {
				return (H2Core::Note::Octave)(nPitch / 12);
			} else {
				return (H2Core::Note::Octave)((nPitch-11) / 12);
			}
		}
		H2Core::Note::Key pitchToKey( int nPitch ) {
			return (H2Core::Note::Key)(nPitch - 12 * pitchToOctave( nPitch ));
		}
		int octaveKeyToPitch( H2Core::Note::Octave octave, H2Core::Note::Key key ) {
			return 12 * (int)octave + (int)key;
		}

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
									float fProbability,
									bool noteOff,
									bool isDelete );

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

		virtual std::vector<SelectionIndex> elementsIntersecting( QRect r ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
		virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
		virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
		virtual void mouseDragEndEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		virtual QRect getKeyboardCursorRect() override;


	public slots:
		virtual void updateEditor( bool bPatternOnly = false ) override;
		virtual void selectAll() override;
		virtual void deleteSelection() override;
		virtual void paste() override;

	private:

		bool m_bNeedsUpdate;
		bool m_bNeedsBackgroundUpdate;

		void finishUpdateEditor();

		unsigned m_nOctaves;

		QPixmap *m_pBackground;
		QPixmap *m_pTemp;
		int m_pOldPoint;

		// Note pitch position of cursor
		int m_nCursorPitch;
		QPoint cursorPosition();

		QScrollArea *m_pScrollView;

		void createBackground();
		void drawPattern();
		void drawNote( H2Core::Note *pNote, QPainter *pPainter );

		void addOrRemoveNote( int nColumn, int nRealColumn, int nLine,
							  int nNotekey, int nOctave,
							  bool bDoAdd = true, bool bDoDelete = true );

		virtual void paintEvent(QPaintEvent *ev) override;
		virtual void keyPressEvent ( QKeyEvent * ev ) override;
		virtual void focusInEvent ( QFocusEvent * ev ) override;

		int __selectedInstrumentnumber;
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

