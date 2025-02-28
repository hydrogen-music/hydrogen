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
		: m_nReadIndex( 0 )
		, m_nWriteIndex( 0 )
		, m_bSilent( false )
{
	__instance = this;

	for ( int i = 0; i < MAX_EVENTS; ++i ) {
		m_eventsBuffer[ i ].type = EVENT_NONE;
		m_eventsBuffer[ i ].value = 0;
	}
}


EventQueue::~EventQueue()
{
//	infoLog( "DESTROY" );
}


void EventQueue::pushEvent( const EventType type, const int nValue )
{
	std::lock_guard< std::mutex > lock( m_mutex );
	unsigned int nIndex = ++m_nWriteIndex;
	nIndex = nIndex % MAX_EVENTS;
	Event ev;
	ev.type = type;
	ev.value = nValue;
//	INFOLOG( QString( "[pushEvent] %1 : %2 %3" ).arg( nIndex ).arg( ev.type ).arg( ev.value ) );

	/* If the event queue is full, log an error. We could drop the old event, or the new event we're trying to
	   place. It's preferable to drop the oldest event in the queue, on the basis that many
	   change-of-state-events are probably no longer relevant or redundant based on newer events in the queue,
	   so we keep the new event. However, since the new event has overwritten the oldest event in the queue,
	   we also adjust the read pointer, otherwise popEvent would return this newest event on the next call,
	   then subsequent calls would get newer entries. */

	if ( ! m_bSilent &&
		 m_nWriteIndex > m_nReadIndex + MAX_EVENTS ) {
		ERRORLOG( QString( "Event queue full, lost event type %1 value %2" )
				  .arg( m_eventsBuffer[nIndex].type )
				  .arg( m_eventsBuffer[nIndex].value ));
		m_nReadIndex++;
	}

	m_eventsBuffer[ nIndex ] = ev;

}


Event EventQueue::popEvent()
{
	std::lock_guard< std::mutex > lock( m_mutex );
	if ( m_nReadIndex == m_nWriteIndex ) {
		Event ev;
		ev.type = EVENT_NONE;
		ev.value = 0;
		return ev;
	}
	unsigned int nIndex = ++m_nReadIndex;
	nIndex = nIndex % MAX_EVENTS;
//	INFOLOG( QString( "[popEvent] %1 : %2 %3" ).arg( nIndex ).arg( m_eventsBuffer[ nIndex ].type ).arg( m_eventsBuffer[ nIndex ].value ) );
	return m_eventsBuffer[ nIndex ];
}

QString EventQueue::toQString( const QString& sPrefix, bool bShort ) {
	std::lock_guard< std::mutex > lock( m_mutex );

	const int nReadIndex =  m_nReadIndex % MAX_EVENTS;
	const int nWriteIndex =  m_nWriteIndex % MAX_EVENTS;

	QString s = Base::sPrintIndention;
	QString sOutput, sIndexPrefix;
	if ( ! bShort ) {
		sOutput = QString( "%1[EventQueue]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nReadIndex: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nReadIndex ) )
			.append( QString( "%1%2m_nWriteIndex: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nWriteIndex ) )
			.append( QString( "%1%2m_eventsBuffer: \n" ).arg( sPrefix ).arg( s ) );
		for ( int ii = 0; ii < MAX_EVENTS; ii++ ) {
			sIndexPrefix = "";
			if ( ii == nReadIndex ) {
				sIndexPrefix.append( "[READ] " );
			}
			if ( ii == nWriteIndex ) {
				sIndexPrefix.append( "[WRITE] " );
			}

			sOutput.append( QString( "%1%1%2%3: %4%5\n" ).arg( sPrefix ).arg( s )
							.arg( ii ).arg( sIndexPrefix )
							.arg( m_eventsBuffer[ ii ].toQString( "", true ) ) );
		}
		sOutput.append( QString( "\n%1%2m_bSilent: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bSilent ) );
	}
	else {
		sOutput = QString( "[EventQueue] " )
			.append( QString( "m_nReadIndex: %1" ).arg( m_nReadIndex ) )
			.append( QString( ", m_nWriteIndex: %1" ).arg( m_nWriteIndex ) )
			.append( QString( ", m_eventsBuffer: [" ) );
		for ( int ii = 0; ii < MAX_EVENTS; ii++ ) {
			sIndexPrefix = "";
			if ( ii == nReadIndex ) {
				sIndexPrefix.append( "[READ] " );
			}
			if ( ii == nWriteIndex ) {
				sIndexPrefix.append( "[WRITE] " );
			}

			sOutput.append( QString( "%1: %2%3, " ).arg( ii ).arg( sIndexPrefix )
							.arg( m_eventsBuffer[ ii ].toQString( "", true ) ) );
		}
		sOutput.append( QString( "], m_bSilent: %1" ).arg( m_bSilent ) );
	}

	return sOutput;
}

};
