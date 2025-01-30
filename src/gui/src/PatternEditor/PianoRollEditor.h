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

#ifndef PIANO_ROLL_EDITOR_H
#define PIANO_ROLL_EDITOR_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Note.h>
#include "../EventListener.h"
#include "../Selection.h"
#include "PatternEditor.h"
#include "NotePropertiesRuler.h"
#include "../Widgets/WidgetWithScalableFont.h"

#include <QtGui>
#include <QtWidgets>

/** \ingroup docGUI*/
class PianoRollEditor: public PatternEditor, protected WidgetWithScalableFont<7, 9, 11>, public H2Core::Object<PianoRollEditor>
{
    H2_OBJECT(PianoRollEditor)
    Q_OBJECT
	public:
		PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel,
						 QScrollArea *pScrollView );
		~PianoRollEditor();


		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
	virtual void songModeActivationEvent() override;
		// ~ Implements EventListener interface

		void addOrDeleteNoteAction( int nColumn,
									int pressedLine,
									int selectedPatternNumber,
									int selectedinstrument,
									int oldLength,
									float oldVelocity,
									float fOldPan,
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


		// Selection manager interface
		//! Selections are indexed by Note pointers.

		virtual std::vector<SelectionIndex> elementsIntersecting( QRect r ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
	virtual void mousePressEvent( QMouseEvent *ev ) override;
		virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
		virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		virtual QRect getKeyboardCursorRect() override;


	public slots:
		virtual void updateEditor( bool bPatternOnly = false ) override;
		virtual void selectAll() override;
		virtual void deleteSelection() override;
		virtual void paste() override;
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private:
		void createBackground() override;
		void drawPattern();
		void drawFocus( QPainter& painter );
		/**
		 * Draw a note
		 *
		 * @param pNote Particular note to draw
		 * @param pPainter Painting device
		 * @param bIsForeground Whether the @a pNote is contained in the
		 *   pattern currently shown in the pattern editor (the one
		 *   selected in the song editor)
		 */
	void drawNote( H2Core::Note *pNote, QPainter *pPainter, bool bIsForeground );

		void addOrRemoveNote( int nColumn, int nRealColumn, int nLine,
							  int nNotekey, int nOctave,
							  bool bDoAdd = true, bool bDoDelete = true );

		virtual void paintEvent(QPaintEvent *ev) override;
		virtual void keyPressEvent ( QKeyEvent * ev ) override;
		
		void finishUpdateEditor();
		
		bool m_bNeedsUpdate;
		bool m_bNeedsBackgroundUpdate;

		QPixmap *m_pBackground;
		QPixmap *m_pTemp;

		// Note pitch position of cursor
		int m_nCursorPitch;
		QPoint cursorPosition();

		QScrollArea *m_pScrollView;
};

#endif

