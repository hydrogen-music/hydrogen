/*
 * Hydrogen
 * Copyright(c) 2023-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "MidiMessage.h"

#include <core/Basics/Note.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include "Midi/Midi.h"

namespace H2Core
{

MidiMessage::MidiMessage()
	: m_timePoint( Clock::now() ),
	  m_type( Type::Unknown ),
	  m_data1( Midi::ParameterInvalid ),
	  m_data2( Midi::ParameterInvalid ),
	  m_channel( Midi::ChannelInvalid ),
	  m_nFrameOffset( 0 )
{
}

MidiMessage::MidiMessage(
	Type type,
	Midi::Parameter data1,
	Midi::Parameter data2,
	Midi::Channel channel
)
	: m_timePoint( Clock::now() ),
	  m_type( type ),
	  m_data1( data1 ),
	  m_data2( data2 ),
	  m_channel( channel ),
	  m_nFrameOffset( 0 )
{
}

void MidiMessage::clear()
{
	m_timePoint = Clock::now();
	m_type = Type::Unknown;
	m_data1 = Midi::ParameterInvalid;
	m_data2 = Midi::ParameterInvalid;
	m_channel = Midi::ChannelInvalid;
	m_sysexData.clear();
	m_nFrameOffset = 0;
}

Midi::Channel MidiMessage::deriveChannel( int nStatusByte )
{
    int nValue;
	if ( nStatusByte >= 128 && nStatusByte < 144 ) {
		nValue =  nStatusByte - 128;
	}
	else if ( nStatusByte >= 144 && nStatusByte < 160 ) {
		nValue =  nStatusByte - 144;
	}
	else if ( nStatusByte >= 160 && nStatusByte < 176 ) {
		nValue =  nStatusByte - 160;
	}
	else if ( nStatusByte >= 176 && nStatusByte < 192 ) {
		nValue =  nStatusByte - 176;
	}
	else if ( nStatusByte >= 192 && nStatusByte < 208 ) {
		nValue =  nStatusByte - 192;
	}
	else if ( nStatusByte >= 208 && nStatusByte < 224 ) {
		nValue =  nStatusByte - 208;
	}
	else if ( nStatusByte >= 224 && nStatusByte < 240 ) {
		nValue =  nStatusByte - 224;
	}
	// System Common Messages
	else if ( nStatusByte == 240 ) {
		nValue =  nStatusByte - 224;
	}
	else {
		return Midi::ChannelInvalid;
	}

	// Midi::Channel within Hydrogen represent user-facing values. Since the
	// numerical value of channel `1` is `0` within the MIDI standard, we have
	// to convert it.
    return Midi::channelFromIntClamp( nValue + 1 );
}

MidiMessage::Type MidiMessage::deriveType( int nStatusByte ) {
	if ( nStatusByte >= 128 && nStatusByte < 144 ) {
		return Type::NoteOff;
	}
	else if ( nStatusByte >= 144 && nStatusByte < 160 ) {
		return Type::NoteOn;
	}
	else if ( nStatusByte >= 160 && nStatusByte < 176 ) {
		return Type::PolyphonicKeyPressure;
	}
	else if ( nStatusByte >= 176 && nStatusByte < 192 ) {
		return Type::ControlChange;
	}
	else if ( nStatusByte >= 192 && nStatusByte < 208 ) {
		return Type::ProgramChange;
	}
	else if ( nStatusByte >= 208 && nStatusByte < 224 ) {
		return Type::ChannelPressure;
	}
	else if ( nStatusByte >= 224 && nStatusByte < 240 ) {
		return Type::PitchWheel;
	}
	// System Common Messages
	else if ( nStatusByte == 240 ) {
		return Type::Sysex;
	}
	else if ( nStatusByte == 241 ) {
		return Type::QuarterFrame;
	}
	else if ( nStatusByte == 242 ) {
		return Type::SongPos;
	}
	else if ( nStatusByte == 243 ) {
		return Type::SongSelect;
	}
	// 244, 245 are undefined/reserved
	else if ( nStatusByte == 246 ) {
		return Type::TuneRequest;
	}
	// 247 indicates the end of a SysEx (240) message
	// System Realtime Messages
	else if ( nStatusByte == 248 ) {
		return Type::TimingClock;
	}
	// 249 is undefined/reserved
	else if ( nStatusByte == 250 ) {
		return Type::Start;
	}
	else if ( nStatusByte == 251 ) {
		return Type::Continue;
	}
	else if ( nStatusByte == 252 ) {
		return Type::Stop;
	}
	// 253 is undefined/reserved
	else if ( nStatusByte == 254 ) {
		return Type::ActiveSensing;
	}
	else if ( nStatusByte == 255 ) {
		return Type::Reset;
	}
	else {
		return Type::Unknown;
	}
}

MidiMessage MidiMessage::from( const MidiMessage& otherMsg ) {
	MidiMessage msg;
	msg.m_type = otherMsg.m_type;
	msg.m_data1 = otherMsg.m_data1;
	msg.m_data2 = otherMsg.m_data2;
	msg.m_channel = otherMsg.m_channel;
	msg.m_nFrameOffset = otherMsg.m_nFrameOffset;

	if ( otherMsg.m_sysexData.size() > 0 ) {
		msg.m_sysexData.resize( otherMsg.m_sysexData.size() );
		for ( int ii = 0; ii < otherMsg.m_sysexData.size(); ++ii ) {
			msg.m_sysexData[ ii ] = otherMsg.m_sysexData[ ii ];
		}
	}

	return msg;
}

MidiMessage MidiMessage::from( const ControlChange& controlChange ) {
	MidiMessage msg;
	if ( controlChange.channel != Midi::ChannelOff &&
		 controlChange.channel != Midi::ChannelInvalid ) {
		// By providing a negative value the resulting message will be
		// suppressed.
		msg.setType( Type::ControlChange );
		msg.setData1( controlChange.parameter );
		msg.setData2( controlChange.value );
		msg.setChannel( controlChange.channel );
	}

	return msg;
}

MidiMessage MidiMessage::from( std::shared_ptr<Note> pNote ) {
	MidiMessage msg;

	// In case we do not have a valid note or MIDI output was turned off for the
	// instrument associated to the provided note, we do not assign a valid type
	// and the message will be dropped.
	if ( pNote != nullptr && pNote->getInstrument() != nullptr ) {
		const auto noteRef = Preferences::get_instance()
								 ->getMidiInstrumentMap()
								 ->getOutputMapping( pNote );
		if ( noteRef.channel == Midi::ChannelOff ||
			 noteRef.channel == Midi::ChannelInvalid ) {
			// Dropping message
			return msg;
		}

		msg.setType( Type::NoteOn );
		msg.setData1( static_cast<Midi::Parameter>( noteRef.note ) );
		msg.setData2( pNote->getMidiVelocity() );
		msg.setChannel( noteRef.channel );
	}

	return msg;
}

MidiMessage MidiMessage::from( const NoteOff& noteOff ) {
	MidiMessage msg;
	if ( noteOff.channel != Midi::ChannelOff &&
		 noteOff.channel != Midi::ChannelInvalid ) {
		// By providing a negative value the resulting message will be
		// suppressed.
		msg.setType( Type::NoteOff );
		msg.setData1( static_cast<Midi::Parameter>( noteOff.note ) );
		msg.setData2( noteOff.velocity );
		msg.setChannel( noteOff.channel );
	}

	return msg;
}

bool MidiMessage::operator==( const MidiMessage& other ) const {
	if ( m_type != other.m_type ||
		 m_data1 != other.m_data1 ||
		 m_data2 != other.m_data2 ||
		 m_channel != other.m_channel ||
		 m_sysexData.size() != other.m_sysexData.size() ) {
		return false;
	}

	for ( int ii = 0; ii < m_sysexData.size(); ++ii ) {
		if ( m_sysexData[ ii ] != other.m_sysexData[ ii ] ) {
			return false;
		}
	}

	return true;
}

bool MidiMessage::operator!=( const MidiMessage& other ) const {
	return ! MidiMessage::operator==( other );
}

QString MidiMessage::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiMessage]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_timePoint: %3\n" )
					 .arg( H2Core::timePointToQString( m_timePoint ) ) )
			.append( QString( "%1%2m_type: %3\n" )
					 .arg( TypeToQString( m_type ) ) )
			.append( QString( "%1%2m_data1: %3\n" )
					 .arg( static_cast<int>( m_data1 ) ) )
			.append( QString( "%1%2m_data2: %3\n" )
					 .arg( static_cast<int>( m_data2 ) ) )
			.append( QString( "%1%2m_nFrameOffset: %3\n" )
					 .arg( m_nFrameOffset ) )
			.append( QString( "%1%2m_channel: %3\n" )
					 .arg( static_cast<int>(m_channel) ) )
			.append( QString( "%1%2m_sysexData: [" ) );
		bool bIsFirst = true;
		for ( const auto& dd : m_sysexData ) {
			if ( bIsFirst ) {
				sOutput.append( QString( "%1" ).arg( dd ) );
				bIsFirst = false;
			}
			else {
				sOutput.append( QString( " %1" ).arg( dd ) );
			}
		}
		sOutput.append( "]" );
	}
	else {
		sOutput = QString( "[MidiMessage] " )
			.append( QString( "m_timePoint: %1" )
					 .arg( H2Core::timePointToQString( m_timePoint ) ) )
			.append( QString( ", m_type: %1" ).arg( TypeToQString( m_type ) ) )
			.append( QString( ", m_data1: %1" ).arg( static_cast<int>( m_data1 ) ) )
			.append( QString( ", m_data2: %1" ).arg( static_cast<int>( m_data2 ) ) )
			.append( QString( ", m_channel: %1" ).arg( static_cast<int>(m_channel) ) )
			.append( QString( ", m_nFrameOffset: %1" ).arg( static_cast<int>(m_nFrameOffset) ) )
			.append( QString( ", m_sysexData: [" ) );
		bool bIsFirst = true;
		for ( const auto& dd : m_sysexData ) {
			if ( bIsFirst ) {
				sOutput.append( QString( "%1" ).arg( dd ) );
				bIsFirst = false;
			}
			else {
				sOutput.append( QString( " %1" ).arg( dd ) );
			}
		}
		sOutput.append( "]" );
	}
	
	return sOutput;
}

QString MidiMessage::TypeToQString( Type type ) {
	QString sType;
	switch( type ) {
	case Type::Sysex:
		sType = "SYSEX";
		break;
	case Type::NoteOn:
		sType = "NOTE_ON";
		break;
	case Type::NoteOff:
		sType = "NOTE_OFF";
		break;
	case Type::PolyphonicKeyPressure:
		sType = "POLYPHONIC_KEY_PRESSURE";
		break;
	case Type::ControlChange:
		sType = "CONTROL_CHANGE";
		break;
	case Type::ProgramChange:
		sType = "PROGRAM_CHANGE";
		break;
	case Type::ChannelPressure:
		sType = "CHANNEL_PRESSURE";
		break;
	case Type::PitchWheel:
		sType = "PITCH_WHEEL";
		break;
	case Type::Start:
		sType = "START";
		break;
	case Type::Continue:
		sType = "CONTINUE";
		break;
	case Type::Stop:
		sType = "STOP";
		break;
	case Type::SongPos:
		sType = "SONG_POS";
		break;
	case Type::QuarterFrame:
		sType = "QUARTER_FRAME";
		break;
	case Type::SongSelect:
		sType = "SONG_SELECT";
		break;
	case Type::TuneRequest:
		sType = "TUNE_REQUEST";
		break;
	case Type::TimingClock:
		sType = "TIMING_CLOCK";
		break;
	case Type::ActiveSensing:
		sType = "ACTIVE_SENSING";
		break;
	case Type::Reset:
		sType = "RESET";
		break;
	case Type::Unknown:
	default:
		sType = "Unknown MIDI message type";
	}

	return std::move( sType );
}

};
