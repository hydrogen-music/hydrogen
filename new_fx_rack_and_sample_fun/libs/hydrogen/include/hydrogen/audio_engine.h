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

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <hydrogen/Object.h>
#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/synth/Synth.h>

#include <pthread.h>
#include <string>
#include <cassert>

#ifndef RIGHT_HERE
#define RIGHT_HERE __FILE__, __LINE__, __PRETTY_FUNCTION__
#endif

namespace H2Core
{

///
/// Audio Engine main class (Singleton).
///
class AudioEngine : public Object
{
public:
	static void create_instance();
	static AudioEngine* get_instance() { assert(__instance); return __instance; }
	~AudioEngine();

	/* Mutex locking and unlocking
	 *
	 * Easy usage:  Use the RIGHT_HERE macro like this...
	 *     AudioEngine::get_instance()->lock( RIGHT_HERE );
	 *
	 * More complex usage:  The parameters file and function
	 * need to be pointers to null-terminated strings that are
	 * persistent for the entire session.  This does *not*
	 * include the return value of std::string::c_str(), or
	 * QString::toLocal8Bit().data().
	 *
	 * Tracing the locks:  Enable the Logger::AELockTracing
	 * logging level.  When you do, there will be a performance
	 * penalty because the strings will be converted to a
	 * QString.  At the moment, you'll have to do that with
	 * your debugger.
	 *
	 * Notes: The order of the parameters match GCC's
	 * implementation of the assert() macros.
	 */
	void lock( const char* file, unsigned int line, const char* function );
	bool try_lock( const char* file, unsigned int line, const char* function ); /// Return true on success (locked).
	void unlock();

	Sampler* get_sampler();
	Synth* get_synth();

private:
	static AudioEngine* __instance;

	Sampler* __sampler;
	Synth* __synth;

	/// Mutex for syncronized access to the Song object and the AudioEngine.
	pthread_mutex_t __engine_mutex;

	struct _locker_struct {
		const char* file;
		unsigned int line;
		const char* function;
	} __locker;
			
	AudioEngine();
};

};


#endif
