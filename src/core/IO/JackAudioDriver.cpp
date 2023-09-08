/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/IO/JackAudioDriver.h>
#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_

#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <jack/metadata.h>
#include <QProcess>

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Files.h>
#include <core/Helpers/Filesystem.h>
#include <core/Preferences/Preferences.h>
#include <core/Globals.h>
#include <core/EventQueue.h>

#ifdef H2CORE_HAVE_LASH
#include <core/Lash/LashClient.h>
#endif

namespace H2Core {

int JackAudioDriver::jackDriverSampleRate( jack_nframes_t nframes, void* param ){
	// Used for logging.
	Base * __object = ( Base * )param;
	// The __INFOLOG macro uses the Base *__object and not the
	// Object instance as INFOLOG does. It will call
	// __object->logger()->log( H2Core::Logger::Info, ..., msg )
	// (see object.h).
	__INFOLOG( QString("New JACK sample rate: [%1]/sec")
			   .arg( QString::number( static_cast<int>(nframes) ) ) );
	JackAudioDriver::jackServerSampleRate = nframes;
	return 0;
}

int JackAudioDriver::jackDriverBufferSize( jack_nframes_t nframes, void* param ){
	// This function does _NOT_ have to be realtime safe.
	Base * __object = ( Base * )param;
	__INFOLOG( QString("new JACK buffer size: [%1]")
			   .arg( QString::number( static_cast<int>(nframes) ) ) );
	JackAudioDriver::jackServerBufferSize = nframes;
	return 0;
}
	
void JackAudioDriver::jackDriverShutdown( void* arg )
{
	UNUSED( arg );

	JackAudioDriver::pJackDriverInstance->m_pClient = nullptr;
	Hydrogen::get_instance()->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}
int JackAudioDriver::jackXRunCallback( void *arg ) {
	UNUSED( arg );
	++JackAudioDriver::jackServerXRuns;
	EventQueue::get_instance()->push_event( EVENT_XRUN, 0 );
	return 0;
}

unsigned long JackAudioDriver::jackServerSampleRate = 0;
int JackAudioDriver::jackServerXRuns = 0;
jack_nframes_t JackAudioDriver::jackServerBufferSize = 0;
JackAudioDriver* JackAudioDriver::pJackDriverInstance = nullptr;

JackAudioDriver::JackAudioDriver( JackProcessCallback m_processCallback )
	: AudioOutput(),
	  m_nTrackPortCount( 0 ),
	  m_pClient( nullptr ),
	  m_pOutputPort1( nullptr ),
	  m_pOutputPort2( nullptr ),
	  m_nTimebaseTracking( -1 ),
	  m_timebaseState( Timebase::None )
{
	auto pPreferences = Preferences::get_instance();
	
	m_bConnectDefaults = pPreferences->m_bJackConnectDefaults;

	JackAudioDriver::pJackDriverInstance = this;
	this->m_processCallback = m_processCallback;

	
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

void JackAudioDriver::relocateUsingBBT()
{
	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK timebase disabled in the Preferences" );
		return;
	}
	if ( m_timebaseState != Timebase::Slave ) {
		ERRORLOG( QString( "Relocation using BBT information can only be used in the presence of another Jack timebase master" ) );
		return;
	}

	// Sometime the JACK server does send seemingly random nuisance.
	if ( m_JackTransportPos.beat_type < 1 ||
		 m_JackTransportPos.bar < 1 ||
		 m_JackTransportPos.beat < 1 ||
		 m_JackTransportPos.beats_per_bar < 1 ||
		 m_JackTransportPos.beats_per_minute < MIN_BPM ||
		 m_JackTransportPos.beats_per_minute > MAX_BPM ||
		 m_JackTransportPos.ticks_per_beat < 1 ) {
		ERRORLOG( QString( "Unsupported to BBT content. beat_type: %1, bar: %2, beat: %3, beats_per_bar: %4, beats_per_minute: %5, ticks_per_beat: %6" )
				  .arg( m_JackTransportPos.beat_type )
				  .arg( m_JackTransportPos.bar )
				  .arg( m_JackTransportPos.beat )
				  .arg( m_JackTransportPos.beats_per_bar )
				  .arg( m_JackTransportPos.beats_per_minute )
				  .arg( m_JackTransportPos.ticks_per_beat ) );
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pSong == nullptr ) {
		// Expected behavior if Hydrogen is exited while playback is
		// still running.
		// DEBUGLOG( "No song set." );
		return;
	}

	float fTicksPerBeat = static_cast<float>( pSong->getResolution() / m_JackTransportPos.beat_type * 4 );

	long barTicks = 0;
	float fAdditionalTicks = 0;
	float fNumberOfBarsPassed = 0;
	if ( pHydrogen->getMode() == Song::Mode::Song ) {
 
		if ( Preferences::get_instance()->m_JackBBTSync ==
			 Preferences::JackBBTSyncMethod::identicalBars ) {
			barTicks = pHydrogen->getTickForColumn( m_JackTransportPos.bar - 1 );

			if ( barTicks < 0 ) {
				barTicks = 0;
			}
		} else if ( Preferences::get_instance()->m_JackBBTSync ==
					Preferences::JackBBTSyncMethod::constMeasure ) {
			// Length of a pattern * fBarConversion provides the number of
			// bars in Jack's point of view a Hydrogen pattern does cover.
			float fBarConversion = pSong->getResolution() * 4 *
				m_JackTransportPos.beats_per_bar /
				m_JackTransportPos.beat_type;
			float fNextIncrement = 0;
			int nBarJack = m_JackTransportPos.bar - 1;
			int nLargeNumber = 100000;
			int nMinimumPatternLength = nLargeNumber;
			int nNumberOfPatternsPassed = 0;

			// Checking how many of Hydrogen's patterns are covered by the
			// bar provided by Jack.
			auto pPatternGroup = pSong->getPatternGroupVector();
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
			barTicks = pHydrogen->getTickForColumn( nNumberOfPatternsPassed );
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

	pAudioEngine->locate( fNewTick, false );
}

bool JackAudioDriver::compareAdjacentBBT() const
{
	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK timebase disabled in the Preferences" );
	}
	
	if ( m_JackTransportPos.beats_per_minute !=
		 m_previousJackTransportPos.beats_per_minute ) {
		// DEBUGLOG( QString( "Change in tempo from [%1] to [%2]" )
		// 		  .arg( m_previousJackTransportPos.beats_per_minute )
		// 		  .arg( m_JackTransportPos.beats_per_minute ) );
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
	if ( m_JackTransportPos.tick != nNewTick &&
		 nNewTick + 1 >= m_JackTransportPos.ticks_per_beat ) {
		nNewTick = remainder( nNewTick, m_JackTransportPos.ticks_per_beat );

		if ( m_previousJackTransportPos.beat + 1 >
			 m_previousJackTransportPos.beats_per_bar ) {
			if ( m_JackTransportPos.bar !=
				m_previousJackTransportPos.bar + 1 ||
				m_JackTransportPos.beat != 1 ) {
				// DEBUGLOG( QString( "Change in position from bar:beat [%1]:[%2] to [%3]:[%4]*" )
				// 		  .arg( m_previousJackTransportPos.bar )
				// 		  .arg( m_previousJackTransportPos.beat )
				// 		  .arg( m_JackTransportPos.bar )
				// 		  .arg( m_JackTransportPos.beat ) );
				return false;
			}
		} else {
			if ( m_JackTransportPos.bar !=
				m_previousJackTransportPos.bar ||
				m_JackTransportPos.beat !=
				m_previousJackTransportPos.beat + 1 ) {
				// DEBUGLOG( QString( "Change in position from bar:beat [%1]:[%2] to [%3]:[%4]**" )
				// 		  .arg( m_previousJackTransportPos.bar )
				// 		  .arg( m_previousJackTransportPos.beat )
				// 		  .arg( m_JackTransportPos.bar )
				// 		  .arg( m_JackTransportPos.beat ) );
				return false;
			}
		}
	} else if ( m_JackTransportPos.bar !=
				m_previousJackTransportPos.bar ||
				m_JackTransportPos.beat !=
				m_previousJackTransportPos.beat ) {
		// DEBUGLOG( QString( "Change in position from bar:beat [%1]:[%2] to [%3]:[%4]***" )
		// 		  .arg( m_previousJackTransportPos.bar )
		// 		  .arg( m_previousJackTransportPos.beat )
		// 		  .arg( m_JackTransportPos.bar )
		// 		  .arg( m_JackTransportPos.beat ) );
		return false;
	}

	if ( abs( m_JackTransportPos.tick - nNewTick ) > 1 &&
		 abs( m_JackTransportPos.tick -
			  m_JackTransportPos.ticks_per_beat - nNewTick ) > 1 &&
		 abs( m_JackTransportPos.tick +
			  m_JackTransportPos.ticks_per_beat - nNewTick ) > 1 ) {
		// DEBUGLOG( QString( "Change in position from tick [%1] to [%2] instead of [%3]" )
		// 		  .arg( m_previousJackTransportPos.tick )
		// 		  .arg( m_JackTransportPos.tick )
		// 		  .arg( nNewTick ));
		return false;
	}
		
	return true;
}

void JackAudioDriver::updateTransportPosition()
{
	if ( Preferences::get_instance()->m_bJackTransportMode !=
	     Preferences::USE_JACK_TRANSPORT ){
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	const bool bTimebaseEnabled = Preferences::get_instance()->m_bJackTimebaseEnabled;
	
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
		pAudioEngine->setNextState( AudioEngine::State::Ready );
		break;
		
	case JackTransportRolling: // Transport is playing
		pAudioEngine->setNextState( AudioEngine::State::Playing );
		break;

	case JackTransportStarting:
		// Waiting for sync ready. If there are slow-sync clients,
		// this can take more than one cycle.
		pAudioEngine->setNextState( AudioEngine::State::Ready );
		break;
		
	default:
		ERRORLOG( "Unknown jack transport state" );
	}
	
	if ( pHydrogen->getSong() == nullptr ) {
		// Expected behavior if Hydrogen is exited while playback is
		// still running.
		// DEBUGLOG( "No song set." );
		return;
	}

	if ( bTimebaseEnabled ) {
		// Update the status regrading JACK timebase master.
		if ( m_JackTransportState != JackTransportStopped ) {
			if ( m_nTimebaseTracking > 1 ) {
				m_nTimebaseTracking--;
			} else if ( m_nTimebaseTracking == 1 ) {
				// JackTimebaseCallback not called anymore -> timebase client
				m_nTimebaseTracking = 0;
				m_timebaseState = Timebase::Slave;
				EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
														static_cast<int>(Timebase::Slave) );
			}
		}
		if ( m_nTimebaseTracking == 0 && 
			 !( m_JackTransportPos.valid & JackPositionBBT ) ) {
			// No external timebase master anymore -> regular client
			m_nTimebaseTracking = -1;
			m_timebaseState = Timebase::None;
			EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
													static_cast<int>(Timebase::None) );
		} else if ( m_nTimebaseTracking < 0 && 
					( m_JackTransportPos.valid & JackPositionBBT ) ) {
			// External timebase master detected -> timebase client
			m_nTimebaseTracking = 0;
			m_timebaseState = Timebase::Slave;
			EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
													static_cast<int>(Timebase::Slave) );
		}
	}
	
