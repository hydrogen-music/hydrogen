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
#include "hydrogen/config.h"

#ifdef WIN32
#    include "hydrogen/timehelper.h"
#else
#    include <unistd.h>
#    include <sys/time.h>
#endif

#include <pthread.h>
#include <cassert>
#include <cstdio>
#include <deque>
#include <queue>
#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <hydrogen/event_queue.h>
#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/playlist.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/automation_path.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/fx/LadspaFX.h>
#include <hydrogen/fx/Effects.h>

#include <hydrogen/Preferences.h>
#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/midi_map.h>
#include <hydrogen/timeline.h>

#ifdef H2CORE_HAVE_OSC
#include <hydrogen/nsm_client.h>
#include <hydrogen/osc_server.h>
#endif

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/jack_audio_driver.h>
#include <hydrogen/IO/NullDriver.h>
#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/IO/MidiOutput.h>
#include <hydrogen/IO/CoreMidiDriver.h>
#include <hydrogen/IO/TransportInfo.h>
#include <hydrogen/IO/OssDriver.h>
#include <hydrogen/IO/FakeDriver.h>
#include <hydrogen/IO/AlsaAudioDriver.h>
#include <hydrogen/IO/PortAudioDriver.h>
#include <hydrogen/IO/DiskWriterDriver.h>
#include <hydrogen/IO/AlsaMidiDriver.h>
#include <hydrogen/IO/JackMidiDriver.h>
#include <hydrogen/IO/PortMidiDriver.h>
#include <hydrogen/IO/CoreAudioDriver.h>
#include <hydrogen/IO/PulseAudioDriver.h>

namespace H2Core
{

// GLOBALS

// info
float				m_fMasterPeak_L = 0.0f;		///< Master peak (left channel)
float				m_fMasterPeak_R = 0.0f;		///< Master peak (right channel)
float				m_fProcessTime = 0.0f;		///< time used in process function
float				m_fMaxProcessTime = 0.0f;	///< max ms usable in process with no xrun
//~ info

/**
 * Fallback speed in beats per minute.
 *
 * It is set by Hydrogen::setNewBpmJTM() and accessed via
 * Hydrogen::getNewBpmJTM().
 */
float				m_fNewBpmJTM = 120;

/**
 * Pointer to the current instance of the audio driver.
 *
 * Initialized with NULL inside audioEngine_init(). Inside
 * audioEngine_startAudioDrivers() either the audio driver specified
 * in Preferences::m_sAudioDriver and created via createDriver() or
 * the NullDriver, in case the former failed, will be assigned.
 */	
AudioOutput *			m_pAudioDriver = nullptr;
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
MidiInput *			m_pMidiDriver = nullptr;
/**
 * MIDI output
 *
 * In audioEngine_startAudioDrivers() it is assigned the midi driver
 * specified in Preferences::m_sMidiDriver.
 */
MidiOutput *			m_pMidiDriverOut = nullptr;

// overload the > operator of Note objects for priority_queue
struct compare_pNotes {
	bool operator() (Note* pNote1, Note* pNote2) {
		return (pNote1->get_humanize_delay()
				+ pNote1->get_position() * m_pAudioDriver->m_transport.m_fTickSize)
			    >
			    (pNote2->get_humanize_delay()
			    + pNote2->get_position() * m_pAudioDriver->m_transport.m_fTickSize);
	}
};

/// Song Note FIFO
std::priority_queue<Note*, std::deque<Note*>, compare_pNotes > m_songNoteQueue;
std::deque<Note*>		m_midiNoteQueue;	///< Midi Note FIFO

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
bool				m_bAppendNextPattern;		///< Add the next pattern to the list instead of replace.
bool				m_bDeleteNextPattern;		///< Delete the next pattern from the list.
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
PatternList*			m_pPlayingPatterns;
/**
 * Index of the current PatternList in the
 * Song::__pattern_group_sequence.
 *
 * A value of -1 corresponds to "pattern list could not be found".
 *
 * Assigned using findPatternInTick() in
 * audioEngine_updateNoteQueue(), queried using
 * Hydrogen::getPatternPos() and set using Hydrogen::setPatternPos()
 * if it AudioEngine is playing.
 *
 * It is initialized with -1 value in audioEngine_init(), and reset to
 * the same value in audioEngine_start(), and
 * Hydrogen::stopExportSong(). In Hydrogen::startExportSong() it will
 * be set to 0. Please note that ALL of these functions do access the
 * variable directly!
 */
int				m_nSongPos; // TODO: rename it to something more
							// accurate, like m_nPatternListNumber

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
int				m_nSelectedPatternNumber;
/**
 *
 *
 * Initialized to 0 in audioEngine_init().
 */
int				m_nSelectedInstrumentNumber;
/**
 * Pointer to the metronome.
 *
 * Initialized in audioEngine_init().
 */
Instrument *			m_pMetronomeInstrument = nullptr;

// Buffers used in the process function
unsigned			m_nBufferSize = 0;
/**
 * Pointer to the audio buffer of the left stereo output returned by
 * AudioOutput::getOut_L().
 *
 * Initialized to NULL in audioEngine_init(), assigned in
 * audioEngine_startAudioDrivers(), reset in
 * audioEngine_process_clearAudioBuffers(), and populated with the
 * actual audio in audioEngine_process().
 */
float *				m_pMainBuffer_L = nullptr;
/**
 * Pointer to the audio buffer of the right stereo output returned by
 * AudioOutput::getOut_R().
 *
 * Initialized to NULL in audioEngine_init(), assigned in
 * audioEngine_startAudioDrivers(), reset in
 * audioEngine_process_clearAudioBuffers(), and populated with the
 * actual audio in audioEngine_process().
 */
float *				m_pMainBuffer_R = nullptr;
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
int				m_audioEngineState = STATE_UNINITIALIZED;	

#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_
float				m_fFXPeak_L[MAX_FX];
float				m_fFXPeak_R[MAX_FX];
#endif

/**
 * Beginning of the current pattern in ticks.
 *
 * It is set using finPatternInTick() and reset to -1 in
 * audioEngine_start(), audioEngine_stop(),
 * Hydrogen::startExportSong(), and
 * Hydrogen::triggerRelocateDuringPlay() (if the playback it in
 * Song::PATTERN_MODE).
 */
int				m_nPatternStartTick = -1;
/**
 * Ticks passed since the beginning of the current pattern.
 *
 * Queried using Hydrogen::getTickPosition().
 *
 * Initialized to 0 in audioEngine_init() and reset to 0 in
 * Hydrogen::setPatternPos(), if the AudioEngine is not playing, in
 * audioEngine_start(), Hydrogen::startExportSong() and
 * Hydrogen::stopExportSong(), which marks the beginning of a Song.
 */
unsigned int	m_nPatternTickPosition = 0;

/** Set to the total number of ticks in a Song in findPatternInTick()
    if Song::SONG_MODE is chosen and playback is at least in the
    second loop.*/
int				m_nSongSizeInTicks = 0;

/** Updated in audioEngine_updateNoteQueue().*/
struct timeval			m_currentTickTime;

/**
 * Variable keeping track of the transport position in realtime.
 *
 * Even if the audio engine is stopped, the variable will be
 * incremented by #m_nBufferSize (as audioEngine_process() would do at
 * the end of each cycle) to support realtime keyboard and MIDI event
 * timing. It is set using Hydrogen::setRealtimeFrames(), accessed via
 * Hydrogen::getRealtimeFrames(), and updated in
 * audioEngine_process_transport() using the current transport
 * position TransportInfo::m_nFrames.
 */
unsigned long			m_nRealtimeFrames = 0;
unsigned int			m_naddrealtimenotetickposition = 0;

// PROTOTYPES
/**
 * Initialization of the H2Core::AudioEngine called in Hydrogen::Hydrogen().
 *
 * -# It creates two new instances of the H2Core::PatternList and stores them
      in #m_pPlayingPatterns and #m_pNextPatterns.
 * -# It sets #m_nSongPos = -1.
 * -# It sets #m_nSelectedPatternNumber, #m_nSelectedInstrumentNumber,
      and #m_nPatternTickPosition to 0.
 * -# It sets #m_pMetronomeInstrument, #m_pAudioDriver,
      #m_pMainBuffer_L, #m_pMainBuffer_R to NULL.
 * -# It uses the current time to a random seed via std::srand(). This
      way the states of the pseudo-random number generator are not
      cross-correlated between different runs of Hydrogen.
 * -# It initializes the metronome with the sound stored in
      H2Core::Filesystem::click_file_path() by creating a new
      Instrument with #METRONOME_INSTR_ID as first argument.
 * -# It sets the H2Core::AudioEngine state #m_audioEngineState to
      #STATE_INITIALIZED.
 * -# It calls H2Core::Effects::create_instance() (if the
      #H2CORE_HAVE_LADSPA is set),
      H2Core::AudioEngine::create_instance(), and
      H2Core::Playlist::create_instance().
 * -# Finally, it pushes the H2Core::EVENT_STATE, #STATE_INITIALIZED
      on the H2Core::EventQueue using
      H2Core::EventQueue::push_event().
 *
 * If the current state of the H2Core::AudioEngine #m_audioEngineState is not
 * ::STATE_UNINITIALIZED, it will thrown an error and
 * H2Core::AudioEngine::unlock() it.
 */
void				audioEngine_init();
void				audioEngine_destroy();
/**
 * If the audio engine is in state #m_audioEngineState #STATE_READY,
 * this function will
 * - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f
 * - sets TransportInfo::m_nFrames to @a nTotalFrames
 * - sets m_nSongPos and m_nPatternStartTick to -1
 * - m_nPatternTickPosition to 0
 * - calls updateTickSize()
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
int				audioEngine_start( bool bLockEngine = false, unsigned nTotalFrames = 0 );
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
void				audioEngine_stop( bool bLockEngine = false );
/**
 * Updates the global objects of the audioEngine according to new #Song.
 *
 * Calls audioEngine_setupLadspaFX() on
 * m_pAudioDriver->getBufferSize(),
 * audioEngine_process_checkBPMChanged(),
 * audioEngine_renameJackPorts(), adds its first pattern to
 * #m_pPlayingPatterns, relocates the audio driver to the beginning of
 * the #Song, and updates the BPM.
 *
 * \param pNewSong #Song to load.
 */
void				audioEngine_setSong(Song *pNewSong );
/**
 * Does the necessary cleanup of the global objects in the audioEngine.
 *
 * Class the clear() member of #m_pPlayingPatterns and
 * #m_pNextPatterns as well as audioEngine_clearNoteQueue();
 */
void				audioEngine_removeSong();
static void			audioEngine_noteOn( Note *note );

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
 * (if #H2CORE_HAVE_LADSPA is defined) to #m_pMainBuffer_L and
 * #m_pMainBuffer_R and sets we peak values for #m_fFXPeak_L,
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
 * \param nframes Buffersize. If it doesn't match #m_nBufferSize, the
   latter will be set to @a nframes.
 * \param arg Unused.
 * \return
 * - __1__ : kill the audio driver thread. This will be used if either
 * the DiskWriterDriver or FakeDriver are used and the end of the Song
 * is reached (audioEngine_updateNoteQueue() returned -1 ). 
 * - __0__ : else
 */
int				audioEngine_process( uint32_t nframes, void *arg );
inline void			audioEngine_clearNoteQueue();
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
inline void			audioEngine_process_checkBPMChanged(Song *pSong);
inline void			audioEngine_process_playNotes( unsigned long nframes );
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
 * #m_nRealtimeFrames will be incremented by #m_nBufferSize to support
 * realtime keyboard and MIDI event timing.
 *
 * If the H2Core::m_audioEngineState is neither in #STATE_READY nor
 * #STATE_PLAYING the function will immediately return.
 */
inline void			audioEngine_process_transport();

inline unsigned		audioEngine_renderNote( Note* pNote, const unsigned& nBufferSize );
// TODO: Add documentation of doErase, inPunchArea, and
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
inline int			audioEngine_updateNoteQueue( unsigned nFrames );
inline void			audioEngine_prepNoteQueue();

/**
 * Find a PatternList corresponding to the supplied tick position @a
 * nTick.
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
inline int			findPatternInTick( int nTick, bool bLoopMode, int* pPatternStartTick );

void				audioEngine_seek( long long nFrames, bool bLoopMode = false );

void				audioEngine_restartAudioDrivers();
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
void				audioEngine_startAudioDrivers();
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
void				audioEngine_stopAudioDrivers();

/** Gets the current time.
 * \return Current time obtained by gettimeofday()*/
inline timeval currentTime2()
{
	struct timeval now;
	gettimeofday( &now, nullptr );
	return now;
}

inline int randomValue( int max )
{
	return rand() % max;
}

inline float getGaussian( float z )
{
	// gaussian distribution -- dimss
	float x1, x2, w;
	do {
		x1 = 2.0 * ( ( ( float ) rand() ) / RAND_MAX ) - 1.0;
		x2 = 2.0 * ( ( ( float ) rand() ) / RAND_MAX ) - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );

	w = sqrtf( ( -2.0 * logf( w ) ) / w );
	return x1 * w * z + 0.0; // tunable
}

void audioEngine_raiseError( unsigned nErrorCode )
{
	EventQueue::get_instance()->push_event( EVENT_ERROR, nErrorCode );
}
/** Function calculating the tick size by
 * \code{.cpp}
 * m_pAudioDriver->getSampleRate() * 60.0 / Song::__bpm / Song::__resolution 
 * \endcode
 * and storing it in TransportInfo::m_fTickSize
 */
void updateTickSize()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	float sampleRate = ( float ) m_pAudioDriver->getSampleRate();
	m_pAudioDriver->m_transport.m_fTickSize =
		( sampleRate * 60.0 /  pSong->__bpm / pSong->__resolution );		
}

void audioEngine_init()
{
	___INFOLOG( "*** Hydrogen audio engine init ***" );

	// check current state
	if ( m_audioEngineState != STATE_UNINITIALIZED ) {
		___ERRORLOG( "Error the audio engine is not in UNINITIALIZED state" );
		AudioEngine::get_instance()->unlock();
		return;
	}

	m_pPlayingPatterns = new PatternList();
	m_pNextPatterns = new PatternList();
	m_nSongPos = -1;
	m_nSelectedPatternNumber = 0;
	m_nSelectedInstrumentNumber = 0;
	m_nPatternTickPosition = 0;
	m_pMetronomeInstrument = nullptr;
	m_pAudioDriver = nullptr;

	m_pMainBuffer_L = nullptr;
	m_pMainBuffer_R = nullptr;

	srand( time( nullptr ) );

	// Create metronome instrument
	// Get the path to the file of the metronome sound.
	QString sMetronomeFilename = Filesystem::click_file_path();
	m_pMetronomeInstrument =
			new Instrument( METRONOME_INSTR_ID, "metronome" );
	
	InstrumentLayer* pLayer = 
		new InstrumentLayer( Sample::load( sMetronomeFilename ) );
	InstrumentComponent* pCompo = new InstrumentComponent( 0 );
	pCompo->set_layer(pLayer, 0);
	m_pMetronomeInstrument->get_components()->push_back( pCompo );
	m_pMetronomeInstrument->set_is_metronome_instrument(true);

	// Change the current audio engine state
	m_audioEngineState = STATE_INITIALIZED;

#ifdef H2CORE_HAVE_LADSPA
	Effects::create_instance();
#endif
	AudioEngine::create_instance();
	Playlist::create_instance();

	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_INITIALIZED );

}

