/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: MidiDriver.h,v 1.8 2005/05/01 19:51:40 comix Exp $
 *
 */

#ifndef MIDI_DRIVER_H
#define MIDI_DRIVER_H

#include "../Object.h"
#include <string>
#include <vector>

class MidiMessage
{
	public:
		enum MidiMessageType {
			UNKNOWN,
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
			SONG_POS
		};

		MidiMessageType m_type;
		int m_nData1;
		int m_nData2;
		int m_nChannel;

		MidiMessage()
		 : m_type( UNKNOWN )
		 , m_nChannel( -1 )
		 , m_nData1( -1 )
		 , m_nData2( -1 )
		{
		}
};


class MidiPortInfo
{
	public:
		string m_sName;
		int m_nClient;
		int m_nPort;
};


class MidiDriver : public Object
{
	public:
		MidiDriver(std::string sDriverName);
		~MidiDriver();

		virtual void open() = 0;
		virtual void close() = 0;
		virtual std::vector<std::string> getOutputPortList() = 0;

		void setActive(bool isActive) {	m_bActive = isActive;	}
		void handleMidiMessage(const MidiMessage& msg);

	protected:
		bool m_bActive;

		void handleNoteOnMessage( const MidiMessage& msg );
		void handleNoteOffMessage( const MidiMessage& msg );



};


 #endif

