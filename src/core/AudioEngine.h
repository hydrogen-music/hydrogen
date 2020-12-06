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

#include <core/config.h>
#include <core/Object.h>
#include <core/Sampler/Sampler.h>
#include <core/Synth/Synth.h>

#include <string>
#include <cassert>
#include <mutex>
#include <thread>
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

typedef int  ( *audioProcessCallback )( uint32_t, void * );

namespace H2Core
{
	class MidiOutput;
	class MidiInput;
	class EventQueue;

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

	/**
	 * Assert that the calling thread is the current holder of the
	 * AudioEngine lock.
	 */
	void assertLocked( );
	
	static float compute_tick_size( const int nSampleRate, const float fBpm, const int nResolution);

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
	
	/**
	 * Update the tick size based on the current tempo without affecting
	 * the current transport position.
	 *
	 * To access a change in the tick size, the value stored in
	 * TransportInfo::m_fTickSize will be compared to the one calculated
	 * from the AudioOutput::getSampleRate(), Song::__bpm, and
	 * Song::__resolution. Thus, if any of those quantities did change,
	 * the transport position will be recalculated.
	 *
	 * The new transport position gets calculated by 
	 * \code{.cpp}
	 * ceil( m_pAudioDriver->m_transport.m_nFrames/
	 *       m_pAudioDriver->m_transport.m_fTickSize ) *
	 * m_pAudioDriver->getSampleRate() * 60.0 / Song::__bpm / Song::__resolution 
	 * \endcode
	 *
	 * If the JackAudioDriver is used and the audio engine is playing, a
	 * potential mismatch in the transport position is determined by
	 * JackAudioDriver::calculateFrameOffset() and covered by
	 * JackAudioDriver::updateTransportInfo() in the next cycle.
	 *
	 * Finally, EventQueue::push_event() is called with
	 * #EVENT_RECALCULATERUBBERBAND and -1 as arguments.
	 *
	 * Called in audioEngine_process() and audioEngine_setSong(). The
	 * function will only perform actions if #m_audioEngineState is in
	 * either #STATE_READY or #STATE_PLAYING.
	 */
	void	process_checkBPMChanged(Song *pSong);
	
	/** Relocate using the audio driver and update the
	 * #m_fElapsedTime.
	 *
	 * \param nFrame Next transport position in frames.
	 */
	void locate( unsigned long nFrame );
	
	
	/** Clear all audio buffers.
	 *
	 * It locks the audio output buffer using #mutex_OutputPointer, gets
	 * fresh pointers to the output buffers #m_pMainBuffer_L and
	 * #m_pMainBuffer_R using AudioOutput::getOut_L() and
	 * AudioOutput::getOut_R() of the current instance of the audio driver
	 * #m_pAudioDriver, and overwrites their memory with
	 * \code{.cpp}
	 * nFrames * sizeof( float ) 
	 * \endcode
	 * zeros.
	 *
	 * If the JACK driver is used and Preferences::m_bJackTrackOuts is set
	 * to true, the stereo buffers for all tracks of the components of
	 * each instrument will be reset as well.  If LadspaFX are used, the
	 * output buffers of all effects LadspaFX::m_pBuffer_L and
	 * LadspaFX::m_pBuffer_L have to be reset as well.
	 *
	 * If the audio driver #m_pAudioDriver isn't set yet, it will just
	 * unlock and return.
	 */
	void clearAudioBuffers( uint32_t nFrames );
	
	/**
	 * Create an audio driver using audioEngine_process() as its argument
	 * based on the provided choice and calling their _init()_ function to
	 * trigger their initialization.
	 *
	 * For a listing of all possible choices, please see
	 * Preferences::m_sAudioDriver.
	 *
	 * \param sDriver String specifying which audio driver should be
	 * created.
	 * \return Pointer to the freshly created audio driver. If the
	 * creation resulted in a NullDriver, the corresponding object will be
	 * deleted and a null pointer returned instead.
	 */
	AudioOutput* createDriver( const QString& sDriver );
	
