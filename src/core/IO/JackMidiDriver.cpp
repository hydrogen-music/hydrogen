/*-
 * Copyright (c) 2011 Hans Petter Selasky <hselasky@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <core/IO/JackMidiDriver.h>

#if defined( H2CORE_HAVE_JACK ) || _DOXYGEN_

#include <core/AudioEngine/AudioEngine.h>
#include <core/Globals.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiMessage.h>
#include <core/NsmClient.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {

void JackMidiDriver::lockMidiPart( void )
{
	pthread_mutex_lock( &m_midiMutex );
}

void JackMidiDriver::unlockMidiPart( void )
{
	pthread_mutex_unlock( &m_midiMutex );
}

void JackMidiDriver::writeJackMidi( jack_nframes_t nframes )
{
	int error;
	int events;
	int i;
	void* buf;
	jack_midi_event_t event;
	uint8_t buffer[13];	 // 13 is needed if we get sysex goto messages

	if ( m_pMidiInputPort == nullptr ) {
		return;
	}

	buf = jack_port_get_buffer( m_pMidiInputPort, nframes );
	if ( buf == nullptr ) {
		return;
	}

#ifdef JACK_MIDI_NEEDS_NFRAMES
	events = jack_midi_get_event_count( buf, nframes );
#else
	events = jack_midi_get_event_count( buf );
#endif

	for ( i = 0; i < events; i++ ) {
#ifdef JACK_MIDI_NEEDS_NFRAMES
		error = jack_midi_event_get( &event, buf, i, nframes );
#else
		error = jack_midi_event_get( &event, buf, i );
#endif
		if ( error ) {
			continue;
		}

		if ( m_nRunning < 1 ) {
			continue;
		}

		MidiMessage msg;

		error = event.size;
		if ( error > (int) sizeof( buffer ) ) {
			error = (int) sizeof( buffer );
		}

		memset( buffer, 0, sizeof( buffer ) );
		memcpy( buffer, event.buffer, error );

		msg.setType( MidiMessage::deriveType( buffer[0] ) );
		msg.setChannel( MidiMessage::deriveChannel( buffer[0] ) );
		if ( msg.getType() == MidiMessage::Type::Sysex ) {
			if ( buffer[3] == 06 ) {  // MMC message
				for ( int i = 0; i < sizeof( buffer ) && i < 6; i++ ) {
					msg.appendToSysexData( buffer[i] );
				}
			}
			else {
				for ( int i = 0; i < sizeof( buffer ); i++ ) {
					msg.appendToSysexData( buffer[i] );
				}
			}
		}
		else {
			// All other MIDI messages
			msg.setData1( Midi::parameterFromIntClamp( buffer[1] ) );
			msg.setData2( Midi::parameterFromIntClamp( buffer[2] ) );
		}
		enqueueInputMessage( msg );
	}
}

void JackMidiDriver::sendControlChangeMessage( const MidiMessage& msg )
{
	uint8_t buffer[4];

	// Midi::Channel within Hydrogen represent user-facing values. Since the
	// numerical value of channel `1` is `0` within the MIDI standard, we have
	// to convert it.
	buffer[0] = 0xB0 | ( static_cast<int>( msg.getChannel() ) - 1 );
	buffer[1] = static_cast<int>( msg.getData1() );
	buffer[2] = static_cast<int>( msg.getData2() );
	buffer[3] = 0;

	jackMidiOutEvent( buffer, 3 );
}

void JackMidiDriver::readJackMidi( jack_nframes_t nframes )
{
	uint8_t* buffer;
	void* buf;
	jack_nframes_t t;
	uint8_t data[1];
	uint8_t len;

	if ( m_pMidiOutputPort == nullptr ) {
		return;
	}

	buf = jack_port_get_buffer( m_pMidiOutputPort, nframes );
	if ( buf == nullptr ) {
		return;
	}

#ifdef JACK_MIDI_NEEDS_NFRAMES
	jack_midi_clear_buffer( buf, nframes );
#else
	jack_midi_clear_buffer( buf );
#endif

	t = 0;
	lockMidiPart();
	while ( ( t < nframes ) && ( m_midiRxOutPosition != m_midiRxInPosition ) ) {
		len = m_jackMidiBuffer[4 * m_midiRxInPosition];
		if ( len == 0 ) {
			m_midiRxInPosition++;
			if ( m_midiRxInPosition >= JACK_MIDI_BUFFER_MAX ) {
				m_midiRxInPosition = 0;
			}
			continue;
		}

#ifdef JACK_MIDI_NEEDS_NFRAMES
		buffer = jack_midi_event_reserve( buf, t, len, nframes );
#else
		buffer = jack_midi_event_reserve( buf, t, len );
#endif
		if ( buffer == nullptr ) {
			break;
		}
		t++;
		m_midiRxInPosition++;
		if ( m_midiRxInPosition >= JACK_MIDI_BUFFER_MAX ) {
			m_midiRxInPosition = 0;
		}
		memcpy( buffer, m_jackMidiBuffer + ( 4 * m_midiRxInPosition ) + 1, len );
	}
	unlockMidiPart();
}

void JackMidiDriver::jackMidiOutEvent( uint8_t buf[4], uint8_t len )
{
	uint32_t next_pos;

	lockMidiPart();

	next_pos = m_midiRxOutPosition + 1;
	if ( next_pos >= JACK_MIDI_BUFFER_MAX ) {
		next_pos = 0;
	}

	if ( next_pos == m_midiRxInPosition ) {
		/* buffer is full */
		unlockMidiPart();
		return;
	}

	if ( len > 3 ) {
		len = 3;
	}

	m_jackMidiBuffer[( 4 * next_pos )] = len;
	m_jackMidiBuffer[( 4 * next_pos ) + 1] = buf[0];
	m_jackMidiBuffer[( 4 * next_pos ) + 2] = buf[1];
	m_jackMidiBuffer[( 4 * next_pos ) + 3] = buf[2];

	m_midiRxOutPosition = next_pos;

	unlockMidiPart();
}

