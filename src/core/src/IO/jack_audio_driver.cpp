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
#include <algorithm>
#include <cmath>

#include <hydrogen/hydrogen.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
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

int JackAudioDriver::jackDriverSampleRate( jack_nframes_t nframes, void* param ){
	// Used for logging.
	Object* __object = ( Object* )param;
	QString msg = QString("Jack SampleRate changed: the sample rate is now %1/sec").arg( QString::number( static_cast<int>(nframes) ) );
	// The __INFOLOG macro uses the Object *__object and not the
	// Object instance as INFOLOG does. It will call
	// __object->logger()->log( H2Core::Logger::Info, ..., msg )
	// (see object.h).
	__INFOLOG( msg );
	JackAudioDriver::jackServerSampleRate = nframes;
	return 0;
}

int JackAudioDriver::jackDriverBufferSize( jack_nframes_t nframes, void* arg ){
	// This function does _NOT_ have to be realtime safe.
	JackAudioDriver::jackServerBufferSize = nframes;
	return 0;
}
	
void JackAudioDriver::jackDriverShutdown( void* arg )
{
	UNUSED( arg );

	JackAudioDriver::pJackDriverInstance->m_pClient = nullptr;
	Hydrogen::get_instance()->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}


const char* JackAudioDriver::__class_name = "JackAudioDriver";
unsigned long JackAudioDriver::jackServerSampleRate = 0;
jack_nframes_t JackAudioDriver::jackServerBufferSize = 0;
JackAudioDriver* JackAudioDriver::pJackDriverInstance = nullptr;
int JackAudioDriver::nWaits = 0;

JackAudioDriver::JackAudioDriver( JackProcessCallback m_processCallback )
	: AudioOutput( __class_name ),
	  m_frameOffset( 0 ),
	  m_nTrackPortCount( 0 ),
	  m_pClient( nullptr ),
	  m_pOutputPort1( nullptr ),
	  m_pOutputPort2( nullptr ),
	  m_nIsTimebaseMaster( -1 )
{
	INFOLOG( "INIT" );
	
	auto pPreferences = Preferences::get_instance();
	
	m_bConnectDefaults = pPreferences->m_bJackConnectDefaults;
	
	__track_out_enabled = pPreferences->m_bJackTrackOuts;

	m_transport.m_status = TransportInfo::STOPPED;
	m_transport.m_nFrames = 0;
	m_transport.m_fTickSize = 100;
	m_transport.m_fBPM = 120;

	JackAudioDriver::pJackDriverInstance = this;
	this->m_processCallback = m_processCallback;

	m_nTimebaseTracking = -1;
	m_timebaseState = Timebase::None;
	
	// Destination ports the output of Hydrogen will be connected
	// to.
	m_sOutputPortName1 = pPreferences->m_sJackPortName1;
	m_sOutputPortName2 = pPreferences->m_sJackPortName2;
	
	memset( m_pTrackOutputPortsL, 0, sizeof(m_pTrackOutputPortsL) );
	memset( m_pTrackOutputPortsR, 0, sizeof(m_pTrackOutputPortsR) );

	m_JackTransportState  = JackTransportStopped;
}

JackAudioDriver::~JackAudioDriver()
{
	INFOLOG( "DESTROY" );
	disconnect();
}

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

	bool bConnectDefaults = m_bConnectDefaults;

	memset( m_pTrackOutputPortsL, 0, sizeof(m_pTrackOutputPortsL) );
	memset( m_pTrackOutputPortsR, 0, sizeof(m_pTrackOutputPortsR) );

#ifdef H2CORE_HAVE_LASH
	if ( Preferences::get_instance()->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient && lashClient->isConnected()){
			// INFOLOG( "[LASH] Sending JACK client name to LASH server" );
			lashClient->sendJackClientName();

			if (!lashClient->isNewProject()){
				bConnectDefaults = false;
			}
		}
	}
