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

#include <hydrogen/EventQueue.h>

namespace H2Core {

EventQueue* EventQueue::m_pInstance = NULL;

EventQueue* EventQueue::getInstance()
{
	if ( !m_pInstance ) {
		m_pInstance = new EventQueue();
	}
	return m_pInstance;
}


EventQueue::EventQueue()
 : Object( "EventQueue" )
 , m_nReadIndex( 0 )
 , m_nWriteIndex( 0 )
{
//	infoLog( "INIT" );

	for ( int i = 0; i < MAX_EVENTS; ++i ) {
		m_eventsBuffer[ i ].m_type = EVENT_NONE;
		m_eventsBuffer[ i ].m_nValue = 0;
	}
}


EventQueue::~EventQueue()
{
//	infoLog( "DESTROY" );
}


void EventQueue::pushEvent( EventType type, int nValue )
{
	int nIndex = ++m_nWriteIndex;
	nIndex = nIndex % MAX_EVENTS;
//	infoLog( "[pushEvent] " + toString( nIndex ) );
	Event ev;
	ev.m_type = type;
	ev.m_nValue = nValue;
	m_eventsBuffer[ nIndex ] = ev;
}


Event EventQueue::popEvent()
{
	if ( m_nReadIndex == m_nWriteIndex ) {
		Event ev;
		ev.m_type = EVENT_NONE;
		ev.m_nValue = 0;
		return ev;
	}
	int nIndex = ++m_nReadIndex;
	nIndex = nIndex % MAX_EVENTS;
//	infoLog( "[popEvent] " + toString( nIndex ) );
	return m_eventsBuffer[ nIndex ];
}

};
