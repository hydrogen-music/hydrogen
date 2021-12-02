/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <core/config.h>
#include <core/Object.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>
#include <core/Synth/Synth.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/TransportInfo.h>
#include <core/CoreActionController.h>

#include <core/IO/AudioOutput.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/FakeDriver.h>

#include <memory>
#include <string>
#include <cassert>
#include <mutex>
#include <thread>
#include <chrono>
#include <deque>
#include <queue>

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
	class PatternList;
	class Drumkit;
	class Song;
	
/**
 * Audio Engine main class.
 *
 * It serves as a container for the Sampler and Synth stored in the
 * #m_pSampler and #m_pSynth member objects and provides a mutex
 * #m_EngineMutex enabling the user to synchronize the access of the
 * Song object and the AudioEngine itself. lock() and try_lock() can
 * be called by a thread to lock the engine and unlock() to make it
 * accessible for other threads once again.
 *
 * \ingroup docCore docAudioEngine
 */ 
class AudioEngine : public H2Core::TransportInfo, public H2Core::Object<AudioEngine>
{
	H2_OBJECT(AudioEngine)
public:

	/** Audio Engine states  (It's ok to use ==, <, and > when testing)*/
	enum class State {
		/**
		 * State of the H2Core::AudioEngine H2Core::m_state. Not even the
		 * constructors have been called.
		 */
		Uninitialized =	1,
		/**
		 * State of the H2Core::AudioEngine H2Core::m_state. Not ready,
		 * but most pointers are now valid or NULL.
		 */
		Initialized = 2,
		/**
		 * State of the H2Core::AudioEngine H2Core::m_state. Drivers are
		 * set up, but not ready to process audio.
		 */
		Prepared = 3,
		/**
		 * State of the H2Core::AudioEngine H2Core::m_state. Ready to
		 * process audio.
		 */
		Ready = 4,
		/**
		 * State of the H2Core::AudioEngine H2Core::m_state. Currently
		 * playing a sequence.
		 */
		Playing = 5
	};

	/**
	 * Constructor of the AudioEngine.
	 */
	AudioEngine();


	/** 
	 * Destructor of the AudioEngine.
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
	void			lock( const char* file, unsigned int line, const char* function );

	/**
	 * Mutex locking of the AudioEngine.
	 *
	 * This function is equivalent to lock() but returns false
	 * immediaely if the lock cannot be obtained immediately.
	 *
	 * \param file File the locking occurs in.
	 * \param line Line of the file the locking occurs in.
	 * \param function Function the locking occurs in.
	 *
	 * \return
	 * - true : On success
	 * - false : Else
	 */
	bool			tryLock( const char* file, 
						   unsigned int line, 
						   const char* function );

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
	bool			tryLockFor( std::chrono::microseconds duration, 
								  const char* file, 
								  unsigned int line, 
								  const char* function );

	/**
	 * Mutex unlocking of the AudioEngine.
	 *
	 * Unlocks the AudioEngine to allow other threads acces, and leaves #__locker untouched.
	 */
	void			unlock();

	/**
	 * Assert that the calling thread is the current holder of the
	 * AudioEngine lock.
	 */
	void			assertLocked( );
	void			noteOn( Note *note );

	/**
	 * Main audio processing function called by the audio drivers whenever
	 * there is work to do.
	 *
	 * In short, it resets the audio buffers, checks the current transport
	 * position and configuration, updates the queue of notes, which are
	 * about to be played, plays those notes and writes their output to
	 * the audio buffers, and, finally, increment the transport position
	 * in order to move forward in time.
	 *
	 * \param nframes Buffersize.
	 * \param arg Unused.
	 * \return
	 * - __2__ : Failed to aquire the audio engine lock, no processing took place.
	 * - __1__ : kill the audio driver thread. This will be used if either
	 * the DiskWriterDriver or FakeDriver are used and the end of the Song
	 * is reached (audioEngine_updateNoteQueue() returned -1 ). 
	 * - __0__ : else
	 */
	static int                      audioEngine_process( uint32_t nframes, void *arg );

