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

#include <hydrogen/LocalFileMng.h>
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
#include <hydrogen/playlist.h>
#include <hydrogen/timeline.h>

#ifdef H2CORE_HAVE_OSC
#include <hydrogen/nsm_client.h>
#include <hydrogen/osc_server.h>
#endif

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/JackAudioDriver.h>
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
float					m_fMasterPeak_L = 0.0f;		///< Master peak (left channel)
float					m_fMasterPeak_R = 0.0f;		///< Master peak (right channel)
float					m_fProcessTime = 0.0f;		///< time used in process function
float					m_fMaxProcessTime = 0.0f;	///< max ms usable in process with no xrun
//~ info


//jack time master
float					m_nNewBpmJTM = 120;
unsigned long			m_nHumantimeFrames = 0;
//~ jack time master

AudioOutput *			m_pAudioDriver = NULL;	///< Audio output
QMutex					mutex_OutputPointer;     ///< Mutex for audio output pointer, allows multiple readers
///< When locking this AND AudioEngine, always lock AudioEngine first.
MidiInput *				m_pMidiDriver = NULL;	///< MIDI input
MidiOutput *			m_pMidiDriverOut = NULL;	///< MIDI output

// overload the the > operator of Note objects for priority_queue
struct compare_pNotes {
	bool operator() (Note* pNote1, Note* pNote2) {
		return (pNote1->get_humanize_delay()
				+ pNote1->get_position() * m_pAudioDriver->m_transport.m_nTickSize)
				>
				(pNote2->get_humanize_delay()
				 + pNote2->get_position() * m_pAudioDriver->m_transport.m_nTickSize);
	}
};

/// Song Note FIFO
std::priority_queue<Note*, std::deque<Note*>, compare_pNotes > m_songNoteQueue;
std::deque<Note*>		m_midiNoteQueue;	///< Midi Note FIFO

PatternList*			m_pNextPatterns;		///< Next pattern (used only in Pattern mode)
bool					m_bAppendNextPattern;		///< Add the next pattern to the list instead of replace.
bool					m_bDeleteNextPattern;		///< Delete the next pattern from the list.

PatternList*			m_pPlayingPatterns;
int						m_nSongPos;				///< Is the position inside the song

int						m_nSelectedPatternNumber;
int						m_nSelectedInstrumentNumber;

Instrument *			m_pMetronomeInstrument = NULL;	///< Metronome instrument

// Buffers used in the process function
unsigned				m_nBufferSize = 0;
float *					m_pMainBuffer_L = NULL;
float *					m_pMainBuffer_R = NULL;

Hydrogen*				hydrogenInstance = NULL;   ///< Hydrogen class instance (used for log)

int						m_audioEngineState = STATE_UNINITIALIZED;	///< Audio engine state

#ifdef H2CORE_HAVE_LADSPA
float					m_fFXPeak_L[MAX_FX];
float					m_fFXPeak_R[MAX_FX];
#endif

int						m_nPatternStartTick = -1;
unsigned int			m_nPatternTickPosition = 0;
int						m_nLookaheadFrames = 0;

// used in findPatternInTick
int						m_nSongSizeInTicks = 0;

struct timeval			m_currentTickTime;

unsigned long			m_nRealtimeFrames = 0;
unsigned int			m_naddrealtimenotetickposition = 0;

// PROTOTYPES
void					audioEngine_init();
void					audioEngine_destroy();
int						audioEngine_start( bool bLockEngine = false, unsigned nTotalFrames = 0 );
void					audioEngine_stop( bool bLockEngine = false );
void					audioEngine_setSong(Song *pNewSong );
void					audioEngine_removeSong();
static void				audioEngine_noteOn( Note *note );

int						audioEngine_process( uint32_t nframes, void *arg );
inline void				audioEngine_clearNoteQueue();
inline void				audioEngine_process_checkBPMChanged(Song *pSong);
inline void				audioEngine_process_playNotes( unsigned long nframes );
inline void				audioEngine_process_transport();

inline unsigned			audioEngine_renderNote( Note* pNote, const unsigned& nBufferSize );
inline int				audioEngine_updateNoteQueue( unsigned nFrames );
inline void				audioEngine_prepNoteQueue();

inline int				findPatternInTick( int tick, bool loopMode, int *patternStartTick );

void					audioEngine_seek( long long nFrames, bool bLoopMode = false );

void					audioEngine_restartAudioDrivers();
void					audioEngine_startAudioDrivers();
void					audioEngine_stopAudioDrivers();

inline timeval currentTime2()
{
	struct timeval now;
	gettimeofday( &now, NULL );
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

void updateTickSize()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	float sampleRate = ( float ) m_pAudioDriver->getSampleRate();
	m_pAudioDriver->m_transport.m_nTickSize =
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
	m_pMetronomeInstrument = NULL;
	m_pAudioDriver = NULL;

	m_pMainBuffer_L = NULL;
	m_pMainBuffer_R = NULL;

	srand( time( NULL ) );

	// Create metronome instrument
	QString sMetronomeFilename = Filesystem::click_file();
	m_pMetronomeInstrument =
			new Instrument( METRONOME_INSTR_ID, "metronome" );
	InstrumentLayer* pLayer = new InstrumentLayer( Sample::load( sMetronomeFilename ) );
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
	m_pPlayingPatterns = NULL;

	delete m_pNextPatterns;
	m_pNextPatterns = NULL;

	delete m_pMetronomeInstrument;
	m_pMetronomeInstrument = NULL;

	AudioEngine::get_instance()->unlock();
}

