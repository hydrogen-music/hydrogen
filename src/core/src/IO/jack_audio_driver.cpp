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

#include <hydrogen/IO/JackAudioDriver.h>
#ifdef H2CORE_HAVE_JACK

#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/playlist.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/helpers/files.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>

#ifdef H2CORE_HAVE_LASH
#include <hydrogen/LashClient.h>
#endif

#ifdef H2CORE_HAVE_JACKSESSION
#include <jack/session.h>
#endif

namespace H2Core
{

unsigned long	jack_server_sampleRate = 0;
jack_nframes_t	jack_server_bufferSize = 0;
JackAudioDriver *	pJackDriverInstance = nullptr;

int jackDriverSampleRate( jack_nframes_t nframes, void *param )
{
	Object* __object = ( Object* )param;
	QString msg = QString("Jack SampleRate changed: the sample rate is now %1/sec").arg( QString::number( (int) nframes ) );
	__INFOLOG( msg );
	jack_server_sampleRate = nframes;
	return 0;
}

int jackDriverBufferSize( jack_nframes_t nframes, void * /*arg*/ )
{
	// This function does _NOT_ have to be realtime safe.
	jack_server_bufferSize = nframes;
	return 0;
}

void jackDriverShutdown( void *arg )
{
	UNUSED( arg );

	pJackDriverInstance->m_pClient = nullptr;
	Hydrogen::get_instance()->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}


const char* JackAudioDriver::__class_name = "JackAudioDriver";

JackAudioDriver::JackAudioDriver( JackProcessCallback processCallback )
	: AudioOutput( __class_name )
{
	INFOLOG( "INIT" );
	__track_out_enabled = Preferences::get_instance()->m_bJackTrackOuts;	// allow per-track output

	pJackDriverInstance = this;
	this->processCallback = processCallback;

	must_relocate = 0;
	locate_countdown = 0;
	bbt_frame_offset = 0;
	track_port_count = 0;

	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );
}

JackAudioDriver::~JackAudioDriver()
{
	INFOLOG( "DESTROY" );
	disconnect();
}

// return 0: ok
// return 1: cannot activate client
// return 2: cannot connect output port
// return 3: Jack server not running
// return 4: output port = NULL
int JackAudioDriver::connect()
{
	INFOLOG( "connect" );

	if ( jack_activate ( m_pClient ) ) {
		Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT );
		return 1;
	}


	bool connect_output_ports = m_bConnectOutFlag;

	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );

