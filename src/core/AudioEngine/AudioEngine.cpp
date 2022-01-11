/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/AudioEngine/AudioEngine.h>

#ifdef WIN32
#    include "core/Timehelper.h"
#else
#    include <unistd.h>
#    include <sys/time.h>
#endif

#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Basics/Song.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Sampler/Sampler.h>
#include <core/Helpers/Filesystem.h>

#include <core/IO/AudioOutput.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/NullDriver.h>
#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>
#include <core/IO/CoreMidiDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/FakeDriver.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/JackMidiDriver.h>
#include <core/IO/PortMidiDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/PulseAudioDriver.h>

#include <core/Hydrogen.h>	// TODO: remove this line as soon as possible
#include <core/Preferences/Preferences.h>
#include <cassert>
#include <limits>
#include <random>

namespace H2Core
{

const int AudioEngine::nMaxTimeHumanize = 2000;

inline int randomValue( int max )
{
	return rand() % max;
}

inline float getGaussian( float z )
{
	// gaussian distribution -- dimss
	float x1, x2, w;
	do {
		x1 = 2.0 * ( ( ( float ) rand() ) / static_cast<float>(RAND_MAX) ) - 1.0;
		x2 = 2.0 * ( ( ( float ) rand() ) / static_cast<float>(RAND_MAX) ) - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );

	w = sqrtf( ( -2.0 * logf( w ) ) / w );
	return x1 * w * z + 0.0; // tunable
}


/** Gets the current time.
 * \return Current time obtained by gettimeofday()*/
inline timeval currentTime2()
{
	struct timeval now;
	gettimeofday( &now, nullptr );
	return now;
}

AudioEngine::AudioEngine()
		: TransportInfo()
		, m_pSampler( nullptr )
		, m_pSynth( nullptr )
		, m_pAudioDriver( nullptr )
		, m_pMidiDriver( nullptr )
		, m_pMidiDriverOut( nullptr )
		, m_state( State::Initialized )
		, m_pMetronomeInstrument( nullptr )
		, m_nPatternStartTick( -1 )
		, m_nPatternTickPosition( 0 )
		, m_fSongSizeInTicks( 0 )
		, m_nRealtimeFrames( 0 )
		, m_nAddRealtimeNoteTickPosition( 0 )
		, m_fMasterPeak_L( 0.0f )
		, m_fMasterPeak_R( 0.0f )
		, m_nColumn( -1 )
		, m_nextState( State::Ready )
		, m_fProcessTime( 0.0f )
		, m_fMaxProcessTime( 0.0f )
		, m_fNextBpm( 120 )
		, m_fTickOffset( 0 )
		, m_fLastTickIntervalEnd( -1 )
		, m_nLastPlayingPatternsColumn( -1 )
		, m_nFrameOffset( 0 )
{

	m_pSampler = new Sampler;
	m_pSynth = new Synth;
	
	gettimeofday( &m_currentTickTime, nullptr );
	
	m_pEventQueue = EventQueue::get_instance();
	
	srand( time( nullptr ) );

	// Create metronome instrument
	// Get the path to the file of the metronome sound.
	QString sMetronomeFilename = Filesystem::click_file_path();
	m_pMetronomeInstrument = std::make_shared<Instrument>( METRONOME_INSTR_ID, "metronome" );
	
	auto pLayer = std::make_shared<InstrumentLayer>( Sample::load( sMetronomeFilename ) );
	auto pCompo = std::make_shared<InstrumentComponent>( 0 );
	pCompo->set_layer(pLayer, 0);
	m_pMetronomeInstrument->get_components()->push_back( pCompo );
	m_pMetronomeInstrument->set_is_metronome_instrument(true);
	
	m_pPlayingPatterns = new PatternList();
	m_pPlayingPatterns->setNeedsLock( true );
	m_pNextPatterns = new PatternList();
	m_pNextPatterns->setNeedsLock( true );
	
	m_AudioProcessCallback = &audioEngine_process;

#ifdef H2CORE_HAVE_LADSPA
	Effects::create_instance();
#endif

}

AudioEngine::~AudioEngine()
{
	stopAudioDrivers();
	if ( getState() != State::Initialized ) {
		___ERRORLOG( "Error the audio engine is not in State::Initialized" );
		return;
	}
	m_pSampler->stopPlayingNotes();

	this->lock( RIGHT_HERE );
	___INFOLOG( "*** Hydrogen audio engine shutdown ***" );

	clearNoteQueue();

	// change the current audio engine state
	setState( State::Uninitialized );

	EventQueue::get_instance()->push_event( EVENT_STATE, static_cast<int>(State::Uninitialized) );

	delete m_pPlayingPatterns;
	m_pPlayingPatterns = nullptr;

	delete m_pNextPatterns;
	m_pNextPatterns = nullptr;

	m_pMetronomeInstrument = nullptr;

	this->unlock();
	
#ifdef H2CORE_HAVE_LADSPA
	delete Effects::get_instance();
#endif

//	delete Sequencer::get_instance();
	delete m_pSampler;
	delete m_pSynth;
}

Sampler* AudioEngine::getSampler() const
{
	assert(m_pSampler);
	return m_pSampler;
}

Synth* AudioEngine::getSynth() const
{
	assert(m_pSynth);
	return m_pSynth;
}

void AudioEngine::lock( const char* file, unsigned int line, const char* function )
{
	#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "by %1 : %2 : %3" ).arg( function ).arg( line ).arg( file ) );
	}
	#endif

	m_EngineMutex.lock();
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_LockingThread = std::this_thread::get_id();
}

bool AudioEngine::tryLock( const char* file, unsigned int line, const char* function )
{
	#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "by %1 : %2 : %3" ).arg( function ).arg( line ).arg( file ) );
	}
	#endif
	bool res = m_EngineMutex.try_lock();
	if ( !res ) {
		// Lock not obtained
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_LockingThread = std::this_thread::get_id();
	#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__, QString( "locked" ) );
	}
	#endif
	return true;
}

bool AudioEngine::tryLockFor( std::chrono::microseconds duration, const char* file, unsigned int line, const char* function )
{
	#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "by %1 : %2 : %3" ).arg( function ).arg( line ).arg( file ) );
	}
	#endif
	bool res = m_EngineMutex.try_lock_for( duration );
	if ( !res ) {
		// Lock not obtained
		WARNINGLOG( QString( "Lock timeout: lock timeout %1:%2:%3, lock held by %4:%5:%6" )
					.arg( file ).arg( function ).arg( line )
					.arg( __locker.file ).arg( __locker.function ).arg( __locker.line ));
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_LockingThread = std::this_thread::get_id();
	
	#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__, QString( "locked" ) );
	}
	#endif
	return true;
}

void AudioEngine::unlock()
{
	// Leave "__locker" dirty.
	m_LockingThread = std::thread::id();
	m_EngineMutex.unlock();
	#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__, QString( "" ) );
	}
	#endif
}

void AudioEngine::startPlayback()
{
	___INFOLOG( "" );

	// check current state
	if ( getState() != State::Ready ) {
		___ERRORLOG( "Error the audio engine is not in State::Ready" );
		return;
	}

	// change the current audio engine state
	setState( State::Playing );
	m_pEventQueue->push_event( EVENT_STATE, static_cast<int>(State::Playing) );
}

void AudioEngine::stopPlayback()
{
	___INFOLOG( "" );

	// check current state
	if ( getState() != State::Playing ) {
		___ERRORLOG( QString( "Error the audio engine is not in State::Playing but [%1]" )
					 .arg( static_cast<int>( getState() ) ) );
		return;
	}

	// change the current audio engine state
	setState( State::Ready );
	m_pEventQueue->push_event( EVENT_STATE, static_cast<int>(State::Ready) );
}

void AudioEngine::reset( bool bWithJackBroadcast ) {
	
	const auto pHydrogen = Hydrogen::get_instance();
	
	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;

	setFrames( 0 );
	setTick( 0 );
	setColumn( -1 );
	m_nPatternStartTick = -1;
	m_nPatternTickPosition = 0;
	m_fTickOffset = 0;
	m_nFrameOffset = 0;
	m_fLastTickIntervalEnd = -1;
	m_nLastPlayingPatternsColumn = -1;

	updateBpmAndTickSize();
	
	clearNoteQueue();

	
#ifdef H2CORE_HAVE_JACK
	if ( pHydrogen->haveJackTransport() && bWithJackBroadcast ) {
		// Tell all other JACK clients to relocate as well. This has
		// to be called after updateFrames().
		static_cast<JackAudioDriver*>( m_pAudioDriver )->locateTransport( 0 );
	}
#endif
}

float AudioEngine::computeTickSize( const int nSampleRate, const float fBpm, const int nResolution)
{
	float fTickSize = nSampleRate * 60.0 / fBpm / nResolution;
	
	return fTickSize;
}

long long AudioEngine::computeFrame( double fTick, float fTickSize ) {
	return std::round( fTick * fTickSize );
}

double AudioEngine::computeTick( long long nFrame, float fTickSize ) {
	return nFrame / fTickSize;
}

float AudioEngine::getElapsedTime() const {
	
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pDriver = pHydrogen->getAudioOutput();
	assert( pDriver );
	
	if ( pDriver->getSampleRate() == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		return 0;
	}

	return getFrames() / static_cast<float>(pDriver->getSampleRate());
}

void AudioEngine::locate( const double fTick, bool bWithJackBroadcast ) {
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pDriver = pHydrogen->getAudioOutput();

	// We relocate transport to the exact position of the tick
	m_fLastTickIntervalEnd = -1;
	m_nLastPlayingPatternsColumn = -1;
	m_nFrameOffset = 0;

	long long nNewFrame = computeFrameFromTick( fTick, &m_fTickOffset );
	setFrames( nNewFrame );
	updateTransportPosition( fTick, false );
	
#ifdef H2CORE_HAVE_JACK
	if ( pHydrogen->haveJackTransport() && bWithJackBroadcast ) {
		// Tell all other JACK clients to relocate as well. This has
		// to be called after updateFrames().
		static_cast<JackAudioDriver*>( m_pAudioDriver )->locateTransport( getFrames() );
	}
#endif
}

void AudioEngine::locateToFrame( const long long nFrame ) {
	const auto pHydrogen = Hydrogen::get_instance();

	setFrames( nFrame );

	double fNewTick = computeTickFromFrame( nFrame );
	m_fTickOffset = 0;
	m_nFrameOffset = 0;
	m_fLastTickIntervalEnd = -1;
	m_nLastPlayingPatternsColumn = -1;
	
	updateTransportPosition( fNewTick, pHydrogen->getSong()->isLoopEnabled() );
}

void AudioEngine::incrementTransportPosition( uint32_t nFrames ) {

	auto pSong = Hydrogen::get_instance()->getSong();

	if ( pSong == nullptr ) {
		return;
	}	

	setFrames( getFrames() + nFrames );

	double fNewTick = computeTickFromFrame( getFrames() );
	m_fTickOffset = 0;
		
	// DEBUGLOG( QString( "nFrames: %1, old frames: %2, getDoubleTick(): %3, newTick: %4, ticksize: %5" )
	// 		  .arg( nFrames )
	// 		  .arg( getFrames() - nFrames )
	// 		  .arg( getDoubleTick(), 0, 'f' )
	// 		  .arg( fNewTick, 0, 'f' )
	// 		  .arg( getTickSize(), 0, 'f' ) );

	updateTransportPosition( fNewTick, pSong->isLoopEnabled() );
}

void AudioEngine::updateTransportPosition( double fTick, bool bUseLoopMode ) {
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	const auto pDriver = pHydrogen->getAudioOutput();

	assert( pSong );

	// WARNINGLOG( QString( "[Before] tick: %1, pTickPos: %2, pStartPos: %3" )
	// 			.arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick ) );
	
	setTick( fTick );

	// Column, tick since the beginning of the current pattern, and
	// number of ticks till be the beginning of the current pattern
	// corresponding to nTick.
	int nNewColumn;
	if ( pHydrogen->getMode() == Song::Mode::Song ) {

		nNewColumn = pHydrogen->getColumnForTick( std::floor( fTick ),
												  bUseLoopMode,
												  &m_nPatternStartTick );

		if ( fTick > m_fSongSizeInTicks &&
			 m_fSongSizeInTicks != 0 ) {
			// When using the JACK audio driver the overall
			// transport position will be managed by an external
			// server. Since it is agnostic of all the looping in
			// its clients, it will only increment time and
			// Hydrogen has to take care of the looping itself. 
			m_nPatternTickPosition =
				std::fmod( std::floor( fTick ) - m_nPatternStartTick,
						   m_fSongSizeInTicks );
		} else {
			m_nPatternTickPosition = std::floor( fTick ) - m_nPatternStartTick;
		}

		if ( m_nColumn != nNewColumn ) {
			setColumn( nNewColumn );
			EventQueue::get_instance()->push_event( EVENT_COLUMN_CHANGED, 0 );
		}

	} else if ( pHydrogen->getMode() == Song::Mode::Pattern ) {

		int nPatternSize;
		if ( m_pPlayingPatterns->size() != 0 ) {
			nPatternSize = m_pPlayingPatterns->longest_pattern_length();
		} else {
			nPatternSize = MAX_NOTES;
		}

		// Transport went past the end of the pattern or Pattern mode
		// was just activated.
		if ( m_nPatternStartTick == -1 ||
			 fTick >= m_nPatternStartTick + nPatternSize  ) {
			if ( nPatternSize > 0 ) {
			m_nPatternStartTick +=
				std::floor( ( std::floor( fTick ) - m_nPatternStartTick ) /
							nPatternSize ) * nPatternSize;
			} else {
				m_nPatternStartTick = std::floor( fTick );
			}
		}

		m_nPatternTickPosition = static_cast<long>(std::floor( fTick ))
			- m_nPatternStartTick;
		if ( nPatternSize > 0 && m_nPatternTickPosition > nPatternSize ) {
			m_nPatternTickPosition = ( static_cast<long>(std::floor( fTick ))
									   - m_nPatternStartTick ) %
				nPatternSize;
		}
	}
	updateBpmAndTickSize();
	
	// WARNINGLOG( QString( "[After] tick: %1, pTickPos: %2, pStartPos: %3" )
	// 			.arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick ) );
	
}

