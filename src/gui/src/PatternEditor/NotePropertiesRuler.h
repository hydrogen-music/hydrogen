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

#include <QtGui>
#include <QtWidgets>

#include <core/Basics/Note.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <map>
#include <memory>
#include <set>

#include "PatternEditor.h"

/** \ingroup docGUI*/
//! NotePropertiesEditor is (currently) a single class instantiated in different "modes" to select
//! which property it edits. There are individual instances for each property which are hidden and
//! shown depending on what the user selects.
class NotePropertiesRuler : public PatternEditor,
							public H2Core::Object<NotePropertiesRuler>
{
    H2_OBJECT(NotePropertiesRuler)
	Q_OBJECT
	public:
		enum class Layout {
			Normalized,
			Centered,
			KeyOctave,
		};

		NotePropertiesRuler( QWidget *parent, Property property, Layout layout );
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
		virtual bool canMoveElements() const override { return false; };
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
		virtual void selectAll() override;
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
		void drawNote( QPainter& painter, std::shared_ptr<H2Core::Note> pNote,
					   NoteStyle noteStyle, int nOffsetX = 0 );

		void paintEvent(QPaintEvent *ev) override;
		void wheelEvent(QWheelEvent *ev) override;
		void keyPressEvent( QKeyEvent *ev ) override;
		void addUndoAction( const QString& sUndoContext );
		void prepareUndoAction( QMouseEvent* pEvent );

		//! Map of notes currently in the pattern -> old notes with their
		//! properties. Populated at the beginning of a properties editing
		//! gesture.
		std::map< std::shared_ptr<H2Core::Note>,
				  std::shared_ptr<H2Core::Note> > m_oldNotes;

		/** In order to ensure hovering a note in another editor highlight
		 * corresponding note in the ruler, we have to store the x offset the
		 * note was rendered with and apply it to the hovered note as well. */
		std::map< std::shared_ptr<H2Core::Note>, int > m_offsetMap;

		/** @param bKey If set to `true`, @a fDelta will be applied to the key
		 * section in Property::KeyOctave. */
		bool adjustNotePropertyDelta(
			std::vector< std::shared_ptr<H2Core::Note> > notes, float fDelta,
			bool bKey );

		int m_nDrawPreviousColumn;
		Layout m_layout;

		/** Retrieve all (foreground) notes currently rendered in the ruler.
		 *
		 * This includes:
		 *  1. notes in the selected row of the current pattern
		 *  2. already selected notes (of the current pattern but possibly from
		 *     another row)
		 *  3. hovered notes */
		std::set< std::shared_ptr<H2Core::Note> > getAllNotes() const;
};


#endif
