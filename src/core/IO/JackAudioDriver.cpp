/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
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

#define JACK_DEBUG 0

#define J_DEBUGLOG(x) if ( __logger->should_log( Logger::Debug ) ) { \
		__logger->log( Logger::Debug, _class_name(), __FUNCTION__, \
					   QString( "%1" ).arg( x ), "\033[37m" ); }

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

#if JACK_DEBUG
	___INFOLOG( "" );
#endif

	JackAudioDriver::pJackDriverInstance->m_pClient = nullptr;
	Hydrogen::get_instance()->getAudioEngine()->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}
int JackAudioDriver::jackXRunCallback( void *arg ) {
	UNUSED( arg );
	++JackAudioDriver::jackServerXRuns;

#if JACK_DEBUG
	___INFOLOG( QString( "New XRun. [%1] in total" )
			   .arg( JackAudioDriver::jackServerXRuns ) );
#endif

#ifdef HAVE_INTEGRATION_TESTS
	// Xruns do mess up the current transport position and we might get the same
	// frame two times in a row while the audio engine is already at a new
	// position.
	JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
#endif
	EventQueue::get_instance()->push_event( EVENT_XRUN, 0 );
	return 0;
}

unsigned long JackAudioDriver::jackServerSampleRate = 0;
int JackAudioDriver::jackServerXRuns = 0;
jack_nframes_t JackAudioDriver::jackServerBufferSize = 0;
#ifdef HAVE_INTEGRATION_TESTS
long JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
#endif
JackAudioDriver* JackAudioDriver::pJackDriverInstance = nullptr;

JackAudioDriver::JackAudioDriver( JackProcessCallback m_processCallback )
	: AudioOutput()
	, m_nTrackPortCount( 0 )
	, m_pClient( nullptr )
	, m_pOutputPort1( nullptr )
	, m_pOutputPort2( nullptr )
	, m_timebaseTracking( TimebaseTracking::None )
	, m_timebaseState( Timebase::None )
	, m_fLastTimebaseBpm( 120 )
	, m_nTimebaseFrameOffset( 0 )
	, m_lastTransportBits( 0 )
#ifdef HAVE_INTEGRATION_TESTS
	, m_bIntegrationRelocationLoop( false )
	, m_bIntegrationCheckRelocationLoop( false )