void audioEngine_destroy()
{
	// check current state
	if ( m_audioEngineState != STATE_INITIALIZED ) {
		___ERRORLOG( "Error the audio engine is not in INITIALIZED state" );
		return;
	}
	AudioEngine::get_instance()->get_sampler()->stop_playing_notes();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	___INFOLOG( "*** Hydrogen audio engine shutdown ***" );

	// delete all copied notes in the song notes queue
	while ( !m_songNoteQueue.empty() ) {
		m_songNoteQueue.top()->get_instrument()->dequeue();
		delete m_songNoteQueue.top();
		m_songNoteQueue.pop();
	}
	// delete all copied notes in the midi notes queue
	for ( unsigned i = 0; i < m_midiNoteQueue.size(); ++i ) {
		delete m_midiNoteQueue[i];
	}
	m_midiNoteQueue.clear();

	// change the current audio engine state
	m_audioEngineState = STATE_UNINITIALIZED;

	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_UNINITIALIZED );

	delete m_pPlayingPatterns;
	m_pPlayingPatterns = nullptr;

	delete m_pNextPatterns;
	m_pNextPatterns = nullptr;

	delete m_pMetronomeInstrument;
	m_pMetronomeInstrument = nullptr;

	AudioEngine::get_instance()->unlock();
}

int audioEngine_start( bool bLockEngine, unsigned nTotalFrames )
{
	if ( bLockEngine ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
	}

	___INFOLOG( "[audioEngine_start]" );

	// check current state
	if ( m_audioEngineState != STATE_READY ) {
		___ERRORLOG( "Error the audio engine is not in READY state" );
		if ( bLockEngine ) {
			AudioEngine::get_instance()->unlock();
		}
		return 0;	// FIXME!!
	}

	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;
	// Reset the current transport position.
	m_pAudioDriver->m_transport.m_nFrames = nTotalFrames;
	m_nSongPos = -1;
	m_nPatternStartTick = -1;
	m_nPatternTickPosition = 0;

	// prepare the tick size for this song
	updateTickSize();

	// change the current audio engine state
	m_audioEngineState = STATE_PLAYING;
	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_PLAYING );

	if ( bLockEngine ) {
		AudioEngine::get_instance()->unlock();
	}
	return 0;
}

void audioEngine_stop( bool bLockEngine )
{
	if ( bLockEngine ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
	}
	___INFOLOG( "[audioEngine_stop]" );

	// check current state
	if ( m_audioEngineState != STATE_PLAYING ) {
		___ERRORLOG( "Error the audio engine is not in PLAYING state" );
		if ( bLockEngine ) {
			AudioEngine::get_instance()->unlock();
		}
		return;
	}

	// change the current audio engine state
	m_audioEngineState = STATE_READY;
	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_READY );

	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;
	//	m_nPatternTickPosition = 0;
	m_nPatternStartTick = -1;

	// delete all copied notes in the song notes queue
	while(!m_songNoteQueue.empty()){
		m_songNoteQueue.top()->get_instrument()->dequeue();
		delete m_songNoteQueue.top();
		m_songNoteQueue.pop();
	}

	// delete all copied notes in the midi notes queue
	for ( unsigned i = 0; i < m_midiNoteQueue.size(); ++i ) {
		delete m_midiNoteQueue[i];
	}
	m_midiNoteQueue.clear();

	if ( bLockEngine ) {
		AudioEngine::get_instance()->unlock();
	}
}

inline void audioEngine_process_checkBPMChanged(Song* pSong)
{
	if ( m_audioEngineState != STATE_READY
		 && m_audioEngineState != STATE_PLAYING ) {
		return;
	}

	long long oldFrame;
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() && 
		 m_audioEngineState != STATE_PLAYING ) {
		oldFrame = static_cast< JackAudioDriver* >( m_pAudioDriver )->m_currentPos;
			
	} else {
		oldFrame = m_pAudioDriver->m_transport.m_nFrames;
	}
#else
	oldFrame = m_pAudioDriver->m_transport.m_nFrames;
#endif
	float fOldTickSize = m_pAudioDriver->m_transport.m_fTickSize;
	float fNewTickSize = AudioEngine::compute_tick_size( m_pAudioDriver->getSampleRate(), pSong->__bpm, pSong->__resolution );

	// Nothing changed - avoid recomputing
	if ( fNewTickSize == fOldTickSize ) {
		return;
	}
	m_pAudioDriver->m_transport.m_fTickSize = fNewTickSize;

	if ( fNewTickSize == 0 || fOldTickSize == 0 ) {
		return;
	}

	float fTickNumber = (float)oldFrame / fOldTickSize;

	// update frame position in transport class
	m_pAudioDriver->m_transport.m_nFrames = ceil(fTickNumber) * fNewTickSize;
	
	___WARNINGLOG( QString( "Tempo change: Recomputing ticksize and frame position. Old TS: %1, new TS: %2, new pos: %3" )
		.arg( fOldTickSize ).arg( fNewTickSize )
		.arg( m_pAudioDriver->m_transport.m_nFrames ) );
	
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->calculateFrameOffset(oldFrame);
	}
#endif
	EventQueue::get_instance()->push_event( EVENT_RECALCULATERUBBERBAND, -1);
}

inline void audioEngine_process_playNotes( unsigned long nframes )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	unsigned int framepos;

	if (  m_audioEngineState == STATE_PLAYING ) {
		framepos = m_pAudioDriver->m_transport.m_nFrames;
	} else {
		// use this to support realtime events when not playing
		framepos = pHydrogen->getRealtimeFrames();
	}

	AutomationPath *vp = pSong->get_velocity_automation_path();
	

	// reading from m_songNoteQueue
	while ( !m_songNoteQueue.empty() ) {
		Note *pNote = m_songNoteQueue.top();

		float velocity_adjustment = 1.0f;
		if ( pSong->get_mode() == Song::SONG_MODE ) {
			float fPos = m_nSongPos + (pNote->get_position()%192) / 192.f;
			velocity_adjustment = vp->get_value(fPos);
		}

		// verifico se la nota rientra in questo ciclo
		unsigned int noteStartInFrames =
				(int)( pNote->get_position() * m_pAudioDriver->m_transport.m_fTickSize );

		// if there is a negative Humanize delay, take into account so
		// we don't miss the time slice.  ignore positive delay, or we
		// might end the queue processing prematurely based on NoteQueue
		// placement.  the sampler handles positive delay.
		if (pNote->get_humanize_delay() < 0) {
			noteStartInFrames += pNote->get_humanize_delay();
		}

		// m_nTotalFrames <= NotePos < m_nTotalFrames + bufferSize
		bool isNoteStart = ( ( noteStartInFrames >= framepos )
							 && ( noteStartInFrames < ( framepos + nframes ) ) );
		bool isOldNote = noteStartInFrames < framepos;

		if ( isNoteStart || isOldNote ) {
			// Humanize - Velocity parameter
			pNote->set_velocity( pNote->get_velocity() * velocity_adjustment );

			float rnd = (float)rand()/(float)RAND_MAX;
			if (pNote->get_probability() < rnd) {
				m_songNoteQueue.pop();
				pNote->get_instrument()->dequeue();
				continue;
			}

			if ( pSong->get_humanize_velocity_value() != 0 ) {
				float random = pSong->get_humanize_velocity_value() * getGaussian( 0.2 );
				pNote->set_velocity(
							pNote->get_velocity()
							+ ( random
								- ( pSong->get_humanize_velocity_value() / 2.0 ) )
							);
				if ( pNote->get_velocity() > 1.0 ) {
					pNote->set_velocity( 1.0 );
				} else if ( pNote->get_velocity() < 0.0 ) {
					pNote->set_velocity( 0.0 );
				}
			}

			// Random Pitch ;)
			const float fMaxPitchDeviation = 2.0;
			pNote->set_pitch( pNote->get_pitch()
							  + ( fMaxPitchDeviation * getGaussian( 0.2 )
								  - fMaxPitchDeviation / 2.0 )
							  * pNote->get_instrument()->get_random_pitch_factor() );


			/*
					  * Check if the current instrument has the property "Stop-Note" set.
					  * If yes, a NoteOff note is generated automatically after each note.
					  */
			Instrument * noteInstrument = pNote->get_instrument();
			if ( noteInstrument->is_stop_notes() ){
				Note *pOffNote = new Note( noteInstrument,
										   0.0,
										   0.0,
										   0.0,
										   0.0,
										   -1,
										   0 );
				pOffNote->set_note_off( true );
				AudioEngine::get_instance()->get_sampler()->note_on( pOffNote );
				delete pOffNote;
			}

			AudioEngine::get_instance()->get_sampler()->note_on( pNote );
			m_songNoteQueue.pop(); // rimuovo la nota dalla lista di note
			pNote->get_instrument()->dequeue();
			// raise noteOn event
			int nInstrument = pSong->get_instrument_list()->index( pNote->get_instrument() );
			if( pNote->get_note_off() ){
				delete pNote;
			}

			EventQueue::get_instance()->push_event( EVENT_NOTEON, nInstrument );
			continue;
		} else {
			// this note will not be played
			break;
		}
	}
}


void audioEngine_seek( long long nFrames, bool bLoopMode )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	if ( m_pAudioDriver->m_transport.m_nFrames == nFrames ) {
		return;
	}

	if ( nFrames < 0 ) {
		___ERRORLOG( "nFrames < 0" );
	}

	char tmp[200];
	sprintf( tmp, "seek in %lld (old pos = %d)",
			 nFrames,
			 ( int )m_pAudioDriver->m_transport.m_nFrames );
	___INFOLOG( tmp );

	m_pAudioDriver->m_transport.m_nFrames = nFrames;

	int tickNumber_start = ( unsigned )(
				m_pAudioDriver->m_transport.m_nFrames
				/ m_pAudioDriver->m_transport.m_fTickSize );
	//	sprintf(tmp, "[audioEngine_seek()] tickNumber_start = %d", tickNumber_start);
	//	__instance->infoLog(tmp);

	bool loop = pSong->is_loop_enabled();

	if ( bLoopMode ) {
		loop = true;
	}

	m_nSongPos = findPatternInTick( tickNumber_start, loop, &m_nPatternStartTick );
	//	sprintf(tmp, "[audioEngine_seek()] m_nSongPos = %d", m_nSongPos);
	//	__instance->infoLog(tmp);
	
	audioEngine_clearNoteQueue();
}

inline void audioEngine_process_transport()
{
	if ( m_audioEngineState != STATE_READY
	  && m_audioEngineState != STATE_PLAYING
	) return;

	// Considering JackAudioDriver:
	// Compares the current transport state, speed in bpm, and
	// transport position with a query request to the JACK
	// server. It will only overwrite m_transport.m_nFrames, if
	// the transport position was changed by the user by
	// e.g. clicking on the timeline.
	m_pAudioDriver->updateTransportInfo();

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	// Update the state of the audio engine depending on the
	// status of the audio driver. E.g. if the JACK transport was
	// started by another client, the audio engine has to be
	// started as well.
	switch ( m_pAudioDriver->m_transport.m_status ) {
	case TransportInfo::ROLLING:
		if ( m_audioEngineState == STATE_READY ) {
			// false == no engine lock. Already locked
			// this should set STATE_PLAYING
			audioEngine_start( false, m_pAudioDriver->m_transport.m_nFrames );
		}

		// So, we are not playing even after attempt to start engine
		if ( m_audioEngineState != STATE_PLAYING ) {
			return;
		}

		/* Now we're playing | Update BPM */
		if ( pSong->__bpm != m_pAudioDriver->m_transport.m_fBPM ) {
			___INFOLOG( QString( "song bpm: (%1) gets transport bpm: (%2)" )
				.arg( pSong->__bpm )
				.arg( m_pAudioDriver->m_transport.m_fBPM )
			);

			pHydrogen->setBPM( m_pAudioDriver->m_transport.m_fBPM );
		}

		// Update the variable m_nRealtimeFrames keeping track
		// of the current transport position.
		pHydrogen->setRealtimeFrames( m_pAudioDriver->m_transport.m_nFrames );
		break;
	case TransportInfo::STOPPED:
		// So, we are not playing even after attempt to start engine
		if ( m_audioEngineState == STATE_PLAYING ) {
			// false == no engine lock. Already locked
			audioEngine_stop( false );
		}

		// go ahead and increment the realtimeframes by buffersize
		// to support our realtime keyboard and midi event timing
		// TODO: use method like setRealtimeFrames
		m_nRealtimeFrames += m_nBufferSize;
		break;
	}
}

void audioEngine_clearNoteQueue()
{
	//___INFOLOG( "clear notes...");

	// delete all copied notes in the song notes queue
	while (!m_songNoteQueue.empty()) {
		m_songNoteQueue.top()->get_instrument()->dequeue();
		delete m_songNoteQueue.top();
		m_songNoteQueue.pop();
	}

	AudioEngine::get_instance()->get_sampler()->stop_playing_notes();

	// delete all copied notes in the midi notes queue
	for ( unsigned i = 0; i < m_midiNoteQueue.size(); ++i ) {
		delete m_midiNoteQueue[i];
	}
	m_midiNoteQueue.clear();

}

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
inline void audioEngine_process_clearAudioBuffers( uint32_t nFrames )
{
	QMutexLocker mx( &mutex_OutputPointer );

	// clear main out Left and Right
	if ( m_pAudioDriver ) {
		m_pMainBuffer_L = m_pAudioDriver->getOut_L();
		m_pMainBuffer_R = m_pAudioDriver->getOut_R();
	} else {
		m_pMainBuffer_L = m_pMainBuffer_R = nullptr;
	}
	if ( m_pMainBuffer_L ) {
		memset( m_pMainBuffer_L, 0, nFrames * sizeof( float ) );
	}
	if ( m_pMainBuffer_R ) {
		memset( m_pMainBuffer_R, 0, nFrames * sizeof( float ) );
	}

#ifdef H2CORE_HAVE_JACK
	JackAudioDriver * jo = dynamic_cast<JackAudioDriver*>(m_pAudioDriver);
	// Check whether the Preferences::m_bJackTrackOuts option was
	// set. It enables a per-track creation of the output
	// ports. All of them have to be reset as well.
	if( jo && jo->has_track_outs() ) {
		float* buf;
		int k;
		for( k=0 ; k<jo->getNumTracks() ; ++k ) {
			buf = jo->getTrackOut_L(k);
			if( buf ) {
				memset( buf, 0, nFrames * sizeof( float ) );
			}
			buf = jo->getTrackOut_R(k);
			if( buf ) {
				memset( buf, 0, nFrames * sizeof( float ) );
			}
		}
	}
#endif

	mx.unlock();

#ifdef H2CORE_HAVE_LADSPA
	if ( m_audioEngineState >= STATE_READY ) {
		Effects* pEffects = Effects::get_instance();
		for ( unsigned i = 0; i < MAX_FX; ++i ) {	// clear FX buffers
			LadspaFX* pFX = pEffects->getLadspaFX( i );
			if ( pFX ) {
				assert( pFX->m_pBuffer_L );
				assert( pFX->m_pBuffer_R );
				memset( pFX->m_pBuffer_L, 0, nFrames * sizeof( float ) );
				memset( pFX->m_pBuffer_R, 0, nFrames * sizeof( float ) );
			}
		}
	}
#endif
}

