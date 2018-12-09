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

#include <hydrogen/IO/jack_audio_driver.h>
#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_

#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <hydrogen/hydrogen.h>
#include <hydrogen/audio_engine.h>
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

namespace H2Core {

/**
 * Sample rate of the JACK audio server.
 *
 * It is set by the callback function jackDriverSampleRate()
 * registered in the JACK server and accessed via
 * JackAudioDriver::getSampleRate(). Its initialization is handled by
 * JackAudioDriver::init(), which sets it to the sample rate of the
 * Hydrogen's external JACK client via _jack_get_sample_rate()_
 * (jack/jack.h). 
 */
unsigned long		jack_server_sampleRate = 0;
/**
 * Buffer size of the JACK audio server.
 *
 * It is set by the callback function jackDriverBufferSize()
 * registered in the JACK server and accessed via
 * JackAudioDriver::getBufferSize(). Its initialization is handled by
 * JackAudioDriver::init(), which sets it to the buffer size of the
 * Hydrogen's external JACK client via _jack_get_buffer_size()_
 * (jack/jack.h). 
 */
jack_nframes_t		jack_server_bufferSize = 0;
/**
 * Instance of the JackAudioDriver.
 */
JackAudioDriver *	pJackDriverInstance = nullptr;


/**
 * Callback function for the JACK audio server to set the sample rate
 * #H2Core::jack_server_sampleRate and prints a message to
 * the #__INFOLOG, which has to be included via a Logger instance in
 * the provided @a param.
 *
 * It gets registered as a callback function of the JACK server in
 * JackAudioDriver::init() using _jack_set_sample_rate_callback()_.
 *
 * \param nframes New sample rate. The object has to be of type
 * _jack_nframes_t_, which is defined in the jack/types.h header.
 * \param param Object containing a Logger member to display the
 * change in the sample rate in its INFOLOG.
 *
 * @return 0 on success
 */
int jackDriverSampleRate( jack_nframes_t nframes, void* param ){
	// Used for logging.
	Object* __object = ( Object* )param;
	QString msg = QString("Jack SampleRate changed: the sample rate is now %1/sec").arg( QString::number( (int) nframes ) );
	// The __INFOLOG macro uses the Object *__object and not the
	// Object instance as INFOLOG does. It will call
	// __object->logger()->log( H2Core::Logger::Info, ..., msg )
	// (see object.h).
	__INFOLOG( msg );
	jack_server_sampleRate = nframes;
	return 0;
}
/**
 * Callback function for the JACK audio server to set the buffer size
 * #H2Core::jack_server_bufferSize.
 *
 * It gets registered as a callback function of the JACK server in
 * JackAudioDriver::init() using _jack_set_buffer_size_callback()_.
 *
 * \param nframes New buffer size. The object has to be of type @a
 * jack_nframes_t, which is defined in the jack/types.h header.
 * \param arg Not used within the function but kept for compatibility
 * reasons since the _JackBufferSizeCallback_ (jack/types.h) requires a
 * second input argument @a arg of type _void_, which is a pointer
 * supplied by the jack_set_buffer_size_callback() function.
 *
 * @return 0 on success
 */
int jackDriverBufferSize( jack_nframes_t nframes, void* arg ){
	// This function does _NOT_ have to be realtime safe.
	jack_server_bufferSize = nframes;
	return 0;
}
/**
 * Callback function for the JACK audio server to shutting down the
 * JACK driver.
 *
 * The JackAudioDriver::m_pClient pointer stored in the current
 * instance of the JACK audio driver #pJackDriverInstance is set to
 * the nullptr and a Hydrogen::JACK_SERVER_SHUTDOWN error is raised
 * using Hydrogen::raiseError().
 *
 * It gets registered as a callback function of the JACK server in
 * JackAudioDriver::init() using _jack_on_shutdown()_.
 *
 * \param arg Not used within the function but kept for compatibility
 * reasons since _jack_shutdown()_ (jack/jack.h) the argument @a arg
 * of type void.
 */	
void jackDriverShutdown( void* arg )
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
	// __track_out_enabled is inherited from AudioOutput and
	// instantiated with false. It will be used by the Sampler and
	// Hydrogen itself to check whether JackAudioDriver is ordered
	// to create per-track audio output ports.
	__track_out_enabled = Preferences::get_instance()->m_bJackTrackOuts;