#endif
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
		Hydrogen::get_instance()->getAudioEngine()->raiseError( Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT );
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
			Hydrogen::get_instance()->getAudioEngine()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		if ( jack_connect( m_pClient, jack_port_name( m_pOutputPort1 ),
				   portnames[0] ) != 0 ||
		     jack_connect( m_pClient, jack_port_name( m_pOutputPort2 ),
				   portnames[1] ) != 0 ) {
			ERRORLOG( "Couldn't connect to first pair of Jack input ports" );
			Hydrogen::get_instance()->getAudioEngine()->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
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
			Hydrogen::get_instance()->getAudioEngine()->raiseError( Hydrogen::JACK_CANNOT_CLOSE_CLIENT );
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

const jack_position_t& JackAudioDriver::getJackPosition() const {
	return m_JackTransportPos;
}

bool JackAudioDriver::isBBTValid( const jack_position_t& pos ) {
	if ( ! ( pos.valid & JackPositionBBT ) ) {
		// No BBT information
		return false;
	}

	// Sometime the JACK server does send seemingly random nuisance.
	if ( pos.beat_type < 1 ||
		 pos.bar < 1 ||
		 pos.beat < 1 ||
		 pos.beat > pos.beats_per_bar ||
		 pos.beats_per_bar < 1 ||
		 pos.beats_per_minute < MIN_BPM ||
		 pos.beats_per_minute > MAX_BPM ||
		 pos.tick < 0 ||
		 pos.tick >= pos.ticks_per_beat ||
		 pos.ticks_per_beat < 1 ||
		 std::isnan( pos.bar_start_tick ) ||
		 std::isnan( pos.beats_per_bar ) ||
		 std::isnan( pos.beat_type ) ||
		 std::isnan( pos.ticks_per_beat ) ||
		 std::isnan( pos.beats_per_minute ) ) {
#if JACK_DEBUG
		J_DEBUGLOG( QString( "Invalid BBT content. beat_type: %1, bar: %2, beat: %3, tick: %4, beats_per_bar: %5, beats_per_minute: %6, ticks_per_beat: %7, bar_start_tick: %8, beat_type: %9" )
					.arg( pos.beat_type ).arg( pos.bar ).arg( pos.beat )
					.arg( pos.tick ).arg( pos.beats_per_bar )
					.arg( pos.beats_per_minute ).arg( pos.ticks_per_beat )
					.arg( pos.bar_start_tick ).arg( pos.beat_type ) );
#endif
		ERRORLOG( "Invalid timebase information. Hydrogen falls back to frame-based relocation. In case you encounter this error frequently, you might considering to disabling JACK timebase support in the Preferences in order to avoid glitches." );
		return false;
	}

	return true;
}

double JackAudioDriver::bbtToTick( const jack_position_t& pos ) {

	auto pHydrogen = Hydrogen::get_instance();

	int nResolution = Song::nDefaultResolution;
	Song::LoopMode loopMode = Song::LoopMode::Enabled;
	long nSongSizeInTicks = 0;
	auto pSong = pHydrogen->getSong();
	if ( pSong != nullptr ) {
		nResolution = pSong->getResolution();
		loopMode = pSong->getLoopMode();
		nSongSizeInTicks = pSong->lengthInTicks();
#if JACK_DEBUG
	} else {
		WARNINGLOG( "No song set" );
#endif
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();

	const double fTicksPerBeat =
		static_cast<double>( nResolution / pos.beat_type * 4 );

	bool bEndOfSongReached = false;
	long nBarTicks = 0;
	if ( pHydrogen->getMode() == Song::Mode::Song ) {

		// We disregard any relation between patterns/columns in Hydrogen
		// and the bar information provided by JACK. Instead, we assume a
		// constant measure for the whole song relocate to the tick encoded
		// in BBT information.
		//
		// We also have to convert between the tick size used within
		// Hydrogen and the one used by the current Timebase controller.
		nBarTicks = pos.bar_start_tick * ( fTicksPerBeat / pos.ticks_per_beat );

		// Check whether the resulting ticks exceeds the end of the song.
		if ( ( loopMode == Song::LoopMode::Disabled ||
			   loopMode == Song::LoopMode::Finishing ) &&
			 nBarTicks >= nSongSizeInTicks ) {
			bEndOfSongReached = true;
		}
	}

	double fNewTick;
	if ( bEndOfSongReached ) {
		fNewTick = -1;
#if JACK_DEBUG
		J_DEBUGLOG( "[end of song reached]" );
#endif
	}
	else {
		fNewTick = static_cast<double>(nBarTicks) +
			( pos.beat - 1 ) * fTicksPerBeat +
			pos.tick * ( fTicksPerBeat / pos.ticks_per_beat );
	}

#if JACK_DEBUG
	J_DEBUGLOG( QString( "Calculated tick [%1] from pos.bar: %2, nBarTicks: %3, pos.beat: %4, fTicksPerBeat: %5, pos.tick: %6, pos.ticks_per_beat: %7, bEndOfSongReached: %8" )
			  .arg( fNewTick ).arg( pos.bar ).arg( nBarTicks )
			  .arg( pos.beat ).arg( fTicksPerBeat )
			  .arg( pos.tick ).arg( pos.ticks_per_beat )
			  .arg( bEndOfSongReached ) );
#endif

	return fNewTick;
}

void JackAudioDriver::transportToBBT( const TransportPosition& transportPos,
									  jack_position_t* pJackPosition ) {
	int nResolution = Song::nDefaultResolution;
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		nResolution = pSong->getResolution();
#if JACK_DEBUG
	} else {
		WARNINGLOG( "No song set" );
#endif
	}

	// We use the longest playing pattern as reference.
	Pattern* pPattern = nullptr;
	int nPatternLength = 0;
	auto pPatternList = transportPos.getPlayingPatterns();
	for ( std::vector<Pattern*>::const_iterator ppPattern = pPatternList->cbegin();
		  ppPattern < pPatternList ->cend(); ppPattern++ ) {
		if ( (*ppPattern)->get_length() > nPatternLength ) {
			nPatternLength = (*ppPattern)->get_length();
			pPattern = *ppPattern;
		}

		for ( const auto& ppVirtualPattern : *(*ppPattern)->get_flattened_virtual_patterns() ) {
			if ( ppVirtualPattern->get_length() > nPatternLength ) {
				nPatternLength = ppVirtualPattern->get_length();
				pPattern = ppVirtualPattern;
			}
		}
	}

	float fNumerator, fDenumerator;
	if ( pPattern != nullptr ) {
		fNumerator = nPatternLength * pPattern->get_denominator() / MAX_NOTES;
		fDenumerator = pPattern->get_denominator();
	}
	else {
		fNumerator = 4;
		fDenumerator = 4;
	}
	const float fTicksPerBeat =
		static_cast<float>(nResolution) * 4 / fDenumerator;

	pJackPosition->frame_rate =
		Hydrogen::get_instance()->getAudioOutput()->getSampleRate();
	pJackPosition->ticks_per_beat = fTicksPerBeat;
	pJackPosition->valid = JackPositionBBT;
	// Time signature "numerator"
	pJackPosition->beats_per_bar = fNumerator;
	// Time signature "denominator"
	pJackPosition->beat_type = fDenumerator;
	pJackPosition->beats_per_minute = static_cast<double>(transportPos.getBpm());

	if ( transportPos.getFrame() < 1 || transportPos.getColumn() == -1 ) {
		// We have to be careful about column == -1. In song mode with loop mode
		// disabled Hydrogen will just stop transport and set column to -1 while
		// still holding the former tick and frame values. (This is important to
		// properly rendering fade outs and realtime event).
		pJackPosition->bar = 1;
		pJackPosition->beat = 1;
		pJackPosition->tick = 0;
		pJackPosition->bar_start_tick = 0;
	}
	else {
		// +1 since the counting bars starts at 1.
		pJackPosition->bar = transportPos.getColumn() + 1;

		// Number of ticks that have elapsed between frame 0 and the
		// first beat of the next measure.
		pJackPosition->bar_start_tick = transportPos.getPatternStartTick();

		pJackPosition->beat = static_cast<int>(std::floor(
			static_cast<float>(transportPos.getPatternTickPosition()) /
			static_cast<float>(pJackPosition->ticks_per_beat)));
		// +1 since the counting beats starts at 1.
		pJackPosition->beat++;

		// Counting ticks starts at 0.
		pJackPosition->tick = std::fmod(
			static_cast<double>(transportPos.getPatternTickPosition()),
			pJackPosition->ticks_per_beat );
	}
}

void JackAudioDriver::relocateUsingBBT()
{
	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK timebase disabled in the Preferences" );
		return;
	}
	if ( m_timebaseState != Timebase::Listener ) {
		ERRORLOG( QString( "Relocation using BBT information can only be used in the presence of another JACK Timebase controller" ) );
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pSong == nullptr ) {
		// Expected behavior if Hydrogen is exited while playback is
		// still running.
#if JACK_DEBUG
		J_DEBUGLOG( "No song set." );
#endif
		return;
	}


	const double fNewTick = bbtToTick( m_JackTransportPos );

	if ( fNewTick == -1 ) {
		// End of song reached.
		if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
			pAudioEngine->stop();
			pAudioEngine->stopPlayback();
		}

#if JACK_DEBUG
		J_DEBUGLOG( "Exceeding length of song. Locating back to start." );
#endif

		// It is important to relocate to the beginning of the song. If we would
		// stay at the end or beyond, Hydrogen would stop every attempt to start
		// playback again. And it's most probably not obvious to the user why it
		// does so.
		pAudioEngine->locate( 0, false );

		// Reset the offset as we loose information in truncating the transport
		// position.
		m_nTimebaseFrameOffset = 0;
	}
	else {

#if JACK_DEBUG
		J_DEBUGLOG( QString( "Locate to tick [%1]" ).arg( fNewTick ) );
#endif

		pAudioEngine->locate( fNewTick, false );
	}

	EventQueue::get_instance()->push_event( EVENT_RELOCATION, 0 );

	m_nTimebaseFrameOffset = pAudioEngine->getTransportPosition()->getFrame() -
		m_JackTransportPos.frame;

	return;
}

