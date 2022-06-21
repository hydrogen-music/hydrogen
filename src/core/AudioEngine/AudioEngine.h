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
 * #m_pSampler and #m_pSynth member objects, takes care of transport
 * control and note processing, and provides a mutex #m_EngineMutex
 * enabling the user to synchronize the access of the Song object and
 * the AudioEngine itself. lock() and try_lock() can be called by a
 * thread to lock the engine and unlock() to make it accessible for
 * other threads once again.
 *
 * The audio engine does not have one but two consistent states with
 * respect it its member variables. #m_fTick, #m_nFrames,
 * #m_fTickOffset, #m_fTickMismatch, #m_fBpm, #m_fTickSize,
 * #m_nFrameOffset, #m_state, and #m_nRealtimeFrames are associated
 * with the current transport position. #m_fLastTickIntervalEnd,
 * #m_nColumn, #m_nPatternSize, #m_nPatternStartTick, and
 * #m_nPatternTickPosition determine the current position
 * updateNoteQueue() is adding notes from #m_pPlayingPatterns into
 * #m_songNoteQueue. Since the latter is ahead of the current
 * transport position by a non-constant (tempo-dependent) lookahead,
 * both states are out of sync while in playback but in sync again
 * once the transport gets relocated (which resets the lookahead). But
 * within themselves both states are consistent.
 *
 * \ingroup docCore docAudioEngine
 */ 
class AudioEngine : public H2Core::TransportInfo, public H2Core::Object<AudioEngine>
{
	H2_OBJECT(AudioEngine)
public:

	/** Audio Engine states.*/
	enum class State {
		/**
		 * Not even the constructors have been called.
		 */
		Uninitialized =	1,
		/**
		 * Not ready, but most pointers are now valid or NULL.
		 */
		Initialized = 2,
		/**
		 * Drivers are set up, but not ready to process audio.
		 */
		Prepared = 3,
		/**
		 * Ready to process audio.
		 */
		Ready = 4,
		/**
		 * Currently playing a sequence.
		 */
		Playing = 5,
		/**
		 * State used during the unit tests of the
		 * AudioEngine. Transport is not rolling but when calling a
		 * function of the process cycle it is ensured all its code
		 * and subsequent functions will be executed.
		 */
		Testing = 6
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
	 * Unlocks the AudioEngine to allow other threads access, and leaves #__locker untouched.
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
	 * - __2__ : Failed to acquire the audio engine lock, no processing took place.
	 * - __1__ : kill the audio driver thread.
	 * - __0__ : else
	 */
	static int                      audioEngine_process( uint32_t nframes, void *arg );

	/**
	 * Calculates the number of frames that make up a tick.
	 */
	static float	computeTickSize( const int nSampleRate, const float fBpm, const int nResolution);
	/**
	 * Calculates a tick equivalent to @a nFrame.
	 *
	 * The function takes all passed tempo markers into account and
	 * depends on the sample rate @a nSampleRate. It also assumes that
	 * sample rate and resolution are constant over the whole song.
	 *
	 * @param nFrame Transport position in frame which should be
	 * converted into ticks.
	 * @param nSampleRate If set to 0, the sample rate provided by the
	 * audio driver will be used.
	 */
	double computeTickFromFrame( long long nFrame, int nSampleRate = 0 ) const;

	/**
	 * Calculates the frame equivalent to @a fTick.
	 *
	 * The function takes all passed tempo markers into account and
	 * depends on the sample rate @a nSampleRate. It also assumes that
	 * sample rate and resolution are constant over the whole song.
	 *
	 * @param fTick Current transport position in ticks.
	 * @param fTickMismatch Since ticks are stored as doubles and there
	 * is some loss in precision, this variable is used report how
	 * much @fTick exceeds/is ahead of the resulting frame.
	 * @param nSampleRate If set to 0, the sample rate provided by the
	 * audio driver will be used.
	 *
	 * @return frame
	 */
	long long computeFrameFromTick( double fTick, double* fTickMismatch, int nSampleRate = 0 ) const;


