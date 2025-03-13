/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <limits>
#include <sstream>

#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Random.h>
#include <core/Hydrogen.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/AudioOutput.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/CoreMidiDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/FakeDriver.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/JackMidiDriver.h>
#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>
#include <core/IO/NullDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/PortMidiDriver.h>
#include <core/IO/PulseAudioDriver.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>

#define AUDIO_ENGINE_DEBUG 0

namespace H2Core
{

#define AE_INFOLOG(x) INFOLOG( QString( "[%1] %2" ) \
	.arg( Hydrogen::get_instance()->getAudioEngine()->getDriverNames() ).arg( x ) );
#define AE_WARNINGLOG(x) WARNINGLOG( QString( "[%1] %2" ) \
	.arg( Hydrogen::get_instance()->getAudioEngine()->getDriverNames() ).arg( x ) );
#define AE_ERRORLOG(x) ERRORLOG( QString( "[%1] %2" ) \
	.arg( Hydrogen::get_instance()->getAudioEngine()->getDriverNames() ).arg( x ) );
#define AE_DEBUGLOG(x) if ( __logger->should_log( Logger::Debug ) ) { \
		__logger->log( Logger::Debug, _class_name(), __FUNCTION__, \
					   QString( "%1" ).arg( x ), "\033[34;1m" ); }

/** Gets the current time.
 * \return Current time obtained by gettimeofday()*/
inline timeval currentTime2()
{
	struct timeval now;
	gettimeofday( &now, nullptr );
	return now;
}

AudioEngine::AudioEngine()
		: m_pSampler( nullptr )
		, m_pAudioDriver( nullptr )
		, m_pMidiDriver( nullptr )
		, m_pMidiDriverOut( nullptr )
		, m_state( State::Initialized )
		, m_pMetronomeInstrument( nullptr )
		, m_fSongSizeInTicks( 4 * H2Core::nTicksPerQuarter )
		, m_nRealtimeFrame( 0 )
		, m_fMasterPeak_L( 0.0f )
		, m_fMasterPeak_R( 0.0f )
		, m_nextState( State::Ready )
		, m_fProcessTime( 0.0f )
		, m_fLadspaTime( 0.0f )
		, m_fMaxProcessTime( 0.0f )
		, m_fNextBpm( 120 )
		, m_pLocker({nullptr, 0, nullptr, false})
		, m_fLastTickEnd( 0 )
		, m_bLookaheadApplied( false )
		, m_nLoopsDone( 0 )
{
	m_pTransportPosition = std::make_shared<TransportPosition>( "Transport" );
	m_pQueuingPosition = std::make_shared<TransportPosition>( "Queuing" );
	
	m_pSampler = new Sampler;

	srand( time( nullptr ) );

	// Create metronome instrument
	// Get the path to the file of the metronome sound.
	QString sMetronomeFilename = Filesystem::click_file_path();
	m_pMetronomeInstrument = std::make_shared<Instrument>( METRONOME_INSTR_ID, "metronome" );
	
	auto pLayer = std::make_shared<InstrumentLayer>( Sample::load( sMetronomeFilename ) );
	auto pComponent = m_pMetronomeInstrument->getComponent( 0 );
	if ( pComponent != nullptr ) {
		pComponent->setLayer( pLayer, 0 );
	} else {
		___ERRORLOG( "Invalid default component" );
	}
	m_pMetronomeInstrument->setVolume(
		Preferences::get_instance()->m_fMetronomeVolume );
	
	m_AudioProcessCallback = &audioEngine_process;

#ifdef H2CORE_HAVE_LADSPA
	Effects::create_instance();
#endif
}

AudioEngine::~AudioEngine()
{
	stopAudioDrivers();
	if ( getState() != State::Initialized ) {
		AE_ERRORLOG( "Error the audio engine is not in State::Initialized" );
		return;
	}
	m_pSampler->stopPlayingNotes();

	this->lock( RIGHT_HERE );
	AE_INFOLOG( "*** Hydrogen audio engine shutdown ***" );

	clearNoteQueues();

	setState( State::Uninitialized );

	m_pTransportPosition->reset();
	m_pTransportPosition = nullptr;
	m_pQueuingPosition->reset();
	m_pQueuingPosition = nullptr;

	m_pMetronomeInstrument = nullptr;

	this->unlock();
	
#ifdef H2CORE_HAVE_LADSPA
	delete Effects::get_instance();
#endif

	delete m_pSampler;
}

Sampler* AudioEngine::getSampler() const
{
	assert(m_pSampler);
	return m_pSampler;
}

void AudioEngine::lock( const char* file, unsigned int line, const char* function )
{
#ifdef H2CORE_HAVE_DEBUG
	// Is there a more convenient way to convert the thread id to QSTring?
	std::stringstream tmpStream;
	tmpStream << std::this_thread::get_id();
	if (__logger->should_log(Logger::Locks)) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1] : %2 : [line: %3] : %4" )
					   .arg( QString::fromStdString( tmpStream.str() ) )
					   .arg( function ).arg( line ).arg( file ) );
	}
#endif

	m_EngineMutex.lock();
	m_pLocker.file = file;
	m_pLocker.line = line;
	m_pLocker.function = function;
	m_pLocker.isLocked = true;
	m_LockingThread = std::this_thread::get_id();

#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1] locked" )
					   .arg( QString::fromStdString( tmpStream.str() ) ) );
	}
#endif
}

bool AudioEngine::tryLock( const char* file, unsigned int line, const char* function )
{
#ifdef H2CORE_HAVE_DEBUG
	// Is there a more convenient way to convert the thread id to QSTring?
	std::stringstream tmpStream;
	tmpStream << std::this_thread::get_id();
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1] : %2 : [line: %3] : %4" )
					   .arg( QString::fromStdString( tmpStream.str() ) )
					   .arg( function ).arg( line ).arg( file ) );
	}
#endif
	bool res = m_EngineMutex.try_lock();
	if ( !res ) {
		// Lock not obtained
		return false;
	}
	m_pLocker.file = file;
	m_pLocker.line = line;
	m_pLocker.function = function;
	m_pLocker.isLocked = true;
	m_LockingThread = std::this_thread::get_id();

#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1] locked" )
					   .arg( QString::fromStdString( tmpStream.str() ) ) );
	}
#endif

	return true;
}

bool AudioEngine::tryLockFor( const std::chrono::microseconds& duration, const char* file, unsigned int line, const char* function )
{
	std::stringstream tmpStream;
	tmpStream << std::this_thread::get_id();
#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1] : %2 : [line: %3] : %4" )
					   .arg( QString::fromStdString( tmpStream.str() ) )
					   .arg( function ).arg( line ).arg( file ) );
	}
#endif

	bool res = m_EngineMutex.try_lock_for( duration );
	if ( !res ) {
		// Lock not obtained
		AE_WARNINGLOG( QString( "[thread id: %1] : Lock timeout: lock timeout %2:%3:%4, lock held by %5:%6:%7" )
					   .arg( QString::fromStdString( tmpStream.str() ) )
					   .arg( file ).arg( function ).arg( line )
					   .arg( m_pLocker.file ).arg( m_pLocker.function )
					   .arg( m_pLocker.line ));
		return false;
	}
	m_pLocker.file = file;
	m_pLocker.line = line;
	m_pLocker.function = function;
	m_pLocker.isLocked = true;
	m_LockingThread = std::this_thread::get_id();

#ifdef H2CORE_HAVE_DEBUG
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1] locked" )
					   .arg( QString::fromStdString( tmpStream.str() ) ) );
	}
#endif

	return true;
}

void AudioEngine::unlock()
{
	// Leave "__locker" dirty as it indicated the function which
	// locked the audio engine.
	m_pLocker.isLocked = false;

	m_LockingThread = std::thread::id();
	m_EngineMutex.unlock();

#ifdef H2CORE_HAVE_DEBUG
	std::stringstream tmpStream;
	tmpStream << std::this_thread::get_id();
	if ( __logger->should_log( Logger::Locks ) ) {
		__logger->log( Logger::Locks, _class_name(), __FUNCTION__,
					   QString( "[thread id: %1]" )
					   .arg( QString::fromStdString( tmpStream.str() ) ) );
	}
#endif
}

void AudioEngine::startPlayback()
{
	AE_INFOLOG( "" );

	if ( getState() != State::Ready ) {
	   AE_ERRORLOG( "Error the audio engine is not in State::Ready" );
		return;
	}

	setState( State::Playing );
	
	handleSelectedPattern();
}

void AudioEngine::stopPlayback( Event::Trigger trigger )
{
	AE_INFOLOG( "" );

	if ( getState() != State::Playing ) {
		AE_ERRORLOG( QString( "Error the audio engine is not in State::Playing but [%1]" )
				  .arg( static_cast<int>( getState() ) ) );
		return;
	}

	setState( State::Ready, trigger );
}

void AudioEngine::reset( bool bWithJackBroadcast, Event::Trigger trigger ) {
	const auto pHydrogen = Hydrogen::get_instance();
	
	clearNoteQueues();
	
	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;

#ifdef H2CORE_HAVE_LADSPA
	for ( int ii = 0; ii < MAX_FX; ++ii ) {
		m_fFXPeak_L[ ii ] = 0;
		m_fFXPeak_R[ ii ] = 0;
	}
#endif

	m_fLastTickEnd = 0;
	m_nLoopsDone = 0;
	m_bLookaheadApplied = false;

	setNextBpm( 120 );

	m_pTransportPosition->reset();
	m_pQueuingPosition->reset();

	updateBpmAndTickSize( m_pTransportPosition, trigger );
	updateBpmAndTickSize( m_pQueuingPosition, trigger );

	updatePlayingPatterns( trigger );
	
#ifdef H2CORE_HAVE_JACK
	if ( pHydrogen->hasJackTransport() && bWithJackBroadcast ) {
		// Tell the JACK server to locate to the beginning as well
		// (done in the next run of audioEngine_process()).
		static_cast<JackAudioDriver*>( m_pAudioDriver )->locateTransport( 0 );
	}
#endif
}

float AudioEngine::computeTickSize( const int nSampleRate, const float fBpm )
{
	float fTickSize = nSampleRate * 60.0 / fBpm / H2Core::nTicksPerQuarter;
	
	return fTickSize;
}

double AudioEngine::computeDoubleTickSize( const int nSampleRate, const float fBpm )
{
	double fTickSize = static_cast<double>(nSampleRate) * 60.0 /
		static_cast<double>(fBpm) /
		static_cast<double>(H2Core::nTicksPerQuarter);
	
	return fTickSize;
}

float AudioEngine::getElapsedTime() const {
	
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pDriver = pHydrogen->getAudioOutput();
	
	if ( pDriver == nullptr || pDriver->getSampleRate() == 0 ) {
		return 0;
	}

	return ( m_pTransportPosition->getFrame() -
			 m_pTransportPosition->getFrameOffsetTempo() )/
		static_cast<float>(pDriver->getSampleRate());
}

void AudioEngine::locate( const double fTick, bool bWithJackBroadcast,
						  Event::Trigger trigger ) {
	const auto pHydrogen = Hydrogen::get_instance();

#ifdef H2CORE_HAVE_JACK
	// In case Hydrogen is using the JACK server to sync transport, it
	// is up to the server to relocate to a different position. It
	// does so after the current cycle of audioEngine_process() and we
	// will pick it up at the beginning of the next one.
	if ( pHydrogen->hasJackTransport() && bWithJackBroadcast ) {

		double fNewTick = fTick;
		// As the tick mismatch is lost when converting a sought location from
		// ticks into frames, sending it to the JACK server, receiving it in a
		// callback, and providing it here, we will use a heuristic in order to
		// not experience any glitches upon relocation.
		if ( std::fmod( fTick, std::floor( fTick ) ) >= 0.97 ) {
			fNewTick = std::round( fTick );
			AE_INFOLOG( QString( "Tick [%1] will be rounded to [%2] in order to avoid glitches" )
						.arg( fTick, 0, 'E', -1 ).arg( fNewTick ) );
		}

		double fTickMismatch;
		const long long nNewFrame =	TransportPosition::computeFrameFromTick(
			fNewTick, &fTickMismatch );

#if AUDIO_ENGINE_DEBUG
		AE_DEBUGLOG( QString( "[Locate via JACK server] fTick: %1, fNewTick: %2, nNewFrame: %3" )
					 .arg( fTick ).arg( fNewTick ) .arg( nNewFrame ) );
#endif

		static_cast<JackAudioDriver*>( m_pAudioDriver )->
			locateTransport( nNewFrame );
		return;
	}
#endif

	resetOffsets();
	m_fLastTickEnd = fTick;
	const long long nNewFrame = TransportPosition::computeFrameFromTick(
		fTick, &m_pTransportPosition->m_fTickMismatch );

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "fTick: %1, nNewFrame: %2" )
				 .arg( fTick ).arg( nNewFrame ) );
#endif

	updateTransportPosition( fTick, nNewFrame, m_pTransportPosition, trigger );
	m_pQueuingPosition->set( m_pTransportPosition );
	
	handleTempoChange();
}

void AudioEngine::locateToFrame( const long long nFrame ) {

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "nFrame: %1" ).arg( nFrame ) );
#endif

	resetOffsets();

	const double fNewTick = TransportPosition::computeTickFromFrame( nFrame );
	m_fLastTickEnd = fNewTick;

	// Assure tick<->frame can be converted properly using mismatch.
	const long long nNewFrame = TransportPosition::computeFrameFromTick(
		fNewTick, &m_pTransportPosition->m_fTickMismatch );

	updateTransportPosition( fNewTick, nNewFrame, m_pTransportPosition );
	m_pQueuingPosition->set( m_pTransportPosition );

	handleTempoChange();

	// While the locate() function is wrapped by a caller in the
	// CoreActionController - which takes care of queuing the
	// relocation event - this function is only meant to be used in
	// very specific circumstances and has to queue it itself.
	EventQueue::get_instance()->pushEvent( Event::Type::Relocation, 0 );
}