void JackAudioDriver::updateTransportPosition()
{
	if ( Preferences::get_instance()->m_nJackTransportMode !=
	     Preferences::USE_JACK_TRANSPORT ){
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	const bool bTimebaseEnabled = Preferences::get_instance()->m_bJackTimebaseEnabled;

#ifdef HAVE_INTEGRATION_TESTS
	const int nPreviousXruns = JackAudioDriver::jackServerXRuns;
#endif

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
#if JACK_DEBUG
		J_DEBUGLOG( "No song set." );
#endif
		return;
	}

	if ( m_JackTransportPos.valid & JackPositionBBT ) {
		m_fLastTimebaseBpm =
			static_cast<float>(m_JackTransportPos.beats_per_minute );
	}

#if JACK_DEBUG
	J_DEBUGLOG( QString( "JACK state: %1, TimebaseFrameOffset: %2, pos: %3" )
			  .arg( JackTransportStateToQString( m_JackTransportState ) )
			  .arg( m_nTimebaseFrameOffset )
			  .arg( JackTransportPosToQString( m_JackTransportPos ) ) );
	J_DEBUGLOG( QString( "Timebase state: %1, tracking: %2" )
			  .arg( TimebaseToQString( m_timebaseState ) )
			  .arg( TimebaseTrackingToQString( m_timebaseTracking ) ) );
#endif

	// We rely on the JackTimebaseCallback to give us a thumbs up every time it
	// is called. But since this is not happening while transport is stopped or
	// starting, we have to omit those cases.
	if ( bTimebaseEnabled && m_JackTransportState == JackTransportRolling ) {
		// Update the status regrading JACK Timebase.
		if ( m_timebaseState == Timebase::Controller ) {
			if ( m_timebaseTracking == TimebaseTracking::Valid ) {
				m_timebaseTracking = TimebaseTracking::OnHold;
			}
			else {
				// JackTimebaseCallback not called anymore -> timebase
				// listener/normal client
				m_timebaseTracking = TimebaseTracking::Valid;
				if ( m_JackTransportPos.valid & JackPositionBBT ) {
					m_timebaseState = Timebase::Listener;
				}
				else {
					m_timebaseState = Timebase::None;
				}

#if JACK_DEBUG
				J_DEBUGLOG( QString( "Updating Timebase [0] [%1] -> [%2]" )
							.arg( TimebaseToQString( Timebase::Controller ) )
							.arg( TimebaseToQString( m_timebaseState ) ) );
#endif

				m_nTimebaseFrameOffset = 0;
				EventQueue::get_instance()->push_event(
					EVENT_JACK_TIMEBASE_STATE_CHANGED,
					static_cast<int>(m_timebaseState) );
			}
		}
		else {
			// Update state with respect to an external Timebase controller
			if ( m_JackTransportPos.valid & JackPositionBBT ) {
				// There is an external controller
				if ( m_timebaseState != Timebase::Listener ) {

#if JACK_DEBUG
				J_DEBUGLOG( QString( "Updating Timebase [1] [%1] -> [%2]" )
							.arg( TimebaseToQString( m_timebaseState ) )
							.arg( TimebaseToQString( Timebase::Listener ) ) );
#endif

					m_timebaseState = Timebase::Listener;
					m_nTimebaseFrameOffset = 0;
					EventQueue::get_instance()->push_event(
						EVENT_JACK_TIMEBASE_STATE_CHANGED,
						static_cast<int>(m_timebaseState) );
				}
				if ( m_timebaseTracking != TimebaseTracking::Valid ) {
					m_timebaseTracking = TimebaseTracking::Valid;
				}
			}
			else {
				if ( m_timebaseState == Timebase::Listener &&
					 m_timebaseTracking == TimebaseTracking::Valid ) {
					// There might have been a relocation by another listener (or
					// us). We wait till the next processing cycle in order to
					// decide whether to drop the BBT support or not.
					m_timebaseTracking = TimebaseTracking::OnHold;
				}
				else {
					m_timebaseTracking = TimebaseTracking::Valid;

#if JACK_DEBUG
				J_DEBUGLOG( QString( "Updating Timebase [2] [%1] -> [%2]" )
							.arg( TimebaseToQString( m_timebaseState ) )
							.arg( TimebaseToQString( Timebase::None ) ) );
#endif

					m_timebaseState = Timebase::None;
					m_nTimebaseFrameOffset = 0;
					EventQueue::get_instance()->push_event(
						EVENT_JACK_TIMEBASE_STATE_CHANGED,
						static_cast<int>(m_timebaseState) );
				}
			}
		}
	}

	// The relocation could be either triggered by an user interaction
	// (e.g. clicking the forward button or clicking somewhere on the
	// timeline) or by a different JACK client.
	const bool bRelocation =
		( pAudioEngine->getTransportPosition()->getFrame() -
		  pAudioEngine->getTransportPosition()->getFrameOffsetTempo() -
		  m_nTimebaseFrameOffset ) != m_JackTransportPos.frame;
	if ( bRelocation || ( m_lastTransportBits != m_JackTransportPos.valid &&
						  isBBTValid( m_JackTransportPos ) ) ) {

#if JACK_DEBUG
		if ( bRelocation ) {
			J_DEBUGLOG( QString( "[relocation detected] frames: %1, offset: %2, Jack frames: %3, m_nTimebaseFrameOffset: %4, timebase mode: %5" )
						.arg( pAudioEngine->getTransportPosition()->getFrame() )
						.arg( pAudioEngine->getTransportPosition()->getFrameOffsetTempo() )
						.arg( m_JackTransportPos.frame )
						.arg( m_nTimebaseFrameOffset )
						.arg( TimebaseToQString( m_timebaseState ) ) );
		}
		else {
			J_DEBUGLOG( QString( "[BBT info available] update transport" ) );
		}
#endif

		if ( bTimebaseEnabled && m_timebaseState == Timebase::Listener &&
			 isBBTValid( m_JackTransportPos ) ) {
			relocateUsingBBT();
		}
		else {
			pAudioEngine->locateToFrame( m_JackTransportPos.frame );
			m_nTimebaseFrameOffset = 0;
		}

		m_lastTransportBits = m_JackTransportPos.valid;

#ifdef HAVE_INTEGRATION_TESTS
		// Used to check whether we can find the proper position right away
		// during the integration tests. If e.g. an offset is off, we get
		// trapped in a relocation loop.
		//
		// We only perform the check in case no XRun occurred over the course of
		// this function. They would mess things up.
		if ( m_bIntegrationCheckRelocationLoop && bRelocation &&
			 nPreviousXruns == JackAudioDriver::jackServerXRuns ) {
			if ( JackAudioDriver::m_nIntegrationLastRelocationFrame !=
				 m_JackTransportPos.frame ) {
				JackAudioDriver::m_nIntegrationLastRelocationFrame =
					m_JackTransportPos.frame;
			} else {
				ERRORLOG( QString( "Relocation Loop! [%1] is detected as relocation a second time." )
						  .arg( m_JackTransportPos.frame ) );
				m_bIntegrationRelocationLoop = true;
			}
		}
#endif

#if JACK_DEBUG
		J_DEBUGLOG( QString( "[relocation done] m_nTimebaseFrameOffset: %1, new pos: %2" )
					.arg( m_nTimebaseFrameOffset )
					.arg( pAudioEngine->getTransportPosition()->toQString() ) );
#endif
	}

	return;
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

float* JackAudioDriver::getTrackOut_L( std::shared_ptr<Instrument> instr,
									   int nComponentIdx )
{
	return getTrackOut_L(m_trackMap[instr->get_id()][ nComponentIdx ]);
}

float* JackAudioDriver::getTrackOut_R( std::shared_ptr<Instrument> instr,
									   int nComponentIdx )
{
	return getTrackOut_R(m_trackMap[instr->get_id()][ nComponentIdx ]);
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
	if ( jack_set_process_callback(
			 m_pClient, this->m_processCallback, nullptr ) != 0 ) {
		ERRORLOG( "Unable to set process callback" );
	}

	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/
	if ( jack_set_sample_rate_callback(
			 m_pClient, jackDriverSampleRate, this ) != 0 ) {
		ERRORLOG( "Unable to set sample rate callback" );
	}

	/* tell JACK server to update us if the buffer size
	   (frames per process cycle) changes.
	*/
	if ( jack_set_buffer_size_callback(
			 m_pClient, jackDriverBufferSize, this ) != 0 ) {
		ERRORLOG( "Unable to set buffersize callback" );
	}

	/* display an XRun event in the GUI.*/
	if ( jack_set_xrun_callback( m_pClient, jackXRunCallback, nullptr ) != 0 ) {
		ERRORLOG( "Unable to set buffersize callback" );
	}

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
	if ( jack_set_property( m_pClient, jack_port_uuid( m_pOutputPort1 ),
							JACK_METADATA_PRETTY_NAME, "Main Output L",
							"text/plain" ) != 0 ) {
		ERRORLOG( "Unable to set pretty name of left main output" );
	}
	m_pOutputPort2 = jack_port_register( m_pClient, "out_R", JACK_DEFAULT_AUDIO_TYPE,
					    JackPortIsOutput, 0 );
	if ( jack_set_property( m_pClient, jack_port_uuid( m_pOutputPort2 ),
							JACK_METADATA_PRETTY_NAME, "Main Output R",
							"text/plain" ) != 0 ) {
		ERRORLOG( "Unable to set pretty name of left main output" );
	}
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	if ( ( m_pOutputPort1 == nullptr ) || ( m_pOutputPort2 == nullptr ) ) {
		pHydrogen->getAudioEngine()->raiseError(
			Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
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

	if ( pPreferences->m_nJackTransportMode == Preferences::USE_JACK_TRANSPORT &&
		 pPreferences->m_bJackTimebaseMode == Preferences::USE_JACK_TIMEBASE_CONTROL &&
		 pPreferences->m_bJackTimebaseEnabled ){
		initTimebaseControl();
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

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
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
	std::shared_ptr<InstrumentComponent> ppComponent;
	for ( int n = 0; n <= nInstruments - 1; n++ ) {
		pInstrument = pInstrumentList->get( n );
		for ( int ii = 0; ii < pInstrument->get_components()->size(); ++ii ) {
			ppComponent = pInstrument->get_component( ii );
			if ( ppComponent == nullptr ) {
				continue;
			}

			setTrackOutput( nTrackCount, pInstrument, ppComponent, pSong);
			m_trackMap[ pInstrument->get_id() ][ ii ] = nTrackCount;
			nTrackCount++;
		}
	}
	// clean up unused ports
	jack_port_t *pPortL, *pPortR;
	for ( int n = nTrackCount; n < m_nTrackPortCount; n++ ) {
		pPortL = m_pTrackOutputPortsL[n];
		pPortR = m_pTrackOutputPortsR[n];
		m_pTrackOutputPortsL[n] = nullptr;
		if ( jack_port_unregister( m_pClient, pPortL ) != 0 ) {
			ERRORLOG( QString( "Unable to unregister left port [%1]" ).arg( n ) );
		}
		m_pTrackOutputPortsR[n] = nullptr;
		if ( jack_port_unregister( m_pClient, pPortR ) != 0 ) {
			ERRORLOG( QString( "Unable to unregister right port [%1]" ).arg( n ) );
		}
	}

	m_nTrackPortCount = nTrackCount;
}

void JackAudioDriver::setTrackOutput( int n, std::shared_ptr<Instrument> pInstrument, std::shared_ptr<InstrumentComponent> pInstrumentComponent, std::shared_ptr<Song> pSong )
{
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	if ( pInstrument == nullptr ) {
		ERRORLOG( "Invalid instrument" );
		return;
	}

	if ( pInstrumentComponent == nullptr ) {
		ERRORLOG( "Invalid instrument component" );
		return;
	}

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
				Hydrogen::get_instance()->getAudioEngine()->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}
		}
		m_nTrackPortCount = n + 1;
	}

	// Now that we're sure there is an n'th port, rename it.
	sComponentName = QString( "Track_%1_%2_%3_" ).arg( n + 1 )
		.arg( pInstrument->get_name() ).arg( pInstrumentComponent->getName() );

	if ( jack_port_rename( m_pClient, m_pTrackOutputPortsL[n],
						   ( sComponentName + "L" ).toLocal8Bit() ) != 0 ) {
		ERRORLOG( QString( "Unable to rename left port of track [%1] to [%2]" )
				  .arg( n ).arg( sComponentName + "L" ) );
	}
	if ( jack_port_rename( m_pClient, m_pTrackOutputPortsR[n],
					  ( sComponentName + "R" ).toLocal8Bit() ) != 0 ) {
		ERRORLOG( QString( "Unable to rename right port of track [%1] to [%2]" )
				  .arg( n ).arg( sComponentName + "R" ) );
	}
}

void JackAudioDriver::startTransport() {
#if JACK_DEBUG
	J_DEBUGLOG( "" );
#endif

	if ( m_pClient != nullptr ) {
		jack_transport_start( m_pClient );
	} else {
		ERRORLOG( "No client registered" );
	}
}

void JackAudioDriver::stopTransport() {
#if JACK_DEBUG
	J_DEBUGLOG( "" );
#endif

	if ( m_pClient != nullptr ) {
		jack_transport_stop( m_pClient );
	} else {
		ERRORLOG( "No client registered" );
	}
}

void JackAudioDriver::locateTransport( long long nFrame )
{
	const auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	if ( m_pClient != nullptr ) {
		if ( m_timebaseState == Timebase::Controller ) {
			// We have to provided all BBT information as well when relocating
			// as Timebase controller.
			m_nextJackTransportPos.frame = nFrame;
			transportToBBT( *pAudioEngine->getTransportPosition(),
							&m_nextJackTransportPos );
#if JACK_DEBUG
			J_DEBUGLOG( QString( "Relocate to position: %1" )
					  .arg( JackTransportPosToQString( m_nextJackTransportPos ) ) );
#endif

			if ( jack_transport_reposition( m_pClient,
											&m_nextJackTransportPos ) != 0 ) {
				ERRORLOG( QString( "Position rejected [%1]" )
						  .arg( JackTransportPosToQString( m_nextJackTransportPos ) ) );
			}
		}
		else {
			long long nNewFrame = nFrame;
			if ( m_timebaseState == Timebase::Listener ) {
				// We have to guard against negative values which themselves are
				// nothing bad. They just tell that time was rescaled by a
				// measure change in the controller in such a way, Hydrogen
				// expects the origin of transport beyond 0.
				nNewFrame = std::max( static_cast<long long>(0),
									  nFrame - m_nTimebaseFrameOffset );
			}
#if JACK_DEBUG
			J_DEBUGLOG( QString( "Relocate to nFrame: %1, nNewFrame: %2, m_nTimebaseFrameOffset: %3, timebase state: %4" )
					  .arg( nFrame ).arg( nNewFrame )
					  .arg( m_nTimebaseFrameOffset )
					  .arg( TimebaseToQString( m_timebaseState ) ) );
#endif

			// jack_transport_locate() (jack/transport.h )
			// re-positions the transport to a new frame number. May
			// be called at any time by any client.
			if ( jack_transport_locate( m_pClient, nNewFrame ) != 0 ) {
				ERRORLOG( QString( "Invalid relocation request to frame [%1]" )
						  .arg( nNewFrame ) );
			}
		}
	} else {
		ERRORLOG( "No client registered" );
	}
}

void JackAudioDriver::initTimebaseControl()
{
	if ( m_pClient == nullptr ) {
		ERRORLOG( "No client yet" );
		return;
	}

	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK Timebase disabled in the Preferences" );
		return;
	}

	auto pPreferences = Preferences::get_instance();
	if ( pPreferences->m_bJackTimebaseMode == Preferences::USE_JACK_TIMEBASE_CONTROL ) {
		int nReturnValue = jack_set_timebase_callback(m_pClient, 0,
						     JackTimebaseCallback, this);
		if ( nReturnValue != 0 ){
			pPreferences->m_bJackTimebaseMode = Preferences::NO_JACK_TIMEBASE_CONTROL;
			WARNINGLOG( QString( "Hydrogen was not able to register itself as Timebase controller: [%1]" )
						.arg( nReturnValue ) );
		}
		else {
			m_timebaseTracking = TimebaseTracking::Valid;

#if JACK_DEBUG
			J_DEBUGLOG( QString( "Updating Timebase [2] [%1] -> [%2]" )
						.arg( TimebaseToQString( m_timebaseState ) )
						.arg( TimebaseToQString( Timebase::Controller ) ) );
#endif

			m_timebaseState = Timebase::Controller;
			EventQueue::get_instance()->push_event(
				EVENT_JACK_TIMEBASE_STATE_CHANGED,
				static_cast<int>(m_timebaseState) );
		}
	}
	else {
		WARNINGLOG( "Timebase control should currently not be requested by Hydrogen" );
	    releaseTimebaseControl();
	}
}