	/** Keep track of the tick position in realtime.
	 *
	 * \return Current position in ticks.
	 */
	unsigned long		getRealtimeTickPosition() const;
	
	static float	computeTickSize( const int nSampleRate, const float fBpm, const int nResolution);

	/** Resets a number of member variables to their initial state.*/
	void reset();

	/** \return #m_pSampler */
	Sampler*		getSampler() const;
	/** \return #m_pSynth */
	Synth*			getSynth() const;

	/** \return #m_fElapsedTime */
	float			getElapsedTime() const;	
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
	void			calculateElapsedTime( unsigned sampleRate, unsigned long nFrame, int nResolution );

	/** 
	 * Creation and initialization of all audio and MIDI drivers called in
	 * Hydrogen::Hydrogen().
	 */
	void			startAudioDrivers();
	/**
	 * Assign an INITIATED audio driver.*/
	void			setAudioDrivers( AudioOutput* pAudioDriver );
	/**
	 * Stops all audio and MIDI drivers.
	 */
	void			stopAudioDrivers();
					
	void			restartAudioDrivers();
					
	void			setupLadspaFX();
	
	/**
	 * Hands the provided Song to JackAudioDriver::makeTrackOutputs() if
	 * @a pSong is not a null pointer and the audio driver #m_pAudioDriver
	 * is an instance of the JackAudioDriver.
	 * \param pSong Song for which per-track output ports should be generated.
	 */
	void			renameJackPorts(std::shared_ptr<Song> pSong);
	
 
	void			setAudioDriver( AudioOutput* pAudioDriver );
	AudioOutput*	getAudioDriver() const;

	/* retrieve the midi (input) driver */
	MidiInput*		getMidiDriver() const;
	/* retrieve the midi (output) driver */
	MidiOutput*		getMidiOutDriver() const;
	

		
	
	void raiseError( unsigned nErrorCode );
	
	State 			getState() const;

	void 			setMasterPeak_L( float value );
	float 			getMasterPeak_L() const;

	void	 		setMasterPeak_R( float value );
	float 			getMasterPeak_R() const;

	float			getProcessTime() const;
	float			getMaxProcessTime() const;

	int				getPatternTickPosition() const;

	int				getColumn() const;

	PatternList*	getNextPatterns() const;
	PatternList*	getPlayingPatterns() const;
	
	unsigned long	getRealtimeFrames() const;

	void			setAddRealtimeNoteTickPosition( unsigned int tickPosition );
	unsigned int	getAddRealtimeNoteTickPosition() const; 

	const struct timeval& 	getCurrentTickTime() const;
	
	/** Calculates the lookahead for a specific tick size.
	 *
	 * During the humanization the onset of a Note will be moved
	 * Note::__lead_lag times the value calculated by this function.
	 *
	 * Since the size of a tick is tempo dependent, @a fTickSize
	 * allows you to calculate the lead-lag factor for an arbitrary
	 * position on the Timeline.
	 *
	 * \param fTickSize Number of frames that make up one tick.
	 *
	 * \return Five times the current size of a tick
	 * (TransportInfo::m_fTickSize) (in frames)
	 */
	static int		calculateLeadLagFactor( float fTickSize );
	/** Calculates time offset (in frames) used to determine the notes
	 * process by the audio engine.
	 *
	 * Due to the humanization there might be negative offset in the
	 * position of a particular note. To be able to still render it
	 * appropriately, we have to look into and handle notes from the
	 * future.
	 *
	 * The Lookahead is the sum of the #m_nMaxTimeHumanize and
	 * calculateLeadLagFactor() plus one (since it has to be larger
	 * than that).
	 *
	 * \param fTickSize Number of frames that make up one tick. Passed
	 * to calculateLeadLagFactor().
	 *
	 * \return Frame offset*/
	static int		calculateLookahead( float fTickSize );

