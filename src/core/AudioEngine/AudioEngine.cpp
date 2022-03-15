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
		, m_nPatternStartTick( 0 )
		, m_nPatternTickPosition( 0 )
		, m_fSongSizeInTicks( 0 )
		, m_nRealtimeFrames( 0 )
		, m_nAddRealtimeNoteTickPosition( 0 )
		, m_fMasterPeak_L( 0.0f )
		, m_fMasterPeak_R( 0.0f )
		, m_nColumn( -1 )
		, m_nextState( State::Ready )
		, m_fProcessTime( 0.0f )
		, m_fLadspaTime( 0.0f )
		, m_fMaxProcessTime( 0.0f )
		, m_fNextBpm( 120 )
		, m_fTickMismatch( 0 )
		, m_fLastTickIntervalEnd( -1 )
		, m_nLastPlayingPatternsColumn( -1 )
		, m_nFrameOffset( 0 )
		, m_fTickOffset( 0 )
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
		ERRORLOG( "Error the audio engine is not in State::Initialized" );
		return;
	}
	m_pSampler->stopPlayingNotes();

	this->lock( RIGHT_HERE );
	INFOLOG( "*** Hydrogen audio engine shutdown ***" );

	clearNoteQueue();

	// change the current audio engine state
	setState( State::Uninitialized );

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
	INFOLOG( "" );

	// check current state
	if ( getState() != State::Ready ) {
	   ERRORLOG( "Error the audio engine is not in State::Ready" );
		return;
	}

	// change the current audio engine state
	setState( State::Playing );
}

void AudioEngine::stopPlayback()
{
	INFOLOG( "" );

	// check current state
	if ( getState() != State::Playing ) {
		ERRORLOG( QString( "Error the audio engine is not in State::Playing but [%1]" )
				  .arg( static_cast<int>( getState() ) ) );
		return;
	}

	setState( State::Ready );
}