/// Start playing
/// return 0 = OK
/// return -1 = NULL Audio Driver
/// return -2 = Driver connect() error
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
	m_pAudioDriver->m_transport.m_nFrames = nTotalFrames;	// reset total frames
	m_nSongPos = -1;
	m_nPatternStartTick = -1;
	m_nPatternTickPosition = 0;

	// prepare the tickSize for this song
	updateTickSize();

	// change the current audio engine state
	m_audioEngineState = STATE_PLAYING;
	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_PLAYING );

	if ( bLockEngine ) {
		AudioEngine::get_instance()->unlock();
	}
	return 0; // per ora restituisco sempre OK
}

/// Stop the audio engine
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

//
///  Update Tick size and frame position in the audio driver from Song->__bpm
//
inline void audioEngine_process_checkBPMChanged(Song* pSong)
{
	if ( m_audioEngineState != STATE_READY
	  && m_audioEngineState != STATE_PLAYING
	) return;

	float fOldTickSize = m_pAudioDriver->m_transport.m_nTickSize;
	float fNewTickSize = m_pAudioDriver->getSampleRate() * 60.0 / pSong->__bpm / pSong->__resolution;

	// Nothing changed - avoid recomputing
	if ( fNewTickSize == fOldTickSize )
		return;

	// update tick size in transport class
	m_pAudioDriver->m_transport.m_nTickSize = fNewTickSize;

	if ( fNewTickSize == 0 || fOldTickSize == 0 )
		return;

	___WARNINGLOG( "Tempo change: Recomputing ticksize and frame position" );
	float fTickNumber = m_pAudioDriver->m_transport.m_nFrames / fOldTickSize;

	// update frame position in transport class
	m_pAudioDriver->m_transport.m_nFrames = ceil(fTickNumber) * fNewTickSize;

#ifdef H2CORE_HAVE_JACK
	if ( JackAudioDriver::class_name() == m_pAudioDriver->class_name()
		&& m_audioEngineState == STATE_PLAYING )
	{
		static_cast< JackAudioDriver* >( m_pAudioDriver )->calculateFrameOffset();
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
				(int)( pNote->get_position() * m_pAudioDriver->m_transport.m_nTickSize );

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
				/ m_pAudioDriver->m_transport.m_nTickSize );
	//	sprintf(tmp, "[audioEngine_seek()] tickNumber_start = %d", tickNumber_start);
	//	hydrogenInstance->infoLog(tmp);

	bool loop = pSong->is_loop_enabled();

	if ( bLoopMode ) {
		loop = true;
	}

	m_nSongPos = findPatternInTick( tickNumber_start, loop, &m_nPatternStartTick );
	//	sprintf(tmp, "[audioEngine_seek()] m_nSongPos = %d", m_nSongPos);
	//	hydrogenInstance->infoLog(tmp);

	audioEngine_clearNoteQueue();
}

