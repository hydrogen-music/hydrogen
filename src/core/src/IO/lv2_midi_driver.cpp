/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 */


#include <hydrogen/IO/LV2MidiDriver.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>


#ifdef WIN32
#include <windows.h>
#endif


#include <pthread.h>

namespace H2Core
{
const char* LV2MidiDriver::__class_name = "Lv2MidiDriver";

LV2MidiDriver::LV2MidiDriver()
		: MidiInput( __class_name ), MidiOutput( __class_name ), Object( __class_name )
		, m_bRunning( false )
{
}

LV2MidiDriver::~LV2MidiDriver()
{
}

void LV2MidiDriver::open()
{
	INFOLOG( "[open]" );
}


void LV2MidiDriver::close()
{
	INFOLOG( "[close]" );
}


void LV2MidiDriver::forwardMidiMessage(const MidiMessage& pMsg)
{
	handleMidiMessage( pMsg );
}

std::vector<QString> LV2MidiDriver::getInputPortList()
{
	std::vector<QString> portList;

	return portList;
}

std::vector<QString> LV2MidiDriver::getOutputPortList()
{
	std::vector<QString> portList;
	
	return portList;
}

void LV2MidiDriver::handleQueueNote(Note* pNote)
{
}

void LV2MidiDriver::handleQueueNoteOff( int channel, int key, int velocity )
{

}

void LV2MidiDriver::handleQueueAllNoteOff()
{
}

void LV2MidiDriver::handleOutgoingControlChange( int param, int value, int channel )
{
}

};