#endif

	if ( bConnectDefaults ) {
		// Connect the left and right default ports of Hydrogen.
		//
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
		if ( jack_connect( m_pClient, jack_port_name( m_pOutputPort1 ),
				   m_sOutputPortName1.toLocal8Bit() ) == 0 &&
		     jack_connect( m_pClient, jack_port_name( m_pOutputPort2 ),
				   m_sOutputPortName2.toLocal8Bit() ) == 0 ) {
			return 0;
		}

		WARNINGLOG( "Could not connect to the saved output ports. Connect to the first pair of input ports instead." );
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
		if ( jack_connect( m_pClient, jack_port_name( m_pOutputPort1 ),
				   portnames[0] ) != 0 ||
		     jack_connect( m_pClient, jack_port_name( m_pOutputPort2 ),
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
	
	jack_client_t* pOldClient = m_pClient;
	
	m_pClient = nullptr;
	
	if ( pOldClient != nullptr ) {
		INFOLOG( "calling jack_client_close" );
		int nReturnCode = jack_client_close( pOldClient );
		if ( nReturnCode != 0 ) {
			ERRORLOG( "Error in jack_client_close" );
			Hydrogen::get_instance()->raiseError( Hydrogen::JACK_CANNOT_CLOSE_CLIENT );
		}
	}
	m_pClient = nullptr;
}

void JackAudioDriver::deactivate()
{
	if ( m_pClient != nullptr ) {
		INFOLOG( "calling jack_deactivate" );
		int nReturnCode = jack_deactivate( m_pClient );
		if ( nReturnCode != 0 ) {
			ERRORLOG( "Error in jack_deactivate" );
		}
	}
	memset( m_pTrackOutputPortsL, 0, sizeof(m_pTrackOutputPortsL) );
	memset( m_pTrackOutputPortsR, 0, sizeof(m_pTrackOutputPortsR) );
}

unsigned JackAudioDriver::getBufferSize()
{
	return JackAudioDriver::jackServerBufferSize;
}

unsigned JackAudioDriver::getSampleRate()
{
	return JackAudioDriver::jackServerSampleRate;
}

void JackAudioDriver::clearPerTrackAudioBuffers( uint32_t nFrames )
{
	if ( m_pClient != nullptr &&
		 Preferences::get_instance()->m_bJackTrackOuts ) {
		float* pBuffer;
		
		for ( int ii = 0; ii < m_nTrackPortCount; ++ii ) {
			pBuffer = getTrackOut_L( ii );
			if ( pBuffer != nullptr ) {
				memset( pBuffer, 0, nFrames * sizeof( float ) );
			}
			pBuffer = getTrackOut_R( ii );
			if ( pBuffer != nullptr ) {
				memset( pBuffer, 0, nFrames * sizeof( float ) );
			}
		}
	}
}

void JackAudioDriver::calculateFrameOffset(long long oldFrame)
{
	if ( Hydrogen::get_instance()->getState() == STATE_PLAYING ) {
		m_frameOffset = m_JackTransportPos.frame - m_transport.m_nFrames;
	} else {
		m_frameOffset = oldFrame - m_transport.m_nFrames;
	}
}

void JackAudioDriver::relocateUsingBBT()
{
	if ( m_nIsTimebaseMaster != 0 ) {
		ERRORLOG( QString( "Relocation using BBT information can only be used in the presence of another Jack timebase master" ) );
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	float fTicksPerBeat = static_cast<float>( pSong->__resolution / m_JackTransportPos.beat_type * 4 );

	long barTicks = 0;
	float fAdditionalTicks = 0;
	float fNumberOfBarsPassed = 0;
	if ( pSong->get_mode() == Song::SONG_MODE ) {

		if ( Preferences::get_instance()->m_JackBBTSync ==
			 Preferences::JackBBTSyncMethod::identicalBars ) {
			barTicks = pHydrogen->getTickForPosition( m_JackTransportPos.bar - 1 );

			if ( barTicks < 0 ) {
				barTicks = 0;
			}
		} else if ( Preferences::get_instance()->m_JackBBTSync ==
					Preferences::JackBBTSyncMethod::constMeasure ) {
			// Length of a pattern * fBarConversion provides the number of
			// bars in Jack's point of view a Hydrogen pattern does cover.
			float fBarConversion = pSong->__resolution * 4 *
				m_JackTransportPos.beats_per_bar /
				m_JackTransportPos.beat_type;
			float fNextIncrement = 0;
			int nBarJack = m_JackTransportPos.bar - 1;
			int nLargeNumber = 100000;
			int nMinimumPatternLength = nLargeNumber;
			int nNumberOfPatternsPassed = 0;

			// Checking how many of Hydrogen's patterns are covered by the
			// bar provided by Jack.
			auto pPatternGroup = pSong->get_pattern_group_vector();
			for ( const PatternList* ppPatternList : *pPatternGroup ) {
				nMinimumPatternLength = nLargeNumber;

				// If there are multiple patterns at a single bar (in
				// Hydrogen) the length of the shortest one used for
				// playback.
				for ( int ii = 0; ii < ppPatternList->size(); ++ii ) {
					if ( ppPatternList->get( ii )->get_length() <
						 nMinimumPatternLength ) {
						nMinimumPatternLength = ppPatternList->get( ii )->get_length();
					}
				}
			
				if ( nMinimumPatternLength == nLargeNumber ){
					fNextIncrement = 0;
				} else {
					fNextIncrement =
						static_cast<float>(nMinimumPatternLength) /
						fBarConversion;
				}
			
				if ( static_cast<float>(nBarJack) < ( fNumberOfBarsPassed + fNextIncrement ) ) {
					break;
				}
			
				fNumberOfBarsPassed += fNextIncrement;
				++nNumberOfPatternsPassed;
			}

			// Position of the resulting pattern in ticks.
			barTicks = pHydrogen->getTickForPosition( nNumberOfPatternsPassed );
			if ( barTicks < 0 ) {
				barTicks = 0;
			} else if ( fNextIncrement > 1 &&
						fNumberOfBarsPassed != nBarJack ) {
				// If pattern is longer than what is considered a bar in
				// Jack's point of view, some additional ticks have to be
				// added whenever transport passes the first bar contained
				// in the pattern.
				fAdditionalTicks = fTicksPerBeat * 4 *
					( fNextIncrement - 1 );
			}

			// std::cout << "[relocateUsingBBT] "
			// 		  << "nNumberOfPatternsPassed: " << nNumberOfPatternsPassed
			// 		  << ", fAdditionalTicks: " << fAdditionalTicks
			// 		  << ", nBarJack: " << nBarJack
			// 		  << ", fNumberOfBarsPassed: " << fNumberOfBarsPassed
			// 		  << ", fBarConversion: " << fBarConversion
			// 		  << ", barTicks: " << barTicks
			// 		  << std::endl;
		} else {
			ERRORLOG( QString( "Unsupported m_JackBBTSync option [%1]" )
					  .arg( static_cast<int>(Preferences::get_instance()->m_JackBBTSync) ) );
		}
	}

	float fNewTick = static_cast<float>(barTicks) + fAdditionalTicks +
		( m_JackTransportPos.beat - 1 ) * fTicksPerBeat +
		m_JackTransportPos.tick * ( fTicksPerBeat / m_JackTransportPos.ticks_per_beat );

	float fNewTickSize = AudioEngine::compute_tick_size( getSampleRate(), m_JackTransportPos.beats_per_minute, pSong->__resolution );

	if ( fNewTickSize == 0 ) {
		ERRORLOG(QString("Improper tick size [%1] for tick [%2]" )
				 .arg( fNewTickSize ).arg( fNewTick ) );
		return;
	}

	int nPatternStart;
	int nPattern = pHydrogen->getPosForTick( fNewTick, &nPatternStart );

	// NOTE this prevents audioEngine_process_checkBPMChanged
	// in Hydrogen.cpp from recalculating things.
	m_transport.m_fTickSize = fNewTickSize;
	m_transport.m_nFrames = static_cast<long long>(fNewTick * fNewTickSize);
	m_frameOffset = m_JackTransportPos.frame - m_transport.m_nFrames;

	float fBPM = static_cast<float>(m_JackTransportPos.beats_per_minute);
	if ( m_transport.m_fBPM != fBPM ) {
		setBpm( fBPM );
		pHydrogen->getSong()->__bpm = fBPM;
		pHydrogen->setNewBpmJTM( fBPM );
	}
}

void JackAudioDriver::updateTransportInfo()
{
	if ( Preferences::get_instance()->m_bJackTransportMode !=
	     Preferences::USE_JACK_TRANSPORT ){
		return;
	}
	
	// jack_transport_query() (jack/transport.h) queries the
	// current transport state and position. If called from the
	// process thread, the second argument, which is a pointer to
	// a structure for returning current transport, corresponds to
	// the first frame of the current cycle and the state returned
	// is valid for the entire cycle. #m_JackTransportPos.valid
	// will show which fields contain valid data. If
	// #m_JackTransportPos is NULL, do not return position
	// information.
	m_JackTransportState = jack_transport_query( m_pClient, &m_JackTransportPos );

	switch ( m_JackTransportState ) {
	case JackTransportStopped: // Transport is halted
		m_transport.m_status = TransportInfo::STOPPED;
		return;
		
	case JackTransportRolling: // Transport is playing
		m_transport.m_status = TransportInfo::ROLLING;
		break;

	case JackTransportStarting: 
		// Waiting for sync ready. If there are slow-sync clients,
		// this can take more than one cycle.
		m_transport.m_status = TransportInfo::STOPPED;

		if ( m_nIsTimebaseMaster == 0 ) {
			return;
		}
		
		break;
		
	default:
		ERRORLOG( "Unknown jack transport state" );
	}

	printState();
	
	m_currentPos = m_JackTransportPos.frame;
	
	// Update the status regrading JACK timebase master.
	if ( m_JackTransportState != JackTransportStopped ) {
		if ( m_nTimebaseTracking > 1 ) {
			m_nTimebaseTracking--;
		} else if ( m_nTimebaseTracking == 1 ) {
			// JackTimebaseCallback not called anymore -> timebase client
			m_nTimebaseTracking = 0;
			m_timebaseState = Timebase::Slave;
		}
	}
	if ( m_nTimebaseTracking == 0 && 
				!(m_JackTransportPos.valid & JackPositionBBT) ) {
		// No external timebase master anymore -> regular client
		m_nTimebaseTracking = -1;
		m_timebaseState = Timebase::None;
	} else if ( m_nTimebaseTracking < 0 && 
				(m_JackTransportPos.valid & JackPositionBBT) ) {
		// External timebase master detected -> timebase client
		m_nTimebaseTracking = 0;
		m_timebaseState = Timebase::Slave;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	
	// The relocation could be either triggered by an user interaction
	// (e.g. clicking the forward button or clicking somewhere on the
	// timeline) or by a different JACK client.
	if ( m_transport.m_nFrames + m_frameOffset != m_JackTransportPos.frame ) {
		// Reset playback to the beginning of the pattern if Hydrogen
		// is in pattern mode.
		pHydrogen->resetPatternStartTick();

		if ( m_nIsTimebaseMaster != 0 ) {
			m_transport.m_nFrames = m_JackTransportPos.frame;
		} else {
			relocateUsingBBT();
		}
		
			// There maybe was an offset introduced when passing a
			// tempo marker.
			m_frameOffset = 0;
		} else {
			relocateUsingBBT();
		}
	}

	if ( m_timebaseState == Timebase::Slave ){
		// There is a JACK timebase master and it's not us. If it
		// provides a tempo that differs from the local one, we will
		// use the former instead.
		if ( m_transport.m_fBPM !=
			 static_cast<float>(m_JackTransportPos.beats_per_minute ) ||
			 !compareAdjacentBBT() ) {
			// printState();
			relocateUsingBBT();
		}
	} else {
		// Checks for local changes in speed (introduced by the user
		// using BPM markers on the timeline) and update tempo
		// accordingly.
		pHydrogen->setTimelineBpm();
	}

	if ( m_nIsTimebaseMaster == 0 ) {
		m_previousJackTransportPos = m_JackTransportPos;
	}
}

bool JackAudioDriver::compareAdjacentBBT() const
{
	if ( m_JackTransportPos.beats_per_minute !=
		 m_previousJackTransportPos.beats_per_minute ) {
		INFOLOG( QString( "Change in tempo from [%1] to [%2]" )
				 .arg( m_previousJackTransportPos.beats_per_minute )
				 .arg( m_JackTransportPos.beats_per_minute ) );
		return false;
	}

	double expectedTickUpdate =
		( m_JackTransportPos.frame - m_previousJackTransportPos.frame ) *
		m_JackTransportPos.beats_per_minute *
		m_JackTransportPos.ticks_per_beat /
		m_JackTransportPos.frame_rate / 60;
	
	int32_t nNewTick = m_previousJackTransportPos.tick +
		floor( expectedTickUpdate );

	// The rounding is the task of the external timebase master. So,
	// we need to be a little generous in here to be sure to match its
	// decision.
	if ( nNewTick + 1 >= m_JackTransportPos.ticks_per_beat ) {
		nNewTick = remainder( nNewTick, m_JackTransportPos.ticks_per_beat );

		if ( m_previousJackTransportPos.beat + 1 >
			 m_previousJackTransportPos.beats_per_bar ) {
			if ( m_JackTransportPos.bar !=
				m_previousJackTransportPos.bar + 1 ||
				m_JackTransportPos.beat != 1 ) {
				INFOLOG( QString( "Change in position from bar:beat [%1]:[%2] to [%3]:[%4]" )
						 .arg( m_previousJackTransportPos.bar )
						 .arg( m_previousJackTransportPos.beat )
						 .arg( m_JackTransportPos.bar )
						 .arg( m_JackTransportPos.beat ) );
				return false;
			}
		} else {
			if ( m_JackTransportPos.bar !=
				m_previousJackTransportPos.bar ||
				m_JackTransportPos.beat !=
				m_previousJackTransportPos.beat + 1 ) {
				INFOLOG( QString( "Change in position from bar:beat [%1]:[%2] to [%3]:[%4]" )
						 .arg( m_previousJackTransportPos.bar )
						 .arg( m_previousJackTransportPos.beat )
						 .arg( m_JackTransportPos.bar )
						 .arg( m_JackTransportPos.beat ) );
				return false;
			}
		}
	} else if ( m_JackTransportPos.bar !=
				m_previousJackTransportPos.bar ||
				m_JackTransportPos.beat !=
				m_previousJackTransportPos.beat ) {
		INFOLOG( QString( "Change in position from bar:beat [%1]:[%2] to [%3]:[%4]" )
				 .arg( m_previousJackTransportPos.bar )
				 .arg( m_previousJackTransportPos.beat )
				 .arg( m_JackTransportPos.bar )
				 .arg( m_JackTransportPos.beat ) );
		return false;
	}

	if ( abs( m_JackTransportPos.tick - nNewTick ) > 1 &&
		 abs( m_JackTransportPos.tick -
			  m_JackTransportPos.ticks_per_beat - nNewTick ) > 1 &&
		 abs( m_JackTransportPos.tick +
			  m_JackTransportPos.ticks_per_beat - nNewTick ) > 1 ) {
		INFOLOG( QString( "Change in position from tick [%1] to [%2] instead of [%3]" )
				 .arg( m_previousJackTransportPos.tick )
				 .arg( m_JackTransportPos.tick )
				 .arg( nNewTick ));
		return false;
	}
		
	return true;
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
	jack_default_audio_sample_t *out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer( m_pOutputPort1, JackAudioDriver::jackServerBufferSize ));
	return out;
}

float* JackAudioDriver::getOut_R()
{
	jack_default_audio_sample_t *out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer( m_pOutputPort2, JackAudioDriver::jackServerBufferSize ));
	return out;
}

float* JackAudioDriver::getTrackOut_L( unsigned nTrack )
{
	if ( nTrack > static_cast<unsigned>(m_nTrackPortCount) ) {
		return nullptr;
	}
	
	jack_port_t* pPort = m_pTrackOutputPortsL[nTrack];
	jack_default_audio_sample_t* out = nullptr;
	if( pPort ) {
		out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer( pPort, JackAudioDriver::jackServerBufferSize));
	}
	return out;
}

