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

#include "AlsaMidiDriver.h"

#ifdef ALSA_SUPPORT

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>

#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>

#include <pthread.h>
#include <hydrogen/note.h>
#include <hydrogen/instrument.h>

#ifdef LASH_SUPPORT
#include <hydrogen/LashClient.h>
#endif

using namespace std;

namespace H2Core
{

pthread_t midiDriverThread;

bool isMidiDriverRunning = false;

snd_seq_t *seq_handle = NULL;
int npfd;
struct pollfd *pfd;
int portId;
int clientId;
int outPortId;


void* alsaMidiDriver_thread( void* param )
{
	AlsaMidiDriver *pDriver = ( AlsaMidiDriver* )param;
	_INFOLOG( "starting" );

	if ( seq_handle != NULL ) {
		_ERRORLOG( "seq_handle != NULL" );
		pthread_exit( NULL );
	}

	int err;
	if ( ( err = snd_seq_open( &seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0 ) ) < 0 ) {
		_ERRORLOG( QString( "Error opening ALSA sequencer: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		pthread_exit( NULL );
	}

	snd_seq_set_client_name( seq_handle, "Hydrogen" );

	if ( ( portId = snd_seq_create_simple_port( 	seq_handle,
	                "Hydrogen Midi-In",
	                SND_SEQ_PORT_CAP_WRITE |
	                SND_SEQ_PORT_CAP_SUBS_WRITE,
	                SND_SEQ_PORT_TYPE_APPLICATION
	                                          )
	     ) < 0 ) {
		_ERRORLOG( "Error creating sequencer port." );
		pthread_exit( NULL );
	}
	
	if ( ( outPortId = snd_seq_create_simple_port( 	seq_handle,
	                "Hydrogen Midi-Out",
	                SND_SEQ_PORT_CAP_READ |
	                SND_SEQ_PORT_CAP_SUBS_READ,
	                SND_SEQ_PORT_TYPE_APPLICATION
	                                          )
	     ) < 0 ) {
		_ERRORLOG( "Error creating sequencer port." );
		pthread_exit( NULL );
	}
	
	clientId = snd_seq_client_id( seq_handle );

#ifdef LASH_SUPPORT
	if ( Preferences::get_instance()->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient && lashClient->isConnected())
		{
			lashClient->setAlsaClientId((unsigned char) clientId);
		}
	}
#endif


	int m_local_addr_port = portId;
	int m_local_addr_client = clientId;

	QString sPortName = Preferences::get_instance()->m_sMidiPortName;
	int m_dest_addr_port = -1;
	int m_dest_addr_client = -1;
	pDriver->getPortInfo( sPortName, m_dest_addr_client, m_dest_addr_port );
	_INFOLOG( "MIDI port name: "  + sPortName );
	_INFOLOG( "MIDI addr client: " +  m_dest_addr_client );
	_INFOLOG( "MIDI addr port: " + m_dest_addr_port );

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
		int ret = snd_seq_subscribe_port( seq_handle, subs );
		if ( ret < 0 ) {
			_ERRORLOG( QString( "snd_seq_subscribe_port(%1:%2) error" ).arg( m_dest_addr_client ).arg( m_dest_addr_port ) );
		}
	}

	_INFOLOG( QString( "Midi input port at %1:%2" ).arg( clientId ).arg( portId ) );

	npfd = snd_seq_poll_descriptors_count( seq_handle, POLLIN );
	pfd = ( struct pollfd* )alloca( npfd * sizeof( struct pollfd ) );
	snd_seq_poll_descriptors( seq_handle, pfd, npfd, POLLIN );

	_INFOLOG( "MIDI Thread INIT" );
	while ( isMidiDriverRunning ) {
		if ( poll( pfd, npfd, 100 ) > 0 ) {
			pDriver->midi_action( seq_handle );
		}
	}
	snd_seq_close ( seq_handle );
	seq_handle = NULL;
	_INFOLOG( "MIDI Thread DESTROY" );

	pthread_exit( NULL );
	return NULL;
}





AlsaMidiDriver::AlsaMidiDriver()
		: MidiInput( "AlsaMidiDriver" ), MidiOutput( "AlsaMidiDriver" ), Object( "AlsaMidiDriver" )
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
	pthread_attr_init( &attr );
	pthread_create( &midiDriverThread, &attr, alsaMidiDriver_thread, ( void* )this );
}