void AudioEngine::resetOffsets() {
	clearNoteQueues();

	m_fLastTickEnd = 0;
	m_nLoopsDone = 0;
	m_bLookaheadApplied = false;

	m_pTransportPosition->setFrameOffsetTempo( 0 );
	m_pTransportPosition->setTickOffsetQueuing( 0 );
	m_pTransportPosition->setTickOffsetSongSize( 0 );
	m_pTransportPosition->setLastLeadLagFactor( 0 );
	m_pQueuingPosition->setFrameOffsetTempo( 0 );
	m_pQueuingPosition->setTickOffsetQueuing( 0 );
	m_pQueuingPosition->setTickOffsetSongSize( 0 );
	m_pQueuingPosition->setLastLeadLagFactor( 0 );
}

void AudioEngine::incrementTransportPosition( uint32_t nFrames ) {
	const long long nNewFrame = m_pTransportPosition->getFrame() + nFrames;
	const double fNewTick = TransportPosition::computeTickFromFrame( nNewFrame );
	m_pTransportPosition->m_fTickMismatch = 0;

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "nFrames: %1, old frame: %2, new frame: %3, old tick: %4, new tick: %5, ticksize: %6" )
			  .arg( nFrames )
			  .arg( m_pTransportPosition->getFrame() )
			  .arg( nNewFrame )
			  .arg( m_pTransportPosition->getDoubleTick(), 0, 'f' )
			  .arg( fNewTick, 0, 'f' )
			  .arg( m_pTransportPosition->getTickSize(), 0, 'f' ) );
#endif

	updateTransportPosition( fNewTick, nNewFrame, m_pTransportPosition );

	// We are not updating the queuing position in here. This will be
	// done in updateNoteQueue().
}

bool AudioEngine::isEndOfSongReached( std::shared_ptr<TransportPosition> pPos ) const {
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr && pSong->getMode() == Song::Mode::Song &&
		 ( pSong->getLoopMode() == Song::LoopMode::Disabled &&
		   pPos->getDoubleTick() >= m_fSongSizeInTicks ||
		   pSong->getLoopMode() == Song::LoopMode::Finishing &&
		   pPos->getDoubleTick() >= m_fSongSizeInTicks *
		   (1 + static_cast<double>(m_nLoopsDone)) ) ) {
		return true;
	}

	return false;
}

void AudioEngine::updateTransportPosition( double fTick, long long nFrame,
										   std::shared_ptr<TransportPosition> pPos,
										   Event::Trigger trigger ) {

	const auto pHydrogen = Hydrogen::get_instance();

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[Before] fTick: %1, nFrame: %2, pos: %3" )
				.arg( fTick, 0, 'f' )
				.arg( nFrame )
				.arg( pPos->toQString( "", true ) ) );
#endif

	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		updateSongTransportPosition( fTick, nFrame, pPos, trigger );
	}
	else {  // Song::Mode::Pattern
		updatePatternTransportPosition( fTick, nFrame, pPos, trigger );
	}

	updateBpmAndTickSize( pPos, trigger );


	// Beat - Bar (- Tick) information is a coarse grained position
	// information and might not change on small position increments.
	bool bBBTChanged = false;
	const int nBar = std::max( pPos->getColumn(), 0 ) + 1;
	if ( nBar != pPos->getBar() ) {
		pPos->setBar( nBar );
		bBBTChanged = true;
	}

	const int nBeat = static_cast<int>(
		std::floor(static_cast<float>(pPos->getPatternTickPosition()) /
				   H2Core::nTicksPerQuarter )) + 1;
	if ( pPos->getBeat() != nBeat ) {
		pPos->setBeat( nBeat );
		bBBTChanged = true;
	}

	if ( pPos == m_pTransportPosition && bBBTChanged &&
		 trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent( Event::Type::BbtChanged, 0 );
	}

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[After] fTick: %1, nFrame: %2, pos: %3, frame: %4" )
				.arg( fTick, 0, 'f' )
				.arg( nFrame )
				.arg( pPos->toQString( "", true ) )
				.arg( pPos->getFrame() ) );
#endif

}

void AudioEngine::updatePatternTransportPosition( double fTick, long long nFrame,
												  std::shared_ptr<TransportPosition> pPos,
												  Event::Trigger trigger ) {

	auto pHydrogen = Hydrogen::get_instance();

	pPos->setTick( fTick );
	pPos->setFrame( nFrame );
	
	const double fPatternStartTick =
		static_cast<double>(pPos->getPatternStartTick());
	const int nPatternSize = pPos->getPatternSize();
	
	if ( fTick >= fPatternStartTick + static_cast<double>(nPatternSize) ||
		 fTick < fPatternStartTick ) {
		// Transport went past the end of the pattern or Pattern mode
		// was just activated.
		pPos->setPatternStartTick( pPos->getPatternStartTick() +
								   static_cast<long>(std::floor( ( fTick - fPatternStartTick ) /
																 static_cast<double>(nPatternSize) )) *
								   nPatternSize );

		// In stacked pattern mode we will only update the playing
		// patterns if the transport of the original pattern is looped
		// back to the beginning. This way all patterns start fresh.
		//
		// In selected pattern mode pattern change does occur
		// asynchonically by user interaction.
		if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
			updatePlayingPatternsPos( pPos, trigger );
		}
	}

	long nPatternTickPosition = static_cast<long>(std::floor( fTick )) -
		pPos->getPatternStartTick();
	if ( nPatternTickPosition > nPatternSize ) {
		nPatternTickPosition = ( static_cast<long>(std::floor( fTick ))
								 - pPos->getPatternStartTick() ) %
			nPatternSize;
	}
	pPos->setPatternTickPosition( nPatternTickPosition );
}

void AudioEngine::updateSongTransportPosition( double fTick, long long nFrame,
											   std::shared_ptr<TransportPosition> pPos,
											   Event::Trigger trigger ) {

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[Before] fTick: %1, nFrame: %2, m_fSongSizeInTicks: %3, pos: %4" )
				.arg( fTick, 0, 'f' )
				.arg( nFrame )
				.arg( m_fSongSizeInTicks, 0, 'f' )
				.arg( pPos->toQString( "", true ) ) );
#endif

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();

	pPos->setTick( fTick );
	pPos->setFrame( nFrame );

	if ( fTick < 0 ) {
		AE_ERRORLOG( QString( "[%1] Provided tick [%2] is negative!" )
				  .arg( pPos->getLabel() )
				  .arg( fTick, 0, 'f' ) );
		return;
	}

	int nNewColumn;
	if ( pSong == nullptr || pSong->getPatternGroupVector()->size() == 0 ) {
		// There are no patterns in song.
		pPos->setPatternStartTick( 0 );
		pPos->setPatternTickPosition( 0 );
		nNewColumn = 0;
	}
	else {
		long nPatternStartTick;
		nNewColumn = pHydrogen->getColumnForTick(
			std::floor( fTick ), pSong->isLoopEnabled(), &nPatternStartTick );
		pPos->setPatternStartTick( nPatternStartTick );

		// While the current tick position is constantly increasing,
		// m_nPatternStartTick is only defined between 0 and
		// m_fSongSizeInTicks. We will take care of the looping next.
		if ( nNewColumn == -1 ) {
			// Loop mode is not activated and the new position lies beyond the
			// end of the song. We jump to the beginning to indicated transport
			// is "finished".
			pPos->setPatternTickPosition( 0 );
		}
		else if ( fTick >= m_fSongSizeInTicks && m_fSongSizeInTicks != 0 ) {
			pPos->setPatternTickPosition(
				std::fmod( std::floor( fTick ) - nPatternStartTick,
						   m_fSongSizeInTicks ) );
		}
		else {
			pPos->setPatternTickPosition( std::floor( fTick ) - nPatternStartTick );
		}
	}
	
	if ( pPos->getColumn() != nNewColumn ) {
		pPos->setColumn( nNewColumn );

		updatePlayingPatternsPos( pPos, trigger );

		if ( pPos == m_pTransportPosition ) {
			if ( trigger == Event::Trigger::Suppress ) {
				handleSelectedPattern( trigger  );
			}
			else {
				handleSelectedPattern( Event::Trigger::Force );
			}
		}
	}

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[After] fTick: %1, nFrame: %2, m_fSongSizeInTicks: %3, pos: %4, frame: %5" )
				.arg( fTick, 0, 'f' )
				.arg( nFrame )
				.arg( m_fSongSizeInTicks, 0, 'f' )
				.arg( pPos->toQString( "", true ) )
				.arg( pPos->getFrame() ) );
#endif

}

void AudioEngine::updateBpmAndTickSize( std::shared_ptr<TransportPosition> pPos,
										Event::Trigger trigger ) {
	if ( ! ( m_state == State::Playing ||
			 m_state == State::Ready ||
			 m_state == State::Testing ) ) {
		return;
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	const float fOldBpm = pPos->getBpm();
	
	float fNewBpm = getBpmAtColumn( pPos->getColumn() );
	// If we are in Pattern mode or Timeline is not activated in Song Mode +
	// there is no external application controling tempo of Hydrogen, we are
	// free to apply a new tempo. This was set by the user either via UI or
	// MIDI/OSC message.
	if ( pHydrogen->getJackTimebaseState() !=
		 JackAudioDriver::Timebase::Listener &&
		 ( ( pSong != nullptr && ! pSong->getIsTimelineActivated() ) ||
				pHydrogen->getMode() != Song::Mode::Song ) &&
		 fNewBpm != m_fNextBpm ) {
		fNewBpm = m_fNextBpm;
	}

	if ( fNewBpm != fOldBpm ) {
		pPos->setBpm( fNewBpm );
		if ( pPos == m_pTransportPosition &&
			 trigger != Event::Trigger::Suppress ) {
			EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, 0 );
		}
	}

	const float fOldTickSize = pPos->getTickSize();
	const float fNewTickSize = AudioEngine::computeTickSize(
		static_cast<float>(m_pAudioDriver->getSampleRate()), fNewBpm );

	// Nothing changed - avoid recomputing
#if defined(WIN32) and !defined(WIN64)
	// For some reason two identical numbers (according to their
	// values when printing them) are not equal to each other in 32bit
	// Windows. Course graining the tick change in here will do no
	// harm except of for preventing tiny tempo changes. Integer value
	// changes should not be affected.
	if ( std::abs( fNewTickSize - fOldTickSize ) < 1e-2 ) {
#else
	if ( fNewTickSize == fOldTickSize ) {
#endif
		return;
	}

	if ( fNewTickSize == 0 ) {
		AE_ERRORLOG( QString( "[%1] Something went wrong while calculating the tick size. [oldTS: %2, newTS: %3]" )
				  .arg( pPos->getLabel() )
				  .arg( fOldTickSize, 0, 'f' ).arg( fNewTickSize, 0, 'f' ) );
		return;
	}

	// The lookahead in updateNoteQueue is tempo dependent (since it
	// contains both tick and frame components). By resetting this
	// variable we allow that the next one calculated to have
	// arbitrary values.
	pPos->setLastLeadLagFactor( 0 );

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG(QString( "[%1] [%7,%8] sample rate: %2, tick size: %3 -> %4, bpm: %5 -> %6" )
			 .arg( pPos->getLabel() )
			 .arg( static_cast<float>(m_pAudioDriver->getSampleRate()))
			 .arg( fOldTickSize, 0, 'f' )
			 .arg( fNewTickSize, 0, 'f' )
			 .arg( fOldBpm, 0, 'f' )
			 .arg( pPos->getBpm(), 0, 'f' )
			 .arg( pPos->getFrame() )
			 .arg( pPos->getDoubleTick(), 0, 'f' ) );
#endif
	
	pPos->setTickSize( fNewTickSize );
	
	calculateTransportOffsetOnBpmChange( pPos );
}

void AudioEngine::calculateTransportOffsetOnBpmChange( std::shared_ptr<TransportPosition> pPos ) {

	// If we deal with a single speed for the whole song, the frames
	// since the beginning of the song are tempo-dependent and have to
	// be recalculated.
	const long long nNewFrame =
		TransportPosition::computeFrameFromTick( pPos->getDoubleTick(),
												 &pPos->m_fTickMismatch );
	pPos->setFrameOffsetTempo( nNewFrame - pPos->getFrame() +
							   pPos->getFrameOffsetTempo() );

	if ( m_bLookaheadApplied ) {
			// if ( m_fLastTickEnd != 0 ) {
		const long long nNewLookahead =
			getLeadLagInFrames( pPos->getDoubleTick() ) +
			AudioEngine::nMaxTimeHumanize + 1;
		const double fNewTickEnd = TransportPosition::computeTickFromFrame(
			nNewFrame + nNewLookahead ) + pPos->getTickMismatch();
		pPos->setTickOffsetQueuing( fNewTickEnd - m_fLastTickEnd );

#if AUDIO_ENGINE_DEBUG
		AE_DEBUGLOG( QString( "[%1 : [%2] timeline] old frame: %3, new frame: %4, tick: %5, nNewLookahead: %6, pPos->getFrameOffsetTempo(): %7, pPos->getTickOffsetQueuing(): %8, fNewTickEnd: %9, m_fLastTickEnd: %10" )
				  .arg( pPos->getLabel() )
				  .arg( Hydrogen::get_instance()->isTimelineEnabled() )
				  .arg( pPos->getFrame() )
				  .arg( nNewFrame )
				  .arg( pPos->getDoubleTick(), 0, 'f' )
				  .arg( nNewLookahead )
				  .arg( pPos->getFrameOffsetTempo() )
				  .arg( pPos->getTickOffsetQueuing(), 0, 'f' )
				  .arg( fNewTickEnd, 0, 'f' )
				  .arg( m_fLastTickEnd, 0, 'f' )
				  );
#endif
		
	}

	// Happens when the Timeline was either toggled or tempo
	// changed while the former was deactivated.
	if ( pPos->getFrame() != nNewFrame ) {
		pPos->setFrame( nNewFrame );
	}

	if ( pPos == m_pTransportPosition ) {
		handleTempoChange();
	}
}

void AudioEngine::clearAudioBuffers( uint32_t nFrames )
{
	m_MutexOutputPointer.lock();
	float *pBuffer_L, *pBuffer_R;

	// clear main out Left and Right
	if ( m_pAudioDriver != nullptr ) {
		pBuffer_L = m_pAudioDriver->getOut_L();
		pBuffer_R = m_pAudioDriver->getOut_R();
		assert( pBuffer_L != nullptr && pBuffer_R != nullptr );
		memset( pBuffer_L, 0, nFrames * sizeof( float ) );
		memset( pBuffer_R, 0, nFrames * sizeof( float ) );
	}
	
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->hasJackAudioDriver() ) {
		JackAudioDriver* pJackAudioDriver = static_cast<JackAudioDriver*>(m_pAudioDriver);
	
		if ( pJackAudioDriver != nullptr ) {
			pJackAudioDriver->clearPerTrackAudioBuffers( nFrames );
		}
	}
#endif

	m_MutexOutputPointer.unlock();

#ifdef H2CORE_HAVE_LADSPA
	if ( getState() == State::Ready ||
		 getState() == State::Playing ||
		 getState() == State::Testing ) {
		Effects* pEffects = Effects::get_instance();
		for ( unsigned i = 0; i < MAX_FX; ++i ) {	// clear FX buffers
			auto pFX = pEffects->getLadspaFX( i );
			if ( pFX != nullptr ) {
				assert( pFX->m_pBuffer_L );
				assert( pFX->m_pBuffer_R );
				memset( pFX->m_pBuffer_L, 0, nFrames * sizeof( float ) );
				memset( pFX->m_pBuffer_R, 0, nFrames * sizeof( float ) );
			}
		}
	}
#endif
}

