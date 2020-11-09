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

#include <string>
#include <cassert>
#include <mutex>
#include <chrono>

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
	 * \return a pointer to the current AudioEngine singleton
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
	 * Lock the AudioEngine for exclusive access by this thread.
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
	 * This function is equivalent to lock() but returns false
	 * immediaely if the lock canot be obtained immediately.
	 *
	 * \param file File the locking occurs in.
	 * \param line Line of the file the locking occurs in.
	 * \param function Function the locking occurs in.
	 *
	 * \return
	 * - true : On success
	 * - false : Else
	 */
	bool try_lock( const char* file, unsigned int line, const char* function );

	/**
	 * Mutex locking of the AudioEngine.
	 *
	 * This function is equivalent to lock() but will only wait for a
	 * given period of time. If the lock cannot be acquired in this
	 * time, it will return false.
	 *
	 * \param duration Time (in microseconds) to wait for the lock.
	 * \param file File the locking occurs in.
	 * \param line Line of the file the locking occurs in.
	 * \param function Function the locking occurs in.
	 *
	 * \return
	 * - true : On successful acquisition of the lock
	 * - false : On failure
	 */
	bool try_lock_for( std::chrono::microseconds duration, const char* file, unsigned int line, const char* function );
	/**
	 * Mutex unlocking of the AudioEngine.
	 *
	 * Unlocks the AudioEngine to allow other threads acces, and leaves #__locker untouched.
	 */
	void unlock();
	
	
	 static float compute_tick_size(int sampleRate, float bpm, int resolution);

	/** \return #__sampler */
	Sampler* get_sampler();
	/** \return #__synth */
	Synth* get_synth();
	
	/** \return #m_fElapsedTime */
	float getElapsedTime() const;
	/** Calculates the elapsed time for an arbitrary position.
	 *
	 * After locating the transport position to @a nFrame the function
	 * calculates the amount of time required to reach the position
	 * during playback. If the Timeline is activated, it will take all
	 * markers and the resulting tempo changes into account.
	 *
	 * Right now the tempo in the region before the first marker
	 * is undefined. In order to make reproducible estimates of the
	 * elapsed time, this function assume it to have the same BPM as
	 * the first marker.
	 *
	 * \param sampleRate Temporal resolution used by the sound card in
	 * frames per second.
	 * \param nFrame Next transport position in frames.
	 * \param nResolution Resolution of the Song (number of ticks per 
	 *   quarter).
	 */
	void calculateElapsedTime( unsigned sampleRate, unsigned long nFrame, int nResolution );
	/** Increments #m_fElapsedTime at the end of a process cycle.
	 *
	 * At the end of H2Core::audioEngine_process() this function will
	 * be used to add the time passed during the last process cycle to
	 * #m_fElapsedTime.
	 *
	 * \param bufferSize Number of frames process during a cycle of
	 * the audio engine.
	 * \param sampleRate Temporal resolution used by the sound card in
	 * frames per second.
	 */
	void updateElapsedTime( unsigned bufferSize, unsigned sampleRate );
	
	/** Relocate using the audio driver and update the
	 * #m_fElapsedTime.
	 *
	 * \param nFrame Next transport position in frames.
	 */
	void  			locate( unsigned long nFrame );

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
	std::timed_mutex __engine_mutex;

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
	
	/** Time in seconds since the beginning of the Song.
	 *
	 * In Hydrogen the current transport position is not measured in
	 * time but in past ticks. Whenever transport is passing a BPM
	 * marker on the Timeline, the tick size and effectively also time
	 * will be rescaled. To nevertheless show the correct time elapsed
	 * since the beginning of the Song, this variable will be used.
	 *
	 * At the end of each transport cycle updateElapsedTime() will be
	 * used to increment it (its smallest resolution is thus
	 * controlled by the buffer size). If, instead, a relocation was
	 * triggered by the user or an external transport control
	 * (e.g. JACK server), calculateElapsedTime() will be used to
	 * determine the time anew.
	 *
	 * If loop transport is enabled #Song::__is_loop_enabled, the
	 * elapsed time will increase constantly. However, if relocation
	 * did happen, only the time relative to the beginning of the Song
	 * will be calculated irrespective of the number of loops played
	 * so far. 
	 *
	 * Retrieved using getElapsedTime().
	 */
	float m_fElapsedTime;

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

inline float AudioEngine::getElapsedTime() const {
	return m_fElapsedTime;
}
	
};


#endif
