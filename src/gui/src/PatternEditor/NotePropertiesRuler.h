/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

/** \ingroup docGUI*/
//! NotePropertiesEditor is (currently) a single class instantiated in different "modes" to select
//! which property it edits. There are individual instances for each property which are hidden and
//! shown depending on what the user selects.
class NotePropertiesRuler : public PatternEditor, protected WidgetWithScalableFont<7, 9, 11>
{
    H2_OBJECT(NotePropertiesRuler)
	Q_OBJECT
	public:
		enum class Layout {
			Normalized,
			Centered,
			KeyOctave,
		};

		NotePropertiesRuler( QWidget *parent, Mode mode, Layout layout );
		~NotePropertiesRuler();
		
		NotePropertiesRuler(const NotePropertiesRuler&) = delete;
		NotePropertiesRuler& operator=( const NotePropertiesRuler& rhs ) = delete;

		//! @name Property draw (right-click drag) gestures
		//! 
		//! The user can right-click drag notes (or just left-click a single one)
		//! on a note's bar or dot to change that property. Properties are
		//! updated live during the draw gesture, with 'undo' information being
		//! written at the end.
		//! @{
		void propertyDrawStart( QMouseEvent *ev );
		void propertyDrawUpdate( QMouseEvent *ev );
		void propertyDrawEnd();
		//! @}

		//! @name PatternEditor interfaces
		//! @{
		virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
		virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
		virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
		virtual void mouseDragEndEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveUpdateEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;
		virtual void selectionMoveCancelEvent() override;
		//! @}


	public slots:
		virtual void updateEditor( bool bPatternOnly = false ) override;
		virtual void selectAll() override;
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
		void scrolled( int );

	private:

		/** Height of a single line in the key section. */
		static constexpr int nKeyLineHeight = 10;
		/** Height of the whole octave section. */
		static constexpr int nOctaveHeight = 90;
		/** Height of the non-interactive space in KeyOctave editor between
		 * octave and key section. It is contained within the octave part. */
		static constexpr int nKeyOctaveSpaceHeight = 10;
		/** The height of the overall KeyOctave Editor. It will be calculated
		 * during runtime using the other constexprs. */
		static int nKeyOctaveHeight;
		/** Height of all editors except the KeyOctave one. */
		static constexpr int nDefaultHeight = 100;

		void createBackground() override;
	void drawDefaultBackground( QPainter& painter, int nHeight = 0, int nIncrement = 0 );
		void drawPattern() override;
		void drawFocus( QPainter& painter ) override;
		void drawNote( QPainter& painter, H2Core::Note* pNote,
					   NoteStyle noteStyle, int nOffsetX = 0 );

		void paintEvent(QPaintEvent *ev) override;
		void wheelEvent(QWheelEvent *ev) override;
		void keyPressEvent( QKeyEvent *ev ) override;
		void addUndoAction();
		void prepareUndoAction( const QPoint& point );
		void enterEvent( QEvent *ev ) override;
		void leaveEvent( QEvent *ev ) override;

		//! Map of notes currently in the pattern -> old notes with their
		//! properties. Populated at the beginning of a properties editing
		//! gesture.
		std::map< H2Core::Note *, H2Core::Note *> m_oldNotes;
		void clearOldNotes();

		bool adjustNotePropertyDelta( H2Core::Note *pNote,
									  float fDelta,
									  bool bMessage = false );

		int m_nDrawPreviousColumn;
		bool m_bEntered;
		Layout m_layout;
};


#endif