AudioOutput* AudioEngine::createAudioDriver( const Preferences::AudioDriver& driver )
{
	AE_INFOLOG( QString( "Creating driver [%1]" )
				.arg( Preferences::audioDriverToQString( driver ) ) );

	const auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	AudioOutput *pAudioDriver = nullptr;

	if ( driver == Preferences::AudioDriver::Oss ) {
		pAudioDriver = new OssDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::Jack ) {
		pAudioDriver = new JackAudioDriver( m_AudioProcessCallback );
#ifdef H2CORE_HAVE_JACK
		if ( auto pJackDriver = dynamic_cast<JackAudioDriver*>( pAudioDriver ) ) {
			pJackDriver->setConnectDefaults(
				Preferences::get_instance()->m_bJackConnectDefaults
			);
		}
#endif
	}
	else if ( driver == Preferences::AudioDriver::Alsa ) {
		pAudioDriver = new AlsaAudioDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::PortAudio ) {
		pAudioDriver = new PortAudioDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::CoreAudio ) {
		pAudioDriver = new CoreAudioDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::PulseAudio ) {
		pAudioDriver = new PulseAudioDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::Fake ) {
		AE_WARNINGLOG( "*** Using FAKE audio driver ***" );
		pAudioDriver = new FakeDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::Disk ) {
		pAudioDriver = new DiskWriterDriver( m_AudioProcessCallback );
	}
	else if ( driver == Preferences::AudioDriver::Null ) {
		pAudioDriver = new NullDriver( m_AudioProcessCallback );
	}
	else {
		AE_ERRORLOG( QString( "Unknown driver [%1]" )
				.arg( Preferences::audioDriverToQString( driver ) ) );
		raiseError( Hydrogen::UNKNOWN_DRIVER );
		return nullptr;
	}

	if ( pAudioDriver == nullptr ) {
		AE_INFOLOG( QString( "Unable to create driver [%1]" )
				.arg( Preferences::audioDriverToQString( driver ) ) );
		return nullptr;
	}

	// Initialize the audio driver
	int nRes = pAudioDriver->init( pPref->m_nBufferSize );
	if ( nRes != 0 ) {
		AE_ERRORLOG( QString( "Error code [%2] while initializing audio driver [%1]." )
				.arg( Preferences::audioDriverToQString( driver ) ).arg( nRes ) );
		delete pAudioDriver;
		return nullptr;
	}

	this->lock( RIGHT_HERE );
	m_MutexOutputPointer.lock();

	// Some audio drivers require to be already registered in the
	// AudioEngine while being connected.
	m_pAudioDriver = pAudioDriver;

	if ( pSong != nullptr ) {
		setState( State::Ready );
	} else {
		setState( State::Prepared );
	}

	// Unlocking earlier might execute the jack process() callback before we
	// are fully initialized.
	m_MutexOutputPointer.unlock();
	this->unlock();
	
	nRes = m_pAudioDriver->connect();
	if ( nRes != 0 ) {
		raiseError( Hydrogen::ERROR_STARTING_DRIVER );
		AE_ERRORLOG( QString( "Error code [%2] while connecting audio driver [%1]." )
				.arg( Preferences::audioDriverToQString( driver ) ).arg( nRes ) );

		this->lock( RIGHT_HERE );
		m_MutexOutputPointer.lock();
		
		delete m_pAudioDriver;
		m_pAudioDriver = nullptr;
		
		m_MutexOutputPointer.unlock();
		this->unlock();

		return nullptr;
	}

	if ( pSong != nullptr && pHydrogen->hasJackAudioDriver() ) {
		pHydrogen->renameJackPorts( pSong );
	}

	lock( RIGHT_HERE );
	setupLadspaFX();

	if ( pSong != nullptr ) {
		handleDriverChange();
	}
	unlock();

	EventQueue::get_instance()->pushEvent( Event::Type::DriverChanged, 0 );

	return pAudioDriver;
}

void AudioEngine::startAudioDrivers()
{
	AE_INFOLOG("");
	const auto pPref = Preferences::get_instance();
	
	if ( getState() != State::Initialized ) {
		AE_ERRORLOG( QString( "Audio engine is not in State::Initialized but [%1]" )
				  .arg( static_cast<int>( getState() ) ) );
		return;
	}

	if ( m_pAudioDriver != nullptr ) {	// check if audio driver is still alive
		AE_ERRORLOG( "The audio driver is still alive" );
	}
	if ( m_pMidiDriver != nullptr ) {	// check if midi driver is still alive
		AE_ERRORLOG( "The MIDI driver is still active" );
	}

	const auto audioDriver = pPref->m_audioDriver;

	if ( audioDriver != Preferences::AudioDriver::Auto ) {
		createAudioDriver( audioDriver );
	}
	else {
		AudioOutput* pAudioDriver;
		for ( const auto& ddriver : Preferences::getSupportedAudioDrivers() ) {
			if ( ( pAudioDriver = createAudioDriver( ddriver ) ) != nullptr ) {
				break;
			}
		}
	}

	if ( m_pAudioDriver == nullptr ) {
		AE_ERRORLOG( QString( "Couldn't start audio driver [%1], falling back to NullDriver" )
				  .arg( Preferences::audioDriverToQString( audioDriver ) ) );
		createAudioDriver( Preferences::AudioDriver::Null );
	}

	this->lock( RIGHT_HERE );
	m_MutexOutputPointer.lock();
	
	if ( pPref->m_sMidiDriver == "ALSA" ) {
#ifdef H2CORE_HAVE_ALSA
		AlsaMidiDriver *alsaMidiDriver = new AlsaMidiDriver();
		m_pMidiDriverOut = alsaMidiDriver;
		m_pMidiDriver = alsaMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( pPref->m_sMidiDriver == "PortMidi" ) {
#ifdef H2CORE_HAVE_PORTMIDI
		PortMidiDriver* pPortMidiDriver = new PortMidiDriver();
		m_pMidiDriver = pPortMidiDriver;
		m_pMidiDriverOut = pPortMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( pPref->m_sMidiDriver == "CoreMIDI" ) {
#ifdef H2CORE_HAVE_COREMIDI
		CoreMidiDriver *coreMidiDriver = new CoreMidiDriver();
		m_pMidiDriver = coreMidiDriver;
		m_pMidiDriverOut = coreMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( pPref->m_sMidiDriver == "JACK-MIDI" ) {
#ifdef H2CORE_HAVE_JACK
		JackMidiDriver *jackMidiDriver = new JackMidiDriver();
		m_pMidiDriverOut = jackMidiDriver;
		m_pMidiDriver = jackMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	}
	
	m_MutexOutputPointer.unlock();
	this->unlock();
}

void AudioEngine::stopAudioDrivers()
{
	AE_INFOLOG( "" );

	this->lock( RIGHT_HERE );

	if ( m_state == State::Playing ) {
		this->stopPlayback();
	}

	if ( ( m_state != State::Prepared )
		 && ( m_state != State::Ready ) ) {
		AE_ERRORLOG( QString( "Audio engine is not in State::Prepared or State::Ready but [%1]" )
				  .arg( static_cast<int>(m_state) ) );
		this->unlock();
		return;
	}


	setState( State::Initialized );

	if ( m_pMidiDriver != nullptr ) {
		m_pMidiDriver->close();
		delete m_pMidiDriver;
		m_pMidiDriver = nullptr;
		m_pMidiDriverOut = nullptr;
	}

	if ( m_pAudioDriver != nullptr ) {
		m_pAudioDriver->disconnect();
		m_MutexOutputPointer.lock();
		delete m_pAudioDriver;
		m_pAudioDriver = nullptr;
		m_MutexOutputPointer.unlock();
	}

	this->unlock();
}

void AudioEngine::restartAudioDrivers()
{
	bool bPlaying = m_state == State::Playing;
	if ( m_pAudioDriver != nullptr ) {
		stopAudioDrivers();
	}
	startAudioDrivers();
	if ( bPlaying ) {
		this->startPlayback();
	}

}

void AudioEngine::handleLoopModeChanged() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr &&
		 pSong->getLoopMode() == Song::LoopMode::Finishing ) {
		m_nLoopsDone = static_cast<int>(std::floor(
			m_pTransportPosition->getDoubleTick() / m_fSongSizeInTicks ));
	}
}

void AudioEngine::handleDriverChange() {

	if ( Hydrogen::get_instance()->getSong() == nullptr ) {
		AE_WARNINGLOG( "no song set yet" );
		return;
	}
	
	handleTimelineChange();
}

float AudioEngine::getBpmAtColumn( int nColumn ) {

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		AE_WARNINGLOG( "no song set yet" );
		return MIN_BPM;
	}

	float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	if ( pHydrogen->getJackTimebaseState() ==
		 JackAudioDriver::Timebase::Listener ) {
		// Hydrogen is using the BPM broadcasted by the JACK
		// server. This one does solely depend on external
		// applications and will NOT be stored in the Song.
		const float fJackTimebaseBpm = pHydrogen->getJackTimebaseControllerBpm();
		if ( ! std::isnan( fJackTimebaseBpm ) ) {
			if ( fBpm != fJackTimebaseBpm ) {
				fBpm = fJackTimebaseBpm;
#if AUDIO_ENGINE_DEBUG
				AE_DEBUGLOG( QString( "Tempo update by the JACK server [%1]")
							 .arg( fJackTimebaseBpm ) );
#endif
			}
		} else {
			AE_ERRORLOG( "Unable to retrieve tempo from JACK server" );
		}
	}
	else if ( pSong->getIsTimelineActivated() &&
				pHydrogen->getMode() == Song::Mode::Song ) {

		const float fTimelineBpm = pHydrogen->getTimeline()->getTempoAtColumn( nColumn );
		if ( fTimelineBpm != fBpm ) {
#if AUDIO_ENGINE_DEBUG
			AE_DEBUGLOG( QString( "Set tempo to timeline value [%1]")
						 .arg( fTimelineBpm ) );
#endif
			fBpm = fTimelineBpm;
		}
	}
	else {
		// Change in speed due to user interaction with the BPM widget
		// or corresponding MIDI or OSC events.
		if ( pAudioEngine->getNextBpm() != fBpm ) {
#if AUDIO_ENGINE_DEBUG
			AE_DEBUGLOG( QString( "BPM changed via Widget, OSC, or MIDI from [%1] to [%2]." )
					  .arg( fBpm ).arg( pAudioEngine->getNextBpm() ) );
			// We do not return AudioEngine::m_fNextBpm since it is considered
			// transient until applied in updateBpmAndTickSize(). It's not the
			// current tempo.
#endif
		}
	}
	return fBpm;
}

void AudioEngine::setupLadspaFX()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

#ifdef H2CORE_HAVE_LADSPA
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		auto pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX == nullptr ) {
			return;
		}

		pFX->deactivate();

		pFX->connectAudioPorts( pFX->m_pBuffer_L, pFX->m_pBuffer_R,
								pFX->m_pBuffer_L, pFX->m_pBuffer_R );
		pFX->activate();
	}
#endif
}

void AudioEngine::raiseError( unsigned nErrorCode )
{
	EventQueue::get_instance()->pushEvent( Event::Type::Error, nErrorCode );
}

void AudioEngine::handleSelectedPattern( Event::Trigger trigger ) {
	
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	
	if ( pSong != nullptr && pHydrogen->isPatternEditorLocked() ) {

		// Default value is used to deselect the current pattern in
		// case none was found.
		int nPatternNumber = -1;

		const int nColumn = std::max( m_pTransportPosition->getColumn(), 0 );
		if ( nColumn < (*pSong->getPatternGroupVector()).size() ) {

			const auto pPatternList = pSong->getPatternList();
			if ( pPatternList != nullptr ) {

				const auto pColumn = ( *pSong->getPatternGroupVector() )[ nColumn ];

				int nIndex;
				for ( const auto& pattern : *pColumn ) {
					nIndex = pPatternList->index( pattern );

					if ( nIndex > nPatternNumber ) {
						nPatternNumber = nIndex;
					}
				}
			}
		}

		pHydrogen->setSelectedPatternNumber( nPatternNumber, true, trigger );
	}
}

void AudioEngine::handleSongModeChanged( Event::Trigger trigger ) {
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		AE_ERRORLOG( "no song set" );
		return;
	}

	m_fSongSizeInTicks = pSong->lengthInTicks();
	reset( true, trigger );
	setNextBpm( pSong->getBpm() );
}

