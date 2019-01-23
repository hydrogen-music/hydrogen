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

#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <hydrogen/object.h>
#include <hydrogen/basics/note.h>
#include <cassert>

#define MAX_EVENTS 1024

namespace H2Core
{

enum EventType {
	EVENT_NONE,
	EVENT_STATE,
	EVENT_PATTERN_CHANGED,
	EVENT_PATTERN_MODIFIED,
	EVENT_SELECTED_PATTERN_CHANGED,
	EVENT_SELECTED_INSTRUMENT_CHANGED,
	EVENT_PARAMETERS_INSTRUMENT_CHANGED,
	EVENT_MIDI_ACTIVITY,
	EVENT_XRUN,
	EVENT_NOTEON,
	EVENT_ERROR,
	EVENT_METRONOME,
	EVENT_RECALCULATERUBBERBAND,
	EVENT_PROGRESS,
	EVENT_JACK_SESSION,
	EVENT_PLAYLIST_LOADSONG,
	EVENT_UNDO_REDO,
	EVENT_SONG_MODIFIED,
	EVENT_TEMPO_CHANGED
};


class Event
{
public:
	EventType type;
	int value;
};

///
/// Event queue: is the way the engine talks to the GUI
///
class EventQueue : public H2Core::Object
{
	H2_OBJECT
public:/**
	* If #__instance equals 0, a new EventQueue singleton will be
	 * created and stored in it.
	 *
	 * It is called in Hydrogen::create_instance().
	 */
	static void create_instance();
	/**
	 * Returns a pointer to the current EventQueue singleton
	 * stored in #__instance.
	 */
	static EventQueue* get_instance() { assert(__instance); return __instance; }
	~EventQueue();

	void push_event( EventType type, int nValue );
	Event pop_event();

		struct AddMidiNoteVector
		{
				int m_column;       //position
				int m_row;          //instrument row
				int m_pattern;      // pattern number
				int m_length;
				float f_velocity;
				float f_pan_L;
				float f_pan_R;
				Note::Key nk_noteKeyVal;
				Note::Octave no_octaveKeyVal;
				bool b_isMidi;
				bool b_isInstrumentMode;
				bool b_noteExist;
		};
		std::vector<AddMidiNoteVector> m_addMidiNoteVector;

private:
	EventQueue();
	/**
	 * Object holding the current EventQueue singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static EventQueue *__instance;

	int __read_index;
	int __write_index;
	Event __events_buffer[ MAX_EVENTS ];
};

};

#endif