	/** \return #m_pSampler */
	Sampler*		getSampler() const;
	/** \return #m_pSynth */
	Synth*			getSynth() const;

	/** \return Time passed since the beginning of the song*/
	float			getElapsedTime() const;	

	/** 
	 * Creation and initialization of all audio and MIDI drivers called in
	 * Hydrogen::Hydrogen().
	 */
	void			startAudioDrivers();
	/**
	 * Stops all audio and MIDI drivers.
	 */
	void			stopAudioDrivers();
	AudioOutput*	getAudioDriver() const;
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
	AudioOutput*	createAudioDriver( const QString& sDriver );
					
	void			restartAudioDrivers();
					
	void			setupLadspaFX();
	
	/**
	 * Hands the provided Song to JackAudioDriver::makeTrackOutputs() if
	 * @a pSong is not a null pointer and the audio driver #m_pAudioDriver
	 * is an instance of the JackAudioDriver.
	 * \param pSong Song for which per-track output ports should be generated.
	 */
	void			renameJackPorts(std::shared_ptr<Song> pSong);

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

	long			getPatternTickPosition() const;
	long			getPatternStartTick() const;

	int				getColumn() const;
	long long		getFrameOffset() const;
	void			setFrameOffset( long long nFrameOffset );
	double  		getTickOffset() const;

	const PatternList*	getNextPatterns() const;
	const PatternList*	getPlayingPatterns() const;
	
	long long		getRealtimeFrames() const;

	const struct timeval& 	getCurrentTickTime() const;
	
	/** Maximum lead lag factor in ticks.
	 *
	 * During the humanization the onset of a Note will be moved
	 * Note::__lead_lag times the value calculated by this function.
	 */
	static double	getLeadLagInTicks();
	
	/** Calculates lead lag factor (in frames) relative to the
	 * transport position @a fTick
	 *
	 * During the humanization the onset of a Note will be moved
	 * Note::__lead_lag times the value calculated by this function.
	 */
	long long		getLeadLagInFrames( double fTick );
	/** Calculates time offset (in frames) the AudioEngine is ahead of
	 * the transport position @a fTick.
	 *
	 * Due to the humanization there might be negative offset in the
	 * position of a particular note. To be able to still render it
	 * appropriately, we have to look into and handle notes from the
	 * future.
	 *
	 * Since the tick size (and thus the lead lag factor in frames)
	 * can change at an arbitrary point if the Timeline is activated,
	 * the lookahead will be calculated relative to @a fTick.
	 *
	 * \return Frame offset*/
	long long getLookaheadInFrames( double fTick );

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
	 * adopted during the next processing cycle.*/
	void setNextBpm( float fNextBpm );
	float getNextBpm() const;

	/** Compatibility layer for external classes pretending that ticks
		are still integer.*/
	long getTick() const;

	static float 	getBpmAtColumn( int nColumn );

	/**
	 * Function to be called every time length of the current song
	 * does change, e.g. by toggling a pattern or altering its length.
	 *
	 * It will adjust both the current transport information as well
	 * as the note queues in order to prevent any glitches.
	 */
	void updateSongSize();

	void removePlayingPattern( int nIndex );
	/**
	 * Update the list of patterns currently played back.
	 *
	 * This works in three different ways.
	 *
	 * 1. In case the song is in Song::Mode::Song when entering a new
	 * @a nColumn #m_pPlayingPatterns will be flushed and all patterns
	 * activated in the provided column will be added.
	 * 2. While in Song::PatternMode::Selected the function
	 * ensures the currently selected pattern is the only pattern in
	 * #m_pPlayingPatterns.
	 * 3. While in Song::PatterMode::Stacked all patterns
	 * in #m_pNextPatterns not already present in #m_pPlayingPatterns
	 * will be added in the latter and the ones already present will
	 * be removed.
	 *
	 * \param nColumn Desired location in song mode.
	 * \param nTick Desired location in pattern mode.
	 */
	void updatePlayingPatterns( int nColumn, long nTick = 0 );
	/** 
	 * Add pattern @a nPatternNumber to #m_pNextPatterns or deletes it
	 * in case it is already present.
	 */
	void toggleNextPattern( int nPatternNumber );
	/**
	 * Add pattern @a nPatternNumber to #m_pNextPatterns as well as
	 * the whole content of #m_pPlayingPatterns. After the next call
	 * to updatePlayingPatterns() only @a nPatternNumber will be left
	 * playing.
	 */
	void flushAndAddNextPattern( int nPatternNumber );