float* JackAudioDriver::getTrackOut_R( unsigned nTrack )
{
	if( nTrack > static_cast<unsigned>(m_nTrackPortCount) ) {
		return nullptr;
	}
	
	jack_port_t* pPort = m_pTrackOutputPortsR[nTrack];
	jack_default_audio_sample_t* out = nullptr;
	if( pPort ) {
		out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer( pPort, JackAudioDriver::jackServerBufferSize));
	}
	return out;
}

float* JackAudioDriver::getTrackOut_L( Instrument* instr, InstrumentComponent* pCompo)
{
	return getTrackOut_L(m_trackMap[instr->get_id()][pCompo->get_drumkit_componentID()]);
}

float* JackAudioDriver::getTrackOut_R( Instrument* instr, InstrumentComponent* pCompo)
{
	return getTrackOut_R(m_trackMap[instr->get_id()][pCompo->get_drumkit_componentID()]);
}


#define CLIENT_FAILURE(msg) {						\
	ERRORLOG("Could not connect to JACK server (" msg ")"); 	\
	if ( m_pClient != nullptr ) {						\
		ERRORLOG("...but JACK returned a non-null pointer?"); 	\
		m_pClient = nullptr;					\
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
	auto pPreferences = Preferences::get_instance();
	
	QString sClientName = "Hydrogen";

#ifdef H2CORE_HAVE_OSC
	QString sNsmClientId = pPreferences->getNsmClientId();

	if( !sNsmClientId.isEmpty() ){
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
		if ( pPreferences->getJackSessionUUID().isEmpty() ){
			m_pClient = jack_client_open( sClientName.toLocal8Bit(),
						      JackNullOption,
						      &status);
		} else {
			// Unique name of the JACK server used within
			// the JACK session.
			const QByteArray uuid = pPreferences->getJackSessionUUID().toLocal8Bit();
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
			if ( m_pClient != nullptr ) {
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
				if ( m_pClient != nullptr ) {
					CLIENT_SUCCESS("Client pointer is *not* null..."
						       " assuming we're OK");
				}
			} else {
				CLIENT_SUCCESS("Connected to JACK server");
			}
		}
	}

	if ( m_pClient == nullptr ) {
		return -1;
	}

	JackAudioDriver::jackServerSampleRate = jack_get_sample_rate( m_pClient );
	JackAudioDriver::jackServerBufferSize = jack_get_buffer_size( m_pClient );

	pPreferences->m_nSampleRate = JackAudioDriver::jackServerSampleRate;
	pPreferences->m_nBufferSize = JackAudioDriver::jackServerBufferSize;

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
	jack_set_process_callback( m_pClient, this->m_processCallback, nullptr );

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
	m_pOutputPort1 = jack_port_register( m_pClient, "out_L", JACK_DEFAULT_AUDIO_TYPE,
					    JackPortIsOutput, 0 );
	m_pOutputPort2 = jack_port_register( m_pClient, "out_R", JACK_DEFAULT_AUDIO_TYPE,
					    JackPortIsOutput, 0 );

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	if ( ( m_pOutputPort1 == nullptr ) || ( m_pOutputPort2 == nullptr ) ) {
		pHydrogen->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
		return 4;
	}

#ifdef H2CORE_HAVE_LASH
	if ( pPreferences->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if ( lashClient->isConnected() ) {
			lashClient->setJackClientName(sClientName.toLocal8Bit().constData());
		}
	}
#endif

#ifdef H2CORE_HAVE_JACKSESSION
	jack_set_session_callback(m_pClient, jack_session_callback, (void*)this);
#endif

	if ( pPreferences->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT &&
		 pPreferences->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ){
		initTimebaseMaster();
	}
	
	return 0;
}