	/** 
	 * Creation and initialization of all audio and MIDI drivers called in
	 * Hydrogen::Hydrogen().
	 *
	 * Which audio driver to use is specified in
	 * Preferences::m_sAudioDriver. If "Auto" is selected, it will try to
	 * initialize drivers using createDriver() in the following order: 
	 * - Windows:  "PortAudio", "Alsa", "CoreAudio", "Jack", "Oss",
	 *   and "PulseAudio" 
	 * - all other systems: "Jack", "Alsa", "CoreAudio", "PortAudio",
	 *   "Oss", and "PulseAudio".
	 * If all of them return NULL, #m_pAudioDriver will be initialized
	 * with the NullDriver instead. If a specific choice is contained in
	 * Preferences::m_sAudioDriver and createDriver() returns NULL, the
	 * NullDriver will be initialized too.
	 *
	 * It probes Preferences::m_sMidiDriver to create a midi driver using
	 * either AlsaMidiDriver::AlsaMidiDriver(),
	 * PortMidiDriver::PortMidiDriver(), CoreMidiDriver::CoreMidiDriver(),
	 * or JackMidiDriver::JackMidiDriver(). Afterwards, it sets
	 * #m_pMidiDriverOut and #m_pMidiDriver to the freshly created midi
	 * driver and calls their open() and setActive( true ) functions.
	 *
	 * If a Song is already present, the state of the AudioEngine
	 * #m_audioEngineState will be set to #STATE_READY, the bpm of the
	 * #m_pAudioDriver will be set to the tempo of the Song Song::__bpm
	 * using AudioOutput::setBpm(), and #STATE_READY is pushed on the
	 * EventQueue. If no Song is present, the state will be
	 * #STATE_PREPARED and no bpm will be set.
	 *
	 * All the actions mentioned so far will be performed after locking
	 * both the AudioEngine using AudioEngine::lock() and the mutex of the
	 * audio output buffer #mutex_OutputPointer. When they are completed
	 * both mutex are unlocked and the audio driver is connected via
	 * AudioOutput::connect(). If this is not successful, the audio driver
	 * will be overwritten with the NullDriver and this one is connected
	 * instead.
	 *
	 * Finally, audioEngine_renameJackPorts() (if #H2CORE_HAVE_JACK is set)
	 * and audioEngine_setupLadspaFX() are called.
	 *
	 * The state of the AudioEngine #m_audioEngineState must not be in
	 * #STATE_INITIALIZED or the function will just unlock both mutex and
	 * returns.
	 */
	void	startAudioDrivers();
	
	/**
	 * Stops all audio and MIDI drivers.
	 *
	 * Uses audioEngine_stop() if the AudioEngine is still in state
	 * #m_audioEngineState #STATE_PLAYING, sets its state to
	 * #STATE_INITIALIZED, locks the AudioEngine using
	 * AudioEngine::lock(), deletes #m_pMidiDriver and #m_pAudioDriver and
	 * reinitializes them to NULL. 
	 *
	 * If #m_audioEngineState is neither in #STATE_PREPARED or
	 * #STATE_READY, the function returns before deleting anything.
	 */
	void	stopAudioDrivers();
	
	void	restartAudioDrivers();
	
	void	setupLadspaFX( unsigned nBufferSize );
	
	/**
	 * Hands the provided Song to JackAudioDriver::makeTrackOutputs() if
	 * @a pSong is not a null pointer and the audio driver #m_pAudioDriver
	 * is an instance of the JackAudioDriver.
	 * \param pSong Song for which per-track output ports should be generated.
	 */
	void	renameJackPorts(Song * pSong);
	
	//Set the callback of the audio engine. TODO SMO: Remove after refactoring
	void setCallback(audioProcessCallback Callback);
	
	/**
	 * Pointer to the current instance of the audio driver.
	 *
	 * Initialized with NULL inside audioEngine_init(). Inside
	 * audioEngine_startAudioDrivers() either the audio driver specified
	 * in Preferences::m_sAudioDriver and created via createDriver() or
	 * the NullDriver, in case the former failed, will be assigned.
	 */	
	AudioOutput *		m_pAudioDriver;
	
	AudioOutput *		getAudioDriver();
	
	/**
	 * Mutex for locking the pointer to the audio output buffer, allowing
	 * multiple readers.
	 *
	 * When locking this __and__ the AudioEngine, always lock the
	 * AudioEngine first using AudioEngine::lock() or
	 * AudioEngine::try_lock(). Always use a QMutexLocker to lock this
	 * mutex.
	 */
	QMutex				mutex_OutputPointer;
	
	/**
	 * MIDI input
	 *
	 * In audioEngine_startAudioDrivers() it is assigned the midi driver
	 * specified in Preferences::m_sMidiDriver.
	 */
	MidiInput *			m_pMidiDriver;
	MidiInput*			getMidiDriver();
	
