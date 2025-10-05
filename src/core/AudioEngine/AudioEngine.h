/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/AudioEngine/AudioEngineTests.h>
#include <core/Basics/Event.h>
#include <core/config.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/IO/AudioOutput.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/FakeAudioDriver.h>
#include <core/IO/JackAudioDriver.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>


#include <cassert>
#include <chrono>
#include <deque>
#include <QString>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>

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
	class Drumkit;
	class Instrument;
	class MidiBaseDriver;
	class Note;
	class PatternList;
	class Song;
	class TransportPosition;
	
/**
 * The audio engine deals with two distinct #TransportPosition. The
 * first (and most important one) is #m_pTransportPosition which
 * indicated the current position of the audio rendering and playhead
 * as well as all patterns associated with it. This is also the one
 * other parts of Hydrogen are concerned with.
 *
 * The second one is #m_pQueuingPosition which is only used
 * internally. It is one lookahead ahead of #m_pTransportPosition,
 * used for inserting notes into the song queue, and required in order
 * to supported lead and lag of notes. Formerly, this second transport
 * state was trimmed to a couple of variables making its update less
 * expensive. However, this showed to be quite error prone as things
 * tend to went out of sync.
 *
 * All tick information (apart from note handling in
 * updateNoteQueue()) are handled as double internally. But due to
 * historical reasons the GUI and the remainder of the core only
 * access a version of the current tick rounded to integer.
 *
 * \ingroup docCore docAudioEngine
 */ 