	/**
	 * Sets m_nextState to State::Playing. This will start the audio
	 * engine during the next call of the audioEngine_process callback
	 * function.
	 *
	 * If the JACK audio driver is used, a request to start transport
	 * is send to the JACK server instead.
	 */
	void play();
	/**
	 * Sets m_nextState to State::Ready. This will stop the audio
	 * engine during the next call of the audioEngine_process callback
	 * function.
	 *
	 * If the JACK audio driver is used, a request to stop transport
	 * is send to the JACK server instead.
	 */
	void stop();

	/** Stores the new speed into a separate variable which will be
	 * adopted next time the processCheckBpmChanged() is entered.*/
	void setNextBpm( float fNextBpm );
	float getNextBpm() const;

	static float 	getBpmAtColumn( int nColumn );

	/** Is allowed to call setSong().*/
	friend void Hydrogen::setSong( std::shared_ptr<Song> pSong );
	/** Is allowed to call removeSong().*/
	friend void Hydrogen::removeSong();
	/** Is allowed to use locate() to directly set the position in
		frames as well as to used setColumn and setPatternTickPos to
		move the arrow in the SongEditorPositionRuler even when
		playback is stopped.*/
	friend bool CoreActionController::locateToFrame( unsigned long nFrame, bool );
	/** Is allowed to set m_state to State::Prepared via setState()*/
	friend int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo, bool conditional );
	/** Is allowed to set m_state to State::Ready via setState()*/
	friend int FakeDriver::connect();
	/** Is allowed to set m_nextState via setNextState() according to
		what the JACK server reports and to call relocateTransport().*/
	friend void JackAudioDriver::updateTransportInfo();
	/** Is allowed to call relocateTransport().*/
	friend void JackAudioDriver::relocateUsingBBT();
