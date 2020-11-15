/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * CoreMidi driver for Hydrogen
 * Copyright(c) 2005-2006 by Jonathan Dempsey
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
 * Code cleanup (20060222 Jonathan Dempsey)
 * More cleaning (20060511 Jonathan Dempsey)
 * ... and a little bit more cleaning . . . (20060512 Jonathan Dempsey)
 * Added CFRelease code (20060514 Jonathan Dempsey)
 */

#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/IO/CoreMidiDriver.h>

#if defined(H2CORE_HAVE_COREMIDI) || _DOXYGEN_

namespace H2Core
{


static void midiProc ( const MIDIPacketList * pktlist,
					   void * readProcRefCon,
					   void * srcConnRefCon )
{
	UNUSED( srcConnRefCon );

	MIDIPacket* packet = ( MIDIPacket * )pktlist->packet;

		//_ERRORLOG( QString( "MIDIPROC packets # %1" ).arg( pktlist->numPackets ) );

	CoreMidiDriver *instance = ( CoreMidiDriver * )readProcRefCon;
	MidiMessage msg;
	for ( uint i = 0; i < pktlist->numPackets; i++ ) {
		int nEventType = packet->data[0];
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
					   msg.m_type = MidiMessage::SYSEX;
					   for(int i = 0; i< packet->length;i++){
							  msg.m_sysexData.push_back(packet->data[i]);
					   }
					   instance->handleMidiMessage( msg );
					   packet = MIDIPacketNext( packet );
					   return;
		} else {
			___ERRORLOG( QString( "Unhandled midi message type: %1" ).arg( nEventType ) );
			___INFOLOG( "MIDI msg: " );
			// instance->errorLog( "Unhandled midi message type: " + to_string( nEventType ) );
			// instance->infoLog( "MIDI msg: " );
		}

		msg.m_nData1 = packet->data[1];
		msg.m_nData2 = packet->data[2];
		instance->handleMidiMessage( msg );
		packet = MIDIPacketNext( packet );
	}
}


const char* CoreMidiDriver::__class_name = "CoreMidiDriver";

CoreMidiDriver::CoreMidiDriver()
		: MidiInput( __class_name ) ,MidiOutput( __class_name ), Object( __class_name )
		, m_bRunning( false )
{
	INFOLOG( "INIT" );
	OSStatus err = noErr;

	QString sMidiPortName = Preferences::get_instance()->m_sMidiPortName;
	err = MIDIClientCreate ( CFSTR( "h2MIDIClient" ), NULL, NULL, &h2MIDIClient );
	if ( err != noErr ) {
		ERRORLOG( QString( "Cannot create CoreMIDI client: %1" ).arg( err ));
	}

	err = MIDIInputPortCreate ( h2MIDIClient, CFSTR( "h2InputPort" ), midiProc, this, &h2InputRef );
	if ( err != noErr ) {
		ERRORLOG( QString( "Cannot create CoreMIDI input port: %1" ).arg( err ));
	}

	err = MIDIOutputPortCreate ( h2MIDIClient, CFSTR( "h2OutputPort" ), &h2OutputRef );
	if ( err != noErr ) {
		ERRORLOG( QString( "Cannot create CoreMIDI output port: %1" ).arg( err ));
	}

	err = MIDISourceCreate ( h2MIDIClient, CFSTR( "Hydrogen" ), &h2VirtualOut );
	if ( err != noErr ) {
		ERRORLOG( QString( "Cannot create CoreMIDI virtual output: %1" ).arg( err ));
	}
}



CoreMidiDriver::~CoreMidiDriver()
{
	/*if ( isMidiDriverRunning ) {
		close();
	} */
	close();
	INFOLOG( "DESTROY" );
}



void CoreMidiDriver::open()
{
	INFOLOG( "open" );

	OSStatus err = noErr;

	QString sMidiPortName = Preferences::get_instance()->m_sMidiPortName;

	cmSources = MIDIGetNumberOfSources();
	unsigned i;
	for ( i = 0; i < cmSources; i++ ) {
		CFStringRef H2MidiNames;
		cmH2Src = MIDIGetSource( i );

		if ( cmH2Src ) {
			err = MIDIObjectGetStringProperty( cmH2Src, kMIDIPropertyName, &H2MidiNames );
			char cmName[64];
			err = CFStringGetCString( H2MidiNames, cmName, 64, kCFStringEncodingASCII );
			QString h2MidiPortName = cmName;
			if ( h2MidiPortName == sMidiPortName ) {
				MIDIPortConnectSource ( h2InputRef, cmH2Src, NULL );
				m_bRunning = true;
			}
		}
		CFRelease ( H2MidiNames );
	}

	int n = MIDIGetNumberOfDestinations();
	if (n > 0) {
		cmH2Dst = MIDIGetDestination(0);
	}

	if (cmH2Dst != 0) {
		CFStringRef H2MidiNames;

		MIDIObjectGetStringProperty(cmH2Dst, kMIDIPropertyName, &H2MidiNames);
		//CFStringGetCString(pname, name, sizeof(name), 0);
		//MIDIPortConnectSource ( h2OutputRef, cmH2Dst, NULL );
		MIDIPortConnectSource ( h2OutputRef, cmH2Dst, NULL );
		if( H2MidiNames != NULL){
			CFRelease( H2MidiNames );
		}
	}
}