class AudioEngine : public H2Core::Object<AudioEngine>
{
	H2_OBJECT(AudioEngine)
public:

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
		/** Transport rolling yet. But a specific number of metronome */
		CountIn = 5,
		/**
		 * Transport is rolling.
		 */
		Playing = 6,
		/**
		 * State used during the unit tests of the
		 * AudioEngine. Transport is not rolling but when calling a
		 * function of the process cycle it is ensured all its code
		 * and subsequent functions will be executed.
		 */
		Testing = 7
	};
		static QString StateToQString( const State& state );

	/**
	 * Maximum value the standard deviation of the Gaussian
	 * distribution the random velocity contribution will be drawn
	 * from can take.
	 *
	 * The actual standard deviation used during processing is this
	 * value multiplied with #Song::m_fHumanizeVelocityValue.
	 */
	static constexpr float fHumanizeVelocitySD = 0.2;
	/**
	 * Maximum value the standard deviation of the Gaussian
	 * distribution the random pitch contribution will be drawn from
	 * can take.
	 *
	 * The actual standard deviation used during processing is this
	 * value multiplied with #Instrument::__random_pitch_factor of the
	 * instrument associated with the particular #Note.
	 */
	static constexpr float fHumanizePitchSD = 0.4;
	/**
	 * Maximum value the standard deviation of the Gaussian
	 * distribution the random pitch contribution will be drawn from
	 * can take.
	 *
	 * The actual standard deviation used during processing is this
	 * value multiplied with #Instrument::__random_pitch_factor of the
	 * instrument associated with the particular #Note.
	 */
	static constexpr float fHumanizeTimingSD = 0.3;
	/**
	 * Maximum time (in frames) a note's position can be off due to
	 * the humanization (lead-lag).
	 */
	static constexpr int nMaxTimeHumanize = 2000;

	AudioEngine();

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
	bool			tryLockFor( const std::chrono::microseconds& duration,
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
	void			assertLocked( const QString& sClass, const char* sFunction,
								  const QString& sMsg );
	void			noteOn( std::shared_ptr<Note> pNote );

	/**
	 * Main audio processing function called by the audio drivers whenever
	 * there is work to do.
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
	static float	computeTickSize( const int nSampleRate, const float fBpm );
	static double computeDoubleTickSize(const int nSampleRate, const float fBpm );

	Sampler*		getSampler() const;

	/** \return Time passed since the beginning of the song*/
	float			getElapsedTime() const;	

	/** * Choice, creation, and initialization of th audio driver. */
	void			startAudioDriver( Event::Trigger trigger );
	void			stopAudioDriver( Event::Trigger trigger );
	AudioOutput*	getAudioDriver() const;
	/**
	 * Create an audio driver using audioEngine_process() as its argument
	 * based on the provided choice and calling their _init()_ function to
	 * trigger their initialization.
	 *
	 * For a listing of all possible choices, please see
	 * #H2Core::Preferences::AudioDriver.
	 *
	 * \param driver Specific audio driver.
	 * \return Pointer to the freshly created audio driver. If the
	 * creation resulted in a NullDriver, the corresponding object will be
	 * deleted and a null pointer returned instead.
	 */
	AudioOutput*	createAudioDriver( const Preferences::AudioDriver& driver,
									   Event::Trigger trigger );
					
		void startMidiDriver( Event::Trigger trigger );
		void stopMidiDriver( Event::Trigger trigger );
		std::shared_ptr<MidiBaseDriver> getMidiDriver() const;

	void			setupLadspaFX();


	std::shared_ptr<Instrument> getMetronomeInstrument() const;
		
	
	void raiseError( unsigned nErrorCode );
	
	const State& 	getState() const;

	void 			setMasterPeak_L( float value );
	float 			getMasterPeak_L() const;

	void	 		setMasterPeak_R( float value );
	float 			getMasterPeak_R() const;

	float			getProcessTime() const;
	float			getMaxProcessTime() const;

	const std::shared_ptr<TransportPosition> getTransportPosition() const;

	const std::shared_ptr<PatternList>	getNextPatterns() const;
	const std::shared_ptr<PatternList>	getPlayingPatterns() const;
	
	long long		getRealtimeFrame() const;

	/** Maximum lead lag factor in ticks.
	 *
	 * During humanization the onset of a Note will be moved
	 * Note::__lead_lag times the value calculated by this function.
	 */
	static double	getLeadLagInTicks();
	
	/** Calculates lead lag factor (in frames) relative to the
	 * transport position @a fTick
	 *
	 * During the humanization the onset of a Note will be moved
	 * Note::__lead_lag times the value calculated by this function.
	 */
	static long long getLeadLagInFrames( double fTick );

	double getSongSizeInTicks() const;

		int getCountInMetronomeTicks() const;

	/**
	 * Marks the audio engine to be started during the next call of
	 * the audioEngine_process() callback function.
	 *
	 * If the JACK audio driver is used, a request to start transport
	 * is send to the JACK server instead.
	 */
	void play();
	/**
	 * Marks the audio engine to be stopped during the next call of
	 * the audioEngine_process() callback function.
	 *
	 * If the JACK audio driver is used, a request to stop transport
	 * is send to the JACK server instead.
	 */
	void stop();

	/** Stores the new speed into a separate variable which will be
	 * adopted during the next processing cycle.
	 *
	 * Setting this variable requires the audio engine to be locked!
	 * (Else, tempo handling within audioEngine_process() might be
	 * inconsistent and cause the playhead to glitch).
	 */
	void setNextBpm( float fNextBpm );
	float getNextBpm() const;

	static float 	getBpmAtColumn( int nColumn );

	/**
	 * Function to be called every time the length of the current song
	 * does change, e.g. by toggling a pattern or altering its length.
	 *
	 * It will adjust both the current transport information as well
	 * as the note queues in order to prevent any glitches.
	 */
	void updateSongSize( Event::Trigger trigger = Event::Trigger::Default );

	void removePlayingPattern( std::shared_ptr<Pattern> pPattern );
	/**
	 * Update the list of currently played patterns associated with
	 * #m_pTransportPosition and #m_pQueuingPosition.
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
	 */
	void updatePlayingPatterns( Event::Trigger trigger );
	void clearNextPatterns();
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

	void updateVirtualPatterns();

	/**
	 * Returns the size of #m_songNoteQueue.
	 *
	 * Required to not end unit tests prematurely.
	 */
	int getEnqueuedNotesNumber() const;

	/** Stops all playback, transport, and note rendering and set the engine
		 * in #State::Prepared. (It is needs some interaction/configuration in
		 * order to start again.) */
	void			prepare( Event::Trigger trigger );
	bool			isEndOfSongReached( std::shared_ptr<TransportPosition> pPos ) const;

		void makeTrackPorts( std::shared_ptr<Song> pSong,
							 std::shared_ptr<Drumkit> pOldDrumkit = nullptr );

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

		/** Is allowed to start playback. */
		friend void Hydrogen::restartAudioDriver();
	/** Is allowed to call setSong().*/
	friend void Hydrogen::setSong( std::shared_ptr<Song> pSong );
	/** Is allowed to use locate() to directly set the position in
		frames as well as to used setColumn and setPatternTickPos to
		move the arrow in the SongEditorPositionRuler even when
		playback is stopped.*/
	friend void Hydrogen::updateSelectedPattern( bool );
	/** Uses handleTimelineChange() */
	friend void Hydrogen::setIsTimelineActivated( bool );
	/** Uses handleTimelineChange() */
	friend bool CoreActionController::addTempoMarker( int, float );
	/** Uses handleTimelineChange() */
	friend bool CoreActionController::deleteTempoMarker( int );
	friend bool CoreActionController::locateToTick( long nTick, bool );
	friend bool CoreActionController::activateSongMode( bool );
	friend bool CoreActionController::activateLoopMode( bool );
	friend bool CoreActionController::setDrumkit( std::shared_ptr<Drumkit> );
	friend bool CoreActionController::removeInstrument(
		std::shared_ptr<Instrument> );
	friend bool CoreActionController::replaceInstrument(
		std::shared_ptr<Instrument>, std::shared_ptr<Instrument> );
	friend bool CoreActionController::startCountIn();
	/** Is allowed to set m_state to State::Ready via setState()*/
	friend int FakeAudioDriver::connect();

	friend class AudioEngineTests;
		friend class JackAudioDriver;
private:

	/**
	 * Keeps the selected pattern in line with the one the transport
	 * position resides in while in Song::Mode::Song.
	 *
	 * If multiple patterns are present in the current column, the pattern
	 * recorded notes will be inserted in (bottom-most one) will be used.
	 */
	void handleSelectedPattern( Event::Trigger trigger = Event::Trigger::Force );
	
	inline void			processPlayNotes( unsigned long nframes );

	void reset( bool bWithJackBroadcast = true,
				Event::Trigger trigger = Event::Trigger::Default );

	void resetOffsets();

	/**
	 * Ideally we just floor the provided tick. When relocating to a
	 * specific tick, it's converted counterpart is stored as the
	 * transport position in frames, which is then used to calculate
	 * the tick start again. These conversions back and forth can
	 * introduce rounding error that get larger for larger tick
	 * numbers and could result in a computed start tick of
	 * 86753.999999934 when transport was relocated to 86754. As we do
	 * not want to cover notes prior to our current transport
	 * position, we have to account for such rounding errors.
	 */
	double coarseGrainTick( double fTick );

		/** Flush the incoming MIDI note queue and song note queue.
		 *
		 * @param pInstrument particular instrument for which notes will be
		 *   removed (`nullptr` to release them all) */
	void			clearNoteQueues(
		std::shared_ptr<Instrument> pInstrument = nullptr );
	/** Clear all audio buffers.
	 */
	void			clearAudioBuffers( uint32_t nFrames );
	/**
	 * Takes all notes from the currently playing patterns, from the
	 * MIDI queue #m_midiNoteQueue, and those triggered by the
	 * metronome and pushes them onto #m_songNoteQueue for playback.
	 */
	void			updateNoteQueue( unsigned nIntervalLengthInFrames );
	void 			processAudio( uint32_t nFrames );
	long long 		computeTickInterval( double* fTickStart, double* fTickEnd, unsigned nIntervalLengthInFrames );
	void			updateBpmAndTickSize( std::shared_ptr<TransportPosition> pTransportPosition,
										  Event::Trigger trigger = Event::Trigger::Default );
	void			calculateTransportOffsetOnBpmChange(
		std::shared_ptr<TransportPosition> pTransportPosition,
		float fTickSizeOld, float fTickSizeNew );

	void			setRealtimeFrame( long long nFrame );
	void updatePlayingPatternsPos( std::shared_ptr<TransportPosition> pPos,
								   Event::Trigger trigger );
	
	void			setSong( std::shared_ptr<Song>pNewSong );
	void 			setState( const State& state,
							  Event::Trigger trigger = Event::Trigger::Default );
	void 			setNextState( const State& state );

	void				startPlayback();
	
	void			stopPlayback( Event::Trigger trigger = Event::Trigger::Default );
	
	void			locate( const double fTick, bool bWithJackBroadcast = true,
							Event::Trigger trigger = Event::Trigger::Default );
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
	void			updateTransportPosition( double fTick, long long nFrame,
											 std::shared_ptr<TransportPosition> pPos,
											 Event::Trigger trigger = Event::Trigger::Default );
	void			updateSongTransportPosition( double fTick, long long nFrame,
												 std::shared_ptr<TransportPosition> pPos,
												 Event::Trigger trigger = Event::Trigger::Force );
	void			updatePatternTransportPosition( double fTick, long long nFrame,
													std::shared_ptr<TransportPosition> pPos,
													Event::Trigger trigger = Event::Trigger::Default );

		void startCountIn();

	/**
	 * Updates all notes in #m_songNoteQueue and #m_midiNoteQueue to
	 * be still valid after a tempo change.
	 */
	void handleTempoChange();

	/**
	 * Updates the transport states and all notes in #m_songNoteQueue
	 * and #m_midiNoteQueue after adding or deleting a TempoMarker or
	 * enabling/disabling the #Timeline.
	 *
	 * If the #Timeline is activated, adding or removing a TempoMarker
	 * does effectively has the same effects as a relocation with
	 * respect to the transport position in frames. It's tick
	 * counterpart, however, is not affected. This function ensures
	 * they are in sync again.
	 */
	void handleTimelineChange();
	
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

	/** In order to properly support #H2Core::Song::LoopMode::Finishing -
	 * transport was already looped a couple of times and the user is pressing
	 * the loop button again to deactivate loop mode - we have to capture the
	 * number of loops already applied. */
	void handleLoopModeChanged();

	/**
	 * Called whenever Hydrogen switches from #Song::Mode::Song into
	 * #Song::Mode::Pattern or the other way around.
	 */
	void handleSongModeChanged( Event::Trigger trigger );

	QString getDriverNames() const;

	Sampler* 			m_pSampler;
	AudioOutput *		m_pAudioDriver;
	std::shared_ptr<MidiBaseDriver>		m_pMidiDriver;

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
	 * Contains the current or last context in which the audio engine
	 * was locked as well as the locking state.
	 */
	struct _locker_struct {
		const char* file;
		unsigned int line;
		const char* function;
		bool isLocked;
	} m_pLocker;


		/** In milliseconds. */
	float				m_fProcessTime;
		/** In milliseconds. */
	float				m_fMaxProcessTime;
		/** In milliseconds. */
	float				m_fLadspaTime;

	std::shared_ptr<TransportPosition> m_pTransportPosition;
	std::shared_ptr<TransportPosition> m_pQueuingPosition;

	/** Set to the total number of ticks in a Song.*/
	double				m_fSongSizeInTicks;

	/**
	 * Variable keeping track of the transport position in realtime.
	 *
	 * Even if the audio engine is stopped, the variable will be
	 * incremented (as audioEngine_process() would do at the beginning
	 * of each cycle) to support realtime keyboard and MIDI event
	 * timing.
	 *
	 * This member is monotonically increasing regardless of ongoing tempo, song
	 * size, etc. changes.
	 */
	long long		m_nRealtimeFrame;
	/**
	 * Version of #m_nRealtimeFrame which is rescaled upon tempo changes.
	 *
	 * If tempo changes, the tick size (number of frames per tick) is changed as
	 * well. Since Hydrogen's #AudioEngine is based on ticks, all frame-based
	 * values do have to be rescaled in order to still represent the same
	 * position (in ticks) as before. #m_nRealtimeFrame itself does not
	 * correspond to any position in the song, timeline etc. It is just a
	 * monotonically increasing number used to handle realtime events, like
	 * MIDI, virtual keyboard, or OSC. But when relying on it for #H2Core::Note
	 * placement, as when counting in, we have to properly rescale it too in
	 * order for our notes to still end up in the right position after the user
	 * changes tempo. */
	long long		m_nRealtimeFrameScaled;

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
		bool operator() ( std::shared_ptr<Note> pNote1,
						  std::shared_ptr<Note> pNote2 );
	};

	std::priority_queue<std::shared_ptr<Note>,
		std::deque<std::shared_ptr<Note>>, compare_pNotes > m_songNoteQueue;
	std::deque<std::shared_ptr<Note>>	m_midiNoteQueue;	///< Midi Note FIFO
	
	/**
	 * Pointer to the metronome.
	 */
	std::shared_ptr<Instrument>		m_pMetronomeInstrument;

	float 			m_fNextBpm;
	double m_fLastTickEnd;
	bool m_bLookaheadApplied;

	/** Indicates how many loops the transport already did when the user presses
	 * the Loop button again. */
	int m_nLoopsDone;

		/** Count in stuff.
		 * @{
		 * How many metronome notes we have already issued. Used to emphasize
		 * the first one. */
		int m_nCountInMetronomeTicks;
		/** At this tick the count in will be started. It also serves as a
		 * references for calculating which tick is supposed to be paired with a
		 * metronome note. */
		long m_nCountInStartTick;
		/** First tick which will be _not_ included into the count in. */
		long m_nCountInEndTick;
		double m_fCountInTickInterval;
		float m_fCountInTickSizeStart;
		long long m_nCountInFrameOffset;
		/** Up to which realtime frame #m_nRealtimeFrame we will continue to
		 * count in.
		 * @} */
		long long m_nCountInEndFrame;
};

