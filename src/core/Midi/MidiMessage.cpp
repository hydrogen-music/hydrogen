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

namespace H2Core
{

void MidiMessage::clear() {
	m_type = Type::Unknown;
	m_nData1 = -1;
	m_nData2 = -1;
	m_nChannel = -1;
	m_sysexData.clear();
}

int MidiMessage::deriveChannel( int nStatusByte ) {
	if ( nStatusByte >= 128 && nStatusByte < 144 ) {
		return nStatusByte - 128;
	}
	else if ( nStatusByte >= 144 && nStatusByte < 160 ) {
		return nStatusByte - 144;
	}
	else if ( nStatusByte >= 160 && nStatusByte < 176 ) {
		return nStatusByte - 160;
	}
	else if ( nStatusByte >= 176 && nStatusByte < 192 ) {
		return nStatusByte - 176;
	}
	else if ( nStatusByte >= 192 && nStatusByte < 208 ) {
		return nStatusByte - 192;
	}
	else if ( nStatusByte >= 208 && nStatusByte < 224 ) {
		return nStatusByte - 208;
	}
	else if ( nStatusByte >= 224 && nStatusByte < 240 ) {
		return nStatusByte - 224;
	}
	// System Common Messages
	else if ( nStatusByte == 240 ) {
		return nStatusByte - 224;
	}
	else {
		return -1;
	}
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
}

MidiMessage MidiMessage::from( std::shared_ptr<Note> pNote ) {
	MidiMessage msg;

	if ( pNote == nullptr || pNote->getInstrument() == nullptr ) {
		// In case we do not have a valid note, we do not construct a valid
		// message either.
	}
	if ( pNote->getInstrument()->getMidiOutChannel() == 0 ) {
		// MIDI output was turned off for the instrument associated to the
		// provided note.
	}
	else {
		msg.m_type = Type::NoteOn;
		msg.m_nData1 = std::clamp( pNote->getMidiKey(), 0, 127 );
		msg.m_nData2 = std::clamp( pNote->getMidiVelocity(), 0, 127 );
		msg.m_nChannel = std::clamp(
			pNote->getInstrument()->getMidiOutChannel(), 0, 15 );
	}

	return msg;
}

QString MidiMessage::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiMessage]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_type: %3\n" )
					 .arg( TypeToQString( m_type ) ) )
			.append( QString( "%1%2m_nData1: %3\n" )
					 .arg( m_nData1 ) )
			.append( QString( "%1%2m_nData2: %3\n" )
					 .arg( m_nData2 ) )
			.append( QString( "%1%2m_nChannel: %3\n" )
					 .arg( m_nChannel ) )
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
			.append( QString( "m_type: %1" ).arg( TypeToQString( m_type ) ) )
			.append( QString( ", m_nData1: %1" ).arg( m_nData1 ) )
			.append( QString( ", m_nData2: %1" ).arg( m_nData2 ) )
			.append( QString( ", m_nChannel: %1" ).arg( m_nChannel ) )
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

QString MidiMessage::EventToQString( const Event& event ) {
	QString sEvent;
	
	switch ( event ) {
	case Event::Note:
		sEvent = "NOTE";
		break;
	case Event::CC:
		sEvent = "CC";
		break;
	case Event::PC:
		sEvent = "PROGRAM_CHANGE";
		break;
	case Event::MmcStop:
		sEvent = "MMC_STOP";
		break;
	case Event::MmcPlay:
		sEvent = "MMC_PLAY";
		break;
	case Event::MmcPause:
		sEvent = "MMC_PAUSE";
		break;
	case Event::MmcDeferredPlay:
		sEvent = "MMC_DEFERRED_PLAY";
		break;
	case Event::MmcRewind:
		sEvent = "MMC_REWIND";
		break;
	case Event::MmcFastForward:
		sEvent = "MMC_FAST_FORWARD";
		break;
	case Event::MmcRecordStrobe:
		sEvent = "MMC_RECORD_STROBE";
		break;
	case Event::MmcRecordExit:
		sEvent = "MMC_RECORD_EXIT";
		break;
	case Event::MmcRecordReady:
		sEvent = "MMC_RECORD_READY";
		break;
	case Event::Null:
	default:
		sEvent = "";
	}

	return std::move( sEvent );
}

MidiMessage::Event MidiMessage::QStringToEvent( const QString& sEvent ) {
	if ( sEvent == "NOTE" ) {
		return Event::Note;
	}
	else if ( sEvent == "CC" ) {
		return Event::CC;
	}
	else if ( sEvent == "PROGRAM_CHANGE" ) {
		return Event::PC;
	}
	else if ( sEvent == "MMC_STOP" ) {
		return Event::MmcStop;
	}
	else if ( sEvent == "MMC_PLAY" ) {
		return Event::MmcPlay;
	}
	else if ( sEvent == "MMC_PAUSE" ) {
		return Event::MmcPause;
	}
	else if ( sEvent == "MMC_DEFERRED_PLAY" ) {
		return Event::MmcDeferredPlay;
	}
	else if ( sEvent == "MMC_FAST_FORWARD" ) {
		return Event::MmcFastForward;
	}
	else if ( sEvent == "MMC_REWIND" ) {
		return Event::MmcRewind;
	}
	else if ( sEvent == "MMC_RECORD_STROBE" ) {
		return Event::MmcRecordStrobe;
	}
	else if ( sEvent == "MMC_RECORD_EXIT" ) {
		return Event::MmcRecordExit;
	}
	else if ( sEvent == "MMC_RECORD_READY" ) {
		return Event::MmcRecordReady;
	} 
	else {
		return Event::Null;
	}
}

QStringList MidiMessage::getEventList() {
	QStringList eventList;
	eventList << EventToQString( Event::Null )
			  << EventToQString( Event::MmcPlay )
			  << EventToQString( Event::MmcDeferredPlay )
			  << EventToQString( Event::MmcStop )
			  << EventToQString( Event::MmcFastForward )
			  << EventToQString( Event::MmcRewind )
			  << EventToQString( Event::MmcRecordStrobe )
			  << EventToQString( Event::MmcRecordExit )
			  << EventToQString( Event::MmcRecordReady )
			  << EventToQString( Event::MmcPause )
			  << EventToQString( Event::Note )
			  << EventToQString( Event::CC )
			  << EventToQString( Event::PC );

	return std::move( eventList );
}
};