	/**
	 * Updates the transport state and all notes in #m_songNoteQueue
	 * after adding or deleting a TempoMarker or enabling/disabling
	 * the #Timeline.
	 *
	 * If the #Timeline is activated, adding or removing a TempoMarker
	 * does effectively has the same effects as a relocation with
	 * respect to the transport position in frames. It's tick
	 * counterpart, however, is not affected. This function ensures
	 * they are in sync again.
	 *
	 * Updates all notes in #m_songNoteQueue to be still valid after a
	 * tempo change.
	 *
	 * See handleTimelineChange().
	 */
	void handleTimelineChange();

	/** 
	 * Unit test checking for consistency when converting frames to
	 * ticks and back.
	 *
	 * @return true on success.
	 */
	bool testFrameToTickConversion();
	/** 
	 * Unit test checking the incremental update of the transport
	 * position in audioEngine_process().
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	bool testTransportProcessing();
	/** 
	 * Unit test checking the relocation of the transport
	 * position in audioEngine_process().
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	bool testTransportRelocation();
	/** 
	 * Unit test checking consistency of tick intervals processed in
	 * updateNoteQueue() (no overlap and no holes).
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	bool testComputeTickInterval();
	/** 
	 * Unit test checking consistency of transport position when
	 * playback was looped at least once and the song size is changed
	 * by toggling a pattern.
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	bool testSongSizeChange();
	/** 
	 * Unit test checking consistency of transport position when
	 * playback was looped at least once and the song size is changed
	 * by toggling a pattern.
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	bool testSongSizeChangeInLoopMode();
	/** 
	 * Unit test checking that all notes in a song are picked up once.
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	bool testNoteEnqueuing();
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix, bool bShort = true ) const override;

	/** Is allowed to call setSong().*/
	friend void Hydrogen::setSong( std::shared_ptr<Song> pSong );
	/** Is allowed to call removeSong().*/
	friend void Hydrogen::removeSong();
	/** Is allowed to use locate() to directly set the position in
		frames as well as to used setColumn and setPatternTickPos to
		move the arrow in the SongEditorPositionRuler even when
		playback is stopped.*/
	friend void Hydrogen::updateSelectedPattern( bool );
	friend bool CoreActionController::locateToTick( long nTick, bool );
	/** Is allowed to set m_state to State::Ready via setState()*/
	friend int FakeDriver::connect();
	friend void JackAudioDriver::updateTransportInfo();
	friend void JackAudioDriver::relocateUsingBBT();
private:

	/**
	 * Sets the Hydrogen::m_nSelectedPatternNumber to the pattern
	 * recorded notes will be inserted in.
	 */
	void handleSelectedPattern();
	
	inline void			processPlayNotes( unsigned long nframes );
	/**
	 * Converts a tick into frames under the assumption of a constant
	 * @a fTickSize since the beginning of the song (sample rate,
	 * tempo, and resolution did not change).
	 *
	 * As the assumption above usually does not hold,
	 * computeFrameFromTick() should be used instead while this
	 * function is only meant for internal use.
	 */
	static long long computeFrame( double fTick, float fTickSize );
	/**
	 * Converts a frame into ticks under the assumption of a constant
	 * @a fTickSize since the beginning of the song (sample rate,
	 * tempo, and resolution did not change).
	 *
	 * As the assumption above usually does not hold,
	 * computeTickFromFrame() should be used instead while this
	 * function is only meant for internal use.
	 */
	static double computeTick( long long nFrame, float fTickSize );
	