void AlsaMidiDriver::close()
{
	isMidiDriverRunning = false;
	pthread_join( midiDriverThread, NULL );
}





void AlsaMidiDriver::midi_action( snd_seq_t *seq_handle )
{
	Hydrogen *engine = Hydrogen::get_instance();
	int nState = engine->getState();
	if ( ( nState != STATE_READY ) && ( nState != STATE_PLAYING ) ) {
// 		ERRORLOG( "Skipping midi event! Audio engine not ready." );
		return;
	}

//	bool useMidiTransport = true;

	snd_seq_event_t *ev;
	do {
		if ( !seq_handle ) {
			break;
		}
		snd_seq_event_input( seq_handle, &ev );

		if ( m_bActive ) {

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
				msg.m_nData1 = ev->data.control.param;
				msg.m_nData2 = ev->data.control.value;
				msg.m_nChannel = ev->data.control.channel;
				break;

			case SND_SEQ_EVENT_SYSEX: {
				msg.m_type = MidiMessage::SYSEX;
				snd_midi_event_t *seq_midi_parser;
				if ( snd_midi_event_new( 32, &seq_midi_parser ) ) {
					ERRORLOG( "Error creating midi event parser" );
				}
				unsigned char midi_event_buffer[ 256 ];
				int _bytes_read = snd_midi_event_decode( seq_midi_parser, midi_event_buffer, 32, ev );

				for ( int i = 0; i < _bytes_read; ++i ) {
					msg.m_sysexData.push_back( midi_event_buffer[ i ] );
				}
			}
			break;

			case SND_SEQ_EVENT_QFRAME:
				msg.m_type = MidiMessage::QUARTER_FRAME;
				break;

			case SND_SEQ_EVENT_CLOCK:
				//cout << "!!! Midi CLock" << endl;
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
				msg.m_type = MidiMessage::PROGRAM_CHANGE;
				msg.m_nData1 = ev->data.control.value;
				break;

			case SND_SEQ_EVENT_CLIENT_EXIT:
				INFOLOG( "SND_SEQ_EVENT_CLIENT_EXIT" );
				break;

			case SND_SEQ_EVENT_PORT_SUBSCRIBED:
				INFOLOG( "SND_SEQ_EVENT_PORT_SUBSCRIBED" );
				break;

			case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
				INFOLOG( "SND_SEQ_EVENT_PORT_UNSUBSCRIBED" );
				break;

			case SND_SEQ_EVENT_SENSING:
				break;

			default:
				WARNINGLOG( QString( "Unknown MIDI Event. type = %1" ).arg( ( int )ev->type ) );
			}
			if ( msg.m_type != MidiMessage::UNKNOWN ) {
				handleMidiMessage( msg );
			}
		}
		snd_seq_free_event( ev );
	} while ( snd_seq_event_input_pending( seq_handle, 0 ) > 0 );
}




std::vector<QString> AlsaMidiDriver::getOutputPortList()
{
	vector<QString> outputList;

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
			int cap =  snd_seq_port_info_get_capability( pinfo );

			if ( snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client( pinfo ) && snd_seq_port_info_get_client( pinfo ) != 0 ) {
				// output ports
				if  (
				    ( cap & SND_SEQ_PORT_CAP_SUBS_READ ) != 0 &&
				    snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client( pinfo )
				) {
					INFOLOG( snd_seq_port_info_get_name( pinfo ) );
					outputList.push_back( snd_seq_port_info_get_name( pinfo ) );
					//info.m_nClient = snd_seq_port_info_get_client(pinfo);
					//info.m_nPort = snd_seq_port_info_get_port(pinfo);
				}
			}
		}
	}

	return outputList;
}