void AudioEngine::processPlayNotes( unsigned long nframes )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	long long nFrame;
	if ( getState() == State::Playing || getState() == State::Testing ) {
		// Current transport position.
		nFrame = m_pTransportPosition->getFrame();
	} else {
		// In case the playback is stopped we pretend it is still
		// rolling using the realtime ticks while disregarding tempo
		// changes in the Timeline. This is important as we want to
		// continue playing back notes in the sampler and process
		// realtime events, by e.g. MIDI or Hydrogen's virtual
		// keyboard.
		nFrame = getRealtimeFrame();
	}

	while ( !m_songNoteQueue.empty() ) {
		auto pNote = m_songNoteQueue.top();
		if ( pNote == nullptr || pNote->getInstrument() == nullptr ) {
			m_songNoteQueue.pop();
			continue;
		}

		const long long nNoteStartInFrames = pNote->getNoteStart();

#if AUDIO_ENGINE_DEBUG
		AE_DEBUGLOG( QString( "m_pTransportPosition->getDoubleTick(): %1, m_pTransportPosition->getFrame(): %2, nframes: %3, " )
				  .arg( m_pTransportPosition->getDoubleTick() )
				  .arg( m_pTransportPosition->getFrame() )
				  .arg( nframes )
				  .append( pNote->toQString( "", true ) ) );
#endif

		if ( nNoteStartInFrames < nFrame + static_cast<long long>(nframes) ) {

			float fNoteProbability = pNote->getProbability();
			if ( fNoteProbability != 1. ) {
				// Current note is skipped with a certain probability.
				if ( fNoteProbability < (float) rand() / (float) RAND_MAX ) {
					m_songNoteQueue.pop();
					pNote->getInstrument()->dequeue( pNote );
					continue;
				}
			}

			/*
			 * Check if the current instrument has the property "Stop-Note" set.
			 * If yes, a NoteOff note is generated automatically after each note.
			 */
			auto pNoteInstrument = pNote->getInstrument();
			if ( pNoteInstrument->isStopNotes() ){
				auto pOffNote = std::make_shared<Note>( pNoteInstrument );
				pOffNote->setNoteOff( true );
				m_pSampler->noteOn( pOffNote );
			}

			if ( ! pNote->getInstrument()->hasSamples() ) {
				m_songNoteQueue.pop();
				pNote->getInstrument()->dequeue( pNote );
				continue;
			}

			if ( pNoteInstrument == m_pMetronomeInstrument ) {
				EventQueue::get_instance()->pushEvent(
					Event::Type::Metronome, pNote->getPitch() == 0 ? 1 : 0 );
			}

			m_pSampler->noteOn( pNote );
			m_songNoteQueue.pop();
			pNote->getInstrument()->dequeue( pNote );
			
			const int nInstrument = pSong->getDrumkit()->getInstruments()->index( pNote->getInstrument() );

			// Check whether the instrument could be found.
			if ( nInstrument != -1 ) {
				EventQueue::get_instance()->pushEvent(
					Event::Type::NoteOn, nInstrument );
			}
			
			continue;
		} else {
			// this note will not be played
			break;
		}
	}
}

void AudioEngine::clearNoteQueues( std::shared_ptr<Instrument> pInstrument )
{
	// notes in the song queue. Attention: their instruments are enqueued.
	if ( pInstrument == nullptr ) {
		// delete all copied notes in the note queues
		while ( !m_songNoteQueue.empty() ) {
			auto pNote = m_songNoteQueue.top();
			if ( pNote != nullptr ) {
				if ( pNote->getInstrument() != nullptr ) {
					pNote->getInstrument()->dequeue( pNote );
				}
			}
			m_songNoteQueue.pop();
		}
	}
	else {
		// delete just notes of a particular instrument
		//
		// AFAICS we can not erase from m_songNoteQueue since it is a priority
		// queue. Instead, we construct it anew.
		std::vector<std::shared_ptr<Note>> notes;
		for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
			auto ppNote = m_songNoteQueue.top();
			if ( ppNote == nullptr || ppNote->getInstrument() == nullptr ||
				 ppNote->getInstrument() == pInstrument ) {
				if ( ppNote->getInstrument() != nullptr ) {
					ppNote->getInstrument()->dequeue( ppNote );
				}
			}
			else {
				// We keep this one
				notes.push_back( ppNote );
			}
		}
		for ( auto& ppNote : notes ) {
			m_songNoteQueue.push( ppNote );
		}

	}

	// Notes of MIDI note queue (no instrument enqueued in here).
	for ( auto it = m_midiNoteQueue.begin(); it != m_midiNoteQueue.end(); ) {
		auto ppNote = *it;
		if ( ppNote == nullptr || ppNote->getInstrument() == nullptr ||
			 ( pInstrument == nullptr ||
			   ppNote->getInstrument() == pInstrument ) ) {
			it = m_midiNoteQueue.erase( it );
		}
		else {
			++it;
		}
	}
}

int AudioEngine::audioEngine_process( uint32_t nframes, void* /*arg*/ )
{
	AudioEngine* pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	// For the JACK driver it is very important (#1867) to not do anything while
	// the JACK client is stopped/closed. Otherwise it will segfault on mutex
	// locking or message logging.
	if ( pAudioEngine->m_pAudioDriver == nullptr ||
		 ( ! ( pAudioEngine->getState() == AudioEngine::State::Ready ||
			   pAudioEngine->getState() == AudioEngine::State::Playing ) &&
		   dynamic_cast<JackAudioDriver*>(pAudioEngine->m_pAudioDriver) != nullptr ) ) {
		return 0;
	}
	timeval startTimeval = currentTime2();
	const auto sDrivers = pAudioEngine->getDriverNames();

	pAudioEngine->clearAudioBuffers( nframes );

	// Calculate maximum time to wait for audio engine lock. Using the
	// last calculated processing time as an estimate of the expected
	// processing time for this frame.
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
	 * audio processing.
	 */
	if ( !pAudioEngine->tryLockFor( std::chrono::microseconds( (int)(1000.0*fSlackTime) ),
							  RIGHT_HERE ) ) {
		___ERRORLOG( QString( "[%1] Failed to lock audioEngine in allowed %2 ms, missed buffer" )
					 .arg( sDrivers ).arg( fSlackTime ) );

		if ( dynamic_cast<DiskWriterDriver*>(pAudioEngine->m_pAudioDriver) != nullptr ) {
			// Returning the special return value "2" enables the disk 
			// writer driver - which does not require running in
			// realtime - to repeat the processing of the current data.
			return 2;
		}

		return 0;
	}

	// Now that the engine is locked we properly check its state.
	if ( ! ( pAudioEngine->getState() == AudioEngine::State::Ready ||
			 pAudioEngine->getState() == AudioEngine::State::Playing ) ) {
		pAudioEngine->unlock();
		return 0;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();

	// Sync transport with server (in case the current audio driver is
	// designed that way)
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->hasJackTransport() ) {
		auto pAudioDriver = pHydrogen->getAudioOutput();
		if ( pAudioDriver == nullptr ) {
			___ERRORLOG( QString( "[%1] AudioDriver is not ready!" )
						 .arg( sDrivers ) );
			assert( pAudioDriver );
			return 1;
		}
		static_cast<JackAudioDriver*>( pAudioDriver )->updateTransportPosition();
	}
#endif

	// Check whether the tempo was changed.
	pAudioEngine->updateBpmAndTickSize( pAudioEngine->m_pTransportPosition );
	pAudioEngine->updateBpmAndTickSize( pAudioEngine->m_pQueuingPosition );

	// Update the state of the audio engine depending on whether it
	// was started or stopped by the user.
	if ( pAudioEngine->m_nextState == State::Playing ) {
		if ( pAudioEngine->getState() == State::Ready ) {
			pAudioEngine->startPlayback();
		}
		
		pAudioEngine->setRealtimeFrame( pAudioEngine->m_pTransportPosition->getFrame() );
	} else {
		if ( pAudioEngine->getState() == State::Playing ) {
			pAudioEngine->stopPlayback();
		}
		
		// go ahead and increment the realtimeframes by nFrames
		// to support our realtime keyboard and midi event timing
		pAudioEngine->setRealtimeFrame( pAudioEngine->getRealtimeFrame() +
										 static_cast<long long>(nframes) );
	}

	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	pAudioEngine->updateNoteQueue( nframes );

	pAudioEngine->processAudio( nframes );

	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {

		// Check whether the end of the song has been reached.
		if ( pAudioEngine->isEndOfSongReached(
				 pAudioEngine->m_pTransportPosition ) ) {

			___INFOLOG( QString( "[%1] End of song received" ).arg( sDrivers ) );

			if ( pHydrogen->getMidiOutput() != nullptr ) {
				pHydrogen->getMidiOutput()->handleQueueAllNoteOff();
			}

			pAudioEngine->stop();
			pAudioEngine->stopPlayback();
			pAudioEngine->locate( 0 );

			// Tell GUI to move the playhead position to the beginning of
			// the song again since it only updates it in case transport
			// is rolling.
			EventQueue::get_instance()->pushEvent( Event::Type::Relocation, 0 );

			if ( dynamic_cast<FakeDriver*>(pAudioEngine->m_pAudioDriver) !=
				 nullptr ) {
				___INFOLOG( QString( "[%1] End of song." ).arg( sDrivers ) );

				// TODO This part of the code might not be reached
				// anymore.
				pAudioEngine->unlock();
				return 1;	// kill the audio AudioDriver thread
			}
		}
		else {
			// We are not at the end of the song, keep rolling.
			pAudioEngine->incrementTransportPosition( nframes );
		}
	}

	timeval finishTimeval = currentTime2();
	pAudioEngine->m_fProcessTime =
			( finishTimeval.tv_sec - startTimeval.tv_sec ) * 1000.0
			+ ( finishTimeval.tv_usec - startTimeval.tv_usec ) / 1000.0;
	
#ifdef CONFIG_DEBUG
	if ( pAudioEngine->m_fProcessTime > pAudioEngine->m_fMaxProcessTime ) {
		___WARNINGLOG( "" );
		___WARNINGLOG( "----XRUN----" );
		___WARNINGLOG( QString( "[%1] XRUN of %2 msec (%3 > %4)" )
					   .arg( sDrivers )
					   .arg( ( pAudioEngine->m_fProcessTime - pAudioEngine->m_fMaxProcessTime ) )
					   .arg( pAudioEngine->m_fProcessTime )
					   .arg( pAudioEngine->m_fMaxProcessTime ) );
		___WARNINGLOG( QString( "Ladspa process time = %1" ).arg( fLadspaTime ) );
		___WARNINGLOG( "------------" );
		___WARNINGLOG( "" );
		
		EventQueue::get_instance()->pushEvent( Event::Type::Xrun, -1 );
	}
#endif

	pAudioEngine->unlock();

	return 0;
}

void AudioEngine::processAudio( uint32_t nFrames ) {

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	processPlayNotes( nFrames );

	float *pBuffer_L = m_pAudioDriver->getOut_L(),
		*pBuffer_R = m_pAudioDriver->getOut_R();
	assert( pBuffer_L != nullptr && pBuffer_R != nullptr );

	getSampler()->process( nFrames );
	float* out_L = getSampler()->m_pMainOut_L;
	float* out_R = getSampler()->m_pMainOut_R;
	for ( unsigned i = 0; i < nFrames; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

#ifdef H2CORE_HAVE_LADSPA
	timeval ladspaTime_start = currentTime2();

	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		auto pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX != nullptr && pFX->isEnabled() ) {
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

	timeval ladspaTime_end = currentTime2();
	m_fLadspaTime =
			( ladspaTime_end.tv_sec - ladspaTime_start.tv_sec ) * 1000.0
			+ ( ladspaTime_end.tv_usec - ladspaTime_start.tv_usec ) / 1000.0;
#else
	m_fLadspaTime = 0.0;
#endif

	float fPeak_L = m_fMasterPeak_L, fPeak_R = m_fMasterPeak_R;
	for ( unsigned i = 0; i < nFrames; ++i ) {
		fPeak_L = std::max( fPeak_L, pBuffer_L[i] );
		fPeak_R = std::max( fPeak_R, pBuffer_R[i] );
	}

	m_fMasterPeak_L = fPeak_L;
	m_fMasterPeak_R = fPeak_R;
}

void AudioEngine::setState( const AudioEngine::State& state,
							Event::Trigger trigger ) {
	if ( m_state == state ) {
		if ( trigger == Event::Trigger::Force ) {
			EventQueue::get_instance()->pushEvent( Event::Type::State,
													static_cast<int>(state) );
		}
		return;
	}

	m_state = state;
	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent( Event::Type::State,
												static_cast<int>(state) );
	}
}

void AudioEngine::setNextBpm( float fNextBpm ) {
	if ( fNextBpm > MAX_BPM ) {
		m_fNextBpm = MAX_BPM;
		AE_WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
					.arg( fNextBpm ).arg( MAX_BPM ) );
	}
	else if ( fNextBpm < MIN_BPM ) {
		m_fNextBpm = MIN_BPM;
		AE_WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
					.arg( fNextBpm ).arg( MIN_BPM ) );
	}
	
	m_fNextBpm = fNextBpm;
}

