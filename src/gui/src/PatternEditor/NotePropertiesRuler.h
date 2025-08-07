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

class KeyOctaveLabel : public QLabel, public H2Core::Object<KeyOctaveLabel>
{
	H2_OBJECT(KeyOctaveLabel)
	Q_OBJECT

	public:

		enum class Type {
			Key,
			Octave
		};

		KeyOctaveLabel( QWidget* pParent, const QString& sText, int nY,
						bool bAlternateBackground, Type type );
		~KeyOctaveLabel();

		void updateColors();
		void updateFont();

	private:
		virtual void paintEvent( QPaintEvent* pEvent ) override;

		bool m_bAlternateBackground;
		Type m_type;
		QColor m_backgroundColor;
};

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

		NotePropertiesRuler( QWidget *parent, Property property, Layout layout );
		~NotePropertiesRuler();
		
		NotePropertiesRuler(const NotePropertiesRuler&) = delete;
		NotePropertiesRuler& operator=( const NotePropertiesRuler& rhs ) = delete;

		/** Sets a specific property of @a pNote based on the relative y
		 * coordinate @a fYValue retrieved using eventToYValue(). */
		static bool applyProperty( std::shared_ptr<H2Core::Note> pNote,
								   PatternEditor::Property property,
								   float fYValue );
		/** Transforms the y coordinate of the provided event into a value which
		 * is intended to be used with applyProperty(). */
		float eventToYValue( QMouseEvent* pEvent ) const;

		void updateColors();
		void updateFont();

		//! @name Editor::Base interfaces
		//! @{
		bool canMoveElements() const override { return false; };
		std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;
		void selectionMoveUpdateEvent( QMouseEvent *ev ) override;
		void selectionMoveEndEvent( QInputEvent *ev ) override;
		void selectionMoveCancelEvent() override;
		QPoint gridPointToPoint( const H2Core::GridPoint& gridPoint ) const override;
		H2Core::GridPoint pointToGridPoint( const QPoint& point,
											bool bHonorQuantization ) const override;
		void moveCursorDown( QKeyEvent* ev, Editor::Step step ) override;
		void moveCursorUp( QKeyEvent* ev, Editor::Step step ) override;
		void mouseDrawStart( QMouseEvent *ev ) override;
		void mouseDrawUpdate( QMouseEvent *ev ) override;
		void mouseDrawEnd() override;
		void createBackground() override;
		//! @}


	public slots:
		virtual void selectAll() override;
		void scrolled( int );

	private:

	void drawDefaultBackground( QPainter& painter, int nHeight = 0, int nIncrement = 0 );
		void drawPattern() override;
		void drawNote( QPainter& painter, std::shared_ptr<H2Core::Note> pNote,
					   NoteStyle noteStyle, int nOffsetX = 0 );
		/** Since properties of notes within the same row would end up being
		 * painted on top of eachother, we go through the notes column by column
		 * and add small horizontal offsets to each additional note to hint
		 * their existence.
		 *
		 * In addition, we first aggregate all notes residing at the same
		 * position (column) in the same row and sort them according to their
		 * pitch. This way they order is not seemingly random (else notes would
		 * be order according to the insertion time into the pattern. An
		 * unintuitive measure from user perspective with all our redo/undo
		 * facilities.)
		 *
		 * Also, we ensure selected notes will be rendered more prominently than
		 * not selected ones. */
		void sortAndDrawNotes( QPainter& p,
							   std::vector< std::shared_ptr<H2Core::Note> > notes,
							   NoteStyle baseStyle,
							   bool bUpdateOffsets );

		void paintEvent(QPaintEvent *ev) override;
		void wheelEvent(QWheelEvent *ev) override;
		void addUndoAction( const QString& sUndoContext );
		void prepareUndoAction( QMouseEvent* pEvent );

		std::vector<KeyOctaveLabel*> m_labels;

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
		void applyCursorDelta( float fDelta );

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