	// The relocation could be either triggered by an user interaction
	// (e.g. clicking the forward button or clicking somewhere on the
	// timeline) or by a different JACK client.
	if ( ( pAudioEngine->getTransportPosition()->getFrame() -
		   pAudioEngine->getTransportPosition()->getFrameOffsetTempo() ) !=
		 m_JackTransportPos.frame ) {
		
		// DEBUGLOG( QString( "[relocation detected] frames: %1, offset: %2, Jack frames: %3" )
		// 		 .arg( pAudioEngine->getTransportPosition()->getFrame() )
		// 		 .arg( pAudioEngine->getTransportPosition()->getFrameOffsetTempo() )
		// 		 .arg( m_JackTransportPos.frame ) );
		
		if ( ! bTimebaseEnabled || m_timebaseState != Timebase::Slave ) {
			pAudioEngine->locateToFrame( m_JackTransportPos.frame );
		} else {
			relocateUsingBBT();
		}
	}

	if ( bTimebaseEnabled && m_timebaseState == Timebase::Slave ){
		m_previousJackTransportPos = m_JackTransportPos;
		
		// There is a JACK timebase master and it's not us. If it
		// provides a tempo that differs from the local one, we will
		// use the former instead.
		if ( pAudioEngine->getTransportPosition()->getBpm() !=
			 static_cast<float>(m_JackTransportPos.beats_per_minute ) ||
			 !compareAdjacentBBT() ) {
			relocateUsingBBT();
		}
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

float* JackAudioDriver::getTrackOut_L( std::shared_ptr<Instrument> instr, std::shared_ptr<InstrumentComponent> pCompo)
{
	return getTrackOut_L(m_trackMap[instr->get_id()][pCompo->get_drumkit_componentID()]);
}

float* JackAudioDriver::getTrackOut_R( std::shared_ptr<Instrument> instr, std::shared_ptr<InstrumentComponent> pCompo)
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
		m_pClient = jack_client_open( sClientName.toLocal8Bit(),
					      JackNullOption,
					      &status);

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
	jack_set_buffer_size_callback( m_pClient, jackDriverBufferSize, this );

	/* display an XRun event in the GUI.*/
	jack_set_xrun_callback( m_pClient, jackXRunCallback, nullptr );

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
	jack_set_property( m_pClient, jack_port_uuid( m_pOutputPort1 ),
					   JACK_METADATA_PRETTY_NAME, "Main Output L", "text/plain" );
	m_pOutputPort2 = jack_port_register( m_pClient, "out_R", JACK_DEFAULT_AUDIO_TYPE,
					    JackPortIsOutput, 0 );
	jack_set_property( m_pClient, jack_port_uuid( m_pOutputPort2 ),
					   JACK_METADATA_PRETTY_NAME, "Main Output R", "text/plain" );
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

	if ( pPreferences->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT &&
		 pPreferences->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER &&
		 pPreferences->m_bJackTimebaseEnabled ){
		initTimebaseMaster();
	}

	// TODO: Apart from the makeTrackOutputs() all other calls should
	// be redundant.
	//
	// Whenever there is a Song present, create per track outputs (if
	// activated in the Preferences).
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong != nullptr ) {
		makeTrackOutputs( pSong );
	}
	
