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

#include <hydrogen/IO/JackOutput.h>
#ifdef JACK_SUPPORT

#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/Song.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/globals.h>

#ifdef LASH_SUPPORT
#include <hydrogen/LashClient.h>
#endif

namespace H2Core
{

unsigned long jack_server_sampleRate = 0;
jack_nframes_t jack_server_bufferSize = 0;
JackOutput *jackDriverInstance = NULL;

int jackDriverSampleRate( jack_nframes_t nframes, void *arg )
{
	UNUSED( arg );
	QString msg = QString("Jack SampleRate changed: the sample rate is now %1/sec").arg( QString::number( (int) nframes ) );
	_INFOLOG( msg );
	jack_server_sampleRate = nframes;
	return 0;
}


int jackDriverBufferSize( jack_nframes_t nframes, void * /*arg*/ )
{
	/* This function does _NOT_ have to be realtime safe.
	 */
	jack_server_bufferSize = nframes;
	return 0;
}

void jackDriverShutdown( void *arg )
{
	UNUSED( arg );
//	jackDriverInstance->deactivate();
	jackDriverInstance->client = NULL;
	Hydrogen::get_instance()->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}




JackOutput::JackOutput( JackProcessCallback processCallback )
		: AudioOutput( "JackOutput" )
{
	INFOLOG( "INIT" );
	__track_out_enabled = Preferences::get_instance()->m_bJackTrackOuts;	// allow per-track output

	jackDriverInstance = this;
	this->processCallback = processCallback;

	must_relocate = 0;
	locate_countdown = 0;
	bbt_frame_offset = 0;
	track_port_count = 0;

	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );
}



JackOutput::~JackOutput()
{
	INFOLOG( "DESTROY" );
	disconnect();
}



// return 0: ok
// return 1: cannot activate client
// return 2: cannot connect output port
// return 3: Jack server not running
// return 4: output port = NULL
int JackOutput::connect()
{
	INFOLOG( "connect" );

	if ( jack_activate ( client ) ) {
		Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT );
		return 1;
	}


	bool connect_output_ports = connect_out_flag;
	
	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );

#ifdef LASH_SUPPORT
	if ( Preferences::get_instance()->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient && lashClient->isConnected())
		{
	//		infoLog("[LASH] Sending Jack client name to LASH server");
			lashClient->sendJackClientName();
			
			if (!lashClient->isNewProject())
			{
				connect_output_ports = false;
			}
		}
	}