	/**
	 * MIDI output
	 *
	 * In audioEngine_startAudioDrivers() it is assigned the midi driver
	 * specified in Preferences::m_sMidiDriver.
	 */
	MidiOutput *		m_pMidiDriverOut;
	MidiOutput*			getMidiOutDriver();
	
	/**
	 * Pointer to the audio buffer of the left stereo output returned by
	 * AudioOutput::getOut_L().
	 */
	float*	m_pMainBuffer_L;
	
	/**
	 * Pointer to the audio buffer of the right stereo output returned by
	 * AudioOutput::getOut_R().
	 */
	float*	m_pMainBuffer_R;
	
	void raiseError( unsigned nErrorCode );
	
	int getState() const;
	
	void setState(int state);

private:
	/**
	 * Object holding the current AudioEngine singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static AudioEngine* __instance;
	
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

	/** Local instance of the Sampler. */
	Sampler* __sampler;
	/** Local instance of the Synth. */
	Synth* __synth;
	
	EventQueue* m_pEventQueue;

	/**
	 * Mutex for synchronizing the access to the Song object and
	 * the AudioEngine.
	 *
	 * It can be used lock the access using either lock() or
	 * try_lock() and to unlock it via unlock(). It is
	 * initialized in AudioEngine() and not explicitly exited.
	 */
	std::timed_mutex __engine_mutex;

	/**
	 * Thread ID of the current holder of the AudioEngine lock.
	 */
	std::thread::id m_lockingThread;

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
	float	m_fElapsedTime;
	
	/**
	 * Current state of the H2Core::AudioEngine. 
	 *
	 * It is supposed to take five different states:
	 *
	 * - #STATE_UNINITIALIZED:	1      Not even the constructors have been called.
	 * - #STATE_INITIALIZED:	2      Not ready, but most pointers are now valid or NULL
	 * - #STATE_PREPARED:		3      Drivers are set up, but not ready to process audio.
	 * - #STATE_READY:		4      Ready to process audio
	 * - #STATE_PLAYING:		5      Currently playing a sequence.
	 * 
	 * It gets initialized with #STATE_UNINITIALIZED.
	 */	
	int	m_State;
	
	audioProcessCallback m_AudioProcessCallback;
};


/**
 * AudioEngineLocking
 *
 * This is a base class for shared data structures which may be
 * modified by the AudioEngine. These should only be modified or
 * trusted by a thread holding the AudioEngine lock.
 *
 * Any class which implements a data structure which can be modified
 * by the AudioEngine can inherit from this, and use the protected
 * "assertLocked()" method to ensure that methods are called only
 * with appropriate locking.
 *
 * Checking is only done on debug builds.
 */
class AudioEngineLocking {

	bool m_bNeedsLock;

protected:
	/**
	 *  Assert that the AudioEngine lock is held if needed.
	 */
	void assertAudioEngineLocked() const {
#ifndef NDEBUG
		if ( m_bNeedsLock ) {
			AudioEngine::get_instance()->assertLocked();
		}
#endif
	}


public:

	/**
	 * The audio processing thread can modify some PatternLists. For
	 * these structures, the audio engine lock must be held for any
	 * thread to access them.
	 */
	void setNeedsLock( bool bNeedsLock ) {
		m_bNeedsLock = bNeedsLock;
	}

	AudioEngineLocking() {
		m_bNeedsLock = false;
	}
};


inline float AudioEngine::getElapsedTime() const {
	return m_fElapsedTime;
}

inline void AudioEngine::assertLocked( ) {
#ifndef NDEBUG
	assert( m_lockingThread == std::this_thread::get_id() );
#endif
}

inline int AudioEngine::getState() const {
	return m_State;
}

inline void AudioEngine::setState(int state) {
	m_State = state;
}

inline void AudioEngine::setCallback(audioProcessCallback Callback) {
	m_AudioProcessCallback = Callback;
}

inline 	AudioOutput *	AudioEngine::getAudioDriver() {
	return m_pAudioDriver;
}

inline 	MidiInput *	AudioEngine::getMidiDriver() {
	return m_pMidiDriver;
}

inline 	MidiOutput *	AudioEngine::getMidiOutDriver() {
	return m_pMidiDriverOut;
}

};

#endif