void AudioEngine::updateBpmAndTickSize( bool bRunInPreparedState ) {
	if ( bRunInPreparedState && m_state == State::Prepared ) {
		// keep running when directly called during testings
	} else if ( m_state != State::Playing && m_state != State::Ready ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	float fNewBpm = getBpmAtColumn( pHydrogen->getAudioEngine()->getColumn() );
	if ( fNewBpm != getBpm() ) {
		setBpm( fNewBpm );
		EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, 0 );
	}

	float fOldTickSize = getTickSize();
	float fNewTickSize = AudioEngine::computeTickSize( static_cast<float>(m_pAudioDriver->getSampleRate()),
													   getBpm(), pSong->getResolution() );

	// DEBUGLOG(QString( "sample rate: %1, tick size: %2, bpm: %3" )
	// 		 .arg( static_cast<float>(m_pAudioDriver->getSampleRate()))
	// 		 .arg( fNewTickSize, 0, 'f' )
	// 		 .arg( getBpm() ), 0, 'f' );
	
	// Nothing changed - avoid recomputing
	if ( fNewTickSize == fOldTickSize ) {
		return;
	}

	if ( fNewTickSize == 0 ) {
		ERRORLOG( QString( "Something went wrong while calculating the tick size. [oldTS: %1, newTS: %2]" )
				  .arg( fOldTickSize, 0, 'f' ).arg( fNewTickSize, 0, 'f' ) );
		return;
	}
	
	setTickSize( fNewTickSize ); 

	// If we deal with a single speed for the whole song, the frames
	// since the beginning of the song are tempo-dependent and have to
	// be recalculated.
	if ( ! pHydrogen->isTimelineEnabled() ) {
		long long nNewFrames = computeFrameFromTick( getDoubleTick(), &m_fTickOffset );
		m_nFrameOffset = nNewFrames - getFrames() + m_nFrameOffset;

		// DEBUGLOG( QString( "old frame: %1, new frame: %2, tick: %3, old tick size: %4, new tick size: %5" )
		// 		  .arg( getFrames() ).arg( nNewFrames ).arg( getDoubleTick(), 0, 'f' )
		// 		  .arg( fOldTickSize, 0, 'f' ).arg( fNewTickSize, 0, 'f' ) );
		
		setFrames( nNewFrames );
	}

	if ( fOldTickSize == 0 ) {
		ERRORLOG( QString( "Previous tick size was invalid. No rescaling is performed. [oldTS: %1, newTS: %2]." )
				  .arg( fOldTickSize, 0, 'f' ).arg( fNewTickSize, 0, 'f' ) );
		return;
	}
}
				
// This function uses the assumption that sample rate and resolution
// are constant over the whole song.
long long AudioEngine::computeFrameFromTick( const double fTick, double* fTickOffset, int nSampleRate ) const {

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	assert( pSong );

	if ( nSampleRate == 0 ) {
		nSampleRate = pHydrogen->getAudioOutput()->getSampleRate();
	}
	const int nResolution = pSong->getResolution();

	const float fTickSize = AudioEngine::computeTickSize( nSampleRate,
													getBpm(),
													nResolution );
	
	if ( nSampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		*fTickOffset = 0;
		return 0;
	}

	if ( fTick == 0 ) {
		*fTickOffset = 0;
		return 0;
	}
		
	const auto tempoMarkers = pTimeline->getAllTempoMarkers();
	
	long long nNewFrames = 0;
	if ( pHydrogen->isTimelineEnabled() &&
		 ! ( tempoMarkers.size() == 1 &&
			 pTimeline->isFirstTempoMarkerSpecial() ) ) {

		double fNewTick = fTick;
		double fRemainingTicks = fTick;
		double fNextTick, fPassedTicks = 0;
		double fNextTickSize;
		double fNewFrames = 0;

		int nColumns = pSong->getPatternGroupVector()->size();

		while ( fRemainingTicks > 0 ) {
		
			for ( int ii = 1; ii <= tempoMarkers.size(); ++ii ) {
				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTick = m_fSongSizeInTicks;
				} else {
					fNextTick =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ) );
				}

				fNextTickSize =
					static_cast<double>(AudioEngine::computeTickSize( nSampleRate,
																	  tempoMarkers[ ii - 1 ]->fBpm,
																	  nResolution ));
				
				if ( fRemainingTicks >= ( fNextTick - fPassedTicks ) ) {
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the current transport position.
					fNewFrames += ( fNextTick - fPassedTicks ) * fNextTickSize;

					// DEBUGLOG( QString( "fTick: %1, fNewFrames: %2, nNextTick: %3, nRemainingTicks: %4, nPassedTicks: %5, fNextTickSize: %6, col: %7, bpm: %8" )
					// 		  .arg( fTick, 0, 'f' )
					// 		  .arg( fNewFrames, 0, 'f' )
					// 		  .arg( fNextTick, 0, 'f' )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fPassedTicks, 0, 'f' )
					// 		  .arg( fNextTickSize, 0, 'f' )
					// 		  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					// 		  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					// 		  );
					
					fRemainingTicks -= fNextTick - fPassedTicks;
					fPassedTicks = fNextTick;

				} else {
					// We are within this segment.
					fNewFrames += fRemainingTicks * fNextTickSize;

					nNewFrames = static_cast<long long>( std::round( fNewFrames ) );
					*fTickOffset = ( fNewFrames - static_cast<double>( nNewFrames ) ) /
						fNextTickSize;

					// DEBUGLOG( QString( "fTick: %1, fNewFrames: %2, nNewFrames: %9, fTickOffset: %10, nNextTick: %3, nRemainingTicks: %4, nPassedTicks: %5, fNextTickSize: %6, col: %7, bpm: %8" )
					// 		  .arg( fTick, 0, 'f' )
					// 		  .arg( fNewFrames, 0, 'f' )
					// 		  .arg( fNextTick, 0, 'f' )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fPassedTicks, 0, 'f' )
					// 		  .arg( fNextTickSize, 0, 'f' )
					// 		  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					// 		  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					// 		  .arg( nNewFrames )
					// 		  .arg( *fTickOffset, 0, 'f' )
					// 		  );

					fRemainingTicks -= fNewTick - fPassedTicks;
					break;
				}
			}

			if ( fRemainingTicks != 0 ) {
				// The provided fTick is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames (fNewFrames).
				int nRepetitions = std::floor(fTick / m_fSongSizeInTicks);

				// DEBUGLOG( QString( "nSongSizeInFrames: %1, nRepetitions: %2" )
				// 		  .arg( fNewFrames, 0, 'f' ).arg( nRepetitions ) );
				
				fNewFrames *= nRepetitions;
				fNewTick = std::fmod( fTick, m_fSongSizeInTicks );
				fRemainingTicks = fNewTick;
				fPassedTicks = 0;

				if ( std::isinf( fNewFrames ) ||
					 static_cast<long long>(fNewFrames) >
					 std::numeric_limits<long long>::max() ) {
					ERRORLOG( QString( "Provided ticks [%1] are too large." ).arg( fTick ) );
					return 0;
				}
			}
		}
	} else {
		
		// No Timeline but a single tempo for the whole song.
		double fNewFrames = static_cast<double>(fTick) *
			static_cast<double>(fTickSize);
		nNewFrames = static_cast<long long>( std::round( fNewFrames ) );
		*fTickOffset = ( fNewFrames - static_cast<double>(nNewFrames ) ) /
			static_cast<double>(fTickSize);

		// DEBUGLOG(QString("nNewFrames: %1, fTick: %2, fTickSize: %3, fTickOffset: %4" )
		// 		 .arg( nNewFrames ).arg( fTick, 0, 'f' ).arg( fTickSize, 0, 'f' )
		// 		 .arg( *fTickOffset, 0, 'f' ));
		
	}
	
	return nNewFrames;
}

double AudioEngine::computeTickFromFrame( const long long nFrame, int nSampleRate ) const {
	const auto pHydrogen = Hydrogen::get_instance();

	if ( nFrame < 0 ) {
		ERRORLOG( QString( "Provided frame [%1] must be non-negative" ).arg( nFrame ) );
	}
	
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	assert( pSong );

	if ( nSampleRate == 0 ) {
		nSampleRate = pHydrogen->getAudioOutput()->getSampleRate();
	}
	const int nResolution = pSong->getResolution();
	double fTick = 0;

	const float fTickSize = AudioEngine::computeTickSize( nSampleRate,
														  getBpm(),
														  nResolution );
	
	if ( nSampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		return fTick;
	}

	if ( nFrame == 0 ) {
		return fTick;
	}
		
	const auto tempoMarkers = pTimeline->getAllTempoMarkers();
	
	if ( pHydrogen->isTimelineEnabled() &&
		 ! ( tempoMarkers.size() == 1 &&
			 pTimeline->isFirstTempoMarkerSpecial() ) ) {

		// We are using double precision in here to avoid rounding
		// errors.
		double fRemainingFrames = static_cast<double>(nFrame);
		double fNextFrames = 0;
		double fNextTicks, fPassedTicks = 0;
		double fNextTickSize;
		long long nRemainingFrames;

		int nColumns = pSong->getPatternGroupVector()->size();

		while ( fRemainingFrames > 0 ) {
		
			for ( int ii = 1; ii <= tempoMarkers.size(); ++ii ) {

				fNextTickSize =
					static_cast<double>(AudioEngine::computeTickSize( nSampleRate,
																	  tempoMarkers[ ii - 1 ]->fBpm,
																	  nResolution ));
				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTicks = m_fSongSizeInTicks;
				} else {
					fNextTicks =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ));
				}
				fNextFrames = (fNextTicks - fPassedTicks) * fNextTickSize;

				// DEBUGLOG(QString( "[timeline] nFrame: %1, fTick: %11, sampleRate: %2, tickSize: %3, nNextTicks: %5, fNextFrames: %6, col: %7, bpm: %8, nPassedTicks: %9, fRemainingFrames: %10" )
				// 		 .arg( nFrame )
				// 		 .arg( nSampleRate )
				// 		 .arg( fNextTickSize, 0, 'f' )
				// 		 .arg( fNextTicks, 0, 'f' )
				// 		 .arg( fNextFrames, 0, 'f' )
				// 		 .arg( tempoMarkers[ ii -1 ]->nColumn )
				// 		 .arg( tempoMarkers[ ii -1 ]->fBpm )
				// 		 .arg( fPassedTicks, 0, 'f' )
				// 		 .arg( fRemainingFrames, 0, 'f' )
				// 		 .arg( fTick, 0, 'f' )
				// 		 );
		
				if ( fNextFrames < fRemainingFrames ) {
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the transport position.
					fTick += fNextTicks - fPassedTicks;

					fRemainingFrames -= fNextFrames;
					fPassedTicks = fNextTicks;

				} else {
					// We are within this segment.
					// We use a floor in here because only integers
					// frames are supported.
					double fNewTick = fRemainingFrames /
						fNextTickSize;

					// DEBUGLOG(QString( "%1 + %2 = %3" )
					// 		 .arg( fNewTick, 0 , 'f' )
					// 		 .arg( fTick, 0 , 'f' )
					// 		 .arg( fTick + fNewTick, 0 , 'f' ) );

					fTick += fNewTick;
											
					fRemainingFrames = 0;
					break;
				}
			}

			if ( fRemainingFrames != 0 ) {
				// The provided nFrame is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames.
				double fSongSizeInFrames = static_cast<double>(nFrame) -
					fRemainingFrames;
				int nRepetitions = std::floor(static_cast<double>(nFrame) / fSongSizeInFrames);
				if ( m_fSongSizeInTicks * nRepetitions >
					 std::numeric_limits<double>::max() ) {
					ERRORLOG( QString( "Provided frames [%1] are too large." ).arg( nFrame ) );
					return 0;
				}
				fTick = m_fSongSizeInTicks * nRepetitions;
						  
				fRemainingFrames =
					std::fmod( static_cast<double>(nFrame), fSongSizeInFrames );
				fPassedTicks = 0;

				// DEBUGLOG( QString( "fSongSizeInFrames: %1, fRemainingFrames: %4,  nRepetitions: %2, m_fSongSizeInTicks: %3" )
				// 		  .arg( fSongSizeInFrames, 0, 'f' ).arg( nRepetitions )
				// 		  .arg( m_fSongSizeInTicks, 0, 'f' ).arg( fRemainingFrames, 0, 'f' ) );
				
			}
		}
	} else {
	
		// No Timeline. Constant tempo/tick size for the whole song.
		fTick = static_cast<double>(nFrame) / static_cast<double>(fTickSize);

		// DEBUGLOG(QString( "[no timeline] nFrame: %1, sampleRate: %2, tickSize: %3" )
		// 		 .arg( nFrame ).arg( nSampleRate ).arg( fTickSize, 0, 'f' ) );

	}
	
	return fTick;
}