#endif
	
	if ( connect_output_ports ) {
//	if ( connect_out_flag ) {
		// connect the ports
		if ( jack_connect( client, jack_port_name( output_port_1 ), output_port_name_1.toLocal8Bit() ) == 0 &&
		        jack_connect ( client, jack_port_name( output_port_2 ), output_port_name_2.toLocal8Bit() ) == 0 ) {
			return 0;
		}

		INFOLOG( "Could not connect so saved out-ports. Connecting to first pair of in-ports" );
		const char ** portnames = jack_get_ports ( client, NULL, NULL, JackPortIsInput );
		if ( !portnames || !portnames[0] || !portnames[1] ) {
			ERRORLOG( "Could't locate two Jack input port" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		if ( jack_connect( client, jack_port_name( output_port_1 ), portnames[0] ) != 0 ||
		        jack_connect( client, jack_port_name( output_port_2 ), portnames[1] ) != 0 ) {
			ERRORLOG( "Could't connect to first pair of Jack input ports" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		free( portnames );
	}
	return 0;
}





void JackOutput::disconnect()
{
	INFOLOG( "disconnect" );

	deactivate();
	jack_client_t *oldClient = client;
	client = NULL;
	if ( oldClient ) {
		INFOLOG( "calling jack_client_close" );
		int res = jack_client_close( oldClient );
		if ( res ) {
			ERRORLOG( "Error in jack_client_close" );
			// FIXME: raise exception
		}
	}
	client = NULL;
}




void JackOutput::deactivate()
{
	INFOLOG( "[deactivate]" );
	if ( client ) {
		INFOLOG( "calling jack_deactivate" );
		int res = jack_deactivate( client );
		if ( res ) {
			ERRORLOG( "Error in jack_deactivate" );
		}
	}
	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );
}

unsigned JackOutput::getBufferSize()
{
	return jack_server_bufferSize;
}

unsigned JackOutput::getSampleRate()
{
	return jack_server_sampleRate;
}

void JackOutput::calculateFrameOffset()
{
	bbt_frame_offset = m_JackTransportPos.frame - m_transport.m_nFrames;
}

int oldpo = 0;
//int changer = 0;

void JackOutput::locateInNCycles( unsigned long frame, int cycles_to_wait )
{
	locate_countdown = cycles_to_wait;
	locate_frame = frame;
}

/// Take beat-bar-tick info from the Jack system, and translate it to a new internal frame position and ticksize.
void JackOutput::relocateBBT()
{
	//wolke if hydrogen is jack time master this is not relevant
	if( Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER &&  m_transport.m_status != TransportInfo::ROLLING) {
		m_transport.m_nFrames = Hydrogen::get_instance()->getHumantimeFrames() - getBufferSize(); // have absolut nothing to do with the old ardour transport bug 
		WARNINGLOG( "Relocate: Call it off" );
		calculateFrameOffset();
	 	return;
	} else {
		if ( m_transport.m_status != TransportInfo::ROLLING || !( m_JackTransportPos.valid & JackPositionBBT ) /**the last check is *probably* redundant*/ ){
			calculateFrameOffset();
			return;
		}
		
		INFOLOG( "..." );
	
		Hydrogen * H = Hydrogen::get_instance();
		Song * S = H->getSong();
	
		float hydrogen_TPB = ( float )( S->__resolution / m_JackTransportPos.beat_type * 4 );
	
		long bar_ticks = 0;
		//long beat_ticks = 0;
		if ( S->get_mode() == Song::SONG_MODE ) {
			bar_ticks = H->getTickForPosition( m_JackTransportPos.bar-1  ); // (Reasonable?) assumption that one pattern is _always_ 1 bar long!
			if ( bar_ticks < 0 ) bar_ticks = 0; // ignore error NOTE This is wrong -- if loop state is off, transport should just stop ??
		}
		float hydrogen_ticks_to_locate =  bar_ticks + ( m_JackTransportPos.beat-1 )*hydrogen_TPB + m_JackTransportPos.tick *( hydrogen_TPB/m_JackTransportPos.ticks_per_beat ) ;
	
// 		INFOLOG( QString( "Position from Time Master: BBT [%1,%2,%3]" ) . arg( m_JackTransportPos.bar ) . arg( m_JackTransportPos.beat ) . arg( m_JackTransportPos.tick ) );
// 		WARNINGLOG( QString(bbt) + " -- Tx/Beat = "+to_string(m_JackTransportPos.ticks_per_beat)+", Meter "+to_string(m_JackTransportPos.beats_per_bar)+"/"+to_string(m_JackTransportPos.beat_type)+" =>tick " + to_string( hydrogen_ticks_to_locate ) );
	
		float fNewTickSize = getSampleRate() * 60.0 /  m_transport.m_nBPM / S->__resolution;
		// not S->m_fBPM !??
	
		if ( fNewTickSize == 0 ) return; // ??!?
	
		// NOTE this _should_ prevent audioEngine_process_checkBPMChanged in Hydrogen.cpp from recalculating things.
		m_transport.m_nTickSize = fNewTickSize;
	
		long long nNewFrames = ( long long )( hydrogen_ticks_to_locate * fNewTickSize );
#ifndef JACK_NO_BBT_OFFSET
		if ( m_JackTransportPos.valid & JackBBTFrameOffset )
			nNewFrames += m_JackTransportPos.bbt_offset ;
#endif
		m_transport.m_nFrames = nNewFrames;
	
		/// offset between jack- and internal position
		calculateFrameOffset();
	}
}

///
/// When jack_transport_start() is called, it takes effect from the next processing cycle.
/// The location info from the timebase_master, if there is one, will not be available until the _next_ next cycle.
/// The code must therefore wait one cycle before syncing up with timebase_master.
///

void JackOutput::updateTransportInfo()
{
	if ( locate_countdown == 1 )
		locate( locate_frame );
	if ( locate_countdown > 0 )
		locate_countdown--;

	if ( Preferences::get_instance()->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT   ) {
		m_JackTransportState = jack_transport_query( client, &m_JackTransportPos );


		// update m_transport with jack-transport data
		switch ( m_JackTransportState ) {
		case JackTransportStopped:
			m_transport.m_status = TransportInfo::STOPPED;
			//infoLog( "[updateTransportInfo] STOPPED - frames: " + to_string(m_transportPos.frame) );
			break;

		case JackTransportRolling:
			if ( m_transport.m_status != TransportInfo::ROLLING && ( m_JackTransportPos.valid & JackPositionBBT ) ) {
				must_relocate = 2;
				//WARNINGLOG( "Jack transport starting: Resyncing in 2 x Buffersize!!" );
			}
			m_transport.m_status = TransportInfo::ROLLING;
			//infoLog( "[updateTransportInfo] ROLLING - frames: " + to_string(m_transportPos.frame) );
			break;

		case JackTransportStarting:
			m_transport.m_status = TransportInfo::STOPPED;
			//infoLog( "[updateTransportInfo] STARTING (stopped) - frames: " + to_string(m_transportPos.frame) );
			break;

		default:
			ERRORLOG( "Unknown jack transport state" );
		}


		// FIXME
		// TickSize and BPM
		Hydrogen * H = Hydrogen::get_instance();

		if ( m_JackTransportPos.valid & JackPositionBBT ) {
			float bpm = ( float )m_JackTransportPos.beats_per_minute;
			if ( m_transport.m_nBPM != bpm ) {

				
				if ( Preferences::get_instance()->m_bJackMasterMode == Preferences::NO_JACK_TIME_MASTER ){
// 					WARNINGLOG( QString( "Tempo change from jack-transport: %1" ).arg( bpm ) );
					m_transport.m_nBPM = bpm;
					must_relocate = 1; // The tempo change has happened somewhere during the previous cycle; relocate right away.

// This commenting out is rude perhaps, but I cant't figure out what this bit is doing.
// In any case, setting must_relocate = 1 here causes too many relocates. Jakob Lund
/*				} else { 
					if ( m_transport.m_status == TransportInfo::STOPPED ) {
						oldpo = H->getPatternPos();
						must_relocate = 1;
						//changer =1;
					}*/

				}
				
				// Hydrogen::get_instance()->setBPM( m_JackTransportPos.beats_per_minute ); // unnecessary, as Song->m_BPM gets updated in audioEngine_process_transport (after calling this function)
			}
		}

		if ( m_transport.m_nFrames + bbt_frame_offset != m_JackTransportPos.frame ) {
			if ( ( m_JackTransportPos.valid & JackPositionBBT ) && must_relocate == 0 ) {
				WARNINGLOG( "Frame offset mismatch; triggering resync in 2 cycles" );
				must_relocate = 2;
			} else {
				if ( Preferences::get_instance()->m_bJackMasterMode == Preferences::NO_JACK_TIME_MASTER ) {
					// If There's no timebase_master, and audioEngine_process_checkBPMChanged handled a tempo change during last cycle, the offset doesn't match, but hopefully it was calculated correctly:

					//this perform Jakobs mod in pattern mode, but both m_transport.m_nFrames works with the same result in pattern Mode
					// in songmode the first case dont work. 
					//so we can remove this "if query" and only use this old mod: m_transport.m_nFrames = H->getHumantimeFrames();
					//because to get the songmode we have to add this "H2Core::Hydrogen *m_pEngine" to the header file
					//if we remove this we also can remove *m_pEngine from header
					if ( m_pEngine->getSong()->get_mode() == Song::PATTERN_MODE  ){
						m_transport.m_nFrames = m_JackTransportPos.frame/* - bbt_frame_offset*/; ///see comment in svn changeset 753
					}
					else
					{
						m_transport.m_nFrames = H->getHumantimeFrames();
					}
					// In jack 'slave' mode, if there's no master, the following line is needed to be able to relocate by clicking the song ruler (wierd corner case, but still...)
					if ( m_transport.m_status == TransportInfo::ROLLING )
							H->triggerRelocateDuringPlay();
				} else {
					///this is experimantal... but it works for the moment... fix me fix :-) wolke
					// ... will this actually happen? keeping it for now ( jakob lund )
					m_transport.m_nFrames = H->getHumantimeFrames() - getBufferSize();// have nothing to do with the old ardour transport bug
				}
			}
		}
		
		// humantime fix
		if ( H->getHumantimeFrames() != m_JackTransportPos.frame ) {

			H->setHumantimeFrames(m_JackTransportPos.frame);
			//WARNINGLOG("fix Humantime " + to_string (m_JackTransportPos.frame));
		}

		if ( must_relocate == 1 ) {
			//WARNINGLOG( "Resyncing!" );
			relocateBBT();
			if ( m_transport.m_status == TransportInfo::ROLLING ) {
				H->triggerRelocateDuringPlay();
			}
		}
		
		if ( must_relocate > 0 ) must_relocate--;
	}
}



float* JackOutput::getOut_L()
{
	jack_default_audio_sample_t *out = ( jack_default_audio_sample_t * ) jack_port_get_buffer ( output_port_1, jack_server_bufferSize );
	return out;
}

float* JackOutput::getOut_R()
{
	jack_default_audio_sample_t *out = ( jack_default_audio_sample_t * ) jack_port_get_buffer ( output_port_2, jack_server_bufferSize );
	return out;
}



float* JackOutput::getTrackOut_L( unsigned nTrack )
{
	if(nTrack > (unsigned)track_port_count ) return 0;
	jack_port_t *p = track_output_ports_L[nTrack];
	jack_default_audio_sample_t* out = 0;
	if( p ) {
		out = (jack_default_audio_sample_t*) jack_port_get_buffer( p, jack_server_bufferSize);
	}
	return out;
}

float* JackOutput::getTrackOut_R( unsigned nTrack )
{
	if(nTrack > (unsigned)track_port_count ) return 0;
	jack_port_t *p = track_output_ports_R[nTrack];
	jack_default_audio_sample_t* out = 0;
	if( p ) {
		out = (jack_default_audio_sample_t*) jack_port_get_buffer( p, jack_server_bufferSize);
	}
	return out;
}


#define CLIENT_FAILURE(msg) {						\
		ERRORLOG("Could not connect to JACK server (" msg ")"); \
		if (client) {						\
			ERRORLOG("...but JACK returned a non-null pointer?"); \
			(client) = 0;					\
		}							\
		if (tries) ERRORLOG("...trying again.");		\
	}


#define CLIENT_SUCCESS(msg) {				\
		assert(client);				\
		INFOLOG(msg);				\
		tries = 0;				\
	}