void JackAudioDriver::releaseTimebaseControl()
{
	if ( m_pClient == nullptr ) {
		ERRORLOG( QString( "Not fully initialized yet" ) );
		return;
	}

	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		ERRORLOG( "This function should not have been called with JACK timebase disabled in the Preferences" );
		return;
	}

	if ( jack_release_timebase( m_pClient ) ) {
		ERRORLOG( "Unable to release Timebase control" );
	}

	m_timebaseTracking = TimebaseTracking::Valid;
	if ( m_JackTransportPos.valid & JackPositionBBT &&
		 m_timebaseState != Timebase::Controller ) {
		// Having an external controller while this function is called should be
		// rarely the case. But we still have to handle it.
		m_timebaseState = Timebase::Listener;
	}
	else {
		m_timebaseState = Timebase::None;
	}

#if JACK_DEBUG
	J_DEBUGLOG( QString( "Updating Timebase [%1] -> [%2]" )
				.arg( TimebaseToQString( Timebase::Controller ) )
				.arg( TimebaseToQString( m_timebaseState ) ) );
#endif

	EventQueue::get_instance()->push_event(
		EVENT_JACK_TIMEBASE_STATE_CHANGED, static_cast<int>(m_timebaseState) );

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

	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	std::shared_ptr<TransportPosition> pPos = nullptr;

	pAudioEngine->lock( RIGHT_HERE );

	const long long nInitialFrame = pJackPosition->frame;

	const auto posFromFrame = [&]( long long nFrame,
								   jack_position_t* pJackPosition ) {
		if ( nFrame == pAudioEngine->getTransportPosition()->getFrame() ) {
			// Requested transport position coincides with the current one of the
			// Audio Engine. We can reuse it.
			pPos = pAudioEngine->getTransportPosition();
		}
		else {
			pPos = std::make_shared<TransportPosition>( "JackTimebaseCallback" );
			const auto fTick = TransportPosition::computeTickFromFrame(
				nFrame );
			pAudioEngine->updateTransportPosition( fTick, nFrame, pPos );
		}

		transportToBBT( *pPos, pJackPosition );
	};

	// In the face of heavy load - can be triggered by enabling JackAudioDriver,
	// TransportPosition, and AudioEngine debug logs - XRuns occur and the frame
	// information provided by the JACK server glitches(!!!!). So, it just
	// changes under the hood in the pointer passed to this callback. The very
	// quantity we should calculate BBT information based on. Well we try a
	// second time in order to avoid glitches.
	posFromFrame( nInitialFrame, pJackPosition );

	if ( nInitialFrame != pJackPosition->frame ) {
		ERRORLOG( QString( "Provided frame glitched! Tring again using new one..." ) );
		const long long nSecondFrame = pJackPosition->frame;
		posFromFrame( nSecondFrame, pJackPosition );
	}

	// Tell Hydrogen it is still in Timebase control.
	if ( pDriver->m_timebaseTracking != TimebaseTracking::Valid ) {
		pDriver->m_timebaseTracking = TimebaseTracking::Valid;
	}
	if ( pDriver->m_timebaseState != Timebase::Controller ) {

#if JACK_DEBUG
		J_DEBUGLOG( QString( "Updating Timebase [%1] -> [%2]" )
					.arg( TimebaseToQString( pDriver->m_timebaseState ) )
					.arg( TimebaseToQString( Timebase::Controller ) ) );
#endif

		pDriver->m_timebaseState = Timebase::Controller;

		EventQueue::get_instance()->push_event(
			EVENT_JACK_TIMEBASE_STATE_CHANGED,
			static_cast<int>(pDriver->m_timebaseState) );
	}

