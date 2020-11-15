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

#include <hydrogen/audio_engine.h>

#include <hydrogen/fx/Effects.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/sampler/Sampler.h>

#include <hydrogen/hydrogen.h>	// TODO: remove this line as soon as possible
#include <hydrogen/Preferences.h>
#include <cassert>

namespace H2Core
{


AudioEngine* AudioEngine::__instance = nullptr;
const char* AudioEngine::__class_name = "AudioEngine";


void AudioEngine::create_instance()
{
	if( __instance == nullptr ) {
		__instance = new AudioEngine;
	}
}

AudioEngine::AudioEngine()
		: Object( __class_name )
		, __sampler( nullptr )
		, __synth( nullptr )
		, m_fElapsedTime( 0 )
{
	__instance = this;
	INFOLOG( "INIT" );

	__sampler = new Sampler;
	__synth = new Synth;

#ifdef H2CORE_HAVE_LADSPA
	Effects::create_instance();
#endif

}



AudioEngine::~AudioEngine()
{
	INFOLOG( "DESTROY" );
#ifdef H2CORE_HAVE_LADSPA
	delete Effects::get_instance();
#endif

//	delete Sequencer::get_instance();
	delete __sampler;
	delete __synth;
}



Sampler* AudioEngine::get_sampler()
{
	assert(__sampler);
	return __sampler;
}




Synth* AudioEngine::get_synth()
{
	assert(__synth);
	return __synth;
}

void AudioEngine::lock( const char* file, unsigned int line, const char* function )
{
	__engine_mutex.lock();
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
}



bool AudioEngine::try_lock( const char* file, unsigned int line, const char* function )
{
	bool res = __engine_mutex.try_lock();
	if ( !res ) {
		// Lock not obtained
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	return true;
}

bool AudioEngine::try_lock_for( std::chrono::microseconds duration, const char* file, unsigned int line, const char* function )
{
	bool res = __engine_mutex.try_lock_for( duration );
	if ( !res ) {
		// Lock not obtained
		WARNINGLOG( QString( "Lock timeout: lock timeout %1:%2%3, lock held by %4:%5:%6" )
					.arg( file ).arg( function ).arg( line )
					.arg( __locker.file ).arg( __locker.function ).arg( __locker.line ));
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	return true;
}

float AudioEngine::compute_tick_size( const int nSampleRate, const float fBpm, const int nResolution)
{
	float fTickSize = nSampleRate * 60.0 / fBpm / nResolution;
	
	return fTickSize;
}
	
void AudioEngine::calculateElapsedTime( const unsigned sampleRate, const unsigned long nFrame, const int nResolution ) {
	const auto pHydrogen = Hydrogen::get_instance();
	float fTickSize = pHydrogen->getAudioOutput()->m_transport.m_fTickSize;
	
	if ( fTickSize == 0 || sampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		m_fElapsedTime = 0;
		return;
	}
	
	if ( nFrame == 0 ) {
		m_fElapsedTime = 0;
		return;
	}
	
	const unsigned long currentTick = static_cast<unsigned long>(static_cast<float>(nFrame) / fTickSize );
	
	if ( !Preferences::get_instance()->getUseTimelineBpm() ){
		
		int nPatternStartInTicks;
		const int nCurrentPatternNumber = pHydrogen->getPosForTick( currentTick, &nPatternStartInTicks );
		long totalTicks = pHydrogen->getTickForPosition( nCurrentPatternNumber );
		
		// The code above calculates the number of ticks elapsed since
		// the beginning of the Song till the start of the current
		// pattern. The following line covers the remain ticks.
		totalTicks += static_cast<long>(currentTick - nPatternStartInTicks);
		
		m_fElapsedTime = static_cast<float>(totalTicks) * fTickSize / 
			static_cast<float>(sampleRate);
	} else {

		m_fElapsedTime = 0;

		int nPatternStartInTicks;
		long totalTicks;
		long previousTicks = 0;
		float fPreviousTickSize;

		auto tempoMarkers = pHydrogen->getTimeline()->getAllTempoMarkers();
		
		// TODO: how to handle the BPM before the first marker?
		fPreviousTickSize = compute_tick_size( static_cast<int>(sampleRate), 
											   tempoMarkers[0]->fBpm, nResolution );
		
		// For each BPM marker on the Timeline we will get the number
		// of ticks since the previous marker/beginning and convert
		// them into time using tick size corresponding to the tempo.
		for ( auto const& mmarker: tempoMarkers ){
			totalTicks = pHydrogen->getTickForPosition( mmarker->nBar );
			    
			if ( totalTicks < currentTick ) {
				m_fElapsedTime += static_cast<float>(totalTicks - previousTicks) * 
					fPreviousTickSize / static_cast<float>(sampleRate);
			} else {
				m_fElapsedTime += static_cast<float>(currentTick - previousTicks) * 
					fPreviousTickSize / static_cast<float>(sampleRate);
				return;
			}

			fPreviousTickSize = compute_tick_size( static_cast<int>(sampleRate), 
												   mmarker->fBpm, nResolution );
			previousTicks = totalTicks;
		}
		
		const int nCurrentPatternNumber = pHydrogen->getPosForTick( currentTick, &nPatternStartInTicks );
		totalTicks = pHydrogen->getTickForPosition( nCurrentPatternNumber );
		
		// The code above calculates the number of ticks elapsed since
		// the beginning of the Song till the start of the current
		// pattern. The following line covers the remain ticks.
		totalTicks += static_cast<long>(currentTick - nPatternStartInTicks);

		m_fElapsedTime += static_cast<float>(totalTicks - previousTicks) * fPreviousTickSize / 
			static_cast<float>(sampleRate);
	}
}

void AudioEngine::updateElapsedTime( const unsigned bufferSize, const unsigned sampleRate ){
	m_fElapsedTime += static_cast<float>(bufferSize) / static_cast<float>(sampleRate);
}

void AudioEngine::locate( const unsigned long nFrame ) {
	
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pDriver = pHydrogen->getAudioOutput();
	pDriver->locate( nFrame );
	AudioEngine::get_instance()->calculateElapsedTime( pDriver->getSampleRate(),
													   nFrame,
													   pHydrogen->getSong()->__resolution );
}

void AudioEngine::unlock()
{
	// Leave "__locker" dirty.
	__engine_mutex.unlock();
}


}; // namespace H2Core