void AudioEngine::clearAudioBuffers( uint32_t nFrames )
{
	QMutexLocker mx( &m_MutexOutputPointer );
	float *pBuffer_L, *pBuffer_R;

	// clear main out Left and Right
	if ( m_pAudioDriver ) {
		pBuffer_L = m_pAudioDriver->getOut_L();
		pBuffer_R = m_pAudioDriver->getOut_R();
		assert( pBuffer_L != nullptr && pBuffer_R != nullptr );
		memset( pBuffer_L, 0, nFrames * sizeof( float ) );
		memset( pBuffer_R, 0, nFrames * sizeof( float ) );
	}
	
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackAudioDriver() ) {
		JackAudioDriver * pJackAudioDriver = dynamic_cast<JackAudioDriver*>(m_pAudioDriver);
	
		if( pJackAudioDriver ) {
			pJackAudioDriver->clearPerTrackAudioBuffers( nFrames );
		}
	}
#endif

	mx.unlock();

#ifdef H2CORE_HAVE_LADSPA
	if ( getState() == State::Ready || getState() == State::Playing ) {
		Effects* pEffects = Effects::get_instance();
		for ( unsigned i = 0; i < MAX_FX; ++i ) {	// clear FX buffers
			LadspaFX* pFX = pEffects->getLadspaFX( i );
			if ( pFX ) {
				assert( pFX->m_pBuffer_L );
				assert( pFX->m_pBuffer_R );
				memset( pFX->m_pBuffer_L, 0, nFrames * sizeof( float ) );
				memset( pFX->m_pBuffer_R, 0, nFrames * sizeof( float ) );
			}
		}
	}
#endif
}

AudioOutput* AudioEngine::createDriver( const QString& sDriver )
{
	___INFOLOG( QString( "Driver: '%1'" ).arg( sDriver ) );
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *pDriver = nullptr;

	if ( sDriver == "OSS" ) {
		pDriver = new OssDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::_class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "JACK" ) {
		pDriver = new JackAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::_class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		} else {
#ifdef H2CORE_HAVE_JACK
			static_cast<JackAudioDriver*>(pDriver)->setConnectDefaults(
						Preferences::get_instance()->m_bJackConnectDefaults
						);
#endif
		}
	} else if ( sDriver == "ALSA" ) {
		pDriver = new AlsaAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::_class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "PortAudio" ) {
		pDriver = new PortAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::_class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	//#ifdef Q_OS_MACX
	else if ( sDriver == "CoreAudio" ) {
		___INFOLOG( "Creating CoreAudioDriver" );
		pDriver = new CoreAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::_class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	//#endif
	else if ( sDriver == "PulseAudio" ) {
		pDriver = new PulseAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::_class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	else if ( sDriver == "Fake" ) {
		___WARNINGLOG( "*** Using FAKE audio driver ***" );
		pDriver = new FakeDriver( m_AudioProcessCallback );
	} else {
		___ERRORLOG( "Unknown driver " + sDriver );
		raiseError( Hydrogen::UNKNOWN_DRIVER );
	}

	if ( pDriver  ) {
		// initialize the audio driver
		int res = pDriver->init( pPref->m_nBufferSize );
		if ( res != 0 ) {
			___ERRORLOG( "Error starting audio driver [audioDriver::init()]" );
			delete pDriver;
			pDriver = nullptr;
		}
	}
	return pDriver;
}

void AudioEngine::startAudioDrivers()
{
	Preferences *preferencesMng = Preferences::get_instance();

	// Lock both the AudioEngine and the audio output buffers.
	this->lock( RIGHT_HERE );
	QMutexLocker mx(&m_MutexOutputPointer);

	___INFOLOG( "[audioEngine_startAudioDrivers]" );
	
	// check current state
	if ( getState() != State::Initialized ) {
		___ERRORLOG( QString( "Audio engine is not in State::Initialized but [%1]" )
					 .arg( static_cast<int>( getState() ) ) );
		this->unlock();
		return;
	}

	if ( m_pAudioDriver ) {	// check if the audio m_pAudioDriver is still alive
		___ERRORLOG( "The audio driver is still alive" );
	}
	if ( m_pMidiDriver ) {	// check if midi driver is still alive
		___ERRORLOG( "The MIDI driver is still active" );
	}

	QString sAudioDriver = preferencesMng->m_sAudioDriver;
#if defined(WIN32)
	QStringList drivers = { "PortAudio", "JACK" };
#elif defined(__APPLE__)
    QStringList drivers = { "CoreAudio", "JACK", "PulseAudio", "PortAudio" };
#else /* Linux */
    QStringList drivers = { "JACK", "ALSA", "OSS", "PulseAudio", "PortAudio" };
#endif

	if ( sAudioDriver != "Auto" ) {
		drivers.removeAll( sAudioDriver );
		drivers.prepend( sAudioDriver );
	}
	AudioOutput* pAudioDriver;
	for ( QString sDriver : drivers ) {
		if ( ( pAudioDriver = createDriver( sDriver ) ) != nullptr ) {
			if ( sDriver != sAudioDriver && sAudioDriver != "Auto" ) {
				___ERRORLOG( QString( "Couldn't start preferred driver %1, falling back to %2" )
							 .arg( sAudioDriver ).arg( sDriver ) );
			}
			break;
		}
	}
	
	if ( preferencesMng->m_sMidiDriver == "ALSA" ) {
#ifdef H2CORE_HAVE_ALSA
		// Create MIDI driver
		AlsaMidiDriver *alsaMidiDriver = new AlsaMidiDriver();
		m_pMidiDriverOut = alsaMidiDriver;
		m_pMidiDriver = alsaMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "PortMidi" ) {
#ifdef H2CORE_HAVE_PORTMIDI
		PortMidiDriver* pPortMidiDriver = new PortMidiDriver();
		m_pMidiDriver = pPortMidiDriver;
		m_pMidiDriverOut = pPortMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "CoreMIDI" ) {
#ifdef H2CORE_HAVE_COREMIDI
		CoreMidiDriver *coreMidiDriver = new CoreMidiDriver();
		m_pMidiDriver = coreMidiDriver;
		m_pMidiDriverOut = coreMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "JACK-MIDI" ) {
#ifdef H2CORE_HAVE_JACK
		JackMidiDriver *jackMidiDriver = new JackMidiDriver();
		m_pMidiDriverOut = jackMidiDriver;
		m_pMidiDriver = jackMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	}
	
	mx.unlock();
	this->unlock();

	setAudioDriver( pAudioDriver );
}
	
void AudioEngine::setAudioDriver( AudioOutput* pAudioDriver ) {
	INFOLOG( "" );
	if ( pAudioDriver == nullptr ) {
		raiseError( Hydrogen::ERROR_STARTING_DRIVER );
		ERRORLOG( "Error starting audio driver. Using the NULL output audio driver instead." );

		// use the NULL output driver
		pAudioDriver = new NullDriver( audioEngine_process );
		pAudioDriver->init( 0 );
	}
	
	this->lock( RIGHT_HERE );
	QMutexLocker mx(&m_MutexOutputPointer);

	m_pAudioDriver = pAudioDriver;

	// change the current audio engine state
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong ) {
		m_state = State::Ready;
	} else {
		m_state = State::Prepared;
	}

	m_pEventQueue->push_event( EVENT_STATE, static_cast<int>( getState() ) );
	// Unlocking earlier might execute the jack process() callback before we
	// are fully initialized.
	mx.unlock();
	this->unlock();
	
	if ( m_pAudioDriver != nullptr &&
		 m_pAudioDriver->class_name() != DiskWriterDriver::_class_name() ) {
		int res = m_pAudioDriver->connect();
		if ( res != 0 ) {
			raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			ERRORLOG( "Error starting audio driver [audioDriver::connect()]" );
			ERRORLOG( "Using the NULL output audio driver" );

			mx.relock();
			delete m_pAudioDriver;
			m_pAudioDriver = new NullDriver( m_AudioProcessCallback );
			mx.unlock();
			m_pAudioDriver->init( 0 );
			m_pAudioDriver->connect();
		}

#ifdef H2CORE_HAVE_JACK
		if ( pSong != nullptr ) {
			pHydrogen->renameJackPorts( pSong );
		}
#endif
		
		setupLadspaFX();
	}
}

void AudioEngine::stopAudioDrivers()
{
	INFOLOG( "" );

	// check current state
	if ( m_state == State::Playing ) {
		this->stopPlayback(); 
	}

	if ( ( m_state != State::Prepared )
		 && ( m_state != State::Ready ) ) {
		ERRORLOG( QString( "Audio engine is not in State::Prepared or State::Ready but [%1]" )
				  .arg( static_cast<int>(m_state) ) );
		return;
	}

	this->lock( RIGHT_HERE );

	// change the current audio engine state
	m_state = State::Initialized;
	m_pEventQueue->push_event( EVENT_STATE, static_cast<int>(State::Initialized) );

	// delete MIDI driver
	if ( m_pMidiDriver != nullptr ) {
		m_pMidiDriver->close();
		delete m_pMidiDriver;
		m_pMidiDriver = nullptr;
		m_pMidiDriverOut = nullptr;
	}

	// delete audio driver
	if ( m_pAudioDriver != nullptr ) {
		m_pAudioDriver->disconnect();
		QMutexLocker mx( &m_MutexOutputPointer );
		delete m_pAudioDriver;
		m_pAudioDriver = nullptr;
		mx.unlock();
	}

	this->unlock();
}

/** 
 * Restart all audio and midi drivers by calling first
 * stopAudioDrivers() and then startAudioDrivers() 
 */
void AudioEngine::restartAudioDrivers()
{
	if ( m_pAudioDriver != nullptr ) {
		stopAudioDrivers();
	}
	startAudioDrivers();
}

float AudioEngine::getBpmAtColumn( int nColumn ) {

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	float fBpm = pAudioEngine->getBpm();

	// Check for a change in the current BPM.
	if ( pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Slave &&
		 pHydrogen->getMode() == Song::Mode::Song ) {
		// Hydrogen is using the BPM broadcast by the JACK
		// server. This one does solely depend on external
		// applications and will NOT be stored in the Song.
		float fJackMasterBpm = pHydrogen->getMasterBpm();
		if ( ! std::isnan( fJackMasterBpm ) && fBpm != fJackMasterBpm ) {
			fBpm = fJackMasterBpm;
			// DEBUGLOG( QString( "Tempo update by the JACK server [%1]").arg( fJackMasterBpm ) );
		}
	} else if ( Preferences::get_instance()->getUseTimelineBpm() &&
				pHydrogen->getMode() == Song::Mode::Song ) {

		float fTimelineBpm = pHydrogen->getTimeline()->getTempoAtColumn( nColumn );
		if ( fTimelineBpm != fBpm ) {
			// DEBUGLOG( QString( "Set tempo to timeline value [%1]").arg( fTimelineBpm ) );
			fBpm = fTimelineBpm;
		}

	} else {
		// Change in speed due to user interaction with the BPM widget
		// or corresponding MIDI or OSC events.
		if ( pAudioEngine->getNextBpm() != fBpm ) {
			// DEBUGLOG( QString( "BPM changed via Widget, OSC, or MIDI from [%1] to [%2]." )
			// 		  .arg( fBpm ).arg( pAudioEngine->getNextBpm() ) );
			fBpm = pAudioEngine->getNextBpm();
		}
	}
	return fBpm;
}

void AudioEngine::setupLadspaFX()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( ! pSong ) {
		return;
	}

#ifdef H2CORE_HAVE_LADSPA
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX == nullptr ) {
			return;
		}

		pFX->deactivate();

		Effects::get_instance()->getLadspaFX( nFX )->connectAudioPorts(
					pFX->m_pBuffer_L,
					pFX->m_pBuffer_R,
					pFX->m_pBuffer_L,
					pFX->m_pBuffer_R
					);
		pFX->activate();
	}
#endif
}

void AudioEngine::raiseError( unsigned nErrorCode )
{
	m_pEventQueue->push_event( EVENT_ERROR, nErrorCode );
}