#if JACK_DEBUG
	J_DEBUGLOG( QString( "Interal transport pos: %1" )
			  .arg( pPos->toQString() ) );
	J_DEBUGLOG( QString( "JACK pos: %1" )
			  .arg( JackTransportPosToQString( *pJackPosition ) ) );
#endif

	pAudioEngine->unlock();
}


JackAudioDriver::Timebase JackAudioDriver::getTimebaseState() const {
	if ( Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		return m_timebaseState;
	}
	return Timebase::None;
}

float JackAudioDriver::getTimebaseControllerBpm() const {
	if ( m_timebaseState != Timebase::Listener ) {
		return std::nan("no tempo, no masters");
	}

	return m_fLastTimebaseBpm;
}

int JackAudioDriver::getXRuns() const {
	return JackAudioDriver::jackServerXRuns;
}

void JackAudioDriver::printState() const {

	auto pHydrogen = Hydrogen::get_instance();

	J_DEBUGLOG( QString( "m_JackTransportState: %1,\n m_JackTransportPos: %2,\nm_timebaseState: %3, current pattern column: %4" )
			  .arg( m_JackTransportState )
			  .arg( JackTransportPosToQString( m_JackTransportPos ) )
			  .arg( static_cast<int>(m_timebaseState) )
			  .arg( pHydrogen->getAudioEngine()->getTransportPosition()->
					getColumn() ) );
}

