/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

namespace H2Core
{

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


class MidiPortInfo
{
public:
	QString m_sName;
	int m_nClient;
	int m_nPort;
};


};

#endif

