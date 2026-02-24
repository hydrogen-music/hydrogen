/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

	// SysEx messages in PortMidi spread across multiple PmEvents and
	// it is our responsibility to put them together.
	MidiMessage sysExMsg;
	while ( instance->m_bRunning && instance->m_pMidiIn != nullptr ) {
		length = Pm_Read( instance->m_pMidiIn, buffer, 1 );
		if ( length > 0 ) {

			int nEventType = Pm_MessageStatus( buffer[0].message );

			if ( nEventType > 127 && nEventType != 247 && nEventType < 256 ) {
				// New MIDI message received.
				//
				// In case of a SysEx message spanning multiple
				// PmEvents only the first one will have SysEx status
				// byte. In all remaining events it is omit and the
				// first byte is an actual data byte [0,127]. The
				// termination of such an SysEx message is indicated
				// using 247 which by itself must not be interpreted
				// as the beginning of a new message.
				//
				// 'System Realtime' messages are allowed to occur in
				// between events corresponding to one and the same
				// SysEx message but all other event types indicated
				// that either the previous SysEx message was
				// completed or that it was truncated (e.g. MIDI cable
				// removed).
				if ( nEventType < 248 ) {
					// No System Realtime event
					sysExMsg.clear();
				}

				if ( nEventType == 240 ) {
					// New SysEx message
					sysExMsg.m_type = MidiMessage::SYSEX;
					if ( PortMidiDriver::appendSysExData( &sysExMsg,
														  buffer[0].message ) ) {
						instance->handleMidiMessage( sysExMsg );
					}
				}
				else {
					// Other MIDI message consisting only of a single PmEvent.
					MidiMessage msg;
					msg.setType( nEventType );
					msg.m_nData1 = Pm_MessageData1( buffer[0].message );
					msg.m_nData2 = Pm_MessageData2( buffer[0].message );
					instance->handleMidiMessage( msg );
				}
			}
			else if ( nEventType >= 256 ) {
				__ERRORLOG( QString( "Unsupported midi message type: [%1]" )
							.arg( nEventType ) );
			}
			else {
				// Continuation of a SysEx message.
				if ( PortMidiDriver::appendSysExData( &sysExMsg,
													  buffer[0].message ) ) {
					instance->handleMidiMessage( sysExMsg );
				}
			}
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
						.arg( PortMidiDriver::translatePmError( static_cast<PmError>(length) ) ) );
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
				  .arg( PortMidiDriver::translatePmError( err ) ) );
	}
}


PortMidiDriver::~PortMidiDriver()
{
	PmError err = Pm_Terminate();
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Terminate: [%1]" )
				  .arg( PortMidiDriver::translatePmError( err ) ) );
	}
}

void PortMidiDriver::handleOutgoingControlChange( int param, int value, int channel )
{
	if ( m_pMidiOut == nullptr ) {
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
				const auto sMidiPortNameLocal8Bit = sMidiPortName.toLocal8Bit();
				if ( strcmp( pInfo->name, sMidiPortNameLocal8Bit.constData() ) == 0 &&
					 sMidiPortName != Preferences::getNullMidiPort() ) {
					nDeviceId = i;
				}
			}
	
			if ( pInfo->output == TRUE ) {
				const auto sMidiOutputPortNameLocal8Bit = sMidiOutputPortName.toLocal8Bit();
				if ( strcmp( pInfo->name, sMidiOutputPortNameLocal8Bit.constData() ) == 0 &&
					 sMidiOutputPortName != Preferences::getNullMidiPort() ) {
					nOutDeviceId = i;
				}
			}
			INFOLOG( QString( "%1%2%3device called [%4] using [%5] MIDI API" )
					 .arg( nDeviceId == i || nOutDeviceId == i ? "Using " :
						   "Found available " )
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
					  .arg( PortMidiDriver::translatePmError( err ) ) );
			m_pMidiIn = nullptr;
		}
	}
	else {
		// If no input device was selected, there is no error in here.
		if ( sMidiPortName != Preferences::getNullMidiPort() ) {
			WARNINGLOG( QString( "MIDI input device [%1] not found." )
					  .arg( sMidiPortName ) );
		} else {
			INFOLOG( QString( "No MIDI input device selected" ) );
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
					  .arg( PortMidiDriver::translatePmError( err ) ) );
			m_pMidiOut = nullptr;
		}
	}
	else {
		// If no output device was selected, there is no error in here.
		if ( sMidiOutputPortName != Preferences::getNullMidiPort() ) {
			WARNINGLOG( QString( "MIDI output device [%1] not found." )
						.arg( sMidiOutputPortName ) );
		} else {
			INFOLOG( QString( "No MIDI output device selected" ) );
		}
		m_pMidiOut = nullptr;
	}

	if ( m_pMidiIn != nullptr ) {
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
					  .arg( PortMidiDriver::translatePmError( err ) ) );
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
				  .arg( PortMidiDriver::translatePmError( err ) ) );
	}

	//Note on
	event.message = Pm_Message(0x90 | channel, key, velocity);
	err = Pm_Write(m_pMidiOut, &event, 1);
	if ( err != pmNoError ) {
		ERRORLOG( QString( "Error in Pm_Write for Note on: [%1]" )
				  .arg( PortMidiDriver::translatePmError( err ) ) );
	}
}

void PortMidiDriver::handleQueueNoteOff( int channel, int key, int velocity )
{
	if ( m_pMidiOut == nullptr ) {
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
				  .arg( PortMidiDriver::translatePmError( err ) ) );
	}
}

void PortMidiDriver::handleQueueAllNoteOff()
{
	if ( m_pMidiOut == nullptr ) {
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
					  .arg( PortMidiDriver::translatePmError( err ) ) );
		}
	}
}

bool PortMidiDriver::appendSysExData( MidiMessage* pMidiMessage, PmMessage msg ) {
	// End of exception byte indicating the end of a SysEx message.
	unsigned char eox = 247;
	unsigned char c = msg & 0x000000ffUL;
	pMidiMessage->m_sysexData.push_back( c );
	if ( c == eox ) {
		return true;
	}

    c = (msg & 0x0000ff00UL) >>  8;
	pMidiMessage->m_sysexData.push_back( c );
	if ( c == eox ) {
		return true;
	}

	c = (msg & 0x00ff0000UL) >> 16;
	pMidiMessage->m_sysexData.push_back( c );
	if ( c == eox ) {
		return true;
	}

	c = (msg & 0xff000000UL) >> 24;
	pMidiMessage->m_sysexData.push_back( c );
	if ( c == eox ) {
		return true;
	}

	return false;
}

QString PortMidiDriver::translatePmError( PmError err ) {
	QString sRes( Pm_GetErrorText( err ) );
	if ( err == pmHostError ) {
		// Get OS-dependent part of the error messages, e.g. something
		// went wrong in the underlying ALSA driver.
		char msg[100];
		Pm_GetHostErrorText( msg, 100 );
		sRes.append( QString( ": [%1]" ).arg( msg ) );
	}

	return std::move( sRes );
}
};

#endif	// H2CORE_HAVE_PORTMIDI
