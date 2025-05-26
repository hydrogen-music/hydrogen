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

#ifndef NOTE_PROPERTIES_RULER_H
#define NOTE_PROPERTIES_RULER_H

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Basics/Note.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <map>

#include "PatternEditor.h"

namespace H2Core
{
	class Pattern;
	class NoteKey;
}

class PatternEditorPanel;

/** \ingroup docGUI*/
//! NotePropertiesEditor is (currently) a single class instantiated in different "modes" to select
//! which property it edits. There are individual instances for each property which are hidden and
//! shown depending on what the user selects.
class NotePropertiesRuler : public PatternEditor, protected WidgetWithScalableFont<7, 9, 11>
{
    H2_OBJECT(NotePropertiesRuler)
	Q_OBJECT
	public:

		NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, Mode mode );
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
	virtual void mousePressEvent( QMouseEvent *ev ) override;
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
		void onPreferencesChanged( H2Core::Preferences::Changes changes );
		void scrolled( int );

	private:

		/** Height of a single line in the key section. */
		static constexpr int nNoteKeyLineHeight = 10;
		/** Height of the whole octave section. */
		static constexpr int nNoteKeyOctaveHeight = 90;
		/** Height of the non-interactive space in NoteKey editor between octave
		 * and key section. It is contained within the octave part. */
		static constexpr int nNoteKeySpaceHeight = 10;
		/** The height of the overall NoteKey Editor. It will be calculated
		 * during runtime using the other constexprs. */
		static int nNoteKeyHeight;
		/** Height of all editors except the NoteKey one. */
		static constexpr int nDefaultHeight = 100;

		bool m_bNeedsUpdate;
		void createBackground() override;
	void drawDefaultBackground( QPainter& painter, int nHeight = 0, int nIncrement = 0 );
		void drawFocus( QPainter& painter );

		double m_fLastSetValue;
		bool m_bValueHasBeenSet;

		void createNormalizedBackground(QPixmap *pixmap);
		void createCenteredBackground(QPixmap *pixmap);
		void createNoteKeyBackground(QPixmap *pixmap);

		void paintEvent(QPaintEvent *ev) override;
		void wheelEvent(QWheelEvent *ev) override;
		void keyPressEvent( QKeyEvent *ev ) override;
		void addUndoAction();
		void prepareUndoAction( int x );
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
		void leaveEvent( QEvent *ev ) override;

		virtual void mouseMoveEvent( QMouseEvent *ev ) override;


		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
	virtual void songModeActivationEvent() override;
		// ~ Implements EventListener interface
		
		int m_nSelectedPatternNumber;

		//! Map of notes currently in the pattern -> old notes with their properties. Populated at the
		//! beginning of a properties editing gesture.
		std::map< H2Core::Note *, H2Core::Note *> m_oldNotes;
		void clearOldNotes();

		void adjustNotePropertyDelta( H2Core::Note *pNote, float fDelta, bool bMessage = false );

		int m_nDragPreviousColumn;
		bool m_bEntered;
};


#endif
