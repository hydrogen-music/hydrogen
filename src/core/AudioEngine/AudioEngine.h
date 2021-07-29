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
#include <core/Sampler/Sampler.h>
#include <core/Synth/Synth.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/TransportInfo.h>
#include <core/CoreActionController.h>

#include <core/IO/AudioOutput.h>

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

// Audio Engine states  (It's ok to use ==, <, and > when testing)
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Not even the
 * constructors have been called.
 */
#define STATE_UNINITIALIZED	1
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Not ready,
 * but most pointers are now valid or NULL.
 */
#define STATE_INITIALIZED	2
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Drivers are
 * set up, but not ready to process audio.
 */
#define STATE_PREPARED		3
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Ready to
 * process audio.
 */
#define STATE_READY		4
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Currently
 * playing a sequence.
 */
#define STATE_PLAYING		5

typedef int  ( *audioProcessCallback )( uint32_t, void * );

namespace H2Core
{
	class MidiOutput;
	class MidiInput;
	class EventQueue;
	class PatternList;
	class Hydrogen;
	
/**
 * Audio Engine main class.
 *
 * It serves as a container for the Sampler and Synth stored in the
 * #m_pSampler and #m_pSynth member objects and provides a mutex
 * #__engine_mutex enabling the user to synchronize the access of the
 * Song object and the AudioEngine itself. lock() and try_lock() can
 * be called by a thread to lock the engine and unlock() to make it
 * accessible for other threads once again.
 */ 
class AudioEngine : public H2Core::TransportInfo
{
	H2_OBJECT
public:

	/**
	* Initialization of the H2Core::AudioEngine (concstructed in Hydrogen::Hydrogen()).
	*
	* -# It creates two new instances of the H2Core::PatternList and stores them 
	* in #m_pPlayingPatterns and #m_pNextPatterns.
	* -# It sets #m_nSongPos = -1.
	* -# It sets #m_nSelectedPatternNumber, #m_nSelectedInstrumentNumber,
	*	and #m_nPatternTickPosition to 0.
	* -# It sets #m_pMetronomeInstrument, #m_pAudioDriver to NULL.
	* -# It uses the current time to a random seed via std::srand(). This
	*	way the states of the pseudo-random number generator are not
	*	cross-correlated between different runs of Hydrogen.
	* -# It initializes the metronome with the sound stored in
	*	H2Core::Filesystem::click_file_path() by creating a new
	*	Instrument with #METRONOME_INSTR_ID as first argument.
	* -# It sets the H2Core::AudioEngine state #m_audioEngineState to
	*	#STATE_INITIALIZED.
	* -# It calls H2Core::Effects::create_instance() (if the
	*	#H2CORE_HAVE_LADSPA is set),
	*
	* If the current state of the H2Core::AudioEngine #m_audioEngineState is not
	* ::STATE_UNINITIALIZED, it will thrown an error and
	* H2Core::AudioEngine::unlock() it.
	*/

	/**
	 * Constructor of the AudioEngine.
	 *
	 * - Initializes the Mutex of the AudioEngine #__engine_mutex
	 *   by calling _pthread_mutex_init()_ (pthread.h) on its
	 *   address.
	 * - Assigns a new instance of the Sampler to #m_pSampler and of
	 *   the Synth to #m_pSynth.
	 */
	AudioEngine();


	/** 
	 * Destructor of the AudioEngine.
	 *
	 * Deletes the Effects singleton and the #m_pSampler and
	 * #m_pSynth objects.
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
	
	
	void			destroy();

	/**
	 * If the audio engine is in state #m_audioEngineState #STATE_READY,
	 * this function will
	 * - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f
	 * - sets TransportInfo::m_nFrames to @a nTotalFrames
	 * - sets m_nSongPos and m_nPatternStartTick to -1
	 * - m_nPatternTickPosition to 0
	 * - sets #m_audioEngineState to #STATE_PLAYING
	 * - pushes the #EVENT_STATE #STATE_PLAYING using EventQueue::push_event()
	 *
	 * \param bLockEngine Whether or not to lock the audio engine before
	 *   performing any actions. The audio engine __must__ be locked! This
	 *   option should only be used, if the process calling this function
	 *   did already locked it.
	 * \param nTotalFrames New value of the transport position.
	 * \return 0 regardless what happens inside the function.
	 */
	int				start( bool bLockEngine = false, unsigned nTotalFrames = 0 );
	