void AlsaMidiDriver::getPortInfo( const QString& sPortName, int& nClient, int& nPort )
{
	if ( seq_handle == NULL ) {
		ERRORLOG( "seq_handle = NULL " );
		return;
	}

	if ( sPortName == "None" ) {
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
			int cap =  snd_seq_port_info_get_capability( pinfo );
			if ( snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client( pinfo ) && snd_seq_port_info_get_client( pinfo ) != 0 ) {
				// output ports
				if 	(
				    ( cap & SND_SEQ_PORT_CAP_SUBS_READ ) != 0 &&
				    snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client( pinfo )
				) {
					QString sName = snd_seq_port_info_get_name( pinfo );
					if ( sName == sPortName ) {
						nClient = snd_seq_port_info_get_client( pinfo );
						nPort = snd_seq_port_info_get_port( pinfo );

						INFOLOG( QString( "nClient %1" ).arg( nClient ) );
						INFOLOG( QString( "nPort %1" ).arg( nPort ) );
						return;
					}
				}
			}
		}
	}
	ERRORLOG( "Midi port " + sPortName + " not found" );
}

void AlsaMidiDriver::handleQueueNote(Note* pNote)
{	
	if ( seq_handle == NULL ) {
		ERRORLOG( "seq_handle = NULL " );
		return;
	}

	int channel = pNote->get_instrument()->get_midi_out_channel();
	if (channel < 0) {
		return;
	}
	int key = (pNote->m_noteKey.m_nOctave +3 ) * 12 + pNote->m_noteKey.m_key + pNote->get_instrument()->get_midi_out_note() - 60;
	//int key = pNote->get_instrument()->get_midi_out_note();
	int velocity = pNote->get_velocity() * 127;

	snd_seq_event_t ev;	
	
	//Note off
	snd_seq_ev_clear(&ev);
        snd_seq_ev_set_source(&ev, outPortId);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);
	snd_seq_ev_set_noteoff(&ev, channel, key, velocity);
	snd_seq_event_output(seq_handle, &ev);
	snd_seq_drain_output(seq_handle);
	
	//Note on
	//snd_seq_event_input(seq_handle, &ev);
	snd_seq_ev_clear(&ev);	
        snd_seq_ev_set_source(&ev, outPortId);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);
        //snd_seq_event_output_direct( seq_handle, ev );
	
	snd_seq_ev_set_noteon(&ev, channel, key, velocity);
	snd_seq_event_output(seq_handle, &ev);

        //snd_seq_free_event(ev);
	snd_seq_drain_output(seq_handle);
}

void AlsaMidiDriver::handleQueueNoteOff( int channel, int key, int velocity )
{	
	if ( seq_handle == NULL ) {
		ERRORLOG( "seq_handle = NULL " );
		return;
	}

//	channel = pNote->get_instrument()->get_midi_out_channel();
	if (channel < 0) {
		return;
	}

//	key = (pNote->m_noteKey.m_nOctave +3 ) * 12 + pNote->m_noteKey.m_key;
//	int velocity = pNote->get_velocity() * 127;

	snd_seq_event_t ev;	
	
	//Note off
	snd_seq_ev_clear(&ev);
        snd_seq_ev_set_source(&ev, outPortId);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);
	snd_seq_ev_set_noteoff(&ev, channel, key, velocity);
	snd_seq_event_output(seq_handle, &ev);
	snd_seq_drain_output(seq_handle);
}

void AlsaMidiDriver::handleQueueAllNoteOff()
{
	if ( seq_handle == NULL ) {
		ERRORLOG( "seq_handle = NULL " );
		return;
	}
	
	InstrumentList *instList = Hydrogen::get_instance()->getSong()->get_instrument_list();
		
	unsigned int numInstruments = instList->get_size();
	for (int index = 0; index < numInstruments; ++index) {
		Instrument *curInst = instList->get(index);
	
		int channel = curInst->get_midi_out_channel();
		if (channel < 0) {
			continue;
		}
		int key = curInst->get_midi_out_note();
	
		snd_seq_event_t ev;	
	
		//Note off
		snd_seq_ev_clear(&ev);
	        snd_seq_ev_set_source(&ev, outPortId);
	        snd_seq_ev_set_subs(&ev);
	        snd_seq_ev_set_direct(&ev);
		snd_seq_ev_set_noteoff(&ev, channel, key, 0);
		snd_seq_event_output(seq_handle, &ev);
		snd_seq_drain_output(seq_handle);
	}
}

};

#endif // ALSA_SUPPORT

