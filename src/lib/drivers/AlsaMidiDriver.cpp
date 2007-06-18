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
 * $Id: AlsaMidiDriver.cpp,v 1.15 2005/06/23 11:53:43 comix Exp $
 *
 */

#include "AlsaMidiDriver.h"

#ifdef ALSA_SUPPORT

#include "lib/Preferences.h"
#include "lib/Hydrogen.h"
#include "lib/EventQueue.h"

#include <pthread.h>

pthread_t midiDriverThread;

bool isMidiDriverRunning = false;

snd_seq_t *seq_handle = NULL;
int npfd;
struct pollfd *pfd;
int portId;
int clientId;


void* alsaMidiDriver_thread(void* param)
{
	AlsaMidiDriver *pDriver = ( AlsaMidiDriver* )param;
	pDriver->infoLog("alsaMidiDriver_thread() starting");

	if ( seq_handle != NULL ) {
		pDriver->errorLog( "alsaMidiDriver_thread(): seq_handle != NULL" );
		pthread_exit( NULL );
	}

	int err;
	if ( ( err = snd_seq_open( &seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0 ) ) < 0 ) {
		pDriver->errorLog( "Error opening ALSA sequencer: " + string( snd_strerror(err) ) );
		pthread_exit( NULL );
	}

	snd_seq_set_client_name( seq_handle, "Hydrogen" );

	if ( (portId = snd_seq_create_simple_port( 	seq_handle,
									"Hydrogen Midi-In",
									SND_SEQ_PORT_CAP_WRITE |
									SND_SEQ_PORT_CAP_SUBS_WRITE,
									SND_SEQ_PORT_TYPE_APPLICATION
									)
			) < 0)
	{
		pDriver->errorLog("Error creating sequencer port.");
		pthread_exit(NULL);
	}
	clientId = snd_seq_client_id( seq_handle );

	int m_local_addr_port = portId;
	int m_local_addr_client = clientId;

	string sPortName = Preferences::getInstance()->m_sMidiPortName;
	int m_dest_addr_port = -1;
	int m_dest_addr_client = -1;
	pDriver->getPortInfo( sPortName, m_dest_addr_client, m_dest_addr_port );
	pDriver->infoLog( "MIDI port name: " + sPortName );
	pDriver->infoLog( "MIDI addr client: " + toString( m_dest_addr_client ) );
	pDriver->infoLog( "MIDI addr port: " + toString( m_dest_addr_port ) );

	if ( ( m_dest_addr_port != -1 ) && ( m_dest_addr_client != -1 ) ) {
		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca( &subs );
		snd_seq_addr_t sender, dest;

		sender.client = m_dest_addr_client;
		sender.port = m_dest_addr_port;
		dest.client = m_local_addr_client;
		dest.port = m_local_addr_port;

		/* set in and out ports */
		snd_seq_port_subscribe_set_sender( subs, &sender );
		snd_seq_port_subscribe_set_dest( subs, &dest );

		/* subscribe */
		int ret = snd_seq_subscribe_port(seq_handle, subs);
		if ( ret < 0 ){
			pDriver->errorLog( "snd_seq_connect_from(" + toString(m_dest_addr_client) + ":" + toString(m_dest_addr_port) +" error" );
		}
	}

	pDriver->infoLog( "Midi input port at " + toString(clientId) + ":" + toString(portId) );

	npfd = snd_seq_poll_descriptors_count( seq_handle, POLLIN );
	pfd = ( struct pollfd* )alloca( npfd * sizeof( struct pollfd ) );
	snd_seq_poll_descriptors( seq_handle, pfd, npfd, POLLIN );

	pDriver->infoLog("MIDI Thread INIT");
	while( isMidiDriverRunning ) {
		if ( poll( pfd, npfd, 100 ) > 0 ) {
			pDriver->midi_action( seq_handle );
		}
	}
	snd_seq_close ( seq_handle );
	seq_handle = NULL;
	pDriver->infoLog("MIDI Thread DESTROY");

	pthread_exit(NULL);
	return NULL;
}





