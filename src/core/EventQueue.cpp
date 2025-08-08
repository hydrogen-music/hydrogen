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

#include <core/Hydrogen.h>

namespace H2Core
{

EventQueue* EventQueue::__instance = nullptr;

void EventQueue::create_instance()
{
	if ( __instance == nullptr ) {
		__instance = new EventQueue;
	}
}


EventQueue::EventQueue() : m_bSilent( false ) {
	__instance = this;
}


EventQueue::~EventQueue() {
}


void EventQueue::pushEvent( const Event::Type type, const int nValue ) {
	std::lock_guard< std::mutex > lock( m_mutex );

	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen == nullptr ||
		 pHydrogen->getGUIState() == Hydrogen::GUIState::startup ||
		 pHydrogen->getGUIState() == Hydrogen::GUIState::shutdown ) {
		return;
	}

	/* The event queue is full. We could drop the old event, or the new event
	   we're trying to place. It's preferable to drop the oldest event in the
	   queue, on the basis that many change-of-state-events are probably no
	   longer relevant or redundant based on newer events in the queue, so we
	   keep the new event. */
	while ( m_eventQueue.size() >= EventQueue::nMaxEvents ) {
		auto pOldestEvent = std::move( m_eventQueue.front() );
		m_eventQueue.pop_front();
		if ( ! m_bSilent ) {
			ERRORLOG( QString( "Event queue full. Dropping oldest event: [%1]" )
					  .arg( pOldestEvent != nullptr ?
							pOldestEvent->toQString() : "nullptr" ) );
		}
	}

	m_eventQueue.push_back( std::make_unique<Event>( type, nValue ) );
}

std::unique_ptr<Event> EventQueue::popEvent() {
	std::lock_guard< std::mutex > lock( m_mutex );

	if ( m_eventQueue.empty() ) {
		return nullptr;
	}

	auto pEvent = std::move( m_eventQueue.front() );
	m_eventQueue.pop_front();

	return std::move( pEvent );
}

QString EventQueue::toQString( const QString& sPrefix, bool bShort ) {
	std::lock_guard< std::mutex > lock( m_mutex );

	QString s = Base::sPrintIndention;
	QString sOutput, sIndexPrefix;
	if ( ! bShort ) {
		sOutput = QString( "%1[EventQueue]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bSilent: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bSilent ) )
			.append( QString( "%1%2m_eventQueue: \n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppEvent : m_eventQueue ) {
			sOutput.append( QString( "%1" )
							.arg( ppEvent->toQString( sPrefix + s, bShort ) ) );
		}
	}
	else {
		sOutput = QString( "[EventQueue] " )
			.append( QString( "m_bSilent: %1" ).arg( m_bSilent ) )
			.append( QString( ", m_eventQueue: [" ) );
		for ( const auto& ppEvent : m_eventQueue ) {
			sOutput.append( QString( "%1" )
							.arg( ppEvent->toQString( "", bShort ) ) );
		}
	}

	return sOutput;
}

};
