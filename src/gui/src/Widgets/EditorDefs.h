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

#ifndef EDITOR_DEFS_H
#define EDITOR_DEFS_H

#include <QString>

namespace Editor {
	enum class Type {
		Horizontal,
		Grid
	};
	static QString tpeToQString( const Type& type ) {
		switch( type ) {
		case Type::Horizontal:
			return QString( "Horizontal" );
		case Type::Grid:
			return QString( "Grid" );
		default:
			return QString( "Unknown editor type [%1]" )
				.arg( static_cast<int>(type) );
		}
	}

	enum class Instance {
		None = 0,
		DrumPattern = 1,
		PianoRoll = 2,
		NotePropertiesRuler = 3,
		SongEditor = 4
	};
	static QString instanceToQString( const Instance& instance ) {
		switch ( instance ) {
		case Instance::None:
			return "None";
		case Instance::DrumPattern:
			return "DrumPattern";
		case Instance::PianoRoll:
			return "PianoRoll";
		case Instance::NotePropertiesRuler:
			return "NotePropertiesRuler";
		case Instance::SongEditor:
			return "SongEditor";
		default:
			return QString( "Unknown instance [%1]" )
				.arg( static_cast<int>(instance) ) ;
		}
	}

	/** Specifies which parts of the editor need updating on a paintEvent().
	 * Bigger numerical values imply updating elements with lower ones as
	 * well.*/
	enum class Update {
		/** Just paint transient elements, like hovered notes, cursor, focus
		 * or lasso. */
		None = 0,
		/** Update notes, pattern, etc. including selection of a cached
		 * background image. */
		Content = 1,
		/** Update the background image. */
		Background = 2
	};
	static QString updateToQString( const Update& update ) {
		switch ( update ) {
		case Update::Background:
			return "Background";
		case Update::Content:
			return "Content";
		case Update::None:
			return "None";
		default:
			return QString( "Unknown update [%1]" )
				.arg( static_cast<int>(update) ) ;
		}
	}

	/** Main action performed when interacting with elements. */
	enum class Action {
		/** Add a new element */
		Add,
		/** Deletes an existing element */
		Delete,
		/** If an elements exists, delete it. If not, create a new one. */
		Toggle
	};
	static Action undoAction( Action action ) {
		switch( action ) {
			case Action::Add:
				return Action::Delete;
			case Action::Delete:
				return Action::Add;
			case Action::Toggle:
				return Action::Toggle;
			default:
				___ERRORLOG( "Unknown action" );
				return action;
		}
	};
	static QString actionToQString( const Action& action ) {
		switch( action ) {
			case Action::Add:
				return "Delete";
			case Action::Delete:
				return "Add";
			case Action::Toggle:
				return "Toggle";
			default:
				return "Unknown action";
		}
	};

	/** Additional action, which can be performed in combination with
	 * #Action. */
	enum class ActionModifier {
		None = 0x000,
		/** Add the new note to the current selection. */
		AddToSelection = 0x001,
		/** Move cursor to focus newly added note. */
		MoveCursorTo = 0x002,
		/** Play back the new note in case hear notes is enabled. */
		Playback = 0x004
	};
	static QString actionModifierToQString( const ActionModifier& modifier ) {
		QStringList strings;
		if ( static_cast<char>(modifier) &
			 static_cast<char>(ActionModifier::None) ) {
			strings << "None";
		}
		if ( static_cast<char>(modifier) &
			 static_cast<char>(ActionModifier::AddToSelection) ) {
			strings << "AddToSelection";
		}
		if ( static_cast<char>(modifier) &
			 static_cast<char>(ActionModifier::MoveCursorTo) ) {
			strings << "MoveCursorTo";
		}
		if ( static_cast<char>(modifier) &
			 static_cast<char>(ActionModifier::Playback) ) {
			strings << "Playback";
		}
		return strings.join( ", " );
	}

	/** Symbolic step sizes employed on keyboard interactions. */
	enum class Step {
		None = 0,
		Tiny = 1,
		Character = 2,
		Word = 3,
		Page = 4,
		Document = 5
	};
	static QString stepToQString( const Step& step ) {
		switch ( step ) {
		case Step::None:
			return "None";
		case Step::Tiny:
			return "Tiny";
		case Step::Character:
			return "Character";
		case Step::Word:
			return "Word";
		case Step::Page:
			return "Page";
		case Step::Document:
			return "Document";
		default:
			return QString( "Unknown step [%1]" ).arg( static_cast<int>(step) ) ;
		}
	}
	static constexpr int nWordSize = 5;
	static constexpr int nPageSize = 15;

	/** Specifies what happens on left-mouse button interaction. */
	enum class Input {
		/** Clicking will delete an existing element or create a new one and
		 * dragging will either move the clicked note (and its corresponding
		 * selection) or start a new lasso for selection. */
		Select = 0,
		/** Dragging will delete existing/add new elements on all encountered
		 * grid points. For horizontal editors, this might change the values of
		 * existing elements instead. */
		Draw = 1,
		/** Dragging will change the properties of the clicked element. */
		Edit = 2
	};
	static QString inputToQString( const Input& input ) {
		switch ( input ) {
		case Input::Select:
			return "Select";
		case Input::Draw:
			return "Draw";
		case Input::Edit:
			return "Edit";
		default:
			return QString( "Unknown input [%1]" ).arg( static_cast<int>(input) );
		}
	}

	/** Distance in pixel the cursor is allowed to be away from a note to still
	 * be associated with it.
	 *
	 * Note that for very small resolutions a smaller margin will be used to
	 * still allow adding notes to adjacent grid cells. */
	static constexpr int nDefaultCursorMargin = 10;
}

#endif // EDITOR_DEFS_H