	pJackDriverInstance = this;
	this->processCallback = processCallback;

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
int JackAudioDriver::connect()
{
	INFOLOG( "connect" );

	// The `jack_activate' function is defined in the jack/jack.h
	// header files and tells the JACK server that the program is
	// ready to start processing audio. It returns 0 on success
	// and a non-zero error code otherwise.
	if ( jack_activate( m_pClient ) ) {
		Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT );
		return 1;
	}

	bool connect_output_ports = m_bConnectOutFlag;

	memset( track_output_ports_L, 0, sizeof(track_output_ports_L) );
	memset( track_output_ports_R, 0, sizeof(track_output_ports_R) );

#ifdef H2CORE_HAVE_LASH
	if ( Preferences::get_instance()->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient && lashClient->isConnected()){
			// INFOLOG( "[LASH] Sending JACK client name to LASH server" );
			lashClient->sendJackClientName();

			if (!lashClient->isNewProject()){
				connect_output_ports = false;
			}
		}
	}
#endif

	if ( connect_output_ports ) {
		// Connect the ports.
		// The `jack_connect' function is defined in the
		// jack/jack.h file. It establishes a connection between
		// two ports. When a connection exists, data written
		// to the source port will be available to be read at
		// the destination port. Returns 0 on success, exits
		// if the connection is already made, and returns a
		// non-zero error code otherwise.
		// Syntax: jack_connect( jack_client_t jack_client,
		//                       const char *source_port )
		//                       const char *destination_port
		// )
		// The `jack_port_name' function is also defined in
		// the jack/jack.h header returns the full name of a
		// provided port of type jack_port_t.
		if ( jack_connect( m_pClient, jack_port_name( output_port_1 ),
				   output_port_name_1.toLocal8Bit() ) == 0 &&
		     jack_connect( m_pClient, jack_port_name( output_port_2 ),
				   output_port_name_2.toLocal8Bit() ) == 0 ) {
			return 0;
		}

		INFOLOG( "Could not connect to the saved output ports. Connect to the first pair of input ports instead." );
		// The `jack_get_ports' is defined in the jack/jack.h
		// header file and performs a lookup of ports of the
		// JACK server based on their e.g. flags. It returns a
		// NULL-terminated array of ports that match the
		// specified arguments. The caller is responsible for
		// calling jack_free() any non-NULL returned
		// value.
		const char ** portnames = jack_get_ports( m_pClient, nullptr, nullptr, JackPortIsInput );
		if ( !portnames || !portnames[0] || !portnames[1] ) {
			ERRORLOG( "Couldn't locate two Jack input ports" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		if ( jack_connect( m_pClient, jack_port_name( output_port_1 ),
				   portnames[0] ) != 0 ||
		     jack_connect( m_pClient, jack_port_name( output_port_2 ),
				   portnames[1] ) != 0 ) {
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
	m_pClient = nullptr;
	if ( oldClient ) {
		INFOLOG( "calling jack_client_close" );
		int res = jack_client_close( oldClient );
		if ( res ) {
			ERRORLOG( "Error in jack_client_close" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CLOSE_CLIENT );
		}
	}
	m_pClient = nullptr;
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
	INFOLOG( QString( "bbt_frame_offset: %1" ). arg( bbt_frame_offset ) );
}

void JackAudioDriver::locateInNCycles( unsigned long frame, int cycles_to_wait )
{
	locate_countdown = cycles_to_wait;
	locate_frame = frame;
}

void JackAudioDriver::updateTransportInfo()
{
	// The following four lines do only cover the special case of
	// audioEngine_updateNoteQueue() returning -1 in
	// audioEngine_process() and triggering an addition relocation
	// of the transport position to the beginning of the song
	// using locateInNCycles() to possibly keep synchronization
	// with Ardour.
	if ( locate_countdown == 1 )
		locate( locate_frame );
	if ( locate_countdown > 0 )
		locate_countdown--;
	
	if ( Preferences::get_instance()->m_bJackTransportMode !=
	     Preferences::USE_JACK_TRANSPORT ){
		return;
	}
	// jack_transport_query() (jack/transport.h) queries the
	// current transport state and position. If called from the
	// process thread, the second argument, which is a pointer to
	// a structure for returning current transport, corresponds to
	// the first frame of the current cycle and the state returned
	// is valid for the entire cycle. #m_JackTransportPos->valid
	// will show which fields contain valid data. If
	// #m_JackTransportPos is NULL, do not return position
	// information.
	m_JackTransportState = jack_transport_query( m_pClient, &m_JackTransportPos );

	// WARNINGLOG( QString( "[Jack-Query] state: %1, frame: %2, position bit: %3, bpm: %4" )
	// 	 .arg( m_JackTransportState )
	// 	 .arg( m_JackTransportPos.frame )
	// 	 .arg( m_JackTransportPos.valid )
	// 	 .arg( m_JackTransportPos.beats_per_minute ) );
	// Update the TransportInfo in m_transport based on the state
	// returned by jack_transport_query.
	switch ( m_JackTransportState ) {
	case JackTransportStopped: // Transport is halted
		m_transport.m_status = TransportInfo::STOPPED;
		//INFOLOG( "[updateTransportInfo] STOPPED - frames: " + to_string(m_transportPos.frame) );
		break;
		
	case JackTransportRolling: // Transport is playing
		m_transport.m_status = TransportInfo::ROLLING;
		//INFOLog( "[updateTransportInfo] ROLLING - frames: " + to_string(m_transportPos.frame) );
		break;

	case JackTransportStarting: // Waiting for sync ready. If
				    // there are slow-sync clients,
				    // this can take more than one
				    // cycle.
		m_transport.m_status = TransportInfo::STOPPED;
		//INFOLOG( "[updateTransportInfo] STARTING (stopped) - frames: " + to_string(m_transportPos.frame) );
		break;
		
	default:
		ERRORLOG( "Unknown jack transport state" );
	}

	// Updating TickSize and BPM
	Hydrogen * H = Hydrogen::get_instance();
	// JACK may have re-located us anywhere. Therefore, we have to
	// check for a bpm change every cycle. We will do so by
	// updating both the global speed of the song, as well as the
	// fallback speed (H->getNewBpmJTM) with the local tempo at
	// the current position on the timeline.
	H->setTimelineBpm(); 

	// Check whether another JACK master is present (the second if
	// clause won't evaluate to true if Hydrogen itself is the
	// timebase master) and whether it changed the transport
	// state. Note that only the JACK timebase master and not
	// arbitrary clients can do this. If so, the speed is updated
	// an a relocation according to the bar, beat, and tick
	// information will be triggered right away.
	if ( m_JackTransportPos.valid & JackPositionBBT ) {
		float bpm = ( float )m_JackTransportPos.beats_per_minute;
		if ( m_transport.m_nBPM != bpm ) {
			if ( Preferences::get_instance()->m_bJackMasterMode ==
			     Preferences::NO_JACK_TIME_MASTER ){
					// The speed of the Song will be updated by function
					// calling update_transport_info()
					// audioEngine_process_transport().
					m_transport.m_nBPM = bpm;

					// The tick size will be updated by the
					// audioEngine_process_checkBPMChanged()
					// function afterwards.
			}
		}
	}

        // This clause detects a re-location, which was either
        // triggered by an user-interaction (e.g. clicking the forward
        // button or clicking somewhere on the timeline) or by a
        // different JACK client.
	if ( m_transport.m_nFrames + bbt_frame_offset != m_JackTransportPos.frame ) {
		INFOLOG( "A relocation took place" );
		if ( ( m_JackTransportPos.valid & JackPositionBBT ) ){
			// There is a JACK timebase master.
			if ( !m_bHydrogenIsJackTimebaseMaster ){
				// But it's not us.
				// Absorbing the current bbt_frame_offset value.
				m_transport.m_nFrames = m_JackTransportPos.frame *
					m_fOldTickSize/ m_transport.m_nTickSize;
				bbt_frame_offset = 0;
				// INFOLOG( QString( "External JACK timebase master. Resetting frame from %1 to %2 using the old tick size %3 and new one %4" )
				// 	 .arg( m_transport.m_nFrames )
				// 	 .arg( m_JackTransportPos.frame )
				// 	 .arg( m_fOldTickSize )
				// 	 .arg( m_transport.m_nTickSize ) );
			} else {
				// INFOLOG( QString( "Hydrogen as JACK timebase master. Possible mismatch between transport %1, client %2, bbt_frame_offset %3" )
				// 	 .arg( m_transport.m_nFrames )
				// 	 .arg( m_JackTransportPos.frame )
				// 	 .arg( bbt_frame_offset ) );
				// We are timebase master ourselves.
				// Resetting the offset.
				bbt_frame_offset = 0;
				// We do not update the position in frames directly.
				// Instead, the JACK server is asked to relocate
				// and we use its current location.
				m_transport.m_nFrames = m_JackTransportPos.frame;
			}
		} else {
			// Only normal JACK clients connected
			// to the server.
			// INFOLOG( "Relocation as normal JACK client" );
			m_transport.m_nFrames = m_JackTransportPos.frame;
			bbt_frame_offset = 0;
			if ( m_transport.m_status == TransportInfo::ROLLING )
				H->triggerRelocateDuringPlay();
		}
	}
	// Only used if Hydrogen is in JACK timebase master
	// mode. Updated every cycle.
	if ( H->getHumantimeFrames() != m_JackTransportPos.frame ) {
		H->setHumantimeFrames(m_JackTransportPos.frame);
		// WARNINGLOG( QString( "fix Humantime: %1" ).arg( m_JackTransportPos.frame ) );
	}
}

float* JackAudioDriver::getOut_L()
{
	/**
	 * This returns a pointer to the memory area associated with
	 * the specified port. For an output port, it will be a memory
	 * area that can be written to; for an input port, it will be
	 * an area containing the data from the port's connection(s),
	 * or zero-filled. if there are multiple inbound connections,
	 * the data will be mixed appropriately.
	 */
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
	if(nTrack > (unsigned)track_port_count ) return nullptr;
	jack_port_t *p = track_output_ports_L[nTrack];
	jack_default_audio_sample_t* out = nullptr;
	if( p ) {
		out = (jack_default_audio_sample_t*) jack_port_get_buffer( p, jack_server_bufferSize);
	}
	return out;
}

float* JackAudioDriver::getTrackOut_R( unsigned nTrack )
{
	if(nTrack > (unsigned)track_port_count ) return nullptr;
	jack_port_t *p = track_output_ports_R[nTrack];
	jack_default_audio_sample_t* out = nullptr;
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
	ERRORLOG("Could not connect to JACK server (" msg ")"); 	\
	if (m_pClient) {						\
		ERRORLOG("...but JACK returned a non-null pointer?"); 	\
		(m_pClient) = nullptr;					\
	}								\
	if (nTries) ERRORLOG("...trying again.");			\
}


#define CLIENT_SUCCESS(msg) {						\
	assert(m_pClient);						\
	INFOLOG(msg);							\
	nTries = 0;							\
}

int JackAudioDriver::init( unsigned bufferSize )
{
	// Destination ports the output of Hydrogen will be connected
	// to.
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

	// The address of the status object will be used by JACK to
	// return information from the open operation.
	jack_status_t status;
	// Sometimes jackd doesn't stop and start fast enough.
	int nTries = 2;
	while ( nTries > 0 ) {
		--nTries;

		// Open an external client session with the JACK
		// server.  The `jack_client_open' function is defined
		// in the jack/jack.h header. With it, clients may
		// choose which of several servers to connect, and
		// control whether and how to start the server
		// automatically, if it was not already running. Its
		// first argument _client_name_ of is at most
		// jack_client_name_size() characters. The name scope
		// is local to each server. Unless forbidden by the
		// JackUseExactName option, the server will modify
		// this name to create a unique variant, if
		// needed. The second argument _options_ is formed by
		// OR-ing together JackOptions bits. Only the
		// JackOpenOptions bits are allowed. _status_ (if
		// non-NULL) is an address for JACK to return
		// information from the open operation. This status
		// word is formed by OR-ing together the relevant
		// JackStatus bits.  Depending on the _status_, an
		// optional argument _server_name_ selects from among
		// several possible concurrent server
		// instances. Server names are unique to each user. It
		// returns an opaque client handle if successful. If
		// this is NULL, the open operation failed, *status
		// includes JackFailure and the caller is not a JACK
		// client.
#ifdef H2CORE_HAVE_JACKSESSION
		if (pPref->getJackSessionUUID().isEmpty()){
			m_pClient = jack_client_open( sClientName.toLocal8Bit(),
						      JackNullOption,
						      &status);
		} else {
			// Unique name of the JACK server used within
			// the JACK session.
			const QByteArray uuid = pPref->getJackSessionUUID().toLocal8Bit();
			// Using the JackSessionID option and the
			// supplied SessionID Token the sessionmanager
			// is able to identify the client again.
			m_pClient = jack_client_open( sClientName.toLocal8Bit(),
						      JackSessionID,
						      &status,
						      uuid.constData());
		}
#else
		m_pClient = jack_client_open( sClientName.toLocal8Bit(),
					      JackNullOption,
					      &status);
#endif
		// Check what did happen during the opening of the
		// client. CLIENT_SUCCESS sets the nTries variable
		// to 0 while CLIENT_FAILURE resets m_pClient to the
		// nullptr.
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

	if (m_pClient == nullptr) return -1;

	// Here, client should either be valid, or NULL.
	jack_server_sampleRate = jack_get_sample_rate( m_pClient );
	jack_server_bufferSize = jack_get_buffer_size( m_pClient );

	pPref->m_nSampleRate = jack_server_sampleRate;
	pPref->m_nBufferSize = jack_server_bufferSize;

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
	jack_set_process_callback( m_pClient, this->processCallback, nullptr );

	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/
	jack_set_sample_rate_callback( m_pClient, jackDriverSampleRate, this );

	/* tell JACK server to update us if the buffer size
	   (frames per process cycle) changes.
	*/
	jack_set_buffer_size_callback( m_pClient, jackDriverBufferSize, nullptr );

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown( m_pClient, jackDriverShutdown, nullptr );

	// Create two new ports for Hydrogen's client. These are
	// objects used for moving data of any type in or out of the
	// client. Ports may be connected in various ways. The
	// function `jack_port_register' (jack/jack.h) is called like
	// jack_port_register( jack_client_t *client, 
	//                     const char *port_name,
        //                     const char *port_type,
        //                     unsigned long flags,
        //                     unsigned long buffer_size)
	//
	// All ports have a type, which may be any non-NULL and non-zero
	// length string, passed as an argument. Some port types are built
	// into the JACK API, currently only JACK_DEFAULT_AUDIO_TYPE.
	// It returns a _jack_port_t_ pointer on success, otherwise NULL.
	output_port_1 = jack_port_register( m_pClient, "out_L", JACK_DEFAULT_AUDIO_TYPE,
					    JackPortIsOutput, 0 );
	output_port_2 = jack_port_register( m_pClient, "out_R", JACK_DEFAULT_AUDIO_TYPE,
					    JackPortIsOutput, 0 );

	Hydrogen *pEngine = Hydrogen::get_instance();
	if ( ( output_port_1 == nullptr ) || ( output_port_2 == nullptr ) ) {
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
	jack_set_session_callback(m_pClient, jack_session_callback, (void*)this);
#endif

	if ( pPref->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ){
		// Make Hydrogen the timebase master, regardless if there
		// is already a timebase master present.
		m_nJackConditionalTakeOver = 0;
		// Make Hydrogen the JACK timebase master.
		initTimeMaster();
	} else {
		m_bHydrogenIsJackTimebaseMaster = false;
	}
	
	return 0;
}

void JackAudioDriver::makeTrackOutputs( Song* pSong )
{

	// Only execute the body of this function if a per-track
	// creation of the output ports is desired.
	if( Preferences::get_instance()->m_bJackTrackOuts == false )
		return;

	InstrumentList * pInstruments = pSong->get_instrument_list();
	Instrument * pInstr;
	int nInstruments = ( int ) pInstruments->size();

	// create dedicated channel output ports
	WARNINGLOG( QString( "Creating / renaming %1 ports" ).arg( nInstruments ) );

	int nTrackCount = 0;

	// Resets the `track_map' matrix.
	for( int i = 0 ; i < MAX_INSTRUMENTS ; i++ ){
		for ( int j = 0 ; j < MAX_COMPONENTS ; j++ ){
			track_map[i][j] = 0;
		}
	}
	// Creates a new output track or reassigns an existing one for
	// each component of each instrument and stores the result in
	// the `track_map'.
	for ( int n = 0; n <= nInstruments - 1; n++ ) {
		pInstr = pInstruments->get( n );
		for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			InstrumentComponent* pCompo = *it;
			setTrackOutput( nTrackCount, pInstr, pCompo, pSong);
			track_map[pInstr->get_id()][pCompo->get_drumkit_componentID()] = nTrackCount;
			nTrackCount++;
		}
	}
	// clean up unused ports
	jack_port_t *p_L, *p_R;
	for ( int n = nTrackCount; n < track_port_count; n++ ) {
		p_L = track_output_ports_L[n];
		p_R = track_output_ports_R[n];
		track_output_ports_L[n] = nullptr;
		jack_port_unregister( m_pClient, p_L );
		track_output_ports_R[n] = nullptr;
		jack_port_unregister( m_pClient, p_R );
	}

	track_port_count = nTrackCount;
}

/**
 * Give the @a n 'th port the name of @a instr .
 * If the n'th port doesn't exist, new ports up to n are created.
 */
void JackAudioDriver::setTrackOutput( int n, Instrument * instr, InstrumentComponent * pCompo, Song * pSong )
{
	QString chName;

	// The function considers `track_port_count' as the number of
	// ports already present. If its smaller than `n', new ports
	// have to be created.
	if ( track_port_count <= n ) {
		for ( int m = track_port_count; m <= n; m++ ) {
			chName = QString( "Track_%1_" ).arg( m + 1 );
			track_output_ports_L[m] =
				jack_port_register( m_pClient, ( chName + "L" ).toLocal8Bit(),
						     JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

			track_output_ports_R[m] =
				jack_port_register( m_pClient, ( chName + "R" ).toLocal8Bit(),
						    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

			if ( ! track_output_ports_R[m] || ! track_output_ports_L[m] ) {
				Hydrogen::get_instance()->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}
		}
		track_port_count = n + 1;
	}

	// Now we're sure there is an n'th port, rename it.
	DrumkitComponent* pDrumkitComponent = pSong->get_component( pCompo->get_drumkit_componentID() );
	chName = QString( "Track_%1_%2_%3_" ).arg( n + 1 ).arg( instr->get_name() ).arg( pDrumkitComponent->get_name() );

#ifdef HAVE_JACK_PORT_RENAME
	// This differs from jack_port_set_name() by triggering
	// PortRename notifications to clients that have registered a
	// port rename handler.
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
	Preferences* P = Preferences::get_instance();
	if ( P->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT ||
	     P->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ) {
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
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==
	     Preferences::USE_JACK_TRANSPORT /*||
	     Preferences::get_instance()->m_bJackMasterMode ==
	     Preferences::USE_JACK_TIME_MASTER*/ ) {
		if ( m_pClient ) {
			// jack_transport_locate() (jack/transport.h )
			// re-positions the transport to a new frame number.
			// May be called at any time by any client.  The new
			// position takes effect in two process cycles.
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
	INFOLOG("jack session callback");
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
			QString BaseName = baseName( playlist->get( i )->filePath );
			QString newName = jackSessionDirectory + BaseName;
			QString SongPath = reader.getPath( playlist->get( i )->filePath );
			if ( SongPath != nullptr && QFile::copy ( SongPath, newName ) ) {
				/* Keep only filename on list for relative read */
				playlist->get( i )->filePath = BaseName;
				// playlist->get( i )->m_hScript;
			} else {
				/* Note - we leave old path in playlist */
				ERRORLOG( "Can't copy " + playlist->get( i )->filePath + " to " + newName );
				ev->flags = JackSessionSaveError;
			}
		}

		/* Save updated playlist */
		bool relativePaths = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
		if ( Files::savePlaylistPath( jackSessionDirectory + FileName, playlist, relativePaths ) == nullptr ) {
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

void JackAudioDriver::initTimeMaster()
{
	if ( ! m_pClient ) return;

	Preferences* pref = Preferences::get_instance();
	if ( pref->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER) {
		// Defined in jack/transport.h
		// Register as timebase master for the JACK
		// subsystem.
		//
		// The timebase master registers a callback that
		// updates extended position information such as
		// beats or timecode whenever necessary.  Without
		// this extended information, there is no need for
		// this function.
		//
		// There is never more than one master at a time.
		// When a new client takes over, the former @a
		// timebase_callback is no longer called.  Taking
		// over the timebase may be done conditionally, so
		// it fails if there was a master already.
		//
		// @param client the JACK client structure.
		// @param conditional non-zero for a conditional
		// request. 
		// @param timebase_callback is a realtime function
		// that returns position information.
		// @param arg an argument for the @a timebase_callback
		// function. 
		// @return
		//   - 0 on success;
		//   - EBUSY if a conditional request fails because
		// there was already a timebase master;
		//   - other non-zero error code.
		int ret = jack_set_timebase_callback(m_pClient, m_nJackConditionalTakeOver,
						     jack_timebase_callback, this);
		if (ret != 0){
			pref->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
			m_bHydrogenIsJackTimebaseMaster = false;
		} else {
			m_bHydrogenIsJackTimebaseMaster = true;
		}
	} else {
		// Called by the timebase master to release itself
		// from that responsibility (defined in
		// jack/transport.h).
		jack_release_timebase(m_pClient);
		m_bHydrogenIsJackTimebaseMaster = false;
	}
}

void JackAudioDriver::com_release()
{
	if ( m_pClient == nullptr) return;

	jack_release_timebase(m_pClient);
	m_bHydrogenIsJackTimebaseMaster = false;
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
	pos->bar = H->getPosForTick( PlayTick );

	double TPB = H->getTickForHumanPosition( pos->bar );
	if ( TPB < 1 ) return;

	/* We'll cheat there is ticks_per_beat * 4 in bar
	   so every Hydrogen tick will be multiplied by 4 ticks */
	pos->ticks_per_beat = TPB;
	pos->valid = JackPositionBBT;
	pos->beats_per_bar = TPB / 48;
	pos->beat_type = 4.0;
	pos->beats_per_minute = H->getTimelineBpm( pos->bar );
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
		/* how many ticks elapsed from last bar ( where bar == pattern ) */
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