	/** Resets a number of member variables to their initial state.
	 *
	 * This is used to allow a smooth transition between the Song and
	 * Pattern Mode.
	 * \param bWithJackBroadcast Relocate not using the AudioEngine
	 * directly but using the JACK server.
	 */
	void reset(  bool bWithJackBroadcast = true );

	double getDoubleTick() const;
	static double computeDoubleTickSize(const int nSampleRate, const float fBpm, const int nResolution);

	void			clearNoteQueue();
	/** Clear all audio buffers.
	 */
	void			clearAudioBuffers( uint32_t nFrames );
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
	 */
	int				updateNoteQueue( unsigned nFrames );
	void 			processAudio( uint32_t nFrames );
	long long 		computeTickInterval( double* fTickStart, double* fTickEnd, unsigned nFrames );
    
	void			updateBpmAndTickSize();
	
	void			setPatternTickPosition( long nTick );
	void			setColumn( int nColumn );
	void			setRealtimeFrames( long long nFrames );
	
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
	State 			getNextState() const;

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
	 * \param fTick Next transport position in ticks.
	 * \param bWithJackBroadcast Relocate not using the AudioEngine
	 * directly but using the JACK server.
	 */
	void			locate( const double fTick, bool bWithJackBroadcast = true );
	/**
	 * Version of the locate() function intended to be directly used
	 * by frame-based audio drivers / servers.
	 *
	 * @param nFrame Next position in frames. If the provided number
	 * is larger than the song length and loop mode is enabled,
	 * computeTickFromFrame() will wrap it.
	 */
	void			locateToFrame( const long long nFrame );
	void			incrementTransportPosition( uint32_t nFrames );
	void			updateTransportPosition( double fTick );
	void			updateSongTransportPosition( double fTick );
	void			updatePatternTransportPosition( double fTick );

	/**
	 * Updates all notes in #m_songNoteQueue to be still valid after a
	 * tempo change.
	 *
	 * This function will only be used with the #Timeline
	 * disabled. See handleTimelineChange().
	 */
	void handleTempoChange();
	/**
	 * Updates all notes in #m_songNoteQueue to be still valid after a
	 * change in song size.
	 */
	void handleSongSizeChange();

	/**
	 * The audio driver was changed what possible changed the tick
	 * size - which depends on both the sample rate - too. Thus, all
	 * frame-based variables might have become invalid.
	 */
	void handleDriverChange();
	
	/** Helper function */
	bool testCheckTransportPosition( const QString& sContext ) const;
	/**
	 * Takes two instances of Sampler::m_playingNotesQueue and checks
	 * whether matching notes have exactly @a nPassedFrames difference
	 * in their SelectedLayerInfo::SamplePosition.
	 */
	bool testCheckAudioConsistency( const std::vector<std::shared_ptr<Note>> oldNotes,
									const std::vector<std::shared_ptr<Note>> newNotes,
									const QString& sContext,
									int nPassedFrames,
									bool bTestAudio = true,
									float fPassedTicks = 0.0 ) const;
	/**
	 * Toggles the grid cell defined by @a nToggleColumn and @a
	 * nToggleRow twice and checks whether the transport position and
	 * the audio processing remains consistent.
	 */
	bool testCheckConsistency( int nToggleColumn, int nToggleRow, const QString& sContext );
	
	std::vector<std::shared_ptr<Note>> testCopySongNoteQueue();
	/**
	 * Add every Note in @a newNotes not yet contained in @a noteList
	 * to the latter.
	 */
	void testMergeQueues( std::vector<std::shared_ptr<Note>>* noteList,
						  std::vector<std::shared_ptr<Note>> newNotes );
	void testMergeQueues( std::vector<std::shared_ptr<Note>>* noteList,
						  std::vector<Note*> newNotes );

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
	} m_pLocker;

	// time used in process function
	float				m_fProcessTime;

	// max ms usable in process with no xrun
	float				m_fMaxProcessTime;