private:
	
	inline void			processPlayNotes( unsigned long nframes );
	/**
	 * Updating the TransportInfo of the audio driver.
	 */
	inline void			processTransport( unsigned nFrames );

	void			clearNoteQueue();
	/** Clear all audio buffers.
	 */
	void			clearAudioBuffers( uint32_t nFrames );	
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
	AudioOutput*	createDriver( const QString& sDriver );
	/**
	 * Takes all notes from the current patterns, from the MIDI queue
	 * #m_midiNoteQueue, and those triggered by the metronome and pushes
	 * them onto #m_songNoteQueue for playback.
	 *
	 * Apart from the MIDI queue, the extraction of all notes will be
	 * based on their position measured in ticks. Since Hydrogen does
	 * support humanization, which also involves triggering a Note
	 * earlier or later than its actual position, the loop over all ticks
	 * won't be done starting from the current position but at some
	 * position in the future. This value, also called @e lookahead, is
	 * set to the sum of the maximum offsets introduced by both the random
	 * humanization (2000 frames) and the deterministic lead-lag offset (5
	 * times TransportInfo::m_nFrames) plus 1 (note that it's not given in
	 * ticks but in frames!). Hydrogen thus loops over @a nFrames frames
	 * starting at the current position + the lookahead (or at 0 when at
	 * the beginning of the Song).
	 *
	 * \return
	 * - -1 if in Song::SONG_MODE and no patterns left.
	 * - 2 if the current pattern changed with respect to the last
	 * cycle.
	 */
	int				updateNoteQueue( unsigned nFrames );
	
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
	void			updateElapsedTime( unsigned bufferSize, unsigned sampleRate );
	void			processCheckBPMChanged();
	
	void			setPatternTickPosition( int tick );
	void			setColumn( int nColumn );
	void			setRealtimeFrames( unsigned long nFrames );
	
	/**
	 * Updates the global objects of the audioEngine according to new
	 * Song.
	 *
	 * \param pNewSong Song to load.
	 */
	void			setSong( std::shared_ptr<Song>pNewSong );
	/**
	 * Does the necessary cleanup of the global objects in the audioEngine.
	 */
	void			removeSong();
	void 			setState( State state );
	void 			setNextState( State state );

	/**
	 * Resets a number of member variables and sets m_state to
	 * State::Playing.
	 */
	void				startPlayback();
	
	/**
	 * Resets a number of member variables and sets m_state to
	 * State::Ready.
	 */
	void			stopPlayback();
	
	/** Relocate using the audio driver.
	 *
	 * \param nFrame Next transport position in frames.
	 * \param bWithJackBroadcast Relocate not using the AudioEngine
	 * directly but using the JACK server.
	 */
	void			locate( unsigned long nFrame, bool bWithJackBroadcast = true );
	void			relocateTransport( unsigned long nFrame );
	/** Local instance of the Sampler. */
	Sampler* 			m_pSampler;
	/** Local instance of the Synth. */
	Synth* 				m_pSynth;

	/**
	 * Pointer to the current instance of the audio driver.
	 */	
	AudioOutput *		m_pAudioDriver;

	/**
	 * MIDI input
	 */
	MidiInput *			m_pMidiDriver;

	/**
	 * MIDI output
	 */
	MidiOutput *		m_pMidiDriverOut;
	
	EventQueue* 		m_pEventQueue;

	#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_
	float				m_fFXPeak_L[MAX_FX];
	float				m_fFXPeak_R[MAX_FX];
	#endif

	//Master peak (left channel)
	float				m_fMasterPeak_L;

	//Master peak (right channel)
	float				m_fMasterPeak_R;

	/**
	 * Mutex for synchronizing the access to the Song object and
	 * the AudioEngine.
	 *
	 * It can be used lock the access using either lock() or
	 * try_lock() and to unlock it via unlock(). It is
	 * initialized in AudioEngine() and not explicitly exited.
	 */
	std::timed_mutex 	m_EngineMutex;
	
	/**
	 * Mutex for locking the pointer to the audio output buffer, allowing
	 * multiple readers.
	 *
	 * When locking this __and__ the AudioEngine, always lock the
	 * AudioEngine first using AudioEngine::lock() or
	 * AudioEngine::try_lock(). Always use a QMutexLocker to lock this
	 * mutex.
	 */
	QMutex				m_MutexOutputPointer;

	/**
	 * Thread ID of the current holder of the AudioEngine lock.
	 */
	std::thread::id 	m_LockingThread;

	struct _locker_struct {
		const char* file;
		unsigned int line;
		const char* function;
	} __locker;
	
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
	 * If loop transport is enabled #H2Core::Song::m_bIsLoopEnabled,
	 * the elapsed time will increase constantly. However, if
	 * relocation did happen, only the time relative to the beginning
	 * of the Song will be calculated irrespective of the number of
	 * loops played so far.
	 *
	 * Retrieved using getElapsedTime().
	 */
	float				m_fElapsedTime;

	// time used in process function
	float				m_fProcessTime;

	// max ms usable in process with no xrun
	float				m_fMaxProcessTime;

	// updated in audioEngine_updateNoteQueue()
	struct timeval		m_currentTickTime;

	/**
	 * Beginning of the current pattern in ticks.
	 */
	int					m_nPatternStartTick;

	/**
	 * Ticks passed since the beginning of the current pattern.
	 */
	unsigned int		m_nPatternTickPosition;

	/**
	 * Index of the current PatternList/column in the
	 * Song::__pattern_group_sequence.
	 *
	 * A value of -1 corresponds to "pattern list could not be found".
	 */
	int					m_nColumn;
	int					m_nOldColumn;

	/** Set to the total number of ticks in a Song in findPatternInTick()
		if Song::SONG_MODE is chosen and playback is at least in the
		second loop.*/
	int					m_nSongSizeInTicks;

		/**
	 * Patterns to be played next in Song::PATTERN_MODE.
	 */
	PatternList*		m_pNextPatterns;
	
	/**
	 * PatternList containing all Patterns currently played back.
	 */
	PatternList*		m_pPlayingPatterns;

	/**
	 * Variable keeping track of the transport position in realtime.
	 *
	 * Even if the audio engine is stopped, the variable will be
	 * incremented  (as audioEngine_process() would do at the beginning
	 * of each cycle) to support realtime keyboard and MIDI event
	 * timing.
	 */
	unsigned long		m_nRealtimeFrames;
	unsigned int		m_nAddRealtimeNoteTickPosition;

	/**
	 * Current state of the H2Core::AudioEngine.
	 */	
	State				m_state;
	/** 
	 * State assigned during the next call to processTransport(). This
	 * is used to start and stop the audio engine.
	 */
	State				m_nextState;
	
	audioProcessCallback m_AudioProcessCallback;
	
	/// Song Note FIFO
	// overload the > operator of Note objects for priority_queue
	struct compare_pNotes {
		bool operator() (Note* pNote1, Note* pNote2);
	};

	std::priority_queue<Note*, std::deque<Note*>, compare_pNotes > m_songNoteQueue;
	std::deque<Note*>	m_midiNoteQueue;	///< Midi Note FIFO
	
	/**
	 * Pointer to the metronome.
	 *
	 * Initialized in audioEngine_init().
	 */
	std::shared_ptr<Instrument>		m_pMetronomeInstrument;
	/**
	 * Maximum time (in frames) a note's position can be off due to
	 * the humanization (lead-lag).
	 *
	 * Required to calculateLookahead(). Set to 2000.
	 */
	static const int		nMaxTimeHumanize;

	float 			m_fNextBpm;
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
	void assertAudioEngineLocked() const;


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
	assert( m_LockingThread == std::this_thread::get_id() );