void AudioEngine::setSong( std::shared_ptr<Song> pNewSong )
{
	auto pHydrogen = Hydrogen::get_instance();
	
	AE_INFOLOG( QString( "Set song: %1" )
				.arg( pNewSong != nullptr ? pNewSong->getName() : "nullptr" ) );
	
	if ( getState() != State::Prepared ) {
		AE_ERRORLOG( QString( "Error the audio engine is not in State::Prepared but [%1]" )
				  .arg( static_cast<int>( getState() ) ) );
	}

	if ( m_pAudioDriver != nullptr ) {
		setupLadspaFX();
	}

	float fNextBpm;
	if ( pNewSong != nullptr ) {
		fNextBpm = pNewSong->getBpm();
		m_fSongSizeInTicks = static_cast<double>( pNewSong->lengthInTicks() );
	}
	else {
		fNextBpm = MIN_BPM;
		m_fSongSizeInTicks = 4 * H2Core::nTicksPerQuarter;
	}
	// Reset (among other things) the transport position. This causes
	// the locate() call below to update the playing patterns.
	reset( false, Event::Trigger::Suppress );
	setNextBpm( fNextBpm );

	pHydrogen->renameJackPorts( pNewSong );

	setState( State::Ready, Event::Trigger::Suppress );
	// Will also adapt the audio engine to the new song's BPM.
	locate( 0 );

	if ( pNewSong != nullptr ) {
		pHydrogen->setTimeline( pNewSong->getTimeline() );
		pHydrogen->getTimeline()->activate();
	}
	else {
		pHydrogen->setTimeline( nullptr );
	}

	updateSongSize( Event::Trigger::Suppress );
}

void AudioEngine::prepare( Event::Trigger trigger ) {
	if ( getState() == State::Playing ) {
		stop();
		this->stopPlayback( trigger );
	}

	if ( getState() != State::Ready ) {
		AE_ERRORLOG( QString( "Error the audio engine is not in State::Ready but [%1]" )
				  .arg( static_cast<int>( getState() ) ) );
		return;
	}

	m_pSampler->stopPlayingNotes();
	reset( true, trigger );
	m_fSongSizeInTicks = 4 * H2Core::nTicksPerQuarter;

	setState( State::Prepared, trigger );
}

void AudioEngine::updateSongSize( Event::Trigger trigger ) {
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		AE_ERRORLOG( "No song set yet" );
		return;
	}

	auto updatePatternSize = []( std::shared_ptr<TransportPosition> pPos ) {
		if ( pPos->getPlayingPatterns()->size() > 0 ) {
			// No virtual pattern resolution in here
			pPos->setPatternSize( pPos->getPlayingPatterns()->longestPatternLength( false ) );
		} else {
			pPos->setPatternSize( 4 * H2Core::nTicksPerQuarter );
		}
	};
	updatePatternSize( m_pTransportPosition );
	updatePatternSize( m_pQueuingPosition );

	if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		AE_INFOLOG( QString( "[Pattern] Size update: [%1] -> [%2]" )
					.arg( m_fSongSizeInTicks )
					.arg( static_cast<double>( pSong->lengthInTicks() ) ) );
		m_fSongSizeInTicks = static_cast<double>( pSong->lengthInTicks() );

		if ( trigger != Event::Trigger::Suppress ) {
			EventQueue::get_instance()->pushEvent( Event::Type::SongSizeChanged, 0 );
		}
		return;
	}

	// Expected behavior:
	// - changing any part of the song except of the pattern currently
	//   playing shouldn't affect transport position
	// - the current transport position is defined as current column +
	//   current pattern tick position
	// - there shouldn't be a difference in behavior whether the song
	//   was already looped or not
	const double fNewSongSizeInTicks =
		static_cast<double>( pSong->lengthInTicks() );

	// Indicates that the song contains no patterns (before or after
	// song size did change). 
	const bool bEmptySong =
		m_fSongSizeInTicks == 0 || fNewSongSizeInTicks == 0;

	double fNewStrippedTick, fRepetitions;
	if ( m_fSongSizeInTicks != 0 ) {
		// Strip away all repetitions when in loop mode but keep their
		// number. nPatternStartTick and nColumn are only defined
		// between 0 and fSongSizeInTicks.
		fNewStrippedTick = std::fmod( m_pTransportPosition->getDoubleTick(),
									  m_fSongSizeInTicks );
		fRepetitions =
			std::floor( m_pTransportPosition->getDoubleTick() / m_fSongSizeInTicks );
	}
	else {
		// No patterns in song prior to song size change.
		fNewStrippedTick = m_pTransportPosition->getDoubleTick();
		fRepetitions = 0;
	}
	
	const int nOldColumn = m_pTransportPosition->getColumn();

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[Before] fNewStrippedTick: %1, fRepetitions: %2, m_fSongSizeInTicks: %3, fNewSongSizeInTicks: %4, transport: %5, queuing: %6" )
				.arg( fNewStrippedTick, 0, 'f' )
				.arg( fRepetitions )
				.arg( m_fSongSizeInTicks )
				.arg( fNewSongSizeInTicks )
				.arg( m_pTransportPosition->toQString( "", true ) )
				.arg( m_pQueuingPosition->toQString( "", true ) )
				);
#endif

	AE_INFOLOG( QString( "[Song] Size update: [%1] -> [%2]" )
				.arg( m_fSongSizeInTicks ).arg( fNewSongSizeInTicks ) );

	m_fSongSizeInTicks = fNewSongSizeInTicks;

	auto endOfSongReached = [&](){
		if ( getState() == State::Playing ) {
			stop();
			stopPlayback( trigger );
		}
		locate( 0, true, trigger );

#if AUDIO_ENGINE_DEBUG
		AE_DEBUGLOG( QString( "[End of song reached] fNewStrippedTick: %1, fRepetitions: %2, m_fSongSizeInTicks: %3, fNewSongSizeInTicks: %4, transport: %5, queuing: %6" )
					.arg( fNewStrippedTick, 0, 'f' )
					.arg( fRepetitions )
					.arg( m_fSongSizeInTicks )
					.arg( fNewSongSizeInTicks )
					.arg( m_pTransportPosition->toQString( "", true ) )
					.arg( m_pQueuingPosition->toQString( "", true ) )
					);
#endif

		if ( trigger != Event::Trigger::Suppress ) {
			EventQueue::get_instance()->pushEvent( Event::Type::SongSizeChanged, 0 );
		}
	};

	if ( nOldColumn >= pSong->getPatternGroupVector()->size() &&
		pSong->getLoopMode() != Song::LoopMode::Enabled ) {
		// Old column exceeds the new song size.
		endOfSongReached();
		return;
	}
		

	const long nNewPatternStartTick = pHydrogen->getTickForColumn( nOldColumn );

	if ( nNewPatternStartTick == -1 &&
		pSong->getLoopMode() != Song::LoopMode::Enabled ) {
		// Failsave in case old column exceeds the new song size.
		endOfSongReached();
		return;
	}
	
	if ( nNewPatternStartTick != m_pTransportPosition->getPatternStartTick() &&
		 ! bEmptySong ) {
		// A pattern prior to the current position was toggled,
		// enlarged, or shrunk. We need to compensate this in order to
		// keep the current pattern tick position constant.

#if AUDIO_ENGINE_DEBUG
		AE_DEBUGLOG( QString( "[nPatternStartTick mismatch] old: %1, new: %2" )
				  .arg( m_pTransportPosition->getPatternStartTick() )
				  .arg( nNewPatternStartTick ) );
#endif
		
		fNewStrippedTick +=
			static_cast<double>(nNewPatternStartTick -
								m_pTransportPosition->getPatternStartTick());
	}
	
#ifdef H2CORE_HAVE_DEBUG
	const long nNewPatternTickPosition =
		static_cast<long>(std::floor( fNewStrippedTick )) - nNewPatternStartTick;
	if ( nNewPatternTickPosition != m_pTransportPosition->getPatternTickPosition() &&
		 ! bEmptySong ) {
		AE_ERRORLOG( QString( "[nPatternTickPosition mismatch] old: %1, new: %2" )
				  .arg( m_pTransportPosition->getPatternTickPosition() )
				  .arg( nNewPatternTickPosition ) );
	}
#endif

	// Incorporate the looped transport again
	const double fNewTick = fNewStrippedTick + fRepetitions * fNewSongSizeInTicks;
	const long long nNewFrame = TransportPosition::computeFrameFromTick(
		fNewTick, &m_pTransportPosition->m_fTickMismatch );

	double fTickOffset = fNewTick - m_pTransportPosition->getDoubleTick();

	// The tick interval end covered in updateNoteQueue() is stored as
	// double and needs to be more precise (hence updated before
	// rounding).
	m_fLastTickEnd += fTickOffset;

	// Small rounding noise introduced in the calculation does spoil
	// things as we floor the resulting tick offset later on. Hence,
	// we round it to a specific precision.
	fTickOffset *= 1e8;
	fTickOffset = std::round( fTickOffset );
	fTickOffset *= 1e-8;
	m_pTransportPosition->setTickOffsetSongSize( fTickOffset );

	// Moves all notes currently processed by Hydrogen with respect to
	// the offsets calculated above.
	handleSongSizeChange();

	m_pTransportPosition->setFrameOffsetTempo(
		nNewFrame - m_pTransportPosition->getFrame() +
		m_pTransportPosition->getFrameOffsetTempo() );

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG(QString( "[update] nNewFrame: %1, m_pTransportPosition->getFrame() (old): %2, m_pTransportPosition->getFrameOffsetTempo(): %3, fNewTick: %4, m_pTransportPosition->getDoubleTick() (old): %5, m_pTransportPosition->getTickOffsetSongSize() : %6, tick offset (without rounding): %7, fNewSongSizeInTicks: %8, fRepetitions: %9, fNewStrippedTick: %10, nNewPatternStartTick: %11")
			.arg( nNewFrame )
			.arg( m_pTransportPosition->getFrame() )
			.arg( m_pTransportPosition->getFrameOffsetTempo() )
			.arg( fNewTick, 0, 'g', 30 )
			.arg( m_pTransportPosition->getDoubleTick(), 0, 'g', 30 )
			.arg( m_pTransportPosition->getTickOffsetSongSize(), 0, 'g', 30 )
			.arg( fNewTick - m_pTransportPosition->getDoubleTick(), 0, 'g', 30 )
			.arg( fNewSongSizeInTicks, 0, 'g', 30 )
			.arg( fRepetitions, 0, 'f' )
			.arg( fNewStrippedTick, 0, 'f' )
			.arg( nNewPatternStartTick )
			);
#endif

	const auto fOldTickSize = m_pTransportPosition->getTickSize();
	updateTransportPosition( fNewTick, nNewFrame, m_pTransportPosition, trigger );
	
    // Ensure the tick offset is calculated as well (we do not expect
    // the tempo to change hence the following call is most likely not
    // executed during updateTransportPosition()).
#if defined(WIN32) and !defined(WIN64)
	// For some reason two identical numbers (according to their
	// values when printing them) are not equal to each other in 32bit
	// Windows. Course graining the tick change in here will do no
	// harm except of for preventing tiny tempo changes. Integer value
	// changes should not be affected.
	if ( std::abs( m_pTransportPosition->getTickSize() - fOldTickSize ) < 1e-2 ) {
#else
	if ( fOldTickSize == m_pTransportPosition->getTickSize() ) {
#endif
		calculateTransportOffsetOnBpmChange( m_pTransportPosition );
	}
	
	// Updating the queuing position by the same offset to keep them
	// approximately in sync.
	const double fNewTickQueuing = m_pQueuingPosition->getDoubleTick() +
		fTickOffset;
	const long long nNewFrameQueuing = TransportPosition::computeFrameFromTick(
		fNewTickQueuing, &m_pQueuingPosition->m_fTickMismatch );
	// Use offsets calculated above.
	m_pQueuingPosition->set( m_pTransportPosition );
	updateTransportPosition( fNewTickQueuing, nNewFrameQueuing,
							 m_pQueuingPosition, trigger );

	updatePlayingPatterns( trigger );
	
#ifdef H2CORE_HAVE_DEBUG
	if ( nOldColumn != m_pTransportPosition->getColumn() && ! bEmptySong &&
		 nOldColumn != -1 && m_pTransportPosition->getColumn() != -1 ) {
		AE_ERRORLOG( QString( "[nColumn mismatch] old: %1, new: %2" )
				  .arg( nOldColumn )
				  .arg( m_pTransportPosition->getColumn() ) );
	}
#endif

	if ( m_pQueuingPosition->getColumn() == -1 &&
		pSong->getLoopMode() != Song::LoopMode::Enabled ) {
		endOfSongReached();
		return;
	}

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[After] fNewTick: %1, fRepetitions: %2, m_fSongSizeInTicks: %3, fNewSongSizeInTicks: %4, transport: %5, queuing: %6" )
				.arg( fNewTick, 0, 'g', 30 )
				.arg( fRepetitions, 0, 'f' )
				.arg( m_fSongSizeInTicks )
				.arg( fNewSongSizeInTicks )
				.arg( m_pTransportPosition->toQString( "", true ) )
				.arg( m_pQueuingPosition->toQString( "", true ) )
				);
#endif

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent( Event::Type::SongSizeChanged, 0 );
	}
}

void AudioEngine::removePlayingPattern( std::shared_ptr<Pattern> pPattern ) {
	auto removePattern = [&]( std::shared_ptr<TransportPosition> pPos ) {
		auto pPlayingPatterns = pPos->getPlayingPatterns();
		
		for ( int ii = 0; ii < pPlayingPatterns->size(); ++ii ) {
			if ( pPlayingPatterns->get( ii ) == pPattern ) {
				pPlayingPatterns->del( ii );
				break;
			}
		}
	};

	removePattern( m_pTransportPosition );
	removePattern( m_pQueuingPosition );
}

void AudioEngine::updatePlayingPatterns( Event::Trigger trigger ) {
	updatePlayingPatternsPos( m_pTransportPosition, trigger );
	updatePlayingPatternsPos( m_pQueuingPosition, trigger );
}
	
void AudioEngine::updatePlayingPatternsPos( std::shared_ptr<TransportPosition> pPos,
											Event::Trigger trigger ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPlayingPatterns = pPos->getPlayingPatterns();

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[pre] mode: %1%2, %3" )
				 .arg( Song::ModeToQString( pHydrogen->getMode() ) )
				 .arg( pHydrogen->getMode() == Song::Mode::Pattern ?
					   ":" + Song::PatternModeToQString( pHydrogen->getPatternMode() ) :
					   "" )
				 .arg( pPos->toQString() ) );