	// time used to render audio produced byy LADSPA plugins
	float				m_fLadspaTime;

	// updated in audioEngine_updateNoteQueue()
	struct timeval		m_currentTickTime;

	/**
	 * Beginning of the currently playing patterns
	 * (#m_pPlayingPatterns) in ticks.
	 *
	 * Attention: This value can be larger than m_fTick. If transport
	 * is rolling, the playing patterns are updated in
	 * updateNoteQueue() using a lookahead which allows for notes to
	 * be placed not just ahead of time but also back in time using
	 * humanization.
	 *
	 * The current transport position thus corresponds
	 * to #m_fTick = lookahead + #m_nPatternStartTick +
	 * #m_nPatternTickPosition. (The lookahead is both speed and
	 * sample rate dependent).
	 */
	long				m_nPatternStartTick;

	/**
	 * Ticks passed since #m_nPatternStartTick.
	 *
	 * The current transport position thus corresponds
	 * to #m_fTick = lookahead + #m_nPatternStartTick +
	 * #m_nPatternTickPosition. (The lookahead is both speed and
	 * sample rate dependent).
	 */
	long				m_nPatternTickPosition;

	/**
	 * Cached information to determine the end of the currently
	 * playing pattern in ticks (see #m_pPlayingPatterns).
	 */
	int m_nPatternSize;
	/**
	 * Coarse-grained version of #m_nPatternStartTick which can be
	 * used as the index of the current PatternList/column in the
	 * Song::__pattern_group_sequence.
	 *
	 * A value of -1 corresponds to "pattern list could not be found"
	 * and is used to indicate that transport reached the end of the
	 * song (with transport not looped).
	 */
	int					m_nColumn;

	/** Set to the total number of ticks in a Song.*/
	double				m_fSongSizeInTicks;

	/**
	 * Patterns to be played next in stacked Song::Mode::Pattern mode.
	 *
	 * See updatePlayingPatterns() for details.
	 */
	PatternList*		m_pNextPatterns;
	
	/**
	 * PatternList containing all Patterns currently played back.
	 *
	 * See updatePlayingPatterns() for details.
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
	long long		m_nRealtimeFrames;

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
	 */
	static const int		nMaxTimeHumanize;

	float 			m_fNextBpm;
	/** Number of frames TransportInfo::m_nFrames is ahead of
		TransportInfo::m_nTick. */
	double m_fTickMismatch;
	double m_fTickOffset;
	long long m_nFrameOffset;
	double m_fLastTickIntervalEnd;

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

inline AudioEngine::State AudioEngine::getNextState() const {
	return m_nextState;
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

inline long AudioEngine::getPatternStartTick() const {
	return m_nPatternStartTick;
}
inline void AudioEngine::setPatternTickPosition( long nTick ) {
	m_nPatternTickPosition = nTick;
}

inline long AudioEngine::getPatternTickPosition() const {
	return m_nPatternTickPosition;
}

inline void AudioEngine::setColumn( int songPos ) {
	m_nColumn = songPos;
}

inline int AudioEngine::getColumn() const {
	return m_nColumn;
}

inline const PatternList* AudioEngine::getPlayingPatterns() const {
	return m_pPlayingPatterns;
}

inline const PatternList* AudioEngine::getNextPatterns() const {
	return m_pNextPatterns;
}

inline long long AudioEngine::getRealtimeFrames() const {
	return m_nRealtimeFrames;
}

inline void AudioEngine::setRealtimeFrames( long long nFrames ) {
	m_nRealtimeFrames = nFrames;
}

inline float AudioEngine::getNextBpm() const {
	return m_fNextBpm;
}
inline long long AudioEngine::getFrameOffset() const {
	return m_nFrameOffset;
}
inline void AudioEngine::setFrameOffset( long long nFrameOffset ) {
	m_nFrameOffset = nFrameOffset;
}
inline double AudioEngine::getTickOffset() const {
	return m_fTickOffset;
}
};

#endif