AlsaMidiDriver::AlsaMidiDriver()
 : MidiDriver( "AlsaMidiDriver" )
{
//	infoLog("INIT");
}




AlsaMidiDriver::~AlsaMidiDriver()
{
	if ( isMidiDriverRunning ) {
		close();
	}
//	infoLog("DESTROY");
}




void AlsaMidiDriver::open()
{
	// start main thread
	isMidiDriverRunning = true;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&midiDriverThread, &attr, alsaMidiDriver_thread, (void*)this);
}




void AlsaMidiDriver::close()
{
	isMidiDriverRunning = false;
	pthread_join(midiDriverThread, NULL);
}





void AlsaMidiDriver::midi_action(snd_seq_t *seq_handle)
{
//	infoLog( "[midi_action]" );

	Hydrogen *engine = Hydrogen::getInstance();
	int nState = engine->getState();
	if ( (nState != STATE_READY) && (nState != STATE_PLAYING) ) {
		errorLog( "[midi_action] Skipping midi event! Audio engine not ready." );
		return;
	}

	bool useMidiTransport = true;

	snd_seq_event_t *ev;
	do {
		if (!seq_handle) {
			break;
		}
		snd_seq_event_input(seq_handle, &ev);

		if (m_bActive) {

			MidiMessage msg;

			switch ( ev->type ) {
				case SND_SEQ_EVENT_NOTEON:
					msg.m_type = MidiMessage::NOTE_ON;
					msg.m_nData1 = ev->data.note.note;
					msg.m_nData2 = ev->data.note.velocity;
					msg.m_nChannel = ev->data.control.channel;
					break;

				case SND_SEQ_EVENT_NOTEOFF:
					msg.m_type = MidiMessage::NOTE_OFF;
					msg.m_nData1 = ev->data.note.note;
					msg.m_nData2 = ev->data.note.velocity;
					msg.m_nChannel = ev->data.control.channel;
					break;

				case SND_SEQ_EVENT_CONTROLLER:
					msg.m_type = MidiMessage::CONTROL_CHANGE;
					break;

				case SND_SEQ_EVENT_SYSEX:
					{
					// midi dump
						snd_midi_event_t *seq_midi_parser;
						if( snd_midi_event_new( 32, &seq_midi_parser ) ) {
							errorLog( "[midi_action] error creating midi event parser" );
						}
						unsigned char midi_event_buffer[ 256 ];
						int _bytes_read = snd_midi_event_decode( seq_midi_parser, midi_event_buffer, 32, ev );

						sysexEvent( midi_event_buffer, _bytes_read );

						EventQueue::getInstance()->pushEvent( EVENT_MIDI_ACTIVITY, -1 );
					}
					break;

				case SND_SEQ_EVENT_QFRAME:
	//				cout << "quarter frame" << endl;
//					if ( useMidiTransport ) {
//						playEvent();
//					}
					break;

				case SND_SEQ_EVENT_CLOCK:
	//				cout << "Midi CLock" << endl;
					break;

				case SND_SEQ_EVENT_SONGPOS:
					msg.m_type = MidiMessage::SONG_POS;
					break;

				case SND_SEQ_EVENT_START:
					msg.m_type = MidiMessage::START;
					break;

				case SND_SEQ_EVENT_CONTINUE:
					msg.m_type = MidiMessage::CONTINUE;
					break;

				case SND_SEQ_EVENT_STOP:
					msg.m_type = MidiMessage::STOP;
					break;

				case SND_SEQ_EVENT_PITCHBEND:
					break;

				case SND_SEQ_EVENT_PGMCHANGE:
					break;

				case SND_SEQ_EVENT_CLIENT_EXIT:
					infoLog( "[midi_action] SND_SEQ_EVENT_CLIENT_EXIT" );
					break;

				case SND_SEQ_EVENT_PORT_SUBSCRIBED:
					infoLog( "[midi_action] SND_SEQ_EVENT_PORT_SUBSCRIBED" );
					break;

				case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
					infoLog( "[midi_action] SND_SEQ_EVENT_PORT_UNSUBSCRIBED" );
					break;

				case SND_SEQ_EVENT_SENSING:
					break;

				default:
					warningLog( "[midi_action] AlsaMidiDriver: Unknown MIDI Event. type = " + toString((int)ev->type) );
			}
			handleMidiMessage( msg );
		}
		snd_seq_free_event( ev );
	} while ( snd_seq_event_input_pending( seq_handle, 0 ) > 0 );
}