#endif

	if ( pSong == nullptr ) {
		pPlayingPatterns->clear();
		pPos->setPatternSize( 4 * H2Core::nTicksPerQuarter );
		return;
	}

	if ( pHydrogen->getMode() == Song::Mode::Song ) {

		const auto nPrevPatternNumber = pPlayingPatterns->size();

		pPlayingPatterns->clear();

		if ( pSong->getPatternGroupVector()->size() == 0 ) {
			// No patterns in current song.

			pPos->setPatternSize( 4 * H2Core::nTicksPerQuarter );

			if ( pPos == m_pTransportPosition && nPrevPatternNumber > 0 &&
				 trigger != Event::Trigger::Suppress ) {
				EventQueue::get_instance()->pushEvent( Event::Type::PlayingPatternsChanged, 0 );
			}
			return;
		}

		auto nColumn = std::max( pPos->getColumn(), 0 );
		if ( nColumn >= pSong->getPatternGroupVector()->size() ) {
			AE_ERRORLOG( QString( "Provided column [%1] exceeds allowed range [0,%2]. Using 0 as fallback." )
						 .arg( nColumn )
						 .arg( pSong->getPatternGroupVector()->size() - 1 ) );
			nColumn = 0;
		}

		for ( const auto& ppattern : *( *( pSong->getPatternGroupVector() ) )[ nColumn ] ) {
			if ( ppattern != nullptr ) {
				pPlayingPatterns->add( ppattern, true );
			}
		}

		// GUI does not care about the internals of the audio engine
		// and just moves along the transport position.
		// We omit the event when passing from one empty column to the
		// next.
		if ( pPos == m_pTransportPosition &&
			 trigger != Event::Trigger::Suppress &&
			 ( nPrevPatternNumber != 0 || pPlayingPatterns->size() != 0 ) ) {
			EventQueue::get_instance()->pushEvent( Event::Type::PlayingPatternsChanged, 0 );
		}
	}
	else if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
		
		auto pSelectedPattern =
			pSong->getPatternList()->get( pHydrogen->getSelectedPatternNumber() );

		if ( pSelectedPattern != nullptr &&
			 ! ( pPlayingPatterns->size() == 1 &&
				 pPlayingPatterns->get( 0 ) == pSelectedPattern ) ) {
			pPlayingPatterns->clear();
			pPlayingPatterns->add( pSelectedPattern, true );

			// GUI does not care about the internals of the audio
			// engine and just moves along the transport position.
			if ( pPos == m_pTransportPosition &&
				 trigger != Event::Trigger::Suppress ) {
				EventQueue::get_instance()->pushEvent( Event::Type::PlayingPatternsChanged, 0 );
			}
		}
	}
	else if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {

		auto pNextPatterns = pPos->getNextPatterns();
		
		if ( pNextPatterns->size() > 0 ) {
			for ( const auto& ppattern : *pNextPatterns ) {
				if ( ppattern == nullptr ) {
					continue;
				}

				if ( ( pPlayingPatterns->del( ppattern ) ) == nullptr ) {
					// pPattern was not present yet. It will
					// be added
					pPlayingPatterns->add( ppattern, true );
				} else {
					// pPattern was already present. It will
					// be deleted.
					ppattern->removeFlattenedVirtualPatterns( pPlayingPatterns );
				}

				// GUI does not care about the internals of the audio
				// engine and just moves along the transport position.
				if ( pPos == m_pTransportPosition &&
					 trigger != Event::Trigger::Suppress ) {
					EventQueue::get_instance()->pushEvent( Event::Type::PlayingPatternsChanged, 0 );
				}
			}
			pNextPatterns->clear();
		}
	}

	if ( pPlayingPatterns->size() > 0 ) {
		// No virtual pattern resolution in here
		pPos->setPatternSize( pPlayingPatterns->longestPatternLength( false ) );
	} else {
		pPos->setPatternSize( 4 * H2Core::nTicksPerQuarter );
	}

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "[post] mode: %1%2, %3" )
				 .arg( Song::ModeToQString( pHydrogen->getMode() ) )
				 .arg( pHydrogen->getMode() == Song::Mode::Pattern ?
					   ":" + Song::PatternModeToQString( pHydrogen->getPatternMode() ) :
					   "" )
				 .arg( pPos->toQString() ) );
#endif
	
}

void AudioEngine::toggleNextPattern( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		return;
	}
	
	if ( m_pTransportPosition->getNextPatterns()->del( pPattern ) == nullptr ) {
		m_pTransportPosition->getNextPatterns()->add( pPattern );
	}
	if ( m_pQueuingPosition->getNextPatterns()->del( pPattern ) == nullptr ) {
		m_pQueuingPosition->getNextPatterns()->add( pPattern );
	}
}

void AudioEngine::clearNextPatterns() {
	m_pTransportPosition->getNextPatterns()->clear();
	m_pQueuingPosition->getNextPatterns()->clear();
}

void AudioEngine::flushAndAddNextPattern( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();

	bool bAlreadyPlaying = false;
	
	// Note: we will not perform a bound check on the provided pattern
	// number. This way the user can use the SELECT_ONLY_NEXT_PATTERN
	// MIDI or OSC command to flush all playing patterns.
	auto pRequestedPattern = pPatternList->get( nPatternNumber );

	auto flushAndAddNext = [&]( std::shared_ptr<TransportPosition> pPos ) {

		auto pNextPatterns = pPos->getNextPatterns();
		auto pPlayingPatterns = pPos->getPlayingPatterns();
		
		pNextPatterns->clear();
		for ( int ii = 0; ii < pPlayingPatterns->size(); ++ii ) {

			auto pPlayingPattern = pPlayingPatterns->get( ii );
			if ( pPlayingPattern != pRequestedPattern ) {
				pNextPatterns->add( pPlayingPattern );
			}
			else if ( pRequestedPattern != nullptr ) {
				bAlreadyPlaying = true;
			}
		}
	
		// Appending the requested pattern.
		if ( ! bAlreadyPlaying && pRequestedPattern != nullptr ) {
			pNextPatterns->add( pRequestedPattern );
		}
	};

	flushAndAddNext( m_pTransportPosition );
	flushAndAddNext( m_pQueuingPosition );
}

void AudioEngine::updateVirtualPatterns() {

	if ( Hydrogen::get_instance()->getPatternMode() == Song::PatternMode::Stacked ) {
		auto copyPlayingPatterns = [&]( std::shared_ptr<TransportPosition> pPos ) {
			auto pPlayingPatterns = pPos->getPlayingPatterns();
			auto pNextPatterns = pPos->getNextPatterns();

			for ( const auto& ppPattern : *pPlayingPatterns ) {
				pNextPatterns->add( ppPattern );
			}
		};
		copyPlayingPatterns( m_pTransportPosition );
		copyPlayingPatterns( m_pQueuingPosition );
	}

	m_pTransportPosition->getPlayingPatterns()->clear();
	m_pQueuingPosition->getPlayingPatterns()->clear();

	updatePlayingPatterns( Event::Trigger::Default );
	updateSongSize();
}

void AudioEngine::handleTimelineChange() {

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "before:\n%1\n%2" )
			 .arg( m_pTransportPosition->toQString() )
			 .arg( m_pQueuingPosition->toQString() ) );
#endif

	const auto fOldTickSize = m_pTransportPosition->getTickSize();
	updateBpmAndTickSize( m_pTransportPosition );
	updateBpmAndTickSize( m_pQueuingPosition );

#if defined(WIN32) and !defined(WIN64)
	// For some reason two identical numbers (according to their
	// values when printing them) are not equal to each other in 32bit
	// Windows. Course graining the tick change in here will do no
	// harm except of for preventing tiny tempo changes. Integer value
	// changes should not be affected.
	if ( std::abs( m_pTransportPosition->getTickSize() - fOldTickSize ) < 1e-2 ) {
#else
	if ( fOldTickSize == m_pTransportPosition->getTickSize() ) {
#endif
		// As tempo did not change during the Timeline activation, no
		// update of the offsets took place. This, however, is not
		// good, as it makes a significant difference to be located at
		// tick X with e.g. 120 bpm tempo and at X with a 120 bpm
		// tempo marker active but several others located prior to X. 
		calculateTransportOffsetOnBpmChange( m_pTransportPosition );
	}

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "after:\n%1\n%2" )
			 .arg( m_pTransportPosition->toQString() )
			 .arg( m_pQueuingPosition->toQString() ) );
#endif
}

void AudioEngine::handleTempoChange() {
	if ( m_songNoteQueue.size() != 0 ) {

		std::vector<std::shared_ptr<Note>> notes;
		for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
			notes.push_back( m_songNoteQueue.top() );
		}

		if ( notes.size() > 0 ) {
			for ( auto nnote : notes ) {
				nnote->computeNoteStart();
				m_songNoteQueue.push( nnote );
			}
		}

		notes.clear();
		while ( m_midiNoteQueue.size() > 0 ) {
			notes.push_back( m_midiNoteQueue[ 0 ] );
			m_midiNoteQueue.pop_front();
		}

		if ( notes.size() > 0 ) {
			for ( auto nnote : notes ) {
				nnote->computeNoteStart();
				m_midiNoteQueue.push_back( nnote );
			}
		}
	}
	
	getSampler()->handleTimelineOrTempoChange();
}

void AudioEngine::handleSongSizeChange() {
	if ( m_songNoteQueue.size() != 0 ) {

		std::vector<std::shared_ptr<Note>> notes;
		for ( ; ! m_songNoteQueue.empty(); m_songNoteQueue.pop() ) {
			notes.push_back( m_songNoteQueue.top() );
		}

		const long nTickOffset =
			static_cast<long>(std::floor(m_pTransportPosition->getTickOffsetSongSize()));

		if ( notes.size() > 0 ) {
			for ( auto ppNote : notes ) {

#if AUDIO_ENGINE_DEBUG
				AE_DEBUGLOG( QString( "[song queue] name: %1, pos: %2 -> %3, tick offset: %4, tick offset floored: %5" )
							 .arg( ppNote->getInstrument() != nullptr ?
								   ppNote->getInstrument()->getName() :
								   "nullptr" )
						  .arg( ppNote->getPosition() )
						  .arg( std::max( ppNote->getPosition() + nTickOffset,
										  static_cast<long>(0) ) )
						  .arg( m_pTransportPosition->getTickOffsetSongSize(), 0, 'f' )
						  .arg( nTickOffset ) );
#endif
	
				ppNote->setPosition( std::max( ppNote->getPosition() + nTickOffset,
											   static_cast<long>(0) ) );
				ppNote->computeNoteStart();
				m_songNoteQueue.push( ppNote );
			}
		}

		notes.clear();
		while ( m_midiNoteQueue.size() > 0 ) {
			notes.push_back( m_midiNoteQueue[ 0 ] );
			m_midiNoteQueue.pop_front();
		}

		if ( notes.size() > 0 ) {
			for ( auto ppNote : notes ) {

#if AUDIO_ENGINE_DEBUG
				AE_DEBUGLOG( QString( "[midi queue] name: %1, pos: %2 -> %3, tick offset: %4, tick offset floored: %5" )
							 .arg( ppNote->getInstrument() != nullptr ?
								   ppNote->getInstrument()->getName() :
								   "nullptr" )
						  .arg( ppNote->getPosition() )
						  .arg( std::max( ppNote->getPosition() + nTickOffset,
										  static_cast<long>(0) ) )
						  .arg( m_pTransportPosition->getTickOffsetSongSize(), 0, 'f' )
						  .arg( nTickOffset ) );
#endif
		
				ppNote->setPosition( std::max( ppNote->getPosition() + nTickOffset,
											   static_cast<long>(0) ) );
				ppNote->computeNoteStart();
				m_midiNoteQueue.push_back( ppNote );
			}
		}
	}
	
	getSampler()->handleSongSizeChange();
}

long long AudioEngine::computeTickInterval( double* fTickStart, double* fTickEnd, unsigned nIntervalLengthInFrames ) {

	const auto pHydrogen = Hydrogen::get_instance();
	auto pPos = m_pTransportPosition;

	long long nFrameStart, nFrameEnd;

	if ( getState() == State::Ready ) {
		// In case the playback is stopped we pretend it is still
		// rolling using the realtime ticks while disregarding tempo
		// changes in the Timeline. This is important as we want to
		// continue playing back notes in the sampler and process
		// realtime events, by e.g. MIDI or Hydrogen's virtual
		// keyboard.
		nFrameStart = getRealtimeFrame();
	} else {
		// Enters here when either transport is rolling or the unit
		// tests are run.
		nFrameStart = pPos->getFrame();
	}
	
	long long nLeadLagFactor = getLeadLagInFrames( pPos->getDoubleTick() );

	// Timeline disabled: 
	// Due to rounding errors in tick<->frame conversions the leadlag
	// factor in frames can differ by +/-1 even if the corresponding
	// lead lag in ticks is exactly the same.
	//
	// Timeline enabled:
	// With Tempo markers being present the lookahead is not constant
	// anymore. As it determines the position X frames and Y ticks
	// into the future, imagine it being process cycle after cycle
	// moved across a marker. The amount of frames covered by the
	// first and the second tick size will always change and so does
	// the resulting lookahead.
	//
	// This, however, would result in holes and overlaps in tick
	// coverage for the queuing position and note enqueuing in
	// updateNoteQueue(). That's why we stick to a single lead lag
	// factor invalidated each time the tempo of the song does change.
    if ( pPos->getLastLeadLagFactor() != 0 ) {
		if ( pPos->getLastLeadLagFactor() != nLeadLagFactor ) {
			nLeadLagFactor = pPos->getLastLeadLagFactor();
		}
	} else {
		pPos->setLastLeadLagFactor( nLeadLagFactor );
	}
	
	const long long nLookahead = nLeadLagFactor +
		AudioEngine::nMaxTimeHumanize + 1;

	nFrameEnd = nFrameStart + nLookahead +
		static_cast<long long>(nIntervalLengthInFrames);

	// Checking whether transport and queuing position are identical
	// is not enough in here. For specific audio driver parameters and
	// very tiny buffersizes used by drivers with dynamic buffer sizes
	// they both can be identical.
	if ( m_bLookaheadApplied ) {
		nFrameStart += nLookahead;
	}

	*fTickStart = ( TransportPosition::computeTickFromFrame( nFrameStart ) +
					pPos->getTickMismatch() ) - pPos->getTickOffsetQueuing() ;
	*fTickEnd = TransportPosition::computeTickFromFrame( nFrameEnd ) -
		pPos->getTickOffsetQueuing();

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "nFrame: [%1,%2], fTick: [%3, %4], fTick (without offset): [%5,%6], m_pTransportPosition->getTickOffsetQueuing(): %7, nLookahead: %8, nIntervalLengthInFrames: %9, m_pTransportPosition: %10, m_pQueuingPosition: %11,_bLookaheadApplied: %12" )
			 .arg( nFrameStart )
			 .arg( nFrameEnd )
			 .arg( *fTickStart, 0, 'f' )
			 .arg( *fTickEnd, 0, 'f' )
			 .arg( TransportPosition::computeTickFromFrame( nFrameStart ), 0, 'f' )
			 .arg( TransportPosition::computeTickFromFrame( nFrameEnd ), 0, 'f' )
			 .arg( pPos->getTickOffsetQueuing(), 0, 'f' )
			 .arg( nLookahead )
			 .arg( nIntervalLengthInFrames )
			 .arg( pPos->toQString() )
			 .arg( m_pQueuingPosition->toQString() )
			 .arg( m_bLookaheadApplied )
			 );