void JackAudioDriver::makeTrackOutputs( Song* pSong )
{
	if( Preferences::get_instance()->m_bJackTrackOuts == false ) {
		return;
	}

	InstrumentList* pInstrumentList = pSong->get_instrument_list();
	Instrument* pInstrument;
	int nInstruments = static_cast<int>(pInstrumentList->size());

	WARNINGLOG( QString( "Creating / renaming %1 ports" ).arg( nInstruments ) );

	int nTrackCount = 0;

	for( int i = 0 ; i < MAX_INSTRUMENTS ; i++ ){
		for ( int j = 0 ; j < MAX_COMPONENTS ; j++ ){
			m_trackMap[i][j] = 0;
		}
	}
	// Creates a new output track or reassigns an existing one for
	// each component of each instrument and stores the result in
	// the `m_trackMap'.
	InstrumentComponent* pInstrumentComponent;
	for ( int n = 0; n <= nInstruments - 1; n++ ) {
		pInstrument = pInstrumentList->get( n );
		for ( auto it = pInstrument->get_components()->begin();
			  it != pInstrument->get_components()->end(); ++it) {
			
			pInstrumentComponent = *it;
			setTrackOutput( nTrackCount, pInstrument, pInstrumentComponent, pSong);
			m_trackMap[pInstrument->get_id()][pInstrumentComponent->get_drumkit_componentID()] = 
				nTrackCount;
			nTrackCount++;
		}
	}
	// clean up unused ports
	jack_port_t *pPortL, *pPortR;
	for ( int n = nTrackCount; n < m_nTrackPortCount; n++ ) {
		pPortL = m_pTrackOutputPortsL[n];
		pPortR = m_pTrackOutputPortsR[n];
		m_pTrackOutputPortsL[n] = nullptr;
		jack_port_unregister( m_pClient, pPortL );
		m_pTrackOutputPortsR[n] = nullptr;
		jack_port_unregister( m_pClient, pPortR );
	}

	m_nTrackPortCount = nTrackCount;
}

