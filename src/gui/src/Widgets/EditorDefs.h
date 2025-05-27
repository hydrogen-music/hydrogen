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
		DrumPattern = 0,
		PianoRoll = 1,
		NotePropertiesRuler = 2,
		None = 3
	};
	static QString instanceToQString( const Instance& instance ) {
		switch ( instance ) {
		case Instance::DrumPattern:
			return "DrumPattern";
		case Instance::PianoRoll:
			return "PianoRoll";
		case Instance::NotePropertiesRuler:
			return "NotePropertiesRuler";
		case Instance::None:
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
				return "Pattern";
			case Update::None:
				return "None";
			default:
				return QString( "Unknown update [%1]" )
					.arg( static_cast<int>(update) ) ;
		}
	}

	/** Additional action to perform on the first redo() call of
	 * #SE_addOrRemoveNotes. */
	enum class Action {
		None = 0x000,
		/** Add the new note to the current selection. */
		AddToSelection = 0x001,
		/** Move cursor to focus newly added note. */
		MoveCursorTo = 0x002,
		/** Play back the new note in case hear notes is enabled. */
		Playback = 0x004
	};

	/** Distance in pixel the cursor is allowed to be away from a note to still
	 * be associated with it.
	 *
	 * Note that for very small resolutions a smaller margin will be used to
	 * still allow adding notes to adjacent grid cells. */
	static constexpr int nDefaultCursorMargin = 10;
}

#endif // EDITOR_DEFS_H