QString JackAudioDriver::JackTransportPosToQString( const jack_position_t& pos ) {
	return QString( "frame: %1, frame_rate: %2, valid: %3, bar: %4, beat: %5, tick: %6, bar_start_tick: %7, beats_per_bar: %8, beat_type: %9, ticks_per_beat: %10, beats_per_minute: %11, frame_time: %12, next_time: %13" )
		.arg( pos.frame ).arg( pos.frame_rate )
		.arg( pos.valid, 8, 16, QLatin1Char( '0' ) ) // hex value
		.arg( pos.bar ).arg( pos.beat ).arg( pos.tick )
		.arg( pos.bar_start_tick ).arg( pos.beats_per_bar )
		.arg( pos.beat_type ).arg( pos.ticks_per_beat )
		.arg( pos.beats_per_minute ).arg( pos.frame_time )
		.arg( pos.next_time );
}

QString JackAudioDriver::TimebaseToQString( const JackAudioDriver::Timebase& t ) {
	switch( t ) {
		case Timebase::None:
			return "None";
		case Timebase::Listener:
			return "Listener";
		case Timebase::Controller:
			return "Controller";
		default:
			return "Unknown";
	}
}
QString JackAudioDriver::JackTransportStateToQString( const jack_transport_state_t& t ) {
	switch( t ) {
		case JackTransportStopped:
			return "Stopped";
		case JackTransportRolling:
			return "Rolling";
		case JackTransportStarting:
			return "Starting";
		case JackTransportLooping:
			// Only used in old systems.
			return "Looping";
		default:
			// There is the JackTransportStarting state too. But we do not cover
			// it here (yet) for the sake of backward compatibility.
			return QString( "Unknown JackTransportState [%1]" )
				.arg( static_cast<int>(t) );

	}
}

