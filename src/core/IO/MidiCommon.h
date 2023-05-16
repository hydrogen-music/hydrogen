/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef H2_MIDI_COMMON_H
#define H2_MIDI_COMMON_H

#include <core/config.h>
#include <core/Object.h>
#include <string>
#include <vector>

#include <QString>

namespace H2Core
{

/** \ingroup docCore docMIDI */
class MidiMessage
{
public:
	enum MidiMessageType {
		UNKNOWN,
		SYSEX,
		NOTE_ON,
		NOTE_OFF,
		POLYPHONIC_KEY_PRESSURE,
		CONTROL_CHANGE,
		PROGRAM_CHANGE,
		CHANNEL_PRESSURE,
		PITCH_WHEEL,
		SYSTEM_EXCLUSIVE,
		START,
		CONTINUE,
		STOP,
		SONG_POS,
		QUARTER_FRAME
	};
	static QString TypeToQString( MidiMessageType type );

	MidiMessageType m_type;
	int m_nData1;
	int m_nData2;
	int m_nChannel;
	std::vector<unsigned char> m_sysexData;

	MidiMessage()
			: m_type( UNKNOWN )
			, m_nData1( -1 )
			, m_nData2( -1 )
			, m_nChannel( -1 ) {}
};

inline QString MidiMessage::TypeToQString( MidiMessageType type ) {
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
	case MidiMessageType::SYSTEM_EXCLUSIVE:
		sType = "SYSTEM_EXCLUSIVE";
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


/** \ingroup docCore docMIDI */
class MidiPortInfo
{
public:
	QString m_sName;
	int m_nClient;
	int m_nPort;
};


};

#endif