#ifdef H2CORE_HAVE_DEBUG
  #define ASSERT_AUDIO_ENGINE_LOCKED(x) assertAudioEngineLocked( _class_name(), __FUNCTION__, QString( "%1" ).arg( x ) );
#else
  #define ASSERT_AUDIO_ENGINE_LOCKED(x)
#endif

/**
 * This is a base class for data structures which should only be modified or
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
	void assertAudioEngineLocked( const QString& sClass,
								  const char* sFunction,
								  const QString& sMsg ) const;


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

inline const AudioEngine::State& AudioEngine::getState() const {
	return m_state;
}
inline void AudioEngine::setNextState( const AudioEngine::State& state) {
	m_nextState = state;
}

inline AudioOutput*	AudioEngine::getAudioDriver() const {
	return m_pAudioDriver;
}

inline std::shared_ptr<MidiBaseDriver>	AudioEngine::getMidiDriver() const {
	return m_pMidiDriver;
}

inline long long AudioEngine::getRealtimeFrame() const {
	return m_nRealtimeFrame;
}

inline void AudioEngine::setRealtimeFrame( long long nFrame ) {
	m_nRealtimeFrame = nFrame;
}

inline float AudioEngine::getNextBpm() const {
	return m_fNextBpm;
}
inline const std::shared_ptr<TransportPosition> AudioEngine::getTransportPosition() const {
	return m_pTransportPosition;
}
inline double AudioEngine::getSongSizeInTicks() const {
	return m_fSongSizeInTicks;
}
inline int AudioEngine::getCountInMetronomeTicks() const {
	return m_nCountInMetronomeTicks;
}
inline std::shared_ptr<Instrument> AudioEngine::getMetronomeInstrument() const {
	return m_pMetronomeInstrument;
}
inline int AudioEngine::getEnqueuedNotesNumber() const {
	return m_songNoteQueue.size();
}
};

#endif