inline void audioEngine_process_transport()
{
	if ( m_audioEngineState != STATE_READY
	  && m_audioEngineState != STATE_PLAYING
	) return;

	m_pAudioDriver->updateTransportInfo();

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	// Update frame position
	// ??? audioEngine_seek returns IMMEDIATELY
	// when nNewFrames == m_pAudioDriver->m_transport.m_nFrames ???
	// audioEngine_seek( nNewFrames, true );
	switch ( m_pAudioDriver->m_transport.m_status ) {
	case TransportInfo::ROLLING:
		if ( m_audioEngineState == STATE_READY ) {
			// false == no engine lock. Already locked
			// this should set STATE_PLAYING
			audioEngine_start( false, m_pAudioDriver->m_transport.m_nFrames );
		}

		// So, we are not playing even after attempt to start engine
		if ( m_audioEngineState != STATE_PLAYING ) return;

		/* Now we're playing | Update BPM */
		if ( pSong->__bpm != m_pAudioDriver->m_transport.m_nBPM ) {
			___INFOLOG( QString( "song bpm: (%1) gets transport bpm: (%2)" )
				.arg( pSong->__bpm )
				.arg( m_pAudioDriver->m_transport.m_nBPM )
			);
			pHydrogen->setBPM ( m_pAudioDriver->m_transport.m_nBPM );
		}

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

/// Clear all audio buffers
inline void audioEngine_process_clearAudioBuffers( uint32_t nFrames )
{
	QMutexLocker mx( &mutex_OutputPointer );

	// clear main out Left and Right
	if ( m_pAudioDriver ) {
		m_pMainBuffer_L = m_pAudioDriver->getOut_L();
		m_pMainBuffer_R = m_pAudioDriver->getOut_R();
	} else {
		m_pMainBuffer_L = m_pMainBuffer_R = 0;
	}
	if ( m_pMainBuffer_L ) {
		memset( m_pMainBuffer_L, 0, nFrames * sizeof( float ) );
	}
	if ( m_pMainBuffer_R ) {
		memset( m_pMainBuffer_R, 0, nFrames * sizeof( float ) );
	}

#ifdef H2CORE_HAVE_JACK
	JackAudioDriver * jo = dynamic_cast<JackAudioDriver*>(m_pAudioDriver);
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

/// Main audio processing function. Called by audio drivers.
int audioEngine_process( uint32_t nframes, void* /*arg*/ )
{
	timeval startTimeval = currentTime2();

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
					.arg( nframes )
					);
		m_nBufferSize = nframes;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

	audioEngine_process_transport();
	audioEngine_process_checkBPMChanged(pSong); // pSong->__bpm decides tick size

	bool sendPatternChange = false;
	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	int res2 = audioEngine_updateNoteQueue( nframes );
	if ( res2 == -1 ) {	// end of song
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

#ifdef H2CORE_HAVE_JACK
		else if ( m_pAudioDriver->class_name() == JackAudioDriver::class_name() )
		{
			// Do something clever :-s ... Jakob Lund
			// Mainly to keep sync with Ardour.
			static_cast<JackAudioDriver*>(m_pAudioDriver)->locateInNCycles( 0 );
		}
#endif

		return 0;
	} else if ( res2 == 2 ) { // send pattern change
		sendPatternChange = true;
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

	AudioEngine::get_instance()->unlock();

	if ( sendPatternChange ) {
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
		if ( pFX == NULL ) {
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

void audioEngine_renameJackPorts(Song * pSong)
{
#ifdef H2CORE_HAVE_JACK
	// renames jack ports
	if ( ! pSong ) return;

	if ( m_pAudioDriver->class_name() == JackAudioDriver::class_name() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->makeTrackOutputs( pSong );
	}
#endif
}

void audioEngine_setSong( Song * pNewSong )
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

	// update ticksize
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

// return -1 = end of song
// return 2 = send pattern changed event!!
inline int audioEngine_updateNoteQueue( unsigned nFrames )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();

//	static int nLastTick = -1;
	bool bSendPatternChange = false;
	int nMaxTimeHumanize = 2000;
	int nLeadLagFactor = m_pAudioDriver->m_transport.m_nTickSize * 5;  // 5 ticks

	unsigned int framepos;
	if (  m_audioEngineState == STATE_PLAYING ) {
		framepos = m_pAudioDriver->m_transport.m_nFrames;
	} else {
		// use this to support realtime events when not playing
		framepos = pHydrogen->getRealtimeFrames();
	}

	// We need to look ahead in the song for notes with negative offsets
	// from LeadLag or Humanize.  When starting from the beginning, we prime
	// the note queue with notes between 0 and nFrames plus
	// lookahead. lookahead should be equal or greater than the
	// nLeadLagFactor + nMaxTimeHumanize.
	int lookahead = nLeadLagFactor + nMaxTimeHumanize + 1;
	m_nLookaheadFrames = lookahead;

	int tickNumber_start = 0;
	if ( framepos == 0
		 || ( m_audioEngineState == STATE_PLAYING
			  && pSong->get_mode() == Song::SONG_MODE
			  && m_nSongPos == -1 )
	) {
		tickNumber_start = framepos / m_pAudioDriver->m_transport.m_nTickSize;
	} else {
		tickNumber_start = ( framepos + lookahead) / m_pAudioDriver->m_transport.m_nTickSize;
	}
	int tickNumber_end = ( framepos + nFrames + lookahead ) / m_pAudioDriver->m_transport.m_nTickSize;

	// 	___WARNINGLOG( "Lookahead: " + to_string( lookahead
	//	                                        / m_pAudioDriver->m_transport.m_nTickSize ) );
	// get initial timestamp for first tick
	gettimeofday( &m_currentTickTime, NULL );

	for ( int tick = tickNumber_start; tick < tickNumber_end; tick++ ) {
		// midi events now get put into the m_songNoteQueue as well,
		// based on their timestamp
		while ( m_midiNoteQueue.size() > 0 ) {
			Note *note = m_midiNoteQueue[0];
			if ( note->get_position() > tick ) break;

			// printf ("tick=%d  pos=%d\n", tick, note->getPosition());
			m_midiNoteQueue.pop_front();
			note->get_instrument()->enqueue();
			m_songNoteQueue.push( note );
		}

		if (  m_audioEngineState != STATE_PLAYING ) {
			// only keep going if we're playing
			continue;
		}

		// 		if ( m_nPatternStartTick == -1 ) { // for debugging pattern mode :s
		// 			___WARNINGLOG( "m_nPatternStartTick == -1; tick = "
		//			             + to_string( tick ) );
		// 		}


		// SONG MODE
		bool doErase = m_audioEngineState == STATE_PLAYING
				&& Preferences::get_instance()->getRecordEvents()
				&& Preferences::get_instance()->getDestructiveRecord()
				&& Preferences::get_instance()->m_nRecPreDelete == 0;
		if ( pSong->get_mode() == Song::SONG_MODE ) {
			if ( pSong->get_pattern_group_vector()->size() == 0 ) {
				// there's no song!!
				___ERRORLOG( "no patterns in song." );
				m_pAudioDriver->stop();
				return -1;
			}

			m_nSongPos = findPatternInTick( tick, pSong->is_loop_enabled(), &m_nPatternStartTick );

			if ( m_nSongSizeInTicks != 0 ) {
				m_nPatternTickPosition = ( tick - m_nPatternStartTick )
						% m_nSongSizeInTicks;
			} else {
				m_nPatternTickPosition = tick - m_nPatternStartTick;
			}

			if ( m_nPatternTickPosition == 0 ) {
				bSendPatternChange = true;
			}

			// PatternList *pPatternList = (*(pSong->getPatternGroupVector()))[m_nSongPos];
			if ( m_nSongPos == -1 ) {
				___INFOLOG( "song pos = -1" );
				if ( pSong->is_loop_enabled() == true ) {
					m_nSongPos = findPatternInTick( 0, true, &m_nPatternStartTick );
				} else {

					___INFOLOG( "End of Song" );

					if( Hydrogen::get_instance()->getMidiOutput() != NULL ){
						Hydrogen::get_instance()->getMidiOutput()->handleQueueAllNoteOff();
					}

					return -1;
				}
			}
			PatternList *pPatternList = ( *( pSong->get_pattern_group_vector() ) )[m_nSongPos];
			m_pPlayingPatterns->clear();
			for ( int i=0; i< pPatternList->size(); ++i ) {
				Pattern* pattern = pPatternList->get(i);
				m_pPlayingPatterns->add( pattern );
				pattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}
			// Set destructive record depending on punch area
			doErase = doErase && Preferences::get_instance()->inPunchArea(m_nSongPos);
		}
		// PATTERN MODE
		else if ( pSong->get_mode() == Song::PATTERN_MODE )	{
			// per ora considero solo il primo pattern, se ce ne
			// saranno piu' di uno bisognera' prendere quello piu'
			// piccolo

			//m_nPatternTickPosition = tick % m_pCurrentPattern->getSize();
			int nPatternSize = MAX_NOTES;

			if ( Preferences::get_instance()->patternModePlaysSelected() )
			{
				m_pPlayingPatterns->clear();
				Pattern * pattern = pSong->get_pattern_list()->get(m_nSelectedPatternNumber);
				m_pPlayingPatterns->add( pattern );
				pattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}

			if ( m_pPlayingPatterns->size() != 0 ) {
				Pattern *pFirstPattern = m_pPlayingPatterns->get( 0 );
				nPatternSize = pFirstPattern->get_length();
			}

			if ( nPatternSize == 0 ) {
				___ERRORLOG( "nPatternSize == 0" );
			}

			if ( ( tick == m_nPatternStartTick + nPatternSize )
				 || ( m_nPatternStartTick == -1 ) ) {
				if ( m_pNextPatterns->size() > 0 ) {
					Pattern * p;
					for ( uint i = 0; i < m_pNextPatterns->size(); i++ ) {
						p = m_pNextPatterns->get( i );
						// ___WARNINGLOG( QString( "Got pattern # %1" ).arg( i + 1 ) );
						// if the pattern isn't playing, already, start it now.
						if ( ( m_pPlayingPatterns->del( p ) ) == NULL ) {
							m_pPlayingPatterns->add( p );
						}
					}
					m_pNextPatterns->clear();
					bSendPatternChange = true;
				}
				if ( m_nPatternStartTick == -1 ) {
					m_nPatternStartTick = tick - (tick % nPatternSize);
					// ___WARNINGLOG( "set Pattern Start Tick to " ) + to_string( m_nPatternStartTick ) );
				} else {
					m_nPatternStartTick = tick;
				}
			}

			m_nPatternTickPosition = tick - m_nPatternStartTick;
			if ( m_nPatternTickPosition > nPatternSize ) {
				m_nPatternTickPosition = tick % nPatternSize;
			}
		}

		// metronome
		// if (  ( m_nPatternStartTick == tick ) || ( ( tick - m_nPatternStartTick ) % 48 == 0 ) ) 
		if ( m_nPatternTickPosition % 48 == 0 ) {
			float fPitch;
			float fVelocity;
			// 			___INFOLOG( "Beat: " + to_string(m_nPatternTickPosition / 48 + 1)
			//				   + "@ " + to_string( tick ) );
			if ( m_nPatternTickPosition == 0 ) {
				fPitch = 3;
				fVelocity = 1.0;
				EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
			} else {
				fPitch = 0;
				fVelocity = 0.8;
				EventQueue::get_instance()->push_event( EVENT_METRONOME, 0 );
			}
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

		// update the notes queue
		if ( m_pPlayingPatterns->size() != 0 ) {
			for ( unsigned nPat = 0 ;
				  nPat < m_pPlayingPatterns->size() ;
				  ++nPat ) {
				Pattern *pPattern = m_pPlayingPatterns->get( nPat );
				assert( pPattern != NULL );
				Pattern::notes_t* notes = (Pattern::notes_t*)pPattern->get_notes();
				// Delete notes before attempting to play them
				if ( doErase ) {
					FOREACH_NOTE_IT_BOUND(notes,it,m_nPatternTickPosition) {
						Note* pNote = it->second;
						assert( pNote != NULL );
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

				// Now play notes
				FOREACH_NOTE_CST_IT_BOUND(notes,it,m_nPatternTickPosition) {
					Note *pNote = it->second;
					if ( pNote ) {
						pNote->set_just_recorded( false );
						int nOffset = 0;

						// Swing
						float fSwingFactor = pSong->get_swing_factor();

						if ( ( ( m_nPatternTickPosition % 12 ) == 0 )
							 && ( ( m_nPatternTickPosition % 24 ) != 0 ) ) {
							// da l'accento al tick 4, 12, 20, 36...
							nOffset += ( int )(
										6.0
										* m_pAudioDriver->m_transport.m_nTickSize
										* fSwingFactor
										);
						}

						// Humanize - Time parameter
						if ( pSong->get_humanize_time_value() != 0 ) {
							nOffset += ( int )(
										getGaussian( 0.3 )
										* pSong->get_humanize_time_value()
										* nMaxTimeHumanize
										);
						}
						//~
						// Lead or Lag - timing parameter
						nOffset += (int) ( pNote->get_lead_lag()
										   * nLeadLagFactor);
						//~

						if((tick == 0) && (nOffset < 0)) {
							nOffset = 0;
						}
						Note *pCopiedNote = new Note( pNote );
						pCopiedNote->set_position( tick );

						// humanize time
						pCopiedNote->set_humanize_delay( nOffset );
						pNote->get_instrument()->enqueue();
						m_songNoteQueue.push( pCopiedNote );
						//pCopiedNote->dumpInfo();
					}
				}
			}
		}
	}

	// audioEngine_process must send the pattern change event after mutex unlock
	if ( bSendPatternChange ) {
		return 2;
	}
	return 0;
}

/// restituisce l'indice relativo al patternGroup in base al tick
inline int findPatternInTick( int nTick, bool bLoopMode, int *pPatternStartTick )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	assert( pSong );

	int nTotalTick = 0;
	m_nSongSizeInTicks = 0;

	std::vector<PatternList*> *pPatternColumns = pSong->get_pattern_group_vector();
	int nColumns = pPatternColumns->size();

	int nPatternSize;
	for ( int i = 0; i < nColumns; ++i ) {
		PatternList *pColumn = ( *pPatternColumns )[ i ];
		if ( pColumn->size() != 0 ) {
			// tengo in considerazione solo il primo pattern. I
			// pattern nel gruppo devono avere la stessa lunghezza.
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
				// tengo in considerazione solo il primo
				// pattern. I pattern nel gruppo devono avere la
				// stessa lunghezza.
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

	QString err = QString( "[findPatternInTick] tick = %1. No pattern found" ).arg( QString::number(nTick) );
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

AudioOutput* createDriver( const QString& sDriver )
{
	___INFOLOG( QString( "Driver: '%1'" ).arg( sDriver ) );
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *pDriver = NULL;

	if ( sDriver == "Oss" ) {
		pDriver = new OssDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = NULL;
		}
	} else if ( sDriver == "Jack" ) {
		pDriver = new JackAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = NULL;
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
			pDriver = NULL;
		}
	} else if ( sDriver == "PortAudio" ) {
		pDriver = new PortAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = NULL;
		}
	}
	//#ifdef Q_OS_MACX
	else if ( sDriver == "CoreAudio" ) {
		___INFOLOG( "Creating CoreAudioDriver" );
		pDriver = new CoreAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = NULL;
		}
	}
	//#endif
	else if ( sDriver == "PulseAudio" ) {
		pDriver = new PulseAudioDriver( audioEngine_process );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = NULL;
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
			pDriver = NULL;
		}
	}

	return pDriver;
}

/// Start all audio drivers
void audioEngine_startAudioDrivers()
{
	Preferences *preferencesMng = Preferences::get_instance();

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
		if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == NULL ) {
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == NULL ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == NULL ) {
					if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == NULL ) {
						if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == NULL ) {
							if ( ( m_pAudioDriver = createDriver( "PulseAudio" ) ) == NULL ) {
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
		if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == NULL ) {
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == NULL ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == NULL ) {
					if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == NULL ) {
						if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == NULL ) {
							if ( ( m_pAudioDriver = createDriver( "PulseAudio" ) ) == NULL ) {
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
		if ( m_pAudioDriver == NULL ) {
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
		m_pMidiDriver = new PortMidiDriver();
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

		if ( ( m_pMainBuffer_L = m_pAudioDriver->getOut_L() ) == NULL ) {
			___ERRORLOG( "m_pMainBuffer_L == NULL" );
		}
		if ( ( m_pMainBuffer_R = m_pAudioDriver->getOut_R() ) == NULL ) {
			___ERRORLOG( "m_pMainBuffer_R == NULL" );
		}

#ifdef H2CORE_HAVE_JACK
		audioEngine_renameJackPorts( pSong );
#endif

		audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );
	}


}

/// Stop all audio drivers
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
		m_pMidiDriver = NULL;
		m_pMidiDriverOut = NULL;
	}

	// delete audio driver
	if ( m_pAudioDriver ) {
		m_pAudioDriver->disconnect();
		QMutexLocker mx( &mutex_OutputPointer );
		delete m_pAudioDriver;
		m_pAudioDriver = NULL;
		mx.unlock();
	}

	AudioEngine::get_instance()->unlock();
}



/// Restart all audio and midi drivers
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

/// static reference of Hydrogen class (Singleton)
Hydrogen* Hydrogen::__instance = NULL;
const char* Hydrogen::__class_name = "Hydrogen";

Hydrogen::Hydrogen()
	: Object( __class_name )
{
	if ( __instance ) {
		ERRORLOG( "Hydrogen audio engine is already running" );
		throw H2Exception( "Hydrogen audio engine is already running" );
	}

	INFOLOG( "[Hydrogen]" );

	__song = NULL;

	m_bExportSessionIsActive = false;
	m_pTimeline = new Timeline();

	hydrogenInstance = this;

	initBeatcounter();
	// 	__instance = this;
	audioEngine_init();
	// Prevent double creation caused by calls from MIDI thread
	__instance = this;

	audioEngine_startAudioDrivers();
	for(int i = 0; i< MAX_INSTRUMENTS; i++){
		m_nInstrumentLookupTable[i] = i;
	}

	if( Preferences::get_instance()->getOscServerEnabled() )
	{
		OscServer* pOscServer = OscServer::get_instance();
		pOscServer->start();
	}
}

Hydrogen::~Hydrogen()
{
	INFOLOG( "[~Hydrogen]" );

#ifdef H2CORE_HAVE_OSC
	NsmClient* pNsmClient = NsmClient::get_instance();

	if(pNsmClient){
		pNsmClient->shutdown();
	}
#endif


	if ( m_audioEngineState == STATE_PLAYING ) {
		audioEngine_stop();
	}
	removeSong();
	audioEngine_stopAudioDrivers();
	audioEngine_destroy();
	__kill_instruments();

	delete m_pTimeline;

	__instance = NULL;
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

	if ( __instance == 0 ) {
		__instance = new Hydrogen;
	}

	// See audioEngine_init() for:
	// AudioEngine::create_instance();
	// Effects::create_instance();
	// Playlist::create_instance();
}

void Hydrogen::initBeatcounter(void)
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
	if( Hydrogen::get_instance()->getMidiOutput() != NULL ){
		Hydrogen::get_instance()->getMidiOutput()->handleQueueAllNoteOff();
	}

	m_pAudioDriver->stop();
	Preferences::get_instance()->setRecordEvents(false);
}

void Hydrogen::setSong( Song *pSong )
{
	assert ( pSong );

	/* Set first pattern */
	setSelectedPatternNumber( 0 );

	/* Delete previous Song
	*  NOTE: current approach support only one Song
	*        loaded at the same time
	*/
	Song* oldSong = getSong();
	if ( oldSong ) {
		delete oldSong;
		oldSong = NULL;

		/* NOTE: this is actually some kind of cleanup */
		removeSong();
	}

	/* Reset GUI */
	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
	EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	audioEngine_setSong ( pSong );

	__song = pSong;
}

/* Mean: remove current song from memory */
void Hydrogen::removeSong()
{
	__song = NULL;
	audioEngine_removeSong();
}

void Hydrogen::midi_noteOn( Note *note )
{
	audioEngine_noteOn( note );
}

void Hydrogen::addRealtimeNote( int instrument,
								float velocity,
								float pan_L,
								float pan_R,
								float pitch,
								bool noteOff,
								bool forcePlay,
								int msg1 )
{
	UNUSED( pitch );

	Preferences *pref = Preferences::get_instance();
	unsigned int realcolumn = 0;
	unsigned res = pref->getPatternEditorGridResolution();
	int nBase = pref->isPatternEditorUsingTriplets() ? 3 : 4;
	int scalar = ( 4 * MAX_NOTES ) / ( res * nBase );
	bool hearnote = forcePlay;
	int currentPatternNumber;

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *pSong = getSong();
	if ( !pref->__playselectedinstrument ) {
		if ( instrument >= ( int ) pSong->get_instrument_list()->size() ) {
			// unused instrument
			AudioEngine::get_instance()->unlock();
			return;
		}
	}

	// Get current partern and column, compensating for "lookahead" if required
	Pattern* currentPattern = NULL;
	unsigned int column = 0;
	unsigned int lookaheadTicks = m_nLookaheadFrames / m_pAudioDriver->m_transport.m_nTickSize;
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
		if ( currentPattern == NULL ) {
			std::vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();
			for ( int i = 0; i <= ipattern; ++i ) {
				PatternList *pColumn = ( *pColumns )[i];
				currentPattern = pColumn->get( 0 );
				currentPatternNumber = i;
			}
		}

		// Cancel recording if punch area disagrees
		doRecord = pref->inPunchArea( ipattern );

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

	realcolumn = getRealtimeTickPosition();

	if ( pref->getQuantizeEvents() ) {
		// quantize it to scale
		unsigned qcolumn = ( unsigned )::round( column / ( double )scalar ) * scalar;

		//we have to make sure that no beat is added on the last displayed note in a bar
		//for example: if the pattern has 4 beats, the editor displays 5 beats, so we should avoid adding beats an note 5.
		if ( qcolumn == currentPattern->get_length() ) qcolumn = 0;
		column = qcolumn;
	}

	unsigned position = column;
	m_naddrealtimenotetickposition = column;

	Instrument *instrRef = 0;
	if ( pSong ) {
		//getlookuptable index = instrument+36, ziel wert = der entprechende wert -36
		instrRef = pSong->get_instrument_list()->get( m_nInstrumentLookupTable[ instrument ] );
	}

	if ( currentPattern && ( getState() == STATE_PLAYING ) ) {
		if ( doRecord && pref->getDestructiveRecord() && pref->m_nRecPreDelete>0 ) {
			// Delete notes around current note if option toggled
			int postdelete = 0;
			int predelete = 0;
			int prefpredelete = pref->m_nRecPreDelete-1;
			int prefpostdelete = pref->m_nRecPostDelete;
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
				if ( pref->__playselectedinstrument ) {//fix me
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

			if ( pref->__playselectedinstrument ) {
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
			if ( pref->getHearNewNotes() && position <= getTickPosition() )
				hearnote = true;
		} /* if doRecord */
	} else if ( pref->getHearNewNotes() ) {
			hearnote = true;
	} /* if .. STATE_PLAYING */

	if ( !pref->__playselectedinstrument ) {
		if ( hearnote && instrRef ) {
			Note *note2 = new Note( instrRef, realcolumn, velocity, pan_L, pan_R, -1, 0 );
			midi_noteOn( note2 );
		}
	} else if ( hearnote  ) {
		Instrument* pInstr = pSong->get_instrument_list()->get( getSelectedInstrumentNumber() );
		Note *note2 = new Note( pInstr, realcolumn, velocity, pan_L, pan_R, -1, 0 );

		int divider = msg1 / 12;
		Note::Octave octave = (Note::Octave)(divider -3);
		Note::Key notehigh = (Note::Key)(msg1 - (12 * divider));

		//ERRORLOG( QString( "octave: %1, note: %2, instrument %3" ).arg( octave ).arg(notehigh).arg(instrument));
		note2->set_midi_info( notehigh, octave, msg1 );
		midi_noteOn( note2 );
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
	//unsigned long initTick = audioEngine_getTickPosition();
	unsigned int initTick = ( unsigned int )( getRealtimeFrames() / m_pAudioDriver->m_transport.m_nTickSize );
	unsigned long retTick;

	struct timeval currtime;
	struct timeval deltatime;

	double sampleRate = ( double ) m_pAudioDriver->getSampleRate();
	gettimeofday ( &currtime, NULL );

	timersub( &currtime, &m_currentTickTime, &deltatime );

	// add a buffers worth for jitter resistance
	double deltaSec =
			( double ) deltatime.tv_sec
			+ ( deltatime.tv_usec / 1000000.0 )
			+ ( m_pAudioDriver->getBufferSize() / ( double )sampleRate );

	retTick = ( unsigned long ) ( ( sampleRate / ( double ) m_pAudioDriver->m_transport.m_nTickSize ) * deltaSec );

	retTick += initTick;

	return retTick;
}

PatternList* Hydrogen::getCurrentPatternList()
{
	return m_pPlayingPatterns;
}

PatternList * Hydrogen::getNextPatterns()
{
	return m_pNextPatterns;
}

/// Set the next pattern (Pattern mode only)
void Hydrogen::sequencer_setNextPattern( int pos )
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song* pSong = getSong();
	if ( pSong && pSong->get_mode() == Song::PATTERN_MODE ) {
		PatternList *pPatternList = pSong->get_pattern_list();
		Pattern * pPattern = pPatternList->get( pos );
		if ( ( pos >= 0 ) && ( pos < ( int )pPatternList->size() ) ) {
			// if p is already on the next pattern list, delete it.
			if ( m_pNextPatterns->del( pPattern ) == NULL ) {
				// WARNINGLOG( "Adding to nextPatterns" );
				m_pNextPatterns->add( pPattern );
			} /* else {
				// WARNINGLOG( "Removing " + to_string(pos) );
			}*/
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

int Hydrogen::getPatternPos()
{
	return m_nSongPos;
}

/* Return pattern for selected song tick position */
int Hydrogen::getPosForTick( unsigned long TickPos )
{
	Song* pSong = getSong();
	if ( ! pSong ) return 0;

	int patternStartTick;
	return findPatternInTick( TickPos, pSong->is_loop_enabled(), &patternStartTick );
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

	pSongCompoList->clear();
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
  If the old drumkit is bigger then the new drumkit,
  delete all instruments with a bigger pos then
  pDrumkitInstrList->size(). Otherwise the instruments
  from our old instrumentlist with
  pos > pDrumkitInstrList->size() stay in the
  new instrumentlist

 wolke: info!
  this has moved to the end of this function
  because we get lost objects in memory
  now:
  1. the new drumkit will loaded
  2. all not used instruments will complete deleted

 old funktion:
 while ( pDrumkitInstrList->size() < songInstrList->size() )
 {
  songInstrList->del(songInstrList->size() - 1);
 }
 */

	//needed for the new delete function
	int instrumentDiff =  pSongInstrList->size() - pDrumkitInstrList->size();

	for ( unsigned nInstr = 0; nInstr < pDrumkitInstrList->size(); ++nInstr ) {
		Instrument *pInstr = NULL;
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
				 .arg( nInstr )
				 .arg( pDrumkitInstrList->size() )
				 .arg( pNewInstr->get_name() ) );

		// creo i nuovi layer in base al nuovo strumento
		// Moved code from here right into the Instrument class - Jakob Lund.
		pInstr->load_from( pDrumkitInfo, pNewInstr );
	}

	//wolke: new delete funktion
	if ( instrumentDiff >=0 ) {
		int p;	// last position in instrument list
		p = getSong()->get_instrument_list()->size() - 1;

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
void Hydrogen::removeInstrument( int instrumentnumber, bool conditional )
{
	Song* pSong = getSong();
	Instrument *pInstr = pSong->get_instrument_list()->get( instrumentnumber );


	PatternList* pPatternList = pSong->get_pattern_list();

	if ( conditional ) {
		// new! this check if a pattern has an active note if there is an note
		//inside the pattern the intrument would not be deleted
		for ( int nPattern = 0 ;
			  nPattern < (int)pPatternList->size() ;
			  ++nPattern ) {
			if( pPatternList
					->get( nPattern )
					->references( pInstr ) ) {
				DEBUGLOG("Keeping instrument #" + QString::number( instrumentnumber ) );
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
			for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
				InstrumentLayer* pLayer = pCompo->get_layer( nLayer );
				delete pLayer;
				pCompo->set_layer( NULL, nLayer );
			}
		}
		AudioEngine::get_instance()->unlock();
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
		INFOLOG("clear last instrument to empty instrument 1 instead delete the last instrument");
		return;
	}

	// if the instrument was the last on the instruments list, select the
	// next-last
	if ( instrumentnumber
		 >= (int)getSong()->get_instrument_list()->size() - 1 ) {
		Hydrogen::get_instance()
				->setSelectedInstrumentNumber(
					std::max(0, instrumentnumber - 1 )
					);
	}
	//
	// delete the instrument from the instruments list
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	getSong()->get_instrument_list()->del( instrumentnumber );
	// Ensure the selected instrument is not a deleted one
	setSelectedInstrumentNumber( instrumentnumber - 1 );
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

/**
 * Get the ticks for pattern at pattern pos
 * @a int pos -- position in song
 * @return -1 if pos > number of patterns in the song, tick no. > 0 otherwise
 * The driver should be LOCKED when calling this!!
 */
long Hydrogen::getTickForPosition( int pos )
{
	Song* pSong = getSong();

	int nPatternGroups = pSong->get_pattern_group_vector()->size();
	if ( nPatternGroups == 0 ) return -1;

	if ( pos >= nPatternGroups ) {
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
	Pattern *pPattern = NULL;
	for ( int i = 0; i < pos; ++i ) {
		PatternList *pColumn = ( *pColumns )[ i ];
		// prendo solo il primo. I pattern nel gruppo devono avere la
		// stessa lunghezza
		pPattern = pColumn->get( 0 );
		if ( pPattern ) {
			nPatternSize = pPattern->get_length();
		} else {
			nPatternSize = MAX_NOTES;
		}
		totalTick += nPatternSize;
	}
	return totalTick;
}

/// Set the position in the song
void Hydrogen::setPatternPos( int pos )
{
	if ( pos < -1 )
		pos = -1;
	AudioEngine::get_instance()->lock( RIGHT_HERE );
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
				( int ) ( totalTick * m_pAudioDriver->m_transport.m_nTickSize )
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
	gettimeofday(&now, NULL);

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

	// 	m_pAudioDriver->setBpm( fBPM );
	// 	pSong->setBpm( fBPM );

	setBPM( fBPM );

	AudioEngine::get_instance()->unlock();
}

// Called with audioEngine in LOCKED state.
void Hydrogen::setBPM( float fBPM )
{
	Song* pSong = getSong();
	if ( ! m_pAudioDriver || ! pSong ) return;

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
	// FIXME: controllare se e' valido..
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

///m_nBeatCounter
void Hydrogen::setbeatsToCount( int beatstocount)
{
	m_nbeatsToCount = beatstocount;
}

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
		gettimeofday(&m_CurrentTime,NULL);

	m_nEventCount++;

	// Set wm_LastTime to wm_CurrentTime to remind the time:
	m_LastTime = m_CurrentTime;

	// Get new time:
	gettimeofday(&m_CurrentTime,NULL);


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
			if ( m_fBeatCountBpm > 500)
				m_fBeatCountBpm = 500;
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

// jack transport master
unsigned long Hydrogen::getHumantimeFrames()
{
	return m_nHumantimeFrames;
}

void Hydrogen::setHumantimeFrames(unsigned long hframes)
{
	m_nHumantimeFrames = hframes;
}

#ifdef H2CORE_HAVE_JACK
void Hydrogen::offJackMaster()
{
	if ( m_pAudioDriver->class_name() == JackAudioDriver::class_name() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->com_release();
	}
}

void Hydrogen::onJackMaster()
{
	if ( m_pAudioDriver->class_name() == JackAudioDriver::class_name() ) {
		static_cast< JackAudioDriver* >( m_pAudioDriver )->initTimeMaster();
	}
}

unsigned long Hydrogen::getTimeMasterFrames()
{
	float allframes = 0 ;

	if ( m_pAudioDriver->m_transport.m_status == TransportInfo::STOPPED ){

		int oldtick = getTickPosition();
		for (int i = 0; i <= getPatternPos(); i++){
			float framesforposition =
					(long)getTickForHumanPosition(i)
					* (float)m_pAudioDriver->m_transport.m_nTickSize;
			allframes = framesforposition + allframes;
		}
		unsigned long framesfortimemaster = (unsigned int)(
					allframes
					+ oldtick * (float)m_pAudioDriver->m_transport.m_nTickSize
					);
		m_nHumantimeFrames = framesfortimemaster;
		return framesfortimemaster;
	}else
	{
		return m_nHumantimeFrames;
	}
}
#endif

long Hydrogen::getTickForHumanPosition( int humanpos )
{
	Song* pSong = getSong();
	if ( ! pSong ){
		return -1;
	}

	std::vector< PatternList* > * pColumns = pSong->get_pattern_group_vector();

	int nPatternGroups = pColumns->size();
	if ( humanpos >= nPatternGroups ) {
		if ( pSong->is_loop_enabled() ) {
			humanpos = humanpos % nPatternGroups;
		} else {
			return MAX_NOTES;
		}
	}

	if ( humanpos < 1 ){
		return MAX_NOTES;
	}

	PatternList* pPatternList = pColumns->at( humanpos - 1 );
	Pattern *pPattern = pPatternList->get( 0 );
	if ( pPattern ) {
		return pPattern->get_length();
	} else {
		return MAX_NOTES;
	}
}

float Hydrogen::getNewBpmJTM()
{
	return m_nNewBpmJTM;
}

void Hydrogen::setNewBpmJTM( float bpmJTM )
{
	m_nNewBpmJTM = bpmJTM;
}

void Hydrogen::ComputeHumantimeFrames(uint32_t nFrames)
{
	if ( m_audioEngineState == STATE_PLAYING )
	{
		m_nHumantimeFrames = nFrames + m_nHumantimeFrames;
	}
}

//~ jack transport master
void Hydrogen::triggerRelocateDuringPlay()
{
	// This forces the barline position
	if ( getSong()->get_mode() == Song::PATTERN_MODE )
		m_nPatternStartTick = -1;
}

void Hydrogen::togglePlaysSelected()
{
	Song* pSong = getSong();

	if ( pSong->get_mode() != Song::PATTERN_MODE )
		return;

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Preferences *pPref = Preferences::get_instance();
	bool isPlaysSelected = pPref->patternModePlaysSelected();

	if (isPlaysSelected) {
		m_pPlayingPatterns->clear();
		Pattern * pSelectedPattern =
				pSong->get_pattern_list()->get(m_nSelectedPatternNumber);
		m_pPlayingPatterns->add( pSelectedPattern );
	}

	pPref->setPatternModePlaysSelected( !isPlaysSelected );
	AudioEngine::get_instance()->unlock();
}

void Hydrogen::__kill_instruments()
{
	int c = 0;
	Instrument * pInstr = NULL;
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

int Hydrogen::__get_selected_PatterNumber()
{
	return m_nSelectedPatternNumber;
}

unsigned int Hydrogen::__getMidiRealtimeNoteTickPosition()
{
	return m_naddrealtimenotetickposition;
}


// Get TimelineBPM for Pos
float Hydrogen::getTimelineBpm( int Beat )
{
	Song* pSong = getSong();

	// We need return something
	if ( ! pSong ) return getNewBpmJTM();

	float bpm = pSong->__bpm;

	// Pattern mode don't use timeline
	if ( pSong->get_mode() == Song::PATTERN_MODE )
		return bpm;

	//time line test
	if ( ! Preferences::get_instance()->getUseTimelineBpm() )
		return bpm;

	for ( int i = 0; i < static_cast<int>(m_pTimeline->m_timelinevector.size()); i++) {
		if ( m_pTimeline->m_timelinevector[i].m_htimelinebeat > Beat )
			break;

		bpm = m_pTimeline->m_timelinevector[i].m_htimelinebpm;
	}

	return bpm;
}

void Hydrogen::setTimelineBpm()
{
	//time line test
	if ( ! Preferences::get_instance()->getUseTimelineBpm() ) return;

	// Update "engine" BPM
	Song* pSong = getSong();
	float BPM = getTimelineBpm ( getPatternPos() );
	if ( BPM != pSong->__bpm )
		setBPM( BPM );

	// Update "realtime" BPM
	unsigned long PlayTick = getRealtimeTickPosition();
	int RealtimePatternPos = getPosForTick ( PlayTick );
	float RealtimeBPM = getTimelineBpm ( RealtimePatternPos );

	// FIXME: this was already done in setBPM but for "engine" time
	//        so this is actually forcibly overwritten here
	setNewBpmJTM( RealtimeBPM );
}

#ifdef H2CORE_HAVE_OSC
void startOscServer()
{
	OscServer* pOscServer = OscServer::get_instance();
	
	if(pOscServer){
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
