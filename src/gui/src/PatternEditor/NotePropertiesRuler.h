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

#ifndef NOTE_PROPERTIES_RULER_H
#define NOTE_PROPERTIES_RULER_H

#include "../EventListener.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <map>

#include "PatternEditor.h"

namespace H2Core
{
	class Pattern;
	class NoteKey;
}

class PatternEditorPanel;

class NotePropertiesRuler : public PatternEditor
{
    H2_OBJECT
	Q_OBJECT
	public:
		//! NotePropertiesEditor is (currently) a single class instantiated in different "modes" to select
		//! which property it edits. There are individual instances for each property which are hidden and
		//! shown depending on what the user selects.
		enum NotePropertiesMode {
			VELOCITY,
			PAN,
			LEADLAG,
			NOTEKEY,
			PROBABILITY
		};

		NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode );
		~NotePropertiesRuler();
		
		NotePropertiesRuler(const NotePropertiesRuler&) = delete;
		NotePropertiesRuler& operator=( const NotePropertiesRuler& rhs ) = delete;

		//! @name Property drag (or click) gestures
		//! 
		//! The user can drag (or just click) on a note's bar or dot to change that property. Properties are
		//! updated live during the drag gesture, with 'undo' information being written at the end.
		//! @{
		void propertyDragStart( QMouseEvent *ev );
		void propertyDragUpdate( QMouseEvent *ev );
		void propertyDragEnd();
		//! @}

		//! @name PatternEditor interfaces
		//! @{
		virtual std::vector<SelectionIndex> elementsIntersecting( QRect r ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
		virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
		virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
		virtual void mouseDragEndEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveUpdateEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		virtual void selectionMoveCancelEvent() override;
		virtual QRect getKeyboardCursorRect() override;
		//! @}

	public slots:
		virtual void updateEditor( bool bPatternOnly = false ) override;
		virtual void selectAll() override;
		virtual void deleteSelection() override {}
		virtual void copy() override {}
		virtual void paste() override {}
		virtual void cut() override {}

	private:

		bool m_bNeedsUpdate;
		void finishUpdateEditor();

		NotePropertiesMode m_Mode;

		QPixmap *m_pBackground;

		double m_fLastSetValue;
		bool m_bValueHasBeenSet;

		void createVelocityBackground(QPixmap *pixmap);
		void createPanBackground(QPixmap *pixmap);
		void createLeadLagBackground(QPixmap *pixmap);
		void createNoteKeyBackground(QPixmap *pixmap);

		void paintEvent(QPaintEvent *ev) override;
		void wheelEvent(QWheelEvent *ev) override;
		void keyPressEvent( QKeyEvent *ev ) override;
		void focusInEvent( QFocusEvent *ev ) override;
		void focusOutEvent( QFocusEvent *ev ) override;
		void addUndoAction();
		void prepareUndoAction( int x );

		virtual void mouseMoveEvent( QMouseEvent *ev ) override;


		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
		//~ Implements EventListener interface
		
		int m_nSelectedPatternNumber;

		//! Map of notes currently in the pattern -> old notes with their properties. Populated at the
		//! beginning of a properties editing gesture.
		std::map< H2Core::Note *, H2Core::Note *> m_oldNotes;
		void clearOldNotes();

		void adjustNotePropertyDelta( H2Core::Note *pNote, float fDelta, bool bMessage = false );

		double m_nDragPreviousColumn;
};


#endif