int audioEngine_process( uint32_t nframes, void* /*arg*/ )
{
	// ___INFOLOG( QString( "[begin] status: %1, frame: %2, ticksize: %3, bpm: %4" )
	// 	    .arg( m_pAudioDriver->m_transport.m_status )
	// 	    .arg( m_pAudioDriver->m_transport.m_nFrames )
	// 	    .arg( m_pAudioDriver->m_transport.m_fTickSize )
	// 	    .arg( m_pAudioDriver->m_transport.m_fBPM ) );
	timeval startTimeval = currentTime2();

	// Resetting all audio output buffers with zeros.
	audioEngine_process_clearAudioBuffers( nframes );

	/*
	 * The "try_lock" was introduced for Bug #164 (Deadlock after during
	 * alsa driver shutdown). The try_lock *should* only fail in rare circumstances
	 * (like shutting down drivers). In such cases, it seems to be ok to interrupt
	 * audio processing.
	 */
	if(!AudioEngine::get_instance()->try_lock( RIGHT_HERE )){
		return 0;
	}

	if ( m_audioEngineState < STATE_READY) {
		AudioEngine::get_instance()->unlock();
		return 0;
	}

	if ( m_nBufferSize != nframes ) {
		___INFOLOG(
			   QString( "Buffer size changed. Old size = %1, new size = %2" )
			   .arg( m_nBufferSize )
			   .arg( nframes ) );
		m_nBufferSize = nframes;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	// In case of the JackAudioDriver:
	// Query the JACK server for the current status of the
	// transport, start or stop the audio engine depending the
	// results, update the speed of the current song according to
	// the one used by the JACK server, and adjust the current
	// transport position if it was changed by an user interaction
	// (e.g. clicking on the timeline).
	audioEngine_process_transport();
	

	// ___INFOLOG( QString( "[after process] status: %1, frame: %2, ticksize: %3, bpm: %4" )
	// 	    .arg( m_pAudioDriver->m_transport.m_status )
	// 	    .arg( m_pAudioDriver->m_transport.m_nFrames )
	// 	    .arg( m_pAudioDriver->m_transport.m_fTickSize )
	// 	    .arg( m_pAudioDriver->m_transport.m_fBPM ) );
	// Check whether the tick size has changed.
	audioEngine_process_checkBPMChanged(pSong);

	bool bSendPatternChange = false;
	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	int nResNoteQueue = audioEngine_updateNoteQueue( nframes );
	if ( nResNoteQueue == -1 ) {	// end of song
		___INFOLOG( "End of song received, calling engine_stop()" );
		AudioEngine::get_instance()->unlock();
		m_pAudioDriver->stop();
		m_pAudioDriver->locate( 0 ); // locate 0, reposition from start of the song

		if ( ( m_pAudioDriver->class_name() == DiskWriterDriver::class_name() )
			 || ( m_pAudioDriver->class_name() == FakeDriver::class_name() )
			 ) {
			___INFOLOG( "End of song." );
			
			return 1;	// kill the audio AudioDriver thread
		}

		return 0;
	} else if ( nResNoteQueue == 2 ) { // send pattern change
		bSendPatternChange = true;
	}

	// play all notes
	audioEngine_process_playNotes( nframes );

	// SAMPLER
	AudioEngine::get_instance()->get_sampler()->process( nframes, pSong );
	float* out_L = AudioEngine::get_instance()->get_sampler()->__main_out_L;
	float* out_R = AudioEngine::get_instance()->get_sampler()->__main_out_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		m_pMainBuffer_L[ i ] += out_L[ i ];
		m_pMainBuffer_R[ i ] += out_R[ i ];
	}

	// SYNTH
	AudioEngine::get_instance()->get_synth()->process( nframes );
	out_L = AudioEngine::get_instance()->get_synth()->m_pOut_L;
	out_R = AudioEngine::get_instance()->get_synth()->m_pOut_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		m_pMainBuffer_L[ i ] += out_L[ i ];
		m_pMainBuffer_R[ i ] += out_R[ i ];
	}

	timeval renderTime_end = currentTime2();
	timeval ladspaTime_start = renderTime_end;

#ifdef H2CORE_HAVE_LADSPA
	// Process LADSPA FX
	if ( m_audioEngineState >= STATE_READY ) {
		for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
			LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
			if ( ( pFX ) && ( pFX->isEnabled() ) ) {
				pFX->processFX( nframes );

				float *buf_L, *buf_R;
				if ( pFX->getPluginType() == LadspaFX::STEREO_FX ) {
					buf_L = pFX->m_pBuffer_L;
					buf_R = pFX->m_pBuffer_R;
				} else { // MONO FX
					buf_L = pFX->m_pBuffer_L;
					buf_R = buf_L;
				}

				for ( unsigned i = 0; i < nframes; ++i ) {
					m_pMainBuffer_L[ i ] += buf_L[ i ];
					m_pMainBuffer_R[ i ] += buf_R[ i ];
					if ( buf_L[ i ] > m_fFXPeak_L[nFX] )
						m_fFXPeak_L[nFX] = buf_L[ i ];

					if ( buf_R[ i ] > m_fFXPeak_R[nFX] )
						m_fFXPeak_R[nFX] = buf_R[ i ];
				}
			}
		}
	}
#endif
	timeval ladspaTime_end = currentTime2();

	// update master peaks
	float val_L, val_R;
	if ( m_audioEngineState >= STATE_READY ) {
		for ( unsigned i = 0; i < nframes; ++i ) {
			val_L = m_pMainBuffer_L[i];
			val_R = m_pMainBuffer_R[i];

			if ( val_L > m_fMasterPeak_L )
				m_fMasterPeak_L = val_L;

			if ( val_R > m_fMasterPeak_R )
				m_fMasterPeak_R = val_R;

			for (std::vector<DrumkitComponent*>::iterator it = pSong->get_components()->begin() ; it != pSong->get_components()->end(); ++it) {
				DrumkitComponent* drumkit_component = *it;

				float compo_val_L = drumkit_component->get_out_L(i);
				float compo_val_R = drumkit_component->get_out_R(i);

				if( compo_val_L > drumkit_component->get_peak_l() )
					drumkit_component->set_peak_l( compo_val_L );
				if( compo_val_R > drumkit_component->get_peak_r() )
					drumkit_component->set_peak_r( compo_val_R );
			}
		}
	}

	// update total frames number
	if ( m_audioEngineState == STATE_PLAYING ) {
		m_pAudioDriver->m_transport.m_nFrames += nframes;
	}

	timeval finishTimeval = currentTime2();
	m_fProcessTime =
			( finishTimeval.tv_sec - startTimeval.tv_sec ) * 1000.0
			+ ( finishTimeval.tv_usec - startTimeval.tv_usec ) / 1000.0;

	float sampleRate = ( float )m_pAudioDriver->getSampleRate();
	m_fMaxProcessTime = 1000.0 / ( sampleRate / nframes );

#ifdef CONFIG_DEBUG
	if ( m_fProcessTime > m_fMaxProcessTime ) {
		___WARNINGLOG( "" );
		___WARNINGLOG( "----XRUN----" );
		___WARNINGLOG( QString( "XRUN of %1 msec (%2 > %3)" )
					   .arg( ( m_fProcessTime - m_fMaxProcessTime ) )
					   .arg( m_fProcessTime ).arg( m_fMaxProcessTime ) );
		___WARNINGLOG( QString( "Ladspa process time = %1" ).arg( fLadspaTime ) );
		___WARNINGLOG( "------------" );
		___WARNINGLOG( "" );
		// raise xRun event
		EventQueue::get_instance()->push_event( EVENT_XRUN, -1 );
	}
#endif
	// ___INFOLOG( QString( "[end] status: %1, frame: %2, ticksize: %3, bpm: %4" )
	// 	    .arg( m_pAudioDriver->m_transport.m_status )
	// 	    .arg( m_pAudioDriver->m_transport.m_nFrames )
	// 	    .arg( m_pAudioDriver->m_transport.m_fTickSize )
	// 	    .arg( m_pAudioDriver->m_transport.m_fBPM ) );

	AudioEngine::get_instance()->unlock();

	if ( bSendPatternChange ) {
		EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	}
	return 0;
}

void audioEngine_setupLadspaFX( unsigned nBufferSize )
{
	//___INFOLOG( "buffersize=" + to_string(nBufferSize) );

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	if ( ! pSong ) return;

	if ( nBufferSize == 0 ) {
		___ERRORLOG( "nBufferSize=0" );
		return;
	}

#ifdef H2CORE_HAVE_LADSPA
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX == nullptr ) {
			return;
		}

		pFX->deactivate();

		Effects::get_instance()->getLadspaFX( nFX )->connectAudioPorts(
					pFX->m_pBuffer_L,
					pFX->m_pBuffer_R,
					pFX->m_pBuffer_L,
					pFX->m_pBuffer_R
					);
		pFX->activate();
	}
#endif
}

/**
 * Hands the provided Song to JackAudioDriver::makeTrackOutputs() if
 * @a pSong is not a null pointer and the audio driver #m_pAudioDriver
 * is an instance of the JackAudioDriver.
 * \param pSong Song for which per-track output ports should be generated.
 */
void audioEngine_renameJackPorts(Song * pSong)
{
#ifdef H2CORE_HAVE_JACK
	// renames jack ports
	if ( ! pSong ) return;

	if ( Hydrogen::get_instance()->haveJackAudioDriver() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->makeTrackOutputs( pSong );
	}
#endif
}

void audioEngine_setSong( Song* pNewSong )
{
	___WARNINGLOG( QString( "Set song: %1" ).arg( pNewSong->__name ) );

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	// check current state
	// should be set by removeSong called earlier
	if ( m_audioEngineState != STATE_PREPARED ) {
		___ERRORLOG( "Error the audio engine is not in PREPARED state" );
	}

	// setup LADSPA FX
	audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );

	// update tick size
	audioEngine_process_checkBPMChanged( pNewSong );

	// find the first pattern and set as current
	if ( pNewSong->get_pattern_list()->size() > 0 ) {
		m_pPlayingPatterns->add( pNewSong->get_pattern_list()->get( 0 ) );
	}

	audioEngine_renameJackPorts( pNewSong );

	m_pAudioDriver->setBpm( pNewSong->__bpm );

	// change the current audio engine state
	m_audioEngineState = STATE_READY;

	m_pAudioDriver->locate( 0 );

	AudioEngine::get_instance()->unlock();

	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_READY );
}

void audioEngine_removeSong()
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	if ( m_audioEngineState == STATE_PLAYING ) {
		m_pAudioDriver->stop();
		audioEngine_stop( false );
	}

	// check current state
	if ( m_audioEngineState != STATE_READY ) {
		___ERRORLOG( "Error the audio engine is not in READY state" );
		AudioEngine::get_instance()->unlock();
		return;
	}

	m_pPlayingPatterns->clear();
	m_pNextPatterns->clear();
	audioEngine_clearNoteQueue();

	// change the current audio engine state
	m_audioEngineState = STATE_PREPARED;
	AudioEngine::get_instance()->unlock();

	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_PREPARED );
}