/*
void AlsaMidiDriver::controllerEvent( snd_seq_event_t* ev )
{
	int channel = ev->data.control.channel;
	unsigned param = ev->data.control.param;
	unsigned value = ev->data.control.value;
	unsigned controllerParam = ev->data.control.param;

	switch ( controllerParam ) {
		case 1:	// ModWheel
			infoLog( "[controllerEvent] ModWheel control event" );
			break;

		case 7:	// Volume
			infoLog( "[controllerEvent] Volume control event" );
			break;

		case 10:	// Pan
			infoLog( "[controllerEvent] Pan control event" );
			break;

		case 11:	// Expression
			infoLog( "[controllerEvent] Expression control event" );
			break;

		case 64:	// Sustain
			infoLog( "[controllerEvent] Sustain control event" );
			break;

		case 66:	// Sostenuto
			infoLog( "[controllerEvent] Sostenuto control event" );
			break;

		case 91:	// reverb
			infoLog( "[controllerEvent] Reverb control event" );
			break;

		case 93:	// chorus
			infoLog( "[controllerEvent] Chorus control event" );
			break;

		default:
			warningLog( "[controllerEvent] Unhandled Control event" );
			warningLog( "[controllerEvent] controller param = " + toString(param) );
			warningLog( "[controllerEvent] controller channel = " + toString(channel) );
			warningLog( "[controllerEvent] controller value = " + toString(value) );
	}
}
*/




void AlsaMidiDriver::sysexEvent( unsigned char *buf, int nBytes )
{
//	infoLog( "[sysexEvent]" );

	/*
		General MMC message
		0	1	2	3	4	5
		240	127	id	6	cmd	247

		cmd:
		1	stop
		2	play
		3	Deferred play
		4	Fast Forward
		5	Rewind
		6	Record strobe (punch in)
		7	Record exit (punch out)
		9	Pause


		Goto MMC message
		0	1	2	3	4	5	6	7	8	9	10	11	12
		240	127	id	6	68	6	1	hr	mn	sc	fr	ff	247
	*/


	if ( nBytes == 6 ) {
		if ( ( buf[0] == 240 ) && ( buf[1] == 127 ) && ( buf[2] == 127 ) && ( buf[3] == 6 ) ) {
			switch (buf[4] ) {
				case 1:	// STOP
					infoLog( "[sysexEvent] MMC STOP" );
//					stopEvent();
					break;

				case 2:	// PLAY
					warningLog( "[sysexEvent] MMC PLAY not implemented yet." );
					break;

				case 3:	//DEFERRED PLAY
					warningLog( "[sysexEvent] MMC DEFERRED PLAY not implemented yet." );
					break;

				case 4:	// FAST FWD
					warningLog( "[sysexEvent] MMC FAST FWD not implemented yet." );
					break;

				case 5:	// REWIND
					warningLog( "[sysexEvent] MMC REWIND not implemented yet." );
					break;

				case 6:	// RECORD STROBE (PUNCH IN)
					warningLog( "[sysexEvent] MMC PUNCH IN not implemented yet." );
					break;

				case 7:	// RECORD EXIT (PUNCH OUT)
					warningLog( "[sysexEvent] MMC PUNCH OUT not implemented yet." );
					break;

				case 9:	//PAUSE
					warningLog( "[sysexEvent] MMC PAUSE not implemented yet." );
					break;

				default:
					warningLog( "[sysexEvent] Unknown MMC Command" );
//					midiDump( buf, nBytes );
			}
		}
	}
	else if ( nBytes == 13 ) {
		warningLog( "[sysexEvent] MMC GOTO Message not implemented yet" );
//		midiDump( buf, nBytes );
		//int id = buf[2];
		int hr = buf[7];
		int mn = buf[8];
		int sc = buf[9];
		int fr = buf[10];
		int ff = buf[11];
		char tmp[200];
		sprintf( tmp, "[sysexEvent] GOTO %d:%d:%d:%d:%d", hr, mn, sc, fr, ff );
		infoLog( tmp );

	}
	else {
		warningLog( "[sysexEvent] Unknown SysEx message" );
//		midiDump( buf, nBytes );
	}
	EventQueue::getInstance()->pushEvent( EVENT_MIDI_ACTIVITY, -1 );
}