QString JackAudioDriver::TimebaseTrackingToQString( const TimebaseTracking& t ) {
	switch( t ) {
		case TimebaseTracking::Valid:
			return "Valid";
		case TimebaseTracking::OnHold:
			return "OnHold";
		case TimebaseTracking::None:
			return "None";
		default:
			return "Unknown";
	}
}
JackAudioDriver::Timebase JackAudioDriver::TimebaseFromInt( int nState ) {
	switch( nState ) {
	case static_cast<int>(Timebase::Listener):
		return Timebase::Listener;
	case static_cast<int>(Timebase::Controller):
		return Timebase::Controller;
	case static_cast<int>(Timebase::None):
	default:
		return Timebase::None;
	}
}

QString JackAudioDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[JackAudioDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sOutputPortName1: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sOutputPortName1 ) )
			.append( QString( "%1%2m_sOutputPortName2: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sOutputPortName2 ) )
			.append( QString( "%1%2m_trackMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( int rrow = 0; rrow < MAX_INSTRUMENTS; ++rrow ) {
			sOutput.append( QString( "%1%2%2[" ).arg( sPrefix ).arg( s ) );
			for ( int ccolumn = 0; ccolumn < MAX_COMPONENTS; ++ccolumn ) {
				sOutput.append( QString( "%1, " )
								.arg( m_trackMap[ rrow ][ ccolumn ] ) );
			}
			sOutput.append( "]\n" );
		}
		sOutput.append( QString( "%1%2m_nTrackPortCount: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nTrackPortCount ) )
			.append( QString( "%1%2m_JackTransportState: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( JackTransportStateToQString( m_JackTransportState ) ) )
			.append( QString( "%1%2m_JackTransportPos: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( JackTransportPosToQString( m_JackTransportPos ) ) )
			.append( QString( "%1%2m_nextJackTransportPos: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( JackTransportPosToQString( m_nextJackTransportPos ) ) )
			.append( QString( "%1%2m_bConnectDefaults: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bConnectDefaults ) )
			.append( QString( "%1%2m_timebaseState: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( TimebaseToQString( m_timebaseState ) ) )
			.append( QString( "%1%2m_timebaseTracking: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( TimebaseTrackingToQString( m_timebaseTracking ) ) )
			.append( QString( "%1%2m_fLastTimebaseBpm: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fLastTimebaseBpm ) )
			.append( QString( "%1%2m_nTimebaseFrameOffset: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nTimebaseFrameOffset ) )
			.append( QString( "%1%2m_lastTransportBits: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_lastTransportBits ) );
#ifdef HAVE_INTEGRATION_TESTS
			sOutput.append( QString( "%1%2m_nIntegrationLastRelocationFrame: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nIntegrationLastRelocationFrame ) )
			.append( QString( "%1%2m_bIntegrationRelocationLoop: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIntegrationRelocationLoop ) )
			.append( QString( "%1%2m_bIntegrationCheckRelocationLoop: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIntegrationCheckRelocationLoop ) );
#endif
	} else {
		sOutput = QString( "[JackAudioDriver]" ).arg( sPrefix )
			.append( QString( " m_sOutputPortName1: %1" )
					 .arg( m_sOutputPortName1 ) )
			.append( QString( ", m_sOutputPortName2: %1" )
					 .arg( m_sOutputPortName2 ) )
			.append( ", m_trackMap: [" );
		for ( int rrow = 0; rrow < MAX_INSTRUMENTS; ++rrow ) {
			sOutput.append( QString( "[" ) );
			for ( int ccolumn = 0; ccolumn < MAX_COMPONENTS; ++ccolumn ) {
				sOutput.append( QString( "%1, " )
								.arg( m_trackMap[ rrow ][ ccolumn ] ) );
			}
			sOutput.append( "] " );
		}
		sOutput.append( QString( ", m_nTrackPortCount: %1" )
					 .arg( m_nTrackPortCount ) )
			.append( QString( ", m_JackTransportState: %1" )
					 .arg( JackTransportStateToQString( m_JackTransportState ) ) )
			.append( QString( ", m_JackTransportPos: %1" )
					 .arg( JackTransportPosToQString( m_JackTransportPos ) ) )
			.append( QString( ", m_nextJackTransportPos: %1" )
					 .arg( JackTransportPosToQString( m_nextJackTransportPos ) ) )
			.append( QString( ", m_bConnectDefaults: %1" )
					 .arg( m_bConnectDefaults ) )
			.append( QString( ", m_timebaseState: %1" )
					 .arg( TimebaseToQString( m_timebaseState ) ) )
			.append( QString( ", m_timebaseTracking: %1" )
					 .arg( TimebaseTrackingToQString( m_timebaseTracking ) ) )
			.append( QString( ", m_fLastTimebaseBpm: %1" )
					 .arg( m_fLastTimebaseBpm ) )
			.append( QString( ", m_nTimebaseFrameOffset: %1" )
					 .arg( m_nTimebaseFrameOffset ) )
			.append( QString( ", m_lastTransportBits: %1" )
					 .arg( m_lastTransportBits ) );
#ifdef HAVE_INTEGRATION_TESTS
			sOutput.append( QString( ", m_nIntegrationLastRelocationFrame: %1" )
					 .arg( m_nIntegrationLastRelocationFrame ) )
			.append( QString( ", m_bIntegrationRelocationLoop: %1" )
					 .arg( m_bIntegrationRelocationLoop ) )
			.append( QString( ", m_bIntegrationCheckRelocationLoop: %1" )
					 .arg( m_bIntegrationCheckRelocationLoop ) );
#endif
	}

	return sOutput;
}
};

#endif // H2CORE_HAVE_JACK