	return 0;
}

void JackAudioDriver::makeTrackOutputs( std::shared_ptr<Song> pSong )
{
	if( Preferences::get_instance()->m_bJackTrackOuts == false ) {
		return;
	}

	auto pInstrumentList = pSong->getInstrumentList();
	std::shared_ptr<Instrument> pInstrument;
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
	std::shared_ptr<InstrumentComponent> pInstrumentComponent;
	for ( int n = 0; n <= nInstruments - 1; n++ ) {
		pInstrument = pInstrumentList->get( n );
		for ( auto& pInstrumentComponent : *pInstrument->get_components() ) {
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

void JackAudioDriver::setTrackOutput( int n, std::shared_ptr<Instrument> pInstrument, std::shared_ptr<InstrumentComponent> pInstrumentComponent, std::shared_ptr<Song> pSong )
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
	auto pDrumkitComponent = pSong->getComponent( pInstrumentComponent->get_drumkit_componentID() );
	sComponentName = QString( "Track_%1_%2_%3_" ).arg( n + 1 )
		.arg( pInstrument->get_name() ).arg( pDrumkitComponent->get_name() );

	// This differs from jack_port_set_name() by triggering
	// PortRename notifications to clients that have registered a
	// port rename handler.
	jack_port_rename( m_pClient, m_pTrackOutputPortsL[n], ( sComponentName + "L" ).toLocal8Bit() );
	jack_port_rename( m_pClient, m_pTrackOutputPortsR[n], ( sComponentName + "R" ).toLocal8Bit() );
}

void JackAudioDriver::startTransport()
{
	if ( m_pClient != nullptr ) {
		jack_transport_start( m_pClient );
	} else {
		ERRORLOG( "No client registered" );
	}
}

void JackAudioDriver::stopTransport()
{
	if ( m_pClient != nullptr ) {
		jack_transport_stop( m_pClient );
	} else {
		ERRORLOG( "No client registered" );
	}
}

void JackAudioDriver::locateTransport( long long nFrame )
{
	auto pHydrogen = Hydrogen::get_instance();
	
	if ( m_pClient != nullptr ) {
		// jack_transport_locate() (jack/transport.h )
		// re-positions the transport to a new frame number. May
		// be called at any time by any client.
		jack_transport_locate( m_pClient, nFrame );
	} else {
		ERRORLOG( "No client registered" );
	}
}

void JackAudioDriver::initTimebaseMaster()
{
	if ( m_pClient == nullptr ) {
		return;
	}
	
	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK timebase disabled in the Preferences" );
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
			WARNINGLOG( QString( "Hydrogen was not able to register itself as Timebase Master: [%1]" )
						.arg( nReturnValue ) );
		}
		else {
			m_nTimebaseTracking = 2;
			m_timebaseState = Timebase::Master;
			EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
													static_cast<int>(Timebase::Master) );
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

	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK timebase disabled in the Preferences" );
		return;
	}
	
	jack_release_timebase( m_pClient );
	
	if ( m_JackTransportPos.valid & JackPositionBBT ) {
		m_nTimebaseTracking = 0;
		m_timebaseState = Timebase::Slave;
		EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
												static_cast<int>(Timebase::Slave) );
	} else {
		m_nTimebaseTracking = -1;
		m_timebaseState = Timebase::None;
		EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
												static_cast<int>(Timebase::None) );
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pPos = pHydrogen->getAudioEngine()->getTransportPosition();
	if ( pSong == nullptr ) {
		// DEBUGLOG( "No song set." );
		return;
	}

	// Current pattern
	Pattern* pPattern;
	auto pPatternList = pHydrogen->getSong()->getPatternList();
	int nPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( nPatternNumber != -1 &&
		 nPatternNumber < pPatternList->size() ) {
		pPattern = pPatternList->get( nPatternNumber );
	}

	float fNumerator, fDenumerator, fTicksPerBar;
	if ( pPattern != nullptr ) {
		fNumerator = pPattern->get_length() *
			pPattern->get_denominator() / MAX_NOTES;
		fDenumerator = pPattern->get_denominator();
		fTicksPerBar = pPattern->get_length();
	} else {
		fNumerator = 4;
		fDenumerator = 4;
		fTicksPerBar = MAX_NOTES;
	}

	pJackPosition->ticks_per_beat = fTicksPerBar;
	pJackPosition->valid = JackPositionBBT;
	// Time signature "numerator"
	pJackPosition->beats_per_bar = fNumerator;
	// Time signature "denominator"
	pJackPosition->beat_type = fDenumerator;
	pJackPosition->beats_per_minute = static_cast<double>(pPos->getBpm());

	if ( pPos->getFrame() < 1 ) {
		pJackPosition->bar = 1;
		pJackPosition->beat = 1;
		pJackPosition->tick = 0;
		pJackPosition->bar_start_tick = 0;
	} else {
		// +1 since the counting bars starts at 1.
		pJackPosition->bar = pPos->getColumn() + 1;

		// Number of ticks that have elapsed between frame 0 and the
		// first beat of the next measure.
		pJackPosition->bar_start_tick = pPos->getPatternStartTick();

		pJackPosition->beat = pPos->getPatternTickPosition() /
			pJackPosition->ticks_per_beat;
		// +1 since the counting beats starts at 1.
		pJackPosition->beat++;

		// Counting ticks starts at 0.
		pJackPosition->tick = pPos->getPatternTickPosition();
				
	}

	// JackAudioDriver::printJackTransportPos( pJackPosition );
    
	// Tell Hydrogen it is still timebase master.
	pDriver->m_nTimebaseTracking = 2;
}

	
JackAudioDriver::Timebase JackAudioDriver::getTimebaseState() const {
	if ( Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		return m_timebaseState;
	}
	return Timebase::None;
}
float JackAudioDriver::getMasterBpm() const {
	if ( !( m_JackTransportPos.valid & JackPositionBBT ) ||
		 m_timebaseState != Timebase::Slave ) {
		return std::nan("no tempo, no masters");
	}
	
	return static_cast<float>(m_JackTransportPos.beats_per_minute );
}


