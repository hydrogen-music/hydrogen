/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/Object.h>

#define MAX_EVENTS 1024

namespace H2Core {

enum EventType {
	EVENT_NONE,
	EVENT_STATE,
	EVENT_PATTERN_CHANGED,
	EVENT_PATTERN_MODIFIED,
	EVENT_SELECTED_PATTERN_CHANGED,
	EVENT_SELECTED_INSTRUMENT_CHANGED,
	EVENT_MIDI_ACTIVITY,
	EVENT_XRUN,
	EVENT_NOTEON,
	EVENT_ERROR,
	EVENT_METRONOME,
	EVENT_PROGRESS
};


class Event {
	public:
		EventType m_type;
		int m_nValue;
};


class EventQueue : public Object
{
	public:
		static EventQueue* getInstance();
		~EventQueue();

		void pushEvent( EventType type, int nValue );
		Event popEvent();

	private:
		EventQueue();
		static EventQueue *m_pInstance;

		int m_nReadIndex;
		int m_nWriteIndex;
		Event m_eventsBuffer[ MAX_EVENTS ];
};

};

#endif