void AudioEngine::processPlayNotes( unsigned long nframes )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	long long nFrames;
	if ( getState() == State::Playing ) {
		// Current transport position.
		nFrames = getFrames();
		
	} else {
		// In case the playback is stopped and all realtime events,
		// by e.g. MIDI or Hydrogen's virtual keyboard, we disregard
		// tempo changes in the Timeline and pretend the current tick
		// size is valid for all future notes.
		nFrames = getRealtimeFrames();
	}

	// reading from m_songNoteQueue
	while ( !m_songNoteQueue.empty() ) {
		Note *pNote = m_songNoteQueue.top();

		// Compute the note start in frames corresponding to nnTick
		// and the tick sized used to calculate it.
		double fTickOffset;
		// verifico se la nota rientra in questo ciclo
		long long nNoteStartInFrames = computeFrameFromTick( pNote->get_position(), &fTickOffset );
		pNote->setNoteStart( nNoteStartInFrames );
		if ( pHydrogen->isTimelineEnabled() ) {
			pNote->setUsedTickSize( -1 );
		} else {
			pNote->setUsedTickSize( getTickSize() );
		}
		
		// if there is a negative Humanize delay, take into account so
		// we don't miss the time slice.  ignore positive delay, or we
		// might end the queue processing prematurely based on NoteQueue
		// placement.  the sampler handles positive delay.
		if (pNote->get_humanize_delay() < 0) {
			nNoteStartInFrames += pNote->get_humanize_delay();
		}

		// DEBUGLOG( QString( "getDoubleTick(): %1, getFrames(): %2, nframes: %3, " )
		// 		  .arg( getDoubleTick() ).arg( getFrames() )
		// 		  .arg( nframes ).append( pNote->toQString( "", true ) ) );

		if ( nNoteStartInFrames <
			 nFrames + static_cast<long long>(nframes) ) {
			/* Check if the current note has probability != 1
			 * If yes remove call random function to dequeue or not the note
			 */
			float fNoteProbability = pNote->get_probability();
			if ( fNoteProbability != 1. ) {
				if ( fNoteProbability < (float) rand() / (float) RAND_MAX ) {
					m_songNoteQueue.pop();
					pNote->get_instrument()->dequeue();
					continue;
				}
			}

			if ( pSong->getHumanizeVelocityValue() != 0 ) {
				float random = pSong->getHumanizeVelocityValue() * getGaussian( 0.2 );
				pNote->set_velocity(
							pNote->get_velocity()
							+ ( random
								- ( pSong->getHumanizeVelocityValue() / 2.0 ) )
							);
				if ( pNote->get_velocity() > 1.0 ) {
					pNote->set_velocity( 1.0 );
				} else if ( pNote->get_velocity() < 0.0 ) {
					pNote->set_velocity( 0.0 );
				}
			}

			// Offset + Random Pitch ;)
			float fPitch = pNote->get_pitch() + pNote->get_instrument()->get_pitch_offset();
			/* Check if the current instrument has random pitch factor != 0.
			 * If yes add a gaussian perturbation to the pitch
			 */
			float fRandomPitchFactor = pNote->get_instrument()->get_random_pitch_factor();
			if ( fRandomPitchFactor != 0. ) {
				fPitch += getGaussian( 0.4 ) * fRandomPitchFactor;
			}
			pNote->set_pitch( fPitch );

			/*
			 * Check if the current instrument has the property "Stop-Note" set.
			 * If yes, a NoteOff note is generated automatically after each note.
			 */
			auto  noteInstrument = pNote->get_instrument();
			if ( noteInstrument->is_stop_notes() ){
				Note *pOffNote = new Note( noteInstrument,
										   0.0,
										   0.0,
										   0.0,
										   -1,
										   0 );
				pOffNote->set_note_off( true );
				pHydrogen->getAudioEngine()->getSampler()->noteOn( pOffNote );
				delete pOffNote;
			}

			m_pSampler->noteOn( pNote );
			m_songNoteQueue.pop(); // rimuovo la nota dalla lista di note
			pNote->get_instrument()->dequeue();
			// raise noteOn event
			int nInstrument = pSong->getInstrumentList()->index( pNote->get_instrument() );
			if( pNote->get_note_off() ){
				delete pNote;
			}

			m_pEventQueue->push_event( EVENT_NOTEON, nInstrument );
			
			continue;
		} else {
			// this note will not be played
			break;
		}
	}
}

void AudioEngine::clearNoteQueue()
{
	// delete all copied notes in the song notes queue
	while (!m_songNoteQueue.empty()) {
		m_songNoteQueue.top()->get_instrument()->dequeue();
		delete m_songNoteQueue.top();
		m_songNoteQueue.pop();
	}

	// delete all copied notes in the midi notes queue
	for ( unsigned i = 0; i < m_midiNoteQueue.size(); ++i ) {
		delete m_midiNoteQueue[i];
	}
	m_midiNoteQueue.clear();
}

int AudioEngine::audioEngine_process( uint32_t nframes, void* /*arg*/ )
{
	AudioEngine* pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	timeval startTimeval = currentTime2();

	// Resetting all audio output buffers with zeros.
	pAudioEngine->clearAudioBuffers( nframes );

	// Calculate maximum time to wait for audio engine lock. Using the
	// last calculated processing time as an estimate of the expected
	// processing time for this frame, the amount of slack time that
	// we can afford to wait is: m_fMaxProcessTime - m_fProcessTime.

	float sampleRate = static_cast<float>(pAudioEngine->m_pAudioDriver->getSampleRate());
	pAudioEngine->m_fMaxProcessTime = 1000.0 / ( sampleRate / nframes );
	float fSlackTime = pAudioEngine->m_fMaxProcessTime - pAudioEngine->m_fProcessTime;

	// If we expect to take longer than the available time to process,
	// require immediate locking or not at all: we're bound to drop a
	// buffer anyway.
	if ( fSlackTime < 0.0 ) {
		fSlackTime = 0.0;
	}

	/*
	 * The "try_lock" was introduced for Bug #164 (Deadlock after during
	 * alsa driver shutdown). The try_lock *should* only fail in rare circumstances
	 * (like shutting down drivers). In such cases, it seems to be ok to interrupt
	 * audio processing. Returning the special return value "2" enables the disk 
	 * writer driver to repeat the processing of the current data.
	 */
				
	if ( !pAudioEngine->tryLockFor( std::chrono::microseconds( (int)(1000.0*fSlackTime) ),
							  RIGHT_HERE ) ) {
		___ERRORLOG( QString( "Failed to lock audioEngine in allowed %1 ms, missed buffer" ).arg( fSlackTime ) );

		if ( pAudioEngine->m_pAudioDriver->class_name() == DiskWriterDriver::_class_name() ) {
			return 2;	// inform the caller that we could not aquire the lock
		}

		return 0;
	}

	if ( pAudioEngine->getState() != AudioEngine::State::Ready &&
		 pAudioEngine->getState() != AudioEngine::State::Playing ) {
		pAudioEngine->unlock();
		return 0;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	assert( pSong );

	// Sync transport with server (in case the current audio driver is
	// designed that way)
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() ) {
		// Compares the current transport state, speed in bpm, and
		// transport position with a query request to the JACK
		// server. It will only overwrite the transport state, if
		// the transport position was changed by the user by
		// e.g. clicking on the timeline.
		static_cast<JackAudioDriver*>( pHydrogen->getAudioOutput() )->updateTransportInfo();
	}
#endif

	// Check whether the tempo was changed.
	pAudioEngine->updateBpmAndTickSize();

	// Update the state of the audio engine depending on whether it
	// was started or stopped by the user.
	if ( pAudioEngine->getNextState() == State::Playing ) {
		if ( pAudioEngine->getState() == State::Ready ) {
			DEBUGLOG("set playing");
			pAudioEngine->startPlayback();
		}
		
		pAudioEngine->setRealtimeFrames( pAudioEngine->getFrames() );
	} else {
		if ( pAudioEngine->getState() == State::Playing ) {
			pAudioEngine->stopPlayback();
		}
		
		// go ahead and increment the realtimeframes by nFrames
		// to support our realtime keyboard and midi event timing
		pAudioEngine->setRealtimeFrames( pAudioEngine->getRealtimeFrames() +
										 static_cast<long long>(nframes) );
	}
   
	bool bSendPatternChange = false;
	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	int nResNoteQueue = pAudioEngine->updateNoteQueue( nframes );
	if ( nResNoteQueue == -1 ) {	// end of song
		___INFOLOG( "End of song received" );
		pAudioEngine->stop();
		pAudioEngine->stopPlayback();
		pAudioEngine->reset();

		if ( pAudioEngine->m_pAudioDriver->class_name() ==
			 FakeDriver::_class_name() ) {
			return 1;	// kill the audio AudioDriver thread
		}
		
	} else if ( nResNoteQueue == 2 ) { // send pattern change
		bSendPatternChange = true;
	}

	// play all notes
	pAudioEngine->processPlayNotes( nframes );

	float *pBuffer_L = pAudioEngine->m_pAudioDriver->getOut_L(),
		*pBuffer_R = pAudioEngine->m_pAudioDriver->getOut_R();
	assert( pBuffer_L != nullptr && pBuffer_R != nullptr );

	// SAMPLER
	pAudioEngine->getSampler()->process( nframes, pSong );
	float* out_L = pAudioEngine->getSampler()->m_pMainOut_L;
	float* out_R = pAudioEngine->getSampler()->m_pMainOut_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

	// SYNTH
	pAudioEngine->getSynth()->process( nframes );
	out_L = pAudioEngine->getSynth()->m_pOut_L;
	out_R = pAudioEngine->getSynth()->m_pOut_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

	timeval renderTime_end = currentTime2();
	timeval ladspaTime_start = renderTime_end;

#ifdef H2CORE_HAVE_LADSPA
	// Process LADSPA FX
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( ( pFX ) && ( pFX->isEnabled() ) ) {
			pFX->processFX( nframes );

			float *buf_L, *buf_R;
			if ( pFX->getPluginType() == LadspaFX::STEREO_FX ) {
				buf_L = pFX->m_pBuffer_L;
				buf_R = pFX->m_pBuffer_R;
			} else { // MONO FX
				buf_L = pFX->m_pBuffer_L;
				buf_R = buf_L;
			}

			for ( unsigned i = 0; i < nframes; ++i ) {
				pBuffer_L[ i ] += buf_L[ i ];
				pBuffer_R[ i ] += buf_R[ i ];
				if ( buf_L[ i ] > pAudioEngine->m_fFXPeak_L[nFX] ) {
					pAudioEngine->m_fFXPeak_L[nFX] = buf_L[ i ];
				}

				if ( buf_R[ i ] > pAudioEngine->m_fFXPeak_R[nFX] ) {
					pAudioEngine->m_fFXPeak_R[nFX] = buf_R[ i ];
				}
			}
		}
	}
#endif
	timeval ladspaTime_end = currentTime2();


	// update master peaks
	float val_L, val_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		val_L = pBuffer_L[i];
		val_R = pBuffer_R[i];

		if ( val_L > pAudioEngine->m_fMasterPeak_L ) {
			pAudioEngine->m_fMasterPeak_L = val_L;
		}

		if ( val_R > pAudioEngine->m_fMasterPeak_R ) {
			pAudioEngine->m_fMasterPeak_R = val_R;
		}

		for (std::vector<DrumkitComponent*>::iterator it = pSong->getComponents()->begin() ; it != pSong->getComponents()->end(); ++it) {
			DrumkitComponent* drumkit_component = *it;

			float compo_val_L = drumkit_component->get_out_L(i);
			float compo_val_R = drumkit_component->get_out_R(i);

			if( compo_val_L > drumkit_component->get_peak_l() ) {
				drumkit_component->set_peak_l( compo_val_L );
			}
			if( compo_val_R > drumkit_component->get_peak_r() ) {
				drumkit_component->set_peak_r( compo_val_R );
			}
		}
	}

	// increment the transport position
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
		pAudioEngine->incrementTransportPosition( nframes );
	}

	timeval finishTimeval = currentTime2();
	pAudioEngine->m_fProcessTime =
			( finishTimeval.tv_sec - startTimeval.tv_sec ) * 1000.0
			+ ( finishTimeval.tv_usec - startTimeval.tv_usec ) / 1000.0;

	// if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
	// 	INFOLOG(pAudioEngine->m_fProcessTime);
	// }
	
#ifdef CONFIG_DEBUG
	if ( pAudioEngine->m_fProcessTime > m_fMaxProcessTime ) {
		___WARNINGLOG( "" );
		___WARNINGLOG( "----XRUN----" );
		___WARNINGLOG( QString( "XRUN of %1 msec (%2 > %3)" )
					   .arg( ( m_fProcessTime - pAudioEngine->m_fMaxProcessTime ) )
					   .arg( m_fProcessTime ).arg( pAudioEngine->m_fMaxProcessTime ) );
		___WARNINGLOG( QString( "Ladspa process time = %1" ).arg( fLadspaTime ) );
		___WARNINGLOG( "------------" );
		___WARNINGLOG( "" );
		// raise xRun event
		EventQueue::get_instance()->push_event( EVENT_XRUN, -1 );
	}
#endif

	pAudioEngine->unlock();

	if ( bSendPatternChange ) {
		EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	}
	return 0;
}

void AudioEngine::setSong( std::shared_ptr<Song> pNewSong )
{
	___WARNINGLOG( QString( "Set song: %1" ).arg( pNewSong->getName() ) );
	
	this->lock( RIGHT_HERE );

	// check current state
	// should be set by removeSong called earlier
	if ( getState() != State::Prepared ) {
		___ERRORLOG( QString( "Error the audio engine is not in State::Prepared but [%1]" )
					 .arg( static_cast<int>( getState() ) ) );
	}

	// setup LADSPA FX
	if ( m_pAudioDriver != nullptr ) {
		setupLadspaFX();
	}

	// find the first pattern and set as current
	if ( pNewSong->getPatternList()->size() > 0 ) {
		m_pPlayingPatterns->add( pNewSong->getPatternList()->get( 0 ) );
	}

#ifdef H2CORE_HAVE_JACK
	Hydrogen::get_instance()->renameJackPorts( pNewSong );
#endif
	m_fSongSizeInTicks = static_cast<float>( pNewSong->lengthInTicks() );

	// change the current audio engine state
	setState( State::Ready );

	setNextBpm( pNewSong->getBpm() );
	locate( 0 );

	this->unlock();

	m_pEventQueue->push_event( EVENT_STATE, static_cast<int>(State::Ready) );
}