inline int audioEngine_updateNoteQueue( unsigned nFrames )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	// Indicates whether the current pattern list changed with respect
	// to the last cycle.
	bool bSendPatternChange = false;
	float fTickSize = m_pAudioDriver->m_transport.m_fTickSize;
	int nLeadLagFactor = pHydrogen->calculateLeadLagFactor( fTickSize );

	unsigned int framepos;
	if (  m_audioEngineState == STATE_PLAYING ) {
		// Current transport position.
		framepos = m_pAudioDriver->m_transport.m_nFrames;
	} else {
		// Use this to support realtime events, like MIDI, when not
		// playing.
		framepos = pHydrogen->getRealtimeFrames();
	}

	int lookahead = pHydrogen->calculateLookahead( fTickSize );
	int tickNumber_start = 0;
	if ( framepos == 0
		 || ( m_audioEngineState == STATE_PLAYING
			  && pSong->get_mode() == Song::SONG_MODE
			  && m_nSongPos == -1 )
	) {
		tickNumber_start = framepos / fTickSize;
	} else {
		tickNumber_start = ( framepos + lookahead) / fTickSize;
	}
	int tickNumber_end = ( framepos + nFrames + lookahead ) /fTickSize;

	// Get initial timestamp for first tick
	gettimeofday( &m_currentTickTime, nullptr );

	// A tick is the most fine-grained time scale within Hydrogen.
	for ( int tick = tickNumber_start; tick < tickNumber_end; tick++ ) {
		
		// MIDI events now get put into the `m_songNoteQueue` as well,
		// based on their timestamp (which is given in terms of its
		// transport position and not in terms of the date-time as
		// above).
		while ( m_midiNoteQueue.size() > 0 ) {
			Note *pNote = m_midiNoteQueue[0];
			if ( pNote->get_position() > tick ) break;

			m_midiNoteQueue.pop_front();
			pNote->get_instrument()->enqueue();
			m_songNoteQueue.push( pNote );
		}

		if (  m_audioEngineState != STATE_PLAYING ) {
			// only keep going if we're playing
			continue;
		}

		bool doErase = m_audioEngineState == STATE_PLAYING
				&& Preferences::get_instance()->getRecordEvents()
				&& Preferences::get_instance()->getDestructiveRecord()
				&& Preferences::get_instance()->m_nRecPreDelete == 0;
		
		//////////////////////////////////////////////////////////////
		// SONG MODE
		if ( pSong->get_mode() == Song::SONG_MODE ) {
			if ( pSong->get_pattern_group_vector()->size() == 0 ) {
				// there's no song!!
				___ERRORLOG( "no patterns in song." );
				m_pAudioDriver->stop();
				return -1;
			}
	
			m_nSongPos = findPatternInTick( tick, pSong->is_loop_enabled(), &m_nPatternStartTick );

			// The `m_nSongSizeInTicks` variable is only set to some
			// value other than zero in `findPatternInTick()` if
			// either the pattern list was not found of loop mode was
			// enabled and will contain the total size of the song in
			// ticks.
			if ( m_nSongSizeInTicks != 0 ) {
				// When using the JACK audio driver the overall
				// transport position will be managed by an external
				// server. Since it is agnostic of all the looping in
				// its clients, it will only increment time and
				// Hydrogen has to take care of the looping itself. 
				m_nPatternTickPosition = ( tick - m_nPatternStartTick )
						% m_nSongSizeInTicks;
			} else {
				m_nPatternTickPosition = tick - m_nPatternStartTick;
			}

			// Since we are located at the very beginning of the
			// pattern list, it had to change with respect to the last
			// cycle.
			if ( m_nPatternTickPosition == 0 ) {
				bSendPatternChange = true;
			}

			// If no pattern list could not be found, either choose
			// the first one if loop mode is activate or the
			// function returns indicating that the end of the song is
			// reached.
			if ( m_nSongPos == -1 ) {
				___INFOLOG( "song pos = -1" );
				if ( pSong->is_loop_enabled() == true ) {
					// TODO: This function call should be redundant
					// since `findPatternInTick()` is deterministic
					// and was already invoked with
					// `pSong->is_loop_enabled()` as second argument.
					m_nSongPos = findPatternInTick( 0, true, &m_nPatternStartTick );
				} else {

					___INFOLOG( "End of Song" );

					if( Hydrogen::get_instance()->getMidiOutput() != nullptr ){
						Hydrogen::get_instance()->getMidiOutput()->handleQueueAllNoteOff();
					}

					return -1;
				}
			}
			
			// Obtain the current PatternList and use it to overwrite
			// the on in `m_pPlayingPatterns.
			// TODO: Why overwriting it for each and every tick
			//       without check if it did changed? This is highly
			//       inefficient.
			PatternList *pPatternList = ( *( pSong->get_pattern_group_vector() ) )[m_nSongPos];
			m_pPlayingPatterns->clear();
			for ( int i=0; i< pPatternList->size(); ++i ) {
				Pattern* pPattern = pPatternList->get(i);
				m_pPlayingPatterns->add( pPattern );
				pPattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}
			// Set destructive record depending on punch area
			doErase = doErase && Preferences::get_instance()->inPunchArea(m_nSongPos);
		}
		
		//////////////////////////////////////////////////////////////
		// PATTERN MODE
		else if ( pSong->get_mode() == Song::PATTERN_MODE )	{

			int nPatternSize = MAX_NOTES;

			// If the user chose to playback the pattern she focuses,
			// use it to overwrite `m_pPlayingPatterns`.
			if ( Preferences::get_instance()->patternModePlaysSelected() )
			{
				// TODO: Again, a check whether the pattern did change
				// would be more efficient.
				m_pPlayingPatterns->clear();
				Pattern * pattern = pSong->get_pattern_list()->get(m_nSelectedPatternNumber);
				m_pPlayingPatterns->add( pattern );
				pattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}

			if ( m_pPlayingPatterns->size() != 0 ) {
				const Pattern *pFirstPattern = m_pPlayingPatterns->get( 0 );
				nPatternSize = pFirstPattern->get_length();
			}

			if ( nPatternSize == 0 ) {
				___ERRORLOG( "nPatternSize == 0" );
			}

			// If either the beginning of the current pattern was not
			// specified yet or if its end is reached, write the
			// content of `m_pNextPatterns` to `m_pPlayingPatterns`
			// and clear the former one.
			if ( ( tick == m_nPatternStartTick + nPatternSize )
				 || ( m_nPatternStartTick == -1 ) ) {
				if ( m_pNextPatterns->size() > 0 ) {
					Pattern* pPattern;
					for ( uint i = 0; i < m_pNextPatterns->size(); i++ ) {
						pPattern = m_pNextPatterns->get( i );
						// If `pPattern is already present in
						// `m_pPlayingPatterns`, it will be removed
						// from the latter and its `del()` method will
						// return a pointer to the very pattern. The
						// if clause is therefore only entered if the
						// `pPattern` was not already present.
						if ( ( m_pPlayingPatterns->del( pPattern ) ) == nullptr ) {
							m_pPlayingPatterns->add( pPattern );
						}
					}
					m_pNextPatterns->clear();
					bSendPatternChange = true;
				}
				if ( m_nPatternStartTick == -1 && nPatternSize > 0 ) {
					m_nPatternStartTick = tick - (tick % nPatternSize);
				} else {
					m_nPatternStartTick = tick;
				}
			}

			// Since the starting position of the Pattern may have
			// been updated, update the number of ticks passed since
			// the beginning of the pattern too.
			m_nPatternTickPosition = tick - m_nPatternStartTick;
			if ( m_nPatternTickPosition > nPatternSize && nPatternSize > 0 ) {
				m_nPatternTickPosition = tick % nPatternSize;
			}
		}

		//////////////////////////////////////////////////////////////
		// Metronome
		// Only trigger the metronome at a predefined rate.
		if ( m_nPatternTickPosition % 48 == 0 ) {
			float fPitch;
			float fVelocity;
			
			// Depending on whether the metronome beat will be issued
			// at the beginning or in the remainder of the pattern,
			// two different sounds and events will be used.
			if ( m_nPatternTickPosition == 0 ) {
				fPitch = 3;
				fVelocity = 1.0;
				EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
			} else {
				fPitch = 0;
				fVelocity = 0.8;
				EventQueue::get_instance()->push_event( EVENT_METRONOME, 0 );
			}
			
			// Only trigger the sounds if the user enabled the
			// metronome. 
			if ( Preferences::get_instance()->m_bUseMetronome ) {
				m_pMetronomeInstrument->set_volume(
							Preferences::get_instance()->m_fMetronomeVolume
							);
				Note *pMetronomeNote = new Note( m_pMetronomeInstrument,
												 tick,
												 fVelocity,
												 0.5,
												 0.5,
												 -1,
												 fPitch
												 );
				m_pMetronomeInstrument->enqueue();
				m_songNoteQueue.push( pMetronomeNote );
			}
		}

		//////////////////////////////////////////////////////////////
		// Update the notes queue.
		// 
		if ( m_pPlayingPatterns->size() != 0 ) {
			for ( unsigned nPat = 0 ;
				  nPat < m_pPlayingPatterns->size() ;
				  ++nPat ) {
				Pattern *pPattern = m_pPlayingPatterns->get( nPat );
				assert( pPattern != nullptr );
				Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->get_notes();
				// Delete notes before attempting to play them
				if ( doErase ) {
					FOREACH_NOTE_IT_BOUND(notes,it,m_nPatternTickPosition) {
						Note* pNote = it->second;
						assert( pNote != nullptr );
						if ( pNote->get_just_recorded() == false ) {
							EventQueue::AddMidiNoteVector noteAction;
							noteAction.m_column = pNote->get_position();
							noteAction.m_row = pNote->get_instrument_id();
							noteAction.m_pattern = nPat;
							noteAction.f_velocity = pNote->get_velocity();
							noteAction.f_pan_L = pNote->get_pan_l();
							noteAction.f_pan_R = pNote->get_pan_r();
							noteAction.m_length = -1;
							noteAction.no_octaveKeyVal = pNote->get_octave();
							noteAction.nk_noteKeyVal = pNote->get_key();
							noteAction.b_isInstrumentMode = false;
							noteAction.b_isMidi = false;
							noteAction.b_noteExist = false;
							EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);
						}
					}
				}

				// Perform a loop over all notes, which are enclose
				// the position of the current tick, using a constant
				// iterator (notes won't be altered!). After some
				// humanization was applied to onset of each note, it
				// will be added to `m_songNoteQueue` for playback.
				FOREACH_NOTE_CST_IT_BOUND(notes,it,m_nPatternTickPosition) {
					Note *pNote = it->second;
					if ( pNote ) {
						pNote->set_just_recorded( false );
						int nOffset = 0;

						// Swing //
						// Add a constant and periodic offset at
						// predefined positions to the note position.
						// TODO: incorporate the factor of 6.0 either
						// in Song::__swing_factor or make it a member
						// variable.
						float fSwingFactor = pSong->get_swing_factor();
						if ( ( ( m_nPatternTickPosition % 12 ) == 0 )
							 && ( ( m_nPatternTickPosition % 24 ) != 0 ) ) {
							// da l'accento al tick 4, 12, 20, 36...
							nOffset += (int)( 6.0 * fTickSize * fSwingFactor );
						}

						// Humanize - Time parameter //
						// Add a random offset to each note. Due to
						// the nature of the Gaussian distribution,
						// the factor Song::__humanize_time_value will
						// also scale the variance of the generated
						// random variable.
						if ( pSong->get_humanize_time_value() != 0 ) {
							nOffset += ( int )(
										getGaussian( 0.3 )
										* pSong->get_humanize_time_value()
										* pHydrogen->m_nMaxTimeHumanize
										);
						}

						// Lead or Lag - timing parameter //
						// Add a constant offset to all notes.
						nOffset += (int) ( pNote->get_lead_lag()
										   * nLeadLagFactor );

						// No note is allowed to start prior to the
						// beginning of the song.
						if((tick == 0) && (nOffset < 0)) {
							nOffset = 0;
						}
						
						// Generate a copy of the current note, assign
						// it the new offset, and push it to the list
						// of all notes, which are about to be played
						// back.
						// TODO: Why a copy?
						Note *pCopiedNote = new Note( pNote );
						pCopiedNote->set_position( tick );
						pCopiedNote->set_humanize_delay( nOffset );
						pNote->get_instrument()->enqueue();
						m_songNoteQueue.push( pCopiedNote );
					}
				}
			}
		}
	}

	// audioEngine_process() must send the pattern change event after
	// mutex unlock
	if ( bSendPatternChange ) {
		return 2;
	}
	return 0;
}

inline int findPatternInTick( int nTick, bool bLoopMode, int* pPatternStartTick )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	assert( pSong );

	int nTotalTick = 0;
	m_nSongSizeInTicks = 0;

	std::vector<PatternList*> *pPatternColumns = pSong->get_pattern_group_vector();
	int nColumns = pPatternColumns->size();

	// Sum the lengths of all pattern columns and use the macro
	// MAX_NOTES in case some of them are of size zero. If the
	// supplied value nTick is bigger than this and doesn't belong to
	// the next pattern column, we just found the pattern list we were
	// searching for.
	int nPatternSize;
	for ( int i = 0; i < nColumns; ++i ) {
		PatternList *pColumn = ( *pPatternColumns )[ i ];
		if ( pColumn->size() != 0 ) {
			nPatternSize = pColumn->get( 0 )->get_length();
		} else {
			nPatternSize = MAX_NOTES;
		}

		if ( ( nTick >= nTotalTick ) && ( nTick < nTotalTick + nPatternSize ) ) {
			( *pPatternStartTick ) = nTotalTick;
			return i;
		}
		nTotalTick += nPatternSize;
	}

	// If the song is played in loop mode, the tick numbers of the
	// second turn are added on top of maximum tick number of the
	// song. Therefore, we will introduced periodic boundary
	// conditions and start the search again.
	if ( bLoopMode ) {
		m_nSongSizeInTicks = nTotalTick;
		int nLoopTick = 0;
		if ( m_nSongSizeInTicks != 0 ) {
			nLoopTick = nTick % m_nSongSizeInTicks;
		}
		nTotalTick = 0;
		for ( int i = 0; i < nColumns; ++i ) {
			PatternList *pColumn = ( *pPatternColumns )[ i ];
			if ( pColumn->size() != 0 ) {
				nPatternSize = pColumn->get( 0 )->get_length();
			} else {
				nPatternSize = MAX_NOTES;
			}

			if ( ( nLoopTick >= nTotalTick )
				 && ( nLoopTick < nTotalTick + nPatternSize ) ) {
				( *pPatternStartTick ) = nTotalTick;
				return i;
			}
			nTotalTick += nPatternSize;
		}
	}

	QString err = QString( "[findPatternInTick] tick = %1. No pattern list found" ).arg( QString::number(nTick) );
	___ERRORLOG( err );
	return -1;
}

void audioEngine_noteOn( Note *note )
{
	// check current state
	if ( ( m_audioEngineState != STATE_READY )
		 && ( m_audioEngineState != STATE_PLAYING ) ) {
		___ERRORLOG( "Error the audio engine is not in READY state" );
		delete note;
		return;
	}

	m_midiNoteQueue.push_back( note );
}

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
AudioOutput* createDriver( const QString& sDriver )
{
	___INFOLOG( QString( "Driver: '%1'" ).arg( sDriver ) );
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *pDriver = nullptr;

	if ( sDriver == "Oss" ) {
		pDriver = new OssDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "Jack" ) {
		pDriver = new JackAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		} else {
#ifdef H2CORE_HAVE_JACK
			static_cast<JackAudioDriver*>(pDriver)->setConnectDefaults(
						Preferences::get_instance()->m_bJackConnectDefaults
						);
#endif
		}
	} else if ( sDriver == "Alsa" ) {
		pDriver = new AlsaAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "PortAudio" ) {
		pDriver = new PortAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	//#ifdef Q_OS_MACX
	else if ( sDriver == "CoreAudio" ) {
		___INFOLOG( "Creating CoreAudioDriver" );
		pDriver = new CoreAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	//#endif
	else if ( sDriver == "PulseAudio" ) {
		pDriver = new PulseAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	else if ( sDriver == "Fake" ) {
		___WARNINGLOG( "*** Using FAKE audio driver ***" );
		pDriver = new FakeDriver( audioEngine_process );
	} else {
		___ERRORLOG( "Unknown driver " + sDriver );
		audioEngine_raiseError( Hydrogen::UNKNOWN_DRIVER );
	}

	if ( pDriver  ) {
		// initialize the audio driver
		int res = pDriver->init( pPref->m_nBufferSize );
		if ( res != 0 ) {
			___ERRORLOG( "Error starting audio driver [audioDriver::init()]" );
			delete pDriver;
			pDriver = nullptr;
		}
	}

	return pDriver;
}

void audioEngine_startAudioDrivers()
{
	Preferences *preferencesMng = Preferences::get_instance();

	// Lock both the AudioEngine and the audio output buffers.
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	QMutexLocker mx(&mutex_OutputPointer);

	___INFOLOG( "[audioEngine_startAudioDrivers]" );

	// check current state
	if ( m_audioEngineState != STATE_INITIALIZED ) {
		___ERRORLOG( QString( "Error the audio engine is not in INITIALIZED"
							  " state. state=%1" )
					 .arg( m_audioEngineState ) );
		AudioEngine::get_instance()->unlock();
		return;
	}

	if ( m_pAudioDriver ) {	// check if the audio m_pAudioDriver is still alive
		___ERRORLOG( "The audio driver is still alive" );
	}
	if ( m_pMidiDriver ) {	// check if midi driver is still alive
		___ERRORLOG( "The MIDI driver is still active" );
	}


	QString sAudioDriver = preferencesMng->m_sAudioDriver;
	if ( sAudioDriver == "Auto" ) {
	#ifndef WIN32
		if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == nullptr ) {
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == nullptr ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == nullptr ) {
					if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == nullptr ) {
						if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == nullptr ) {
							if ( ( m_pAudioDriver = createDriver( "PulseAudio" ) ) == nullptr ) {
								audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
								___ERRORLOG( "Error starting audio driver" );
								___ERRORLOG( "Using the NULL output audio driver" );

								// use the NULL output driver
								m_pAudioDriver = new NullDriver( audioEngine_process );
								m_pAudioDriver->init( 0 );
							}
						}
					}
				}
			}
		}
	#else
		//On Windows systems, use PortAudio is the prioritized backend
		if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == nullptr ) {
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == nullptr ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == nullptr ) {
					if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == nullptr ) {
						if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == nullptr ) {
							if ( ( m_pAudioDriver = createDriver( "PulseAudio" ) ) == nullptr ) {
								audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
								___ERRORLOG( "Error starting audio driver" );
								___ERRORLOG( "Using the NULL output audio driver" );

								// use the NULL output driver
								m_pAudioDriver = new NullDriver( audioEngine_process );
								m_pAudioDriver->init( 0 );
							}
						}
					}
				}
			}
		}
	#endif
	} else {
		m_pAudioDriver = createDriver( sAudioDriver );
		if ( m_pAudioDriver == nullptr ) {
			audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			___ERRORLOG( "Error starting audio driver" );
			___ERRORLOG( "Using the NULL output audio driver" );

			// use the NULL output driver
			m_pAudioDriver = new NullDriver( audioEngine_process );
			m_pAudioDriver->init( 0 );
		}
	}

	if ( preferencesMng->m_sMidiDriver == "ALSA" ) {
#ifdef H2CORE_HAVE_ALSA
		// Create MIDI driver
		AlsaMidiDriver *alsaMidiDriver = new AlsaMidiDriver();
		m_pMidiDriverOut = alsaMidiDriver;
		m_pMidiDriver = alsaMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "PortMidi" ) {
#ifdef H2CORE_HAVE_PORTMIDI
		PortMidiDriver* pPortMidiDriver = new PortMidiDriver();
		m_pMidiDriver = pPortMidiDriver;
		m_pMidiDriverOut = pPortMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "CoreMidi" ) {
#ifdef H2CORE_HAVE_COREMIDI
		CoreMidiDriver *coreMidiDriver = new CoreMidiDriver();
		m_pMidiDriver = coreMidiDriver;
		m_pMidiDriverOut = coreMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "JackMidi" ) {
#ifdef H2CORE_HAVE_JACK
		JackMidiDriver *jackMidiDriver = new JackMidiDriver();
		m_pMidiDriverOut = jackMidiDriver;
		m_pMidiDriver = jackMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	}

	// change the current audio engine state
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	if ( pSong ) {
		m_audioEngineState = STATE_READY;
		m_pAudioDriver->setBpm( pSong->__bpm );
	} else {
		m_audioEngineState = STATE_PREPARED;
	}

	if ( m_audioEngineState == STATE_PREPARED ) {
		EventQueue::get_instance()->push_event( EVENT_STATE, STATE_PREPARED );
	} else if ( m_audioEngineState == STATE_READY ) {
		EventQueue::get_instance()->push_event( EVENT_STATE, STATE_READY );
	}

	// Unlocking earlier might execute the jack process() callback before we
	// are fully initialized.
	mx.unlock();
	AudioEngine::get_instance()->unlock();

	if ( m_pAudioDriver ) {
		int res = m_pAudioDriver->connect();
		if ( res != 0 ) {
			audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			___ERRORLOG( "Error starting audio driver [audioDriver::connect()]" );
			___ERRORLOG( "Using the NULL output audio driver" );

			mx.relock();
			delete m_pAudioDriver;
			m_pAudioDriver = new NullDriver( audioEngine_process );
			mx.unlock();
			m_pAudioDriver->init( 0 );
			m_pAudioDriver->connect();
		}

		if ( ( m_pMainBuffer_L = m_pAudioDriver->getOut_L() ) == nullptr ) {
			___ERRORLOG( "m_pMainBuffer_L == NULL" );
		}
		if ( ( m_pMainBuffer_R = m_pAudioDriver->getOut_R() ) == nullptr ) {
			___ERRORLOG( "m_pMainBuffer_R == NULL" );
		}

#ifdef H2CORE_HAVE_JACK
		audioEngine_renameJackPorts( pSong );
#endif

		audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );
	}


}