int JackAudioDriver::getXRuns() const {
	return JackAudioDriver::jackServerXRuns;
}

void JackAudioDriver::printState() const {

	auto pHydrogen = Hydrogen::get_instance();
	
	printJackTransportPos( &m_JackTransportPos );
	
	std::cout << "\033[35m[Hydrogen] [JackAudioDriver state]"
			  << ", m_JackTransportState: " << m_JackTransportState
			  << ", m_timebaseState: " << static_cast<int>(m_timebaseState)
			  << ", current pattern column: "
			  << pHydrogen->getAudioEngine()->getTransportPosition()->getColumn()
			  << "\33[0m" << std::endl;
}


void JackAudioDriver::printJackTransportPos( const jack_position_t* pPos ) {
	std::cout << "\033[36m[Hydrogen] [JACK transport]"
			  << " frame: " << pPos->frame
			  << ", frame_rate: " << pPos->frame_rate
			  << std::hex << ", valid: 0x" << pPos->valid
			  << std::dec << ", bar: " << pPos->bar
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

bool JackAudioDriver::checkSupport() {

	bool bJackFound;

	// Classic JACK
	QString sCapture = checkExecutable( "jackd", "--version" );
	if ( ! sCapture.isEmpty() ) {
		bJackFound = true;
		INFOLOG( QString( "'jackd' of version [%1] found." )
				 .arg( sCapture ) );
	}

	// JACK compiled with DBus support (maybe this one is packaged but
	// the classical one isn't).
	//
	// `jackdbus` is supposed to be run by the DBus message daemon and
	// does not have proper CLI options. But it does not fail by
	// passing a `-h` either and this will serve for checking its
	// presence.
	sCapture = checkExecutable( "jackdbus", "-h" );
	if ( ! sCapture.isEmpty() ) {
		bJackFound = true;
		INFOLOG( "'jackdbus' found." );
	}

	// Pipewire JACK interface
	//
	// `pw-jack` has no version query CLI option (yet). But showing
	// the help will serve for checking its presence.
	sCapture = checkExecutable( "pw-jack", "-h" );
	if ( ! sCapture.isEmpty() ) {
		bJackFound = true;
		INFOLOG( "'pw-jack' found." );
	}

	return bJackFound;
}

QString JackAudioDriver::checkExecutable( const QString& sExecutable, const QString& sOption ) {
	QProcess process;
	process.start( sExecutable, QStringList( sOption ) );
	process.waitForFinished( -1 );

	if ( process.exitCode() != 0 ) {
		return "";
	}

	QString sStdout = process.readAllStandardOutput();
	if ( sStdout.isEmpty() ) {
		return "No output";
	}

	return sStdout.trimmed();
}
};

#endif // H2CORE_HAVE_JACK