void AudioEngine::removeSong()
{
	this->lock( RIGHT_HERE );

	if ( getState() == State::Playing ) {
		stop();
		this->stopPlayback();
	}

	// check current state
	if ( getState() != State::Ready ) {
		___ERRORLOG( QString( "Error the audio engine is not in State::Ready but [%1]" )
					 .arg( static_cast<int>( getState() ) ) );
		this->unlock();
		return;
	}

	m_pPlayingPatterns->clear();
	m_pNextPatterns->clear();
	clearNoteQueue();
	m_pSampler->stopPlayingNotes();

	// change the current audio engine state
	setState( State::Prepared );
	this->unlock();

	m_pEventQueue->push_event( EVENT_STATE, static_cast<int>( getState() ) );
}

void AudioEngine::updateSongSize() {

	auto pSong = Hydrogen::get_instance()->getSong();

	if ( pSong == nullptr ) {
		return;
	}

	lock( RIGHT_HERE );

	double fNewSongSizeInTicks = static_cast<double>( pSong->lengthInTicks() );

	// WARNINGLOG( QString( "[Before] frame: %1, bpm: %2, tickSize: %3, column: %4, tick: %5, pTickPos: %6, pStartPos: %7" )
	// 			.arg( getFrames() ).arg( getBpm() )
	// 			.arg( getTickSize(), 0, 'f' )
	// 			.arg( m_nColumn ).arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick ) );

	// transport position was already looped at least once.
	if ( getDoubleTick() > m_fSongSizeInTicks ) {
		double fNewTick = std::fmod( getDoubleTick(), m_fSongSizeInTicks ) +
			std::floor( getDoubleTick() / m_fSongSizeInTicks ) *
			fNewSongSizeInTicks;

		m_fLastTickIntervalEnd += fNewTick - getDoubleTick();
		m_nLastPlayingPatternsColumn = -1;

		setTick( fNewTick );
		
		// DEBUGLOG(QString( "fNewTick: %1, oldS: %2, newS: %3")
		// 		 .arg( fNewTick ).arg( m_fSongSizeInTicks )
		// 		 .arg( fNewSongSizeInTicks )
		// 		 );
	}
	
	m_fSongSizeInTicks = fNewSongSizeInTicks;

	// Ensure transport state is consistent
	long long nNewFrames = computeFrameFromTick( getDoubleTick(), &m_fTickOffset );
		
	// DEBUGLOG(QString( "nNewFrame: %1, old frame: %2, offset [pre]: %3")
	// 		 .arg( nNewFrames ).arg( getFrames() )
	// 		 .arg( m_nFrameOffset )
	// 		 );

	m_nFrameOffset = nNewFrames - getFrames() + m_nFrameOffset;

	setFrames( nNewFrames );
	
	updateTransportPosition( getDoubleTick(),
							 pSong->isLoopEnabled() );
	getSampler()->handleTimelineChange();

	if ( m_nColumn == -1 ) {
		stop();
	}

	// WARNINGLOG( QString( "[After] frame: %1, bpm: %2, tickSize: %3, column: %4, tick: %5, pTickPos: %6, pStartPos: %7" )
	// 			.arg( getFrames() ).arg( getBpm() )
	// 			.arg( getTickSize(), 0, 'f' )
	// 			.arg( m_nColumn ).arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick ) );
	
	unlock();
}

long long AudioEngine::computeTickInterval( double* fTickStart, double* fTickEnd, unsigned nFrames ) {

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pTimeline = pHydrogen->getTimeline();

	long long nFrameStart, nFrameEnd;

	if ( getState() == State::Ready ) {
		// In case the playback is stopped and all realtime events,
		// by e.g. MIDI or Hydrogen's virtual keyboard, we disregard
		// tempo changes in the Timeline and pretend the current tick
		// size is valid for all future notes.
		nFrameStart = getRealtimeFrames();
	} else {
		// Enters here both when the transport is rolling and
		// State::Playing is set as well as with State::Prepared
		// during testing.
		nFrameStart = getFrames();
	}
	
	// We don't use the getLookaheadInFrames() function directly
	// because the lookahead contains both a frame-based and a
	// tick-based component and would be a twice as expensive to
	// calculate using the mentioned call.
	long long nLeadLagFactor = getLeadLagInFrames( getDoubleTick() );
	long long nLookahead = nLeadLagFactor +
		AudioEngine::nMaxTimeHumanize + 1;

	nFrameEnd = nFrameStart + nLookahead +
		static_cast<long long>(nFrames);

	if ( m_fLastTickIntervalEnd != -1 ) {
		nFrameStart += nLookahead;
	}
	
	*fTickStart = computeTickFromFrame( nFrameStart );
	*fTickEnd = computeTickFromFrame( nFrameEnd );

	// Used both in State::Playing with transport is rolling and
	// State::Prepared during the unit tests.
	if ( getState() != State::Ready ) {
		// If there was a change in ticksize, account for the last used
		// lookahead to ensure the tick intervals are aligned.
		if ( m_fLastTickIntervalEnd != -1 &&
			 m_fLastTickIntervalEnd != *fTickStart ) {
			if ( m_fLastTickIntervalEnd > *fTickEnd ) {
				// The last lookahead was larger than the end of the
				// current interval would reach. We will remain at the
				// former interval end until the lookahead was eaten up in
				// future calls to updateNoteQueue() to not produce
				// glitches by non-aligned tick intervals.
				*fTickStart = m_fLastTickIntervalEnd;
				*fTickEnd = m_fLastTickIntervalEnd;
			} else {
				*fTickStart = m_fLastTickIntervalEnd;
			}
		}

		// DEBUGLOG( QString( "tick: [%1,%2], curr tick: %5, curr frame: %4, nFrames: %3, realtime: %6, m_fTickOffset: %7, ticksize: %8, leadlag: %9, nlookahead: %10, m_fLastTickIntervalEnd: %11" )
		// 		  .arg( *fTickStart, 0, 'f' )
		// 		  .arg( *fTickEnd, 0, 'f' )
		// 		  .arg( nFrames )
		// 		  .arg( getFrames() )
		// 		  .arg( getDoubleTick(), 0, 'f' )
		// 		  .arg( getRealtimeFrames() )
		// 		  .arg( m_fTickOffset, 0, 'f' )
		// 		  .arg( getTickSize(), 0, 'f' )
		// 		  .arg( nLeadLagFactor )
		// 		  .arg( nLookahead )
		// 		  .arg( m_fLastTickIntervalEnd, 0, 'f' )
		// 		  );

		if ( m_fLastTickIntervalEnd < *fTickEnd ) {
			m_fLastTickIntervalEnd = *fTickEnd;
		}
	}

	return nLeadLagFactor;
}

