/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 * Code cleanup (20060222 Jonathan Dempsey)
 * More cleaning (20060511 Jonathan Dempsey)
 * ... and a little bit more cleaning . . . (20060512 Jonathan Dempsey)
 * Added CFRelease code (20060514 Jonathan Dempsey)
 */

#include <core/IO/CoreMidiDriver.h>

#include <core/Hydrogen.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>

#if defined(H2CORE_HAVE_COREMIDI) || _DOXYGEN_

namespace H2Core
{


static void midiProc ( const MIDIPacketList * pktlist,
					   void * readProcRefCon,
					   void * srcConnRefCon )
{
	UNUSED( srcConnRefCon );

	MIDIPacket* packet = ( MIDIPacket * )pktlist->packet;

	CoreMidiDriver *instance = ( CoreMidiDriver * )readProcRefCon;
	for ( uint i = 0; i < pktlist->numPackets; i++ ) {
		MidiMessage msg;
		int nEventType = packet->data[0];
		msg.setType( MidiMessage::deriveType( nEventType ) );
		msg.setChannel( MidiMessage::deriveChannel( nEventType ) );

		if ( nEventType == 240 ) {
			// SysEx messages also contain arbitrary data which has to
			// be copied manually.
			for ( int i = 0; i < packet->length; i++ ) {
				msg.appendToSysexData( packet->data[ i ] );
			}
		}
		else {
			msg.setData1( Midi::parameterFromIntClamp( packet->data[1] ) );
			msg.setData2( Midi::parameterFromIntClamp( packet->data[2] ) );
		}

		instance->handleMessage( msg );
		packet = MIDIPacketNext( packet );
	}
}


CoreMidiDriver::CoreMidiDriver()
		: MidiBaseDriver()
		, m_bRunning( false )
{
	
	OSStatus err = noErr;
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
			if ( h2MidiPortName == sMidiPortName &&
				 sMidiPortName != Preferences::getNullMidiPort() ) {
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

std::vector<QString> CoreMidiDriver::getExternalPortList( const PortType& portType ) {
	INFOLOG( "retrieving port list" );
	OSStatus err = noErr;

	std::vector<QString> portList;

	if ( portType == PortType::Input ) {
		cmSources = MIDIGetNumberOfDestinations();
	} else {
		cmSources = MIDIGetNumberOfSources();
	}

	INFOLOG( QString( "Getting number of MIDI %1 sources . . .\n" )
			 .arg( portTypeToQString( portType ) ) );

	unsigned i;
	for ( i = 0; i < cmSources; i++ ) {
		CFStringRef H2MidiNames;
		if ( portType == PortType::Input ) {
			cmH2Src = MIDIGetDestination( i );
		} else {
			cmH2Src = MIDIGetSource( i );
		}
		if ( cmH2Src == 0 ) {
			ERRORLOG( QString( "Could not open %1 device" )
					  .arg( portTypeToQString( portType ) ) );
		}
		if ( cmH2Src ) {
			err = MIDIObjectGetStringProperty( cmH2Src, kMIDIPropertyName, &H2MidiNames );
			INFOLOG ( "Getting MIDI object string property . . .\n" );
			char cmName[ 64 ];
			CFStringGetCString( H2MidiNames, cmName, 64, kCFStringEncodingASCII );
			INFOLOG ( "Getting MIDI object name . . .\n" );
			QString h2MidiPortName = cmName;
			portList.push_back( h2MidiPortName );
		}
		CFRelease( H2MidiNames );
	}

	return portList;
}

bool CoreMidiDriver::isInputActive() const {
	return m_bRunning;
}

bool CoreMidiDriver::isOutputActive() const {
	return cmH2Dst != 0;
}

void CoreMidiDriver::sendNoteOnMessage( const MidiMessage& msg )
{
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	packetList.packet->data[0] = 0x90 | static_cast<int>( msg.getChannel() );
	packetList.packet->data[1] = static_cast<int>( msg.getData1() );
	packetList.packet->data[2] = static_cast<int>( msg.getData2() );

	sendMidiPacket( &packetList );
}

void CoreMidiDriver::sendNoteOffMessage( const MidiMessage& msg )
{
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	packetList.packet->data[0] = 0x80 | static_cast<int>( msg.getChannel() );
	packetList.packet->data[1] = static_cast<int>( msg.getData1() );
	packetList.packet->data[2] = static_cast<int>( msg.getData2() );

	sendMidiPacket( &packetList );
}

void CoreMidiDriver::sendControlChangeMessage( const MidiMessage& msg ) {
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	packetList.packet->data[0] = 0xB0 | static_cast<int>( msg.getChannel() );
	packetList.packet->data[1] = static_cast<int>( msg.getData1() );
	packetList.packet->data[2] = static_cast<int>( msg.getData2() );

	sendMidiPacket( &packetList );
}

void CoreMidiDriver::sendSystemRealTimeMessage( const MidiMessage& msg ) {
	if (cmH2Dst == 0 ) {
		ERRORLOG( "cmH2Dst = 0 " );
		return;
	}

	MIDIPacketList packetList;
	packetList.numPackets = 1;

	packetList.packet->timeStamp = 0;
	packetList.packet->length = 3;
	if ( msg.getType() == MidiMessage::Type::Start ) {
		packetList.packet->data[ 0 ] = 0xFA;
	}
	else if ( msg.getType() == MidiMessage::Type::Continue ) {
		packetList.packet->data[ 0 ] = 0xFB;
	}
	else if ( msg.getType() == MidiMessage::Type::Stop ) {
		packetList.packet->data[ 0 ] = 0xFC;
	}
	else if ( msg.getType() == MidiMessage::Type::TimingClock ) {
		packetList.packet->data[ 0 ] = 0xF8;
	}
	else {
		ERRORLOG( QString( "Unsupported event [%1]" )
				  .arg( MidiMessage::TypeToQString( msg.getType() ) ) );
		return;
	}
	packetList.packet->data[1] = 0;
	packetList.packet->data[2] = 0;

	sendMidiPacket( &packetList );
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

