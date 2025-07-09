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
#include <core/Basics/Drumkit.h>
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
					sysExMsg.m_type = MidiMessage::Type::Sysex;
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
		, m_nVirtualInputDeviceId( -1 )
		, m_nVirtualOutputDeviceId( -1 )
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
			INFOLOG( QString( "%1%2%3%4device called [%5] using [%6] MIDI API" )
					 .arg( nDeviceId == i || nOutDeviceId == i ? "Using " :
						   "Found available " )
					 .arg( pInfo->is_virtual == TRUE ? "virtual " : "" )
					 .arg( pInfo->input == TRUE ? "input " : "" )
					 .arg( pInfo->output == TRUE ? "output " : "" )
					 .arg( pInfo->name ).arg( pInfo->interf ) );
		}
	}

	// In case the user did not select any input or output device to
	// connect to, we create virtual input or output devices. These
	// will allow external applications to connect to Hydrogen
	// themselves. In the code above only Hydrogen itself is able to
	// establish a connection but can not be discovered externally.
	//
	// This feature is not supported on Windows (by PortMidi).
#ifndef WIN32
	if ( nDeviceId == -1 ) {
#ifdef __APPLE__
		// macOS
		nDeviceId = Pm_CreateVirtualInput( "Hydrogen MIDI-in", "CoreMIDI", NULL );
#else
		// Linux
		nDeviceId = Pm_CreateVirtualInput( "Hydrogen MIDI-in", "ALSA", NULL );
#endif
		if ( nDeviceId < 0 ) {
			ERRORLOG( QString( "Unable to create virtual input: [%1]" )
					  .arg( PortMidiDriver::translatePmError(
								static_cast<PmError>(nDeviceId) ) ) );
		}
		else {
			m_nVirtualInputDeviceId = nDeviceId;
		}
	}
#endif

#ifndef WIN32
	if ( nOutDeviceId == -1 ) {
#ifdef __APPLE__
		// macOS
		nOutDeviceId = Pm_CreateVirtualOutput( "Hydrogen MIDI-out", "CoreMIDI", NULL );
#else
		// Linux
		nOutDeviceId = Pm_CreateVirtualOutput( "Hydrogen MIDI-out", "ALSA", NULL );
#endif
		if ( nOutDeviceId < 0 ) {
			ERRORLOG( QString( "Unable to create virtual output: [%1]" )
					  .arg( PortMidiDriver::translatePmError(
								static_cast<PmError>(nOutDeviceId) ) ) );
		}
		else {
			m_nVirtualOutputDeviceId = nOutDeviceId;
		}
	}