void audioEngine_stopAudioDrivers()
{
	___INFOLOG( "[audioEngine_stopAudioDrivers]" );

	// check current state
	if ( m_audioEngineState == STATE_PLAYING ) {
		audioEngine_stop();
	}

	if ( ( m_audioEngineState != STATE_PREPARED )
		 && ( m_audioEngineState != STATE_READY ) ) {
		___ERRORLOG( QString( "Error: the audio engine is not in PREPARED"
							  " or READY state. state=%1" )
					 .arg( m_audioEngineState ) );
		return;
	}

	// change the current audio engine state
	m_audioEngineState = STATE_INITIALIZED;
	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_INITIALIZED );

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	// delete MIDI driver
	if ( m_pMidiDriver ) {
		m_pMidiDriver->close();
		delete m_pMidiDriver;
		m_pMidiDriver = nullptr;
		m_pMidiDriverOut = nullptr;
	}

	// delete audio driver
	if ( m_pAudioDriver ) {
		m_pAudioDriver->disconnect();
		QMutexLocker mx( &mutex_OutputPointer );
		delete m_pAudioDriver;
		m_pAudioDriver = nullptr;
		mx.unlock();
	}

	AudioEngine::get_instance()->unlock();
}



/// Restart all audio and midi drivers by calling first
/// audioEngine_stopAudioDrivers() and then
/// audioEngine_startAudioDrivers().
void audioEngine_restartAudioDrivers()
{
	audioEngine_stopAudioDrivers();
	audioEngine_startAudioDrivers();
}

//----------------------------------------------------------------------------
//
// Implementation of Hydrogen class
//
//----------------------------------------------------------------------------

Hydrogen* Hydrogen::__instance = nullptr;
const char* Hydrogen::__class_name = "Hydrogen";

Hydrogen::Hydrogen()
	: Object( __class_name )
{
	if ( __instance ) {
		ERRORLOG( "Hydrogen audio engine is already running" );
		throw H2Exception( "Hydrogen audio engine is already running" );
	}

	INFOLOG( "[Hydrogen]" );

	__song = nullptr;

	m_bExportSessionIsActive = false;
	m_pTimeline = new Timeline();
	m_pCoreActionController = new CoreActionController();
	m_bActiveGUI = false;
	m_nMaxTimeHumanize = 2000;

	initBeatcounter();
	InstrumentComponent::setMaxLayers( Preferences::get_instance()->getMaxLayers() );
	audioEngine_init();

	// Prevent double creation caused by calls from MIDI thread
	__instance = this;

	audioEngine_startAudioDrivers();
	for(int i = 0; i< MAX_INSTRUMENTS; i++){
		m_nInstrumentLookupTable[i] = i;
	}

#ifdef H2CORE_HAVE_OSC
	if( Preferences::get_instance()->getOscServerEnabled() )
	{
		OscServer* pOscServer = OscServer::get_instance();
		pOscServer->start();
	}
#endif
}

Hydrogen::~Hydrogen()
{
	INFOLOG( "[~Hydrogen]" );

#ifdef H2CORE_HAVE_OSC
	NsmClient* pNsmClient = NsmClient::get_instance();
	if( pNsmClient ) {
		pNsmClient->shutdown();
		delete pNsmClient;
	}
	OscServer* pOscServer = OscServer::get_instance();
	if( pOscServer ) {
		delete pOscServer;
	}
#endif

	if ( m_audioEngineState == STATE_PLAYING ) {
		audioEngine_stop();
	}
	removeSong();
	audioEngine_stopAudioDrivers();
	audioEngine_destroy();
	__kill_instruments();

	delete m_pCoreActionController;
	delete m_pTimeline;

	__instance = nullptr;
}

void Hydrogen::create_instance()
{
	// Create all the other instances that we need
	// ....and in the right order
	Logger::create_instance();
	MidiMap::create_instance();
	Preferences::create_instance();
	EventQueue::create_instance();
	MidiActionManager::create_instance();

#ifdef H2CORE_HAVE_OSC
	NsmClient::create_instance();
	OscServer::create_instance( Preferences::get_instance() );
#endif

	if ( __instance == nullptr ) {
		__instance = new Hydrogen;
	}

	// See audioEngine_init() for:
	// AudioEngine::create_instance();
	// Effects::create_instance();
	// Playlist::create_instance();
}

void Hydrogen::initBeatcounter()
{
	m_ntaktoMeterCompute = 1;
	m_nbeatsToCount = 4;
	m_nEventCount = 1;
	m_nTempoChangeCounter = 0;
	m_nBeatCount = 1;
	m_nCoutOffset = 0;
	m_nStartOffset = 0;
}

/// Start the internal sequencer
void Hydrogen::sequencer_play()
{
	Song* pSong = getSong();
	pSong->get_pattern_list()->set_to_old();
	m_pAudioDriver->play();
}

/// Stop the internal sequencer
void Hydrogen::sequencer_stop()
{
	if( Hydrogen::get_instance()->getMidiOutput() != nullptr ){
		Hydrogen::get_instance()->getMidiOutput()->handleQueueAllNoteOff();
	}

	m_pAudioDriver->stop();
	Preferences::get_instance()->setRecordEvents(false);
}

bool Hydrogen::setPlaybackTrackState( const bool state )
{
	Song* pSong = getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	return pSong->set_playback_track_enabled(state);
}

void Hydrogen::loadPlaybackTrack( const QString filename )
{
	Song* pSong = getSong();
	pSong->set_playback_track_filename(filename);

	AudioEngine::get_instance()->get_sampler()->reinitialize_playback_track();
}

void Hydrogen::setSong( Song *pSong )
{
	assert ( pSong );
	
	// Move to the beginning.
	setSelectedPatternNumber( 0 );

	Song* pCurrentSong = getSong();
	if ( pSong == pCurrentSong ) {
		DEBUGLOG( "pSong == pCurrentSong" );
		return;
	}

	if ( pCurrentSong ) {
		
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		delete pCurrentSong;
		pCurrentSong = nullptr;

		AudioEngine::get_instance()->unlock();

		/* NOTE: 
		 *       - this is actually some kind of cleanup 
		 *       - removeSong cares itself for acquiring a lock
		 */
		removeSong();
	}

	/* Reset GUI */
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
	EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	// In order to allow functions like audioEngine_setupLadspaFX() to
	// load the settings of the new song, like whether the LADSPA FX
	// are activated, __song has to be set prior to the call of
	// audioEngine_setSong().
	__song = pSong;

	
	// Update the audio engine to work with the new song.
	audioEngine_setSong( pSong );

	// load new playback track information
	AudioEngine::get_instance()->get_sampler()->reinitialize_playback_track();
	
	// Push current state of Hydrogen to attached control interfaces,
	// like OSC clients.
	m_pCoreActionController->initExternalControlInterfaces();
}

/* Mean: remove current song from memory */
void Hydrogen::removeSong()
{
	__song = nullptr;
	audioEngine_removeSong();
}

void Hydrogen::midi_noteOn( Note *note )
{
	audioEngine_noteOn( note );
}

