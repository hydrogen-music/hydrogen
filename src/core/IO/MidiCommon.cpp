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

#include <core/IO/MidiCommon.h>

namespace H2Core
{

void MidiMessage::clear() {
	m_type = UNKNOWN;
	m_nData1 = -1;
	m_nData2 = -1;
	m_nChannel = -1;
	m_sysexData.clear();
}

QString MidiMessage::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiMessage]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_type: %3\n" )
					 .arg( MidiMessage::TypeToQString( m_type ) ) )
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
		sOutput.append( "]\n" );
	}
	else {
		sOutput = QString( "[MidiMessage] " )
			.append( QString( "m_type: %1" ).arg( MidiMessage::TypeToQString( m_type ) ) )
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
		sOutput.append( "]\n" );
	}
	
	return sOutput;
}

QString MidiMessage::TypeToQString( MidiMessageType type ) {
	QString sType;
	switch( type ) {
	case MidiMessageType::SYSEX:
		sType = "SYSEX";
		break;
	case MidiMessageType::NOTE_ON:
		sType = "NOTE_ON";
		break;
	case MidiMessageType::NOTE_OFF:
		sType = "NOTE_OFF";
		break;
	case MidiMessageType::POLYPHONIC_KEY_PRESSURE:
		sType = "POLYPHONIC_KEY_PRESSURE";
		break;
	case MidiMessageType::CONTROL_CHANGE:
		sType = "CONTROL_CHANGE";
		break;
	case MidiMessageType::PROGRAM_CHANGE:
		sType = "PROGRAM_CHANGE";
		break;
	case MidiMessageType::CHANNEL_PRESSURE:
		sType = "CHANNEL_PRESSURE";
		break;
	case MidiMessageType::PITCH_WHEEL:
		sType = "PITCH_WHEEL";
		break;
	case MidiMessageType::START:
		sType = "START";
		break;
	case MidiMessageType::CONTINUE:
		sType = "CONTINUE";
		break;
	case MidiMessageType::STOP:
		sType = "STOP";
		break;
	case MidiMessageType::SONG_POS:
		sType = "SONG_POS";
		break;
	case MidiMessageType::QUARTER_FRAME:
		sType = "QUARTER_FRAME";
		break;
	case MidiMessageType::UNKNOWN:
	default:
		sType = "Unknown MIDI message type";
	}

	return std::move( sType );
}

};

