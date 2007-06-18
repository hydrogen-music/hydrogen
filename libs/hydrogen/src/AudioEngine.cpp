/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/AudioEngine.h>

#include <hydrogen/fx/Effects.h>
#include <hydrogen/sequencer/Sequencer.h>
#include <hydrogen/sampler/Sampler.h>

#include <hydrogen/Hydrogen.h>	// TODO: remove this line as soon as possible

namespace H2Core {


AudioEngine* AudioEngine::m_pInstance = NULL;



AudioEngine* AudioEngine::getInstance()
{
	if ( m_pInstance == NULL ) {
		m_pInstance = new AudioEngine();
	}
	return m_pInstance;
}



AudioEngine::AudioEngine()
 : Object( "AudioEngine" )
 , m_pSampler( NULL )
 , m_pSynth( NULL )
 , m_sLocker( "" )
{
	INFOLOG( "INIT");

	pthread_mutex_init( &m_engineLock_mutex, NULL );

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
	delete m_pSampler;
	delete m_pSynth;
}



Sampler* AudioEngine::getSampler()
{
	if ( !m_pSampler ) {
		m_pSampler = new Sampler();
	}

	return m_pSampler;
}




Synth* AudioEngine::getSynth()
{
	if ( !m_pSynth ) {
		m_pSynth = new Synth();
	}

	return m_pSynth;
}




void AudioEngine::lock( const std::string& sLocker )
{
	int res = pthread_mutex_trylock( &m_engineLock_mutex );
	if (res != 0) {
		WARNINGLOG( "trylock != 0. Lock in " + m_sLocker + ". I'll wait for the mutex." );
		pthread_mutex_lock(&m_engineLock_mutex);
	}

	m_sLocker = sLocker;
}



bool AudioEngine::tryLock( const std::string& sLocker )
{
	int res = pthread_mutex_trylock( &m_engineLock_mutex );
	if (res != 0) {
		WARNINGLOG( "trylock != 0. Lock in " + m_sLocker );
		return false;
	}
	m_sLocker = sLocker;

	return true;
}



void AudioEngine::unlock()
{
	pthread_mutex_unlock(&m_engineLock_mutex);
}


}; // namespace H2Core