void JackAudioDriver::setTrackOutput( int n, Instrument* pInstrument, InstrumentComponent *pInstrumentComponent, Song* pSong )
{
	QString sComponentName;

	// The function considers `m_nTrackPortCount' as the number of
	// ports already present. If it's smaller than `n', new ports
	// have to be created.
	if ( m_nTrackPortCount <= n ) {
		for ( int m = m_nTrackPortCount; m <= n; m++ ) {
			sComponentName = QString( "Track_%1_" ).arg( m + 1 );
			m_pTrackOutputPortsL[m] =
				jack_port_register( m_pClient, ( sComponentName + "L" ).toLocal8Bit(),
						     JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

			m_pTrackOutputPortsR[m] =
				jack_port_register( m_pClient, ( sComponentName + "R" ).toLocal8Bit(),
						    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

			if ( ! m_pTrackOutputPortsR[m] || ! m_pTrackOutputPortsL[m] ) {
				Hydrogen::get_instance()->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}
		}
		m_nTrackPortCount = n + 1;
	}

	// Now that we're sure there is an n'th port, rename it.
	DrumkitComponent* pDrumkitComponent = pSong->get_component( pInstrumentComponent->get_drumkit_componentID() );
	sComponentName = QString( "Track_%1_%2_%3_" ).arg( n + 1 )
		.arg( pInstrument->get_name() ).arg( pDrumkitComponent->get_name() );

#ifdef HAVE_JACK_PORT_RENAME
	// This differs from jack_port_set_name() by triggering
	// PortRename notifications to clients that have registered a
	// port rename handler.
	jack_port_rename( m_pClient, m_pTrackOutputPortsL[n], ( sComponentName + "L" ).toLocal8Bit() );
	jack_port_rename( m_pClient, m_pTrackOutputPortsR[n], ( sComponentName + "R" ).toLocal8Bit() );
#else
	jack_port_set_name( m_pTrackOutputPortsL[n], ( sComponentName + "L" ).toLocal8Bit() );
	jack_port_set_name( m_pTrackOutputPortsR[n], ( sComponentName + "R" ).toLocal8Bit() );
#endif
}

void JackAudioDriver::play()
{
	Preferences* pPreferences = Preferences::get_instance();
	if ( pPreferences->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT ) {
		if ( m_pClient != nullptr ) {
			INFOLOG( "jack_transport_start()" );
			jack_transport_start( m_pClient );
		}
	} else {
		m_transport.m_status = TransportInfo::ROLLING;
	}
}

void JackAudioDriver::stop()
{
	Preferences* pPreferences = Preferences::get_instance();
	if ( pPreferences->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT ) {
		if ( m_pClient != nullptr ) {
			INFOLOG( "jack_transport_stop()" );
			jack_transport_stop( m_pClient );
		}
	} else {
		m_transport.m_status = TransportInfo::STOPPED;
	}
}

void JackAudioDriver::locate( unsigned long frame )
{
	if ( ( Preferences::get_instance() )->m_bJackTransportMode ==
	     Preferences::USE_JACK_TRANSPORT ) {
		if ( m_pClient != nullptr ) {
			// jack_transport_locate() (jack/transport.h )
			// re-positions the transport to a new frame number. May
			// be called at any time by any client.
			jack_transport_locate( m_pClient, frame );
		}
	} else {
		m_transport.m_nFrames = static_cast<long long>(frame);
	}
}

void JackAudioDriver::setBpm( float fBPM )
{
	if ( fBPM >= 1 ) {
		m_transport.m_fBPM = fBPM;
	}
}

#ifdef H2CORE_HAVE_JACKSESSION
void JackAudioDriver::jack_session_callback(jack_session_event_t *event, void *arg)
{
	JackAudioDriver* pDriver = static_cast<JackAudioDriver*>(arg);
	if ( pDriver != nullptr ) {
		pDriver->jack_session_callback_impl( event );
	}
}

static QString baseName( QString sPath ) {
	return QFileInfo( sPath ).fileName();
}

void JackAudioDriver::jack_session_callback_impl(jack_session_event_t* event)
{
	enum session_events{
		SAVE_SESSION,
		SAVE_AND_QUIT,
		SAVE_TEMPLATE
	};

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	Preferences* pPreferences = Preferences::get_instance();
	EventQueue* pEventQueue = EventQueue::get_instance();

	jack_session_event_t* ev = static_cast<jack_session_event_t*>(event);

	QString sJackSessionDirectory = static_cast<QString>(ev->session_dir);
	QString sRetval = pPreferences->getJackSessionApplicationPath() + 
		" --jacksessionid " + ev->client_uuid;

	/* Playlist mode */
	Playlist* pPlaylist = Playlist::get_instance();
	if ( pPlaylist->size() > 0 ) {

		if ( pPlaylist->getFilename().isEmpty() ) {
			pPlaylist->setFilename( Filesystem::untitled_playlist_file_name() );
		}

		QString sFileName = baseName( pPlaylist->getFilename() );
		sFileName.replace( QString(" "), QString("_") );
		sRetval += " -p \"${SESSION_DIR}" + sFileName + "\"";

		/* Copy all songs to Session Directory and update playlist */
		SongReader reader;
		for ( uint i = 0; i < pPlaylist->size(); ++i ) {
			QString sBaseName = baseName( pPlaylist->get( i )->filePath );
			QString sNewName = sJackSessionDirectory + sBaseName;
			QString sSongPath = reader.getPath( pPlaylist->get( i )->filePath );
			if ( sSongPath != nullptr && QFile::copy( sSongPath, sNewName ) ) {
				/* Keep only filename on list for relative read */
				pPlaylist->get( i )->filePath = sBaseName;
			} else {
				/* Note - we leave old path in playlist */
				ERRORLOG( "Can't copy " + pPlaylist->get( i )->filePath + " to " + sNewName );
				ev->flags = JackSessionSaveError;
			}
		}

		/* Save updated playlist */
		bool bRelativePaths = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
		if ( Files::savePlaylistPath( sJackSessionDirectory + sFileName, 
									  pPlaylist, bRelativePaths ) == nullptr ) {
			ev->flags = JackSessionSaveError;
		}
		/* Song Mode */
	} else {
		/* Valid Song is needed */
		if ( pSong->get_filename().isEmpty() ) {
			pSong->set_filename( Filesystem::untitled_song_file_name() );
		}

		QString sFileName = baseName( pSong->get_filename() );
		sFileName.replace( QString(" "), QString("_") );
		pSong->set_filename( sJackSessionDirectory + sFileName);

		/* SongReader will look into SESSION DIR anyway */
		sRetval += " -s \"" + sFileName + "\"";

		switch (ev->type) {
			case JackSessionSave:
				pEventQueue->push_event(EVENT_JACK_SESSION, SAVE_SESSION);
				break;
			case JackSessionSaveAndQuit:
				pEventQueue->push_event(EVENT_JACK_SESSION, SAVE_SESSION);
				pEventQueue->push_event(EVENT_JACK_SESSION, SAVE_AND_QUIT);
				break;
			default:
				ERRORLOG( "JackSession: Unknown event type" );
				ev->flags = JackSessionSaveError;
		}
	}

	ev->command_line = strdup( sRetval.toUtf8().constData() );
	jack_session_reply( m_pClient, ev );
	jack_session_event_free( ev );
}
#endif

void JackAudioDriver::initTimebaseMaster()
{
	if ( m_pClient == nullptr ) {
		return;
	}

	Preferences* pPreferences = Preferences::get_instance();
	if ( pPreferences->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER) {
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
		int nReturnValue = jack_set_timebase_callback(m_pClient, 0,
						     JackTimebaseCallback, this);
		if ( nReturnValue != 0 ){
			pPreferences->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
		} else {
			m_nTimebaseTracking = 2;
			m_timebaseState = Timebase::Master;
		}
	} else {
	    releaseTimebaseMaster();
	}
}

void JackAudioDriver::releaseTimebaseMaster()
{
	if ( m_pClient == nullptr ) {
		ERRORLOG( QString( "Not fully initialized yet" ) );
		return;
	}

	jack_release_timebase( m_pClient );
	
	if ( m_JackTransportPos.valid & JackPositionBBT ) {
		m_nTimebaseTracking = 0;
		m_timebaseState = Timebase::Slave;
	} else {
		m_nTimebaseTracking = -1;
		m_timebaseState = Timebase::None;
	}
}

void JackAudioDriver::JackTimebaseCallback(jack_transport_state_t state,
					     jack_nframes_t nFrames,
					     jack_position_t* pJackPosition,
					     int new_pos,
					     void *arg)
{
	JackAudioDriver* pDriver = static_cast<JackAudioDriver*>(arg);
	if ( pDriver == nullptr ){
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	// ---------------------------------------------------------------
	// What is the BBT information?
	//
	// There is no formal definition in the JACK API but the way it is
	// interpreted by Hydrogen is the following:
	//
	// bar: Number of measures played since the beginning of the
	// song. (Note that this may not coincide with the length of a pattern).
	// beat: Number of quarters passed since the beginning of the the
	//     pattern. 
	// tick: Number of ticks passed since the last beat (with respect
	//     to the current frame). 
	//
	// A tick is an internal measure representing the smallest
	// resolution of the transport position in terms of the
	// patterns. It consist of m_transport.m_fTickSize frames, which
	// changes depending on the current tempo.
	// ---------------------------------------------------------------

	// First tick covered during the next cycle.
	float fTickSize = pDriver->m_transport.m_fTickSize;
	unsigned long nextTick = 
		floor(( pJackPosition->frame - pDriver->m_frameOffset ) / 
			  fTickSize );
	
	int nNextPatternStartTick;
	int nNextPattern = 
		pHydrogen->getPosForTick( nextTick, &nNextPatternStartTick );

	// In order to determine the tempo, which will be set by Hydrogen
	// during the next transport cycle, we have to look at the last
	// tick handled in audioEngine_updateNoteQueue() (during this
	// cycle and after the updateTransportInfo() returns.
	unsigned long nextTickInternal = 
		floor(( pJackPosition->frame - pDriver->m_frameOffset + 
				pHydrogen->calculateLookahead( fTickSize ) ) / 
			  fTickSize) - 1;
	int nNextPatternStartTickInternal;
	int nNextPatternInternal = 
		pHydrogen->getPosForTick( nextTickInternal, &nNextPatternStartTickInternal );

	// Calculate the length of the next pattern in ticks == number
	// of ticks in the next bar.
	long ticksPerBar = pHydrogen->getPatternLength( nNextPattern );
	if ( ticksPerBar < 1 ) {
		return;
	}

	pJackPosition->ticks_per_beat = static_cast<double>(ticksPerBar) / 4;
	pJackPosition->valid = JackPositionBBT;
	// Time signature "numerator"
	pJackPosition->beats_per_bar = 
		(static_cast<float>(ticksPerBar) /  static_cast<float>(pSong->__resolution));
	// Time signature "denominator"
	pJackPosition->beat_type = 4.0;
	
	if ( pDriver->m_transport.m_nFrames + pDriver->m_frameOffset != pJackPosition->frame ) {
		// In case of a relocation, wait two full cycles till the new
		// tempo will be broadcast.
		JackAudioDriver::nWaits = 2;
	}

	if ( JackAudioDriver::nWaits == 0 ) {
		// Average tempo in BPM for the block corresponding to
		// pJackPosition. In Hydrogen is guaranteed to be constant within
		// a block.
		pJackPosition->beats_per_minute = 
			static_cast<double>(pHydrogen->getTimelineBpm( nNextPatternInternal ));
	} else {
		pJackPosition->beats_per_minute = static_cast<double>(pDriver->m_transport.m_fBPM);
	}
		
	JackAudioDriver::nWaits = std::max( int(0), JackAudioDriver::nWaits - 1);

	if ( pDriver->m_transport.m_nFrames < 1 ) {
		pJackPosition->bar = 1;
		pJackPosition->beat = 1;
		pJackPosition->tick = 0;
		pJackPosition->bar_start_tick = 0;
	} else {
		// +1 since the counting bars starts at 1.
		pJackPosition->bar = nNextPattern + 1;
		
		/* how many ticks elapsed from last bar ( where bar == pattern ) */
		int32_t nTicksFromBar = ( nextTick % static_cast<int32_t>(ticksPerBar) );

		// Number of ticks that have elapsed between frame 0 and the
		// first beat of the next measure.
		pJackPosition->bar_start_tick = nextTick - nTicksFromBar;

		pJackPosition->beat = nTicksFromBar / pJackPosition->ticks_per_beat;
		// +1 since the counting beats starts at 1.
		pJackPosition->beat++;

		// Counting ticks starts at 0.
		pJackPosition->tick = nTicksFromBar % static_cast<int32_t>(pJackPosition->ticks_per_beat);
				
	}

	// JackAudioDriver::printJackTransportPos( pJackPosition );
    
	// Tell Hydrogen it is still timebase master.
	pDriver->m_nTimebaseTracking = 2;
}

void JackAudioDriver::printState() const {

	auto pHydrogen = Hydrogen::get_instance();
	
	printJackTransportPos( &m_JackTransportPos );
	
	std::cout << "\033[35m[Hydrogen] JackAudioDriver state: "
			  << ", m_transport.m_nFrames: " << m_transport.m_nFrames
			  << ", m_transport.m_fBPM: " << m_transport.m_fBPM
			  << ", m_transport.m_fTickSize: " << m_transport.m_fTickSize
			  << ", m_transport.m_status: " << m_transport.m_status
			  << ", m_frameOffset: " << m_frameOffset
			  << ", m_JackTransportState: " << m_JackTransportState
			  << ", m_nIsTimebaseMaster: " << m_nIsTimebaseMaster
			  << ", m_currentPos: " << m_currentPos
			  << ", pHydrogen->getPatternPos(): " << pHydrogen->getPatternPos()
			  << "\33[0m" << std::endl;
}


void JackAudioDriver::printJackTransportPos( const jack_position_t* pPos ) {
	std::cout << "\033[36m[Hydrogen] JackTransportPosition: "
			  << ", frame: " << pPos->frame 
			  << ", frame_rate: " << pPos->frame_rate
			  << ", valid: " << pPos->valid
			  << ", bar: " << pPos->bar
			  << ", beat: " << pPos->beat
			  << ", tick: " << pPos->tick
			  << ", bar_start_tick: " << pPos->bar_start_tick
			  << ", beats_per_bar: " << pPos->beats_per_bar
			  << ", beat_type: " << pPos->beat_type
			  << ", ticks_per_beat: " << pPos->ticks_per_beat
			  << ", beats_per_minute: " << pPos->beats_per_minute
			  << ", frame_time: " << pPos->frame_time
			  << ", next_time: " << pPos->next_time
			  << "\033[0m" << std::endl;
}
};

#endif // H2CORE_HAVE_JACK
