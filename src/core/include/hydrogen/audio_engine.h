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

#include "hydrogen/config.h"
#include <hydrogen/object.h>
#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/synth/Synth.h>

#include <pthread.h>
#include <string>
#include <cassert>


/** \def RIGHT_HERE
 * Macro intended to be used for the logging of the locking of the
 * H2Core::AudioEngine. But this feature is not implemented yet.
 *
 * It combines two standard macros of the C language \_\_FILE\_\_ and
 * \_\_LINE\_\_ and one macro introduced by the GCC compiler called
 * \_\_PRETTY_FUNCTION\_\_.
 */
#ifndef RIGHT_HERE
#define RIGHT_HERE __FILE__, __LINE__, __PRETTY_FUNCTION__
#endif

namespace H2Core
{

/**
 * Audio Engine main class (Singleton).
 *
 * It serves as a container for the Sampler and Synth stored in the
 * #__sampler and #__synth member objects and provides a mutex
 * #__engine_mutex enabling the user to synchronize the access of the
 * Song object and the AudioEngine itself. lock() and try_lock() can
 * be called by a thread to lock the engine and unlock() to make it
 * accessible for other threads once again.
 */ 
class AudioEngine : public H2Core::Object
{
	H2_OBJECT
public:
	/**
	 * If #__instance equals 0, a new AudioEngine singleton will
	 * be created and stored in it.
	 *
	 * It is called in Hydrogen::audioEngine_init().
	 */
	static void create_instance();
	/**
	 * Returns a pointer to the current AudioEngine singleton
	 * stored in #__instance.
	 */
	static AudioEngine* get_instance() { assert(__instance); return __instance; }
	/** 
	 * Destructor of the AudioEngine.
	 *
	 * Deletes the Effects singleton and the #__sampler and
	 * #__synth objects.
	 */
	~AudioEngine();

	/** Mutex locking of the AudioEngine.
	 *
	 * It passes the address of the #__engine_mutex object to
	 * _pthread_mutex_lock()_ (pthread.h) to lock the AudioEngine
	 * and transfer ownership of the #__engine_mutex to the
	 * calling thread. Only this very thread can unlock() the
	 * engine again.
	 *
	 * The documentation below may serve as a guide for future
	 * implementations. At the moment the logging of the locking
	 * is __not supported yet__ and the arguments will be just
	 * stored in the #__locker variable, which itself won't be
	 * ever used.
	 *
	 * Easy usage:  Use the #RIGHT_HERE macro like this...
	 * \code{.cpp}
	 *     AudioEngine::get_instance()->lock( RIGHT_HERE );
	 * \endcode
	 *
	 * More complex usage: The parameters @a file and @a function
	 * need to be pointers to null-terminated strings that are
	 * persistent for the entire session.  This does *not* include
	 * the return value of std::string::c_str(), or
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
	 *
	 * \param file File the locking occurs in.
	 * \param line Line of the file the locking occurs in.
	 * \param function Function the locking occurs in.
	 */
	void lock( const char* file, unsigned int line, const char* function );
	/**
	 * Mutex locking of the AudioEngine.
	 *
	 * This function is equivalent to lock() but calls
	 * _pthread_mutex_trylock()_ (pthread.h) instead and returns a
	 * bool depending on its results.
	 *
	 * \param file File the locking occurs in.
	 * \param line Line of the file the locking occurs in.
	 * \param function Function the locking occurs in.
	 *
	 * \return
	 * - true : On success (if _pthread_mutex_lock()_ returns
	 *   0)
	 * - false : Else
	 */
	bool try_lock( const char* file, unsigned int line, const char* function );
	/**
	 * Mutex unlocking of the AudioEngine.
	 *
	 * Calls _pthread_mutex_unlock()_ (pthread.h) on the address
	 * of #__engine_mutex and leaves #__locker untouched.
	 */
	void unlock();
	
	
	 static float compute_tick_size(int sampleRate, int bpm, int resolution);

	/** Returns #__sampler */
	Sampler* get_sampler();
	/** Returns #__synth */
	Synth* get_synth();

private:
	/**
	 * Object holding the current AudioEngine singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static AudioEngine* __instance;

	/** Local instance of the Sampler. */
	Sampler* __sampler;
	/** Local instance of the Synth. */
	Synth* __synth;

	/** Mutex for synchronizing the access to the Song object and
	    the AudioEngine. 
	  * 
	  * It can be used lock the access using either lock() or
	  * try_lock() and to unlock it via unlock(). It is
	  * initialized in AudioEngine() and not explicitly exited.
	  */
	pthread_mutex_t __engine_mutex;

	/**
	 * This struct is most probably intended to be used for
	 * logging the locking of the AudioEngine. But neither it nor
	 * the Logger::AELockTracing state is ever used.
	 */
	struct _locker_struct {
		const char* file;
		unsigned int line;
		const char* function;
	} __locker; ///< This struct is most probably intended to be
		    ///< used for logging the locking of the
		    ///< AudioEngine. But neither it nor the
		    ///< Logger::AELockTracing state is ever used.

	/**
	 * Constructor of the AudioEngine.
	 *
	 * - Assigns #__instance to itself.
	 * - Initializes the Mutex of the AudioEngine #__engine_mutex
	 *   by calling _pthread_mutex_init()_ (pthread.h) on its
	 *   address.
	 * - Assigns a new instance of the Sampler to #__sampler and of
	 *   the Synth to #__synth.
	 * - Creates an instance of the Effects singleton. This call
	 *   should not be necessary since this singleton was created
	 *   right before creating the AudioEngine. But its costs are
	 *   cheap, so I just keep it.
	 */
	AudioEngine();
};

};


#endif
