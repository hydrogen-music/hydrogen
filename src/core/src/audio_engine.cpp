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

float AudioEngine::compute_tick_size(int sampleRate, float bpm, int resolution)
{
	float tickSize = sampleRate * 60.0 / bpm / resolution;
	
	return tickSize;
}


void AudioEngine::unlock()
{
	// Leave "__locker" dirty.
	__engine_mutex.unlock();
}


}; // namespace H2Core
