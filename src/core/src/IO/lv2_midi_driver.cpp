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


/*
pthread_t PortMidiDriverThread;

void* PortMidiDriver_thread( void* param )
{
	Object *__object = (Object*)param;
	PortMidiDriver *instance = ( PortMidiDriver* )param;
	__INFOLOG( "PortMidiDriver_thread starting" );

	PmError status;
	int length;
	PmEvent buffer[1];
	while ( instance->m_bRunning ) {
		status = Pm_Poll( instance->m_pMidiIn );
		if ( status == TRUE ) {
			length = Pm_Read( instance->m_pMidiIn, buffer, 1 );
			if ( length > 0 ) {
				MidiMessage msg;

				int nEventType = Pm_MessageStatus( buffer[0].message );
				if ( ( nEventType >= 128 ) && ( nEventType < 144 ) ) {	// note off
					msg.m_nChannel = nEventType - 128;
					msg.m_type = MidiMessage::NOTE_OFF;
				} else if ( ( nEventType >= 144 ) && ( nEventType < 160 ) ) {	// note on
					msg.m_nChannel = nEventType - 144;
					msg.m_type = MidiMessage::NOTE_ON;
				} else if ( ( nEventType >= 160 ) && ( nEventType < 176 ) ) {	// Polyphonic Key Pressure (After-touch)
					msg.m_nChannel = nEventType - 160;
					msg.m_type = MidiMessage::POLYPHONIC_KEY_PRESSURE;
				} else if ( ( nEventType >= 176 ) && ( nEventType < 192 ) ) {	// Control Change
					msg.m_nChannel = nEventType - 176;
					msg.m_type = MidiMessage::CONTROL_CHANGE;
				} else if ( ( nEventType >= 192 ) && ( nEventType < 208 ) ) {	// Program Change
					msg.m_nChannel = nEventType - 192;
					msg.m_type = MidiMessage::PROGRAM_CHANGE;
				} else if ( ( nEventType >= 208 ) && ( nEventType < 224 ) ) {	// Channel Pressure (After-touch)
					msg.m_nChannel = nEventType - 208;
					msg.m_type = MidiMessage::CHANNEL_PRESSURE;
				} else if ( ( nEventType >= 224 ) && ( nEventType < 240 ) ) {	// Pitch Wheel Change
					msg.m_nChannel = nEventType - 224;
					msg.m_type = MidiMessage::PITCH_WHEEL;
				} else if ( ( nEventType >= 240 ) && ( nEventType < 256 ) ) {	// System Exclusive
					msg.m_nChannel = nEventType - 240;
					msg.m_type = MidiMessage::SYSTEM_EXCLUSIVE;
				} else {
					__ERRORLOG( "Unhandled midi message type: " + QString::number( nEventType ) );
					__INFOLOG( "MIDI msg: " );
					__INFOLOG( QString::number( buffer[0].timestamp ) );
					__INFOLOG( QString::number( Pm_MessageStatus( buffer[0].message ) ) );
					__INFOLOG( QString::number( Pm_MessageData1( buffer[0].message ) ) );
					__INFOLOG( QString::number( Pm_MessageData2( buffer[0].message ) ) );
				}

				msg.m_nData1 = Pm_MessageData1( buffer[0].message );
				msg.m_nData2 = Pm_MessageData2( buffer[0].message );

			}
		} else {
#ifdef WIN32
			Sleep( 1 );
#else
			usleep( 100 );
#endif
		}
	}



	__INFOLOG( "MIDI Thread DESTROY" );
	pthread_exit( nullptr );
	return nullptr;
} */

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
