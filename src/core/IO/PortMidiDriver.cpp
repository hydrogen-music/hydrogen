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


#include <core/IO/PortMidiDriver.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Note.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Hydrogen.h>
#include <core/Globals.h>


#ifdef WIN32
#include <windows.h>
#endif

#if defined(H2CORE_HAVE_PORTMIDI) || _DOXYGEN_

#include <porttime.h>
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)

#include <pthread.h>

namespace H2Core
{

pthread_t PortMidiDriverThread;

void* PortMidiDriver_thread( void* param )
{
	Base *__object = (Base *)param;
	PortMidiDriver *instance = ( PortMidiDriver* )param;
	__INFOLOG( "PortMidiDriver_thread starting" );

	PmError status;
	int length;
	PmEvent buffer[1];
	while ( instance->m_bRunning ) {
		length = Pm_Read( instance->m_pMidiIn, buffer, 1 );
		if ( length > 0 ) {
			// New MIDI data available
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
				msg.m_type = MidiMessage::SYSEX;
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

			instance->handleMidiMessage( msg );
		}
		else if ( length == 0 ) {
			// No data available
#ifdef WIN32
			Sleep( 1 );
#else
			usleep( 100 );
#endif
		}
		else {
			// An error occurred, e.g. a buffer overflow.
			__ERRORLOG( QString( "Error in Pm_Read: [%1]" )
						.arg( Pm_GetErrorText( static_cast<PmError>(length) ) ) );
		}
	}



	__INFOLOG( "MIDI Thread DESTROY" );
	pthread_exit( nullptr );
	return nullptr;
}

PortMidiDriver::PortMidiDriver()
		: MidiInput(), MidiOutput(), Object<PortMidiDriver>()
		, m_bRunning( false )
		, m_pMidiIn( nullptr )
		, m_pMidiOut( nullptr )
{
	PmError err = Pm_Initialize();
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Initialize: [%1]" )
				  .arg( Pm_GetErrorText( err ) ) );
	}
}


PortMidiDriver::~PortMidiDriver()
{
	PmError err = Pm_Terminate();
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Terminate: [%1]" )
				  .arg( Pm_GetErrorText( err ) ) );
	}
}

void PortMidiDriver::handleOutgoingControlChange( int param, int value, int channel )
{
	if ( m_pMidiOut == nullptr ) {
		ERRORLOG( "m_pMidiOut = nullptr " );
		return;
	}

	if (channel < 0) {
		return;
	}

	PmEvent event;
	event.timestamp = 0;

	//Control change
	event.message = Pm_Message(0xB0 | channel, param, value);
	Pm_Write(m_pMidiOut, &event, 1);
}



void PortMidiDriver::open()
{
	INFOLOG( "[open]" );

	int nInputBufferSize = 100;

	int nDeviceId = -1;
	int nOutDeviceId = -1;
	QString sMidiPortName = Preferences::get_instance()->m_sMidiPortName;
	QString sMidiOutputPortName = Preferences::get_instance()->m_sMidiOutputPortName;
	int nDevices = Pm_CountDevices();

	// Find named devices
	for ( int i = 0; i < nDevices; i++ ) {
		const PmDeviceInfo *pInfo = Pm_GetDeviceInfo( i );
		
		if ( pInfo == nullptr ) {
			ERRORLOG( QString( "Could not open input device [%1]" ).arg( i ) );
		}
		else {
			if ( pInfo->input == TRUE ) {
				if ( strcmp( pInfo->name, sMidiPortName.toLocal8Bit().constData() ) == 0 &&
					 sMidiPortName != Preferences::getNullMidiPort() ) {
					nDeviceId = i;
				}
			}
	
			if ( pInfo->output == TRUE ) {
				if ( strcmp( pInfo->name, sMidiOutputPortName.toLocal8Bit().constData() ) == 0 &&
					 sMidiOutputPortName != Preferences::getNullMidiPort() ) {
					nOutDeviceId = i;
				}
			}
			INFOLOG( QString( "%1%2%3%4device called [%5] using [%6] MIDI API" )
					 .arg( nDeviceId == i || nOutDeviceId == i ? "Using " :
						   "Found available " )
					 .arg( pInfo->is_virtual == TRUE ? "virtual " : "" )
					 .arg( pInfo->input == TRUE ? "input " : "" )
					 .arg( pInfo->output == TRUE ? "output " : "" )
					 .arg( pInfo->name ).arg( pInfo->interf ) );
		}
	}

	// Open input device if found
	if ( nDeviceId != -1 ) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo( nDeviceId );
		if ( info == nullptr ) {
			ERRORLOG( "Error opening midi input device" );
		}

		// Timer started with 1ms accuracy without any callback
		PtError startErr = Pt_Start( 1, 0, 0 );
		if ( startErr != ptNoError ) {
			QString sError;
			switch( startErr ) {
			case ptHostError:
				sError = QString( "Host error" );
				break;
			case ptAlreadyStarted:
				sError = QString( "Cannot start timer because it is already started" );
				break;
			case ptAlreadyStopped:
				sError = QString( "Cannot stop timer because it is already stopped" );
				break;
			case ptInsufficientMemory:
				sError = QString( "Memory could not be allocated" );
				break;
			}
			ERRORLOG( QString( "Error in Pt_Start: [%1]" ).arg( sError ) );
		}

		PmError err = Pm_OpenInput(
								   &m_pMidiIn,
								   nDeviceId,
								   nullptr,
								   nInputBufferSize,
								   TIME_PROC,
								   nullptr
								   );

		if ( err != pmNoError ) {
			ERRORLOG( QString( "Error in Pm_OpenInput: [%1]" )
					  .arg( Pm_GetErrorText( err ) ) );
			m_pMidiIn = nullptr;
		}
	}
	else {
		// If no input device was selected, there is no error in here.
		if ( sMidiPortName != Preferences::getNullMidiPort() ) {
			WARNINGLOG( QString( "MIDI input device [%1] not found." )
					  .arg( sMidiPortName ) );
		}
		m_pMidiIn = nullptr;
	}

	// Open output device if found
	if ( nOutDeviceId != -1 ) {
		PmError err = Pm_OpenOutput(
									&m_pMidiOut,
									nOutDeviceId,
									nullptr,
									nInputBufferSize,
									TIME_PROC,
									nullptr,
									0
									);

		if ( err != pmNoError ) {
			ERRORLOG( QString( "Error in Pm_OpenOutput: [%1]" )
					  .arg( Pm_GetErrorText( err ) ) );
			m_pMidiOut = nullptr;
		}
	}
	else {
		// If no output device was selected, there is no error in here.
		if ( sMidiOutputPortName != Preferences::getNullMidiPort() ) {
			WARNINGLOG( QString( "MIDI output device [%1] not found." )
						.arg( sMidiOutputPortName ) );
		}
		m_pMidiOut = nullptr;
	}

	if ( m_pMidiOut != nullptr || m_pMidiIn != nullptr ) {
		m_bRunning = true;

		pthread_attr_t attr;
		pthread_attr_init( &attr );
		pthread_create( &PortMidiDriverThread, &attr, PortMidiDriver_thread, ( void* )this );
	}
}