void CoreMidiDriver::close()
{
	OSStatus err = noErr;
	err = MIDIPortDisconnectSource( h2InputRef, cmH2Src );
	err = MIDIPortDispose( h2InputRef );
	err = MIDIEndpointDispose( h2VirtualOut );
	//err = MIDIPortDisconnectSource( h2OutputRef, cmH2Dst );
	//err = MIDIPortDispose( h2OutputRef );
	err = MIDIClientDispose( h2MIDIClient );
}

std::vector<QString> CoreMidiDriver::getInputPortList()
{
	INFOLOG( "retrieving output list" );
	OSStatus err = noErr;

	std::vector<QString> cmPortList;
	
	cmSources = MIDIGetNumberOfDestinations();

	INFOLOG ( "Getting number of MIDI sources . . .\n" );

	unsigned i;
	for ( i = 0; i < cmSources; i++ ) {
		CFStringRef H2MidiNames;
		cmH2Src = MIDIGetDestination( i );
		if ( cmH2Src == 0 ) {
			ERRORLOG( "Could not open output device" );
		}
		if ( cmH2Src ) {
			err = MIDIObjectGetStringProperty( cmH2Src, kMIDIPropertyName, &H2MidiNames );
			INFOLOG ( "Getting MIDI object string property . . .\n" );
			char cmName[ 64 ];
			CFStringGetCString( H2MidiNames, cmName, 64, kCFStringEncodingASCII );
			INFOLOG ( "Getting MIDI object name . . .\n" );
			QString h2MidiPortName = cmName;
			cmPortList.push_back( h2MidiPortName );
		}
		CFRelease( H2MidiNames );
	}

	return cmPortList;
}

std::vector<QString> CoreMidiDriver::getOutputPortList()
{
	INFOLOG( "retrieving output list" );
	OSStatus err = noErr;

	std::vector<QString> cmPortList;
	cmSources = MIDIGetNumberOfSources();

	INFOLOG ( "Getting number of MIDI sources . . .\n" );

	unsigned i;
	for ( i = 0; i < cmSources; i++ ) {
		CFStringRef H2MidiNames;
		cmH2Src = MIDIGetSource( i );
		if ( cmH2Src == 0 ) {
			ERRORLOG( "Could not open input device" );
		}
		if ( cmH2Src ) {
			err = MIDIObjectGetStringProperty( cmH2Src, kMIDIPropertyName, &H2MidiNames );
			INFOLOG ( "Getting MIDI object string property . . .\n" );
			char cmName[ 64 ];
			CFStringGetCString( H2MidiNames, cmName, 64, kCFStringEncodingASCII );
			INFOLOG ( "Getting MIDI object name . . .\n" );
			QString h2MidiPortName = cmName;
			cmPortList.push_back( h2MidiPortName );
		}
		CFRelease( H2MidiNames );
	}

	return cmPortList;
}

void CoreMidiDriver::handleQueueNote(Note* pNote)
{
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	int channel = pNote->get_instrument()->get_midi_out_channel();
	if (channel < 0) {
		return;
	}

	int key = pNote->get_midi_key();
	int velocity = pNote->get_midi_velocity();

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	packetList.packet->data[0] = 0x80 | channel;
	packetList.packet->data[1] = key;
	packetList.packet->data[2] = velocity;

	sendMidiPacket ( &packetList );

	packetList.packet->data[0] = 0x90 | channel;
	packetList.packet->data[1] = key;
	packetList.packet->data[2] = velocity;

	sendMidiPacket ( &packetList );
}

void CoreMidiDriver::handleQueueNoteOff( int channel, int key, int velocity )
{
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

//	int channel = pNote->get_instrument()->get_midi_out_channel();
	if (channel < 0) {
		return;
	}

//	int key = pNote->get_instrument()->get_midi_out_note();
//	int velocity = pNote->get_velocity() * 127;

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	packetList.packet->data[0] = 0x80 | channel;
	packetList.packet->data[1] = key;
	packetList.packet->data[2] = velocity;

	sendMidiPacket ( &packetList );
}

void CoreMidiDriver::handleQueueAllNoteOff()
{
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	InstrumentList *instList = Hydrogen::get_instance()->getSong()->get_instrument_list();

	unsigned int numInstruments = instList->size();
	for (int index = 0; index < numInstruments; ++index) {
		Instrument *curInst = instList->get(index);

		int channel = curInst->get_midi_out_channel();
		if (channel < 0) {
			continue;
		}
		int key = curInst->get_midi_out_note();

		MIDIPacketList packetList;
		packetList.numPackets = 1;

		packetList.packet->timeStamp = 0;
		packetList.packet->length = 3;
		packetList.packet->data[0] = 0x80 | channel;
		packetList.packet->data[1] = key;
		packetList.packet->data[2] = 0;

		sendMidiPacket ( &packetList );
	}
}

void CoreMidiDriver::handleOutgoingControlChange( int param, int value, int channel )
{
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	if (channel < 0) {
		return;
	}

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	packetList.packet->data[0] = 0xB0 | channel;
	packetList.packet->data[1] = param;
	packetList.packet->data[2] = value;

	sendMidiPacket ( &packetList );
}


void CoreMidiDriver::sendMidiPacket (MIDIPacketList *packetList)
{
	OSStatus err = noErr;

	err = MIDISend(h2OutputRef, cmH2Dst, packetList);
	if ( err != noErr ) {
		ERRORLOG( QString( "Cannot send MIDI packet to output port: %1" ).arg( err ));
	}

	err = MIDIReceived(h2VirtualOut, packetList);
	if ( err != noErr ) {
		ERRORLOG( QString( "Cannot send MIDI packet to virtual output: %1" ).arg( err ));
	}
}

} // namespace H2CORE
#endif