int JackOutput::init( unsigned /*nBufferSize*/ )
{

	output_port_name_1 = Preferences::get_instance()->m_sJackPortName1;
	output_port_name_2 = Preferences::get_instance()->m_sJackPortName2;

	QString sClientName = "Hydrogen";
	jack_status_t status;
	int tries = 2;  // Sometimes jackd doesn't stop and start fast enough.
	while ( tries > 0 ) {
		--tries;
		client = jack_client_open(
			sClientName.toLocal8Bit(),
			JackNullOption,
			&status);
		switch(status) {
		case JackFailure:
			CLIENT_FAILURE("unknown error");
			break;
		case JackInvalidOption:
			CLIENT_FAILURE("invalid option");
			break;
		case JackNameNotUnique:
			if (client) {
				sClientName = jack_get_client_name(client);
				CLIENT_SUCCESS(QString("Jack assigned the client name '%1'")
					       .arg(sClientName));
			} else {
				CLIENT_FAILURE("name not unique");
			}
			break;
		case JackServerStarted:
			CLIENT_SUCCESS("JACK Server started for Hydrogen.");
			break;
		case JackServerFailed:
			CLIENT_FAILURE("unable to connect");
			break;
		case JackServerError:
			CLIENT_FAILURE("communication error");
			break;
		case JackNoSuchClient:
			CLIENT_FAILURE("unknown client type");
			break;
		case JackLoadFailure:
			CLIENT_FAILURE("can't load internal client");
			break;
		case JackInitFailure:
			CLIENT_FAILURE("can't initialize client");
			break;
		case JackShmFailure:
			CLIENT_FAILURE("unable to access shared memory");
			break;
		case JackVersionError:
			CLIENT_FAILURE("client/server protocol version mismatch");
		default:
			if (status) {
				ERRORLOG("Unknown status with JACK server.");
				if (client) {
					CLIENT_SUCCESS("Client pointer is *not* null..."
						       " assuming we're OK");
				}
			} else {
				CLIENT_SUCCESS("Connected to JACK server");
			}				
		}
	}

	if (client == 0) {
	    return -1;
	}

	// Here, client should either be valid, or NULL.	
	jack_server_sampleRate = jack_get_sample_rate ( client );
	jack_server_bufferSize = jack_get_buffer_size ( client );


	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
	jack_set_process_callback ( client, this->processCallback, 0 );


	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/
	jack_set_sample_rate_callback ( client, jackDriverSampleRate, 0 );

	/* tell JACK server to update us if the buffer size
	   (frames per process cycle) changes.
	*/
	jack_set_buffer_size_callback ( client, jackDriverBufferSize, 0 );

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown ( client, jackDriverShutdown, 0 );


	/* create two ports */
	output_port_1 = jack_port_register ( client, "out_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
	output_port_2 = jack_port_register ( client, "out_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

	if ( ( output_port_1 == NULL ) || ( output_port_2 == NULL ) ) {
		( Hydrogen::get_instance() )->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
		return 4;
	}


	// clear buffers
//	jack_default_audio_sample_t *out_L = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_1, jack_server_bufferSize);
//	jack_default_audio_sample_t *out_R = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_2, jack_server_bufferSize);
//	memset( out_L, 0, nBufferSize * sizeof( float ) );
//	memset( out_R, 0, nBufferSize * sizeof( float ) );

#ifdef LASH_SUPPORT
	if ( Preferences::get_instance()->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient->isConnected())
		{
		    lashClient->setJackClientName(sClientName.toLocal8Bit().constData());
		}
	}
#endif

	return 0;
}


/**
 * Make sure the number of track outputs match the instruments in @a song , and name the ports.
 */
void JackOutput::makeTrackOutputs( Song * song )
{

	/// Disable Track Outputs
	if( Preferences::get_instance()->m_bJackTrackOuts == false )
			return;
	///

	InstrumentList * instruments = song->get_instrument_list();
	Instrument * instr;
	int nInstruments = ( int )instruments->get_size();

	// create dedicated channel output ports
	WARNINGLOG( QString( "Creating / renaming %1 ports" ).arg( nInstruments ) );

	for ( int n = nInstruments - 1; n >= 0; n-- ) {
		instr = instruments->get( n );
		setTrackOutput( n, instr );
	}
	// clean up unused ports
	jack_port_t *p_L, *p_R;
	for ( int n = nInstruments; n < track_port_count; n++ ) {
		p_L = track_output_ports_L[n];
		p_R = track_output_ports_R[n];
		track_output_ports_L[n] = 0;
		jack_port_unregister( client, p_L );
		track_output_ports_R[n] = 0;
		jack_port_unregister( client, p_R );
	}

	track_port_count = nInstruments;
}

/**
 * Give the @a n 'th port the name of @a instr .
 * If the n'th port doesn't exist, new ports up to n are created.
 */
void JackOutput::setTrackOutput( int n, Instrument * instr )
{

	QString chName;

	if ( track_port_count <= n ) { // need to create more ports
		for ( int m = track_port_count; m <= n; m++ ) {
			chName = QString( "Track_%1_" ).arg( m + 1 );
			track_output_ports_L[m] = jack_port_register ( client, ( chName + "L" ).toLocal8Bit(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
			track_output_ports_R[m] = jack_port_register ( client, ( chName + "R" ).toLocal8Bit(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
			if ( track_output_ports_R[m] == NULL || track_output_ports_L[m] == NULL ) {
				Hydrogen::get_instance()->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}
		}
		track_port_count = n + 1;
	}
	// Now we're sure there is an n'th port, rename it.
	chName = QString( "Track_%1_%2_" ).arg( n + 1 ).arg( instr->get_name() );

	jack_port_set_name( track_output_ports_L[n], ( chName + "L" ).toLocal8Bit() );
	jack_port_set_name( track_output_ports_R[n], ( chName + "R" ).toLocal8Bit() );
}

void JackOutput::play()
{
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT || Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ) {
		if ( client ) {
			INFOLOG( "jack_transport_start()" );
			jack_transport_start( client );
		}
	} else {
		m_transport.m_status = TransportInfo::ROLLING;
	}
}



void JackOutput::stop()
{
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT || Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ) {
		if ( client ) {
			INFOLOG( "jack_transport_stop()" );
			jack_transport_stop( client );
		}
	} else {
		m_transport.m_status = TransportInfo::STOPPED;
	}
}



void JackOutput::locate( unsigned long nFrame )
{
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT /*|| Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER*/ ) {
		if ( client ) {
			WARNINGLOG( QString( "Calling jack_transport_locate(%1)" ).arg( nFrame ) );
			jack_transport_locate( client, nFrame );
		}
	} else {
		m_transport.m_nFrames = nFrame;
	}
}



void JackOutput::setBpm( float fBPM )
{
	WARNINGLOG( QString( "setBpm: %1" ).arg( fBPM ) );
	m_transport.m_nBPM = fBPM;
}


void JackOutput::setPortName( int nPort, bool bLeftChannel, const QString& sName )
{
//	infoLog( "[setPortName] " + sName );
	jack_port_t *pPort;
	if ( bLeftChannel ) {
		pPort = track_output_ports_L[ nPort ];
	} else {
		pPort = track_output_ports_R[ nPort ];
	}

	int err = jack_port_set_name( pPort, sName.toLocal8Bit() );
	if ( err != 0 ) {
		ERRORLOG( " Error in jack_port_set_name!" );
	}
}

int JackOutput::getNumTracks()
{
//	INFOLOG( "get num tracks()" );
	return track_port_count;
}


//beginn jack time master
void JackOutput::initTimeMaster()
{
	if ( client == NULL) return;

	bool cond = false;
	if ( Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER)
	{
		cond = true;
	}else{
		jack_release_timebase(client);
	}

	if ( Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER && 
                 jack_set_timebase_callback(client, cond, jack_timebase_callback, this) == 0)
	{
		Preferences::get_instance()->m_bJackMasterMode = Preferences::USE_JACK_TIME_MASTER ;
		cond = true;
	} else {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER ;
		cond = false;
	}
}


void JackOutput::com_release()
{
	if ( client == NULL) return;

	jack_release_timebase(client);
}


void JackOutput::jack_timebase_callback(jack_transport_state_t state,
					jack_nframes_t nframes,
              				jack_position_t *pos,
					int new_pos, void *arg)
{
	JackOutput *me = static_cast<JackOutput*>(arg);
	if(me) {
		me->jack_timebase_callback_impl(state, nframes, pos, new_pos);
	}
}


void JackOutput::jack_timebase_callback_impl(jack_transport_state_t
					     state, jack_nframes_t nframes,
                                             jack_position_t *pos, int
					     new_pos)
{
	Hydrogen * H = Hydrogen::get_instance();	

	//static double jack_tick;
	//static jack_nframes_t last_frame;
	static jack_nframes_t current_frame;
	static jack_transport_state_t state_current;
	static jack_transport_state_t state_last;
	
	
	state_current = state;
	
	current_frame = H->getTimeMasterFrames();
	nframes = current_frame;
	int posi =  H->getPatternPos();
	if (posi <= 0) posi=1;
	new_pos = posi ;

	
	pos->valid = JackPositionBBT;
	pos->beats_per_bar = H->getTickForHumanPosition( posi )/48;
	pos->beat_type = 4;
	pos->ticks_per_beat = (long)H->getTickForHumanPosition( posi ); 
	pos->beats_per_minute = H->getNewBpmJTM();



	int ticksforbeat = H->getTickForHumanPosition( posi );
	int beatperbar = H->getTickForHumanPosition( posi )/48 ;
	int ticker = (int)(H->getTickPosition()/ 48) ;
	int ptick = (int)(H->getTickPosition());
	int ptickmax = ptick * beatperbar;
	int ptickreal2 = ptick * beatperbar;

	if (ptickmax > ( ticksforbeat ))
	{
	ptickreal2 =  ptickmax -(ticksforbeat * ticker);
	}

	int ppos = H->getPatternPos() + 1;
	if (ppos == 0)
		ppos =1;
	pos->bar = ppos;
	pos->beat = ticker +1 ;
	pos->tick = ptickreal2 ;
	pos->bar_start_tick = pos->bar * pos->beats_per_bar *
		pos->ticks_per_beat;

	if (Hydrogen::get_instance()->getHumantimeFrames()<= 0){
		pos->bar = 1;
		pos->beat = 1;
		pos->tick = 0;
	}

	state_last = state_current;	
}

};

#endif // JACK_SUPPORT