void PortMidiDriver::close()
{
	INFOLOG( "[close]" );
	if ( m_bRunning ) {
		m_bRunning = false;
		pthread_join( PortMidiDriverThread, nullptr );
		PmError err = Pm_Close( m_pMidiIn );
		if ( err != pmNoError ) {
			ERRORLOG( QString( "Error in Pm_Close: [%1]" )
					  .arg( Pm_GetErrorText( err ) ) );
		}
	}
}

std::vector<QString> PortMidiDriver::getInputPortList()
{
	std::vector<QString> portList;

	int nDevices = Pm_CountDevices();
	for ( int i = 0; i < nDevices; i++ ) {
		const PmDeviceInfo *pInfo = Pm_GetDeviceInfo( i );
		if ( pInfo == nullptr ) {
			ERRORLOG( QString( "Could not open output device [%1]" ).arg( i ) );
		} else if ( pInfo->output == TRUE ) {
			INFOLOG( pInfo->name );
			portList.push_back( pInfo->name );
		}
	}

	return portList;
}

std::vector<QString> PortMidiDriver::getOutputPortList()
{
	std::vector<QString> portList;

	int nDevices = Pm_CountDevices();
	for ( int i = 0; i < nDevices; i++ ) {
		const PmDeviceInfo *pInfo = Pm_GetDeviceInfo( i );
		if ( pInfo == nullptr ) {
			ERRORLOG( QString( "Could not open input device [%1]" ).arg( i ) );
		} else if ( pInfo->input == TRUE ) {
			INFOLOG( pInfo->name );
			portList.push_back( pInfo->name );
		}
	}

	return portList;
}

void PortMidiDriver::handleQueueNote(Note* pNote)
{
	if ( m_pMidiOut == nullptr ) {
		ERRORLOG( "m_pMidiOut = nullptr " );
		return;
	}

	int channel = pNote->get_instrument()->get_midi_out_channel();
	if ( channel < 0 ) {
		return;
	}

	int key = pNote->get_midi_key();
	int velocity = pNote->get_midi_velocity();

	PmEvent event;
	event.timestamp = 0;

	//Note off
	event.message = Pm_Message(0x80 | channel, key, velocity);
	PmError err = Pm_Write(m_pMidiOut, &event, 1);
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Write for Note off: [%1]" )
				  .arg( Pm_GetErrorText( err ) ) );
	}

	//Note on
	event.message = Pm_Message(0x90 | channel, key, velocity);
	err = Pm_Write(m_pMidiOut, &event, 1);
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Write for Note on: [%1]" )
				  .arg( Pm_GetErrorText( err ) ) );
	}
}

void PortMidiDriver::handleQueueNoteOff( int channel, int key, int velocity )
{
	if ( m_pMidiOut == nullptr ) {
		ERRORLOG( "m_pMidiOut = nullptr " );
		return;
	}

	if ( channel < 0 ) {
		return;
	}

	PmEvent event;
	event.timestamp = 0;

	//Note off
	event.message = Pm_Message(0x80 | channel, key, velocity);
	PmError err = Pm_Write(m_pMidiOut, &event, 1);
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Write: [%1]" )
				  .arg( Pm_GetErrorText( err ) ) );
	}
}

void PortMidiDriver::handleQueueAllNoteOff()
{
	if ( m_pMidiOut == nullptr ) {
		ERRORLOG( "m_pMidiOut = nullptr " );
		return;
	}

	auto instList = Hydrogen::get_instance()->getSong()->getInstrumentList();

	unsigned int numInstruments = instList->size();
	for (int index = 0; index < numInstruments; ++index) {
		auto pCurInst = instList->get(index);

		int channel = pCurInst->get_midi_out_channel();
		if (channel < 0) {
			continue;
		}
		int key = pCurInst->get_midi_out_note();

		PmEvent event;
		event.timestamp = 0;

		//Note off
		event.message = Pm_Message(0x80 | channel, key, 0);
		PmError err = Pm_Write(m_pMidiOut, &event, 1);
		if ( err != pmNoError ) {
			ERRORLOG( QString( "Error for instrument [%1] in Pm_Write: [%2]" )
					  .arg( pCurInst->get_name() )
					  .arg( Pm_GetErrorText( err ) ) );
		}
	}
}
};

#endif	// H2CORE_HAVE_PORTMIDI
