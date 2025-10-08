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

#include <core/EventQueue.h>

#include <vector>

namespace H2Core
{

EventQueue* EventQueue::__instance = nullptr;

void EventQueue::create_instance()
{
	if ( __instance == nullptr ) {
		__instance = new EventQueue;
	}
}


EventQueue::EventQueue()
		: __read_index( 0 )
		, __write_index( 0 )
		, m_bSilent( false )
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
	std::lock_guard< std::mutex > lock( m_mutex );
	unsigned int nIndex = ++__write_index;
	nIndex = nIndex % MAX_EVENTS;
	Event ev;
	ev.type = type;
	ev.value = nValue;
//	INFOLOG( QString( "[pushEvent] %1 : %2 %3" ).arg( nIndex ).arg( ev.type ).arg( ev.value ) );

	/* If the event queue is full, log an error. We could drop the old event, or the new event we're trying to
	   place. It's preferable to drop the oldest event in the queue, on the basis that many
	   change-of-state-events are probably no longer relevant or redundant based on newer events in the queue,
	   so we keep the new event. However, since the new event has overwritten the oldest event in the queue,
	   we also adjust the read pointer, otherwise pop_event would return this newest event on the next call,
	   then subsequent calls would get newer entries. */

	if ( ! m_bSilent &&
		 __write_index > __read_index + MAX_EVENTS ) {
		ERRORLOG( QString( "Event queue full, lost event type %1 value %2" )
				  .arg( __events_buffer[nIndex].type )
				  .arg( __events_buffer[nIndex].value ));
		__read_index++;
	}

	__events_buffer[ nIndex ] = ev;

}


Event EventQueue::pop_event()
{
	std::lock_guard< std::mutex > lock( m_mutex );
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

void EventQueue::dropEvents( const EventType& type ) {
	std::lock_guard< std::mutex > lock( m_mutex );

	if ( __read_index == __write_index ) {
		return;
	}

	// List of valid elements. Starting with the one nearest to __write_index.
	std::vector<int> indices;
	for ( int ii = __write_index; ii >= __read_index; --ii ) {
		indices.push_back( ii % MAX_EVENTS );
	}

	int nNewIndex = 0;
	for ( int ii = 0; ii < indices.size(); ++ii ) {
		if ( __events_buffer[ indices[ ii ] ].type == type ) {
			// Drop event
			continue;
		}

		if ( ii == nNewIndex ) {
			// Nothing to do
			++nNewIndex;
			continue;
		}

		// Copy an event to a new location.
		__events_buffer[ indices[ nNewIndex ] ].type =
			__events_buffer[ indices[ ii ] ].type;
		__events_buffer[ indices[ nNewIndex ] ].value =
			__events_buffer[ indices[ ii ] ].value;

		++nNewIndex;
	}

	if ( nNewIndex == indices.size() ) {
		return;
	}

	for ( int ii = nNewIndex; ii < indices.size(); ++ii ) {
		__events_buffer[ indices[ ii ] ].type = EVENT_NONE;
		__events_buffer[ indices[ ii ] ].value = 0;
	}

	__read_index += __write_index - __read_index - nNewIndex + 1;
}

};