int AudioEngine::updateNoteQueue( unsigned nFrames )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	// Indicates whether the current pattern list changed with respect
	// to the last cycle.
	bool bSendPatternChange = false;

	double fTickStart, fTickEnd;

	long long nLeadLagFactor =
		computeTickInterval( &fTickStart, &fTickEnd, nFrames );

	// Get initial timestamp for first tick
	gettimeofday( &m_currentTickTime, nullptr );
		
	// MIDI events now get put into the `m_songNoteQueue` as well,
	// based on their timestamp (which is given in terms of its
	// transport position and not in terms of the date-time as above).
	while ( m_midiNoteQueue.size() > 0 ) {
		Note *pNote = m_midiNoteQueue[0];

		// DEBUGLOG( QString( "getDoubleTick(): %1, getFrames(): %2, " )
		// 		  .arg( getDoubleTick() ).arg( getFrames() )
		// 		  .append( pNote->toQString( "", true ) ) );
		
		if ( pNote->get_position() >
			 static_cast<long long>(std::floor( fTickEnd )) ) {
			break;
		}

		m_midiNoteQueue.pop_front();
		pNote->get_instrument()->enqueue();
		m_songNoteQueue.push( pNote );
	}

	if ( getState() != State::Playing ) {
		// only keep going if we're playing
		return 0;
	}

	// Use local representations of the current transport position in
	// order for it to not get into a dirty state.
	int nColumn = m_nColumn;
	long nPatternStartTick = m_nPatternStartTick;
	long nPatternTickPosition = m_nPatternTickPosition;
	long long nNoteStart;
	float fUsedTickSize;

	double fTickOffset;

	AutomationPath* pAutomationPath = pSong->getVelocityAutomationPath();

	// DEBUGLOG( QString( "tick interval: [%1 : %2], curr tick: %3, curr frame: %4")
	// 		  .arg( fTickStart, 0, 'f' ).arg( fTickEnd, 0, 'f' )
	// 		  .arg( getDoubleTick(), 0, 'f' ).arg( getFrames() ) );

	// We loop over integer ticks to ensure that all notes encountered
	// between two iterations belong to the same pattern.
	for ( long nnTick = static_cast<long>(std::floor(fTickStart));
		  nnTick < static_cast<long>(std::floor(fTickEnd)); nnTick++ ) {
		
		//////////////////////////////////////////////////////////////
		// SONG MODE
		if ( pHydrogen->getMode() == Song::Mode::Song ) {
			if ( pSong->getPatternGroupVector()->size() == 0 ) {
				// there's no song!!
				ERRORLOG( "no patterns in song." );
				stop();
				return -1;
			}

			nColumn = pHydrogen->getColumnForTick( nnTick,
												   pSong->isLoopEnabled(),
												   &nPatternStartTick );

			if ( nnTick > std::floor( m_fSongSizeInTicks ) &&
				 std::floor( m_fSongSizeInTicks ) != 0 ) {
				// When using the JACK audio driver the overall
				// transport position will be managed by an external
				// server. Since it is agnostic of all the looping in
				// its clients, it will only increment time and
				// Hydrogen has to take care of the looping itself. 
				nPatternTickPosition = ( nnTick - nPatternStartTick )
					% static_cast<long>(std::floor( m_fSongSizeInTicks ));
			} else {
				nPatternTickPosition = nnTick - nPatternStartTick;
			}

			// Since we are located at the very beginning of the
			// pattern list, it had to change with respect to the last
			// cycle.
			if ( nPatternTickPosition == 0 ) {
				bSendPatternChange = true;
			}

			// If no pattern list could not be found, either choose
			// the first one if loop mode is activate or the
			// function returns indicating that the end of the song is
			// reached.
			if ( nColumn == -1 ||
				 ( pSong->getLoopMode() == Song::LoopMode::Finishing &&
				   nColumn < m_nLastPlayingPatternsColumn ) ) {
				INFOLOG( "End of Song" );

				if( pHydrogen->getMidiOutput() != nullptr ){
					pHydrogen->getMidiOutput()->handleQueueAllNoteOff();
				}

				return -1;
			}

			if ( nColumn != m_nLastPlayingPatternsColumn ) {
				// Obtain the current PatternList and use it to overwrite
				// the on in `m_pPlayingPatterns.
				PatternList *pPatternList = ( *( pSong->getPatternGroupVector() ) )[nColumn];
				m_pPlayingPatterns->clear();
				for ( int i=0; i< pPatternList->size(); ++i ) {
					Pattern* pPattern = pPatternList->get(i);
					m_pPlayingPatterns->add( pPattern );
					pPattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
				}
				m_nLastPlayingPatternsColumn = nColumn;
			}
		}
		
		//////////////////////////////////////////////////////////////
		// PATTERN MODE
		else if ( pHydrogen->getMode() == Song::Mode::Pattern )	{

			int nPatternSize = MAX_NOTES;

			// If the user chose to playback the pattern she focuses,
			// use it to overwrite `m_pPlayingPatterns`.
			if ( Preferences::get_instance()->patternModePlaysSelected() )
			{
				// TODO: Again, a check whether the pattern did change
				// would be more efficient.
				m_pPlayingPatterns->clear();
				Pattern * pattern = pSong->getPatternList()->get( pHydrogen->getSelectedPatternNumber() );
				m_pPlayingPatterns->add( pattern );
				pattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}

			if ( m_pPlayingPatterns->size() != 0 ) {
				nPatternSize = m_pPlayingPatterns->longest_pattern_length();
			}

			if ( nPatternSize == 0 ) {
				___ERRORLOG( "nPatternSize == 0" );
			}

			// If either the beginning of the current pattern was not
			// specified yet or if its end is reached, write the
			// content of `m_pNextPatterns` to `m_pPlayingPatterns`
			// and clear the former one.
			if ( ( nnTick == nPatternStartTick + nPatternSize )
				 || ( nPatternStartTick == -1 ) ) {
				if ( m_pNextPatterns->size() > 0 ) {
					Pattern* pPattern;
					for ( uint i = 0; i < m_pNextPatterns->size(); i++ ) {
						pPattern = m_pNextPatterns->get( i );
						// If `pPattern is already present in
						// `m_pPlayingPatterns`, it will be removed
						// from the latter and its `del()` method will
						// return a pointer to the very pattern. The
						// if clause is therefore only entered if the
						// `pPattern` was not already present.
						if ( ( m_pPlayingPatterns->del( pPattern ) ) == nullptr ) {
							m_pPlayingPatterns->add( pPattern );
						}
					}
					m_pNextPatterns->clear();
					bSendPatternChange = true;
				}
				if ( nPatternStartTick == -1 && nPatternSize > 0 ) {
					nPatternStartTick = nnTick - (nnTick % nPatternSize);
				} else {
					nPatternStartTick = nnTick;
				}
			}

			// Since the starting position of the Pattern may have
			// been updated, update the number of ticks passed since
			// the beginning of the pattern too.
			nPatternTickPosition = nnTick - nPatternStartTick;
			if ( nPatternTickPosition > nPatternSize && nPatternSize > 0 ) {
				nPatternTickPosition = nnTick % nPatternSize;
			}
		}
		
		//////////////////////////////////////////////////////////////
		// Metronome
		// Only trigger the metronome at a predefined rate.
		if ( nPatternTickPosition % 48 == 0 ) {
			float fPitch;
			float fVelocity;
			
			// Depending on whether the metronome beat will be issued
			// at the beginning or in the remainder of the pattern,
			// two different sounds and events will be used.
			if ( nPatternTickPosition == 0 ) {
				fPitch = 3;
				fVelocity = 1.0;
				EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
			} else {
				fPitch = 0;
				fVelocity = 0.8;
				EventQueue::get_instance()->push_event( EVENT_METRONOME, 0 );
			}
			
			// Only trigger the sounds if the user enabled the
			// metronome. 
			if ( Preferences::get_instance()->m_bUseMetronome ) {
				m_pMetronomeInstrument->set_volume(
							Preferences::get_instance()->m_fMetronomeVolume
							);
				Note *pMetronomeNote = new Note( m_pMetronomeInstrument,
												 nnTick,
												 fVelocity,
												 0.f, // pan
												 -1,
												 fPitch
												 );
				m_pMetronomeInstrument->enqueue();
				m_songNoteQueue.push( pMetronomeNote );
			}
		}

		//////////////////////////////////////////////////////////////
		// Update the notes queue.
		//
		// Supporting ticks with float precision:
		// - make FOREACH_NOTE_CST_IT_BOUND loop over all notes
		// `(_it)->first >= (_bound) && (_it)->first < (_bound + 1)`
		// - add remainder of pNote->get_position() % 1 when setting
		// nnTick as new position.
		//
		if ( m_pPlayingPatterns->size() != 0 ) {
			for ( unsigned nPat = 0 ;
				  nPat < m_pPlayingPatterns->size() ;
				  ++nPat ) {
				Pattern *pPattern = m_pPlayingPatterns->get( nPat );
				assert( pPattern != nullptr );
				Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->get_notes();

				// Loop over all notes at tick nPatternTickPosition
				// (associated tick is determined by Note::__position
				// at the time of insertion into the Pattern).
				FOREACH_NOTE_CST_IT_BOUND(notes,it,nPatternTickPosition) {
					Note *pNote = it->second;
					if ( pNote ) {
						pNote->set_just_recorded( false );
						
						/** Time Offset in frames (relative to sample rate)
						*	Sum of 3 components: swing, humanized timing, lead_lag
						*/
						int nOffset = 0;

					   /** Swing 16ths //
						* delay the upbeat 16th-notes by a constant (manual) offset
						*/
						if ( ( ( nPatternTickPosition % ( MAX_NOTES / 16 ) ) == 0 )
							 && ( ( nPatternTickPosition % ( MAX_NOTES / 8 ) ) != 0 )
							 && pSong->getSwingFactor() > 0 ) {
							/* TODO: incorporate the factor MAX_NOTES / 32. either in Song::m_fSwingFactor
							* or make it a member variable.
							* comment by oddtime:
							* 32 depends on the fact that the swing is applied to the upbeat 16th-notes.
							* (not to upbeat 8th-notes as in jazz swing!).
							* however 32 could be changed but must be >16, otherwise the max delay is too long and
							* the swing note could be played after the next downbeat!
							*/
							// If the Timeline is activated, the tick
							// size may change at any
							// point. Therefore, the length in frames
							// of a 16-th note offset has to be
							// calculated for a particular transport
							// position and is not generally applicable.
							nOffset +=
								computeFrameFromTick( nnTick + MAX_NOTES / 32., &fTickOffset ) *
								pSong->getSwingFactor() -
								computeFrameFromTick( nnTick, &fTickOffset );
						}

						/* Humanize - Time parameter //
						* Add a random offset to each note. Due to
						* the nature of the Gaussian distribution,
						* the factor Song::__humanize_time_value will
						* also scale the variance of the generated
						* random variable.
						*/
						if ( pSong->getHumanizeTimeValue() != 0 ) {
							nOffset += ( int )(
										getGaussian( 0.3 )
										* pSong->getHumanizeTimeValue()
										* AudioEngine::nMaxTimeHumanize
										);
						}

						// Lead or Lag - timing parameter //
						// Add a constant offset to all notes.
						nOffset += (int) ( pNote->get_lead_lag() * nLeadLagFactor );

						// Lower bound of the offset. No note is
						// allowed to start prior to the beginning of
						// the song.
						if( nNoteStart + nOffset < 0 ){
							nOffset = -nNoteStart;
						}

						if ( nOffset > AudioEngine::nMaxTimeHumanize ) {
							nOffset = AudioEngine::nMaxTimeHumanize;
						} else if ( nOffset < -1 * AudioEngine::nMaxTimeHumanize ) {
							nOffset = -AudioEngine::nMaxTimeHumanize;
						}
						
						// Generate a copy of the current note, assign
						// it the new offset, and push it to the list
						// of all notes, which are about to be played
						// back.
						// Why a copy? because it has the new offset (including swing and random timing) in its
						// humanized delay, and tick position is expressed referring to start time (and not pattern).
						Note *pCopiedNote = new Note( pNote );
						pCopiedNote->set_humanize_delay( nOffset );

						// DEBUGLOG( QString( "getDoubleTick(): %1, getFrames(): %2, nnTick: %3, " )
						// 		  .arg( getDoubleTick() ).arg( getFrames() ).arg( nnTick )
						// 		  .append( pCopiedNote->toQString("", true ) ) );
						
						pCopiedNote->set_position( nnTick );
						if ( pHydrogen->getMode() == Song::Mode::Song ) {
							float fPos = static_cast<float>( nColumn ) +
								pCopiedNote->get_position() % 192 / 192.f;
							pCopiedNote->set_velocity( pNote->get_velocity() *
													   pAutomationPath->get_value( fPos ) );
						}
						pNote->get_instrument()->enqueue();
						m_songNoteQueue.push( pCopiedNote );
					}
				}
			}
		}
	}

	// audioEngine_process() must send the pattern change event after
	// mutex unlock
	if ( bSendPatternChange ) {
		return 2;
	}
	return 0;
}

void AudioEngine::noteOn( Note *note )
{
	// check current state
	if ( ( getState() != State::Playing ) && ( getState() != State::Ready ) ) {
		___ERRORLOG( QString( "Error the audio engine is not in State::Ready or State::Playing but [%1]" )
					 .arg( static_cast<int>( getState() ) ) );
		delete note;
		return;
	}

	m_midiNoteQueue.push_back( note );
}

bool AudioEngine::compare_pNotes::operator()(Note* pNote1, Note* pNote2)
{
	float fTickSize = Hydrogen::get_instance()->getAudioEngine()->getTickSize();
	return (pNote1->get_humanize_delay() +
			AudioEngine::computeFrame( pNote1->get_position(), fTickSize ) ) >
		(pNote2->get_humanize_delay() +
		 AudioEngine::computeFrame( pNote2->get_position(), fTickSize ) );
}

void AudioEngine::play() {
	
	assert( m_pAudioDriver );

#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() ) {
		// Tell all other JACK clients to start as well and wait for
		// the JACK server to give the signal.
		static_cast<JackAudioDriver*>( m_pAudioDriver )->startTransport();
		return;
	}
#endif

	setNextState( State::Playing );

	if ( m_pAudioDriver->class_name() == FakeDriver::_class_name() ) {
		static_cast<FakeDriver*>( m_pAudioDriver )->processCallback();
	}
}

void AudioEngine::stop() {
	assert( m_pAudioDriver );
	
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() ) {
		// Tell all other JACK clients to stop as well and wait for
		// the JACK server to give the signal.
		static_cast<JackAudioDriver*>( m_pAudioDriver )->stopTransport();
		return;
	}
#endif
	
	setNextState( State::Ready );
}

double AudioEngine::getLeadLagInTicks() {
	return 5;
}

long long AudioEngine::getLeadLagInFrames( double fTick ) {
	double fTickOffset;
	long long nFrameEnd = computeFrameFromTick( fTick + AudioEngine::getLeadLagInTicks(),
						  &fTickOffset );
	long long nFrameStart = computeFrameFromTick( fTick, &fTickOffset );

	return nFrameEnd - nFrameStart;
}

long long AudioEngine::getLookaheadInFrames( double fTick ) {
	return getLeadLagInFrames( fTick ) +
		AudioEngine::nMaxTimeHumanize + 1;
}

double AudioEngine::getDoubleTick() const {
	return TransportInfo::getTick();
}

long AudioEngine::getTick() const {
	return static_cast<long>(std::floor( getDoubleTick() ));
}

void AudioEngine::handleTimelineChange() {

	setFrames( computeFrameFromTick( getDoubleTick(), &m_fTickOffset ) );
	updateBpmAndTickSize();
	getSampler()->handleTimelineChange();
	
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() ) {
		// Tell all other JACK clients to relocate as well. This has
		// to be called after updateFrames().
		static_cast<JackAudioDriver*>( m_pAudioDriver )->locateTransport( getFrames() );
	}
#endif
}