#ifdef H2CORE_HAVE_LASH
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
		// connect the ports
		if ( jack_connect( m_pClient, jack_port_name( output_port_1 ), output_port_name_1.toLocal8Bit() ) == 0 &&
			 jack_connect ( m_pClient, jack_port_name( output_port_2 ), output_port_name_2.toLocal8Bit() ) == 0 ) {
			return 0;
		}

		INFOLOG( "Could not connect so saved out-ports. Connecting to first pair of in-ports" );
		const char ** portnames = jack_get_ports ( m_pClient, NULL, NULL, JackPortIsInput );
		if ( !portnames || !portnames[0] || !portnames[1] ) {
			ERRORLOG( "Couldn't locate two Jack input ports" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		if ( jack_connect( m_pClient, jack_port_name( output_port_1 ), portnames[0] ) != 0 ||
			 jack_connect( m_pClient, jack_port_name( output_port_2 ), portnames[1] ) != 0 ) {
			ERRORLOG( "Couldn't connect to first pair of Jack input ports" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		free( portnames );
	}

	return 0;
}

void JackAudioDriver::disconnect()
{
	INFOLOG( "disconnect" );

	deactivate();
	jack_client_t *oldClient = m_pClient;
	m_pClient = NULL;
	if ( oldClient ) {
		INFOLOG( "calling jack_client_close" );
		int res = jack_client_close( oldClient );
		if ( res ) {
			ERRORLOG( "Error in jack_client_close" );
			// FIXME: raise exception
		}
	}
	m_pClient = NULL;
}

void JackAudioDriver::deactivate()
{
	INFOLOG( "[deactivate]" );
	if ( m_pClient ) {
		INFOLOG( "calling jack_deactivate" );
		int res = jack_deactivate( m_pClient );
		if ( res ) {
			ERRORLOG( "Error in jack_deactivate" );
		}
	}
	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );
}

unsigned JackAudioDriver::getBufferSize()
{
	return jack_server_bufferSize;
}

unsigned JackAudioDriver::getSampleRate()
{
	return jack_server_sampleRate;
}

void JackAudioDriver::calculateFrameOffset()
{
	bbt_frame_offset = m_JackTransportPos.frame - m_transport.m_nFrames;
}

int oldpo = 0;
//int changer = 0;

void JackAudioDriver::locateInNCycles( unsigned long frame, int cycles_to_wait )
{
	locate_countdown = cycles_to_wait;
	locate_frame = frame;
}

/// Take beat-bar-tick info from the Jack system, and translate it to a new internal frame position and ticksize.
void JackAudioDriver::relocateBBT()
{
	Preferences* pPref = Preferences::get_instance();

	//wolke if hydrogen is jack time master this is not relevant
	if ( m_transport.m_status != TransportInfo::ROLLING
	  || pPref->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER
	  || ! ( m_JackTransportPos.valid & JackPositionBBT )
	) {
		WARNINGLOG( "Relocate: Call it off" );
		return;
	}

	//WARNINGLOG( "Resyncing!" );
	INFOLOG( "..." );

	Hydrogen * H = Hydrogen::get_instance();
	Song * S = H->getSong();

	float hydrogen_TPB = ( float )( S->__resolution / m_JackTransportPos.beat_type * 4 );

	long bar_ticks = 0;
	//long beat_ticks = 0;
	if ( S->get_mode() == Song::SONG_MODE ) {
		// (Reasonable?) assumption that one pattern is _always_ 1 bar long!
		bar_ticks = H->getTickForPosition( m_JackTransportPos.bar-1  );
		// ignore error NOTE This is wrong -- if loop state is off, transport should just stop ??
		if ( bar_ticks < 0 ) bar_ticks = 0;
	}

	float hydrogen_ticks_to_locate = bar_ticks + ( m_JackTransportPos.beat-1 ) * hydrogen_TPB +
									 m_JackTransportPos.tick * ( hydrogen_TPB / m_JackTransportPos.ticks_per_beat );

	// INFOLOG( QString( "Position from Time Master: BBT [%1,%2,%3]" ) . arg( m_JackTransportPos.bar ) . arg( m_JackTransportPos.beat ) . arg( m_JackTransportPos.tick ) );
	// WARNINGLOG( QString(bbt) + " -- Tx/Beat = "+to_string(m_JackTransportPos.ticks_per_beat)+", Meter "+to_string(m_JackTransportPos.beats_per_bar)+"/"+to_string(m_JackTransportPos.beat_type)+" =>tick " + to_string( hydrogen_ticks_to_locate ) );

	float fNewTickSize = getSampleRate() * 60.0 /  m_transport.m_nBPM / S->__resolution;
	// not S->m_fBPM !??

	if ( fNewTickSize == 0 ) return; // ??!?

	// NOTE this _should_ prevent audioEngine_process_checkBPMChanged in Hydrogen.cpp from recalculating things.
	m_transport.m_nTickSize = fNewTickSize;

	long long nNewFrames = ( long long )( hydrogen_ticks_to_locate * fNewTickSize );

#ifndef JACK_NO_BBT_OFFSET
	if ( m_JackTransportPos.valid & JackBBTFrameOffset )
		nNewFrames += m_JackTransportPos.bbt_offset;
#endif

	m_transport.m_nFrames = nNewFrames;
}

///
/// When jack_transport_start() is called, it takes effect from the next processing cycle.
/// The location info from the timebase_master, if there is one, will not be available until the _next_ next cycle.
/// The code must therefore wait one cycle before syncing up with timebase_master.
///

void JackAudioDriver::updateTransportInfo()
{
		if ( locate_countdown == 1 )
				locate( locate_frame );
		if ( locate_countdown > 0 )
				locate_countdown--;

		if ( Preferences::get_instance()->m_bJackTransportMode !=  Preferences::USE_JACK_TRANSPORT   ) return;

				m_JackTransportState = jack_transport_query( m_pClient, &m_JackTransportPos );


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
		H->setTimelineBpm(); // dlr: fix #168, jack may have re-located us anywhere, check for bpm change every cycle

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
#if 0 // dlr: fix #169, why do we have a different behaviour for SONG_MODE?
					if ( m_pEngine->getSong()->get_mode() == Song::PATTERN_MODE  ){
						m_transport.m_nFrames = m_JackTransportPos.frame/* - bbt_frame_offset*/; ///see comment in svn changeset 753
					}
					else
					{
						m_transport.m_nFrames = H->getHumantimeFrames();
					}
#else
					m_transport.m_nFrames = m_JackTransportPos.frame;
					bbt_frame_offset = 0; // dlr: stop re-syncing in every cycle when STOPPED
#endif
					// In jack 'slave' mode, if there's no master, the following line is needed to be able to relocate by clicking the song ruler (wierd corner case, but still...)
					if ( m_transport.m_status == TransportInfo::ROLLING )
						H->triggerRelocateDuringPlay();
				} else {
					///this is experimantal... but it works for the moment... fix me fix :-) wolke
					// ... will this actually happen? keeping it for now ( jakob lund )
					m_transport.m_nFrames = H->getHumantimeFrames();
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

float* JackAudioDriver::getOut_L()
{
	jack_default_audio_sample_t *out = ( jack_default_audio_sample_t * ) jack_port_get_buffer ( output_port_1, jack_server_bufferSize );
	return out;
}

float* JackAudioDriver::getOut_R()
{
	jack_default_audio_sample_t *out = ( jack_default_audio_sample_t * ) jack_port_get_buffer ( output_port_2, jack_server_bufferSize );
	return out;
}

float* JackAudioDriver::getTrackOut_L( unsigned nTrack )
{
	if(nTrack > (unsigned)track_port_count ) return 0;
	jack_port_t *p = track_output_ports_L[nTrack];
	jack_default_audio_sample_t* out = 0;
	if( p ) {
		out = (jack_default_audio_sample_t*) jack_port_get_buffer( p, jack_server_bufferSize);
	}
	return out;
}

float* JackAudioDriver::getTrackOut_R( unsigned nTrack )
{
	if(nTrack > (unsigned)track_port_count ) return 0;
	jack_port_t *p = track_output_ports_R[nTrack];
	jack_default_audio_sample_t* out = 0;
	if( p ) {
		out = (jack_default_audio_sample_t*) jack_port_get_buffer( p, jack_server_bufferSize);
	}
	return out;
}

float* JackAudioDriver::getTrackOut_L( Instrument * instr, InstrumentComponent * pCompo)
{
	return getTrackOut_L(track_map[instr->get_id()][pCompo->get_drumkit_componentID()]);
}

float* JackAudioDriver::getTrackOut_R( Instrument * instr, InstrumentComponent * pCompo)
{
	return getTrackOut_R(track_map[instr->get_id()][pCompo->get_drumkit_componentID()]);
}


#define CLIENT_FAILURE(msg) {						\
	ERRORLOG("Could not connect to JACK server (" msg ")"); \
	if (m_pClient) {						\
	ERRORLOG("...but JACK returned a non-null pointer?"); \
	(m_pClient) = nullptr;					\
}							\
	if (nTries) ERRORLOG("...trying again.");		\
}


#define CLIENT_SUCCESS(msg) {				\
	assert(m_pClient);				\
	INFOLOG(msg);				\
	nTries = 0;				\
}