void AudioEngine::reset( bool bWithJackBroadcast ) {
	const auto pHydrogen = Hydrogen::get_instance();
	
	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;

	setFrames( 0 );
	setTick( 0 );
	setColumn( -1 );
	m_nPatternStartTick = 0;
	m_nPatternTickPosition = 0;
	m_fTickMismatch = 0;
	m_nFrameOffset = 0;
	m_fTickOffset = 0;
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

double AudioEngine::computeDoubleTickSize( const int nSampleRate, const float fBpm, const int nResolution)
{
	double fTickSize = static_cast<double>(nSampleRate) * 60.0 /
		static_cast<double>(fBpm) /
		static_cast<double>(nResolution);
	
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

	long long nNewFrame;

	// DEBUGLOG( QString( "fTick: %1" ).arg( fTick ) );

#ifdef H2CORE_HAVE_JACK
	// In case Hydrogen is using the JACK server to sync transport, it
	// has to be up to the server to relocate to a different
	// position. However, if the transport is not rolling, the JACK
	// server won't broadcast the new position we asked for until the
	// transport is rolling again. That's why we relocate internally
	// too - as we do not have to be afraid for transport to get out
	// of sync as it is not rolling.
	if ( pHydrogen->haveJackTransport() && bWithJackBroadcast &&
		 m_state == State::Playing ) {
		nNewFrame = computeFrameFromTick( fTick, &m_fTickMismatch );
	} else {
		reset( false );
		nNewFrame = computeFrameFromTick( fTick, &m_fTickMismatch );
	}

	if ( pHydrogen->haveJackTransport() && bWithJackBroadcast ) {
		static_cast<JackAudioDriver*>( m_pAudioDriver )->locateTransport( nNewFrame );

		if ( m_state == State::Playing ) {
			return;
		}
	}
#else
	reset( false );
	nNewFrame = computeFrameFromTick( fTick, &m_fTickMismatch );
#endif
	
	setFrames( nNewFrame );
	updateTransportPosition( fTick, pHydrogen->getSong()->isLoopEnabled() );

	// Ensure the playing patterns are updated regardless of whether
	// transport is rolling or not.
	updatePlayingPatterns( getColumn(), getTick(), getPatternStartTick() );
}

void AudioEngine::locateToFrame( const long long nFrame ) {
	const auto pHydrogen = Hydrogen::get_instance();
	
	reset( false );
	setFrames( nFrame );

	double fNewTick = computeTickFromFrame( nFrame );
	
	updateTransportPosition( fNewTick, pHydrogen->getSong()->isLoopEnabled() );
	
	// Ensure the playing patterns are updated regardless of whether
	// transport is rolling or not.	
	updatePlayingPatterns( getColumn(), getTick(), getPatternStartTick() );
}

void AudioEngine::incrementTransportPosition( uint32_t nFrames ) {
	auto pSong = Hydrogen::get_instance()->getSong();

	if ( pSong == nullptr ) {
		return;
	}	

	setFrames( getFrames() + nFrames );

	double fNewTick = computeTickFromFrame( getFrames() );
	m_fTickMismatch = 0;
		
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

	// WARNINGLOG( QString( "[Before] frame: %5, tick: %1, pTickPos: %2, pStartPos: %3, column: %4" )
	// 			.arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick )
	// 			.arg( m_nColumn )
	// 			.arg( getFrames() ) );
	
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
			m_nLastPlayingPatternsColumn = m_nColumn;
			setColumn( nNewColumn );
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
		if ( fTick >= m_nPatternStartTick + nPatternSize  ) {
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
	
	// WARNINGLOG( QString( "[After] frame: %5, tick: %1, pTickPos: %2, pStartPos: %3, column: %4" )
	// 			.arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick )
	// 			.arg( m_nColumn )
	// 			.arg( getFrames() ) );
	
}

void AudioEngine::updateBpmAndTickSize() {
	if ( ! ( m_state == State::Playing ||
			 m_state == State::Ready ||
			 m_state == State::Testing ) ) {
		return;
	}
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	float fOldBpm = getBpm();
	
	float fNewBpm = getBpmAtColumn( pHydrogen->getAudioEngine()->getColumn() );
	if ( fNewBpm != getBpm() ) {
		setBpm( fNewBpm );
		EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, 0 );
	}

	float fOldTickSize = getTickSize();
	float fNewTickSize = AudioEngine::computeTickSize( static_cast<float>(m_pAudioDriver->getSampleRate()),
													   getBpm(), pSong->getResolution() );

	// DEBUGLOG(QString( "sample rate: %1, tick size: %2 -> %3, bpm: %4 -> %5" )
	// 		 .arg( static_cast<float>(m_pAudioDriver->getSampleRate()))
	// 		 .arg( fOldTickSize, 0, 'f' )
	// 		 .arg( fNewTickSize, 0, 'f' )
	// 		 .arg( fOldBpm, 0, 'f' )
	// 		 .arg( getBpm(), 0, 'f' ) );
	
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

	if ( ! pHydrogen->isTimelineEnabled() ) {
		// If we deal with a single speed for the whole song, the frames
		// since the beginning of the song are tempo-dependent and have to
		// be recalculated.
		long long nNewFrames = computeFrameFromTick( getDoubleTick(), &m_fTickMismatch );
		m_nFrameOffset = nNewFrames - getFrames() + m_nFrameOffset;

		// DEBUGLOG( QString( "old frame: %1, new frame: %2, tick: %3, old tick size: %4, new tick size: %5" )
		// 		  .arg( getFrames() ).arg( nNewFrames ).arg( getDoubleTick(), 0, 'f' )
		// 		  .arg( fOldTickSize, 0, 'f' ).arg( fNewTickSize, 0, 'f' ) );
		
		setFrames( nNewFrames );

		// In addition, all currently processed notes have to be
		// updated to be still valid.
		handleTempoChange();
		
	} else if ( m_nFrameOffset != 0 ) {
		// In case the frame offset was already set, we have to update
		// it too in order to prevent inconsistencies in the transport
		// and audio rendering when switching off the Timeline during
		// playback, relocating, switching it on again, alter song
		// size or sample rate, and switchting it off again. I know,
		// quite an edge case but still.
		// If we deal with a single speed for the whole song, the frames
		// since the beginning of the song are tempo-dependent and have to
		// be recalculated.
		long long nNewFrames = computeFrameFromTick( getDoubleTick(),
													 &m_fTickMismatch );
		m_nFrameOffset = nNewFrames - getFrames() + m_nFrameOffset;
	}
}
				
// This function uses the assumption that sample rate and resolution
// are constant over the whole song.
long long AudioEngine::computeFrameFromTick( const double fTick, double* fTickMismatch, int nSampleRate ) const {

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	assert( pSong );

	if ( nSampleRate == 0 ) {
		nSampleRate = pHydrogen->getAudioOutput()->getSampleRate();
	}
	const int nResolution = pSong->getResolution();

	const double fTickSize = AudioEngine::computeDoubleTickSize( nSampleRate,
																getBpm(),
																nResolution );
	
	if ( nSampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		*fTickMismatch = 0;
		return 0;
	}

	if ( fTick == 0 ) {
		*fTickMismatch = 0;
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
					AudioEngine::computeDoubleTickSize( nSampleRate,
														tempoMarkers[ ii - 1 ]->fBpm,
														nResolution );
				
				if ( fRemainingTicks > ( fNextTick - fPassedTicks ) ) {
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the current transport position.
					fNewFrames += ( fNextTick - fPassedTicks ) * fNextTickSize;

					// DEBUGLOG( QString( "[segment] fTick: %1, fNewFrames: %2, nNextTick: %3, nRemainingTicks: %4, nPassedTicks: %5, fNextTickSize: %6, col: %7, bpm: %8, tick increment: %9, frame increment: %10" )
					// 		  .arg( fTick, 0, 'f' )
					// 		  .arg( fNewFrames, 0, 'f' )
					// 		  .arg( fNextTick, 0, 'f' )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fPassedTicks, 0, 'f' )
					// 		  .arg( fNextTickSize, 0, 'f' )
					// 		  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					// 		  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					// 		  .arg( fNextTick - fPassedTicks, 0, 'f' )
					// 		  .arg( ( fNextTick - fPassedTicks ) * fNextTickSize, 0, 'g', 30 )
					// 		  );
					
					fRemainingTicks -= fNextTick - fPassedTicks;
					
					fPassedTicks = fNextTick;

				} else {
					// We are within this segment.
					fNewFrames += fRemainingTicks * fNextTickSize;

					nNewFrames = static_cast<long long>( std::round( fNewFrames ) );
					if ( fRemainingTicks != ( fNextTick - fPassedTicks ) ) {
						*fTickMismatch = ( fNewFrames - static_cast<double>( nNewFrames ) ) /
							fNextTickSize;
					} else {
						// We ended at the very tick containing a
						// tempo marker. If the mismatch is negative,
						// we round the tick to a higher value, we
						// need to use the tick size of the next tempo
						// marker. For positive values, the mismatch
						// resides on "this" side of the tempo marker.
						double fMismatchInFrames =
							fNewFrames - static_cast<double>( nNewFrames );
						if ( fMismatchInFrames < 0 ) {
							// Get the tick size of the next tempo
							// marker.
							if ( ii < tempoMarkers.size() ) {

								fNextTickSize =
									AudioEngine::computeDoubleTickSize( nSampleRate,
																		tempoMarkers[ ii ]->fBpm,
																		nResolution );
							} else {
								fNextTickSize =
									AudioEngine::computeDoubleTickSize( nSampleRate,
																		tempoMarkers[ 0 ]->fBpm,
																		nResolution );
							}
						}
						*fTickMismatch = fMismatchInFrames /
							fNextTickSize;
					}

					// DEBUGLOG( QString( "[end] fTick: %1, fNewFrames: %2, nNewFrames: %9, fTickMismatch: %10, nNextTick: %3, nRemainingTicks: %4, nPassedTicks: %5, fNextTickSize: %6, col: %7, bpm: %8, tick increment: %11, frame increment: %12" )
					// 		  .arg( fTick, 0, 'f' )
					// 		  .arg( fNewFrames, 0, 'g', 30 )
					// 		  .arg( fNextTick, 0, 'f' )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fPassedTicks, 0, 'f' )
					// 		  .arg( fNextTickSize, 0, 'f' )
					// 		  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					// 		  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					// 		  .arg( nNewFrames )
					// 		  .arg( *fTickMismatch, 0, 'g', 30 )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fRemainingTicks * fNextTickSize, 0, 'g', 30 )
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
				double fSongSizeInFrames = fNewFrames;
				
				fNewFrames *= static_cast<double>(nRepetitions);
				fNewTick = std::fmod( fTick, m_fSongSizeInTicks );
				fRemainingTicks = fNewTick;
				fPassedTicks = 0;

				// DEBUGLOG( QString( "[repeat] frames covered: %1, ticks covered: %2, ticks remaining: %3, nRepetitions: %4, fSongSizeInFrames: %5" )
				// 		  .arg( fNewFrames, 0, 'g', 30 )
				// 		  .arg( fTick - fNewTick, 0, 'g', 30 )
				// 		  .arg( fRemainingTicks, 0, 'g', 30 )
				// 		  .arg( nRepetitions )
				// 		  .arg( fSongSizeInFrames, 0, 'g', 30 )
				// 		  );

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
			fTickSize;
		nNewFrames = static_cast<long long>( std::round( fNewFrames ) );
		*fTickMismatch = ( fNewFrames - static_cast<double>(nNewFrames ) ) /
			fTickSize;

		// DEBUGLOG(QString("[no-timeline] nNewFrames: %1, fTick: %2, fTickSize: %3, fTickMismatch: %4" )
		// 		 .arg( nNewFrames ).arg( fTick, 0, 'f' ).arg( fTickSize, 0, 'f' )
		// 		 .arg( *fTickMismatch, 0, 'g', 30 ));
		
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

	const double fTickSize =
		AudioEngine::computeDoubleTickSize( nSampleRate,
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
		double fTargetFrames = static_cast<double>(nFrame);
		double fPassedFrames = 0;
		double fNextFrames = 0;
		double fNextTicks, fPassedTicks = 0;
		double fNextTickSize;
		long long nRemainingFrames;

		int nColumns = pSong->getPatternGroupVector()->size();

		while ( fPassedFrames < fTargetFrames ) {
		
			for ( int ii = 1; ii <= tempoMarkers.size(); ++ii ) {

				fNextTickSize =
					AudioEngine::computeDoubleTickSize( nSampleRate,
														tempoMarkers[ ii - 1 ]->fBpm,
														nResolution );
				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTicks = m_fSongSizeInTicks;
				} else {
					fNextTicks =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ));
				}
				fNextFrames = (fNextTicks - fPassedTicks) * fNextTickSize;
		
				if ( fNextFrames < ( fTargetFrames -
									 fPassedFrames ) ) {

					// DEBUGLOG(QString( "[segment] nFrame: %1, fTick: %11, sampleRate: %2, tickSize: %3, nNextTicks: %5, fNextFrames: %6, col: %7, bpm: %8, nPassedTicks: %9, fPassedFrames: %10, tick increment: %12, frame increment: %13" )
					// 		 .arg( nFrame )
					// 		 .arg( nSampleRate )
					// 		 .arg( fNextTickSize, 0, 'f' )
					// 		 .arg( fNextTicks, 0, 'f' )
					// 		 .arg( fNextFrames, 0, 'f' )
					// 		 .arg( tempoMarkers[ ii -1 ]->nColumn )
					// 		 .arg( tempoMarkers[ ii -1 ]->fBpm )
					// 		 .arg( fPassedTicks, 0, 'f' )
					// 		 .arg( fPassedFrames, 0, 'f' )
					// 		 .arg( fTick, 0, 'f' )
					// 		 .arg( fNextTicks - fPassedTicks, 0, 'f' )
					// 		 .arg( (fNextTicks - fPassedTicks) * fNextTickSize, 0, 'g', 30 )
					// 		 );
					
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the transport position.
					fTick += fNextTicks - fPassedTicks;

					fPassedFrames += fNextFrames;
					fPassedTicks = fNextTicks;

				} else {
					// We are within this segment.
					// We use a floor in here because only integers
					// frames are supported.
					double fNewTick = (fTargetFrames - fPassedFrames ) /
						fNextTickSize;

					fTick += fNewTick;
											
					fPassedFrames = fTargetFrames;
					
					// DEBUGLOG(QString( "[end] nFrame: %1, fTick: %11, sampleRate: %2, tickSize: %3, nNextTicks: %5, fNextFrames: %6, col: %7, bpm: %8, nPassedTicks: %9, fPassedFrames: %10, tick increment: %12, frame increment: %13" )
					// 		 .arg( nFrame )
					// 		 .arg( nSampleRate )
					// 		 .arg( fNextTickSize, 0, 'f' )
					// 		 .arg( fNextTicks, 0, 'f' )
					// 		 .arg( fNextFrames, 0, 'f' )
					// 		 .arg( tempoMarkers[ ii -1 ]->nColumn )
					// 		 .arg( tempoMarkers[ ii -1 ]->fBpm )
					// 		 .arg( fPassedTicks, 0, 'f' )
					// 		 .arg( fPassedFrames, 0, 'f' )
					// 		 .arg( fTick, 0, 'f' )
					// 		 .arg( fNewTick, 0, 'f' )
					// 		 .arg( fNewTick * fNextTickSize, 0, 'g', 30 )
					// 		 );
					
					break;
				}
			}

			if ( fPassedFrames != fTargetFrames ) {
				// The provided nFrame is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames.
				double fSongSizeInFrames = fPassedFrames;
				int nRepetitions = std::floor(fTargetFrames / fSongSizeInFrames);
				if ( m_fSongSizeInTicks * nRepetitions >
					 std::numeric_limits<double>::max() ) {
					ERRORLOG( QString( "Provided frames [%1] are too large." ).arg( nFrame ) );
					return 0;
				}
				fTick = m_fSongSizeInTicks * nRepetitions;

				fPassedFrames = static_cast<double>(nRepetitions) *
					fSongSizeInFrames;
				fPassedTicks = 0;

				// DEBUGLOG( QString( "[repeat] frames covered: %1, frames remaining: %2, ticks covered: %3,  nRepetitions: %4, fSongSizeInFrames: %5" )
				// 		  .arg( fPassedFrames, 0, 'g', 30 )
				// 		  .arg( fTargetFrames - fPassedFrames, 0, 'g', 30 )
				// 		  .arg( fTick, 0, 'g', 30 )
				// 		  .arg( nRepetitions )
				// 		  .arg( fSongSizeInFrames, 0, 'g', 30 )
				// 		  );
				
			}
		}
	} else {
	
		// No Timeline. Constant tempo/tick size for the whole song.
		fTick = static_cast<double>(nFrame) / fTickSize;

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
		JackAudioDriver* pJackAudioDriver = static_cast<JackAudioDriver*>(m_pAudioDriver);
	
		if( pJackAudioDriver ) {
			pJackAudioDriver->clearPerTrackAudioBuffers( nFrames );
		}
	}
#endif

	mx.unlock();

#ifdef H2CORE_HAVE_LADSPA
	if ( getState() == State::Ready ||
		 getState() == State::Playing ||
		 getState() == State::Testing ) {
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
	INFOLOG( QString( "Driver: '%1'" ).arg( sDriver ) );
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *pDriver = nullptr;

	if ( sDriver == "OSS" ) {
		pDriver = new OssDriver( m_AudioProcessCallback );
	} else if ( sDriver == "JACK" ) {
		pDriver = new JackAudioDriver( m_AudioProcessCallback );
#ifdef H2CORE_HAVE_JACK
		if ( auto pJackDriver = dynamic_cast<JackAudioDriver*>( pDriver ) ) {
			pJackDriver->setConnectDefaults(
				Preferences::get_instance()->m_bJackConnectDefaults
			);
		}
#endif
	} else if ( sDriver == "ALSA" ) {
		pDriver = new AlsaAudioDriver( m_AudioProcessCallback );
	} else if ( sDriver == "PortAudio" ) {
		pDriver = new PortAudioDriver( m_AudioProcessCallback );
	} else if ( sDriver == "CoreAudio" ) {
		pDriver = new CoreAudioDriver( m_AudioProcessCallback );
	} else if ( sDriver == "PulseAudio" ) {
		pDriver = new PulseAudioDriver( m_AudioProcessCallback );
	} else if ( sDriver == "Fake" ) {
		WARNINGLOG( "*** Using FAKE audio driver ***" );
		pDriver = new FakeDriver( m_AudioProcessCallback );
	} else {
		ERRORLOG( QString( "Unknown driver [%1]" ).arg( sDriver ) );
		raiseError( Hydrogen::UNKNOWN_DRIVER );
	}

	if ( dynamic_cast<NullDriver*>(pDriver) != nullptr ) {
		delete pDriver;
		pDriver = nullptr;
	}

	if ( pDriver != nullptr ) {
		// initialize the audio driver
		int res = pDriver->init( pPref->m_nBufferSize );
		if ( res != 0 ) {
			ERRORLOG( QString( "Error initializing audio driver [%1]. Error code: %2" )
					  .arg( sDriver ).arg( res ) );
			delete pDriver;
			pDriver = nullptr;
		}
	}
	return pDriver;
}

void AudioEngine::startAudioDrivers()
{
	INFOLOG("");
	Preferences *preferencesMng = Preferences::get_instance();

	// Lock both the AudioEngine and the audio output buffers.
	this->lock( RIGHT_HERE );
	QMutexLocker mx(&m_MutexOutputPointer);
	
	// check current state
	if ( getState() != State::Initialized ) {
		ERRORLOG( QString( "Audio engine is not in State::Initialized but [%1]" )
				  .arg( static_cast<int>( getState() ) ) );
		this->unlock();
		return;
	}

	if ( m_pAudioDriver ) {	// check if the audio m_pAudioDriver is still alive
		ERRORLOG( "The audio driver is still alive" );
	}
	if ( m_pMidiDriver ) {	// check if midi driver is still alive
		ERRORLOG( "The MIDI driver is still active" );
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
				ERRORLOG( QString( "Couldn't start preferred driver [%1], falling back to [%2]" )
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
	if ( pSong != nullptr ) {
		setState( State::Ready );
	} else {
		setState( State::Prepared );
	}

	// Unlocking earlier might execute the jack process() callback before we
	// are fully initialized.
	mx.unlock();
	this->unlock();
	
	if ( m_pAudioDriver != nullptr ) {
		int res = m_pAudioDriver->connect();
		if ( res != 0 ) {
			raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			ERRORLOG( QString( "Unable to connect audio driver: %1 . Falling back to NullDriver." )
					  .arg( res ) );

			mx.relock();
			delete m_pAudioDriver;
			m_pAudioDriver = new NullDriver( m_AudioProcessCallback );
			mx.unlock();
			m_pAudioDriver->init( 0 );
			m_pAudioDriver->connect();
		}

		if ( pSong != nullptr && pHydrogen->haveJackAudioDriver() ) {
			pHydrogen->renameJackPorts( pSong );
		}
		
		setupLadspaFX();
	}

	handleDriverChange();
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
	setState( State::Initialized );

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
 * Restart all audio and midi drivers.
 */
void AudioEngine::restartAudioDrivers()
{
	if ( m_pAudioDriver != nullptr ) {
		stopAudioDrivers();
	}
	startAudioDrivers();
}

void AudioEngine::handleDriverChange() {

	if ( Hydrogen::get_instance()->getSong() == nullptr ) {
		WARNINGLOG( "no song set yet" );
		return;
	}
	
	handleTimelineChange();
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
	} else if ( pHydrogen->getSong()->getIsTimelineActivated() &&
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
	if ( getState() == State::Playing || getState() == State::Testing ) {
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
		long long nNoteStartInFrames = pNote->getNoteStart();

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

			// Check whether the instrument could be found.
			if ( nInstrument != -1 ) {
				m_pEventQueue->push_event( EVENT_NOTEON, nInstrument );
			}
			
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

		if ( dynamic_cast<DiskWriterDriver*>(pAudioEngine->m_pAudioDriver) != nullptr ) {
			return 2;	// inform the caller that we could not aquire the lock
		}

		return 0;
	}

	if ( ! ( pAudioEngine->getState() == AudioEngine::State::Ready ||
			 pAudioEngine->getState() == AudioEngine::State::Playing ) ) {
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
   
	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	int nResNoteQueue = pAudioEngine->updateNoteQueue( nframes );
	if ( nResNoteQueue == -1 ) {	// end of song
		___INFOLOG( "End of song received" );
		pAudioEngine->stop();
		pAudioEngine->stopPlayback();
		pAudioEngine->locate( 0 );

		if ( dynamic_cast<FakeDriver*>(pAudioEngine->m_pAudioDriver) != nullptr ) {
			___INFOLOG( "End of song." );
			
			return 1;	// kill the audio AudioDriver thread
		}
	}

	pAudioEngine->processAudio( nframes );

	// increment the transport position
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
		pAudioEngine->incrementTransportPosition( nframes );
	}

	timeval finishTimeval = currentTime2();
	pAudioEngine->m_fProcessTime =
			( finishTimeval.tv_sec - startTimeval.tv_sec ) * 1000.0
			+ ( finishTimeval.tv_usec - startTimeval.tv_usec ) / 1000.0;
	
#ifdef CONFIG_DEBUG
	if ( pAudioEngine->m_fProcessTime > pAudioEngine->m_fMaxProcessTime ) {
		___WARNINGLOG( "" );
		___WARNINGLOG( "----XRUN----" );
		___WARNINGLOG( QString( "XRUN of %1 msec (%2 > %3)" )
					   .arg( ( pAudioEngine->m_fProcessTime - pAudioEngine->m_fMaxProcessTime ) )
					   .arg( pAudioEngine->m_fProcessTime ).arg( pAudioEngine->m_fMaxProcessTime ) );
		___WARNINGLOG( QString( "Ladspa process time = %1" ).arg( fLadspaTime ) );
		___WARNINGLOG( "------------" );
		___WARNINGLOG( "" );
		// raise xRun event
		EventQueue::get_instance()->push_event( EVENT_XRUN, -1 );
	}
#endif

	pAudioEngine->unlock();

	return 0;
}

void AudioEngine::processAudio( uint32_t nFrames ) {

	auto pSong = Hydrogen::get_instance()->getSong();

	// play all notes
	processPlayNotes( nFrames );

	float *pBuffer_L = m_pAudioDriver->getOut_L(),
		*pBuffer_R = m_pAudioDriver->getOut_R();
	assert( pBuffer_L != nullptr && pBuffer_R != nullptr );

	// SAMPLER
	getSampler()->process( nFrames, pSong );
	float* out_L = getSampler()->m_pMainOut_L;
	float* out_R = getSampler()->m_pMainOut_R;
	for ( unsigned i = 0; i < nFrames; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

	// SYNTH
	getSynth()->process( nFrames );
	out_L = getSynth()->m_pOut_L;
	out_R = getSynth()->m_pOut_R;
	for ( unsigned i = 0; i < nFrames; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

	timeval ladspaTime_start = currentTime2();

#ifdef H2CORE_HAVE_LADSPA
	// Process LADSPA FX
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( ( pFX ) && ( pFX->isEnabled() ) ) {
			pFX->processFX( nFrames );

			float *buf_L, *buf_R;
			if ( pFX->getPluginType() == LadspaFX::STEREO_FX ) {
				buf_L = pFX->m_pBuffer_L;
				buf_R = pFX->m_pBuffer_R;
			} else { // MONO FX
				buf_L = pFX->m_pBuffer_L;
				buf_R = buf_L;
			}

			for ( unsigned i = 0; i < nFrames; ++i ) {
				pBuffer_L[ i ] += buf_L[ i ];
				pBuffer_R[ i ] += buf_R[ i ];
				if ( buf_L[ i ] > m_fFXPeak_L[nFX] ) {
					m_fFXPeak_L[nFX] = buf_L[ i ];
				}

				if ( buf_R[ i ] > m_fFXPeak_R[nFX] ) {
					m_fFXPeak_R[nFX] = buf_R[ i ];
				}
			}
		}
	}
#endif
	timeval ladspaTime_end = currentTime2();
	m_fLadspaTime =
			( ladspaTime_end.tv_sec - ladspaTime_start.tv_sec ) * 1000.0
			+ ( ladspaTime_end.tv_usec - ladspaTime_start.tv_usec ) / 1000.0;

	// update master peaks
	float val_L, val_R;
	for ( unsigned i = 0; i < nFrames; ++i ) {
		val_L = pBuffer_L[i];
		val_R = pBuffer_R[i];

		if ( val_L > m_fMasterPeak_L ) {
			m_fMasterPeak_L = val_L;
		}

		if ( val_R > m_fMasterPeak_R ) {
			m_fMasterPeak_R = val_R;
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

}

void AudioEngine::setState( AudioEngine::State state ) {
	m_state = state;
	EventQueue::get_instance()->push_event( EVENT_STATE, static_cast<int>(state) );
}

void AudioEngine::setSong( std::shared_ptr<Song> pNewSong )
{
	INFOLOG( QString( "Set song: %1" ).arg( pNewSong->getName() ) );
	
	this->lock( RIGHT_HERE );

	// check current state
	// should be set by removeSong called earlier
	if ( getState() != State::Prepared ) {
		ERRORLOG( QString( "Error the audio engine is not in State::Prepared but [%1]" )
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

	Hydrogen::get_instance()->renameJackPorts( pNewSong );
	m_fSongSizeInTicks = static_cast<double>( pNewSong->lengthInTicks() );

	// change the current audio engine state
	setState( State::Ready );

	setNextBpm( pNewSong->getBpm() );
	// Will adapt the audio engine to the song's BPM.
	locate( 0 );

	Hydrogen::get_instance()->setTimeline( pNewSong->getTimeline() );

	this->unlock();
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
		ERRORLOG( QString( "Error the audio engine is not in State::Ready but [%1]" )
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
}

void AudioEngine::updateSongSize() {
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	double fNewSongSizeInTicks = static_cast<double>( pSong->lengthInTicks() );

	// WARNINGLOG( QString( "[Before] frame: %1, bpm: %2, tickSize: %3, column: %4, tick: %5, mod(tick): %6, pTickPos: %7, pStartPos: %8, m_fLastTickIntervalEnd: %9, m_fSongSizeInTicks: %10" )
	// 			.arg( getFrames() ).arg( getBpm() )
	// 			.arg( getTickSize(), 0, 'f' )
	// 			.arg( m_nColumn )
	// 			.arg( getDoubleTick(), 0, 'g', 30 )
	// 			.arg( std::fmod( getDoubleTick(), m_fSongSizeInTicks ), 0, 'g', 30 )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick )
	// 			.arg( m_fLastTickIntervalEnd )
	// 			.arg( m_fSongSizeInTicks )
	// 			);

	// Strip away all repetitions when in loop mode but keep their
	// number. m_nPatternStartTick and m_nColumn are only defined
	// between 0 and m_fSongSizeInTicks.
	double fNewTick = std::fmod( getDoubleTick(), m_fSongSizeInTicks );
	double fRepetitions =
		std::floor( getDoubleTick() / m_fSongSizeInTicks );

	//
	m_fSongSizeInTicks = fNewSongSizeInTicks;

	// Expected behavior:
	// - changing any part of the song except of the pattern currently
	//   play shouldn't affect the transport position
	// - there shouldn't be a difference in behavior when the song was
	//   looped at least once
	// - this internal compensation in the transport position will
	//   only be propagated to external audio servers, like JACK, once
	//   a relocation takes place. This temporal loss of sync is done
	//   to avoid audible glitches when e.g. toggling a pattern or
	//   scrolling the pattern length spin boxes. A general intuition
	//   for a loss of synchronization when just changing song parts
	//   in one application can probably be expected.  
	// 
	// We strive for consistency in audio playback and make both the
	// current column/pattern and the transport position within the
	// pattern invariant in this transformation.
	long nNewPatternStartTick = pHydrogen->getTickForColumn( getColumn() );

	if ( nNewPatternStartTick == -1 ) {
		ERRORLOG( QString( "Something went wrong. No tick found for column [%1]" )
				  .arg( getColumn() ) );
	}
	
	if ( nNewPatternStartTick != m_nPatternStartTick ) {

		// DEBUGLOG( QString( "[start tick mismatch] old: %1, new: %2" )
		// 		  .arg( m_nPatternStartTick )
		// 		  .arg( nNewPatternStartTick ) );
		
		fNewTick +=
			static_cast<double>(nNewPatternStartTick - m_nPatternStartTick);
	}

	// Incorporate the looped transport again
	fNewTick += fRepetitions * fNewSongSizeInTicks;
	
	// Ensure transport state is consistent
	long long nNewFrames = computeFrameFromTick( fNewTick, &m_fTickMismatch );

	m_nFrameOffset = nNewFrames - getFrames() + m_nFrameOffset;
	m_fTickOffset = fNewTick - getDoubleTick();

	// Small rounding noise introduced in the calculation might spoil
	// things as we floor the resulting tick offset later on. Hence,
	// we round it to a specific precision.
	m_fTickOffset *= 1e8;
	m_fTickOffset = std::round( m_fTickOffset );
	m_fTickOffset *= 1e-8;
		
	// INFOLOG(QString( "[update] nNewFrame: %1, old frame: %2, frame offset: %3, new tick: %4, old tick: %5, tick offset : %6, tick offset (without rounding): %7, fNewSongSizeInTicks: %8, fRepetitions: %9")
	// 		 .arg( nNewFrames ).arg( getFrames() )
	// 		 .arg( m_nFrameOffset )
	// 		 .arg( fNewTick, 0, 'g', 30 )
	// 		 .arg( getDoubleTick(), 0, 'g', 30 )
	// 		 .arg( m_fTickOffset, 0, 'g', 30 )
	// 		.arg( fNewTick - getDoubleTick(), 0, 'g', 30 )
	// 		 .arg( fNewSongSizeInTicks, 0, 'g', 30 )
	// 		 .arg( fRepetitions, 0, 'g', 30 )
	// 		 );

	setFrames( nNewFrames );
	setTick( fNewTick );

	m_fLastTickIntervalEnd += m_fTickOffset;

	// Moves all notes currently processed by Hydrogen with respect to
	// the offsets calculated above.
	handleSongSizeChange();

	// After tick and frame information as well as notes are updated
	// we will make the remainder of the transport information
	// consistent.
	updateTransportPosition( getDoubleTick(),
							 pSong->isLoopEnabled() );

	if ( m_nColumn == -1 ) {
		stop();
	}

	// WARNINGLOG( QString( "[After] frame: %1, bpm: %2, tickSize: %3, column: %4, tick: %5, pTickPos: %6, pStartPos: %7, m_fLastTickIntervalEnd: %8" )
	// 			.arg( getFrames() ).arg( getBpm() )
	// 			.arg( getTickSize(), 0, 'f' )
	// 			.arg( m_nColumn ).arg( getDoubleTick(), 0, 'f' )
	// 			.arg( m_nPatternTickPosition )
	// 			.arg( m_nPatternStartTick )
	// 			.arg( m_fLastTickIntervalEnd ) );
	
}

void AudioEngine::flushPlayingPatterns() {
	m_pPlayingPatterns->clear();
}

void AudioEngine::updatePlayingPatterns( int nColumn, long nTick, long nPatternStartTick ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		m_pPlayingPatterns->clear();

		if ( nColumn < 0 || nColumn >= pSong->getPatternGroupVector()->size() ) {
			return;
		}

		for ( const auto& ppattern : *( *( pSong->getPatternGroupVector() ) )[ nColumn ] ) {
			m_pPlayingPatterns->add( ppattern );
			ppattern->addFlattenedVirtualPatterns( m_pPlayingPatterns );
		}
		EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, 0 );
		
	} else if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		if ( Preferences::get_instance()->patternModePlaysSelected() ) {
			auto pSelectedPattern =
				pSong->getPatternList()->get( pHydrogen->getSelectedPatternNumber() );
			if ( m_pPlayingPatterns->size() != 1 ||
				 ( m_pPlayingPatterns->size() == 1 &&
				   m_pPlayingPatterns->get( 0 ) != pSelectedPattern ) ) {

				m_pPlayingPatterns->clear();
				m_pPlayingPatterns->add( pSelectedPattern );
				pSelectedPattern->addFlattenedVirtualPatterns( m_pPlayingPatterns );
				
				EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, 0 );
			}
				
		} else {
			// Stacked pattern mode
			int nPatternSize;
			if ( m_pPlayingPatterns->size() != 0 ) {
				nPatternSize = m_pPlayingPatterns->longest_pattern_length();
			} else {
				nPatternSize = 0;
			}

			if ( nPatternSize == 0 ||
				 nTick >= nPatternStartTick + nPatternSize ) {

				if ( m_pNextPatterns->size() > 0 ) {
					for ( const auto& ppattern : *m_pNextPatterns ) {
						// If provided pattern is not part of the
						// list, a nullptr will be returned. Else, a
						// pointer to the deleted pattern will be
						// returned.
						if ( ( m_pPlayingPatterns->del( ppattern ) ) == nullptr ) {
							// pPattern was not present yet. It will
							// be added.
							m_pPlayingPatterns->add( ppattern );
							ppattern->addFlattenedVirtualPatterns( m_pPlayingPatterns );
						} else {
							// pPattern was already present. It will
							// be deleted.
							ppattern->removeFlattenedVirtualPatterns( m_pPlayingPatterns );
						}
						EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, 0 );
					}
					m_pNextPatterns->clear();
				}
			}
		} // Stacked mode
	} // Pattern mode
}

void AudioEngine::toggleNextPattern( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern != nullptr ) {
		if ( m_pNextPatterns->del( pPattern ) == nullptr ) {
			m_pNextPatterns->add( pPattern );
		}
	}
}

void AudioEngine::flushAndAddNextPattern( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPatternList = pSong->getPatternList();

	m_pNextPatterns->clear();
	
	for ( int ii = 0; ii < m_pPlayingPatterns->size(); ++ii ) {
		m_pNextPatterns->add( m_pPlayingPatterns->get( ii ) );
	}
	
	// Appending the requested pattern.
	// Note: we will not perform a bound check on the provided pattern
	// number. This way the user can use the SELECT_ONLY_NEXT_PATTERN
	// MIDI or OSC command to flush all playing patterns.
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern != nullptr ) {
		m_pNextPatterns->add( pPattern );
	}
}

void AudioEngine::handleTimelineChange() {

	setFrames( computeFrameFromTick( getDoubleTick(), &m_fTickMismatch ) );
	updateBpmAndTickSize();

	if ( ! Hydrogen::get_instance()->isTimelineEnabled() ) {
		// In case the Timeline was turned off, the
		// handleTempoChange() function will take over and update all
		// notes currently processed.
		return;
	}

	// Recalculate the note start in frames for all notes currently
	// processed by the AudioEngine.
	if ( m_songNoteQueue.size() > 0 ) {
		std::vector<Note*> notes;
		for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
			notes.push_back( m_songNoteQueue.top() );
		}

		for ( auto nnote : notes ) {
			nnote->computeNoteStart();
			m_songNoteQueue.push( nnote );
		}
	}
	
	getSampler()->handleTimelineOrTempoChange();
}

void AudioEngine::handleTempoChange() {
	if ( m_songNoteQueue.size() == 0 ) {
		return;
	}

	// All notes share the same ticksize state (or things have gone
	// wrong at some point).
	if ( m_songNoteQueue.top()->getUsedTickSize() !=
		 getTickSize() ) {

		std::vector<Note*> notes;
		for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
			notes.push_back( m_songNoteQueue.top() );
		}

		// All notes share the same ticksize state (or things have gone
		// wrong at some point).
		for ( auto nnote : notes ) {
			nnote->computeNoteStart();
			m_songNoteQueue.push( nnote );
		}
	
		getSampler()->handleTimelineOrTempoChange();
	}
}

void AudioEngine::handleSongSizeChange() {
	if ( m_songNoteQueue.size() == 0 ) {
		return;
	}

	std::vector<Note*> notes;
	for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
		notes.push_back( m_songNoteQueue.top() );
	}

	for ( auto nnote : notes ) {

		// DEBUGLOG( QString( "name: %1, pos: %2, new pos: %3, tick offset: %4, tick offset floored: %5" )
		// 		  .arg( nnote->get_instrument()->get_name() )
		// 		  .arg( nnote->get_position() )
		// 		  .arg( std::max( nnote->get_position() +
		// 							   static_cast<long>(std::floor(getTickOffset())),
		// 						  static_cast<long>(0) ) )
		// 		  .arg( getTickOffset() )
		// 		  .arg( std::floor(getTickOffset()) ) );
		
		nnote->set_position( std::max( nnote->get_position() +
									   static_cast<long>(std::floor(getTickOffset())),
									   static_cast<long>(0) ) );
		nnote->computeNoteStart();
		m_songNoteQueue.push( nnote );
	}
	
	getSampler()->handleSongSizeChange();
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

	// INFOLOG( QString( "nFrameStart: %1, nFrameEnd: %2, fTickStart: %3, fTickEnd: %4" )
	// 		 .arg( nFrameStart ).arg( nFrameEnd )
	// 		 .arg( *fTickStart, 0, 'f' ).arg( *fTickEnd, 0, 'f' ) );

	if ( getState() == State::Playing || getState() == State::Testing ) {
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
		pNote->computeNoteStart();
		m_songNoteQueue.push( pNote );
	}

	if ( getState() != State::Playing && getState() != State::Testing ) {
		// only keep going if we're playing
		return 0;
	}

	// Use local representations of the current transport position in
	// order for it to not get into a dirty state.
	int nColumn = m_nColumn;

	// Since we deal with a lookahead the pattern start and tick
	// position in the loop below are _not_ the same as the ones of
	// the audio engine itself. We set the pattern start tick to -1 to
	// signal the code below its state is dirty and needs to be
	// updated. This is more efficient than updating it in every iteration.
	long nPatternStartTick = -1;
	long nPatternTickPosition = -1;
	long long nNoteStart;
	float fUsedTickSize;

	double fTickMismatch;

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
			
			if ( nnTick >= std::floor( m_fSongSizeInTicks ) &&
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
				updatePlayingPatterns( nColumn );
				m_nLastPlayingPatternsColumn = nColumn;
			}
		}
		
		//////////////////////////////////////////////////////////////
		// PATTERN MODE
		else if ( pHydrogen->getMode() == Song::Mode::Pattern )	{

			int nPatternSize = -1;
			if ( nPatternStartTick == -1 ) {
				if ( m_pPlayingPatterns->size() != 0 ) {
					nPatternSize = m_pPlayingPatterns->longest_pattern_length();
				} else {
					nPatternSize = MAX_NOTES;
				}

				if ( nPatternSize > 0 ) {
					nPatternStartTick =
						std::floor( static_cast<float>(nnTick -
													   std::max( nPatternStartTick,
																 static_cast<long>(0) )) /
									static_cast<float>(nPatternSize) ) * nPatternSize;
				} else {
					nPatternStartTick = nnTick;
				}
			}

			updatePlayingPatterns( 0, nnTick, nPatternStartTick );

			if ( nPatternSize == -1 ||
				 nPatternSize != m_pPlayingPatterns->longest_pattern_length() ) {
			
				if ( m_pPlayingPatterns->size() != 0 ) {
					nPatternSize = m_pPlayingPatterns->longest_pattern_length();
				} else {
					nPatternSize = MAX_NOTES;
				}

				if ( nPatternSize == 0 ) {
					ERRORLOG( "nPatternSize == 0" );
				}
			
				if ( nPatternStartTick == -1 ||
					 nnTick >= nPatternStartTick + nPatternSize  ) {
					if ( nPatternSize > 0 ) {
						nPatternStartTick +=
							std::floor( static_cast<float>(nnTick -
														   std::max( nPatternStartTick,
																	 static_cast<long>(0) )) /
										static_cast<float>(nPatternSize) ) * nPatternSize;
					} else {
						nPatternStartTick = nnTick;
					}
				}
			}

			nPatternTickPosition = nnTick - nPatternStartTick;
			if ( nPatternSize > 0 && nPatternTickPosition > nPatternSize ) {
				nPatternTickPosition = ( nnTick - nPatternStartTick ) %
					nPatternSize;
			}

			// DEBUGLOG( QString( "[post] nnTick: %1, nPatternTickPosition: %2, nPatternStartTick: %3, nPatternSize: %4" )
			// 		  .arg( nnTick ).arg( nPatternTickPosition )
			// 		  .arg( nPatternStartTick ).arg( nPatternSize ) );
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
				pMetronomeNote->computeNoteStart();
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
								computeFrameFromTick( nnTick + MAX_NOTES / 32., &fTickMismatch ) *
								pSong->getSwingFactor() -
								computeFrameFromTick( nnTick, &fTickMismatch );
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
						//
						// Why a copy? because it has the new offset
						// (including swing and random timing) in its
						// humanized delay, and tick position is
						// expressed referring to start time (and not
						// pattern).
						Note *pCopiedNote = new Note( pNote );
						pCopiedNote->set_humanize_delay( nOffset );

						// DEBUGLOG( QString( "getDoubleTick(): %1, getFrames(): %2, getColumn(): %3, nnTick: %4, nColumn: %5, " )
						// 		  .arg( getDoubleTick() ).arg( getFrames() )
						// 		  .arg( getColumn() ).arg( nnTick )
						// 		  .arg( nColumn )
						// 		  .append( pCopiedNote->toQString("", true ) ) );
						
						pCopiedNote->set_position( nnTick );
						// Important: this call has to be done _after_
						// setting the position and the humanize_delay.
						pCopiedNote->computeNoteStart();
						
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

	return 0;
}