bool AudioEngine::testFrameToTickConversion() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	bool bNoMismatch = true;
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 3, 100 );
	pCoreActionController->addTempoMarker( 5, 40 );
	pCoreActionController->addTempoMarker( 7, 200 );

	double fFrameOffset1, fFrameOffset2, fFrameOffset3,
		fFrameOffset4, fFrameOffset5, fFrameOffset6;
	
	long long nFrame1 = 342732;
	long long nFrame2 = 1037223;
	long long nFrame3 = 453610333722;
	double fTick1 = computeTickFromFrame( nFrame1 );
	double fTick2 = computeTickFromFrame( nFrame2 );
	double fTick3 = computeTickFromFrame( nFrame3 );
	long long nFrame1Computed = computeFrameFromTick( fTick1, &fFrameOffset1 );
	long long nFrame2Computed = computeFrameFromTick( fTick2, &fFrameOffset2 );
	long long nFrame3Computed = computeFrameFromTick( fTick3, &fFrameOffset3 );
	
	if ( nFrame1Computed != nFrame1 || fFrameOffset1 != 0 ) {
		ERRORLOG( QString( "[1] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4" )
				  .arg( nFrame1 ).arg( fTick1, 0, 'f' ).arg( nFrame1Computed )
				  .arg( fFrameOffset1, 0, 'f' ).toLocal8Bit().data() );
		bNoMismatch = false;
	}
	if ( nFrame2Computed != nFrame2 || fFrameOffset2 != 0 ) {
		ERRORLOG( QString( "[2] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4" )
				  .arg( nFrame2 ).arg( fTick2, 0, 'f' ).arg( nFrame2Computed )
				  .arg( fFrameOffset2, 0, 'f' ).toLocal8Bit().data() );
		bNoMismatch = false;
	}
	if ( nFrame3Computed != nFrame3 || fFrameOffset3 != 0 ) {
		ERRORLOG( QString( "[3] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4" )
				  .arg( nFrame3 ).arg( fTick3, 0, 'f' ).arg( nFrame3Computed )
				  .arg( fFrameOffset3, 0, 'f' ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	double fTick4 = 552;
	double fTick5 = 1939;
	double fTick6 = 534623409;
	long long nFrame4 = computeFrameFromTick( fTick4, &fFrameOffset4 );
	long long nFrame5 = computeFrameFromTick( fTick5, &fFrameOffset5 );
	long long nFrame6 = computeFrameFromTick( fTick6, &fFrameOffset6 );
	double fTick4Computed = computeTickFromFrame( nFrame4 ) +
		fFrameOffset4;
	double fTick5Computed = computeTickFromFrame( nFrame5 ) +
		fFrameOffset5;
	double fTick6Computed = computeTickFromFrame( nFrame6 ) +
		fFrameOffset6;
	
	
	if ( abs( fTick4Computed - fTick4 ) > 1e-9 ) {
		ERRORLOG( QString( "[4] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4" )
				  .arg( nFrame4 ).arg( fTick4, 0, 'f' ).arg( fTick4Computed, 0, 'f' )
				  .arg( fFrameOffset4, 0, 'f' ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	if ( abs( fTick5Computed - fTick5 ) > 1e-9 ) {
		ERRORLOG( QString( "[5] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4" )
				  .arg( nFrame5 ).arg( fTick5, 0, 'f' ).arg( fTick5Computed, 0, 'f' )
				  .arg( fFrameOffset5, 0, 'f' ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	if ( abs( fTick6Computed - fTick6 ) > 1e-9 ) {
		ERRORLOG( QString( "[6] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4" )
				  .arg( nFrame6 ).arg( fTick6, 0, 'f' ).arg( fTick6Computed, 0, 'f' )
				  .arg( fFrameOffset6, 0, 'f' ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	return bNoMismatch;
}

bool AudioEngine::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( true, false );

	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( 1, 4096 );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Prepared );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	double fCheckTick;
	long long nCheckFrame, nLastFrame = 0;
	double fTickOffset;

	bool bNoMismatch = true;

	int nMaxCycles = 2112; // Equals the number of ticks within the
						   // test song.
	int nn = 0;

	while ( getDoubleTick() < m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		incrementTransportPosition( nFrames );

		if ( ! testCheckTransportPosition( "constant tempo" ) ) {
			bNoMismatch = false;
			break;
		}

		if ( getFrames() - nFrames != nLastFrame ) {
			ERRORLOG( QString( "[constant tempo] inconsistent frame update. getFrames(): %1, nFrames: %2, nLastFrame: %3" )
					  .arg( getFrames() ).arg( nFrames ).arg( nLastFrame ) );
			bNoMismatch = false;
			break;
		}
		nLastFrame = getFrames();

		nn++;

		if ( nn > nMaxCycles ) {
			ERRORLOG( "[constant tempo] end of the song wasn't reached in time." );
			bNoMismatch = false;
			break;
		}
	}

	reset( false );
	nLastFrame = 0;

	float fBpm;
	float fLastBpm = getBpm();
	int nCyclesPerTempo = 5;

	long long nTotalFrames = 0;
	
	nn = 0;

	while ( getDoubleTick() < m_fSongSizeInTicks ) {

		fBpm = tempoDist( randomEngine );

		nLastFrame = std::round( nLastFrame * fLastBpm / fBpm );
		
		setNextBpm( fBpm );
		updateBpmAndTickSize( true );

		fLastBpm = fBpm;
		
		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			nFrames = frameDist( randomEngine );

			incrementTransportPosition( nFrames );

			if ( ! testCheckTransportPosition( "variable tempo" ) ) {
				setState( AudioEngine::State::Ready );
				unlock();
				return bNoMismatch;
			}

			if ( ( cc > 0 && getFrames() - nFrames != nLastFrame ) ||
				 // errors in the rescaling of nLastFrame are omitted.
				 ( cc == 0 && abs( getFrames() - nFrames - nLastFrame ) > 1 ) ) {
				ERRORLOG( QString( "[variable tempo] inconsistent frame update. getFrames(): %1, nFrames: %2, nLastFrame: %3" )
						  .arg( getFrames() ).arg( nFrames ).arg( nLastFrame ) );
				bNoMismatch = false;
				setState( AudioEngine::State::Ready );
				unlock();
				return bNoMismatch;
			}
			
			nLastFrame = getFrames();

			// Using the offset Hydrogen can keep track of the actual
			// number of frames passed since the playback was started
			// even in case a tempo change was issued by the user.
			nTotalFrames += nFrames;
			if ( getFrames() - m_nFrameOffset != nTotalFrames ) {
				ERRORLOG( QString( "[variable tempo] frame offset incorrect. getFrames(): %1, m_nFrameOffset: %2, nTotalFrames: %3" )
						  .arg( getFrames() ).arg( m_nFrameOffset ).arg( nTotalFrames ) );
				bNoMismatch = false;
				setState( AudioEngine::State::Ready );
				unlock();
				return bNoMismatch;
			}
		}

		nn++;

		if ( nn > nMaxCycles ) {
			ERRORLOG( "[variable tempo] end of the song wasn't reached in time." );
			bNoMismatch = false;
			break;
		}
	}

	setState( AudioEngine::State::Ready );

	unlock();

	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 1, 100 );
	pCoreActionController->addTempoMarker( 2, 20 );
	pCoreActionController->addTempoMarker( 3, 13.4 );
	pCoreActionController->addTempoMarker( 4, 383.2 );
	pCoreActionController->addTempoMarker( 5, 64.38372 );
	pCoreActionController->addTempoMarker( 6, 96.3 );
	pCoreActionController->addTempoMarker( 7, 240.46 );
	pCoreActionController->addTempoMarker( 8, 200.1 );

	lock( RIGHT_HERE );
	setState( AudioEngine::State::Prepared );

	// Check consistency after switching on the Timeline
	if ( ! testCheckTransportPosition( "timeline: off" ) ) {
		bNoMismatch = false;
	}
	
	nn = 0;
	nLastFrame = 0;

	while ( getDoubleTick() < m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		incrementTransportPosition( nFrames );
		updateBpmAndTickSize( true );

		if ( ! testCheckTransportPosition( "timeline" ) ) {
			bNoMismatch = false;
			break;
		}

		if ( getFrames() - nFrames != nLastFrame ) {
			ERRORLOG( QString( "[timeline] inconsistent frame update. getFrames(): %1, nFrames: %2, nLastFrame: %3" )
					  .arg( getFrames() ).arg( nFrames ).arg( nLastFrame ) );
			bNoMismatch = false;
			break;
		}
		nLastFrame = getFrames();

		nn++;

		if ( nn > nMaxCycles ) {
			ERRORLOG( "[timeline] end of the song wasn't reached in time." );
			bNoMismatch = false;
			break;
		}
	}

	setState( AudioEngine::State::Ready );

	unlock();

	// Check consistency after switching on the Timeline
	pCoreActionController->activateTimeline( false );

	lock( RIGHT_HERE );
	setState( AudioEngine::State::Prepared );

	if ( ! testCheckTransportPosition( "timeline: off" ) ) {
		bNoMismatch = false;
	}

	reset( false );

	setState( AudioEngine::State::Ready );

	unlock();

	// Check consistency of playback in PatternMode
	pCoreActionController->activateSongMode( false );

	lock( RIGHT_HERE );
	setState( AudioEngine::State::Prepared );

	nLastFrame = 0;
	fLastBpm = 0;
	nTotalFrames = 0;

	int nDifferentTempos = 10;

	for ( int tt = 0; tt < nDifferentTempos; ++tt ) {

		fBpm = tempoDist( randomEngine );

		nLastFrame = std::round( nLastFrame * fLastBpm / fBpm );
		
		setNextBpm( fBpm );
		updateBpmAndTickSize( true );

		fLastBpm = fBpm;
		
		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			nFrames = frameDist( randomEngine );

			incrementTransportPosition( nFrames );

			if ( ! testCheckTransportPosition( "pattern mode" ) ) {
				setState( AudioEngine::State::Ready );
				unlock();
				return bNoMismatch;
			}

			if ( ( cc > 0 && getFrames() - nFrames != nLastFrame ) ||
				 // errors in the rescaling of nLastFrame are omitted.
				 ( cc == 0 && abs( getFrames() - nFrames - nLastFrame ) > 1 ) ) {
				ERRORLOG( QString( "[pattern mode] inconsistent frame update. getFrames(): %1, nFrames: %2, nLastFrame: %3" )
						  .arg( getFrames() ).arg( nFrames ).arg( nLastFrame ) );
				bNoMismatch = false;
				setState( AudioEngine::State::Ready );
				unlock();
				return bNoMismatch;
			}
			
			nLastFrame = getFrames();

			// Using the offset Hydrogen can keep track of the actual
			// number of frames passed since the playback was started
			// even in case a tempo change was issued by the user.
			nTotalFrames += nFrames;
			if ( getFrames() - m_nFrameOffset != nTotalFrames ) {
				ERRORLOG( QString( "[pattern mode] frame offset incorrect. getFrames(): %1, m_nFrameOffset: %2, nTotalFrames: %3" )
						  .arg( getFrames() ).arg( m_nFrameOffset ).arg( nTotalFrames ) );
				bNoMismatch = false;
				setState( AudioEngine::State::Ready );
				unlock();
				return bNoMismatch;
			}
		}
	}
	
	reset( false );

	setState( AudioEngine::State::Ready );

	unlock();


	return bNoMismatch;
}

bool AudioEngine::testTransportRelocation() {
	auto pHydrogen = Hydrogen::get_instance();

	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 0, m_fSongSizeInTicks );
	std::uniform_int_distribution<long long> frameDist( 0, m_fSongSizeInTicks * 200 );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Prepared );

	// Check consistency of updated frames and ticks while relocating
	// transport.
	double fNewTick;
	long long nNewFrame;

	bool bNoMismatch = true;

	int nProcessCycles = 100;
	for ( int nn = 0; nn < nProcessCycles; ++nn ) {

		if ( nn < nProcessCycles - 1 ) {
			fNewTick = tickDist( randomEngine );
		} else {
			// There was a rounding error at this particular tick.
			fNewTick = 960;
		}

		locate( fNewTick, false );
		// The version of updateBpmAndTickSize called in locate() does
		// return immediately because the engine is currently in
		// State::Prepared.
		updateBpmAndTickSize( true );

		if ( ! testCheckTransportPosition( "mismatch tick-based" ) ) {
			bNoMismatch = false;
			break;
		}

		// Frame-based relocation
		nNewFrame = frameDist( randomEngine );

		locateToFrame( nNewFrame );
		// The version of updateBpmAndTickSize called in locate() does
		// return immediately because the engine is currently in
		// State::Prepared.
		updateBpmAndTickSize( true );

		if ( ! testCheckTransportPosition( "mismatch frame-based" ) ) {
			bNoMismatch = false;
			break;
		}
	}

	reset( false );
	
	setState( AudioEngine::State::Ready );

	unlock();


	return bNoMismatch;
}

bool AudioEngine::testComputeTickInterval() {
	auto pHydrogen = Hydrogen::get_instance();

	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
	std::uniform_real_distribution<float> frameDist( 1, 4096 );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Prepared );

	// Check consistency of tick intervals processed in
	// updateNoteQueue() (no overlap and no holes). We pretend to
	// receive transport position updates of random size (as e.g. used
	// in PulseAudio).
	
	double fTickStart, fTickEnd;
	double fLastTickStart = 0;
	double fLastTickEnd = 0;
	long long nLeadLagFactor;
	long long nLastLeadLagFactor = 0;
	int nFrames;

	bool bNoMismatch = true;

	int nProcessCycles = 100;
	for ( int nn = 0; nn < nProcessCycles; ++nn ) {

		nFrames = frameDist( randomEngine );

		nLeadLagFactor = computeTickInterval( &fTickStart, &fTickEnd,
											  nFrames );

		if ( nLastLeadLagFactor != 0 &&
			 nLastLeadLagFactor != nLeadLagFactor ) {
			ERRORLOG( QString( "[constant tempo] There should not be altering lead lag with constant tempo [new: %1, prev: %2].")
					  .arg( nLeadLagFactor ).arg( nLastLeadLagFactor ) );
			bNoMismatch = false;
		}
		nLastLeadLagFactor = nLeadLagFactor;	

		if ( nn == 0 && fTickStart != 0 ){
			ERRORLOG( QString( "[constant tempo] First interval [%1,%2] does not start at 0.")
					  .arg( fTickStart, 0, 'f' ).arg( fTickEnd, 0, 'f' ) );
			bNoMismatch = false;
		}

		if ( fTickStart != fLastTickEnd ) {
			ERRORLOG( QString( "[variable tempo] Interval [%1,%2] does not align with previous one [%3,%4]. nFrames: %5, curr tick: %6, curr frames: %7, bpm: %8, tick size: %9, nLeadLagFactor: %10")
					  .arg( fTickStart, 0, 'f' )
					  .arg( fTickEnd, 0, 'f' )
					  .arg( fLastTickStart, 0, 'f' )
					  .arg( fLastTickEnd, 0, 'f' )
					  .arg( nFrames )
					  .arg( getDoubleTick(), 0, 'f' )
					  .arg( getFrames() )
					  .arg( getBpm(), 0, 'f' )
					  .arg( getTickSize(), 0, 'f' )
					  .arg( nLeadLagFactor )
					  );
			bNoMismatch = false;
		}
		
		fLastTickStart = fTickStart;
		fLastTickEnd = fTickEnd;

		incrementTransportPosition( nFrames );
	}

	reset( false );

	fLastTickStart = 0;
	fLastTickEnd = 0;
	
	float fBpm;

	int nTempoChanges = 20;
	int nProcessCyclesPerTempo = 5;
	for ( int tt = 0; tt < nTempoChanges; ++tt ) {

		fBpm = tempoDist( randomEngine );
		setNextBpm( fBpm );
		updateBpmAndTickSize( true );
		
		for ( int cc = 0; cc < nProcessCyclesPerTempo; ++cc ) {

			nFrames = frameDist( randomEngine );

			nLeadLagFactor = computeTickInterval( &fTickStart, &fTickEnd,
												  nFrames );

			if ( cc == 0 && tt == 0 && fTickStart != 0 ){
				ERRORLOG( QString( "[variable tempo] First interval [%1,%2] does not start at 0.")
						  .arg( fTickStart, 0, 'f' )
						  .arg( fTickEnd, 0, 'f' ) );
				bNoMismatch = false;
			}

			if ( fTickStart != fLastTickEnd ) {
				ERRORLOG( QString( "[variable tempo] Interval [%1,%2] does not align with previous one [%3,%4]. nFrames: %5, curr tick: %6, curr frames: %7, bpm: %8, tick size: %9, nLeadLagFactor: %10")
						  .arg( fTickStart, 0, 'f' )
						  .arg( fTickEnd, 0, 'f' )
						  .arg( fLastTickStart, 0, 'f' )
						  .arg( fLastTickEnd, 0, 'f' )
						  .arg( nFrames )
						  .arg( getDoubleTick(), 0, 'f' )
						  .arg( getFrames() )
						  .arg( getBpm(), 0, 'f' )
						  .arg( getTickSize(), 0, 'f' )
						  .arg( nLeadLagFactor )
						  );
				bNoMismatch = false;
			}

			fLastTickStart = fTickStart;
			fLastTickEnd = fTickEnd;

			incrementTransportPosition( nFrames );
		}
	}
	
	reset( false );

	setState( AudioEngine::State::Ready );

	unlock();


	return bNoMismatch;
}


bool AudioEngine::testSongSizeChangeInLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( true, false );

	lock( RIGHT_HERE );

	int nColumns = pHydrogen->getSong()->getPatternGroupVector()->size();

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> frameDist( 1, m_fSongSizeInTicks );
	std::uniform_int_distribution<int> columnDist( nColumns, nColumns + 100 );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Prepared );

	uint32_t nFrames = 500;
	double fInitialSongSize = m_fSongSizeInTicks;
	int nNewColumn;

	bool bNoMismatch = true;

	int nNumberOfTogglings = 1;

	for ( int nn = 0; nn < nNumberOfTogglings; ++nn ) {

		locate( fInitialSongSize + frameDist( randomEngine ) );

		if ( ! testCheckTransportPosition( "relocation" ) ) {
			bNoMismatch = false;
			break;
		}

		incrementTransportPosition( nFrames );
		updateBpmAndTickSize( true );

		if ( ! testCheckTransportPosition( "first increment" ) ) {
			bNoMismatch = false;
			break;
		}

		nNewColumn = columnDist( randomEngine );

		unlock();
		pCoreActionController->toggleGridCell( nNewColumn, 0 );
		lock( RIGHT_HERE );

		if ( ! testCheckTransportPosition( "first toggling" ) ) {
			bNoMismatch = false;
			break;
		}

		if ( fInitialSongSize == m_fSongSizeInTicks ) {
			ERRORLOG( QString( "[first toggling] no song enlargement %1")
					  .arg( m_fSongSizeInTicks ) );
			bNoMismatch = false;
			break;
		}

		incrementTransportPosition( nFrames );
		updateBpmAndTickSize( true );

		if ( ! testCheckTransportPosition( "second increment" ) ) {
			bNoMismatch = false;
			break;
		}
														  
		unlock();
		pCoreActionController->toggleGridCell( nNewColumn, 0 );
		lock( RIGHT_HERE );

		if ( ! testCheckTransportPosition( "second toggling" ) ) {
			bNoMismatch = false;
			break;
		}
		
		if ( fInitialSongSize != m_fSongSizeInTicks ) {
			ERRORLOG( QString( "[second toggling] song size mismatch original: %1, new: %2" )
					  .arg( fInitialSongSize ).arg( m_fSongSizeInTicks ) );
			bNoMismatch = false;
			break;
		}

		incrementTransportPosition( nFrames );
		updateBpmAndTickSize( true );

		if ( ! testCheckTransportPosition( "third increment" ) ) {
			bNoMismatch = false;
			break;
		}
	}

	setState( AudioEngine::State::Ready );

	unlock();

	return bNoMismatch;
}

bool AudioEngine::testCheckTransportPosition( const QString& sContext) const {

	double fTickOffset;
	double fCheckTick = computeTickFromFrame( getFrames() );
	long long nCheckFrame = computeFrameFromTick( getDoubleTick(), &fTickOffset );
	
	if ( abs( fCheckTick + fTickOffset - getDoubleTick() ) > 1e-9 ||
		 abs( fTickOffset - m_fTickOffset ) > 1e-9 ||
		 nCheckFrame != getFrames() ) {
		ERRORLOG( QString( "[%9] mismatch. frame: %1, check frame: %2, tick: %3, check tick: %4, offset: %5, check offset: %6, tick size: %7, bpm: %8" )
				  .arg( getFrames() )
				  .arg( nCheckFrame )
				  .arg( getDoubleTick(), 0 , 'f', 9 )
				  .arg( fCheckTick, 0 , 'f', 9 )
				  .arg( m_fTickOffset, 0 , 'f', 9 )
				  .arg( fTickOffset, 0 , 'f', 9 )
				  .arg( getTickSize(), 0 , 'f' )
				  .arg( getBpm(), 0 , 'f' )
				  .arg( sContext )
				  );
		return false;
	}

	return true;
}

QString AudioEngine::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[AudioEngine]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nFrames: %3\n" ).arg( sPrefix ).arg( s ).arg( getFrames() ) )
			.append( QString( "%1%2m_fTick: %3\n" ).arg( sPrefix ).arg( s ).arg( getDoubleTick(), 0, 'f' ) )
			.append( QString( "%1%2m_fTickSize: %3\n" ).arg( sPrefix ).arg( s ).arg( getTickSize(), 0, 'f' ) )
			.append( QString( "%1%2m_fBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( getBpm(), 0, 'f' ) )
			.append( QString( "%1%2m_fNextBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( AudioEngine::m_fNextBpm, 0, 'f' ) )
			.append( QString( "%1%2m_state: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_state) ) )
			.append( QString( "%1%2m_nextState: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_nextState) ) )
			.append( QString( "%1%2m_currentTickTime: %3 ms\n" ).arg( sPrefix ).arg( s ).arg( m_currentTickTime.tv_sec * 1000 + m_currentTickTime.tv_usec / 1000) )
			.append( QString( "%1%2m_nPatternStartTick: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPatternStartTick ) )
			.append( QString( "%1%2m_nPatternTickPosition: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPatternTickPosition ) )
			.append( QString( "%1%2m_nColumn: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nColumn ) )
			.append( QString( "%1%2m_fSongSizeInTicks: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fSongSizeInTicks, 0, 'f' ) )
			.append( QString( "%1%2m_fTickOffset: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fTickOffset, 0, 'f' ) )
			.append( QString( "%1%2m_fLastTickIntervalEnd: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fLastTickIntervalEnd ) )
			.append( QString( "%1%2m_pSampler: \n" ).arg( sPrefix ).arg( s ) )
			.append( QString( "%1%2m_pSynth: \n" ).arg( sPrefix ).arg( s ) )
			.append( QString( "%1%2m_pAudioDriver: \n" ).arg( sPrefix ).arg( s ) )
			.append( QString( "%1%2m_pMidiDriver: \n" ).arg( sPrefix ).arg( s ) )
			.append( QString( "%1%2m_pMidiDriverOut: \n" ).arg( sPrefix ).arg( s ) )
			.append( QString( "%1%2m_pEventQueue: \n" ).arg( sPrefix ).arg( s ) );
#ifdef H2CORE_HAVE_LADSPA
		sOutput.append( QString( "%1%2m_fFXPeak_L: [" ).arg( sPrefix ).arg( s ) );
		for ( auto ii : m_fFXPeak_L ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( QString( "]\n%1%2m_fFXPeak_R: [" ).arg( sPrefix ).arg( s ) );
		for ( auto ii : m_fFXPeak_R ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( QString( " ]\n" ) );
#endif
		sOutput.append( QString( "%1%2m_fMasterPeak_L: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fMasterPeak_L ) )
			.append( QString( "%1%2m_fMasterPeak_R: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fMasterPeak_R ) )
			.append( QString( "%1%2m_fProcessTime: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fProcessTime ) )
			.append( QString( "%1%2m_fMaxProcessTime: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fMaxProcessTime ) )
			.append( QString( "%1%2m_pNextPatterns: %3\n" ).arg( sPrefix ).arg( s ).arg( m_pNextPatterns->toQString( sPrefix + s ), bShort ) )
			.append( QString( "%1%2m_pPlayingPatterns: %3\n" ).arg( sPrefix ).arg( s ).arg( m_pPlayingPatterns->toQString( sPrefix + s ), bShort ) )
			.append( QString( "%1%2m_nRealtimeFrames: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nRealtimeFrames ) )
			.append( QString( "%1%2m_nAddRealtimeNoteTickPosition: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nAddRealtimeNoteTickPosition ) )
			.append( QString( "%1%2m_AudioProcessCallback: \n" ).arg( sPrefix ).arg( s ) )
			.append( QString( "%1%2m_songNoteQueue: length = %3\n" ).arg( sPrefix ).arg( s ).arg( m_songNoteQueue.size() ) );
		sOutput.append( QString( "%1%2m_midiNoteQueue: [\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& nn : m_midiNoteQueue ) {
			sOutput.append( nn->toQString( sPrefix + s, bShort ) );
		}
		sOutput.append( QString( "]\n%1%2m_pMetronomeInstrument: %3\n" ).arg( sPrefix ).arg( s ).arg( m_pMetronomeInstrument->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2nMaxTimeHumanize: %3\n" ).arg( sPrefix ).arg( s ).arg( AudioEngine::nMaxTimeHumanize ) );
		
	} else {
		sOutput = QString( "%1[AudioEngine]" ).arg( sPrefix )
			.append( QString( ", m_nFrames: %1" ).arg( getFrames() ) )
			.append( QString( ", m_fTick: %1" ).arg( getDoubleTick(), 0, 'f' ) )
			.append( QString( ", m_fTickSize: %1" ).arg( getTickSize(), 0, 'f' ) )
			.append( QString( ", m_fBpm: %1" ).arg( getBpm(), 0, 'f' ) )
			.append( QString( ", m_fNextBpm: %1" ).arg( AudioEngine::m_fNextBpm, 0, 'f' ) )
			.append( QString( ", m_state: %1" ).arg( static_cast<int>(m_state) ) )
			.append( QString( ", m_nextState: %1" ).arg( static_cast<int>(m_nextState) ) )
			.append( QString( ", m_currentTickTime: %1 ms" ).arg( m_currentTickTime.tv_sec * 1000 + m_currentTickTime.tv_usec / 1000) )
			.append( QString( ", m_nPatternStartTick: %1" ).arg( m_nPatternStartTick ) )
			.append( QString( ", m_nPatternTickPosition: %1" ).arg( m_nPatternTickPosition ) )
			.append( QString( ", m_nColumn: %1" ).arg( m_nColumn ) )
			.append( QString( ", m_fSongSizeInTicks: %1" ).arg( m_fSongSizeInTicks, 0, 'f' ) )
			.append( QString( ", m_fTickOffset: %1" ).arg( m_fTickOffset, 0, 'f' ) )
			.append( QString( ", m_fLastTickIntervalEnd: %1" ).arg( m_fLastTickIntervalEnd ) )
			.append( QString( ", m_pSampler:" ) )
			.append( QString( ", m_pSynth:" ) )
			.append( QString( ", m_pAudioDriver:" ) )
			.append( QString( ", m_pMidiDriver:" ) )
			.append( QString( ", m_pMidiDriverOut:" ) )
			.append( QString( ", m_pEventQueue:" ) );
#ifdef H2CORE_HAVE_LADSPA
		sOutput.append( QString( ", m_fFXPeak_L: [" ) );
		for ( auto ii : m_fFXPeak_L ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( QString( "], m_fFXPeak_R: [" ) );
		for ( auto ii : m_fFXPeak_R ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( QString( " ]" ) );
#endif
		sOutput.append( QString( ", m_fMasterPeak_L: %1" ).arg( m_fMasterPeak_L ) )
			.append( QString( ", m_fMasterPeak_R: %1" ).arg( m_fMasterPeak_R ) )
			.append( QString( ", m_fProcessTime: %1" ).arg( m_fProcessTime ) )
			.append( QString( ", m_fMaxProcessTime: %1" ).arg( m_fMaxProcessTime ) )
			.append( QString( ", m_pNextPatterns: %1" ).arg( m_pNextPatterns->toQString( sPrefix + s ), bShort ) )
			.append( QString( ", m_pPlayingPatterns: %1" ).arg( m_pPlayingPatterns->toQString( sPrefix + s ), bShort ) )
			.append( QString( ", m_nRealtimeFrames: %1" ).arg( m_nRealtimeFrames ) )
			.append( QString( ", m_nAddRealtimeNoteTickPosition: %1" ).arg( m_nAddRealtimeNoteTickPosition ) )
			.append( QString( ", m_AudioProcessCallback:" ) )
			.append( QString( ", m_songNoteQueue: length = %1" ).arg( m_songNoteQueue.size() ) );
		sOutput.append( QString( ", m_midiNoteQueue: [" ) );
		for ( const auto& nn : m_midiNoteQueue ) {
			sOutput.append( nn->toQString( sPrefix + s, bShort ) );
		}
		sOutput.append( QString( "], m_pMetronomeInstrument: id = %1" ).arg( m_pMetronomeInstrument->get_id() ) )
			.append( QString( ", nMaxTimeHumanize: id %1" ).arg( AudioEngine::nMaxTimeHumanize ) );
	}
	
	return sOutput;
}

void AudioEngineLocking::assertAudioEngineLocked() const 
{
#ifndef NDEBUG
		if ( m_bNeedsLock ) {
			H2Core::Hydrogen::get_instance()->getAudioEngine()->assertLocked();
		}
#endif
}

}; // namespace H2Core