void Hydrogen::addRealtimeNote(	int		instrument,
								float	velocity,
								float	pan_L,
								float	pan_R,
								float	pitch,
								bool	noteOff,
								bool	forcePlay,
								int		msg1 )
{
	UNUSED( pitch );

	Preferences *pPreferences = Preferences::get_instance();
	unsigned int nRealColumn = 0;
	unsigned res = pPreferences->getPatternEditorGridResolution();
	int nBase = pPreferences->isPatternEditorUsingTriplets() ? 3 : 4;
	int scalar = ( 4 * MAX_NOTES ) / ( res * nBase );
	bool hearnote = forcePlay;
	int currentPatternNumber;

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *pSong = getSong();
	if ( !pPreferences->__playselectedinstrument ) {
		if ( instrument >= ( int ) pSong->get_instrument_list()->size() ) {
			// unused instrument
			AudioEngine::get_instance()->unlock();
			return;
		}
	}

	// Get current partern and column, compensating for "lookahead" if required
	const Pattern* currentPattern = nullptr;
	unsigned int column = 0;
	float fTickSize = m_pAudioDriver->m_transport.m_fTickSize;
	unsigned int lookaheadTicks = calculateLookahead( fTickSize ) / fTickSize;
	bool doRecord = pref->getRecordEvents();
	if ( pSong->get_mode() == Song::SONG_MODE && doRecord &&
		 m_audioEngineState == STATE_PLAYING )
	{

		// Recording + song playback mode + actually playing
		PatternList *pPatternList = pSong->get_pattern_list();
		int ipattern = getPatternPos(); // playlist index
		if ( ipattern < 0 || ipattern >= (int) pPatternList->size() ) {
			AudioEngine::get_instance()->unlock(); // unlock the audio engine
			return;
		}
		// Locate column -- may need to jump back in the pattern list
		column = getTickPosition();
		while ( column < lookaheadTicks ) {
			ipattern -= 1;
			if ( ipattern < 0 || ipattern >= (int) pPatternList->size() ) {
				AudioEngine::get_instance()->unlock(); // unlock the audio engine
				return;
			}

			// Convert from playlist index to actual pattern index
			std::vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();
			for ( int i = 0; i <= ipattern; ++i ) {
				PatternList *pColumn = ( *pColumns )[i];
				currentPattern = pColumn->get( 0 );
				currentPatternNumber = i;
			}
			column = column + currentPattern->get_length();
			// WARNINGLOG( "Undoing lookahead: corrected (" + to_string( ipattern+1 ) +
			// "," + to_string( (int) ( column - currentPattern->get_length() ) -
			// (int) lookaheadTicks ) + ") -> (" + to_string(ipattern) +
			// "," + to_string( (int) column - (int) lookaheadTicks ) + ")." );
		}
		column -= lookaheadTicks;
		// Convert from playlist index to actual pattern index (if not already done above)
		if ( currentPattern == nullptr ) {
			std::vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();
			for ( int i = 0; i <= ipattern; ++i ) {
				PatternList *pColumn = ( *pColumns )[i];
				currentPattern = pColumn->get( 0 );
				currentPatternNumber = i;
			}
		}

		// Cancel recording if punch area disagrees
		doRecord = pPreferences->inPunchArea( ipattern );

	} else { // Not song-record mode
		PatternList *pPatternList = pSong->get_pattern_list();

		if ( ( m_nSelectedPatternNumber != -1 )
			 && ( m_nSelectedPatternNumber < ( int )pPatternList->size() ) )
		{
			currentPattern = pPatternList->get( m_nSelectedPatternNumber );
			currentPatternNumber = m_nSelectedPatternNumber;
		}

		if ( ! currentPattern ) {
			AudioEngine::get_instance()->unlock(); // unlock the audio engine
			return;
		}

		// Locate column -- may need to wrap around end of pattern
		column = getTickPosition();
		if ( column >= lookaheadTicks ) {
			column -= lookaheadTicks;
		} else {
			lookaheadTicks %= currentPattern->get_length();
			column = (column + currentPattern->get_length() - lookaheadTicks)
					% currentPattern->get_length();
		}
	}

	nRealColumn = getRealtimeTickPosition();

	if ( pPreferences->getQuantizeEvents() ) {
		// quantize it to scale
		unsigned qcolumn = ( unsigned )::round( column / ( double )scalar ) * scalar;

		//we have to make sure that no beat is added on the last displayed note in a bar
		//for example: if the pattern has 4 beats, the editor displays 5 beats, so we should avoid adding beats an note 5.
		if ( qcolumn == currentPattern->get_length() ) qcolumn = 0;
		column = qcolumn;
	}

	unsigned position = column;
	m_naddrealtimenotetickposition = column;

	Instrument *instrRef = nullptr;
	if ( pSong ) {
		//getlookuptable index = instrument+36, ziel wert = der entprechende wert -36
		instrRef = pSong->get_instrument_list()->get( m_nInstrumentLookupTable[ instrument ] );
	}

	if ( currentPattern && ( getState() == STATE_PLAYING ) ) {
		if ( doRecord && pPreferences->getDestructiveRecord() && pPreferences->m_nRecPreDelete>0 ) {
			// Delete notes around current note if option toggled
			int postdelete = 0;
			int predelete = 0;
			int prefpredelete = pPreferences->m_nRecPreDelete-1;
			int prefpostdelete = pPreferences->m_nRecPostDelete;
			int length = currentPattern->get_length();
			bool fp = false;
			postdelete = column;

			switch (prefpredelete) {
			case 0: predelete = length ; postdelete = 0; fp = true; break;
			case 1: predelete = length ; fp = true; break;
			case 2: predelete = length / 2; fp = true; break;
			case 3: predelete = length / 4; fp = true; break;
			case 4: predelete = length / 8; fp = true; break;
			case 5: predelete = length / 16; fp = true; break;
			case 6: predelete = length / 32; fp = true; break;
			case 7: predelete = length / 64; fp = true; break;
			case 8: predelete = length / 64; break;
			case 9: predelete = length / 32; break;
			case 10: predelete = length / 16; break;
			case 11: predelete = length / 8; break;
			case 12: predelete = length / 4; break;
			case 13: predelete = length / 2; break;
			case 14: predelete = length; break;
			case 15: break;
			default : predelete = 1; break;
			}

			if (!fp ) {
				switch (prefpostdelete) {
				case 0: postdelete = column; break;
				case 1: postdelete -= length / 64; break;
				case 2: postdelete -= length / 32; break;
				case 3: postdelete -= length / 16; break;
				case 4: postdelete -= length / 8; break;
				case 5: postdelete -= length / 4; break;
				case 6: postdelete -= length / 2; break;
				case 7: postdelete -= length ; break;
				default : postdelete = column; break;
				}

				if (postdelete<0) postdelete = 0;
			}

			Pattern::notes_t* notes = (Pattern::notes_t*)currentPattern->get_notes();
			FOREACH_NOTE_IT_BEGIN_END(notes,it) {
				Note *pNote = it->second;
				assert( pNote );

				int currentPosition = pNote->get_position();
				if ( pPreferences->__playselectedinstrument ) {//fix me
					if ( pSong->get_instrument_list()->get( getSelectedInstrumentNumber()) == pNote->get_instrument() )
					{
						if (prefpredelete>=1 && prefpredelete <=14 ) pNote->set_just_recorded( false );

						if ( (prefpredelete == 15) && (pNote->get_just_recorded() == false))
						{
							bool replaceExisting = false;
							if (column == currentPosition) replaceExisting = true;
							EventQueue::AddMidiNoteVector noteAction;
							noteAction.m_column = currentPosition;
							noteAction.m_row = pNote->get_instrument_id(); //getSelectedInstrumentNumber();
							noteAction.m_pattern = currentPatternNumber;
							noteAction.f_velocity = velocity;
							noteAction.f_pan_L = pan_L;
							noteAction.f_pan_R = pan_R;
							noteAction.m_length = -1;
							int divider = msg1 / 12;
							noteAction.no_octaveKeyVal = (Note::Octave)(divider -3);
							noteAction.nk_noteKeyVal = (Note::Key)(msg1 - (12 * divider));
							noteAction.b_isInstrumentMode = replaceExisting;
							noteAction.b_isMidi = true;
							noteAction.b_noteExist = replaceExisting;
							EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);
							continue;
						}
						if ( ( pNote->get_just_recorded() == false )
							 && (static_cast<int>( pNote->get_position() ) >= postdelete
								 && pNote->get_position() < column + predelete +1 )
							 ) {
							bool replaceExisting = false;
							if (column == currentPosition) replaceExisting = true;
							EventQueue::AddMidiNoteVector noteAction;
							noteAction.m_column = currentPosition;
							noteAction.m_row = pNote->get_instrument_id(); //getSelectedInstrumentNumber();
							noteAction.m_pattern = currentPatternNumber;
							noteAction.f_velocity = velocity;
							noteAction.f_pan_L = pan_L;
							noteAction.f_pan_R = pan_R;
							noteAction.m_length = -1;
							int divider = msg1 / 12;
							noteAction.no_octaveKeyVal = (Note::Octave)(divider -3);
							noteAction.nk_noteKeyVal = (Note::Key)(msg1 - (12 * divider));
							noteAction.b_isInstrumentMode = replaceExisting;
							noteAction.b_isMidi = true;
							noteAction.b_noteExist = replaceExisting;
							EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);
						}
					}
					continue;
				}

				if ( !fp && pNote->get_instrument() != instrRef ) {
					continue;
				}

				if (prefpredelete>=1 && prefpredelete <=14 )
					pNote->set_just_recorded( false );

				if ( (prefpredelete == 15) && (pNote->get_just_recorded() == false))
				{
					bool replaceExisting = false;
					if (column == currentPosition) replaceExisting = true;
					EventQueue::AddMidiNoteVector noteAction;
					noteAction.m_column = currentPosition;
					noteAction.m_row =  pNote->get_instrument_id();//m_nInstrumentLookupTable[ instrument ];
					noteAction.m_pattern = currentPatternNumber;
					noteAction.f_velocity = velocity;
					noteAction.f_pan_L = pan_L;
					noteAction.f_pan_R = pan_R;
					noteAction.m_length = -1;
					noteAction.no_octaveKeyVal = (Note::Octave)0;
					noteAction.nk_noteKeyVal = (Note::Key)0;
					noteAction.b_isInstrumentMode = false;
					noteAction.b_isMidi = false;
					noteAction.b_noteExist = replaceExisting;
					EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);
					continue;
				}

				if ( ( pNote->get_just_recorded() == false )
					 && ( static_cast<int>( pNote->get_position() ) >= postdelete
						  && pNote->get_position() <column + predelete +1 )
					 ) {
					bool replaceExisting = false;
					if (column == currentPosition) replaceExisting = true;
					EventQueue::AddMidiNoteVector noteAction;
					noteAction.m_column = currentPosition;
					noteAction.m_row =  pNote->get_instrument_id();//m_nInstrumentLookupTable[ instrument ];
					noteAction.m_pattern = currentPatternNumber;
					noteAction.f_velocity = velocity;
					noteAction.f_pan_L = pan_L;
					noteAction.f_pan_R = pan_R;
					noteAction.m_length = -1;
					noteAction.no_octaveKeyVal = (Note::Octave)0;
					noteAction.nk_noteKeyVal = (Note::Key)0;
					noteAction.b_isInstrumentMode = false;
					noteAction.b_isMidi = false;
					noteAction.b_noteExist = replaceExisting;
					EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);
				}
			} /* FOREACH */
		} /* if dorecord ... */

		assert( currentPattern );
		if ( doRecord ) {
			EventQueue::AddMidiNoteVector noteAction;
			noteAction.m_column = column;
			noteAction.m_pattern = currentPatternNumber;
			noteAction.f_velocity = velocity;
			noteAction.f_pan_L = pan_L;
			noteAction.f_pan_R = pan_R;
			noteAction.m_length = -1;
			noteAction.b_isMidi = true;

			if ( pPreferences->__playselectedinstrument ) {
				instrRef = pSong->get_instrument_list()->get( getSelectedInstrumentNumber() );
				int divider = msg1 / 12;
				noteAction.m_row = getSelectedInstrumentNumber();
				noteAction.no_octaveKeyVal = (Note::Octave)(divider -3);
				noteAction.nk_noteKeyVal = (Note::Key)(msg1 - (12 * divider));
				noteAction.b_isInstrumentMode = true;
			} else {
				instrRef = pSong->get_instrument_list()->get( m_nInstrumentLookupTable[ instrument ] );
				noteAction.m_row =  m_nInstrumentLookupTable[ instrument ];
				noteAction.no_octaveKeyVal = (Note::Octave)0;
				noteAction.nk_noteKeyVal = (Note::Key)0;
				noteAction.b_isInstrumentMode = false;
			}

			Note* pNoteold = currentPattern->find_note( noteAction.m_column, -1, instrRef, noteAction.nk_noteKeyVal, noteAction.no_octaveKeyVal );
			noteAction.b_noteExist = ( pNoteold ) ? true : false;

			EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);

			// hear note if its not in the future
			if ( pPreferences->getHearNewNotes() && position <= getTickPosition() ) {
				hearnote = true;
			}
		} /* if doRecord */
	} else if ( pPreferences->getHearNewNotes() ) {
			hearnote = true;
	} /* if .. STATE_PLAYING */

	if ( !pPreferences->__playselectedinstrument ) {
		if ( hearnote && instrRef ) {
			Note *pNote2 = new Note( instrRef, nRealColumn, velocity, pan_L, pan_R, -1, 0 );
			midi_noteOn( pNote2 );
		}
	} else if ( hearnote  ) {
		Instrument* pInstr = pSong->get_instrument_list()->get( getSelectedInstrumentNumber() );
		Note *pNote2 = new Note( pInstr, nRealColumn, velocity, pan_L, pan_R, -1, 0 );

		int divider = msg1 / 12;
		Note::Octave octave = (Note::Octave)(divider -3);
		Note::Key notehigh = (Note::Key)(msg1 - (12 * divider));

		//ERRORLOG( QString( "octave: %1, note: %2, instrument %3" ).arg( octave ).arg(notehigh).arg(instrument));
		pNote2->set_midi_info( notehigh, octave, msg1 );
		midi_noteOn( pNote2 );
	}

	AudioEngine::get_instance()->unlock(); // unlock the audio engine
}

float Hydrogen::getMasterPeak_L()
{
	return m_fMasterPeak_L;
}

float Hydrogen::getMasterPeak_R()
{
	return m_fMasterPeak_R;
}

unsigned long Hydrogen::getTickPosition()
{
	return m_nPatternTickPosition;
}

unsigned long Hydrogen::getRealtimeTickPosition()
{
	// Get the realtime transport position in frames and convert
	// it into ticks.
	unsigned int initTick = ( unsigned int )( getRealtimeFrames() /
						  m_pAudioDriver->m_transport.m_fTickSize );
	unsigned long retTick;

	struct timeval currtime;
	struct timeval deltatime;

	double sampleRate = ( double ) m_pAudioDriver->getSampleRate();
	gettimeofday ( &currtime, nullptr );

	// Definition macro from timehelper.h calculating the time
	// difference between `currtime` and `m_currentTickTime`
	// (`currtime`-`m_currentTickTime`) and storing the results in
	// `deltatime`. It uses both the .tv_sec (seconds) and
	// .tv_usec (microseconds) members of the timeval struct.
	timersub( &currtime, &m_currentTickTime, &deltatime );

	// add a buffers worth for jitter resistance
	double deltaSec =
			( double ) deltatime.tv_sec
			+ ( deltatime.tv_usec / 1000000.0 )
			+ ( m_pAudioDriver->getBufferSize() / ( double )sampleRate );

	retTick = ( unsigned long ) ( ( sampleRate / ( double ) m_pAudioDriver->m_transport.m_fTickSize ) * deltaSec );

	retTick += initTick;

	return retTick;
}

// TODO: make this function inline in the header
PatternList* Hydrogen::getCurrentPatternList()
{
	return m_pPlayingPatterns;
}

// TODO: make this function inline in the header
PatternList * Hydrogen::getNextPatterns()
{
	return m_pNextPatterns;
}

void Hydrogen::sequencer_setNextPattern( int pos )
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song* pSong = getSong();
	if ( pSong && pSong->get_mode() == Song::PATTERN_MODE ) {
		PatternList* pPatternList = pSong->get_pattern_list();
		
		// Check whether `pos` is in range of the pattern list.
		if ( ( pos >= 0 ) && ( pos < ( int )pPatternList->size() ) ) {
			Pattern* pPattern = pPatternList->get( pos );
			
			// If the pattern is already in the `m_pNextPatterns`, it
			// will be removed from the latter and its `del()` method
			// will return a pointer to the very pattern. The if
			// clause is therefore only entered if the `pPattern` was
			// not already present.
			if ( m_pNextPatterns->del( pPattern ) == nullptr ) {
				m_pNextPatterns->add( pPattern );
			}
		} else {
			ERRORLOG( QString( "pos not in patternList range. pos=%1 patternListSize=%2" )
					  .arg( pos ).arg( pPatternList->size() ) );
			m_pNextPatterns->clear();
		}
	} else {
		ERRORLOG( "can't set next pattern in song mode" );
		m_pNextPatterns->clear();
	}

	AudioEngine::get_instance()->unlock();
}

void Hydrogen::sequencer_setOnlyNextPattern( int pos )
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	
	Song* pSong = getSong();
	if ( pSong && pSong->get_mode() == Song::PATTERN_MODE ) {
		PatternList* pPatternList = pSong->get_pattern_list();
		
		// Clear the list of all patterns scheduled to be processed
		// next and fill them with those currently played.
		m_pNextPatterns->clear( );
		Pattern* pPattern;
		for ( int nPattern = 0 ; nPattern < (int)m_pPlayingPatterns->size() ; ++nPattern ) {
			pPattern = m_pPlayingPatterns->get( nPattern );
			m_pNextPatterns->add( pPattern );
		}
		
		// Appending the requested pattern.
		pPattern = pPatternList->get( pos );
		m_pNextPatterns->add( pPattern );
	} else {
		ERRORLOG( "can't set next pattern in song mode" );
		m_pNextPatterns->clear();
	}
	
	AudioEngine::get_instance()->unlock();
}

// TODO: make variable name and getter/setter consistent.
int Hydrogen::getPatternPos()
{
	return m_nSongPos;
}

/* Return pattern for selected song tick position */
int Hydrogen::getPosForTick( unsigned long TickPos, int* nPatternStartTick )
{
	Song* pSong = getSong();
	if ( pSong == nullptr ) {
		return 0;
	}

	return findPatternInTick( TickPos, pSong->is_loop_enabled(), nPatternStartTick );
}

int Hydrogen::calculateLeadLagFactor( float fTickSize ){
	return fTickSize * 5;
}

int Hydrogen::calculateLookahead( float fTickSize ){
	// Introduce a lookahead of 5 ticks. Since the ticksize is
	// depending of the current tempo of the song, this component does
	// make the lookahead dynamic.
	int nLeadLagFactor = calculateLeadLagFactor( fTickSize );

	// We need to look ahead in the song for notes with negative offsets
	// from LeadLag or Humanize.
	return nLeadLagFactor + m_nMaxTimeHumanize + 1;
}

void Hydrogen::restartDrivers()
{
	audioEngine_restartAudioDrivers();
}