int JackAudioDriver::init( unsigned /*nBufferSize*/ )
{
	Preferences* pPref = Preferences::get_instance();
	output_port_name_1 = pPref->m_sJackPortName1;
	output_port_name_2 = pPref->m_sJackPortName2;

	QString sClientName = "Hydrogen";

#ifdef H2CORE_HAVE_OSC
	QString sNsmClientId = pPref->getNsmClientId();

	if(!sNsmClientId.isEmpty()){
		sClientName = sNsmClientId;
	}
#endif


	jack_status_t status;
	int nTries = 2;  // Sometimes jackd doesn't stop and start fast enough.
	while ( nTries > 0 ) {
		--nTries;

#ifdef H2CORE_HAVE_JACKSESSION
		if (pPref->getJackSessionUUID().isEmpty()){
			m_pClient = jack_client_open(
						 sClientName.toLocal8Bit(),
						 JackNullOption,
						 &status);
		} else {
			const QByteArray uuid = pPref->getJackSessionUUID().toLocal8Bit();
			m_pClient = jack_client_open(
						 sClientName.toLocal8Bit(),
						 JackSessionID,
						 &status,
						 uuid.constData());
		}
#else
		m_pClient = jack_client_open(
					 sClientName.toLocal8Bit(),
					 JackNullOption,
					 &status);
#endif
		switch(status) {
			case JackFailure:
				CLIENT_FAILURE("unknown error");
				break;
			case JackInvalidOption:
				CLIENT_FAILURE("invalid option");
				break;
			case JackNameNotUnique:
				if (m_pClient) {
					sClientName = jack_get_client_name(m_pClient);
					CLIENT_SUCCESS(QString("Jack assigned the client name '%1'").arg(sClientName));
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
				break;
			default:
				if (status) {
					ERRORLOG("Unknown status with JACK server.");
					if (m_pClient) {
						CLIENT_SUCCESS("Client pointer is *not* null..."
									   " assuming we're OK");
					}
				} else {
					CLIENT_SUCCESS("Connected to JACK server");
				}
		}
	}

	if (m_pClient == 0) return -1;

	// Here, client should either be valid, or NULL.
	jack_server_sampleRate = jack_get_sample_rate ( m_pClient );
	jack_server_bufferSize = jack_get_buffer_size ( m_pClient );

	pPref->m_nSampleRate = jack_server_sampleRate;
	pPref->m_nBufferSize = jack_server_bufferSize;

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
	jack_set_process_callback ( m_pClient, this->processCallback, 0 );

	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/
	jack_set_sample_rate_callback ( m_pClient, jackDriverSampleRate, this );

	/* tell JACK server to update us if the buffer size
	   (frames per process cycle) changes.
	*/
	jack_set_buffer_size_callback ( m_pClient, jackDriverBufferSize, 0 );

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown ( m_pClient, jackDriverShutdown, 0 );

	/* create two ports */
	output_port_1 = jack_port_register ( m_pClient, "out_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
	output_port_2 = jack_port_register ( m_pClient, "out_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

	Hydrogen *pEngine = Hydrogen::get_instance();
	if ( ( output_port_1 == NULL ) || ( output_port_2 == NULL ) ) {
		pEngine->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
		return 4;
	}

#ifdef H2CORE_HAVE_LASH
	if ( pPref->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient->isConnected()) {
			lashClient->setJackClientName(sClientName.toLocal8Bit().constData());
		}
	}
#endif

#ifdef H2CORE_HAVE_JACKSESSION
	jack_set_session_callback (m_pClient, jack_session_callback, (void*)this);
#endif

	if ( pPref->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER )
		initTimeMaster();

	return 0;
}

/**
 * Make sure the number of track outputs match the instruments in @a song , and name the ports.
 */
void JackAudioDriver::makeTrackOutputs( Song * pSong )
{

	/// Disable Track Outputs
	if( Preferences::get_instance()->m_bJackTrackOuts == false )
		return;
	///

	InstrumentList * pInstruments = pSong->get_instrument_list();
	Instrument * pInstr;
	int nInstruments = ( int ) pInstruments->size();

	// create dedicated channel output ports
	WARNINGLOG( QString( "Creating / renaming %1 ports" ).arg( nInstruments ) );

	int nTrackCount = 0;

	for( int i = 0 ; i < MAX_INSTRUMENTS ; i++ ){
		for ( int j = 0 ; j < MAX_COMPONENTS ; j++ ){
			track_map[i][j] = 0;
		}
	}
	
	for ( int n = 0; n <= nInstruments - 1; n++ ) {
		pInstr = pInstruments->get( n );
		for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			InstrumentComponent* pCompo = *it;
			setTrackOutput( nTrackCount, pInstr , pCompo, pSong);
			track_map[pInstr->get_id()][pCompo->get_drumkit_componentID()] = nTrackCount;
			nTrackCount++;
		}
	}
	// clean up unused ports
	jack_port_t *p_L, *p_R;
	for ( int n = nTrackCount; n < track_port_count; n++ ) {
		p_L = track_output_ports_L[n];
		p_R = track_output_ports_R[n];
		track_output_ports_L[n] = 0;
		jack_port_unregister( m_pClient, p_L );
		track_output_ports_R[n] = 0;
		jack_port_unregister( m_pClient, p_R );
	}

	track_port_count = nTrackCount;
}

/**
 * Give the @a n 'th port the name of @a instr .
 * If the n'th port doesn't exist, new ports up to n are created.
 */
void JackAudioDriver::setTrackOutput( int n, Instrument * instr, InstrumentComponent * compo, Song * song )
{
	QString chName;

	if ( track_port_count <= n ) { // need to create more ports
		for ( int m = track_port_count; m <= n; m++ ) {
			chName = QString( "Track_%1_" ).arg( m + 1 );
			track_output_ports_L[m] = jack_port_register ( m_pClient, ( chName + "L" ).toLocal8Bit(),
														   JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

			track_output_ports_R[m] = jack_port_register ( m_pClient, ( chName + "R" ).toLocal8Bit(),
														   JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

			if ( ! track_output_ports_R[m] || ! track_output_ports_L[m] ) {
				Hydrogen::get_instance()->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}
		}
		track_port_count = n + 1;
	}

	// Now we're sure there is an n'th port, rename it.
	DrumkitComponent* pDrumkitComponent = song->get_component( compo->get_drumkit_componentID() );
	chName = QString( "Track_%1_%2_%3_" ).arg( n + 1 ).arg( instr->get_name() ).arg( pDrumkitComponent->get_name() );

#ifdef HAVE_JACK_PORT_RENAME
	jack_port_rename( m_pClient, track_output_ports_L[n], ( chName + "L" ).toLocal8Bit() );
	jack_port_rename( m_pClient, track_output_ports_R[n], ( chName + "R" ).toLocal8Bit() );
#else
	jack_port_set_name( track_output_ports_L[n], ( chName + "L" ).toLocal8Bit() );
	jack_port_set_name( track_output_ports_R[n], ( chName + "R" ).toLocal8Bit() );
#endif
}

void JackAudioDriver::play()
{
	Preferences* P = Preferences::get_instance();
	if ( P->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT ||
		 P->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER
		 ) {
		if ( m_pClient ) {
			INFOLOG( "jack_transport_start()" );
			jack_transport_start( m_pClient );
		}
	} else {
		m_transport.m_status = TransportInfo::ROLLING;
	}
}

void JackAudioDriver::stop()
{
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT || Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ) {
		if ( m_pClient ) {
			INFOLOG( "jack_transport_stop()" );
			jack_transport_stop( m_pClient );
		}
	} else {
		m_transport.m_status = TransportInfo::STOPPED;
	}
}

void JackAudioDriver::locate( unsigned long nFrame )
{
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT /*|| Preferences::get_instance()->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER*/ ) {
		if ( m_pClient ) {
			WARNINGLOG( QString( "Calling jack_transport_locate(%1)" ).arg( nFrame ) );
			jack_transport_locate( m_pClient, nFrame );
		}
	} else {
		m_transport.m_nFrames = nFrame;
	}
}

void JackAudioDriver::setBpm( float fBPM )
{
	WARNINGLOG( QString( "setBpm: %1" ).arg( fBPM ) );
	m_transport.m_nBPM = fBPM;
}

int JackAudioDriver::getNumTracks()
{
	//	INFOLOG( "get num tracks()" );
	return track_port_count;
}

#ifdef H2CORE_HAVE_JACKSESSION
void JackAudioDriver::jack_session_callback(jack_session_event_t *event, void *arg)
{
	JackAudioDriver *me = static_cast<JackAudioDriver*>(arg);
	if(me) me->jack_session_callback_impl(event);
}

static QString baseName ( QString path ) {
	return QFileInfo( path ).fileName();
}

void JackAudioDriver::jack_session_callback_impl(jack_session_event_t *event)
{
	INFOLOG("jack session calback");
	enum session_events{
		SAVE_SESSION,
		SAVE_AND_QUIT,
		SAVE_TEMPLATE
	};

	Hydrogen* H = Hydrogen::get_instance();
	Song* S = H->getSong();
	Preferences* P = Preferences::get_instance();
	EventQueue* EQ = EventQueue::get_instance();

	jack_session_event_t *ev = (jack_session_event_t *) event;

	QString jackSessionDirectory = (QString) ev->session_dir;
	QString retval = P->getJackSessionApplicationPath() + " --jacksessionid " + ev->client_uuid;

	/* Playlist mode */
	Playlist* playlist = Playlist::get_instance();
	if ( playlist->size() > 0 ) {

		if ( playlist->getFilename().isEmpty() ) playlist->setFilename( Filesystem::untitled_playlist_file_name() );

		QString FileName = baseName ( playlist->getFilename() );
		FileName.replace ( QString(" "), QString("_") );
		retval += " -p \"${SESSION_DIR}" + FileName + "\"";

		/* Copy all songs to Session Directory and update playlist */
		SongReader reader;
		for ( uint i = 0; i < playlist->size(); ++i ) {
			QString BaseName = baseName ( playlist->get( i )->m_hFile );
			QString newName = jackSessionDirectory + BaseName;
			QString SongPath = reader.getPath ( playlist->get( i )->m_hFile );
			if ( SongPath != NULL && QFile::copy ( SongPath, newName ) ) {
				/* Keep only filename on list for relative read */
				playlist->get( i )->m_hFile = BaseName;
				// playlist->get( i )->m_hScript;
			} else {
				/* Note - we leave old path in playlist */
				ERRORLOG ( "Can't copy " + playlist->get( i )->m_hFile + " to " + newName );
				ev->flags = JackSessionSaveError;
			}
		}

		/* Save updated playlist */
		bool relativePaths = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
		if ( Files::savePlaylistPath( jackSessionDirectory + FileName, playlist, relativePaths ) == NULL ) {
			ev->flags = JackSessionSaveError;
		}
		/* Song Mode */
	} else {
		/* Valid Song is needed */
		if ( S->get_filename().isEmpty() ) S->set_filename( Filesystem::untitled_song_file_name() );

		QString FileName = baseName ( S->get_filename() );
		FileName.replace ( QString(" "), QString("_") );
		S->set_filename(jackSessionDirectory + FileName);

		/* SongReader will look into SESSION DIR anyway */
		retval += " -s \"" + FileName + "\"";

		switch (ev->type) {
			case JackSessionSave:
				EQ->push_event(EVENT_JACK_SESSION, SAVE_SESSION);
				break;
			case JackSessionSaveAndQuit:
				EQ->push_event(EVENT_JACK_SESSION, SAVE_SESSION);
				EQ->push_event(EVENT_JACK_SESSION, SAVE_AND_QUIT);
				break;
			default:
				ERRORLOG( "JackSession: Unknown event type" );
				ev->flags = JackSessionSaveError;
		}
	}

	ev->command_line = strdup( retval.toUtf8().constData() );
	jack_session_reply (m_pClient, ev );
	jack_session_event_free (ev);
}
#endif

//beginn jack time master
void JackAudioDriver::initTimeMaster()
{
	if ( ! m_pClient ) return;

	Preferences* pref = Preferences::get_instance();
	if ( pref->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER) {
		int ret = jack_set_timebase_callback(m_pClient, m_bCond, jack_timebase_callback, this);
		if (ret != 0) pref->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
	} else {
		jack_release_timebase(m_pClient);
	}
}

void JackAudioDriver::com_release()
{
	if ( m_pClient == NULL) return;

	jack_release_timebase(m_pClient);
}

void JackAudioDriver::jack_timebase_callback(jack_transport_state_t state,
										jack_nframes_t nframes,
										jack_position_t *pos,
										int new_pos,
										void *arg)
{
	JackAudioDriver *me = static_cast<JackAudioDriver*>(arg);
	if (! me) return;

	Hydrogen * H = Hydrogen::get_instance();
	Song* S = H->getSong();
	if ( ! S ) return;

	unsigned long PlayTick = ( pos->frame - me->bbt_frame_offset ) / me->m_transport.m_nTickSize;
	pos->bar = H->getPosForTick ( PlayTick );

	double TPB = H->getTickForHumanPosition( pos->bar );
	if ( TPB < 1 ) return;

	/* We'll cheat there is ticks_per_beat * 4 in bar
	   so every Hydrogen tick will be multipled by 4 ticks */
	pos->ticks_per_beat = TPB;
	pos->valid = JackPositionBBT;
	pos->beats_per_bar = TPB / 48;
	pos->beat_type = 4.0;
	pos->beats_per_minute = H->getTimelineBpm ( pos->bar );
	pos->bar++;

	// Probably there will never be an offset, cause we are the master ;-)
#ifndef JACK_NO_BBT_OFFSET
	pos->valid = static_cast<jack_position_bits_t> ( pos->valid | JackBBTFrameOffset );
	pos->bbt_offset = 0;
#endif

	if (H->getHumantimeFrames() < 1) {
		pos->beat = 1;
		pos->tick = 0;
		pos->bar_start_tick = 0;
	} else {
		/* how many ticks elpased from last bar ( where bar == pattern ) */
		int32_t TicksFromBar = ( PlayTick % (int32_t) pos->ticks_per_beat ) * 4;

		pos->bar_start_tick = PlayTick - TicksFromBar;

		pos->beat = TicksFromBar / pos->ticks_per_beat;
		pos->beat++;

		pos->tick = TicksFromBar % (int32_t) pos->ticks_per_beat;
#if 0
//		printf ( "\e[0K\rBar %d, Beat %d, Tick %d, BPB %g, BarStartTick %g",
		printf ( "Bar %d, Beat %d, Tick %d, BPB %g, BarStartTick %g\n",
			pos->bar, pos->beat,pos->tick, pos->beats_per_bar, pos->bar_start_tick );
#endif
	}
}

}

#endif // H2CORE_HAVE_JACK