#endif

	// Open input device if found
	if ( nDeviceId >= 0 ) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo( nDeviceId );
		if ( info == nullptr ) {
			ERRORLOG( "Error opening midi input device" );
		}

		// Timer started with 1ms accuracy without any callback
		PtError startErr = Pt_Start( 1, 0, 0 );
		if ( startErr != ptNoError ) {
			QString sError;
			switch( startErr ) {
			case ptNoError:
				sError = QString();
				break;
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
	if ( nOutDeviceId >= 0 ) {
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

		PmError err;
		if ( m_pMidiIn != nullptr ) {
			err = Pm_Close( m_pMidiIn );
			if ( err != pmNoError ) {
				ERRORLOG( QString( "Unable to close PortMidi input device: [%1]" )
						  .arg( PortMidiDriver::translatePmError( err ) ) );
			}
		}
		if ( m_pMidiOut != nullptr ) {
			err = Pm_Close( m_pMidiOut );
			if ( err != pmNoError ) {
				ERRORLOG( QString( "Unable to close PortMidi output device: [%1]" )
						  .arg( PortMidiDriver::translatePmError( err ) ) );
			}
		}

		// In case virtual devices were created, we have to take care
		// of deleting them ourselves
		if ( m_nVirtualInputDeviceId != -1 ) {
			err = Pm_DeleteVirtualDevice( m_nVirtualInputDeviceId );
			if ( err != pmNoError ) {
				ERRORLOG( QString( "Unable to delete virtual input device: [%1]" )
						  .arg( PortMidiDriver::translatePmError( err ) ) );
			}
			m_nVirtualInputDeviceId = -1;
		}
		if ( m_nVirtualOutputDeviceId != -1 ) {
			err = Pm_DeleteVirtualDevice( m_nVirtualOutputDeviceId );
			if ( err != pmNoError ) {
				ERRORLOG( QString( "Unable to delete virtual output device: [%1]" )
						  .arg( PortMidiDriver::translatePmError( err ) ) );
			}
			m_nVirtualOutputDeviceId = -1;
		}
	}
}

std::vector<QString> PortMidiDriver::getInputPortList()
{
	std::vector<QString> portList;

	const int nDevices = Pm_CountDevices();
	for ( int ii = 0; ii < nDevices; ii++ ) {
		if ( ii != m_nVirtualInputDeviceId && ii != m_nVirtualOutputDeviceId ) {
			// Be sure to avoid a potential virtual device created by
			// Hydrogen itself (it can not possibly be connected to
			// since those virtual devices are deleted when restarting
			// the PortMidiDriver - which is done to establish a
			// connection). Also, there is no real use case and an
			// extreme risk to lock Hydrogen due to MIDI signal
			// feedback loops.
			const PmDeviceInfo *pInfo = Pm_GetDeviceInfo( ii );
			if ( pInfo == nullptr ) {
				ERRORLOG( QString( "Could not open output device [%1]" ).arg( ii ) );
			}
			else if ( pInfo->output == TRUE ) {
				INFOLOG( pInfo->name );
				portList.push_back( pInfo->name );
			}
		}
	}

	return portList;
}

std::vector<QString> PortMidiDriver::getOutputPortList()
{
	std::vector<QString> portList;

	const int nDevices = Pm_CountDevices();
	for ( int ii = 0; ii < nDevices; ii++ ) {
		if ( ii != m_nVirtualInputDeviceId && ii != m_nVirtualOutputDeviceId ) {
			// Be sure to avoid a potential virtual device created by
			// Hydrogen itself (it can not possibly be connected to
			// since those virtual devices are deleted when restarting
			// the PortMidiDriver - which is done to establish a
			// connection). Also, there is no real use case and an
			// extreme risk to lock Hydrogen due to MIDI signal
			// feedback loops.
			const PmDeviceInfo *pInfo = Pm_GetDeviceInfo( ii );
			if ( pInfo == nullptr ) {
				ERRORLOG( QString( "Could not open input device [%1]" ).arg( ii ) );
			}
			else if ( pInfo->input == TRUE ) {
				INFOLOG( pInfo->name );
				portList.push_back( pInfo->name );
			}
		}
	}

	return portList;
}

void PortMidiDriver::handleQueueNote( std::shared_ptr<Note> pNote)
{
	if ( m_pMidiOut == nullptr ) {
		return;
	}
	if ( pNote == nullptr || pNote->getInstrument() == nullptr ) {
		ERRORLOG( "Invalid note" );
		return;
	}

	int channel = pNote->getInstrument()->getMidiOutChannel();
	if ( channel < 0 ) {
		return;
	}

	int key = pNote->getMidiKey();
	int velocity = pNote->getMidiVelocity();

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

	auto instList = Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments();

	unsigned int numInstruments = instList->size();
	for (int index = 0; index < numInstruments; ++index) {
		auto pCurInst = instList->get(index);

		int channel = pCurInst->getMidiOutChannel();
		if (channel < 0) {
			continue;
		}
		int key = pCurInst->getMidiOutNote();

		PmEvent event;
		event.timestamp = 0;

		//Note off
		event.message = Pm_Message(0x80 | channel, key, 0);
		PmError err = Pm_Write(m_pMidiOut, &event, 1);
		if ( err != pmNoError ) {
			ERRORLOG( QString( "Error for instrument [%1] in Pm_Write: [%2]" )
					  .arg( pCurInst->getName() )
					  .arg( PortMidiDriver::translatePmError( err ) ) );
		}
	}
}

bool PortMidiDriver::appendSysExData( MidiMessage* pMidiMessage, const PmMessage& msg ) {
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

QString PortMidiDriver::translatePmError( const PmError& err ) {
	QString sRes( Pm_GetErrorText( err ) );
	if ( err == pmHostError ) {
		// Get OS-dependent part of the error messages, e.g. something
		// went wrong in the underlying ALSA driver.
		char *msg;
		Pm_GetHostErrorText( msg, 100 );
		sRes.append( QString( ": [%1]" ).arg( msg ) );
	}

	return std::move( sRes );
}

QString PortMidiDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[PortMidiDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bActive: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bActive ) )
			.append( QString( "%1%2m_bRunning: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bRunning ) )
			.append( QString( "%1%2m_nVirtualInputDeviceId: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nVirtualInputDeviceId ) )
			.append( QString( "%1%2m_nVirtualOutputDeviceId: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nVirtualOutputDeviceId ) );
	} else {
		sOutput = QString( "[PortMidiDriver]" )
			.append( QString( " m_bActive: %1" ).arg( m_bActive ) )
			.append( QString( ", m_bRunning: %1" ).arg( m_bRunning ) )
			.append( QString( ", m_nVirtualInputDeviceId: %1" ).arg( m_nVirtualInputDeviceId ) )
			.append( QString( ", m_nVirtualOutputDeviceId: %1" ).arg( m_nVirtualOutputDeviceId ) );
	}

	return sOutput;
}
};

#endif	// H2CORE_HAVE_PORTMIDI