void Hydrogen::startExportSession(int sampleRate, int sampleDepth )
{
	if ( getState() == STATE_PLAYING ) {
		sequencer_stop();
	}
	
	unsigned nSamplerate = (unsigned) sampleRate;
	
	AudioEngine::get_instance()->get_sampler()->stop_playing_notes();

	Song* pSong = getSong();
	
	m_oldEngineMode = pSong->get_mode();
	m_bOldLoopEnabled = pSong->is_loop_enabled();

	pSong->set_mode( Song::SONG_MODE );
	pSong->set_loop_enabled( true );
	
	/*
	 * Currently an audio driver is loaded
	 * which is not the DiskWriter driver.
	 * Stop the current driver and fire up the DiskWriter.
	 */
	audioEngine_stopAudioDrivers();

	m_pAudioDriver = new DiskWriterDriver( audioEngine_process, nSamplerate, sampleDepth );
	
	m_bExportSessionIsActive = true;
}

void Hydrogen::stopExportSession()
{
	m_bExportSessionIsActive = false;
	
 	audioEngine_stopAudioDrivers();
	
	delete m_pAudioDriver;
	m_pAudioDriver = nullptr;
	
	Song* pSong = getSong();
	pSong->set_mode( m_oldEngineMode );
	pSong->set_loop_enabled( m_bOldLoopEnabled );
	
	audioEngine_startAudioDrivers();

	if ( m_pAudioDriver ) {
		m_pAudioDriver->setBpm( pSong->__bpm );
	} else {
		ERRORLOG( "m_pAudioDriver = NULL" );
	}
}

/// Export a song to a wav file
void Hydrogen::startExportSong( const QString& filename)
{
	// reset
	m_pAudioDriver->m_transport.m_nFrames = 0; // reset total frames
	// TODO: not -1 instead?
	m_nSongPos = 0;
	m_nPatternTickPosition = 0;
	m_audioEngineState = STATE_PLAYING;
	m_nPatternStartTick = -1;

	Preferences *pPref = Preferences::get_instance();

	int res = m_pAudioDriver->init( pPref->m_nBufferSize );
	if ( res != 0 ) {
		ERRORLOG( "Error starting disk writer driver [DiskWriterDriver::init()]" );
	}

	m_pMainBuffer_L = m_pAudioDriver->getOut_L();
	m_pMainBuffer_R = m_pAudioDriver->getOut_R();

	audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );

	audioEngine_seek( 0, false );

	DiskWriterDriver* pDiskWriterDriver = (DiskWriterDriver*) m_pAudioDriver;
	pDiskWriterDriver->setFileName( filename );
	
	res = m_pAudioDriver->connect();
	if ( res != 0 ) {
		ERRORLOG( "Error starting disk writer driver [DiskWriterDriver::connect()]" );
	}
}

void Hydrogen::stopExportSong()
{
	if ( m_pAudioDriver->class_name() != DiskWriterDriver::class_name() ) {
		return;
	}

	AudioEngine::get_instance()->get_sampler()->stop_playing_notes();
	
	m_pAudioDriver->disconnect();

	m_nSongPos = -1;
	m_nPatternTickPosition = 0;
}

/// Used to display audio driver info
AudioOutput* Hydrogen::getAudioOutput()
{
	return m_pAudioDriver;
}

/// Used to display midi driver info
MidiInput* Hydrogen::getMidiInput()
{
	return m_pMidiDriver;
}

MidiOutput* Hydrogen::getMidiOutput()
{
	return m_pMidiDriverOut;
}

void Hydrogen::setMasterPeak_L( float value )
{
	m_fMasterPeak_L = value;
}

void Hydrogen::setMasterPeak_R( float value )
{
	m_fMasterPeak_R = value;
}

int Hydrogen::getState()
{
	return m_audioEngineState;
}

void Hydrogen::setCurrentPatternList( PatternList *pPatternList )
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	m_pPlayingPatterns = pPatternList;
	EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	AudioEngine::get_instance()->unlock();
}

float Hydrogen::getProcessTime()
{
	return m_fProcessTime;
}

float Hydrogen::getMaxProcessTime()
{
	return m_fMaxProcessTime;
}


// Setting conditional to true will keep instruments that have notes if new kit has less instruments than the old one
int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo )
{
	return loadDrumkit( pDrumkitInfo, true );
}

int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo, bool conditional )
{
	assert ( pDrumkitInfo );

	int old_ae_state = m_audioEngineState;
	if( m_audioEngineState >= STATE_READY ) {
		m_audioEngineState = STATE_PREPARED;
	}

	INFOLOG( pDrumkitInfo->get_name() );
	m_currentDrumkit = pDrumkitInfo->get_name();

	std::vector<DrumkitComponent*>* pSongCompoList= getSong()->get_components();
	std::vector<DrumkitComponent*>* pDrumkitCompoList = pDrumkitInfo->get_components();
	
	AudioEngine::get_instance()->lock( RIGHT_HERE );	
	for( auto &pComponent : *pSongCompoList ){
		delete pComponent;
	}
	pSongCompoList->clear();
	AudioEngine::get_instance()->unlock();
	
	for (std::vector<DrumkitComponent*>::iterator it = pDrumkitCompoList->begin() ; it != pDrumkitCompoList->end(); ++it) {
		DrumkitComponent* pSrcComponent = *it;
		DrumkitComponent* pNewComponent = new DrumkitComponent( pSrcComponent->get_id(), pSrcComponent->get_name() );
		pNewComponent->load_from( pSrcComponent );

		pSongCompoList->push_back( pNewComponent );
	}

	//current instrument list
	InstrumentList *pSongInstrList = getSong()->get_instrument_list();
	
	//new instrument list
	InstrumentList *pDrumkitInstrList = pDrumkitInfo->get_instruments();
	
	/*
	 * If the old drumkit is bigger then the new drumkit,
	 * delete all instruments with a bigger pos then
	 * pDrumkitInstrList->size(). Otherwise the instruments
	 * from our old instrumentlist with
	 * pos > pDrumkitInstrList->size() stay in the
	 * new instrumentlist
	 *
	 * wolke: info!
	 * this has moved to the end of this function
	 * because we get lost objects in memory
	 * now:
	 * 1. the new drumkit will loaded
	 * 2. all not used instruments will complete deleted
	 * old function:
	 * while ( pDrumkitInstrList->size() < songInstrList->size() )
	 * {
	 *  songInstrList->del(songInstrList->size() - 1);
	 * }
	 */
	
	//needed for the new delete function
	int instrumentDiff =  pSongInstrList->size() - pDrumkitInstrList->size();
	
	for ( unsigned nInstr = 0; nInstr < pDrumkitInstrList->size(); ++nInstr ) {
		Instrument *pInstr = nullptr;
		if ( nInstr < pSongInstrList->size() ) {
			//instrument exists already
			pInstr = pSongInstrList->get( nInstr );
			assert( pInstr );
		} else {
			pInstr = new Instrument();
			// The instrument isn't playing yet; no need for locking
			// :-) - Jakob Lund.  AudioEngine::get_instance()->lock(
			// "Hydrogen::loadDrumkit" );
			pSongInstrList->add( pInstr );
			// AudioEngine::get_instance()->unlock();
		}

		Instrument *pNewInstr = pDrumkitInstrList->get( nInstr );
		assert( pNewInstr );
		INFOLOG( QString( "Loading instrument (%1 of %2) [%3]" )
				 .arg( nInstr + 1 )
				 .arg( pDrumkitInstrList->size() )
				 .arg( pNewInstr->get_name() ) );

		// Moved code from here right into the Instrument class - Jakob Lund.
		pInstr->load_from( pDrumkitInfo, pNewInstr );
	}

	//wolke: new delete function
	if ( instrumentDiff >= 0 ) {
		for ( int i = 0; i < instrumentDiff ; i++ ){
			removeInstrument(
						getSong()->get_instrument_list()->size() - 1,
						conditional
						);
		}
	}

#ifdef H2CORE_HAVE_JACK
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	renameJackPorts( getSong() );
	AudioEngine::get_instance()->unlock();
#endif

	m_audioEngineState = old_ae_state;
	
	m_pCoreActionController->initExternalControlInterfaces();

	return 0;	//ok
}

// This will check if an instrument has any notes
bool Hydrogen::instrumentHasNotes( Instrument *pInst )
{
	Song* pSong = getSong();
	PatternList* pPatternList = pSong->get_pattern_list();

	for ( int nPattern = 0 ; nPattern < (int)pPatternList->size() ; ++nPattern )
	{
		if( pPatternList->get( nPattern )->references( pInst ) )
		{
			DEBUGLOG("Instrument " + pInst->get_name() + " has notes" );
			return true;
		}
	}

	// no notes for this instrument
	return false;
}

//this is also a new function and will used from the new delete function in
//Hydrogen::loadDrumkit to delete the instruments by number
void Hydrogen::removeInstrument( int instrumentNumber, bool conditional )
{
	Song* pSong = getSong();
	Instrument *pInstr = pSong->get_instrument_list()->get( instrumentNumber );
	PatternList* pPatternList = pSong->get_pattern_list();

	if ( conditional ) {
		// new! this check if a pattern has an active note if there is an note
		//inside the pattern the instrument would not be deleted
		for ( int nPattern = 0 ;
			  nPattern < (int)pPatternList->size() ;
			  ++nPattern ) {
			if( pPatternList
					->get( nPattern )
					->references( pInstr ) ) {
				DEBUGLOG("Keeping instrument #" + QString::number( instrumentNumber ) );
				return;
			}
		}
	} else {
		getSong()->purge_instrument( pInstr );
	}

	InstrumentList* pList = pSong->get_instrument_list();
	if ( pList->size()==1 ){
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		Instrument* pInstr = pList->get( 0 );
		pInstr->set_name( (QString( "Instrument 1" )) );
		for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			InstrumentComponent* pCompo = *it;
			// remove all layers
			for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
				pCompo->set_layer( nullptr, nLayer );
			}
		}
		AudioEngine::get_instance()->unlock();
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
		INFOLOG("clear last instrument to empty instrument 1 instead delete the last instrument");
		return;
	}

	// if the instrument was the last on the instruments list, select the
	// next-last
	if ( instrumentNumber >= (int)getSong()->get_instrument_list()->size() - 1 ) {
		Hydrogen::get_instance()->setSelectedInstrumentNumber(
					std::max(0, instrumentNumber - 1 )
					);
	}
	//
	// delete the instrument from the instruments list
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	getSong()->get_instrument_list()->del( instrumentNumber );
	// Ensure the selected instrument is not a deleted one
	setSelectedInstrumentNumber( instrumentNumber - 1 );
	getSong()->set_is_modified( true );
	AudioEngine::get_instance()->unlock();

	// At this point the instrument has been removed from both the
	// instrument list and every pattern in the song.  Hence there's no way
	// (NOTE) to play on that instrument, and once all notes have stopped
	// playing it will be save to delete.
	// the ugly name is just for debugging...
	QString xxx_name = QString( "XXX_%1" ) . arg( pInstr->get_name() );
	pInstr->set_name( xxx_name );
	__instrument_death_row.push_back( pInstr );
	__kill_instruments(); // checks if there are still notes.

	// this will force a GUI update.
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

void Hydrogen::raiseError( unsigned nErrorCode )
{
	audioEngine_raiseError( nErrorCode );
}

unsigned long Hydrogen::getTotalFrames()
{
	return m_pAudioDriver->m_transport.m_nFrames;
}

void Hydrogen::setRealtimeFrames( unsigned long frames )
{
	m_nRealtimeFrames = frames;
}

unsigned long Hydrogen::getRealtimeFrames()
{
	return m_nRealtimeFrames;
}


long Hydrogen::getTickForPosition( int pos )
{
	Song* pSong = getSong();

	int nPatternGroups = pSong->get_pattern_group_vector()->size();
	if ( nPatternGroups == 0 ) {
		return -1;
	}

	if ( pos >= nPatternGroups ) {
		// The position is beyond the end of the Song, we
		// set periodic boundary conditions or return the
		// beginning of the Song as a fallback.
		if ( pSong->is_loop_enabled() ) {
			pos = pos % nPatternGroups;
		} else {
			WARNINGLOG( QString( "patternPos > nPatternGroups. pos:"
								 " %1, nPatternGroups: %2")
						.arg( pos ) .arg(  nPatternGroups )
						);
			return -1;
		}
	}

	std::vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();
	long totalTick = 0;
	int nPatternSize;
	Pattern *pPattern = nullptr;
	
	for ( int i = 0; i < pos; ++i ) {
		PatternList *pColumn = ( *pColumns )[ i ];
		
		if( pColumn->size() > 0)
		{
			pPattern = pColumn->get( 0 );
			if ( pPattern ) {
				nPatternSize = pPattern->get_length();
			} else {
				nPatternSize = MAX_NOTES;
			}
		} else {
			nPatternSize = MAX_NOTES;
		}
		totalTick += nPatternSize;
	}
	
	return totalTick;
}

void Hydrogen::setPatternPos( int pos )
{
	if ( pos < -1 ) {
		pos = -1;
	}
	
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	// TODO: why?
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
	long totalTick = getTickForPosition( pos );
	if ( totalTick < 0 ) {
		AudioEngine::get_instance()->unlock();
		return;
	}

	if ( getState() != STATE_PLAYING ) {
		// find pattern immediately when not playing
		//		int dummy;
		// 		m_nSongPos = findPatternInTick( totalTick,
		//					        pSong->is_loop_enabled(),
		//					        &dummy );
		m_nSongPos = pos;
		m_nPatternTickPosition = 0;
	}
	m_pAudioDriver->locate(
				( int ) ( totalTick * m_pAudioDriver->m_transport.m_fTickSize )
				);

	AudioEngine::get_instance()->unlock();
}

void Hydrogen::getLadspaFXPeak( int nFX, float *fL, float *fR )
{
#ifdef H2CORE_HAVE_LADSPA
	( *fL ) = m_fFXPeak_L[nFX];
	( *fR ) = m_fFXPeak_R[nFX];
#else
	( *fL ) = 0;
	( *fR ) = 0;
#endif
}

void Hydrogen::setLadspaFXPeak( int nFX, float fL, float fR )
{
#ifdef H2CORE_HAVE_LADSPA
	m_fFXPeak_L[nFX] = fL;
	m_fFXPeak_R[nFX] = fR;
#endif
}

void Hydrogen::onTapTempoAccelEvent()
{
#ifndef WIN32
	INFOLOG( "tap tempo" );
	static timeval oldTimeVal;

	struct timeval now;
	gettimeofday(&now, nullptr);

	float fInterval =
			(now.tv_sec - oldTimeVal.tv_sec) * 1000.0
			+ (now.tv_usec - oldTimeVal.tv_usec) / 1000.0;

	oldTimeVal = now;

	if ( fInterval < 1000.0 ) {
		setTapTempo( fInterval );
	}
#endif
}