static int JackMidiProcessCallback( jack_nframes_t nframes, void* arg )
{
	JackMidiDriver* jmd = (JackMidiDriver*) arg;

	if ( nframes <= 0 ) {
		return ( 0 );
	}

	jmd->readJackMidi( nframes );
	jmd->writeJackMidi( nframes );

	return ( 0 );
}

static void JackMidiShutdown( void* arg )
{
	UNUSED( arg );
	Hydrogen::get_instance()->getAudioEngine()->raiseError(
		Hydrogen::JACK_SERVER_SHUTDOWN
	);
}

JackMidiDriver::JackMidiDriver() : MidiBaseDriver()
{
	pthread_mutex_init( &m_midiMutex, nullptr );

	m_nRunning = 0;
	m_midiRxInPosition = 0;
	m_midiRxOutPosition = 0;
	m_pMidiOutputPort = nullptr;
	m_pMidiInputPort = nullptr;

	QString jackMidiClientId = "Hydrogen";

#ifdef H2CORE_HAVE_OSC
	const QString sNsmClientId = NsmClient::get_instance()->getClientId();

	if ( !sNsmClientId.isEmpty() ) {
		jackMidiClientId = sNsmClientId;
	}
#endif

	jackMidiClientId.append( "-midi" );

	m_pJackClient = jack_client_open(
		jackMidiClientId.toLocal8Bit(), JackNoStartServer, nullptr
	);

	if ( m_pJackClient == nullptr ) {
		return;
	}

	jack_set_process_callback( m_pJackClient, JackMidiProcessCallback, this );

	jack_on_shutdown( m_pJackClient, JackMidiShutdown, nullptr );

	m_pMidiOutputPort = jack_port_register(
		m_pJackClient, "TX", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
	);

	m_pMidiInputPort = jack_port_register(
		m_pJackClient, "RX", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
	);

	jack_activate( m_pJackClient );
}