#endif

	return nLeadLagFactor;
}

	// Ideally we just floor the provided tick. When relocating to
	// a specific tick, it's converted counterpart is stored as the
	// transport position in frames, which is then used to calculate
	// the tick start again. These conversions back and forth can
	// introduce rounding error that get larger for larger tick
	// numbers and could result in a computed start tick of
	// 86753.999999934 when transport was relocated to 86754. As we do
	// not want to cover notes prior to our current transport
	// position, we have to account for such rounding errors.
double AudioEngine::coarseGrainTick( double fTick ) {
		if ( std::ceil( fTick ) - fTick > 0 &&
			 std::ceil( fTick ) - fTick < 1E-6 ) {
			return std::floor( fTick ) + 1;
		}
		else {
			return std::floor( fTick );
		}
	}

void AudioEngine::updateNoteQueue( unsigned nIntervalLengthInFrames )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	double fTickStartComp, fTickEndComp;

	long long nLeadLagFactor =
		computeTickInterval( &fTickStartComp, &fTickEndComp, nIntervalLengthInFrames );

	// MIDI events get put into the `m_songNoteQueue` as well.
	while ( m_midiNoteQueue.size() > 0 ) {
		auto pNote = m_midiNoteQueue[0];
		if ( pNote == nullptr || pNote->getInstrument() == nullptr ) {
			m_midiNoteQueue.pop_front();
		}
		else {

			if ( pNote->getPosition() >
				 static_cast<int>(coarseGrainTick( fTickEndComp )) ) {
				break;
			}

			m_midiNoteQueue.pop_front();
			pNote->getInstrument()->enqueue( pNote );
			pNote->computeNoteStart();
			pNote->humanize();
			m_songNoteQueue.push( pNote );
		}
	}

	if ( getState() != State::Playing && getState() != State::Testing ) {
		return;
	}

	AutomationPath* pAutomationPath = pSong->getVelocityAutomationPath();

	// computeTickInterval() is always called regardless whether
	// transport is rolling or not. But we only mark the lookahead
	// consumed if the associated tick interval was actually traversed
	// by the queuing position.
	if ( ! m_bLookaheadApplied ) {
		m_bLookaheadApplied = true;
	}
	
	const long nTickStart = static_cast<long>(coarseGrainTick( fTickStartComp ));
	const long nTickEnd = static_cast<long>(coarseGrainTick( fTickEndComp ));

	// Only store the last tick interval end if transport is
	// rolling. Else the realtime frame processing will mess things
	// up.
	m_fLastTickEnd = fTickEndComp;

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "tick interval (floor): [%1,%2], tick interval (computed): [%3,%4], nLeadLagFactor: %5, m_fSongSizeInTicks: %6, m_pTransportPosition: %7, m_pQueuingPosition: %8")
				.arg( nTickStart ).arg( nTickEnd )
				.arg( fTickStartComp, 0, 'f' ).arg( fTickEndComp, 0, 'f' )
				.arg( nLeadLagFactor )
				.arg( m_fSongSizeInTicks, 0, 'f' )
				.arg( m_pTransportPosition->toQString() )
				.arg( m_pQueuingPosition->toQString() ) );
#endif

	// We loop over integer ticks to ensure that all notes encountered
	// between two iterations belong to the same pattern.
	for ( long nnTick = nTickStart; nnTick < nTickEnd; ++nnTick ) {

		//////////////////////////////////////////////////////////////
		// Update queuing position and playing patterns.
		if ( pHydrogen->getMode() == Song::Mode::Song ) {

			const long nPreviousPosition = m_pQueuingPosition->getPatternStartTick() +
				m_pQueuingPosition->getPatternTickPosition();

			const long long nNewFrame = TransportPosition::computeFrameFromTick(
				static_cast<double>(nnTick),
				&m_pQueuingPosition->m_fTickMismatch );
			updateSongTransportPosition( static_cast<double>(nnTick),
										 nNewFrame, m_pQueuingPosition );

			if ( isEndOfSongReached( m_pQueuingPosition ) ) {
				// Queueing reached end of the song.
				return;
			}
		}
		else if ( pHydrogen->getMode() == Song::Mode::Pattern )	{

			const long long nNewFrame = TransportPosition::computeFrameFromTick(
				static_cast<double>(nnTick),
				&m_pQueuingPosition->m_fTickMismatch );
			updatePatternTransportPosition( static_cast<double>(nnTick),
											nNewFrame, m_pQueuingPosition );
		}
		
		//////////////////////////////////////////////////////////////
		// Metronome
		// Only trigger the metronome at a predefined rate.
		int nMetronomeTickPosition;
		if ( pSong->getPatternGroupVector()->size() == 0 ) {
			nMetronomeTickPosition = nnTick;
		} else {
			nMetronomeTickPosition = m_pQueuingPosition->getPatternTickPosition();
		}

		if ( nMetronomeTickPosition % H2Core::nTicksPerQuarter == 0 ) {
			float fPitch;
			float fVelocity;
			
			// Depending on whether the metronome beat will be issued
			// at the beginning or in the remainder of the pattern,
			// two different sounds and events will be used.
			if ( nMetronomeTickPosition == 0 ) {
				fPitch = 3;
				fVelocity = VELOCITY_MAX;
			} else {
				fPitch = PITCH_DEFAULT;
				fVelocity = VELOCITY_DEFAULT;
			}
			
			// Only trigger the sounds if the user enabled the
			// metronome. 
			if ( Preferences::get_instance()->m_bUseMetronome ) {
				auto pMetronomeNote = std::make_shared<Note>(
					m_pMetronomeInstrument,
					nnTick,
					fVelocity,
					PAN_DEFAULT, // pan
					LENGTH_ENTIRE_SAMPLE,
					fPitch );
				m_pMetronomeInstrument->enqueue( pMetronomeNote );
				pMetronomeNote->computeNoteStart();
				m_songNoteQueue.push( pMetronomeNote );
			}
		}
			
		if ( pHydrogen->getMode() == Song::Mode::Song &&
			 pSong->getPatternGroupVector()->size() == 0 ) {
			// No patterns in song. We let transport roll in case
			// patterns will be added again and still use metronome.
			if ( Preferences::get_instance()->m_bUseMetronome ) {
				continue;
			} else {
				return;
			}
		}
		//////////////////////////////////////////////////////////////
		// Update the notes queue.
		//
		// Supporting ticks with float precision:
		// - make FOREACH_NOTE_CST_IT_BOUND loop over all notes
		// `(_it)->first >= (_bound) && (_it)->first < (_bound + 1)`
		// - add remainder of pNote->getPosition() % 1 when setting
		// nnTick as new position.
		//
		const auto pPlayingPatterns = m_pQueuingPosition->getPlayingPatterns();
		if ( pPlayingPatterns->size() != 0 ) {
			for ( auto nPat = 0; nPat < pPlayingPatterns->size(); ++nPat ) {
				auto pPattern = pPlayingPatterns->get( nPat );
				assert( pPattern != nullptr );
				Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->getNotes();

				// Loop over all notes at tick nPatternTickPosition
				// (associated tick is determined by Note::__position
				// at the time of insertion into the Pattern).
				FOREACH_NOTE_CST_IT_BOUND_LENGTH(notes, it,
												 m_pQueuingPosition->getPatternTickPosition(),
												 pPattern ) {
					auto pNote = it->second;
					if ( pNote != nullptr &&
						 pNote->getInstrument() != nullptr ) {
						auto pCopiedNote = std::make_shared<Note>( pNote );

						// Lead or Lag.
						// This property is set within the
						// NotePropertiesRuler and only applies to
						// notes picked up from patterns within
						// Hydrogen during transport.
						pCopiedNote->setHumanizeDelay(
							pCopiedNote->getHumanizeDelay() +
							static_cast<int>(
								static_cast<float>(pNote->getLeadLag()) *
								static_cast<float>(nLeadLagFactor) ));
						
						pCopiedNote->setPosition( nnTick );
						pCopiedNote->humanize();

					   /** Swing 16ths
						* delay the upbeat 16th-notes by a constant
						* (manual) offset.
						*
						* This must done _after_ setting the position
						* of the note.
						*/
						if ( ( ( m_pQueuingPosition->getPatternTickPosition() %
								 ( H2Core::nTicksPerQuarter / 4 ) ) == 0 ) &&
							 ( ( m_pQueuingPosition->getPatternTickPosition() %
								 ( H2Core::nTicksPerQuarter / 2 ) ) != 0 ) ) {
							pCopiedNote->swing();
						}
						
						// This must be done _after_ setting the
						// position, humanization, and swing.
						pCopiedNote->computeNoteStart();
						
						if ( pHydrogen->getMode() == Song::Mode::Song ) {
							const float fPos = static_cast<float>( m_pQueuingPosition->getColumn() ) +
								pCopiedNote->getPosition() % 192 / 192.f;
							pCopiedNote->setVelocity( pCopiedNote->getVelocity() *
													   pAutomationPath->get_value( fPos ) );
						}

						// Ensure the custom length of the note does not exceed
						// the length of the current pattern.
						if ( pCopiedNote->getLength() != LENGTH_ENTIRE_SAMPLE ) {
							pCopiedNote->setLength(
								std::min(
									static_cast<long>(pCopiedNote->getLength()),
									static_cast<long>(pPattern->getLength()) -
									m_pQueuingPosition->getPatternTickPosition() ) );
						}

#if AUDIO_ENGINE_DEBUG
						AE_DEBUGLOG( QString( "m_pQueuingPosition: %1, new note: %2" )
								  .arg( m_pQueuingPosition->toQString() )
								  .arg( pCopiedNote->toQString() ) );
#endif

						pCopiedNote->getInstrument()->enqueue( pCopiedNote );
						m_songNoteQueue.push( pCopiedNote );
					}
				}
			}
		}
	}

	return;
}

void AudioEngine::noteOn( std::shared_ptr<Note> pNote )
{
	if ( ! ( getState() == State::Playing ||
			 getState() == State::Ready ||
			 getState() == State::Testing ) ) {
		AE_ERRORLOG( QString( "Error the audio engine is not in State::Ready, State::Playing, or State::Testing but [%1]" )
					 .arg( static_cast<int>( getState() ) ) );
		return;
	}

	m_midiNoteQueue.push_back( pNote );
}

bool AudioEngine::compare_pNotes::operator()( std::shared_ptr<Note> pNote1,
											  std::shared_ptr<Note> pNote2 ) {
	return Note::compareStart( pNote1, pNote2 );
}

void AudioEngine::play() {
	
	assert( m_pAudioDriver );

#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->hasJackTransport() ) {
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
	if ( Hydrogen::get_instance()->hasJackTransport() ) {

#if AUDIO_ENGINE_DEBUG
		AE_DEBUGLOG( "Stopping engine via JACK server" );
#endif

		// Tell all other JACK clients to stop as well and wait for
		// the JACK server to give the signal.
		static_cast<JackAudioDriver*>( m_pAudioDriver )->stopTransport();
		return;
	}
#endif

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( "Mark audio engine to be stop" );
#endif
	
	setNextState( State::Ready );
}

double AudioEngine::getLeadLagInTicks() {
	return 5;
}

long long AudioEngine::getLeadLagInFrames( double fTick ) const {
	double fTmp;
	const long long nFrameStart =
		TransportPosition::computeFrameFromTick( fTick, &fTmp );
	const long long nFrameEnd =
		TransportPosition::computeFrameFromTick( fTick +
												 AudioEngine::getLeadLagInTicks(),
												 &fTmp );

#if AUDIO_ENGINE_DEBUG
	AE_DEBUGLOG( QString( "nFrameStart: %1, nFrameEnd: %2, diff: %3, fTick: %4" )
				.arg( nFrameStart ).arg( nFrameEnd )
				.arg( nFrameEnd - nFrameStart ).arg( fTick, 0, 'f' ) );
#endif

	return nFrameEnd - nFrameStart;
}

const std::shared_ptr<PatternList> AudioEngine::getPlayingPatterns() const {
	if ( m_pTransportPosition != nullptr ) {
		return m_pTransportPosition->getPlayingPatterns();
	}
	return nullptr;
}

const std::shared_ptr<PatternList> AudioEngine::getNextPatterns() const {
	if ( m_pTransportPosition != nullptr ) {
		return m_pTransportPosition->getNextPatterns();
	}
	return nullptr;
}