void AudioEngine::noteOn( Note *note )
{
	// check current state
	if ( ! ( getState() == State::Playing ||
			 getState() == State::Ready ||
			 getState() == State::Testing ) ) {
		ERRORLOG( QString( "Error the audio engine is not in State::Ready, State::Playing, or State::Testing but [%1]" )
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

	if ( dynamic_cast<FakeDriver*>(m_pAudioDriver) != nullptr ) {
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
	double fTickMismatch;
	long long nFrameStart = computeFrameFromTick( fTick, &fTickMismatch );
	long long nFrameEnd = computeFrameFromTick( fTick + AudioEngine::getLeadLagInTicks(),
						  &fTickMismatch );

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
	long long nFrame1Computed = computeFrameFromTick( fTick1, &fFrameOffset1 );
	double fTick2 = computeTickFromFrame( nFrame2 );
	long long nFrame2Computed = computeFrameFromTick( fTick2, &fFrameOffset2 );
	double fTick3 = computeTickFromFrame( nFrame3 );
	long long nFrame3Computed = computeFrameFromTick( fTick3, &fFrameOffset3 );
	
	if ( nFrame1Computed != nFrame1 || std::abs( fFrameOffset1 ) > 1e-10 ) {
		ERRORLOG( QString( "[1] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4, frame diff: %5" )
				  .arg( nFrame1 ).arg( fTick1, 0, 'f' ).arg( nFrame1Computed )
				  .arg( fFrameOffset1, 0, 'E', -1 )
				  .arg( nFrame1Computed - nFrame1 )
				  .toLocal8Bit().data() );
		bNoMismatch = false;
	}
	if ( nFrame2Computed != nFrame2 || std::abs( fFrameOffset2 ) > 1e-10 ) {
		ERRORLOG( QString( "[2] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4, frame diff: %5" )
				  .arg( nFrame2 ).arg( fTick2, 0, 'f' ).arg( nFrame2Computed )
				  .arg( fFrameOffset2, 0, 'E', -1 )
				  .arg( nFrame2Computed - nFrame2 ).toLocal8Bit().data() );
		bNoMismatch = false;
	}
	if ( nFrame3Computed != nFrame3 || std::abs( fFrameOffset3 ) > 1e-6 ) {
		ERRORLOG( QString( "[3] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameOffset: %4, frame diff: %5" )
				  .arg( nFrame3 ).arg( fTick3, 0, 'f' ).arg( nFrame3Computed )
				  .arg( fFrameOffset3, 0, 'E', -1 )
				  .arg( nFrame3Computed - nFrame3 ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	double fTick4 = 552;
	double fTick5 = 1939;
	double fTick6 = 534623409;
	long long nFrame4 = computeFrameFromTick( fTick4, &fFrameOffset4 );
	double fTick4Computed = computeTickFromFrame( nFrame4 ) +
		fFrameOffset4;
	long long nFrame5 = computeFrameFromTick( fTick5, &fFrameOffset5 );
	double fTick5Computed = computeTickFromFrame( nFrame5 ) +
		fFrameOffset5;
	long long nFrame6 = computeFrameFromTick( fTick6, &fFrameOffset6 );
	double fTick6Computed = computeTickFromFrame( nFrame6 ) +
		fFrameOffset6;
	
	
	if ( abs( fTick4Computed - fTick4 ) > 1e-9 ) {
		ERRORLOG( QString( "[4] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4, tick diff: %5" )
				  .arg( nFrame4 ).arg( fTick4, 0, 'f' ).arg( fTick4Computed, 0, 'f' )
				  .arg( fFrameOffset4, 0, 'E' )
				  .arg( fTick4Computed - fTick4 ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	if ( abs( fTick5Computed - fTick5 ) > 1e-9 ) {
		ERRORLOG( QString( "[5] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4, tick diff: %5" )
				  .arg( nFrame5 ).arg( fTick5, 0, 'f' ).arg( fTick5Computed, 0, 'f' )
				  .arg( fFrameOffset5, 0, 'E' )
				  .arg( fTick5Computed - fTick5 ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	if ( abs( fTick6Computed - fTick6 ) > 1e-6 ) {
		ERRORLOG( QString( "[6] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameOffset: %4, tick diff: %5" )
				  .arg( nFrame6 ).arg( fTick6, 0, 'f' ).arg( fTick6Computed, 0, 'f' )
				  .arg( fFrameOffset6, 0, 'E' )
				  .arg( fTick6Computed - fTick6 ).toLocal8Bit().data() );
		bNoMismatch = false;
	}

	return bNoMismatch;
}

bool AudioEngine::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( true, false );

	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Testing );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	double fCheckTick;
	long long nCheckFrame, nLastFrame = 0;

	bool bNoMismatch = true;

	// 2112 is the number of ticks within the test song.
	int nMaxCycles =
		std::max( std::ceil( 2112.0 /
							 static_cast<float>(pPref->m_nBufferSize) *
							getTickSize() * 4.0 ),
				  2112.0 );
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
			ERRORLOG( QString( "[constant tempo] end of the song wasn't reached in time. getFrames(): %1, ticks: %2, getTickSize(): %3, m_fSongSizeInTicks: %4, nMaxCycles: %5" )
					  .arg( getFrames() )
					  .arg( getDoubleTick(), 0, 'f' )
					  .arg( getTickSize(), 0, 'f' )
					  .arg( m_fSongSizeInTicks, 0, 'f' )
					  .arg( nMaxCycles ) );
			bNoMismatch = false;
			break;
		}
	}

	reset( false );
	nLastFrame = 0;

	float fBpm;
	float fLastBpm = getBpm();
	int nCyclesPerTempo = 5;
	int nPrevLastFrame = 0;

	long long nTotalFrames = 0;
	
	nn = 0;

	while ( getDoubleTick() < m_fSongSizeInTicks ) {

		fBpm = tempoDist( randomEngine );

		nPrevLastFrame = nLastFrame;
		nLastFrame =
			static_cast<int>(std::round( static_cast<double>(nLastFrame) *
										 static_cast<double>(fLastBpm) /
										 static_cast<double>(fBpm) ));
		
		setNextBpm( fBpm );
		updateBpmAndTickSize();
		
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
				 ( cc == 0 &&
				   abs( ( getFrames() - nFrames - nLastFrame ) /
						getFrames() ) > 1e-8 ) ) {
				ERRORLOG( QString( "[variable tempo] inconsistent frame update. getFrames(): %1, nFrames: %2, nLastFrame: %3, cc: %4, fLastBpm: %5, fBpm: %6, nPrevLastFrame: %7" )
						  .arg( getFrames() ).arg( nFrames )
						  .arg( nLastFrame ).arg( cc )
						  .arg( fLastBpm, 0, 'f' ).arg( fBpm, 0, 'f' )
						  .arg( nPrevLastFrame ) );
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
		
		fLastBpm = fBpm;

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
	setState( AudioEngine::State::Testing );

	// Check consistency after switching on the Timeline
	if ( ! testCheckTransportPosition( "timeline: off" ) ) {
		bNoMismatch = false;
	}
	
	nn = 0;
	nLastFrame = 0;

	while ( getDoubleTick() < m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		incrementTransportPosition( nFrames );

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
	setState( AudioEngine::State::Testing );

	if ( ! testCheckTransportPosition( "timeline: off" ) ) {
		bNoMismatch = false;
	}

	reset( false );

	setState( AudioEngine::State::Ready );

	unlock();

	// Check consistency of playback in PatternMode
	pCoreActionController->activateSongMode( false );

	lock( RIGHT_HERE );
	setState( AudioEngine::State::Testing );

	nLastFrame = 0;
	fLastBpm = 0;
	nTotalFrames = 0;

	int nDifferentTempos = 10;

	for ( int tt = 0; tt < nDifferentTempos; ++tt ) {

		fBpm = tempoDist( randomEngine );

		nLastFrame = std::round( nLastFrame * fLastBpm / fBpm );
		
		setNextBpm( fBpm );
		updateBpmAndTickSize();

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
	auto pPref = Preferences::get_instance();

	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 0, m_fSongSizeInTicks );
	std::uniform_int_distribution<long long> frameDist( 0, pPref->m_nBufferSize );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Testing );

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

		if ( ! testCheckTransportPosition( "mismatch tick-based" ) ) {
			bNoMismatch = false;
			break;
		}

		// Frame-based relocation
		nNewFrame = frameDist( randomEngine );
		locateToFrame( nNewFrame );

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
	auto pPref = Preferences::get_instance();

	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
	std::uniform_real_distribution<float> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Testing );

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
			 // Since we move a region on two mismatching grids (frame
			 // and tick), it's okay if the calculated is not
			 // perfectly constant. For certain tick ranges more
			 // frames are enclosed than for others (Moire effect). 
			 std::abs( nLastLeadLagFactor - nLeadLagFactor ) > 1 ) {
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

bool AudioEngine::testSongSizeChange() {
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pSong = pHydrogen->getSong();

	lock( RIGHT_HERE );
	reset( false );
	setState( AudioEngine::State::Testing );

	unlock();
	pCoreActionController->locateToColumn( 4 );
	lock( RIGHT_HERE );

	if ( ! testCheckConsistency( 1, 1, "prior" ) ) {
		setState( AudioEngine::State::Ready );
		unlock();
		return false;
	}
		
	// Toggle a grid cell after to the current transport position
	if ( ! testCheckConsistency( 6, 6, "after" ) ) {
		setState( AudioEngine::State::Ready );
		unlock();
		return false;
	}

	// Now we head to the "same" position inside the song but with the
	// transport being looped once.
	int nTestColumn = 4;
	long nNextTick = pHydrogen->getTickForColumn( nTestColumn );
	if ( nNextTick == -1 ) {
		ERRORLOG( QString( "Bad test design: there is no column [%1]" )
				  .arg( nTestColumn ) );
		setState( AudioEngine::State::Ready );
		unlock();
		return false;
	}

	nNextTick += pSong->lengthInTicks();
	
	unlock();
	pCoreActionController->activateLoopMode( true, false );
	pCoreActionController->locateToTick( nNextTick );
	lock( RIGHT_HERE );
	
	if ( ! testCheckConsistency( 1, 1, "looped:prior" ) ) {
		setState( AudioEngine::State::Ready );
		unlock();
		return false;
	}
		
	// Toggle a grid cell after to the current transport position
	if ( ! testCheckConsistency( 6, 6, "looped:after" ) ) {
		setState( AudioEngine::State::Ready );
		unlock();
		return false;
	}

	setState( AudioEngine::State::Ready );
	unlock();

	return true;
}

bool AudioEngine::testSongSizeChangeInLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pPref = Preferences::get_instance();
	
	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( true, false );

	lock( RIGHT_HERE );

	int nColumns = pHydrogen->getSong()->getPatternGroupVector()->size();

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_int_distribution<int> columnDist( nColumns, nColumns + 100 );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Testing );

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

		if ( ! testCheckTransportPosition( "third increment" ) ) {
			bNoMismatch = false;
			break;
		}
	}

	setState( AudioEngine::State::Ready );

	unlock();

	return bNoMismatch;
}

bool AudioEngine::testNoteEnqueuing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pPref = Preferences::get_instance();

	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( false, false );
	pCoreActionController->activateSongMode( true );
	lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( pPref->m_nBufferSize / 2,
												  pPref->m_nBufferSize );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Testing );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	double fCheckTick;
	long long nCheckFrame, nLastFrame = 0;

	bool bNoMismatch = true;

	// 2112 is the number of ticks within the test song.
	int nMaxCycles =
		std::max( std::ceil( 2112.0 /
							 static_cast<float>(pPref->m_nBufferSize) *
							getTickSize() * 4.0 ),
				  2112.0 ); 
	int nn = 0;

	// Don't check the sampler yet. There is still #1521 to fix.
	// Ensure the sampler is clean.
	// while ( getSampler()->isRenderingNotes() ) {
	// 	processAudio( pPref->m_nBufferSize );
	// 	incrementTransportPosition( pPref->m_nBufferSize );
	// 	++nn;
	// 	WARNINGLOG( QString( "nn: %1, note: %2" ).arg( nn )
	// 				.arg( getSampler()->getPlayingNotesQueue()[0]->toQString("", true ) ) );
	// 	if ( nn > 10 ) {
	// 		ERRORLOG("Sampler is in weird state");
	// 		return false;
	// 	}
	// }

	nn = 0;

	bool bEndOfSongReached = false;

	auto notesInSong = pSong->getAllNotes();

	std::vector<std::shared_ptr<Note>> notesInSongQueue;
	std::vector<std::shared_ptr<Note>> notesInSamplerQueue;

	while ( getDoubleTick() < m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		if ( ! bEndOfSongReached ) {
			if ( updateNoteQueue( nFrames ) == -1 ) {
				bEndOfSongReached = true;
			}
		}

		// Add freshly enqueued notes.
		testMergeQueues( &notesInSongQueue,
						 testCopySongNoteQueue() );

		processAudio( nFrames );

		// Don't check the sampler yet. There is still #1521 to fix.
		// testMergeQueues( &notesInSamplerQueue,
		// 				 getSampler()->getPlayingNotesQueue() );

		incrementTransportPosition( nFrames );

		++nn;
		if ( nn > nMaxCycles ) {
			ERRORLOG( QString( "end of the song wasn't reached in time. getFrames(): %1, ticks: %2, getTickSize(): %3, m_fSongSizeInTicks: %4, nMaxCycles: %5" )
					  .arg( getFrames() )
					  .arg( getDoubleTick(), 0, 'f' )
					  .arg( getTickSize(), 0, 'f' )
					  .arg( m_fSongSizeInTicks, 0, 'f' )
					  .arg( nMaxCycles ) );
			bNoMismatch = false;
			break;
		}
	}

	if ( notesInSongQueue.size() !=
		 notesInSong.size() ) {
		QString sMsg = QString( "[song mode] Mismatch between notes count in Song [%1] and NoteQueue [%2]. Song:\n" )
			.arg( notesInSong.size() ).arg( notesInSongQueue.size() );
		for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
			auto note = notesInSong[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "NoteQueue:\n" );
		for ( int ii = 0; ii < notesInSongQueue.size(); ++ii  ) {
			auto note = notesInSongQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		ERRORLOG( sMsg );
		bNoMismatch = false;
	}
	
	// Don't check the sampler yet. There is still #1521 to fix.
	// if ( notesInSamplerQueue.size() !=
	// 	 notesInSong.size() ) {
	// 	QString sMsg = QString( "[song mode] Mismatch between notes count in Song [%1] and Sampler [%2]. Song:\n" )
	// 		.arg( notesInSong.size() ).arg( notesInSamplerQueue.size() );
	// 	for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
	// 		auto note = notesInSong[ ii ];
	// 		sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
	// 					 .arg( ii )
	// 					 .arg( note->get_instrument()->get_name() )
	// 					 .arg( note->get_position() )
	// 					 .arg( note->getNoteStart() )
	// 					 .arg( note->get_velocity() ) );
	// 	}
	// 	sMsg.append( "SamplerQueue:\n" );
	// 	for ( int ii = 0; ii < notesInSamplerQueue.size(); ++ii  ) {
	// 		auto note = notesInSamplerQueue[ ii ];
	// 		sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
	// 					 .arg( ii )
	// 					 .arg( note->get_instrument()->get_name() )
	// 					 .arg( note->get_position() )
	// 					 .arg( note->getNoteStart() )
	// 					 .arg( note->get_velocity() ) );
	// 	}

	// 	ERRORLOG( sMsg );
	// 	bNoMismatch = false;
	// }

	setState( AudioEngine::State::Ready );

	unlock();

	if ( ! bNoMismatch ) {
		return bNoMismatch;
	}

	//////////////////////////////////////////////////////////////////
	// Perform the test in pattern mode
	//////////////////////////////////////////////////////////////////
	
	pCoreActionController->activateSongMode( false );
	pPref->setPatternModePlaysSelected( true );
	pHydrogen->setSelectedPatternNumber( 4 );

	lock( RIGHT_HERE );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	reset( false );

	setState( AudioEngine::State::Testing );

	int nLoops = 5;
	
	nMaxCycles = MAX_NOTES * 2 * nLoops;
	nn = 0;

	// Don't check the sampler yet. There is still #1521 to fix.
	// Ensure the sampler is clean.
	// while ( getSampler()->isRenderingNotes() ) {
	// 	processAudio( pPref->m_nBufferSize );
	// 	incrementTransportPosition( pPref->m_nBufferSize );
	// 	++nn;
	// 	WARNINGLOG( QString( "nn: %1, note: %2" ).arg( nn )
	// 				.arg( getSampler()->getPlayingNotesQueue()[0]->toQString("", true ) ) );
	// 	if ( nn > 10 ) {
	// 		ERRORLOG("Sampler is in weird state");
	// 		return false;
	// 	}
	// }

	auto pPattern = 
		pSong->getPatternList()->get( pHydrogen->getSelectedPatternNumber() );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "null pattern selected [%1]" )
				  .arg( pHydrogen->getSelectedPatternNumber() ) );
		return false;
	}

	std::vector<std::shared_ptr<Note>> notesInPattern;
	for ( int ii = 0; ii < nLoops; ++ii ) {
		FOREACH_NOTE_CST_IT_BEGIN_END( pPattern->get_notes(), it ) {
			if ( it->second != nullptr ) {
				auto note = std::make_shared<Note>( it->second );
				note->set_position( note->get_position() +
									ii * pPattern->get_length() );
				notesInPattern.push_back( note );
			}
		}
	}

	notesInSongQueue.clear();
	notesInSamplerQueue.clear();

	nMaxCycles =
		static_cast<int>(std::max( static_cast<float>(pPattern->get_length()) *
								   static_cast<float>(nLoops) *
								   getTickSize() * 4 /
								   static_cast<float>(pPref->m_nBufferSize),
								   static_cast<float>(MAX_NOTES) *
								   static_cast<float>(nLoops) ));
	nn = 0;

	while ( getDoubleTick() < pPattern->get_length() * nLoops ) {

		nFrames = frameDist( randomEngine );

		updateNoteQueue( nFrames );

		// Add freshly enqueued notes.
		testMergeQueues( &notesInSongQueue,
						 testCopySongNoteQueue() );

		processAudio( nFrames );

		// Don't check the sampler yet. There is still #1521 to fix.
		// testMergeQueues( &notesInSamplerQueue,
		// 				 getSampler()->getPlayingNotesQueue() );

		incrementTransportPosition( nFrames );

		++nn;
		if ( nn > nMaxCycles ) {
			ERRORLOG( QString( "end of the pattern wasn't reached in time. getFrames(): %1, ticks: %2, getTickSize(): %3, pattern length: %4, nMaxCycles: %5, nLoops: %6" )
					  .arg( getFrames() )
					  .arg( getDoubleTick(), 0, 'f' )
					  .arg( getTickSize(), 0, 'f' )
					  .arg( pPattern->get_length() )
					  .arg( nMaxCycles )
					  .arg( nLoops ));
			bNoMismatch = false;
			break;
		}
	}

	// Transport in pattern mode is always looped. We have to pop the
	// notes added during the second run due to the lookahead.
	int nNoteNumber = notesInSongQueue.size();
	for( int ii = 0; ii < nNoteNumber; ++ii ) {
		auto note = notesInSongQueue[ nNoteNumber - 1 - ii ];
		if ( note != nullptr &&
			 note->get_position() >= pPattern->get_length() * nLoops ) {
			notesInSongQueue.pop_back();
		}
	}

	// nNoteNumber = notesInSamplerQueue.size();
	// for( int ii = 0; ii < nNoteNumber; ++ii ) {
	// 	auto note = notesInSamplerQueue[ nNoteNumber - 1 - ii ];
	// 	if ( note != nullptr &&
	// 		 note->get_position() >= pPattern->get_length() * nLoops ) {
	// 		notesInSongQueue.pop_back();
	// 	}
	// }

	if ( notesInSongQueue.size() !=
		 notesInPattern.size() ) {
		QString sMsg = QString( "[pattern mode] Mismatch between notes count in Song [%1] and NoteQueue [%2]. Song:\n" )
			.arg( notesInPattern.size() ).arg( notesInSongQueue.size() );
		for ( int ii = 0; ii < notesInPattern.size(); ++ii  ) {
			auto note = notesInPattern[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "NoteQueue:\n" );
		for ( int ii = 0; ii < notesInSongQueue.size(); ++ii  ) {
			auto note = notesInSongQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		ERRORLOG( sMsg );
		bNoMismatch = false;
	}

	// Don't check the sampler yet. There is still #1521 to fix.
	// if ( notesInSamplerQueue.size() !=
	// 	 notesInPattern.size() ) {
	// 	QString sMsg = QString( "[pattern mode] Mismatch between notes count in Song [%1] and Sampler [%2]. Song:\n" )
	// 		.arg( notesInPattern.size() ).arg( notesInSamplerQueue.size() );
	// 	for ( int ii = 0; ii < notesInPattern.size(); ++ii  ) {
	// 		auto note = notesInPattern[ ii ];
	// 		sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
	// 					 .arg( ii )
	// 					 .arg( note->get_instrument()->get_name() )
	// 					 .arg( note->get_position() )
	// 					 .arg( note->getNoteStart() )
	// 					 .arg( note->get_velocity() ) );
	// 	}
	// 	sMsg.append( "SamplerQueue:\n" );
	// 	for ( int ii = 0; ii < notesInSamplerQueue.size(); ++ii  ) {
	// 		auto note = notesInSamplerQueue[ ii ];
	// 		sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
	// 					 .arg( ii )
	// 					 .arg( note->get_instrument()->get_name() )
	// 					 .arg( note->get_position() )
	// 					 .arg( note->getNoteStart() )
	// 					 .arg( note->get_velocity() ) );
	// 	}

	// 	ERRORLOG( sMsg );
	// 	bNoMismatch = false;
	// }

	setState( AudioEngine::State::Ready );

	unlock();
	
	pCoreActionController->activateSongMode( true );

	return bNoMismatch;
}

void AudioEngine::testMergeQueues( std::vector<std::shared_ptr<Note>>* noteList, std::vector<std::shared_ptr<Note>> newNotes ) {
	bool bNoteFound;
	for ( const auto& newNote : newNotes ) {
		bNoteFound = false;
		// Check whether the notes is already present.
		for ( const auto& presentNote : *noteList ) {
			if ( newNote != nullptr && presentNote != nullptr ) {
				if ( newNote->match( presentNote.get() ) &&
					 newNote->get_position() ==
					 presentNote->get_position() &&
					 newNote->get_velocity() ==
					 presentNote->get_velocity() ) {
					bNoteFound = true;
				}
			}
		}

		if ( ! bNoteFound ) {
			noteList->push_back( std::make_shared<Note>(newNote.get()) );
		}
	}
}

// Used for the Sampler note queue
void AudioEngine::testMergeQueues( std::vector<std::shared_ptr<Note>>* noteList, std::vector<Note*> newNotes ) {
	bool bNoteFound;
	for ( const auto& newNote : newNotes ) {
		bNoteFound = false;
		// Check whether the notes is already present.
		for ( const auto& presentNote : *noteList ) {
			if ( newNote != nullptr && presentNote != nullptr ) {
				if ( newNote->match( presentNote.get() ) &&
					 newNote->get_position() ==
					 presentNote->get_position() &&
					 newNote->get_velocity() ==
					 presentNote->get_velocity() ) {
					bNoteFound = true;
				}
			}
		}

		if ( ! bNoteFound ) {
			noteList->push_back( std::make_shared<Note>(newNote) );
		}
	}
}

bool AudioEngine::testCheckTransportPosition( const QString& sContext) const {

	double fTickMismatch;
	long long nCheckFrame = computeFrameFromTick( getDoubleTick(), &fTickMismatch );
	double fCheckTick = computeTickFromFrame( getFrames() );// + fTickMismatch );
	
	if ( abs( fCheckTick + fTickMismatch - getDoubleTick() ) > 1e-9 ||
		 abs( fTickMismatch - m_fTickMismatch ) > 1e-9 ||
		 nCheckFrame != getFrames() ) {
		ERRORLOG( QString( "[%9] mismatch. frame: %1, check frame: %2, tick: %3, check tick: %4, offset: %5, check offset: %6, tick size: %7, bpm: %8, fCheckTick + fTickMismatch - getDoubleTick(): %10, fTickMismatch - m_fTickMismatch: %11, nCheckFrame - getFrames(): %12" )
				  .arg( getFrames() )
				  .arg( nCheckFrame )
				  .arg( getDoubleTick(), 0 , 'f', 9 )
				  .arg( fCheckTick, 0 , 'f', 9 )
				  .arg( m_fTickMismatch, 0 , 'f', 9 )
				  .arg( fTickMismatch, 0 , 'f', 9 )
				  .arg( getTickSize(), 0 , 'f' )
				  .arg( getBpm(), 0 , 'f' )
				  .arg( sContext )
				  .arg( fCheckTick + fTickMismatch - getDoubleTick(), 0, 'E' )
				  .arg( fTickMismatch - m_fTickMismatch, 0, 'E' )
				  .arg( nCheckFrame - getFrames() )
				  );
		return false;
	}

	return true;
}

bool AudioEngine::testCheckAudioConsistency( const std::vector<std::shared_ptr<Note>> oldNotes,
											 const std::vector<std::shared_ptr<Note>> newNotes, 
											 const QString& sContext,
											 int nPassedFrames, bool bTestAudio,
											 float fPassedTicks ) const {

	bool bNoMismatch = true;
	double fPassedFrames = static_cast<double>(nPassedFrames);
	auto pSong = Hydrogen::get_instance()->getSong();
	
	int nNotesFound = 0;
	for ( const auto& ppNewNote : newNotes ) {
		for ( const auto& ppOldNote : oldNotes ) {
			if ( ppNewNote->match( ppOldNote.get() ) &&
				 ppNewNote->get_humanize_delay() ==
				 ppOldNote->get_humanize_delay() &&
				 ppNewNote->get_velocity() ==
				 ppOldNote->get_velocity() ) {
				++nNotesFound;

				if ( bTestAudio ) {
					// Check for consistency in the Sample position
					// advanced by the Sampler upon rendering.
					for ( int nn = 0; nn < ppNewNote->get_instrument()->get_components()->size(); nn++ ) {
						auto pSelectedLayer = ppOldNote->get_layer_selected( nn );
						
						// The frames passed during the audio
						// processing depends on the sample rate of
						// the driver and sample and has to be
						// adjusted in here. This is equivalent to the
						// question whether Sampler::renderNote() or
						// Sampler::renderNoteResample() was used.
						if ( ppOldNote->getSample( nn )->get_sample_rate() !=
							 Hydrogen::get_instance()->getAudioOutput()->getSampleRate() ||
							 ppOldNote->get_total_pitch() != 0.0 ) {
							// In here we assume the layer pitcyh is zero.
							fPassedFrames = static_cast<double>(nPassedFrames) *
								Note::pitchToFrequency( ppOldNote->get_total_pitch() ) *
								static_cast<float>(ppOldNote->getSample( nn )->get_sample_rate()) /
								static_cast<float>(Hydrogen::get_instance()->getAudioOutput()->getSampleRate());
						}
						
						int nSampleFrames = ( ppNewNote->get_instrument()->get_component( nn )
											  ->get_layer( pSelectedLayer->SelectedLayer )->get_sample()->get_frames() );
						double fExpectedFrames =
							std::min( static_cast<double>(pSelectedLayer->SamplePosition) +
									  fPassedFrames,
									  static_cast<double>(nSampleFrames) );
						if ( std::abs( ppNewNote->get_layer_selected( nn )->SamplePosition -
									   fExpectedFrames ) > 1 ) {
							ERRORLOG( QString( "[%4] glitch in audio render. Diff: %9\nPre: %1\nPost: %2\nwith passed frames: %3, nSampleFrames: %5, fExpectedFrames: %6, sample sampleRate: %7, driver sampleRate: %8\n" )
									  .arg( ppOldNote->toQString( "", true ) )
									  .arg( ppNewNote->toQString( "", true ) )
									  .arg( fPassedFrames, 0, 'f' )
									  .arg( sContext )
									  .arg( nSampleFrames )
									  .arg( fExpectedFrames, 0, 'f' )
									  .arg( ppOldNote->getSample( nn )->get_sample_rate() )
									  .arg( Hydrogen::get_instance()->getAudioOutput()->getSampleRate() )
									  .arg( ppNewNote->get_layer_selected( nn )->SamplePosition -
											fExpectedFrames, 0, 'g', 30 )
									  );
							bNoMismatch = false;
						}
					}
				} else {
					// Check whether changes in note start position
					// were properly applied.
					if ( ppNewNote->get_position() - fPassedTicks !=
						 ppOldNote->get_position() ) {
							ERRORLOG( QString( "[%4] glitch in note queue.\n\nPre: %1 ;\n\nPost: %2 ; with passed ticks: %3\n" )
									  .arg( ppOldNote->toQString( "", true ) )
									  .arg( ppNewNote->toQString( "", true ) )
									  .arg( fPassedTicks )
									  .arg( sContext ) );
							bNoMismatch = false;
					}
				}
			}
		}
	}

	if ( nNotesFound == 0 ) {
		ERRORLOG( QString( "[%1] bad test design. No notes played back." )
				  .arg( sContext ) );
		if ( oldNotes.size() != 0 ) {
			ERRORLOG( "old notes:" );
			for ( auto const& nnote : oldNotes ) {
				ERRORLOG( nnote->toQString( "    ", true ) );
			}
		}
		if ( newNotes.size() != 0 ) {
			ERRORLOG( "new notes:" );
			for ( auto const& nnote : newNotes ) {
				ERRORLOG( nnote->toQString( "    ", true ) );
			}
		}
		ERRORLOG( QString( "curr tick: %1, curr frame: %2, nPassedFrames: %3, fPassedTicks: %4, fTickSize: %5" )
				  .arg( getDoubleTick(), 0, 'f' )
				  .arg( getFrames() )
				  .arg( nPassedFrames )
				  .arg( fPassedTicks, 0, 'f' )
				  .arg( getTickSize(), 0, 'f' ) );
		ERRORLOG( "notes in song:" );
		for ( auto const& nnote : pSong->getAllNotes() ) {
			ERRORLOG( nnote->toQString( "    ", true ) );
		}
		
		bNoMismatch = false;
	}

	return bNoMismatch;
}

std::vector<std::shared_ptr<Note>> AudioEngine::testCopySongNoteQueue() {
	std::vector<Note*> rawNotes;
	std::vector<std::shared_ptr<Note>> notes;
	for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
		rawNotes.push_back( m_songNoteQueue.top() );
		notes.push_back( std::make_shared<Note>( m_songNoteQueue.top() ) );
	}

	for ( auto nnote : rawNotes ) {
		m_songNoteQueue.push( nnote );
	}

	return notes;
}

bool AudioEngine::testCheckConsistency( int nToggleColumn, int nToggleRow, const QString& sContext ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pSong = pHydrogen->getSong();
	
	unsigned long nBufferSize = pHydrogen->getAudioOutput()->getBufferSize();
										  
	
	updateNoteQueue( nBufferSize );

	auto prevNotes = testCopySongNoteQueue();

	processAudio( nBufferSize );

	// Cache some stuff in order to compare it later on.
	long nOldSongSize = pSong->lengthInTicks();
	float fPrevTempo = getBpm();
	float fPrevTickSize = getTickSize();
	double fPrevTickStart, fPrevTickEnd;
	long long nPrevLeadLag;

	// We need to reset this variable in order for
	// computeTickInterval() to behave like just after a relocation.
	m_fLastTickIntervalEnd = -1;
	nPrevLeadLag = computeTickInterval( &fPrevTickStart, &fPrevTickEnd, nBufferSize );

	// TODO: #1521 needs to be fixed first.
	// std::vector<std::shared_ptr<Note>> notes1, notes2;
	// for ( const auto& ppNote : getSampler()->getPlayingNotesQueue() ) {
	// 	notes1.push_back( std::make_shared<Note>( ppNote ) );
	// }

	//////
	// Toggle a grid cell prior to the current transport position
	//////
	
	unlock();
	pCoreActionController->toggleGridCell( nToggleColumn, nToggleRow );
	lock( RIGHT_HERE );

	QString sFirstContext = QString( "%1 : 1. toggling" ).arg( sContext );

	// Check whether there is a change in song size
	long nNewSongSize = pSong->lengthInTicks();
	if ( nNewSongSize == nOldSongSize ) {
		ERRORLOG( QString( "[%1] no change in song size" )
				  .arg( sFirstContext ) );
		return false;
	}

	// Check whether current frame and tick information are still
	// consistent.
	if ( ! testCheckTransportPosition( sFirstContext ) ) {
		return false;
	}

	// m_songNoteQueue have been updated properly.
	auto afterNotes = testCopySongNoteQueue();

	if ( ! testCheckAudioConsistency( prevNotes, afterNotes, sFirstContext,
									  0, false, m_fTickOffset ) ) {
		return false;
	}
	double fTickStart, fTickEnd;
	long long nLeadLag;

	// We need to reset this variable in order for
	// computeTickInterval() to behave like just after a relocation.
	m_fLastTickIntervalEnd = -1;
	nLeadLag = computeTickInterval( &fTickStart, &fTickEnd, nBufferSize );
	if ( std::abs( nLeadLag - nPrevLeadLag ) > 1 ) {
		ERRORLOG( QString( "[%3] LeadLag should be constant since there should be change in tick size. old: %1, new: %2" )
				  .arg( nPrevLeadLag ).arg( nLeadLag ).arg( sFirstContext ) );
		return false;
	}
	if ( std::abs( fTickStart - m_fTickOffset - fPrevTickStart ) > 4e-3 ) {
		ERRORLOG( QString( "[%4] Mismatch in the start of the tick interval handled by updateNoteQueue new: %1, old: %2, old+offset: %3" )
				  .arg( fTickStart, 0, 'f' ).arg( fPrevTickStart, 0, 'f' )
				  .arg( fPrevTickStart + m_fTickOffset, 0, 'f' )
				  .arg( sFirstContext ) );
		return false;
	}
	if ( std::abs( fTickEnd - m_fTickOffset - fPrevTickEnd ) > 4e-3 ) {
		ERRORLOG( QString( "[%4] Mismatch in the end of the tick interval handled by updateNoteQueue new: %1, old: %2, old+offset: %3" )
				  .arg( fTickEnd, 0, 'f' ).arg( fPrevTickEnd, 0, 'f' )
				  .arg( fPrevTickEnd + m_fTickOffset, 0, 'f' )
				  .arg( sFirstContext ) );
		return false;
	}

	// Now we emulate that playback continues without any new notes
	// being added and expect the rendering of the notes currently
	// played back by the Sampler to start off precisely where we
	// stopped before the song size change. New notes might still be
	// added due to the lookahead, so, we just check for the
	// processing of notes we already encountered.
	incrementTransportPosition( nBufferSize );
	processAudio( nBufferSize );
	incrementTransportPosition( nBufferSize );
	processAudio( nBufferSize );

	// Check whether tempo and tick size have not changed.
	if ( fPrevTempo != getBpm() || fPrevTickSize != getTickSize() ) {
		ERRORLOG( QString( "[%1] tempo and ticksize are affected" )
				  .arg( sFirstContext ) );
		return false;
	}

	// TODO: #1521 needs to be fixed first. 
	// for ( const auto& ppNote : getSampler()->getPlayingNotesQueue() ) {
	// 	notes2.push_back( std::make_shared<Note>( ppNote ) );
	// }

	// if ( ! testCheckAudioConsistency( notes1, notes2, sFirstContext,
	// 								  nBufferSize * 2 ) ) {
	// 	return false;
	// }

	//////
	// Toggle the same grid cell again
	//////

	QString sSecondContext = QString( "%1 : 2. toggling" ).arg( sContext );
	
	// TODO: #1521 needs to be fixed first.
	// notes1.clear();
	// for ( const auto& ppNote : getSampler()->getPlayingNotesQueue() ) {
	// 	notes1.push_back( std::make_shared<Note>( ppNote ) );
	// }

	// We deal with a slightly artificial situation regarding
	// m_fLastTickIntervalEnd in here. Usually, in addition to
	// incrementTransportPosition() and	processAudio()
	// updateNoteQueue() would have been called too. This would update
	// m_fLastTickIntervalEnd which is not done in here. This way we
	// emulate a situation that occurs when encountering a change in
	// ticksize (passing a tempo marker or a user interaction with the
	// BPM widget) just before the song size changed.
	double fPrevLastTickIntervalEnd = m_fLastTickIntervalEnd;
	nPrevLeadLag = computeTickInterval( &fPrevTickStart, &fPrevTickEnd, nBufferSize );
	m_fLastTickIntervalEnd = fPrevLastTickIntervalEnd;
	
	unlock();
	pCoreActionController->toggleGridCell( nToggleColumn, nToggleRow );
	lock( RIGHT_HERE );

	// Check whether there is a change in song size
	nOldSongSize = nNewSongSize;
	nNewSongSize = pSong->lengthInTicks();
	if ( nNewSongSize == nOldSongSize ) {
		ERRORLOG( QString( "[%1] no change in song size" )
				  .arg( sSecondContext ) );
		return false;
	}

	// Check whether current frame and tick information are still
	// consistent.
	if ( ! testCheckTransportPosition( sSecondContext ) ) {
		return false;
	}

	// Check whether the notes already enqueued into the
	// m_songNoteQueue have been updated properly.
	prevNotes.clear();
	prevNotes = testCopySongNoteQueue();
	if ( ! testCheckAudioConsistency( afterNotes, prevNotes, sSecondContext,
									  0, false, m_fTickOffset ) ) {
		return false;
	}

	nLeadLag = computeTickInterval( &fTickStart, &fTickEnd, nBufferSize );
	if ( std::abs( nLeadLag - nPrevLeadLag ) > 1 ) {
		ERRORLOG( QString( "[%3] LeadLag should be constant since there should be change in tick size. old: %1, new: %2" )
				  .arg( nPrevLeadLag ).arg( nLeadLag ).arg( sSecondContext ) );
		return false;
	}
	if ( std::abs( fTickStart - m_fTickOffset - fPrevTickStart ) > 4e-3 ) {
		ERRORLOG( QString( "[%4] Mismatch in the start of the tick interval handled by updateNoteQueue new: %1, old: %2, old+offset: %3" )
				  .arg( fTickStart, 0, 'f' ).arg( fPrevTickStart, 0, 'f' )
				  .arg( fPrevTickStart + m_fTickOffset, 0, 'f' )
				  .arg( sSecondContext ) );
		return false;
	}
	if ( std::abs( fTickEnd - m_fTickOffset - fPrevTickEnd ) > 4e-3 ) {
		ERRORLOG( QString( "[%4] Mismatch in the end of the tick interval handled by updateNoteQueue new: %1, old: %2, old+offset: %3" )
				  .arg( fTickEnd, 0, 'f' ).arg( fPrevTickEnd, 0, 'f' )
				  .arg( fPrevTickEnd + m_fTickOffset, 0, 'f' )
				  .arg( sSecondContext )  );
		return false;
	}

	// Now we emulate that playback continues without any new notes
	// being added and expect the rendering of the notes currently
	// played back by the Sampler to start off precisely where we
	// stopped before the song size change. New notes might still be
	// added due to the lookahead, so, we just check for the
	// processing of notes we already encountered.
	incrementTransportPosition( nBufferSize );
	processAudio( nBufferSize );
	incrementTransportPosition( nBufferSize );
	processAudio( nBufferSize );

	// Check whether tempo and tick size have not changed.
	if ( fPrevTempo != getBpm() || fPrevTickSize != getTickSize() ) {
		ERRORLOG( QString( "[%1] tempo and ticksize are affected" )
				  .arg( sSecondContext ) );
		return false;
	}

	// TODO: #1521 needs to be fixed first.
	// notes2.clear();
	// for ( const auto& ppNote : getSampler()->getPlayingNotesQueue() ) {
	// 	notes2.push_back( std::make_shared<Note>( ppNote ) );
	// }

	// if ( ! testCheckAudioConsistency( notes1, notes2, sSecondContext,
	// 								  nBufferSize * 2 ) ) {
	// 	return false;
	// }

	return true;
}

QString AudioEngine::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[AudioEngine]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nFrames: %3\n" ).arg( sPrefix ).arg( s ).arg( getFrames() ) )
			.append( QString( "%1%2m_fTick: %3\n" ).arg( sPrefix ).arg( s ).arg( getDoubleTick(), 0, 'f' ) )
			.append( QString( "%1%2m_nFrameOffset: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nFrameOffset ) )
			.append( QString( "%1%2m_fTickOffset: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fTickOffset, 0, 'f' ) )
			.append( QString( "%1%2m_fTickSize: %3\n" ).arg( sPrefix ).arg( s ).arg( getTickSize(), 0, 'f' ) )
			.append( QString( "%1%2m_fBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( getBpm(), 0, 'f' ) )
			.append( QString( "%1%2m_fNextBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fNextBpm, 0, 'f' ) )
			.append( QString( "%1%2m_state: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_state) ) )
			.append( QString( "%1%2m_nextState: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_nextState) ) )
			.append( QString( "%1%2m_currentTickTime: %3 ms\n" ).arg( sPrefix ).arg( s ).arg( m_currentTickTime.tv_sec * 1000 + m_currentTickTime.tv_usec / 1000) )
			.append( QString( "%1%2m_nPatternStartTick: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPatternStartTick ) )
			.append( QString( "%1%2m_nPatternTickPosition: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPatternTickPosition ) )
			.append( QString( "%1%2m_nColumn: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nColumn ) )
			.append( QString( "%1%2m_fSongSizeInTicks: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fSongSizeInTicks, 0, 'f' ) )
			.append( QString( "%1%2m_fTickMismatch: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fTickMismatch, 0, 'f' ) )
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
			.append( QString( ", m_nFrameOffset: %1" ).arg( m_nFrameOffset ) )
			.append( QString( ", m_fTickOffset: %1" ).arg( m_fTickOffset, 0, 'f' ) )
			.append( QString( ", m_fTickSize: %1" ).arg( getTickSize(), 0, 'f' ) )
			.append( QString( ", m_fBpm: %1" ).arg( getBpm(), 0, 'f' ) )
			.append( QString( ", m_fNextBpm: %1" ).arg( m_fNextBpm, 0, 'f' ) )
			.append( QString( ", m_state: %1" ).arg( static_cast<int>(m_state) ) )
			.append( QString( ", m_nextState: %1" ).arg( static_cast<int>(m_nextState) ) )
			.append( QString( ", m_currentTickTime: %1 ms" ).arg( m_currentTickTime.tv_sec * 1000 + m_currentTickTime.tv_usec / 1000) )
			.append( QString( ", m_nPatternStartTick: %1" ).arg( m_nPatternStartTick ) )
			.append( QString( ", m_nPatternTickPosition: %1" ).arg( m_nPatternTickPosition ) )
			.append( QString( ", m_nColumn: %1" ).arg( m_nColumn ) )
			.append( QString( ", m_fSongSizeInTicks: %1" ).arg( m_fSongSizeInTicks, 0, 'f' ) )
			.append( QString( ", m_fTickMismatch: %1" ).arg( m_fTickMismatch, 0, 'f' ) )
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
