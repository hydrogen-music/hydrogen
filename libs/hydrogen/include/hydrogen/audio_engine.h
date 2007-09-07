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

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <pthread.h>
#include <string>

#include <hydrogen/Object.h>
#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/synth/Synth.h>

namespace H2Core
{

///
/// Audio Engine main class (Singleton).
///
class AudioEngine : public Object
{
public:
	static AudioEngine* get_instance();
	~AudioEngine();

	void lock( const std::string& locker );

	/// Try to lock the mutex. Return true on success.
	bool try_lock( const std::string& locker );
	void unlock();

	Sampler* get_sampler();
	Synth* get_synth();

private:
	static AudioEngine* __instance;

	Sampler* __sampler;
	Synth* __synth;

	/// Mutex for syncronized access to the Song object and the AudioEngine.
	pthread_mutex_t __engine_mutex;
	std::string __locker;

	AudioEngine();
};

};


#endif
