/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <hydrogen/sequencer/Sequencer.h>
#include <hydrogen/sampler/Sampler.h>

#include <hydrogen/hydrogen.h>	// TODO: remove this line as soon as possible

namespace H2Core {


AudioEngine* AudioEngine::__instance = NULL;



AudioEngine* AudioEngine::get_instance()
{
	if ( __instance == NULL ) {
		__instance = new AudioEngine();
	}
	return __instance;
}



AudioEngine::AudioEngine()
 : Object( "AudioEngine" )
 , __sampler( NULL )
 , __synth( NULL )
 , __locker( "" )
{
	INFOLOG( "INIT");

	pthread_mutex_init( &__engine_mutex, NULL );

#ifdef LADSPA_SUPPORT
	Effects::getInstance();
#endif

	Sequencer::getInstance();
}



AudioEngine::~AudioEngine()
{
	INFOLOG( "DESTROY");
#ifdef LADSPA_SUPPORT
	delete Effects::getInstance();
#endif

	delete Sequencer::getInstance();
	delete __sampler;
	delete __synth;
}



Sampler* AudioEngine::get_sampler()
{
	if ( !__sampler ) {
		__sampler = new Sampler();
	}

	return __sampler;
}




Synth* AudioEngine::get_synth()
{
	if ( !__synth ) {
		__synth = new Synth();
	}

	return __synth;
}




void AudioEngine::lock( const std::string& locker )
{
	int res = pthread_mutex_trylock( &__engine_mutex );
	if (res != 0) {
		WARNINGLOG( "trylock != 0. Lock in " + __locker + ". I'll wait for the mutex." );
		pthread_mutex_lock(&__engine_mutex);
	}

	__locker = locker;
}



bool AudioEngine::try_lock( const std::string& locker )
{
	int res = pthread_mutex_trylock( &__engine_mutex );
	if (res != 0) {
		WARNINGLOG( "trylock != 0. Lock in " + __locker );
		return false;
	}
	__locker = locker;

	return true;
}



void AudioEngine::unlock()
{
	pthread_mutex_unlock(&__engine_mutex);
}


}; // namespace H2Core