std::vector<std::string> AlsaMidiDriver::getOutputPortList()
{
	vector<string> outputList;

	if ( seq_handle == NULL ) {
		return outputList;
	}

	snd_seq_client_info_t *cinfo;	// client info
	snd_seq_port_info_t *pinfo;	// port info

	snd_seq_client_info_alloca( &cinfo );
	snd_seq_client_info_set_client( cinfo, -1 );

	/* while the next client one the sequencer is avaiable */
	while ( snd_seq_query_next_client( seq_handle, cinfo ) >= 0 ) {
		// get client from cinfo
		int client = snd_seq_client_info_get_client( cinfo );

		// fill pinfo
		snd_seq_port_info_alloca( &pinfo );
		snd_seq_port_info_set_client( pinfo, client );
		snd_seq_port_info_set_port( pinfo, -1 );

		// while the next port is available
		while ( snd_seq_query_next_port( seq_handle, pinfo ) >= 0 ) {

			/* get its capability */
			int cap =  snd_seq_port_info_get_capability(pinfo);

			if ( snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo) && snd_seq_port_info_get_client(pinfo) != 0 ) {
				// output ports
				if  (
					(cap & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0 &&
					snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo)
				) {
					infoLog( snd_seq_port_info_get_name(pinfo) );
					outputList.push_back( snd_seq_port_info_get_name(pinfo) );
					//info.m_nClient = snd_seq_port_info_get_client(pinfo);
					//info.m_nPort = snd_seq_port_info_get_port(pinfo);
				}
			}
		}
	}

	return outputList;
}

void AlsaMidiDriver::getPortInfo(const std::string& sPortName, int& nClient, int& nPort)
{
	if ( seq_handle == NULL ) {
		errorLog( "[getPortInfo] seq_handle = NULL " );
		return;
	}

	if (sPortName == "None" ) {
		nClient = -1;
		nPort = -1;
		return;
	}

	snd_seq_client_info_t *cinfo;	// client info
	snd_seq_port_info_t *pinfo;	// port info

	snd_seq_client_info_alloca( &cinfo );
	snd_seq_client_info_set_client( cinfo, -1 );

	/* while the next client one the sequencer is avaiable */
	while ( snd_seq_query_next_client( seq_handle, cinfo ) >= 0 ) {
		// get client from cinfo
		int client = snd_seq_client_info_get_client( cinfo );

		// fill pinfo
		snd_seq_port_info_alloca( &pinfo );
		snd_seq_port_info_set_client( pinfo, client );
		snd_seq_port_info_set_port( pinfo, -1 );

		// while the next port is avail
		while ( snd_seq_query_next_port( seq_handle, pinfo ) >= 0 ) {
			int cap =  snd_seq_port_info_get_capability(pinfo);
			if ( snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo) && snd_seq_port_info_get_client(pinfo) != 0 ) {
				// output ports
				if 	(
					(cap & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0 &&
					snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo)
					)
				{
					string sName = snd_seq_port_info_get_name(pinfo);
					if ( sName == sPortName ) {
						nClient = snd_seq_port_info_get_client(pinfo);
						nPort = snd_seq_port_info_get_port(pinfo);

						infoLog( "[getPortinfo] nClient " + toString(nClient) );
						infoLog( "[getPortinfo] nPort " + toString(nPort) );
						return;
					}
				}
			}
		}
	}
	errorLog( "[getPortinfo] Midi port " + sPortName + " not found" );
}


#endif // ALSA_SUPPORT