JackMidiDriver::~JackMidiDriver()
{
	if ( m_pJackClient != nullptr ) {
		if ( jack_port_unregister( m_pJackClient, m_pMidiInputPort ) != 0 ) {
			ERRORLOG( "Failed to unregister jack midi input out" );
		}

		if ( jack_port_unregister( m_pJackClient, m_pMidiOutputPort ) != 0 ) {
			ERRORLOG( "Failed to unregister jack midi input out" );
		}

		// jack_port_unregister( jack_client, output_port);
		if ( jack_deactivate( m_pJackClient ) != 0 ) {
			ERRORLOG( "Failed to unregister jack midi input out" );
		}

		if ( jack_client_close( m_pJackClient ) != 0 ) {
			ERRORLOG( "Failed close jack midi client" );
		}
	}
	pthread_mutex_destroy( &m_midiMutex );
}

void JackMidiDriver::close()
{
	m_nRunning--;
}

bool JackMidiDriver::isInputActive() const
{
	return m_pJackClient != nullptr && m_pMidiInputPort != nullptr;
}

bool JackMidiDriver::isOutputActive() const
{
	return m_pJackClient != nullptr && m_pMidiOutputPort != nullptr;
}

void JackMidiDriver::open()
{
	m_nRunning++;
}

std::vector<QString> JackMidiDriver::getExternalPortList(
	const PortType& portType
)
{
	std::vector<QString> portList;

	portList.push_back( "Default" );

	return portList;
}

void JackMidiDriver::getPortInfo(
	const QString& sPortName,
	int& nClient,
	int& nPort
)
{
	if ( sPortName == Preferences::getNullMidiPort() ) {
		nClient = -1;
		nPort = -1;
		return;
	}

	nClient = 0;
	nPort = 0;
}

void JackMidiDriver::sendNoteOnMessage( const MidiMessage& msg )
{
	uint8_t buffer[4];

	// Midi::Channel within Hydrogen represent user-facing values. Since the
	// numerical value of channel `1` is `0` within the MIDI standard, we have
	// to convert it.
	buffer[0] =
		0x90 | ( static_cast<int>( msg.getChannel() ) - 1 ); /* note on */
	buffer[1] = static_cast<int>( msg.getData1() );
	buffer[2] = static_cast<int>( msg.getData2() );
	buffer[3] = 0;

	jackMidiOutEvent( buffer, 3 );
}

void JackMidiDriver::sendNoteOffMessage( const MidiMessage& msg )
{
	uint8_t buffer[4];

	// Midi::Channel within Hydrogen represent user-facing values. Since the
	// numerical value of channel `1` is `0` within the MIDI standard, we have
	// to convert it.
	buffer[0] =
		0x80 | ( static_cast<int>( msg.getChannel() ) - 1 ); /* note off */
	buffer[1] = static_cast<int>( msg.getData1() );
	buffer[2] = 0;
	buffer[3] = 0;

	jackMidiOutEvent( buffer, 3 );
}

void JackMidiDriver::sendSystemRealTimeMessage( const MidiMessage& msg )
{
	uint8_t buffer[4];

	if ( msg.getType() == MidiMessage::Type::Start ) {
		buffer[0] = 0xFA;
	}
	else if ( msg.getType() == MidiMessage::Type::Continue ) {
		buffer[0] = 0xFB;
	}
	else if ( msg.getType() == MidiMessage::Type::Stop ) {
		buffer[0] = 0xFC;
	}
	else if ( msg.getType() == MidiMessage::Type::TimingClock ) {
		buffer[0] = 0xF8;
	}
	else {
		ERRORLOG( QString( "Unsupported event [%1]" )
					  .arg( MidiMessage::TypeToQString( msg.getType() ) ) );
		return;
	}
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;

	jackMidiOutEvent( buffer, 3 );
}

QString JackMidiDriver::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[JackMidiDriver]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2running: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_nRunning ) )
					  .append( QString( "%1%2rx_in_pos: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_midiRxInPosition ) )
					  .append( QString( "%1%2rx_out_pos: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_midiRxOutPosition ) );
	}
	else {
		sOutput =
			QString( "[JackMidiDriver]" )
				.append( QString( ", running: %1" ).arg( m_nRunning ) )
				.append( QString( ", rx_in_pos: %1" ).arg( m_midiRxInPosition ) )
				.append( QString( ", rx_out_pos: %1" ).arg( m_midiRxOutPosition ) );
	}

	return sOutput;
}

};	// namespace H2Core

#endif /* H2CORE_HAVE_JACK */