#endif
}

inline void	AudioEngine::setMasterPeak_L( float value ) {
	m_fMasterPeak_L = value;
}

inline float AudioEngine::getMasterPeak_L() const {
	return m_fMasterPeak_L;
}

inline void	AudioEngine::setMasterPeak_R( float value ) {
	m_fMasterPeak_R = value;
}

inline float AudioEngine::getMasterPeak_R() const {
	return m_fMasterPeak_R;
}

inline float AudioEngine::getProcessTime() const {
	return m_fProcessTime;
}

inline float AudioEngine::getMaxProcessTime() const {
	return m_fMaxProcessTime;
}

inline const struct timeval& AudioEngine::getCurrentTickTime() const {
	return m_currentTickTime;
}

inline AudioEngine::State AudioEngine::getState() const {
	return m_state;
}

inline void AudioEngine::setState( AudioEngine::State state) {
	m_state = state;
}
inline void AudioEngine::setNextState( AudioEngine::State state) {
	m_nextState = state;
}

inline AudioOutput*	AudioEngine::getAudioDriver() const {
	return m_pAudioDriver;
}

inline 	MidiInput*	AudioEngine::getMidiDriver() const {
	return m_pMidiDriver;
}

inline MidiOutput*	AudioEngine::getMidiOutDriver() const {
	return m_pMidiDriverOut;
}

inline void AudioEngine::setPatternTickPosition(int tick) {
	m_nPatternTickPosition = tick;
}

inline int AudioEngine::getPatternTickPosition() const {
	return m_nPatternTickPosition;
}

inline void AudioEngine::setColumn( int songPos ) {
	m_nColumn = songPos;
}

inline int AudioEngine::getColumn() const {
	return m_nColumn;
}

inline PatternList* AudioEngine::getPlayingPatterns() const {
	return m_pPlayingPatterns;
}

inline PatternList* AudioEngine::getNextPatterns() const {
	return m_pNextPatterns;
}

inline unsigned long AudioEngine::getRealtimeFrames() const {
	return m_nRealtimeFrames;
}

inline void AudioEngine::setRealtimeFrames( unsigned long nFrames ) {
	m_nRealtimeFrames = nFrames;
}

inline unsigned int AudioEngine::getAddRealtimeNoteTickPosition() const {
	return m_nAddRealtimeNoteTickPosition;
}

inline void AudioEngine::setAddRealtimeNoteTickPosition( unsigned int tickPosition) {
	m_nAddRealtimeNoteTickPosition = tickPosition;
}
inline void AudioEngine::setNextBpm( float fNextBpm ) {
	m_fNextBpm = fNextBpm;
}
inline float AudioEngine::getNextBpm() const {
	return m_fNextBpm;
}
	
};

#endif
