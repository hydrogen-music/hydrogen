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

#include <core/IO/LoopBackMidiDriver.h>

namespace H2Core {

LoopBackMidiDriver::LoopBackMidiDriver() : MidiBaseDriver()
										 , m_bActive( false )
{
}

LoopBackMidiDriver::~LoopBackMidiDriver() {
	if ( m_bActive || m_pMessageHandler != nullptr ) {
		close();
	}
}

void LoopBackMidiDriver::close() {
	m_bActive = false;

	if ( m_pMessageHandler != nullptr ) {
		{
			std::scoped_lock lock{ m_messageHandlerMutex };
			m_messageHandlerCV.notify_all();
		}
		m_pMessageHandler->join();
		m_pMessageHandler = nullptr;
	}
}

std::vector<QString> LoopBackMidiDriver::getExternalPortList( const PortType& portType ) {
	std::vector<QString> empty;
	return std::move( empty );
}

bool LoopBackMidiDriver::isInputActive() const {
	return m_bActive;
}
bool LoopBackMidiDriver::isOutputActive() const {
	return m_bActive;
}

void LoopBackMidiDriver::open() {
	if ( m_bActive || m_pMessageHandler != nullptr ) {
		close();
	}

	std::unique_lock lock{ m_messageHandlerMutex };

	m_pMessageHandler = std::make_shared< std::thread >(
		LoopBackMidiDriver::messageHandler, ( void* )this );

	m_messageHandlerCV.wait( lock, [&]{ return m_bActive; } );
}

std::vector<MidiMessage> LoopBackMidiDriver::getBacklogMessages() {
    std::scoped_lock lock{ m_messageHandlerMutex };

	std::vector<MidiMessage> messages;
	messages.reserve( m_backlogQueue.size() );

	for ( const auto& mmessage : m_backlogQueue ) {
		messages.push_back( MidiMessage( mmessage ) );
	}

	return std::move( messages );
}

void LoopBackMidiDriver::clearBacklogMessages() {
	std::scoped_lock lock{ m_messageHandlerMutex };

	m_backlogQueue.clear();
}

int LoopBackMidiDriver::getMessageQueueSize() {
	std::scoped_lock lock{ m_messageHandlerMutex };

	return m_messageQueue.size();
}

QString LoopBackMidiDriver::toQString( const QString& sPrefix, bool bShort ) {
    std::scoped_lock lock{ m_messageHandlerMutex };

	const QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[LoopBackMidiDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bActive: %3,\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bActive ) )
			.append( QString( "%1%2m_messageQueue: [\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& mmessage : m_messageQueue ) {
			sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
							.arg( mmessage.toQString() ) );
		}
		sOutput.append( QString( "%1%2]\n,%1%2m_backlogQueue\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& mmessage : m_backlogQueue ) {
			sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
							.arg( mmessage.toQString() ) );
		}
		sOutput.append( QString( "%1%2]\n" ).arg( sPrefix ).arg( s ) );
	}
	else {
		sOutput = QString( "[LoopBackMidiDriver]" )
			.append( QString( ", m_bActive: %1" ).arg( m_bActive ) )
			.append( ", m_messageQueue: [" );
		for ( const auto& mmessage : m_messageQueue ) {
			sOutput.append( QString( " %1 " ).arg( mmessage.toQString() ) );
		}
		sOutput.append( "], m_backlogQueue: [" );
		for ( const auto& mmessage : m_backlogQueue ) {
			sOutput.append( QString( " %1 " ).arg( mmessage.toQString() ) );
		}
		sOutput.append( "]" );
	}

	return sOutput;
}

void LoopBackMidiDriver::sendControlChangeMessage( const MidiMessage& msg ) {
	enqueueMessage( msg );
}
void LoopBackMidiDriver::sendNoteOnMessage( const MidiMessage& msg ) {
	enqueueMessage( msg );
}
void LoopBackMidiDriver::sendNoteOffMessage( const MidiMessage& msg ) {
	enqueueMessage( msg );
}
void LoopBackMidiDriver::sendSystemRealTimeMessage( const MidiMessage& msg ) {
	enqueueMessage( msg );
}

void LoopBackMidiDriver::messageHandler( void* pInstance ) {
	auto pDriver = static_cast<LoopBackMidiDriver*>( pInstance );
	if ( pDriver == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	// Signal the instance that we are ready.
	pDriver->m_bActive = true;
	pDriver->m_messageHandlerCV.notify_all();

	// Immediately provide output MIDI events as input events.
	while ( pDriver->m_bActive ) {
		std::unique_lock lock{ pDriver->m_messageHandlerMutex };
		pDriver->m_messageHandlerCV.wait(
			lock, [&]{ return pDriver->m_messageQueue.size() > 0 ||
					! pDriver->m_bActive; } );

		if ( ! pDriver->m_bActive ) {
			return;
		}

		while ( pDriver->m_messageQueue.size() > 0 ) {
			const auto midiMessage = pDriver->m_messageQueue.front();
			pDriver->m_backlogQueue.push_back( MidiMessage( midiMessage ) );
			pDriver->enqueueInputMessage( MidiMessage::from( midiMessage ) );
			pDriver->m_messageQueue.pop_front();

			if ( pDriver->m_backlogQueue.size() >
				 LoopBackMidiDriver::nBacklogSize ) {
				pDriver->m_backlogQueue.pop_front();
			}
		}
	}
}

void LoopBackMidiDriver::enqueueMessage( const MidiMessage& msg ) {
	if ( ! m_bActive || m_pMessageHandler == nullptr ) {
		return;
	}

    std::scoped_lock lock{ m_messageHandlerMutex };

	m_messageQueue.push_back( msg );
	if ( m_messageQueue.size() > LoopBackMidiDriver::nMaxQueueSize ) {
		WARNINGLOG( QString( "Message queue is full. Dropping first message [%1]" )
					.arg( m_messageQueue.front().toQString() ) );
		m_messageQueue.pop_front();
	}

	m_messageHandlerCV.notify_all();
}

};
