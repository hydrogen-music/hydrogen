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

#include <hydrogen/event_queue.h>

namespace H2Core
{

EventQueue* EventQueue::__instance = nullptr;

void EventQueue::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new EventQueue;
	}
}

const char* EventQueue::__class_name = "EventQueue";

EventQueue::EventQueue()
		: Object( __class_name )
		, __read_index( 0 )
		, __write_index( 0 )
{
	__instance = this;

	for ( int i = 0; i < MAX_EVENTS; ++i ) {
		__events_buffer[ i ].type = EVENT_NONE;
		__events_buffer[ i ].value = 0;
	}
}


EventQueue::~EventQueue()
{
//	infoLog( "DESTROY" );
}


void EventQueue::push_event( const EventType type, const int nValue )
{
	unsigned int nIndex = ++__write_index;
	nIndex = nIndex % MAX_EVENTS;
	Event ev;
	ev.type = type;
	ev.value = nValue;
//	INFOLOG( QString( "[pushEvent] %1 : %2 %3" ).arg( nIndex ).arg( ev.type ).arg( ev.value ) );
	__events_buffer[ nIndex ] = ev;
}


Event EventQueue::pop_event()
{
	if ( __read_index == __write_index ) {
		Event ev;
		ev.type = EVENT_NONE;
		ev.value = 0;
		return ev;
	}
	unsigned int nIndex = ++__read_index;
	nIndex = nIndex % MAX_EVENTS;
//	INFOLOG( QString( "[popEvent] %1 : %2 %3" ).arg( nIndex ).arg( __events_buffer[ nIndex ].type ).arg( __events_buffer[ nIndex ].value ) );
	return __events_buffer[ nIndex ];
}

};
