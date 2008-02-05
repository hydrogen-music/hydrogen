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


#ifdef COREMIDI_SUPPORT

#include <hydrogen/IO/CoreMidiDriver.h>

#include <hydrogen/Preferences.h>

namespace H2Core
{


static void midiProc ( const MIDIPacketList * pktlist,
                       void * readProcRefCon,
                       void * srcConnRefCon )
{
	UNUSED( srcConnRefCon );

	MIDIPacket* packet = ( MIDIPacket * )pktlist->packet;

	_ERRORLOG( " MIDIPROC packets #" + to_string( pktlist->numPackets ) );

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
			msg.m_nChannel = nEventType - 240;
			msg.m_type = MidiMessage::SYSTEM_EXCLUSIVE;
		} else {
			_ERRORLOG( "Unhandled midi message type: " + to_string( nEventType ) );
			_INFOLOG( "MIDI msg: " );
			// instance->errorLog( "Unhandled midi message type: " + to_string( nEventType ) );
			// instance->infoLog( "MIDI msg: " );
		}

		msg.m_nData1 = packet->data[1];
		msg.m_nData2 = packet->data[2];
		instance->handleMidiMessage( msg );
		packet = MIDIPacketNext( packet );
	}
}



CoreMidiDriver::CoreMidiDriver()
		: MidiInput( "CoreMidiDriver" )
		, m_bRunning( false )
{
	INFOLOG( "INIT" );
	OSStatus err = noErr;

	std::string sMidiPortName = Preferences::getInstance()->m_sMidiPortName;
	err = MIDIClientCreate ( CFSTR( "h2MIDIClient" ), NULL, NULL, &h2MIDIClient );
	err = MIDIInputPortCreate ( h2MIDIClient, CFSTR( "h2InputPort" ), midiProc, this, &h2InputRef );
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

	std::string sMidiPortName = Preferences::getInstance()->m_sMidiPortName;

	cmSources = MIDIGetNumberOfSources();
	unsigned i;
	for ( i = 0; i < cmSources; i++ ) {
		CFStringRef H2MidiNames;
		cmH2Src = MIDIGetSource( i );

		if ( cmH2Src ) {
			err = MIDIObjectGetStringProperty( cmH2Src, kMIDIPropertyName, &H2MidiNames );
			char cmName[64];
			err = CFStringGetCString( H2MidiNames, cmName, 64, kCFStringEncodingASCII );
			std::string h2MidiPortName = ( std::string )cmName;
			if ( h2MidiPortName == sMidiPortName ) {
				MIDIPortConnectSource ( h2InputRef, cmH2Src, NULL );
				m_bRunning = true;
			}
		}
		CFRelease ( H2MidiNames );
	}
}



void CoreMidiDriver::close()
{
	OSStatus err = noErr;
	err = MIDIPortDisconnectSource( h2InputRef, cmH2Src );
	err = MIDIPortDispose( h2InputRef );
	err = MIDIClientDispose( h2MIDIClient );
}



std::vector<std::string> CoreMidiDriver::getOutputPortList()
{
	INFOLOG( "retrieving output list" );
	OSStatus err = noErr;

	std::vector<std::string> cmPortList;
	cmSources = MIDIGetNumberOfSources();

	INFOLOG ( "Getting number of MIDI sources . . .\n" );

	unsigned i;
	for ( i = 0; i < cmSources; i++ ) {
		CFStringRef H2MidiNames;
		cmH2Src = MIDIGetSource( i );
		if ( cmH2Src == NULL ) {
			ERRORLOG( "Could not open input device" );
		}
		if ( cmH2Src ) {
			err = MIDIObjectGetStringProperty( cmH2Src, kMIDIPropertyName, &H2MidiNames );
			INFOLOG ( "Getting MIDI object string property . . .\n" );
			char cmName[ 64 ];
			CFStringGetCString( H2MidiNames, cmName, 64, kCFStringEncodingASCII );
			INFOLOG ( "Getting MIDI object name . . .\n" );
			std::string h2MidiPortName = ( std::string )cmName;
			cmPortList.push_back( h2MidiPortName );
		}
		CFRelease( H2MidiNames );
	}
	return cmPortList;
}

} // namespace H2CORE
#endif