	/**
	 * If the audio engine is in state #m_audioEngineState #STATE_PLAYING,
	 * this function will
	 * - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f
	 * - sets #m_audioEngineState to #STATE_READY
	 * - sets #m_nPatternStartTick to -1
	 * - deletes all copied Note in song notes queue #m_songNoteQueue and
	 *   MIDI notes queue #m_midiNoteQueue
	 * - calls the _clear()_ member of #m_midiNoteQueue
	 *
	 * \param bLockEngine Whether or not to lock the audio engine before
	 *   performing any actions. The audio engine __must__ be locked! This
	 *   option should only be used, if the process calling this function
	 *   did already locked it.
	 */
	void			stop( bool bLockEngine = false );
	
	/**
	 * Updates the global objects of the audioEngine according to new
	 * Song.
	 *
	 * It also updates all member variables of the audio driver specific
	 * to the particular song (BPM and tick size).
	 *
	 * \param pNewSong Song to load.
	 */
	void			setSong(std::shared_ptr<Song>pNewSong );

	/**
	 * Does the necessary cleanup of the global objects in the audioEngine.
	 *
	 * Class the clear() member of #m_pPlayingPatterns and
	 * #m_pNextPatterns as well as audioEngine_clearNoteQueue();
	 */
	void			removeSong();
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
	 * In detail the function
	 * - calls audioEngine_process_clearAudioBuffers() to reset all audio
	 * buffers with zeros.
	 * - calls audioEngine_process_transport() to verify the current
	 * TransportInfo stored in AudioOutput::m_transport. If e.g. the
	 * JACK server is used, an external JACK client might have changed the
	 * speed of the transport (as JACK timebase master) or the transport
	 * position. In such cases, Hydrogen has to sync its internal transport
	 * state AudioOutput::m_transport to reflect these changes. Else our
	 * playback would be off.
	 * - calls audioEngine_process_checkBPMChanged() to check whether the
	 * tick size, the number of frames per bar (size of a pattern), has
	 * changed (see TransportInfo::m_nFrames in case you are unfamiliar
	 * with the term _frames_). This is necessary because the transport
	 * position is often given in ticks within Hydrogen and changing the
	 * speed of the Song, e.g. via Hydrogen::setBPM(), would thus result
	 * in a(n unintended) relocation of the transport location.
	 * - calls audioEngine_updateNoteQueue() and
	 * audioEngine_process_playNotes(), two functions which handle the
	 * selection and playback of notes and will documented at a later
	 * point in time
	 * - If audioEngine_updateNoteQueue() returns with 2, the
	 * EVENT_PATTERN_CHANGED event will be pushed to the EventQueue.
	 * - writes the audio output of the Sampler, Synth, and the LadspaFX
	 * (if #H2CORE_HAVE_LADSPA is defined) to audio output buffers, and
     * sets we peak values for #m_fFXPeak_L,
	 * #m_fFXPeak_R, #m_fMasterPeak_L, and #m_fMasterPeak_R.
	 * - finally increments the transport position
	 * TransportInfo::m_nFrames with the buffersize @a nframes. So, if
	 * this function is called during the next cycle, the transport is
	 * already in the correct position.
	 *
	 * If the H2Core::m_audioEngineState is neither in #STATE_READY nor
	 * #STATE_PLAYING or the locking of the AudioEngine failed, the
	 * function will return 0 without performing any actions.
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
	static int			audioEngine_process( uint32_t nframes, void *arg );
	
	void			clearNoteQueue();
	
	inline void			processPlayNotes( unsigned long nframes );
	/**
	 * Updating the TransportInfo of the audio driver.
	 *
	 * Firstly, it calls AudioOutput::updateTransportInfo() and then
	 * updates the state of the audio engine #m_audioEngineState depending
	 * on the status of the audio driver.  E.g. if the JACK transport was
	 * started by another client, the audio engine has to be started as
	 * well. If TransportInfo::m_status is TransportInfo::ROLLING,
	 * audioEngine_start() is called with
	 * TransportInfo::m_nFrames as argument if the engine is in
	 * #STATE_READY. If #m_audioEngineState is then still not in
	 * #STATE_PLAYING, the function will return. Otherwise, the current
	 * speed is getting updated by calling Hydrogen::setBPM using
	 * TransportInfo::m_fBPM and #m_nRealtimeFrames is set to
	 * TransportInfo::m_nFrames.
	 *
	 * If the status is TransportInfo::STOPPED but the engine is still
	 * running, audioEngine_stop() will be called. In any case,
	 * #m_nRealtimeFrames will be incremented by #nFrames to support
	 * realtime keyboard and MIDI event timing.
	 *
	 * If the H2Core::m_audioEngineState is neither in #STATE_READY nor
	 * #STATE_PLAYING the function will immediately return.
	 */
	inline void			processTransport( unsigned nFrames );
	
	inline unsigned		renderNote( Note* pNote, const unsigned& nBufferSize );
	// TODO: Add documentation of inPunchArea, and
	// m_addMidiNoteVector
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
	 * Within this loop all MIDI notes in #m_midiNoteQueue with a
	 * Note::__position smaller or equal the current tick will be popped
	 * and added to #m_songNoteQueue and the #EVENT_METRONOME Event is
	 * pushed to the EventQueue at a periodic rate. If in addition
	 * Preferences::m_bUseMetronome is set to true,
	 * #m_pMetronomeInstrument will be used to push a 'click' to the
	 * #m_songNoteQueue too. All patterns enclosing the current tick will
	 * be added to #m_pPlayingPatterns and all their containing notes,
	 * which position enclose the current tick too, will be added to the
	 * #m_songNoteQueue. If the Song is in Song::PATTERN_MODE, the
	 * patterns used are not chosen by the actual position but by
	 * #m_nSelectedPatternNumber and #m_pNextPatterns. 
	 *
	 * All notes obtained by the current patterns (and only those) are
	 * also subject to humanization in the onset position of the created
	 * Note. For now Hydrogen does support three options of altering
	 * these:
	 * - @b Swing - A deterministic offset determined by Song::__swing_factor
	 * will be added for some notes in a periodic way.
	 * - @b Humanize - A random offset drawn from Gaussian white noise
	 * with a variance proportional to Song::__humanize_time_value will be
	 * added to every Note.
	 * - @b Lead/Lag - A deterministic offset determined by
	 * Note::__lead_lag will be added for every note.
	 *
	 * If the AudioEngine it not in #STATE_PLAYING, the loop jumps right
	 * to the next tick.
	 *
	 * \return
	 * - -1 if in Song::SONG_MODE and no patterns left.
	 * - 2 if the current pattern changed with respect to the last
	 * cycle.
	 */
	int				updateNoteQueue( unsigned nFrames );
	void			prepNoteQueue();
	
	/**
	 * Find a PatternList/column corresponding to the supplied tick
	 * position @a nTick.
	 *
	 * Adds up the lengths of all pattern columns until @a nTick lies in
	 * between the bounds of a Pattern.
	 *
	 * \param nTick Position in ticks.
	 * \param bLoopMode Whether looping is enabled in the Song, see
	 *   Song::is_loop_enabled(). If true, @a nTick is allowed to be
	 *   larger than the total length of the Song.
	 * \param pPatternStartTick Pointer to an integer the beginning of the
	 *   found pattern list will be stored in (in ticks).
	 * \return
	 *   - -1 : pattern list couldn't be found.
	 *   - >=0 : PatternList index in Song::__pattern_group_sequence.
	 */
	int				getColumnForTick( int nTick, bool bLoopMode, int* pPatternStartTick );
	/**
	 * Get the total number of ticks passed up to a @a nColumn /
	 * pattern group.
	 *
	 * The driver should be LOCKED when calling this!
	 *
	 * \param nColumn pattern group.
	 * \return
	 *  - -1 : if @a nColumn is bigger than the number of patterns in
	 *   the Song and Song::getIsLoopEnabled() is set to false or
	 *   no Patterns could be found at all.
	 *  - >= 0 : the total number of ticks passed.
	 */
	long			getTickForColumn( int nColumn );

	static float	computeTickSize( const int nSampleRate, const float fBpm, const int nResolution);

	/** \return #m_pSampler */
	Sampler*		getSampler();
	/** \return #m_pSynth */
	Synth*			getSynth();

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
	void			processCheckBPMChanged(std::shared_ptr<Song>pSong);
	
	
	/** Clear all audio buffers.
	 *
	 * It locks the audio output buffer using #mutex_OutputPointer, gets
	 * pointers to the output buffers using AudioOutput::getOut_L() and
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
	 * Creation and initialization of all audio and MIDI drivers called in
	 * Hydrogen::Hydrogen().
	 *
	 * Which audio driver to use is specified in
	 * Preferences::m_sAudioDriver. If "Auto" is selected, it will try to
	 * initialize drivers using createDriver() in the following order: 
	 * - Windows:  "PortAudio", "ALSA", "CoreAudio", "JACK", "OSS",
	 *   and "PulseAudio" 
	 * - all other systems: "Jack", "ALSA", "CoreAudio", "PortAudio",
	 *   "OSS", and "PulseAudio".
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
	void			startAudioDrivers();
	
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
	
	//retrieve the current state of the audio engine state, see #m_State
	int 			getState() const;
	//set the current state of the audio engine state, see #m_State
	void 			setState( int state );

	void 			setMasterPeak_L( float value );
	float 			getMasterPeak_L() const;

	void	 		setMasterPeak_R( float value );
	float 			getMasterPeak_R() const;

	float			getProcessTime() const;
	float			getMaxProcessTime() const;

	int				getSelectedPatternNumber() const;
	void			setSelectedPatternNumber( int number );

	void			setPatternStartTick( int tick );

	void			setPatternTickPosition( int tick );
	int				getPatternTickPosition() const;

	void			setSongPos( int songPos );
	int				getSongPos() const;

	PatternList*	getNextPatterns() const;
	PatternList*	getPlayingPatterns() const;
	
	unsigned long	getRealtimeFrames() const;
	void			setRealtimeFrames( unsigned long nFrames );

	unsigned int	getAddRealtimeNoteTickPosition() const; 
	void			setAddRealtimeNoteTickPosition( unsigned int tickPosition );

	struct timeval& 	getCurrentTickTime();

	void play();
	/** Stop transport without resetting the transport position and
		other internal variables.*/
	void pause();

	friend bool CoreActionController::locateToFrame( unsigned long nFrame );
private:
	
	/** Relocate using the audio driver and update the
	 * #m_fElapsedTime.
	 *
	 * \param nFrame Next transport position in frames.
	 */
	void			locate( unsigned long nFrame );
	/** Local instance of the Sampler. */
	Sampler* 			m_pSampler;
	/** Local instance of the Synth. */
	Synth* 				m_pSynth;

	/**
	 * Pointer to the current instance of the audio driver.
	 *
	 * Inside audioEngine_startAudioDrivers() either the audio driver specified
	 * in Preferences::m_sAudioDriver and created via createDriver() or
	 * the NullDriver, in case the former failed, will be assigned.
	 */	
	AudioOutput *		m_pAudioDriver;

	/**
	 * MIDI input
	 *
	 * In audioEngine_startAudioDrivers() it is assigned the midi driver
	 * specified in Preferences::m_sMidiDriver.
	 */
	MidiInput *			m_pMidiDriver;

	/**
	 * MIDI output
	 *
	 * In audioEngine_startAudioDrivers() it is assigned the midi driver
	 * specified in Preferences::m_sMidiDriver.
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
	float				m_fElapsedTime;

	// time used in process function
	float				m_fProcessTime;

	// max ms usable in process with no xrun
	float				m_fMaxProcessTime;

	// updated in audioEngine_updateNoteQueue()
	struct timeval		m_currentTickTime;

	/**
	 * Index of the pattern selected in the GUI or by a MIDI event.
	 *
	 * If Preferences::m_bPatternModePlaysSelected is set to true and the
	 * playback is in Song::PATTERN_MODE, the corresponding pattern will
	 * be assigned to #m_pPlayingPatterns in
	 * audioEngine_updateNoteQueue(). This way the user can specify to
	 * play back the pattern she is currently viewing/editing.
	 *
	 * Queried using Hydrogen::getSelectedPatternNumber() and set by
	 * Hydrogen::setSelectedPatternNumber().
	 *
	 * Initialized to 0 in audioEngine_init().
	 */
	int					m_nSelectedPatternNumber;

	/**
	 * Beginning of the current pattern in ticks.
	 *
	 * It is set using finPatternInTick() and reset to -1 in
	 * audioEngine_start(), audioEngine_stop(),
	 * Hydrogen::startExportSong(), and
	 * Hydrogen::triggerRelocateDuringPlay() (if the playback it in
	 * Song::PATTERN_MODE).
	 */
	int					m_nPatternStartTick;

	/**
	 * Ticks passed since the beginning of the current pattern.
	 *
	 * Queried using Hydrogen::getTickPosition().
	 */
	unsigned int		m_nPatternTickPosition;

	/**
	 * Index of the current PatternList/column in the
	 * Song::__pattern_group_sequence.
	 *
	 * A value of -1 corresponds to "pattern list could not be found".
	 */
	int					m_nSongPos; // TODO: rename it to something more
									// accurate, like m_nPatternListNumber

	/** Set to the total number of ticks in a Song in findPatternInTick()
		if Song::SONG_MODE is chosen and playback is at least in the
		second loop.*/
	int					m_nSongSizeInTicks;

		/**
	 * Patterns to be played next in Song::PATTERN_MODE.
	 *
	 * In audioEngine_updateNoteQueue() whenever the end of the current
	 * pattern is reached the content of #m_pNextPatterns will be added to
	 * #m_pPlayingPatterns.
	 *
	 * Queried with Hydrogen::getNextPatterns(), set by
	 * Hydrogen::sequencer_setNextPattern() and
	 * Hydrogen::sequencer_setOnlyNextPattern(), initialized with an empty
	 * PatternList in audioEngine_init(), destroyed and set to NULL in
	 * audioEngine_destroy(), cleared in audioEngine_remove_Song(), and
	 * updated in audioEngine_updateNoteQueue(). Please note that ALL of
	 * these functions do access the variable directly!
	 */
	PatternList*		m_pNextPatterns;
	
	/**
	 * PatternList containing all Patterns currently played back.
	 *
	 * Queried using Hydrogen::getCurrentPatternList(), set using
	 * Hydrogen::setCurrentPatternList(), initialized with an empty
	 * PatternList in audioEngine_init(), destroyed and set to NULL in
	 * audioEngine_destroy(), set to the first pattern list of the new
	 * song in audioEngine_setSong(), cleared in
	 * audioEngine_removeSong(), reset in Hydrogen::togglePlaysSelected()
	 * and processed in audioEngine_updateNoteQueue(). Please note that
	 * ALL of these functions do access the variable directly!
	 */
	PatternList*		m_pPlayingPatterns;

	/**
	 * Variable keeping track of the transport position in realtime.
	 *
	 * Even if the audio engine is stopped, the variable will be
	 * incremented  (as audioEngine_process() would do at the beginning
	 * of each cycle) to support realtime keyboard and MIDI event
	 * timing. It is set using Hydrogen::setRealtimeFrames(), accessed via
	 * Hydrogen::getRealtimeFrames(), and updated in
	 * audioEngine_process_transport() using the current transport
	 * position TransportInfo::m_nFrames.
	 */
	unsigned long		m_nRealtimeFrames;
	unsigned int		m_nAddRealtimeNoteTickPosition;

	/**
	 * Current state of the H2Core::AudioEngine. 
	 *
	 * It is supposed to take five different states:
	 *
	 * - #STATE_UNINITIALIZED:	1      Not even the constructors have been called.
	 * - #STATE_INITIALIZED:	2      Not ready, but most pointers are now valid or NULL
	 * - #STATE_PREPARED:		3      Drivers are set up, but not ready to process audio.
	 * - #STATE_READY:			4      Ready to process audio
	 * - #STATE_PLAYING:		5      Currently playing a sequence.
	 * 
	 * It gets initialized with #STATE_UNINITIALIZED.
	 */	
	int					m_State;
	
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

inline struct timeval& AudioEngine::getCurrentTickTime() {
	return m_currentTickTime;
}

inline int AudioEngine::getState() const {
	return m_State;
}

inline void AudioEngine::setState(int state) {
	m_State = state;
}

inline void AudioEngine::setAudioDriver( AudioOutput* pAudioDriver ) {
	m_pAudioDriver = pAudioDriver;
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

inline int AudioEngine::getSelectedPatternNumber() const {
	return m_nSelectedPatternNumber;
}

inline void AudioEngine::setSelectedPatternNumber(int number) {
	m_nSelectedPatternNumber = number;
}

inline void AudioEngine::setPatternStartTick(int tick) {
	m_nPatternStartTick = tick;
}

inline void AudioEngine::setPatternTickPosition(int tick) {
	m_nPatternTickPosition = tick;
}

inline int AudioEngine::getPatternTickPosition() const {
	return m_nPatternTickPosition;
}

inline void AudioEngine::setSongPos( int songPos ) {
	m_nSongPos = songPos;
}

inline int AudioEngine::getSongPos() const {
	return m_nSongPos;
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

};

#endif