QString AudioEngine::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;

	std::stringstream threadIdStream;
		threadIdStream << m_LockingThread;

	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[AudioEngine]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_pSampler: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pSampler == nullptr ? "nullptr" :
						   m_pSampler->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_pAudioDriver: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pAudioDriver == nullptr ? "nullptr" :
						   m_pAudioDriver->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_pMidiDriver: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pMidiDriver == nullptr ? "nullptr" :
						   m_pMidiDriver->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_pMidiDriverOut: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pMidiDriverOut == nullptr ? "nullptr" :
						   m_pMidiDriverOut->toQString( sPrefix + s, bShort ) ) );
#ifdef H2CORE_HAVE_LADSPA
		sOutput.append( QString( "%1%2m_fFXPeak_L: [" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ii : m_fFXPeak_L ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( QString( "]\n%1%2m_fFXPeak_R: [" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ii : m_fFXPeak_R ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( QString( " ]\n" ) );
#endif
		sOutput.append( QString( "%1%2m_fMasterPeak_L: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fMasterPeak_L ) )
			.append( QString( "%1%2m_fMasterPeak_R: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fMasterPeak_R ) )
			.append( QString( "%1%2m_LockingThread: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( QString::fromStdString( threadIdStream.str() ) ) );
		sOutput.append( QString( "%1%2m_pLocker: " ).arg( sPrefix ).arg( s ) );
		if ( m_pLocker.file == nullptr || m_pLocker.function == nullptr ){
			sOutput.append( "was not locked yet\n" );
		}
		else {
			if ( m_pLocker.isLocked ) {
				sOutput.append( "is currently locked by: " );
			} else {
				sOutput.append( "was last locked by: " );
			}
			sOutput.append( QString( "[function: %1, line: %2, file: %3]\n" )
							.arg( m_pLocker.function ).arg( m_pLocker.line )
							.arg( m_pLocker.file ) );
		}
		sOutput.append( QString( "%1%2m_fProcessTime: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fProcessTime ) )
			.append( QString( "%1%2m_fMaxProcessTime: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fMaxProcessTime ) )
			.append( QString( "%1%2m_fLadspaTime: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fLadspaTime ) )
			.append( QString( "%1%2m_pTransportPosition:\n").arg( sPrefix ).arg( s ) );
		if ( m_pTransportPosition != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pTransportPosition->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_pQueuingPosition:\n").arg( sPrefix ).arg( s ) );
		if ( m_pQueuingPosition != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pQueuingPosition->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_fSongSizeInTicks: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fSongSizeInTicks, 0, 'f' ) )
			.append( QString( "%1%2m_nRealtimeFrame: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nRealtimeFrame ) )
			.append( QString( "%1%2m_state: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( StateToQString( m_state ) ) )
			.append( QString( "%1%2m_nextState: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( StateToQString( m_nextState ) ) )
			.append( QString( "%1%2m_songNoteQueue: length = %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_songNoteQueue.size() ) )
			.append( QString( "%1%2m_midiNoteQueue: [" ).arg( sPrefix ).arg( s ) );
		for ( const auto& nn : m_midiNoteQueue ) {
			if ( nn != nullptr ) {
				sOutput.append( nn->toQString( sPrefix + s, bShort ) ).append( "\n" );
			}
		}
		sOutput.append( QString( "]\n%1%2m_pMetronomeInstrument: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pMetronomeInstrument == nullptr ? "nullptr" :
						   m_pMetronomeInstrument->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_fNextBpm: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fNextBpm, 0, 'f' ) )
			.append( QString( "%1%2m_fLastTickEnd: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fLastTickEnd, 0, 'f' ) )
			.append( QString( "%1%2m_bLookaheadApplied: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bLookaheadApplied ) )
			.append( QString( "%1%2m_nLoopsDone: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nLoopsDone ) )
			.append( QString( "%1%2nMaxTimeHumanize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( AudioEngine::nMaxTimeHumanize ) )
			.append( QString( "%1%2fHumanizeVelocitySD: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( AudioEngine::fHumanizeVelocitySD ) )
			.append( QString( "%1%2fHumanizePitchSD: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( AudioEngine::fHumanizePitchSD ) )
			.append( QString( "%1%2fHumanizeTimingSD: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( AudioEngine::fHumanizeTimingSD ) );
	}
	else {
		sOutput = QString( "[AudioEngine] " )
			.append( QString( "m_pSampler: %1" )
					 .arg( m_pSampler == nullptr ? "nullptr" :
						   m_pSampler->toQString( "", bShort ) ) )
			.append( QString( ", m_pAudioDriver: %1" )
					 .arg( m_pAudioDriver == nullptr ? "nullptr" :
						   m_pAudioDriver->toQString( "", bShort ) ) )
			.append( QString( ", m_pMidiDriver: %1" )
					 .arg( m_pMidiDriver == nullptr ? "nullptr" :
						   m_pMidiDriver->toQString( "", bShort ) ) )
			.append( QString( ", m_pMidiDriverOut: %1" )
					 .arg( m_pMidiDriverOut == nullptr ? "nullptr" :
						   m_pMidiDriverOut->toQString( "", bShort ) ) );
#ifdef H2CORE_HAVE_LADSPA
		sOutput.append( ", m_fFXPeak_L: [" );
		for ( const auto& ii : m_fFXPeak_L ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( "], m_fFXPeak_R: [" );
		for ( const auto& ii : m_fFXPeak_R ) {
			sOutput.append( QString( " %1" ).arg( ii ) );
		}
		sOutput.append( "]" );
#endif
		sOutput.append( QString( ", m_fMasterPeak_L: %1" )
					 .arg( m_fMasterPeak_L ) )
			.append( QString( ", m_fMasterPeak_R: %1" )
					 .arg( m_fMasterPeak_R ) )
			.append( QString( ", m_LockingThread: %1" )
					 .arg( QString::fromStdString( threadIdStream.str() ) ) );
		sOutput.append( ", m_pLocker: " );
		if ( m_pLocker.file == nullptr || m_pLocker.function == nullptr ){
			sOutput.append( "was not locked yet" );
		}
		else {
			if ( m_pLocker.isLocked ) {
				sOutput.append( "is currently locked by: " );
			} else {
				sOutput.append( "was last locked by: " );
			}
			sOutput.append( QString( "[function: %1, line: %2, file: %3]" )
							.arg( m_pLocker.function ).arg( m_pLocker.line )
							.arg( m_pLocker.file ) );
		}
		sOutput.append( QString( ", m_fProcessTime: %1" )
					 .arg( m_fProcessTime ) )
			.append( QString( ", m_fMaxProcessTime: %1" )
					 .arg( m_fMaxProcessTime ) )
			.append( QString( ", m_fLadspaTime: %1" )
					 .arg( m_fLadspaTime ) )
			.append( ", m_pTransportPosition: ");
		if ( m_pTransportPosition != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pTransportPosition->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( "nullptr" );
		}
		sOutput.append( ", m_pQueuingPosition: " );
		if ( m_pQueuingPosition != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pQueuingPosition->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( "nullptr" );
		}
		sOutput.append( QString( ", m_fSongSizeInTicks: %1" )
					 .arg( m_fSongSizeInTicks, 0, 'f' ) )
			.append( QString( ", m_nRealtimeFrame: %1" )
					 .arg( m_nRealtimeFrame ) )
			.append( QString( ", m_state: %1" )
					 .arg( StateToQString( m_state ) ) )
			.append( QString( ", m_nextState: %1" )
					 .arg( StateToQString( m_nextState ) ) )
			.append( QString( ", m_songNoteQueue: length = %1" )
					 .arg( m_songNoteQueue.size() ) )
			.append( ", m_midiNoteQueue: [" );
		for ( const auto& nn : m_midiNoteQueue ) {
			if ( nn != nullptr ) {
				sOutput.append( QString( "[%1], " ).arg( nn->prettyName() ) );
			}
		}
		sOutput.append( QString( "], m_pMetronomeInstrument: %1" )
					 .arg( m_pMetronomeInstrument == nullptr ? "nullptr" :
						   m_pMetronomeInstrument->toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", m_fNextBpm: %1" )
					 .arg( m_fNextBpm, 0, 'f' ) )
			.append( QString( ", m_fLastTickEnd: %1" )
					 .arg( m_fLastTickEnd, 0, 'f' ) )
			.append( QString( ", m_bLookaheadApplied: %1" )
					 .arg( m_bLookaheadApplied ) )
			.append( QString( ", m_nLoopsDone: %1" )
					 .arg( m_nLoopsDone ) )
			.append( QString( ", nMaxTimeHumanize: %1" )
					 .arg( AudioEngine::nMaxTimeHumanize ) )
			.append( QString( ", fHumanizeVelocitySD: %1" )
					 .arg( AudioEngine::fHumanizeVelocitySD ) )
			.append( QString( ", fHumanizePitchSD: %1" )
					 .arg( AudioEngine::fHumanizePitchSD ) )
			.append( QString( ", fHumanizeTimingSD: %1" )
					 .arg( AudioEngine::fHumanizeTimingSD ) );
	}
	
	return sOutput;
}

QString AudioEngine::StateToQString( const State& state ) {
	switch( state ) {
	case State::Uninitialized:
		return "Uninitialized";
	case State::Initialized:
		return "Initialized";
	case State::Prepared:
		return "Prepared";
	case State::Ready:
		return "Ready";
	case State::Playing:
		return "Playing";
	case State::Testing:
		return "Testing";
	default:
		return QString( "Unknown state [%1]" )
			.arg( static_cast<int>(state) );
	}
}

QString AudioEngine::getDriverNames() const {
	Preferences::AudioDriver audioDriver = Preferences::AudioDriver::Null;
	QString sMidiInputDriver( "unknown" );
	QString sMidiOutputDriver( "unknown" );

	if ( m_pAudioDriver == nullptr ) {
		audioDriver = Preferences::AudioDriver::None;
	}
	else if ( dynamic_cast<JackAudioDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::Jack;
	}
	else if ( dynamic_cast<PortAudioDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::PortAudio;
	}
	else if ( dynamic_cast<CoreAudioDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::CoreAudio;
	}
	else if ( dynamic_cast<PulseAudioDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::PulseAudio;
	}
	else if ( dynamic_cast<OssDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::Oss;
	}
	else if ( dynamic_cast<AlsaAudioDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::Alsa;
	}
	else if ( dynamic_cast<FakeDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::Fake;
	}
	else if ( dynamic_cast<NullDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::Null;
	}
	else if ( dynamic_cast<DiskWriterDriver*>(m_pAudioDriver) != nullptr ) {
		audioDriver = Preferences::AudioDriver::Disk;
	}
	
	if ( m_pMidiDriver == nullptr ) {
		sMidiInputDriver = "nullptr";
#ifdef H2CORE_HAVE_ALSA
	} else if ( dynamic_cast<AlsaMidiDriver*>(m_pMidiDriver) != nullptr ) {
		sMidiInputDriver = "ALSA";
#endif
#ifdef H2CORE_HAVE_PORTMIDI
	} else if ( dynamic_cast<PortMidiDriver*>(m_pMidiDriver) != nullptr ) {
		sMidiInputDriver = "PortMidi";
#endif
#ifdef H2CORE_HAVE_COREMIDI
	} else if ( dynamic_cast<CoreMidiDriver*>(m_pMidiDriver) != nullptr ) {
		sMidiInputDriver = "CoreMidi";
#endif
#ifdef H2CORE_HAVE_JACK
	} else if ( dynamic_cast<JackMidiDriver*>(m_pMidiDriver) != nullptr ) {
		sMidiInputDriver = "JACK";
#endif
	}
		
	if ( m_pMidiDriverOut == nullptr ) {
		sMidiOutputDriver = "nullptr";
#ifdef H2CORE_HAVE_ALSA
	} else if ( dynamic_cast<AlsaMidiDriver*>(m_pMidiDriverOut) != nullptr ) {
		sMidiOutputDriver = "ALSA";
#endif
#ifdef H2CORE_HAVE_PORTMIDI
	} else if ( dynamic_cast<PortMidiDriver*>(m_pMidiDriverOut) != nullptr ) {
		sMidiOutputDriver = "PortMidi";
#endif
#ifdef H2CORE_HAVE_COREMIDI
	} else if ( dynamic_cast<CoreMidiDriver*>(m_pMidiDriverOut) != nullptr ) {
		sMidiOutputDriver = "CoreMidi";
#endif
#ifdef H2CORE_HAVE_JACK
	} else if ( dynamic_cast<JackMidiDriver*>(m_pMidiDriverOut) != nullptr ) {
		sMidiOutputDriver = "JACK";
#endif
	}
	
	auto res = QString( "%1|" )
		.arg( Preferences::audioDriverToQString( audioDriver ) );
	if ( sMidiInputDriver == sMidiOutputDriver ) {
		res.append( QString( "%1" ).arg( sMidiInputDriver ) );
	} else {
		res.append( QString( "in: %1;out: %2" ).arg( sMidiInputDriver )
					.arg( sMidiOutputDriver ) );
	}

	return std::move( res );
}


void AudioEngine::assertLocked( const QString& sClass, const char* sFunction,
								const QString& sMsg ) {
#ifndef NDEBUG
	if ( m_LockingThread != std::this_thread::get_id() ) {
		// Is there a more convenient way to convert the thread id to QSTring?
		std::stringstream tmpStream;
		tmpStream << std::this_thread::get_id();
		ERRORLOG( QString( "[thread id: %1] [%2::%3] %4" )
				  .arg( QString::fromStdString( tmpStream.str() ) )
				  .arg( sClass ).arg( sFunction ).arg( sMsg ) );
		__logger->flush();
		assert( false );
	}
#endif
}

void AudioEngineLocking::assertAudioEngineLocked( const QString& sClass,
												  const char* sFunction,
												  const QString& sMsg ) const
{
#ifndef NDEBUG
		if ( m_bNeedsLock ) {
			H2Core::Hydrogen::get_instance()->getAudioEngine()->
				assertLocked( sClass, sFunction, sMsg );
		}
#endif
}

}; // namespace H2Core