void Hydrogen::setTapTempo( float fInterval )
{

	//	infoLog( "set tap tempo" );
	static float fOldBpm1 = -1;
	static float fOldBpm2 = -1;
	static float fOldBpm3 = -1;
	static float fOldBpm4 = -1;
	static float fOldBpm5 = -1;
	static float fOldBpm6 = -1;
	static float fOldBpm7 = -1;
	static float fOldBpm8 = -1;

	float fBPM = 60000.0 / fInterval;

	if ( fabs( fOldBpm1 - fBPM ) > 20 ) {	// troppa differenza, niente media
		fOldBpm1 = fBPM;
		fOldBpm2 = fBPM;
		fOldBpm3 = fBPM;
		fOldBpm4 = fBPM;
		fOldBpm5 = fBPM;
		fOldBpm6 = fBPM;
		fOldBpm7 = fBPM;
		fOldBpm8 = fBPM;
	}

	if ( fOldBpm1 == -1 ) {
		fOldBpm1 = fBPM;
		fOldBpm2 = fBPM;
		fOldBpm3 = fBPM;
		fOldBpm4 = fBPM;
		fOldBpm5 = fBPM;
		fOldBpm6 = fBPM;
		fOldBpm7 = fBPM;
		fOldBpm8 = fBPM;
	}

	fBPM = ( fBPM + fOldBpm1 + fOldBpm2 + fOldBpm3 + fOldBpm4 + fOldBpm5
			 + fOldBpm6 + fOldBpm7 + fOldBpm8 ) / 9.0;

	INFOLOG( QString( "avg BPM = %1" ).arg( fBPM ) );
	fOldBpm8 = fOldBpm7;
	fOldBpm7 = fOldBpm6;
	fOldBpm6 = fOldBpm5;
	fOldBpm5 = fOldBpm4;
	fOldBpm4 = fOldBpm3;
	fOldBpm3 = fOldBpm2;
	fOldBpm2 = fOldBpm1;
	fOldBpm1 = fBPM;

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	setBPM( fBPM );

	AudioEngine::get_instance()->unlock();
}

void Hydrogen::setBPM( float fBPM )
{
	Song* pSong = getSong();
	if ( ! m_pAudioDriver || ! pSong ){
		return;
	}

	m_pAudioDriver->setBpm( fBPM );
	pSong->__bpm = fBPM;
	setNewBpmJTM ( fBPM );
}

void Hydrogen::restartLadspaFX()
{
	if ( m_pAudioDriver ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );
		AudioEngine::get_instance()->unlock();
	} else {
		ERRORLOG( "m_pAudioDriver = NULL" );
	}
}

int Hydrogen::getSelectedPatternNumber()
{
	return m_nSelectedPatternNumber;
}


void Hydrogen::setSelectedPatternNumberWithoutGuiEvent( int nPat )
{
	Song* pSong = getSong();

	if ( nPat == m_nSelectedPatternNumber
		 || ( nPat + 1 > pSong->get_pattern_list()->size() )
		 ) return;

	if ( Preferences::get_instance()->patternModePlaysSelected() ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		m_nSelectedPatternNumber = nPat;
		AudioEngine::get_instance()->unlock();
	} else {
		m_nSelectedPatternNumber = nPat;
	}
}

void Hydrogen::setSelectedPatternNumber( int nPat )
{
	if ( nPat == m_nSelectedPatternNumber )	return;


	if ( Preferences::get_instance()->patternModePlaysSelected() ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		m_nSelectedPatternNumber = nPat;
		AudioEngine::get_instance()->unlock();
	} else {
		m_nSelectedPatternNumber = nPat;
	}

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}

int Hydrogen::getSelectedInstrumentNumber()
{
	return m_nSelectedInstrumentNumber;
}

void Hydrogen::setSelectedInstrumentNumber( int nInstrument )
{
	if ( m_nSelectedInstrumentNumber == nInstrument )	return;

	m_nSelectedInstrumentNumber = nInstrument;
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

void Hydrogen::refreshInstrumentParameters( int nInstrument )
{
	EventQueue::get_instance()->push_event( EVENT_PARAMETERS_INSTRUMENT_CHANGED, -1 );
}

#ifdef H2CORE_HAVE_JACK
void Hydrogen::renameJackPorts( Song *pSong )
{
	if( Preferences::get_instance()->m_bJackTrackOuts == true ){
		audioEngine_renameJackPorts(pSong);
	}
}
#endif

/** Updates #m_nbeatsToCount
 * \param beatstocount New value*/
void Hydrogen::setbeatsToCount( int beatstocount)
{
	m_nbeatsToCount = beatstocount;
}
/** \return #m_nbeatsToCount*/
int Hydrogen::getbeatsToCount()
{
	return m_nbeatsToCount;
}

void Hydrogen::setNoteLength( float notelength)
{
	m_ntaktoMeterCompute = notelength;
}

float Hydrogen::getNoteLength()
{
	return m_ntaktoMeterCompute;
}

int Hydrogen::getBcStatus()
{
	return m_nEventCount;
}

void Hydrogen::setBcOffsetAdjust()
{
	//individual fine tuning for the m_nBeatCounter
	//to adjust  ms_offset from different people and controller
	Preferences *pPreferences = Preferences::get_instance();

	m_nCoutOffset = pPreferences->m_countOffset;
	m_nStartOffset = pPreferences->m_startOffset;
}

void Hydrogen::handleBeatCounter()
{
	// Get first time value:
	if (m_nBeatCount == 1)
		gettimeofday(&m_CurrentTime,nullptr);

	m_nEventCount++;

	// Set wm_LastTime to wm_CurrentTime to remind the time:
	m_LastTime = m_CurrentTime;

	// Get new time:
	gettimeofday(&m_CurrentTime,nullptr);


	// Build doubled time difference:
	m_nLastBeatTime = (double)(
				m_LastTime.tv_sec
				+ (double)(m_LastTime.tv_usec * US_DIVIDER)
				+ (int)m_nCoutOffset * .0001
				);
	m_nCurrentBeatTime = (double)(
				m_CurrentTime.tv_sec
				+ (double)(m_CurrentTime.tv_usec * US_DIVIDER)
				);
	m_nBeatDiff = m_nBeatCount == 1 ? 0 : m_nCurrentBeatTime - m_nLastBeatTime;

	//if differences are to big reset the beatconter
	if( m_nBeatDiff > 3.001 * 1/m_ntaktoMeterCompute ){
		m_nEventCount = 1;
		m_nBeatCount = 1;
		return;
	}
	// Only accept differences big enough
	if (m_nBeatCount == 1 || m_nBeatDiff > .001) {
		if (m_nBeatCount > 1)
			m_nBeatDiffs[m_nBeatCount - 2] = m_nBeatDiff ;
		// Compute and reset:
		if (m_nBeatCount == m_nbeatsToCount){
			//				unsigned long currentframe = getRealtimeFrames();
			double beatTotalDiffs = 0;
			for(int i = 0; i < (m_nbeatsToCount - 1); i++)
				beatTotalDiffs += m_nBeatDiffs[i];
			double m_nBeatDiffAverage =
					beatTotalDiffs
					/ (m_nBeatCount - 1)
					* m_ntaktoMeterCompute ;
			m_fBeatCountBpm	 =
					(float) ((int) (60 / m_nBeatDiffAverage * 100))
					/ 100;
			AudioEngine::get_instance()->lock( RIGHT_HERE );
			if ( m_fBeatCountBpm > MAX_BPM) {
				m_fBeatCountBpm = MAX_BPM;
			}
			
			setBPM( m_fBeatCountBpm );
			AudioEngine::get_instance()->unlock();
			if (Preferences::get_instance()->m_mmcsetplay
					== Preferences::SET_PLAY_OFF) {
				m_nBeatCount = 1;
				m_nEventCount = 1;
			}else{
				if ( m_audioEngineState != STATE_PLAYING ){
					unsigned bcsamplerate =
							m_pAudioDriver->getSampleRate();
					unsigned long rtstartframe = 0;
					if ( m_ntaktoMeterCompute <= 1){
						rtstartframe =
								bcsamplerate
								* m_nBeatDiffAverage
								* ( 1/ m_ntaktoMeterCompute );
					}else
					{
						rtstartframe =
								bcsamplerate
								* m_nBeatDiffAverage
								/ m_ntaktoMeterCompute ;
					}

					int sleeptime =
							( (float) rtstartframe
							  / (float) bcsamplerate
							  * (int) 1000 )
							+ (int)m_nCoutOffset
							+ (int) m_nStartOffset;
#ifdef WIN32
					Sleep( sleeptime );
#else
					usleep( 1000 * sleeptime );
#endif

					sequencer_play();
				}

				m_nBeatCount = 1;
				m_nEventCount = 1;
				return;
			}
		}
		else {
			m_nBeatCount ++;
		}
	}
	return;
}
//~ m_nBeatCounter

#ifdef H2CORE_HAVE_JACK
void Hydrogen::offJackMaster()
{
	if ( haveJackTransport() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->releaseTimebaseMaster();
	}
}

void Hydrogen::onJackMaster()
{
	if ( haveJackTransport() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->initTimebaseMaster();
	}
}
#endif

long Hydrogen::getPatternLength( int nPattern )
{
	Song* pSong = getSong();
	if ( pSong == nullptr ){
		return -1;
	}

	std::vector< PatternList* > *pColumns = pSong->get_pattern_group_vector();

	int nPatternGroups = pColumns->size();
	if ( nPattern >= nPatternGroups ) {
		if ( pSong->is_loop_enabled() ) {
			nPattern = nPattern % nPatternGroups;
		} else {
			return MAX_NOTES;
		}
	}

	if ( nPattern < 1 ){
		return MAX_NOTES;
	}

	PatternList* pPatternList = pColumns->at( nPattern - 1 );
	Pattern* pPattern = pPatternList->get( 0 );
	if ( pPattern ) {
		return pPattern->get_length();
	} else {
		return MAX_NOTES;
	}
}

float Hydrogen::getNewBpmJTM()
{
	return m_fNewBpmJTM;
}

void Hydrogen::setNewBpmJTM( float bpmJTM )
{
	m_fNewBpmJTM = bpmJTM;
}

//~ jack transport master
void Hydrogen::resetPatternStartTick()
{
	// This forces the barline position
	if ( getSong()->get_mode() == Song::PATTERN_MODE ) {
		m_nPatternStartTick = -1;
	}
}

void Hydrogen::togglePlaysSelected()
{
	Song* pSong = getSong();

	if ( pSong->get_mode() != Song::PATTERN_MODE )
		return;

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Preferences* pPref = Preferences::get_instance();
	bool isPlaysSelected = pPref->patternModePlaysSelected();

	if (isPlaysSelected) {
		m_pPlayingPatterns->clear();
		Pattern* pSelectedPattern =
				pSong->get_pattern_list()->get(m_nSelectedPatternNumber);
		m_pPlayingPatterns->add( pSelectedPattern );
	}

	pPref->setPatternModePlaysSelected( !isPlaysSelected );
	AudioEngine::get_instance()->unlock();
}

void Hydrogen::__kill_instruments()
{
	int c = 0;
	Instrument * pInstr = nullptr;
	while ( __instrument_death_row.size()
			&& __instrument_death_row.front()->is_queued() == 0 ) {
		pInstr = __instrument_death_row.front();
		__instrument_death_row.pop_front();
		INFOLOG( QString( "Deleting unused instrument (%1). "
						  "%2 unused remain." )
				 . arg( pInstr->get_name() )
				 . arg( __instrument_death_row.size() ) );
		delete pInstr;
		c++;
	}
	if ( __instrument_death_row.size() ) {
		pInstr = __instrument_death_row.front();
		INFOLOG( QString( "Instrument %1 still has %2 active notes. "
						  "Delaying 'delete instrument' operation." )
				 . arg( pInstr->get_name() )
				 . arg( pInstr->is_queued() ) );
	}
}



void Hydrogen::__panic()
{
	sequencer_stop();
	AudioEngine::get_instance()->get_sampler()->stop_playing_notes();
}

unsigned int Hydrogen::__getMidiRealtimeNoteTickPosition()
{
	return m_naddrealtimenotetickposition;
}

float Hydrogen::getTimelineBpm( int nBar )
{
	Song* pSong = getSong();

	// We need return something
	if ( pSong == nullptr ) {
		return getNewBpmJTM();
	}

	float fBPM = pSong->__bpm;

	// Pattern mode don't use timeline and will have a constant
	// speed.
	if ( pSong->get_mode() == Song::PATTERN_MODE ) {
		return fBPM;
	}

	// Check whether the user wants Hydrogen to determine the
	// speed by local setting along the timeline or whether she
	// wants to use a global speed instead.
	if ( ! Preferences::get_instance()->getUseTimelineBpm() ) {
		return fBPM;
	}

	// Determine the speed at the supplied beat.
	for ( int i = 0; i < static_cast<int>(m_pTimeline->m_timelinevector.size()); i++) {
		if ( m_pTimeline->m_timelinevector[i].m_htimelinebeat > nBar )
			break;

		fBPM = m_pTimeline->m_timelinevector[i].m_htimelinebpm;
	}

	return fBPM;
}

void Hydrogen::setTimelineBpm()
{
	if ( ! Preferences::get_instance()->getUseTimelineBpm() ||
		 haveJackTimebaseClient() ) {
		return;
	}

	Song* pSong = getSong();
	// Obtain the local speed specified for the current Pattern.
	float fBPM = getTimelineBpm( getPatternPos() );
		
	if ( fBPM != pSong->__bpm ) {
		setBPM( fBPM );
	}

	// Get the realtime pattern position. This also covers
	// keyboard and MIDI input events in case the audio engine is
	// not playing.
	unsigned long PlayTick = getRealtimeTickPosition();
	int nStartPos;
	int nRealtimePatternPos = getPosForTick( PlayTick, &nStartPos );
	float fRealtimeBPM = getTimelineBpm( nRealtimePatternPos );

	// FIXME: this was already done in setBPM but for "engine" time
	//        so this is actually forcibly overwritten here
	setNewBpmJTM( fRealtimeBPM );
}

bool Hydrogen::haveJackAudioDriver() const {
#ifdef H2CORE_HAVE_JACK
	if ( JackAudioDriver::class_name() == m_pAudioDriver->class_name() ){
		return true;
	} else {
		return false;
	}
#else
	return false;
#endif	
}

bool Hydrogen::haveJackTransport() const {
#ifdef H2CORE_HAVE_JACK
	if ( JackAudioDriver::class_name() == m_pAudioDriver->class_name() &&
		 Preferences::get_instance()->m_bJackTransportMode ==
	     Preferences::USE_JACK_TRANSPORT ){
		return true;
	} else {
		return false;
	}
#else
	return false;
#endif	
}

bool Hydrogen::haveJackTimebaseClient() const {
#ifdef H2CORE_HAVE_JACK
	if ( haveJackTransport() ) {
		if ( static_cast<JackAudioDriver*>(m_pAudioDriver)->getIsTimebaseMaster() == 0 ) {
			return true;
		}
	} 
	return false;
#else
	return false;
#endif	
}

#ifdef H2CORE_HAVE_OSC
void startOscServer()
{
	OscServer* pOscServer = OscServer::get_instance();
	
	if( pOscServer ){
		pOscServer->start();
	}
}

void Hydrogen::startNsmClient()
{
	//NSM has to be started before jack driver gets created
	NsmClient* pNsmClient = NsmClient::get_instance();

	if(pNsmClient){
		pNsmClient->createInitialClient();
	}
}
#endif

}; /* Namespace */
