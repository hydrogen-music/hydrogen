/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Hydrogen.cpp,v 1.70 2005/07/18 20:43:01 comix Exp $
 *
 */
#include "config.h"

#ifdef WIN32
	#include "timeHelper.h"
	#include "timersub.h"
#else
	#include <unistd.h>
	#include <sys/time.h>
#endif

#include <pthread.h>
#include <cassert>
#include <cstdio>
#include <deque>
#include <iostream>
#include <ctime>
#include <math.h>
using std::cout;
using std::cerr;
using std::endl;

#include "Sample.h"
#include "LocalFileMng.h"
#include "Hydrogen.h"
#include "EventQueue.h"
#include "ADSR.h"
#include "lib/fx/LadspaFX.h"

#include "drivers/JackDriver.h"
#include "drivers/OssDriver.h"
#include "drivers/NullDriver.h"
#include "drivers/FakeDriver.h"
#include "drivers/AlsaAudioDriver.h"
#include "drivers/PortAudioDriver.h"
#include "drivers/DiskWriterDriver.h"
#include "drivers/GenericDriver.h"
#include "drivers/MidiDriver.h"
#include "drivers/AlsaMidiDriver.h"
#include "drivers/PortMidiDriver.h"
#include "drivers/TransportInfo.h"
#include "Preferences.h"

#include "DataPath.h"

// workaround for gcc 2.96
#if __GNUC__ < 3
inline int round(double x) { return x > 0 ? (int) (x+0.5) : -(int)(-x+0.5); }
#endif


inline static float linear_interpolation( float fVal_A, float fVal_B, float fVal)
{
	return fVal_A * ( 1 - fVal ) + fVal_B * fVal;
//	return fVal_A + fVal * ( fVal_B - fVal_A );
//	return fVal_A + ((fVal_B - fVal_A) * fVal);
}


// GLOBALS

// info
unsigned m_nPlayingNotes = 0;		/// number of playing notes
float m_fMasterPeak_L = 0.0f;			/// Master peak (left channel)
float m_fMasterPeak_R = 0.0f;			/// Master peak (right channel)
float m_fProcessTime = 0.0f;			/// time used in process function
float m_fMaxProcessTime = 0.0f;			/// max ms usable in process with no xrun
//~ info

pthread_mutex_t m_engineLock_mutex;	/// Mutex for syncronized access to the song object
string m_sLocker = "";

GenericDriver *m_pAudioDriver = NULL;		/// Audio Driver

MidiDriver *m_pMidiDriver = NULL;	/// MIDI driver

unsigned m_nMaxNotes;

std::deque<Note*> m_songNoteQueue;		/// Song Note FIFO
std::deque<Note*> m_midiNoteQueue;		/// Midi Note FIFO
std::vector<Note*> m_playingNotesQueue;	/// Playing note FIFO


Song *m_pSong;			/// Current song
Pattern* m_pNextPattern;	/// Next pattern (used only in Pattern mode)


PatternList* m_pPlayingPatterns;
int m_nSongPos;		// E' la posizione all'interno della canzone


int m_nSelectedPatternNumber;
int m_nSelectedInstrumentNumber;

/** Metronome Instrument */
Instrument *m_pMetronomeInstrument = NULL;
Instrument *m_pPreviewInstrument = NULL;


// Buffers used in the process function
unsigned m_nBufferSize = 0;
float *m_pMainBuffer_L = NULL;
float *m_pMainBuffer_R = NULL;
float *m_pTrackBuffers_L[MAX_INSTRUMENTS];
float *m_pTrackBuffers_R[MAX_INSTRUMENTS];
bool m_bUseTrackOuts = false;
bool m_bUseDefaultOuts = false;


Hydrogen* Hydrogen::instance = NULL;		/// static reference of Hydrogen class (Singleton)
Hydrogen* hydrogenInstance = NULL;		/// Hydrogen class instance (used for log)


int  m_audioEngineState = STATE_UNINITIALIZED;	/// Audio engine state



#ifdef LADSPA_SUPPORT
// LADSPA
unsigned m_nFXBufferSize = 0;
float* m_pFXBuffer_L[MAX_FX];
float* m_pFXBuffer_R[MAX_FX];
float m_fFXPeak_L[MAX_FX];
float m_fFXPeak_R[MAX_FX];
#endif


int m_nPatternStartTick = -1;
int m_nPatternTickPosition = 0;

// used in findPatternInTick
int m_nSongSizeInTicks = 0;

struct timeval m_currentTickTime;

unsigned long m_nRealtimeFrames = 0;





// PROTOTYPES
void	audioEngine_init();
void	audioEngine_destroy();
int	audioEngine_start(bool bLockEngine = false, unsigned nTotalFrames = 0);
void	audioEngine_stop(bool bLockEngine = false);
void	audioEngine_setSong(Song *newSong);
void	audioEngine_removeSong();
void	audioEngine_noteOn(Note *note);
void	audioEngine_noteOff(Note *note);
int	audioEngine_process(uint32_t nframes, void *arg);
inline void audioEngine_clearNoteQueue();
inline void audioEngine_process_checkBPMChanged();
inline void audioEngine_process_playNotes(unsigned long nframes);
inline void audioEngine_process_transport();

inline unsigned audioEngine_renderNote(Note* pNote, const unsigned& nBufferSize);
inline int audioEngine_updateNoteQueue(unsigned nFrames);

inline int findPatternInTick( int tick, bool loopMode, int *patternStartTick );

void audioEngine_seek( long long nFrames, bool bLoopMode = false);

void audioEngine_restartAudioDrivers();
void audioEngine_startAudioDrivers();
void audioEngine_stopAudioDrivers();


inline unsigned long currentTime() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
}



inline timeval currentTime2() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return now;
}



inline int randomValue( int max ) {
	return rand() % max;
}


inline float getGaussian( float z ) {
	// gaussian distribution -- dimss
	float x1, x2, w;
	do {
		x1 = 2.0 * (((float) rand()) / RAND_MAX) - 1.0;
		x2 = 2.0 * (((float) rand()) / RAND_MAX) - 1.0;
		w = x1 * x1 + x2 * x2;
	} while(w >= 1.0);

	w = sqrtf( (-2.0 * logf( w ) ) / w );
	return x1 * w * z + 0.5; // tunable
}


inline void audioEngine_lock(const std::string& sLocker) {
	pthread_mutex_lock(&m_engineLock_mutex);
	m_sLocker = sLocker;
}


inline void audioEngine_unlock() {
	pthread_mutex_unlock(&m_engineLock_mutex);
}



void audioEngine_raiseError( unsigned nErrorCode ) {
	EventQueue::getInstance()->pushEvent( EVENT_ERROR, nErrorCode );
}



void updateTickSize() {
//	hydrogenInstance->infoLog("UpdateTickSize");
	float sampleRate = (float)m_pAudioDriver->getSampleRate();
	m_pAudioDriver->m_transport.m_nTickSize = ( sampleRate * 60.0 /  m_pSong->m_fBPM / m_pSong->m_nResolution );
}



void audioEngine_init()
{
	hydrogenInstance->infoLog("*** Hydrogen audio engine init ***");

	// check current state
	if (m_audioEngineState != STATE_UNINITIALIZED) {
		hydrogenInstance->errorLog("[audioEngine_init]: Error the audio engine is not in UNINITIALIZED state");
		audioEngine_unlock();
		return;
	}

	m_pSong = NULL;
	m_pPlayingPatterns = new PatternList();
	m_pNextPattern = NULL;
	m_nSongPos = -1;
	m_nSelectedPatternNumber = 0;
	m_nSelectedInstrumentNumber = 0;
	m_nPatternTickPosition = 0;
	m_pMetronomeInstrument = NULL;
	m_pAudioDriver = NULL;

	m_pMainBuffer_L = NULL;
	m_pMainBuffer_R = NULL;
#ifdef LADSPA_SUPPORT
	for (unsigned i = 0; i < MAX_FX; ++i) {
		m_pFXBuffer_L[i] = NULL;
		m_pFXBuffer_R[i] = NULL;
	}
#endif
	for (unsigned i=0; i < MAX_INSTRUMENTS; ++i) {
		m_pTrackBuffers_L[i] = NULL;
		m_pTrackBuffers_R[i] = NULL;
	}

	Preferences *preferences = Preferences::getInstance();
	m_nMaxNotes = preferences->m_nMaxNotes;
	m_bUseTrackOuts = preferences->m_bJackTrackOuts;
	m_bUseDefaultOuts = preferences->m_bJackConnectDefaults;

	pthread_mutex_init(&m_engineLock_mutex, NULL);

	srand(time(NULL));

	// Create metronome instrument
	string sMetronomeFilename = string(DataPath::getDataPath()) + "/click.wav";
	m_pMetronomeInstrument = new Instrument( sMetronomeFilename, "metronome", 0.0 );
	m_pMetronomeInstrument->m_pADSR = new ADSR();
	m_pMetronomeInstrument->setLayer( new InstrumentLayer( Sample::load( sMetronomeFilename ) ), 0 );

	// instrument used in file preview
	string sEmptySampleFilename = string(DataPath::getDataPath()) + "/emptySample.wav";
	m_pPreviewInstrument = new Instrument( sEmptySampleFilename, "preview", 0.8 );
	m_pPreviewInstrument->m_pADSR = new ADSR();
	m_pPreviewInstrument->setLayer( new InstrumentLayer( Sample::load( sEmptySampleFilename ) ), 0 );

	// Change the current audio engine state
	m_audioEngineState = STATE_INITIALIZED;

	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_INITIALIZED );
}



void audioEngine_destroy()
{
	audioEngine_lock("audioEngine_destroy");
	hydrogenInstance->infoLog("*** Hydrogen audio engine shutdown ***");

	// check current state
	if (m_audioEngineState != STATE_INITIALIZED) {
		hydrogenInstance->errorLog("[audioEngine_destroy] Error the audio engine is not in INITIALIZED state");
		return;
	}

	// change the current audio engine state
	m_audioEngineState = STATE_UNINITIALIZED;

	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_UNINITIALIZED );

	delete m_pPlayingPatterns;
	m_pPlayingPatterns = NULL;

	// delete all copied notes in the song notes queue
	for (unsigned i = 0; i < m_songNoteQueue.size(); ++i) {
		Note *note = m_songNoteQueue[i];
		delete note;
		note = NULL;
	}
	m_songNoteQueue.clear();

	// delete all copied notes in the playing notes queue
	for (unsigned i = 0; i < m_playingNotesQueue.size(); ++i) {
		Note *note = m_playingNotesQueue[i];
		delete note;
		note = NULL;
	}
	m_playingNotesQueue.clear();

	// delete all copied notes in the midi notes queue
	for (unsigned i = 0; i < m_midiNoteQueue.size(); ++i) {
		Note *note = m_midiNoteQueue[i];
		delete note;
		note = NULL;
	}
	m_midiNoteQueue.clear();

	delete m_pMetronomeInstrument;
	m_pMetronomeInstrument = NULL;

	delete m_pPreviewInstrument;
	m_pPreviewInstrument = NULL;

#ifdef LADSPA_SUPPORT
	for (unsigned i = 0; i < MAX_FX; ++i) {
		delete[] m_pFXBuffer_L[i];
		m_pFXBuffer_L[i] = NULL;

		delete[] m_pFXBuffer_R[i];
		m_pFXBuffer_R[i] = NULL;
	}
#endif

	audioEngine_unlock();
}





/// Start playing
/// return 0 = OK
/// return -1 = NULL Audio Driver
/// return -2 = Driver connect() error
int audioEngine_start(bool bLockEngine, unsigned nTotalFrames) {
	if (bLockEngine) {
		audioEngine_lock("audioEngine_start");
	}

	hydrogenInstance->infoLog("[audioEngine_start]");

	// check current state
	if (m_audioEngineState != STATE_READY) {
		hydrogenInstance->errorLog("[audioEngine_start]: Error the audio engine is not in READY state");
		if (bLockEngine) {
			audioEngine_unlock();
		}
		return 0;	// FIXME!!
	}


	Preferences *preferencesMng = Preferences::getInstance();
	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;
	m_pAudioDriver->m_transport.m_nFrames = nTotalFrames;	// reset total frames
	m_nSongPos = -1;
	m_nPatternStartTick = -1;
	m_nPatternTickPosition = 0;

	// prepare the tickSize for this song
	updateTickSize();

	m_nMaxNotes = preferencesMng->m_nMaxNotes;

	// change the current audio engine state
	m_audioEngineState = STATE_PLAYING;
	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_PLAYING );

	if (bLockEngine) {
		audioEngine_unlock();
	}
	return 0; // per ora restituisco sempre OK
}



/// Stop the audio engine
void audioEngine_stop(bool bLockEngine) {
	if (bLockEngine) {
		audioEngine_lock( "audioEngine_stop" );
	}
	hydrogenInstance->infoLog( "[audioEngine_stop]" );

	// check current state
	if (m_audioEngineState != STATE_PLAYING) {
		hydrogenInstance->errorLog("[audioEngine_stop]: Error the audio engine is not in PLAYING state");
		if (bLockEngine) {
			audioEngine_unlock();
		}
		return;
	}

	// change the current audio engine state
	m_audioEngineState = STATE_READY;
	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_READY );

	m_fMasterPeak_L = 0.0f;
	m_fMasterPeak_R = 0.0f;
//	m_nPatternTickPosition = 0;
	m_nPatternStartTick = -1;

	// delete all copied notes in the song notes queue
	for (unsigned i = 0; i < m_songNoteQueue.size(); ++i) {
		Note *note = m_songNoteQueue[i];
		delete note;
	}
	m_songNoteQueue.clear();

/*	// delete all copied notes in the playing notes queue
	for (unsigned i = 0; i < m_playingNotesQueue.size(); ++i) {
		Note *note = m_playingNotesQueue[i];
		delete note;
	}
	m_playingNotesQueue.clear();
*/
	// send a note-off event to all notes present in the playing note queue
	for ( int i = 0; i < m_playingNotesQueue.size(); ++i ) {
		Note *pNote = m_playingNotesQueue[ i ];
		pNote->m_pADSR->release();
	}

	// delete all copied notes in the midi notes queue
	for (unsigned i = 0; i < m_midiNoteQueue.size(); ++i) {
		Note *note = m_midiNoteQueue[i];
		delete note;
	}
	m_midiNoteQueue.clear();


	if (bLockEngine) {
		audioEngine_unlock();
	}
}



inline void audioEngine_process_checkBPMChanged()
{

	if ( ( m_audioEngineState == STATE_READY) || (m_audioEngineState == STATE_PLAYING ) ) {

		float fNewTickSize = ( m_pAudioDriver->getSampleRate() * 60.0 /  m_pSong->m_fBPM / m_pSong->m_nResolution );

		if ( fNewTickSize != m_pAudioDriver->m_transport.m_nTickSize) {
			m_pAudioDriver->m_transport.m_nTickSize = fNewTickSize;

			if (m_pAudioDriver->m_transport.m_nTickSize == 0 ) {
				return;
			}

			// cerco di convertire ...
			float fTickNumber = (float)m_pAudioDriver->m_transport.m_nFrames / (float)m_pAudioDriver->m_transport.m_nTickSize;
//			hydrogenInstance->infoLog( "[audioEngine_process_checkBPMChanged] Old tick number: " + toString( fTickNumber ) );

			float fNewFrames = fTickNumber * fNewTickSize;
			assert( fNewFrames >= 0 );
			if ( fNewFrames != m_pAudioDriver->m_transport.m_nFrames ) {
//				hydrogenInstance->infoLog( "[audioEngine_process_checkBPMChanged] fNewFrames " + toString( fNewFrames ) + ", m_transport.m_nFrames: " + toString( m_pAudioDriver->m_transport.m_nFrames ) );
			}

			// update tickSize
			m_pAudioDriver->m_transport.m_nFrames = fNewFrames;

			fTickNumber = (float)m_pAudioDriver->m_transport.m_nFrames / (float)m_pAudioDriver->m_transport.m_nTickSize;
//			hydrogenInstance->infoLog("[audioEngine_process_checkBPMChanged] New tick number: " + toString( fTickNumber ) );

/*

			// delete all copied notes in the song notes queue
			for (unsigned i = 0; i < m_songNoteQueue.size(); ++i) {
				delete m_songNoteQueue[i];
			}
			m_songNoteQueue.clear();

			// send a note-off event to all notes present in the playing note queue
			for ( int i = 0; i < m_playingNotesQueue.size(); ++i ) {
				Note *pNote = m_playingNotesQueue[ i ];
				pNote->m_pADSR->release();
			}

			// delete all copied notes in the midi notes queue
			for (unsigned i = 0; i < m_midiNoteQueue.size(); ++i) {
				delete m_midiNoteQueue[i];
			}
			m_midiNoteQueue.clear();
*/
		}
	}
}



inline void audioEngine_process_playNotes( unsigned long nframes )
{
	unsigned int framepos;

	if (  m_audioEngineState == STATE_PLAYING ) {
		framepos = m_pAudioDriver->m_transport.m_nFrames;
	}
	else {
		// use this to support realtime events when not playing
		framepos = m_nRealtimeFrames;
	}

	// leggo da m_songNoteQueue
	while (m_songNoteQueue.size() > 0) {
		Note *pNote = m_songNoteQueue[0];

		// verifico se la nota rientra in questo ciclo
		unsigned noteStartInFrames = (unsigned)( pNote->m_nPosition * m_pAudioDriver->m_transport.m_nTickSize );

		// m_nTotalFrames <= NotePos < m_nTotalFrames + bufferSize
		bool isNoteStart = ( ( noteStartInFrames >= framepos ) && ( noteStartInFrames < ( framepos + nframes ) ) );
		bool isOldNote = noteStartInFrames < framepos;
		if ( isNoteStart || isOldNote ) {

			// Humanize - Velocity parameter
			if (m_pSong->getHumanizeVelocityValue() != 0) {
				float random = m_pSong->getHumanizeVelocityValue() * getGaussian( 0.2 );
				pNote->m_fVelocity += (random - (m_pSong->getHumanizeVelocityValue() / 2.0));
				if ( pNote->m_fVelocity > 1.0 ) {
					pNote->m_fVelocity = 1.0;
				}
				else if ( pNote->m_fVelocity < 0.0 ) {
					pNote->m_fVelocity = 0.0;
				}
			}

			// Random Pitch ;)
			const float fMaxPitchDeviation = 2.0;
			pNote->m_fPitch += ( fMaxPitchDeviation * getGaussian( 0.2 ) - fMaxPitchDeviation / 2.0 ) * pNote->getInstrument()->m_fRandomPitchFactor;



			m_playingNotesQueue.push_back( pNote );		// aggiungo la nota alla lista di note da eseguire
			m_songNoteQueue.pop_front();			// rimuovo la nota dalla lista di note


			// exclude notes
			Instrument *pInstr = pNote->getInstrument();
			if ( pInstr == NULL ) {
				hydrogenInstance->errorLog( "[audioEngine_process_playNotes] instr == NULL" );
			}
			if ( pInstr->m_excludeVect.size() != 0 ) {
				for (unsigned i = 0; i < pInstr->m_excludeVect.size(); ++i) {
					Instrument *pExcluded = pInstr->m_excludeVect[ i ];
					// remove all notes listed in excludeVect
					for ( unsigned j = 0; j < m_playingNotesQueue.size(); j++ ) {	// delete older note
						Note *pOldNote = m_playingNotesQueue[ j ];

						if ( pOldNote->getInstrument() == pExcluded ) {
							//hydrogenInstance->warningLog("release");
							pOldNote->m_pADSR->release();
							/*
							hydrogenInstance->warningLog("Delete excluded note");
							m_playingNotesQueue.erase( m_playingNotesQueue.begin() + j );
							delete pOldNote;
							*/
						}
					}
				}
			}

			// raise noteOn event
			int nInstrument = m_pSong->getInstrumentList()->getPos( pNote->getInstrument() );
			EventQueue::getInstance()->pushEvent( EVENT_NOTEON, nInstrument );
			continue;
		}
		else {
			// la nota non andra' in esecuzione
			break;
		}
	}

}



void audioEngine_seek( long long nFrames, bool bLoopMode)
{
	if (m_pAudioDriver->m_transport.m_nFrames == nFrames) {
		return;
	}

	if ( nFrames < 0 ) {
		hydrogenInstance->errorLog( "[audioEngine_seek] nFrames < 0" );
	}

	char tmp[200];
	sprintf(tmp, "[audioEngine_seek()] seek in %d (old pos = %d)", nFrames, (int)m_pAudioDriver->m_transport.m_nFrames);
	hydrogenInstance->infoLog(tmp);

	m_pAudioDriver->m_transport.m_nFrames = nFrames;

	int tickNumber_start = (unsigned)( m_pAudioDriver->m_transport.m_nFrames / m_pAudioDriver->m_transport.m_nTickSize );
//	sprintf(tmp, "[audioEngine_seek()] tickNumber_start = %d", tickNumber_start);
//	hydrogenInstance->infoLog(tmp);

	bool loop = m_pSong->isLoopEnabled();

	if (bLoopMode) {
		loop = true;
	}

	m_nSongPos = findPatternInTick( tickNumber_start, loop, &m_nPatternStartTick );
//	sprintf(tmp, "[audioEngine_seek()] m_nSongPos = %d", m_nSongPos);
//	hydrogenInstance->infoLog(tmp);

	audioEngine_clearNoteQueue();
}



inline void audioEngine_process_transport()
{
	if ( (m_audioEngineState == STATE_READY ) || (m_audioEngineState == STATE_PLAYING) ) {
		m_pAudioDriver->updateTransportInfo();
		unsigned long nNewFrames = m_pAudioDriver->m_transport.m_nFrames;

		audioEngine_seek( nNewFrames, true );

		switch ( m_pAudioDriver->m_transport.m_status ) {
			case TransportInfo::ROLLING:

//				hydrogenInstance->infoLog( "[audioEngine_process_transport] ROLLING - frames: " + toString(m_pAudioDriver->m_transport.m_nFrames) );
				if ( m_audioEngineState == STATE_READY ) {
					//hydrogenInstance->infoLog( "[audioEngine_process_transport] first start frames is " + toString(m_pAudioDriver->m_transport.m_nFrames) );
					audioEngine_start( false, nNewFrames );	// no engine lock
				}

				if (m_pSong->m_fBPM != m_pAudioDriver->m_transport.m_nBPM ) {
					hydrogenInstance->infoLog( "[audioEngine_process_transport] song bpm: " + toString( m_pSong->m_fBPM ) + ", transport bpm: " + toString( m_pAudioDriver->m_transport.m_nBPM ) );
					m_pSong->m_fBPM = m_pAudioDriver->m_transport.m_nBPM;
				}

				m_nRealtimeFrames = m_pAudioDriver->m_transport.m_nFrames;
				break;


			case TransportInfo::STOPPED:

//				hydrogenInstance->infoLog( "[audioEngine_process_transport] STOPPED - frames: " + toString(m_pAudioDriver->m_transport.m_nFrames) );
				if ( m_audioEngineState == STATE_PLAYING ) {
					audioEngine_stop(false);	// no engine lock
				}

				if (m_pSong->m_fBPM != m_pAudioDriver->m_transport.m_nBPM ) {
					m_pSong->m_fBPM = m_pAudioDriver->m_transport.m_nBPM;
				}

				// go ahead and increment the realtimeframes by buffersize
				// to support our realtime keyboard and midi event timing
				m_nRealtimeFrames += m_nBufferSize;
				break;
		}
	}
}



void audioEngine_clearNoteQueue() {
	hydrogenInstance->infoLog("[audioEngine_clearNoteQueue()]");

	for (unsigned i = 0; i < m_songNoteQueue.size(); ++i) {	// delete all copied notes in the song notes queue
		delete m_songNoteQueue[i];
	}
	m_songNoteQueue.clear();

	for (unsigned i = 0; i < m_playingNotesQueue.size(); ++i) {	// delete all copied notes in the playing notes queue
		delete m_playingNotesQueue[i];
	}
	m_playingNotesQueue.clear();

	for (unsigned i = 0; i < m_midiNoteQueue.size(); ++i) {	// delete all copied notes in the midi notes queue
		delete m_midiNoteQueue[i];
	}
	m_midiNoteQueue.clear();

}



/// Clear all audio buffers
inline void audioEngine_process_clearAudioBuffers(uint32_t nFrames)
{
	if ( m_pMainBuffer_L ) {
		memset( m_pMainBuffer_L, 0, nFrames * sizeof( float ) );	// clear main out Left
	}
	if (m_pMainBuffer_R ) {
		memset( m_pMainBuffer_R, 0, nFrames * sizeof( float ) );	// clear main out Right
	}

	if ( ( m_audioEngineState == STATE_READY ) || ( m_audioEngineState == STATE_PLAYING) ) {
#ifdef LADSPA_SUPPORT
		for (unsigned i = 0; i < MAX_FX; ++i) {	// clear FX buffers
			memset( m_pFXBuffer_L[i], 0, nFrames * sizeof( float ) );
			memset( m_pFXBuffer_R[i], 0, nFrames * sizeof( float ) );
		}
#endif
	}
	if (m_bUseTrackOuts) {
	 	for (unsigned n=0; n < MAX_INSTRUMENTS; ++n) {	// clear track buffers
 			memset( m_pTrackBuffers_L[n], 0, nFrames * sizeof( float ) );
 			memset( m_pTrackBuffers_R[n], 0, nFrames * sizeof( float ) );
	 	}
	}
}



/// Main audio processing function. Called by audio drivers.
int audioEngine_process(uint32_t nframes, void *arg)
{
	// try to lock the engine mutex. If fails returns without sound processing
	int res = pthread_mutex_trylock(&m_engineLock_mutex);
	if (res != 0) {
		hydrogenInstance->warningLog( "[audioEngine_process] trylock != 0. Frames = " + toString(m_pAudioDriver->m_transport.m_nFrames ) + ". Lock in " + m_sLocker );
//		hydrogenInstance->warningLog( "[audioEngine_process] trylock != 0" );
		return 0;
	}
	m_sLocker = "audioEngine_process";

	timeval startTimeval = currentTime2();

	if ( m_nBufferSize != nframes ) {
		hydrogenInstance->infoLog( "[audioEngine_process] Buffer size changed. Old size = " + toString(m_nBufferSize) +", new size = " + toString(nframes) );
#ifdef LADSPA_SUPPORT
		if ( m_nFXBufferSize != nframes ) {
			hydrogenInstance->errorLog( "[audioEngine_process] Ladspa FX buffersize != nframes. FXBuffersize = " + toString( m_nFXBufferSize ) );
		}
#endif
		m_nBufferSize = nframes;
	}

	audioEngine_process_transport();
	audioEngine_process_clearAudioBuffers(nframes);
	audioEngine_process_checkBPMChanged();

	bool sendPatternChange = false;
	// always update note queue.. could come from pattern or realtime input (midi, keyboard)
	int res2 = audioEngine_updateNoteQueue( nframes );	// update the notes queue
	if ( res2 == -1 ) {	// end of song
		hydrogenInstance->infoLog( "[audioEngine_process] End of song received, calling engine_stop()" );
		audioEngine_unlock();
		m_pAudioDriver->stop();
		m_pAudioDriver->locate( 0 );	// locate 0, reposition from start of the song

		if ( ( m_pAudioDriver->getClassName() == "DiskWriterDriver" ) || ( m_pAudioDriver->getClassName() == "FakeDriver" ) ) {
			hydrogenInstance->infoLog( "[audioEngine_process] End of song." );
			return 1;	// kill the audio AudioDriver thread
		}

		return 0;
	}
	else if ( res2 == 2 ) {	// send pattern change
		sendPatternChange = true;
	}


	// play all notes
	audioEngine_process_playNotes( nframes );

	// Max notes
	while ( m_playingNotesQueue.size() > m_nMaxNotes ) {
		Note *oldNote = m_playingNotesQueue[ 0 ];
		m_playingNotesQueue.erase( m_playingNotesQueue.begin() );
		delete oldNote;	// FIXME: send note-off instead of removing the note from the list?
	}
	m_nPlayingNotes = m_playingNotesQueue.size();	// update the number of playing notes


	timeval renderTime_start = currentTime2();
	// eseguo tutte le note nella lista di note in esecuzione
	unsigned i = 0;
	Note* pNote;
	while ( i < m_playingNotesQueue.size() ) {
		pNote = m_playingNotesQueue[ i ];		// recupero una nuova nota
		unsigned res = audioEngine_renderNote( pNote, nframes );
		if ( res == 1 ) {	// la nota e' finita
			m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			delete pNote;
			pNote = NULL;
		}
		else {
			++i; // carico la prox nota
		}
	}
	timeval renderTime_end = currentTime2();

	timeval ladspaTime_start = renderTime_end;
#ifdef LADSPA_SUPPORT
	// Process LADSPA FX
	if ( ( m_audioEngineState == STATE_READY ) || ( m_audioEngineState == STATE_PLAYING ) ) {
		for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
			LadspaFX *pFX = m_pSong->getLadspaFX( nFX );
			if ( ( pFX ) && (pFX->isEnabled()) ) {
				pFX->processFX( nframes );
				float *buf_L = NULL;
				float *buf_R = NULL;
				if (pFX->getPluginType() == LadspaFX::STEREO_FX) {	// STEREO FX
					buf_L = m_pFXBuffer_L[nFX];
					buf_R = m_pFXBuffer_R[nFX];
				}
				else { // MONO FX
					buf_L = m_pFXBuffer_L[nFX];
					buf_R = buf_L;
				}
				for ( unsigned i = 0; i < nframes; ++i) {
					m_pMainBuffer_L[ i ] += buf_L[ i ];
					m_pMainBuffer_R[ i ] += buf_R[ i ];
					if ( buf_L[ i ] > m_fFXPeak_L[nFX] )	m_fFXPeak_L[nFX] = buf_L[ i ];
					if ( buf_R[ i ] > m_fFXPeak_R[nFX] )	m_fFXPeak_R[nFX] = buf_R[ i ];
				}
			}
		}
	}
#endif
	timeval ladspaTime_end = currentTime2();

	// update master peaks
	float val_L;
	float val_R;
	if ( m_audioEngineState == STATE_PLAYING || m_audioEngineState == STATE_READY ) {
		for ( unsigned i = 0; i < nframes; ++i) {
			val_L = m_pMainBuffer_L[i];
			val_R = m_pMainBuffer_R[i];
			if (val_L > m_fMasterPeak_L) {
				m_fMasterPeak_L = val_L;
			}
			if (val_R > m_fMasterPeak_R) {
				m_fMasterPeak_R = val_R;
			}
		}
	}

	// update total frames number
	if ( m_audioEngineState == STATE_PLAYING ) {
		m_pAudioDriver->m_transport.m_nFrames += nframes;
	}

	float fRenderTime = (renderTime_end.tv_sec - renderTime_start.tv_sec) * 1000.0 + (renderTime_end.tv_usec - renderTime_start.tv_usec) / 1000.0;
	float fLadspaTime = (ladspaTime_end.tv_sec - ladspaTime_start.tv_sec) * 1000.0 + (ladspaTime_end.tv_usec - ladspaTime_start.tv_usec) / 1000.0;


	timeval finishTimeval = currentTime2();
	m_fProcessTime = (finishTimeval.tv_sec - startTimeval.tv_sec) * 1000.0 + (finishTimeval.tv_usec - startTimeval.tv_usec) / 1000.0;

	float sampleRate = (float)m_pAudioDriver->getSampleRate();
	m_fMaxProcessTime = 1000.0 / (sampleRate / nframes);


	//DEBUG
	if ( m_fProcessTime > m_fMaxProcessTime ) {
		hydrogenInstance->warningLog( "" );
		hydrogenInstance->warningLog( "----XRUN----" );
		hydrogenInstance->warningLog( "XRUN of " + toString((m_fProcessTime - m_fMaxProcessTime)) + string(" msec (") + toString(m_fProcessTime) + string(" > ") + toString(m_fMaxProcessTime) + string(")") );
		hydrogenInstance->warningLog( "Playing notes = " + toString( m_nPlayingNotes ) + string( ", render time = ") + toString( fRenderTime ) );
		hydrogenInstance->warningLog( "Ladspa process time = " + toString( fLadspaTime ) );
		hydrogenInstance->warningLog( "------------" );
		hydrogenInstance->warningLog( "" );
		// raise xRun event
		EventQueue::getInstance()->pushEvent( EVENT_XRUN, -1 );
	}

	audioEngine_unlock();

	if ( sendPatternChange ) {
		EventQueue::getInstance()->pushEvent( EVENT_PATTERN_CHANGED, -1 );
	}

	return 0;
}




inline int audioEngine_renderNote_noResample(
		Sample *pSample,
		Note *pNote,
		unsigned nBufferSize,
		unsigned nInitialSilence,
		float cost_L,
		float cost_R,
		float cost_track,
		float fSendFXLevel_L,
		float fSendFXLevel_R
)
{
	int retValue = 1; // the note is ended

	int nNoteLength = -1;
	if ( pNote->m_nLength != -1) {
		nNoteLength = (int)( pNote->m_nLength * m_pAudioDriver->m_transport.m_nTickSize );
	}

	int nAvail_bytes = pSample->m_nFrames - (int)pNote->m_fSamplePosition;	// verifico il numero di frame disponibili ancora da eseguire

	if (nAvail_bytes > nBufferSize - nInitialSilence) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = 0; // the note is not ended yet
	}

	ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	int nInitialSamplePos = (int)pNote->m_fSamplePosition;
	int nSamplePos = nInitialSamplePos;
	int nTimes = nInitialBufferPos + nAvail_bytes;
	int nInstrument = m_pSong->getInstrumentList()->getPos( pNote->getInstrument() );

	// filter
	bool bUseLPF = pNote->getInstrument()->m_bFilterActive;
	float fResonance = pNote->getInstrument()->m_fResonance;
	float fCutoff = pNote->getInstrument()->m_fCutoff;

	float *pSample_data_L = pSample->m_pData_L;
	float *pSample_data_R = pSample->m_pData_R;

	float fInstrPeak_L = pNote->getInstrument()->m_fPeak_L; // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->getInstrument()->m_fPeak_R; // this value will be reset to 0 by the mixer..

	float fADSRValue;
	float fVal_L;
	float fVal_R;
	for (int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition)  ) {
			if ( pADSR->release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		fADSRValue = pADSR->getValue( 1 );
		fVal_L = pSample_data_L[ nSamplePos ] * cost_L * fADSRValue;
		fVal_R = pSample_data_R[ nSamplePos ] * cost_R * fADSRValue;
 		if (m_bUseTrackOuts) {	// for the track output
			if ( nInstrument != -1 ) {	// -1 is used for audio preview
	 			m_pTrackBuffers_L[ nInstrument ][ nBufferPos ] = pSample_data_L[ nSamplePos ] * cost_track * fADSRValue;
 				m_pTrackBuffers_R[ nInstrument ][ nBufferPos ] = pSample_data_R[ nSamplePos ] * cost_track * fADSRValue;
			}
 		}

 		// Low pass resonant filter
 		if ( bUseLPF ) {
 			pNote->m_fBandPassFilterBuffer_L = fResonance * pNote->m_fBandPassFilterBuffer_L + fCutoff * (fVal_L - pNote->m_fLowPassFilterBuffer_L);
 			pNote->m_fLowPassFilterBuffer_L += fCutoff * pNote->m_fBandPassFilterBuffer_L;
 			fVal_L = pNote->m_fLowPassFilterBuffer_L;

 			pNote->m_fBandPassFilterBuffer_R = fResonance * pNote->m_fBandPassFilterBuffer_R + fCutoff * (fVal_R - pNote->m_fLowPassFilterBuffer_R);
 			pNote->m_fLowPassFilterBuffer_R += fCutoff * pNote->m_fBandPassFilterBuffer_R;
 			fVal_R = pNote->m_fLowPassFilterBuffer_R;
 		}

		// update instr peak
		if (fVal_L > fInstrPeak_L) {	fInstrPeak_L = fVal_L;	}
		if (fVal_R > fInstrPeak_R) {	fInstrPeak_R = fVal_R;	}

		// to main mix
		m_pMainBuffer_L[nBufferPos] += fVal_L;
		m_pMainBuffer_R[nBufferPos] += fVal_R;


		++nSamplePos;
	}
	pNote->m_fSamplePosition += nAvail_bytes;
	pNote->getInstrument()->m_fPeak_L = fInstrPeak_L;
	pNote->getInstrument()->m_fPeak_R = fInstrPeak_R;


#ifdef LADSPA_SUPPORT
	// LADSPA
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = m_pSong->getLadspaFX( nFX );
		float fLevel = pNote->getInstrument()->getFXLevel(nFX);

		if ( ( pFX ) && ( fLevel != 0.0 ) ) {
			fLevel = fLevel * pFX->getVolume();
			float *pBuf_L = m_pFXBuffer_L[nFX];
			float *pBuf_R = m_pFXBuffer_R[nFX];

//			float fFXCost_L = cost_L * fLevel;
//			float fFXCost_R = cost_R * fLevel;
			float fFXCost_L = fLevel * fSendFXLevel_L;
			float fFXCost_R = fLevel * fSendFXLevel_R;

			int nBufferPos = nInitialBufferPos;
			int nSamplePos = nInitialSamplePos;
			for (int i = 0; i < nAvail_bytes; ++i) {
				pBuf_L[ nBufferPos ] += pSample_data_L[ nSamplePos ] * fFXCost_L;
				pBuf_R[ nBufferPos ] += pSample_data_R[ nSamplePos ] * fFXCost_R;
				++nSamplePos;
				++nBufferPos;
			}
		}
	}
	// ~LADSPA
#endif

	return retValue;
}



inline int audioEngine_renderNote_resample(
		Sample *pSample,
		Note *pNote,
		unsigned nBufferSize,
		unsigned nInitialSilence,
		float cost_L,
		float cost_R,
		float cost_track,
		float fLayerPitch,
		float fSendFXLevel_L,
		float fSendFXLevel_R
)
{
	int nNoteLength = -1;
	if ( pNote->m_nLength != -1) {
		nNoteLength = (int)( pNote->m_nLength * m_pAudioDriver->m_transport.m_nTickSize );
	}
	float fNotePitch = pNote->m_fPitch + fLayerPitch;

	float fStep = pow( 1.0594630943593, (double)fNotePitch );
	int nAvail_bytes = (int)( (float)(pSample->m_nFrames - pNote->m_fSamplePosition) / fStep );	// verifico il numero di frame disponibili ancora da eseguire

	int retValue = 1; // the note is ended
	if (nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = 0; // the note is not ended yet
	}

	ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	float fInitialSamplePos = pNote->m_fSamplePosition;
	float fSamplePos = pNote->m_fSamplePosition;
	int nTimes = nInitialBufferPos + nAvail_bytes;
	int nInstrument = m_pSong->getInstrumentList()->getPos( pNote->getInstrument() );

	// filter
	bool bUseLPF = pNote->getInstrument()->m_bFilterActive;
	float fResonance = pNote->getInstrument()->m_fResonance;
	float fCutoff = pNote->getInstrument()->m_fCutoff;

	float *pSample_data_L = pSample->m_pData_L;
	float *pSample_data_R = pSample->m_pData_R;

	float fInstrPeak_L = pNote->getInstrument()->m_fPeak_L; // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->getInstrument()->m_fPeak_R; // this value will be reset to 0 by the mixer..

	float fADSRValue;
	float fVal_L;
	float fVal_R;
	unsigned nSampleFrames = pSample->m_nFrames;

	for (int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition)  ) {
			if ( pADSR->release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		int nSamplePos = (int)fSamplePos;
		float fDiff = fSamplePos - nSamplePos;
		if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
			fVal_L = linear_interpolation( pSample_data_L[ nSampleFrames ], 0, fDiff );
			fVal_R = linear_interpolation( pSample_data_R[ nSampleFrames ], 0, fDiff );
		}
		else {
			fVal_L = linear_interpolation( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff );
			fVal_R = linear_interpolation( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff );
		}

		if (m_bUseTrackOuts) {	// for the track output
			if ( nInstrument != -1 ) {	// -1 is used for audio preview
				m_pTrackBuffers_L[ nInstrument ][nBufferPos] = fVal_L * cost_track * fADSRValue;
				m_pTrackBuffers_R[ nInstrument ][nBufferPos] = fVal_R * cost_track * fADSRValue;
			}
		}

		// ADSR envelope
		fADSRValue = pADSR->getValue( fStep );
		fVal_L = fVal_L * cost_L * fADSRValue;
		fVal_R = fVal_R * cost_R * fADSRValue;

		// Low pass resonant filter
		if ( bUseLPF ) {
			pNote->m_fBandPassFilterBuffer_L = fResonance * pNote->m_fBandPassFilterBuffer_L + fCutoff * (fVal_L - pNote->m_fLowPassFilterBuffer_L);
			pNote->m_fLowPassFilterBuffer_L += fCutoff * pNote->m_fBandPassFilterBuffer_L;
			fVal_L = pNote->m_fLowPassFilterBuffer_L;

			pNote->m_fBandPassFilterBuffer_R = fResonance * pNote->m_fBandPassFilterBuffer_R + fCutoff * (fVal_R - pNote->m_fLowPassFilterBuffer_R);
			pNote->m_fLowPassFilterBuffer_R += fCutoff * pNote->m_fBandPassFilterBuffer_R;
			fVal_R = pNote->m_fLowPassFilterBuffer_R;
		}

		// update instr peak
		if (fVal_L > fInstrPeak_L) {	fInstrPeak_L = fVal_L;	}
		if (fVal_R > fInstrPeak_R) {	fInstrPeak_R = fVal_R;	}

		// to main mix
		m_pMainBuffer_L[nBufferPos] += fVal_L;
		m_pMainBuffer_R[nBufferPos] += fVal_R;

		fSamplePos += fStep;
	}
	pNote->m_fSamplePosition += nAvail_bytes * fStep;
	pNote->getInstrument()->m_fPeak_L = fInstrPeak_L;
	pNote->getInstrument()->m_fPeak_R = fInstrPeak_R;



#ifdef LADSPA_SUPPORT
	// LADSPA
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		LadspaFX *pFX = m_pSong->getLadspaFX( nFX );
		float fLevel = pNote->getInstrument()->getFXLevel(nFX);
		if ( ( pFX ) && ( fLevel != 0.0 ) ) {
			fLevel = fLevel * pFX->getVolume();

			float *pBuf_L = m_pFXBuffer_L[ nFX ];
			float *pBuf_R = m_pFXBuffer_R[ nFX ];

//			float fFXCost_L = cost_L * fLevel;
//			float fFXCost_R = cost_R * fLevel;
			float fFXCost_L = fLevel * fSendFXLevel_L;
			float fFXCost_R = fLevel * fSendFXLevel_R;

			int nBufferPos = nInitialBufferPos;
			float fSamplePos = fInitialSamplePos;
			for (int i = 0; i < nAvail_bytes; ++i) {
				int nSamplePos = (int)fSamplePos;
				float fDiff = fSamplePos - nSamplePos;

				if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
					fVal_L = linear_interpolation( pSample_data_L[nSamplePos], 0, fDiff );
					fVal_R = linear_interpolation( pSample_data_R[nSamplePos], 0, fDiff );
				}
				else{
					fVal_L = linear_interpolation( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff );
					fVal_R = linear_interpolation( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff );
				}

				pBuf_L[ nBufferPos ] += fVal_L * fFXCost_L;
				pBuf_R[ nBufferPos ] += fVal_R * fFXCost_R;
				fSamplePos += fStep;
				++nBufferPos;
			}
		}
	}
#endif

	return retValue;
}


/// Render a note
/// Return 0: the note is not ended
/// Return 1: the note is ended
inline unsigned audioEngine_renderNote(Note* pNote, const unsigned& nBufferSize)
{
	// nFrames = frames disponibili da copiare nei buffer
	unsigned int nFramepos;
	if (  m_audioEngineState == STATE_PLAYING ) {
		nFramepos = m_pAudioDriver->m_transport.m_nFrames;
	}
	else {
		// use this to support realtime events when not playing
		nFramepos = m_nRealtimeFrames;
	}


	Instrument *pInstr = pNote->getInstrument();
	if ( !pInstr ) {
		hydrogenInstance->errorLog( "[audioEngine_renderNote] NULL instrument" );
		return 1;
	}

	float fLayerGain = 1.0;
	float fLayerPitch = 0.0;

	// scelgo il sample da usare in base alla velocity
	Sample *pSample = NULL;
	for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
		InstrumentLayer *pLayer = pInstr->getLayer( nLayer );
		if ( pLayer == NULL ) continue;

		if ( ( pNote->m_fVelocity >= pLayer->m_fStartVelocity ) && ( pNote->m_fVelocity <= pLayer->m_fEndVelocity ) ) {
			pSample = pLayer->m_pSample;
			fLayerGain = pLayer->m_fGain;
			fLayerPitch = pLayer->m_fPitch;
			break;
		}
	}
	if ( !pSample ) {
		hydrogenInstance->warningLog("[audioEngine_renderNote] NULL sample for instrument " + pInstr->m_sName + ". Note velocity: " + toString( pNote->m_fVelocity ) );
		return 1;
	}

	if ( pNote->m_fSamplePosition >= pSample->m_nFrames ) {
		hydrogenInstance->errorLog( "[audioEngine_renderNote] sample position out of bounds. The layer has been resized during note play?" );
		return 1;
	}

	int noteStartInFrames = (int) ( pNote->m_nPosition * m_pAudioDriver->m_transport.m_nTickSize ) + pNote->m_nHumanizeDelay;

	int nInitialSilence = 0;
	if (noteStartInFrames > (int) nFramepos) {	// scrivo silenzio prima dell'inizio della nota
		nInitialSilence = noteStartInFrames - nFramepos;
		int nFrames = nBufferSize - nInitialSilence;
		if (nFrames < 0) {
			int noteStartInFramesNoHumanize = pNote->m_nPosition * m_pAudioDriver->m_transport.m_nTickSize;
			if ( noteStartInFramesNoHumanize > ( nFramepos + nBufferSize ) ) {
				// this note is not valid. it's in the future...let's skip it....
				return 1;
			}
			// delay note execution
			return 0;
		}
	}

	float cost_L = 1.0f;
	float cost_R = 1.0f;
	float cost_track = 1.0f;
	float fSendFXLevel_L = 1.0f;
	float fSendFXLevel_R = 1.0f;

	if ( pInstr->m_bIsMuted || m_pSong->m_bIsMuted ) {	// is instrument muted?
		cost_L = 0.0;
		cost_R = 0.0;

		fSendFXLevel_L = 0.0f;
		fSendFXLevel_R = 0.0f;
	}
	else {	// Precompute some values...
		cost_L = cost_L * pNote->m_fVelocity;		// note velocity
		cost_L = cost_L * pNote->m_fPan_L;		// note pan
		cost_L = cost_L * fLayerGain;				// layer gain
		cost_L = cost_L * pInstr->m_fPan_L;		// instrument pan
		cost_L = cost_L * pInstr->m_fGain;		// instrument gain
		fSendFXLevel_L = cost_L;

		cost_L = cost_L * pInstr->m_fVolume;		// instrument volume
		cost_L = cost_L * m_pSong->getVolume();	// song volume


		cost_R = cost_R * pNote->m_fVelocity;		// note velocity
		cost_R = cost_R * pNote->m_fPan_R;		// note pan
		cost_R = cost_R * fLayerGain;				// layer gain
		cost_R = cost_R * pInstr->m_fPan_R;		// instrument pan
		cost_R = cost_R * pInstr->m_fGain;		// instrument gain
		fSendFXLevel_R = cost_R;

		cost_R = cost_R * pInstr->m_fVolume;		// instrument volume
		cost_R = cost_R * m_pSong->getVolume();	// song pan
	}

	// direct track outputs only use velocity
	cost_track = cost_track * pNote->m_fVelocity;
	cost_track = cost_track * fLayerGain;

	// Se non devo fare resample (drumkit) posso evitare di utilizzare i float e gestire il tutto in
	// maniera ottimizzata
	//	constant^12 = 2, so constant = 2^(1/12) = 1.059463.
	//	float nStep = 1.0;1.0594630943593

	if ( pNote->m_fPitch == 0.0 && fLayerPitch == 0.0 ) {	// NO RESAMPLE
		return audioEngine_renderNote_noResample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track, fSendFXLevel_L, fSendFXLevel_R );
	}
	else {	// RESAMPLE
		return audioEngine_renderNote_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track, fLayerPitch, fSendFXLevel_L, fSendFXLevel_R );
	}
}



void audioEngine_setupLadspaFX( unsigned nBufferSize ) {
	hydrogenInstance->infoLog( "[audioEngine_setupLadspaFX] buffersize=" + toString(nBufferSize) );

	if (m_pSong == NULL) {
		hydrogenInstance->infoLog( "[audioEngine_setupLadspaFX] m_pSong=NULL" );
		return;
	}
	if (nBufferSize == 0) {
		hydrogenInstance->errorLog( "[audioEngine_setupLadspaFX] nBufferSize=0" );
		return;
	}

#ifdef LADSPA_SUPPORT
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		if ( m_pSong->getLadspaFX(nFX) )	m_pSong->getLadspaFX(nFX)->deactivate();
		delete[] m_pFXBuffer_L[nFX];
		delete[] m_pFXBuffer_R[nFX];
		if (nBufferSize != 0) {
			m_nFXBufferSize = nBufferSize;
			m_pFXBuffer_L[nFX] = new float[ nBufferSize ];
			m_pFXBuffer_R[nFX] = new float[ nBufferSize ];

			//touch all the memory (is this really necessary?)
			for (unsigned i = 0; i < nBufferSize; ++i) {
				m_pFXBuffer_L[nFX][i] = 0;
				m_pFXBuffer_R[nFX][i] = 0;
			}
		}
		if ( m_pSong->getLadspaFX(nFX) ) {
			m_pSong->getLadspaFX(nFX)->connectAudioPorts( m_pFXBuffer_L[nFX], m_pFXBuffer_R[nFX], m_pFXBuffer_L[nFX], m_pFXBuffer_R[nFX] );
			m_pSong->getLadspaFX(nFX)->activate();
		}
	}
#endif
}



void audioEngine_setSong(Song *newSong) {
	hydrogenInstance->infoLog( "[audioEngine_setSong] " + newSong->m_sName );

	audioEngine_lock( "audioEngine_setSong" );

	if ( m_audioEngineState == STATE_PLAYING ) {
		m_pAudioDriver->stop();
		audioEngine_stop( false );
	}

	// check current state
	if (m_audioEngineState != STATE_PREPARED) {
		hydrogenInstance->errorLog("[audioEngine_setSong] Error the audio engine is not in PREPARED state");
	}

	m_pPlayingPatterns->clear();
	m_pNextPattern = NULL;

	audioEngine_clearNoteQueue();

	m_pSong = newSong;

	// update ticksize
	audioEngine_process_checkBPMChanged();

	// find the first pattern and set as current
	if ( m_pSong->getPatternList()->getSize() > 0 ) {
		m_pPlayingPatterns->add( m_pSong->getPatternList()->get(0) );
	}

	// setup LADSPA FX
	audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );

	m_pAudioDriver->setBpm( m_pSong->m_fBPM );


	// change the current audio engine state
	m_audioEngineState = STATE_READY;

	m_pAudioDriver->locate( 0 );

	audioEngine_unlock();

	EventQueue::getInstance()->pushEvent( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	EventQueue::getInstance()->pushEvent( EVENT_PATTERN_CHANGED, -1 );
	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_READY );
}



void audioEngine_removeSong() {
	audioEngine_lock( "audioEngine_removeSong" );

	if ( m_audioEngineState == STATE_PLAYING ) {
		m_pAudioDriver->stop();
		audioEngine_stop( false );
	}

	// check current state
	if (m_audioEngineState != STATE_READY) {
		hydrogenInstance->errorLog( "audioEngine_removeSong: Error the audio engine is not in READY state" );
		audioEngine_unlock();
		return;
	}

	m_pSong = NULL;
	m_pPlayingPatterns->clear();
	m_pNextPattern = NULL;

	audioEngine_clearNoteQueue();

	// change the current audio engine state
	m_audioEngineState = STATE_PREPARED;
	audioEngine_unlock();

	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_PREPARED );
}



// return -1 = end of song
// return 2 = send pattern changed event!!
inline int audioEngine_updateNoteQueue(unsigned nFrames)
{
	static int nLastTick = -1;
	bool bSendPatternChange = false;

	unsigned int framepos;
	if (  m_audioEngineState == STATE_PLAYING ) {
		framepos = m_pAudioDriver->m_transport.m_nFrames;
	}
	else {
		// use this to support realtime events when not playing
		framepos = m_nRealtimeFrames;
	}

 	int tickNumber_start = (int)(  framepos / m_pAudioDriver->m_transport.m_nTickSize );
 	int tickNumber_end = (int)( ( framepos + nFrames ) / m_pAudioDriver->m_transport.m_nTickSize );


	int tick = tickNumber_start;

	// get initial timestamp for first tick
	gettimeofday( &m_currentTickTime, NULL );

	while ( tick <= tickNumber_end ) {
		if (tick == nLastTick) {
			++tick;
			continue;
		}
		else {
			nLastTick = tick;
		}


		// midi events now get put into the m_songNoteQueue as well, based on their timestamp
		while ( m_midiNoteQueue.size() > 0 ) {
			Note *note = m_midiNoteQueue[0];

			if ((int)note->m_nPosition <= tick) {
				// printf ("tick=%d  pos=%d\n", tick, note->getPosition());
				m_midiNoteQueue.pop_front();
				m_songNoteQueue.push_back( note );
			}
			else {
				break;
			}
		}

		if (  m_audioEngineState != STATE_PLAYING ) {
			// only keep going if we're playing
			continue;
		}


		// SONG MODE
		if ( m_pSong->getMode() == Song::SONG_MODE ) {
			if ( m_pSong->getPatternGroupVector()->size() == 0 ) {
				// there's no song!!
				hydrogenInstance->errorLog( "[audioEngine_updateNoteQueue] no patterns in song." );
				m_pAudioDriver->stop();
				return -1;
			}

			m_nSongPos = findPatternInTick( tick, m_pSong->isLoopEnabled(), &m_nPatternStartTick );
			if (m_nSongSizeInTicks != 0) {
				m_nPatternTickPosition = (tick - m_nPatternStartTick) % m_nSongSizeInTicks;
			}
			else {
				m_nPatternTickPosition = tick - m_nPatternStartTick;
			}

			if (m_nPatternTickPosition == 0) {
				bSendPatternChange = true;
			}

			PatternList *pPatternList = (*(m_pSong->getPatternGroupVector()))[m_nSongPos];
			if ( m_nSongPos == -1 ) {
				hydrogenInstance->infoLog("[audioEngine_updateNoteQueue] song pos = -1");
				if ( m_pSong->isLoopEnabled() == true ) {	// check if the song loop is enabled
					m_nSongPos = findPatternInTick( 0, true, &m_nPatternStartTick );
				}
				else {
					hydrogenInstance->infoLog( "[audioEngine_updateNoteQueue] End of Song" );
					return -1;
				}
			}
			// copio tutti i pattern
			m_pPlayingPatterns->clear();
			if ( pPatternList ) {
				for (unsigned i = 0; i < pPatternList->getSize(); ++i) {
					m_pPlayingPatterns->add( pPatternList->get(i) );
				}
			}
		}

		// PATTERN MODE
		else if ( m_pSong->getMode() == Song::PATTERN_MODE )	{
			//hydrogenInstance->warningLog( "pattern mode not implemented yet" );

			// per ora considero solo il primo pattern, se ce ne saranno piu' di uno
			// bisognera' prendere quello piu' piccolo

			if ( m_pPlayingPatterns->getSize() != 0 ) {
				Pattern *pFirstPattern = m_pPlayingPatterns->get( 0 );
				int nPatternSize = pFirstPattern->m_nSize;
				if ( nPatternSize == 0 ) {
					hydrogenInstance->errorLog( "[audioEngine_updateNoteQueue] nPatternSize == 0" );
				}

				if ( ( tick == (int)(m_nPatternStartTick + nPatternSize) ) || ( m_nPatternStartTick == -1 ) ) {
					if ( m_pNextPattern != NULL ) {
						//hydrogenInstance->errorLog( "[audioEngine_updateNoteQueue] Aggiorno con nextpattern: " + toString( (int)m_pNextPattern ) );
						m_pPlayingPatterns->clear();
						m_pPlayingPatterns->add( m_pNextPattern );
						m_pNextPattern = NULL;
						bSendPatternChange = true;
					}
					m_nPatternStartTick = tick;
				}
				//m_nPatternTickPosition = tick % m_pCurrentPattern->getSize();
				m_nPatternTickPosition = tick % nPatternSize;
			}
			else {
				hydrogenInstance->errorLog( "[audioEngine_updateNoteQueue] Pattern mode. m_pPlayingPatterns->getSize() = 0" );
				hydrogenInstance->errorLog("[audioEngine_updateNoteQueue()] Panic! Stopping audio engine");
				// PANIC!
				m_pAudioDriver->stop();
			}
		}


		// metronome
//		if (( tick % 48 ) == 0) {
		if(  ( m_nPatternStartTick == tick ) || ( ( tick - m_nPatternStartTick ) % 48 == 0 ) ) {
			float fPitch;
			float fVelocity;
			if (m_nPatternTickPosition == 0) {
				fPitch = 3;
				fVelocity = 1.0;
				EventQueue::getInstance()->pushEvent( EVENT_METRONOME, 1 );
			}
			else {
				fPitch = 0;
				fVelocity = 0.8;
				EventQueue::getInstance()->pushEvent( EVENT_METRONOME, 0 );
			}
			if ( Preferences::getInstance()->m_bUseMetronome ) {
				m_pMetronomeInstrument->m_fVolume = Preferences::getInstance()->m_fMetronomeVolume;

				Note *pMetronomeNote = new Note( m_pMetronomeInstrument, tick, fVelocity, 1.0, 1.0, -1, fPitch );
				m_songNoteQueue.push_back( pMetronomeNote );
			}
		}



		// update the notes queue
		if ( m_pPlayingPatterns->getSize() != 0 ) {
			for (unsigned nPat = 0; nPat < m_pPlayingPatterns->getSize(); ++nPat) {
				Pattern *pPat = m_pPlayingPatterns->get( nPat );
				if (pPat == NULL) {
					hydrogenInstance->errorLog( "[audioEngine_updateNoteQueue] pPat = NULL" );
				}
				//hydrogenInstance->infoLog( "[audioEngine_updateNoteQueue] pPat = " + toString( (int)pPat ) );

				SequenceList *pSequenceList = pPat->m_pSequenceList;
				if ( pSequenceList == NULL ) {
					hydrogenInstance->errorLog( "[audioEngine_updateNoteQueue] pSequenceList = NULL" );
				}
				int nSequences = pSequenceList->getSize();
				for ( int i = 0; i < nSequences; ++i ) {
					Sequence* pSequence = pSequenceList->get(i);
					Note* pNote = pSequence->m_noteList[ m_nPatternTickPosition ];
					if ( pNote ) {
						unsigned nOffset = 0;
						float fVelocity = pNote->m_fVelocity;

						// Swing
						float fSwingFactor = m_pSong->getSwingFactor();
						if ( ( (m_nPatternTickPosition % 12) == 0 ) && ( (m_nPatternTickPosition % 24) != 0) ) {	// da l'accento al tick 4, 12, 20, 36...
							nOffset += (int)( ( 6.0 * m_pAudioDriver->m_transport.m_nTickSize ) * fSwingFactor );
						}

						// Humanize - Time parameter
						int nMaxTimeHumanize = 2000;
						if (m_pSong->getHumanizeTimeValue() != 0) {
							nOffset += (int)( getGaussian( 0.3 ) * m_pSong->getHumanizeTimeValue() * nMaxTimeHumanize );
						}
						//~

						Note *pCopiedNote = new Note( pNote->getInstrument(), tick, fVelocity, pNote->m_fPan_L, pNote->m_fPan_R, pNote->m_nLength, pNote->m_fPitch );
						pCopiedNote->m_nHumanizeDelay = nOffset;	// humanize time
						m_songNoteQueue.push_back( pCopiedNote );
					}
				}
			}
		}
		++tick;
	}

	// audioEngine_process must send the pattern change event after mutex unlock
	if (bSendPatternChange) {
		return 2;
	}
	return 0;
}



/// restituisce l'indice relativo al patternGroup in base al tick
inline int findPatternInTick( int nTick, bool bLoopMode, int *pPatternStartTick )
{
	assert( m_pSong );

	int nTotalTick = 0;
	m_nSongSizeInTicks = 0;

	vector<PatternList*> *pPatternColumns = m_pSong->getPatternGroupVector();
	int nColumns = pPatternColumns->size();

	int nPatternSize;
	for ( int i = 0; i < nColumns; ++i ) {
		PatternList *pColumn = (*pPatternColumns)[ i ];
		if ( pColumn->getSize() != 0 ) {
			// tengo in considerazione solo il primo pattern. I pattern nel gruppo devono avere la stessa lunghezza.
			nPatternSize = pColumn->get(0)->m_nSize;
		}
		else {
			nPatternSize = MAX_NOTES;
		}

		if ( ( nTick >= nTotalTick ) && ( nTick < nTotalTick + nPatternSize ) ) {
			(*pPatternStartTick) = nTotalTick;
			return i;
		}
		nTotalTick += nPatternSize;
	}

	if ( bLoopMode ) {
		m_nSongSizeInTicks = nTotalTick;
		int nLoopTick = 0;
		if (m_nSongSizeInTicks != 0) {
			nLoopTick = nTick % m_nSongSizeInTicks;
		}
		nTotalTick = 0;
		for ( int i = 0; i < nColumns; ++i ) {
			PatternList *pColumn = (*pPatternColumns)[ i ];
			if ( pColumn->getSize() != 0 ) {
				// tengo in considerazione solo il primo pattern. I pattern nel gruppo devono avere la stessa lunghezza.
				nPatternSize = pColumn->get(0)->m_nSize;
			}
			else {
				nPatternSize = MAX_NOTES;
			}

			if ( ( nLoopTick >= nTotalTick ) && ( nLoopTick < nTotalTick + nPatternSize ) ) {
				(*pPatternStartTick) = nTotalTick;
				return i;
			}
			nTotalTick += nPatternSize;
		}
	}

	char tmp[200];
	sprintf( tmp, "[findPatternInTick] tick = %d. No pattern found", nTick );
	hydrogenInstance->errorLog( tmp );
	return -1;
}



void audioEngine_noteOn(Note *note)
{
	// check current state
	if ( (m_audioEngineState != STATE_READY) && (m_audioEngineState != STATE_PLAYING) ) {
		hydrogenInstance->errorLog("audioEngine_noteOn: Error the audio engine is not in READY state");
		delete note;
		return;
	}

	m_midiNoteQueue.push_back(note);
}



void audioEngine_noteOff( Note *note )
{
	if (note == NULL)	{
		hydrogenInstance->errorLog("audioEngine_noteOff: Error, note == NULL");
	}

	audioEngine_lock( "audioEngine_noteOff" );

	// check current state
	if ( (m_audioEngineState != STATE_READY) && (m_audioEngineState != STATE_PLAYING) ) {
		hydrogenInstance->errorLog("audioEngine_noteOff: Error the audio engine is not in READY state");
		delete note;
		audioEngine_unlock();
		return;
	}

	for ( unsigned i = 0; i < m_playingNotesQueue.size(); ++i ) {	// delete old note
		Note *oldNote = m_playingNotesQueue[ i ];

		if ( oldNote->getInstrument() == note->getInstrument() ) {
			m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			delete oldNote;
			break;
		}
	}
	audioEngine_unlock();

	delete note;
}



unsigned long audioEngine_getTickPosition() {
	return m_nPatternTickPosition;
}


GenericDriver* createDriver(const std::string& sDriver)
{
	hydrogenInstance->infoLog("[createDriver] " + sDriver );
	Preferences *pPref = Preferences::getInstance();
	GenericDriver *pDriver = NULL;

	if (sDriver == "Oss") {
		pDriver = new OssDriver( audioEngine_process );
		if (pDriver->getClassName() == "NullDriver") {
			m_bUseTrackOuts = false;
			delete pDriver;
			pDriver = NULL;
		}
	}
	else if (sDriver == "Jack") {
		pDriver = new JackDriver(audioEngine_process);
		if (pDriver->getClassName() == "NullDriver" ) {
			m_bUseTrackOuts = false;
			delete pDriver;
			pDriver = NULL;
		}
		else {
#ifdef JACK_SUPPORT
			// set use track outputs flag
			m_bUseTrackOuts = pPref->m_bJackTrackOuts;
			((JackDriver *) pDriver)->setTrackOuts( m_bUseTrackOuts );

			m_bUseDefaultOuts = pPref->m_bJackConnectDefaults;
			((JackDriver *) pDriver)->setConnectDefaults( m_bUseDefaultOuts );
#endif
		}
	}
	else if (sDriver == "Alsa") {
		pDriver = new AlsaAudioDriver( audioEngine_process );
		if (pDriver->getClassName() == "NullDriver" ) {
			delete pDriver;
			pDriver = NULL;
		}
	}
	else if (sDriver == "PortAudio") {
		pDriver = new PortAudioDriver( audioEngine_process );
		if (pDriver->getClassName() == "NullDriver" ) {
			delete pDriver;
			pDriver = NULL;
		}
	}
	else if (sDriver == "Fake") {
		hydrogenInstance->warningLog( "*** Using FAKE audio driver ***" );
		pDriver = new FakeDriver( audioEngine_process );
	}
	else {
		hydrogenInstance->errorLog( "[createDriver] Unknown driver " + sDriver );
		audioEngine_raiseError( Hydrogen::UNKNOWN_DRIVER );
	}

	if ( pDriver  ) {
		// initialize the audio driver
		int res = pDriver->init( pPref->m_nBufferSize );
		if (res != 0) {
			hydrogenInstance->errorLog("[createDriver] Error starting audio driver [audioDriver::init()]");
			delete pDriver;
			pDriver = NULL;
			m_bUseTrackOuts = false;
		}
	}

	return pDriver;
}


/// Start all audio drivers
void audioEngine_startAudioDrivers() {
	Preferences *preferencesMng = Preferences::getInstance();
	m_bUseTrackOuts = false;


	audioEngine_lock( "audioEngine_startAudioDrivers" );

	hydrogenInstance->infoLog("[audioEngine_startAudioDrivers]");

	// check current state
	if (m_audioEngineState != STATE_INITIALIZED) {
		hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Error the audio engine is not in INITIALIZED state. state=" + toString(m_audioEngineState) );
		audioEngine_unlock();
		return;
	}
	if (m_pAudioDriver) {	// check if the audio m_pAudioDriver is still alive
		hydrogenInstance->errorLog( "[audioEngine_startAudioDrivers] The audio driver is still alive" );
	}
	if (m_pMidiDriver) {	// check if midi driver is still alive
		hydrogenInstance->errorLog( "[audioEngine_startAudioDrivers] The MIDI driver is still active");
	}


	string sAudioDriver = preferencesMng->m_sAudioDriver;
//	sAudioDriver = "Auto";
	if (sAudioDriver == "Auto" ) {
		if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == NULL ) {
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == NULL ) {
				if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == NULL ) {
					if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == NULL ) {
						audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
						hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Error starting audio driver");
						hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Using the NULL output audio driver");

						// use the NULL output driver
						m_bUseTrackOuts = false;
						m_pAudioDriver = new NullDriver( audioEngine_process );
						m_pAudioDriver->init(0);
					}
				}
			}
		}
	}
	else {
		m_pAudioDriver = createDriver( sAudioDriver );
		if ( m_pAudioDriver == NULL ) {
			audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Error starting audio driver");
			hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Using the NULL output audio driver");

			// use the NULL output driver
			m_bUseTrackOuts = false;
			m_pAudioDriver = new NullDriver( audioEngine_process );
			m_pAudioDriver->init(0);
		}
	}

	if ( preferencesMng->m_sMidiDriver == "ALSA" ) {
#ifdef ALSA_SUPPORT
		// Create MIDI driver
		m_pMidiDriver = new AlsaMidiDriver();
		m_pMidiDriver->open();
		m_pMidiDriver->setActive(true);
#endif
	}
	else if ( preferencesMng->m_sMidiDriver == "PortMidi" ) {
#ifdef PORTMIDI_SUPPORT
		m_pMidiDriver = new PortMidiDriver();
		m_pMidiDriver->open();
		m_pMidiDriver->setActive(true);
#endif
	}

	// change the current audio engine state
	if (m_pSong == NULL) {
		m_audioEngineState = STATE_PREPARED;
	}
	else {
		m_audioEngineState = STATE_READY;
	}


	if (m_pSong) {
		m_pAudioDriver->setBpm( m_pSong->m_fBPM );
	}

	audioEngine_unlock();	// test...

	if ( m_pAudioDriver ) {
		int res = m_pAudioDriver->connect();
		if (res != 0) {
			audioEngine_raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Error starting audio driver [audioDriver::connect()]");
			hydrogenInstance->errorLog("[audioEngine_startAudioDrivers] Using the NULL output audio driver");

			delete m_pAudioDriver;
			m_bUseTrackOuts = false;
			m_pAudioDriver = new NullDriver( audioEngine_process );
			m_pAudioDriver->init(0);
			m_pAudioDriver->connect();
		}

		if ( ( m_pMainBuffer_L = m_pAudioDriver->getOut_L() ) == NULL ) {
			hydrogenInstance->errorLog( "[audioEngine_startAudioDrivers] m_pMainBuffer_L == NULL" );
		}
		if ( ( m_pMainBuffer_R = m_pAudioDriver->getOut_R() ) == NULL ) {
			hydrogenInstance->errorLog( "[audioEngine_startAudioDrivers] m_pMainBuffer_R == NULL" );
		}

#ifdef JACK_SUPPORT
		if (m_bUseTrackOuts) {
			for (unsigned i = 0; i < MAX_INSTRUMENTS; ++i) {
				m_pTrackBuffers_L[i] = ((JackDriver*)m_pAudioDriver)->getTrackOut_L(i);
				m_pTrackBuffers_R[i] = ((JackDriver*)m_pAudioDriver)->getTrackOut_R(i);
			}
		}
#endif

		audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );
	}



	if ( m_audioEngineState == STATE_PREPARED ) {
		EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_PREPARED );
	}
	else if ( m_audioEngineState == STATE_READY ) {
		EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_READY );
	}
}



/// Stop all audio drivers
void audioEngine_stopAudioDrivers() {
	hydrogenInstance->infoLog("[audioEngine_stopAudioDrivers]");

	// check current state
	if (m_audioEngineState == STATE_PLAYING) {
		audioEngine_stop();
	}

	if ( (m_audioEngineState != STATE_PREPARED) && (m_audioEngineState != STATE_READY) ) {
		hydrogenInstance->errorLog("audioEngine_stopAudioDrivers: Error the audio engine is not in PREPARED or READY state. state=" + toString(m_audioEngineState) );
		return;
	}

	// delete MIDI driver
	if (m_pMidiDriver) {
		m_pMidiDriver->close();
		delete m_pMidiDriver;
		m_pMidiDriver = NULL;
	}

	// delete audio driver
	if (m_pAudioDriver) {
		m_pAudioDriver->disconnect();
		delete m_pAudioDriver;
		m_pAudioDriver = NULL;
	}


	audioEngine_lock( "audioEngine_stopAudioDrivers" );
	// change the current audio engine state
	m_audioEngineState = STATE_INITIALIZED;
	EventQueue::getInstance()->pushEvent( EVENT_STATE, STATE_INITIALIZED );
	audioEngine_unlock();
}



/// Restart all audio and midi drivers
void audioEngine_restartAudioDrivers() {
	audioEngine_stopAudioDrivers();
	audioEngine_startAudioDrivers();
	//audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );
}






//----------------------------------------------------------------------------
//
// Implementation of Hydrogen class
//
//----------------------------------------------------------------------------




Hydrogen::Hydrogen()
 : Object( "Hydrogen   " )
 {
	if (instance) {
		errorLog( "Hydrogen audio engine is already running" );
	}

	hydrogenInstance = this;
	instance = this;
	audioEngine_init();
	audioEngine_startAudioDrivers();
}



Hydrogen::~Hydrogen() {
	if ( m_audioEngineState == STATE_PLAYING ) {
		audioEngine_stop();
	}
	removeSong();
	audioEngine_stopAudioDrivers();
	audioEngine_destroy();
	instance = NULL;
}



/// Return the Hydrogen instance
Hydrogen* Hydrogen::getInstance() {
	if (instance == NULL) {
		instance = new Hydrogen();
	}
	return instance;
}



/// Start the internal sequencer
void Hydrogen::start() {
	// play from start if pattern mode is enabled
	if (m_pSong->getMode() == Song::PATTERN_MODE) {
		setPatternPos( 0 );
	}
	m_pAudioDriver->play();
}



/// Stop the internal sequencer
void Hydrogen::stop() {
	m_pAudioDriver->stop();
}



void Hydrogen::setSong(Song *newSong) {
	audioEngine_setSong(newSong);
}



void Hydrogen::removeSong() {
	audioEngine_removeSong();
}



Song* Hydrogen::getSong() {
	return m_pSong;
}



void Hydrogen::noteOn(Note *note) {
	audioEngine_noteOn(note);
}



void Hydrogen::noteOff(Note *note) {
	audioEngine_noteOff(note);
}



void Hydrogen::addRealtimeNote(int instrument, float velocity, float pan_L, float pan_R, float pitch, bool forcePlay)
{
	lockEngine( "Hydrogen::addRealtimeNote" );	// lock the audio engine

	unsigned int column = getTickPosition();
	unsigned int realcolumn = 0;

	realcolumn = getRealtimeTickPosition();

	//printf ("orig tickpos=%u  real=%u\n", column, realcolumn);

	Preferences *pref = Preferences::getInstance();

	// quantize it to scale
	unsigned res = pref->getPatternEditorGridResolution();
	int nBase = pref->isPatternEditorUsingTriplets() ? 3 : 4;
	int scalar = (4 * MAX_NOTES) / (res*nBase);
	int qcolumn = (int)::round(column / (double)scalar) * scalar;
	if (qcolumn == MAX_NOTES) qcolumn = 0;

	//printf ("column=%d  qcol=%d\n", column, qcolumn);

	if (pref->getQuantizeEvents()) {
		column = qcolumn;
	}

	unsigned position = column;
	bool hearnote = forcePlay;

	Pattern* currentPattern = NULL;
	PatternList *pPatternList = m_pSong->getPatternList();
	if ( (m_nSelectedPatternNumber != -1) && (m_nSelectedPatternNumber < (int)pPatternList->getSize() ) ) {
		currentPattern = pPatternList->get( m_nSelectedPatternNumber );
	}

	Song *song = getSong();
	Instrument *instrRef=0;
	if (song) {
		instrRef = (song->getInstrumentList())->get(instrument);
	}

	if (currentPattern) {
		SequenceList *sequenceList = currentPattern->m_pSequenceList;
		Sequence *seq = sequenceList->get(instrument);

		if (seq->m_noteList[column] != NULL) {
			// in this case, we'll leave the note alone
			// hear note only if not playing too
			if ( pref->getHearNewNotes() && getState() == STATE_READY) {
				hearnote = true;
			}
		}
		else if (!pref->getRecordEvents()) {
			if ( pref->getHearNewNotes() && (getState() == STATE_READY || getState() == STATE_PLAYING )) {
				hearnote = true;
			}
		}
		else {
			// create the new note
			Note *note = new Note( instrRef, position, velocity, pan_L, pan_R, -1, 0);
			seq->m_noteList[column] = note;

			// hear note if its not in the future
			if ( pref->getHearNewNotes() && position <= getTickPosition()) {
				hearnote = true;
			}

			song->m_bIsModified = true;

			EventQueue::getInstance()->pushEvent( EVENT_PATTERN_MODIFIED, -1 );
		}
	}
	else if (pref->getHearNewNotes()) {
		hearnote = true;
	}

	if (hearnote && instrRef) {
		Note *note2 = new Note( instrRef, realcolumn, velocity, pan_L, pan_R, -1, 0);
		noteOn( note2 );
	}

	unlockEngine(); // unlock the audio engine
}



float Hydrogen::getMasterPeak_L() {
	return m_fMasterPeak_L;
}



float Hydrogen::getMasterPeak_R() {
	return m_fMasterPeak_R;
}



unsigned long Hydrogen::getTickPosition() {
	return audioEngine_getTickPosition();
}



unsigned long Hydrogen::getRealtimeTickPosition()
{
	//unsigned long initTick = audioEngine_getTickPosition();
	unsigned int initTick = (unsigned int)( m_nRealtimeFrames / m_pAudioDriver->m_transport.m_nTickSize );
	unsigned long retTick;

	struct timeval currtime;
	struct timeval deltatime;

	double sampleRate = (double) m_pAudioDriver->getSampleRate();
	gettimeofday (&currtime, NULL);

	timersub( &currtime, &m_currentTickTime, &deltatime );

	// add a buffers worth for jitter resistance
	double deltaSec = (double) deltatime.tv_sec + (deltatime.tv_usec / 1000000.0) +  (m_pAudioDriver->getBufferSize() / (double)sampleRate);

	retTick = (unsigned long) ((sampleRate / (double) m_pAudioDriver->m_transport.m_nTickSize) * deltaSec);

	retTick = initTick + retTick;

	return retTick;
}



PatternList* Hydrogen::getCurrentPatternList() {
	return m_pPlayingPatterns;
}



/// Set the next pattern (Pattern mode only)
void Hydrogen::setNextPattern( int pos )
{
	audioEngine_lock( "Hydrogen::setNextPattern" );
	//infoLog( "[setNextPattern] " + toString(pos) );

	if ( m_pSong && m_pSong->getMode() == Song::PATTERN_MODE ) {
		PatternList *patternList = m_pSong->getPatternList();
		if ( (pos >= 0) && ( pos < (int)patternList->getSize() ) ) {
			m_pNextPattern = patternList->get(pos);
		}
		else {
			errorLog("[setNextPattern] pos not in patternList range. pos=" + toString(pos) + " patternListSize=" + toString(patternList->getSize()));
			m_pNextPattern = NULL;
		}
	}
	else {
		errorLog("[setNextPattern] can't set next pattern in song mode");
		m_pNextPattern = NULL;
	}

	audioEngine_unlock();
}



int Hydrogen::getPatternPos()
{
	return m_nSongPos;
}



unsigned Hydrogen::getPlayingNotes()
{
	return m_nPlayingNotes;
}



void Hydrogen::restartDrivers()
{
	audioEngine_restartAudioDrivers();
}



/// Export a song to a wav file, returns the elapsed time in mSec
void Hydrogen::startExportSong(const std::string& filename)
{
	if ( getState() == STATE_PLAYING ) {
		stop();
	}
	Preferences *pPref = Preferences::getInstance();

	m_oldEngineMode = m_pSong->getMode();
	m_bOldUseTrackOuts = m_bUseTrackOuts;
	m_bOldLoopEnabled = m_pSong->isLoopEnabled();

	m_pSong->setMode( Song::SONG_MODE );
	m_bUseTrackOuts = false;
	m_pSong->setLoopEnabled( false );
	unsigned nSamplerate = m_pAudioDriver->getSampleRate();

	// stop all audio drivers
	audioEngine_stopAudioDrivers();

	/*
		FIXME: Questo codice fa davvero schifo....
	*/

	m_pAudioDriver = new DiskWriterDriver( audioEngine_process, nSamplerate, filename );

	// reset
	m_pAudioDriver->m_transport.m_nFrames = 0;	// reset total frames
	m_pAudioDriver->setBpm( m_pSong->m_fBPM );
	m_nSongPos = 0;
	m_nPatternTickPosition = 0;
	m_audioEngineState = STATE_PLAYING;
	m_nPatternStartTick = -1;

	int res = m_pAudioDriver->init( pPref->m_nBufferSize );
	if (res != 0) {
		errorLog("[exportSong] Error starting disk writer driver [DiskWriterDriver::init()]");
	}

	m_pMainBuffer_L = m_pAudioDriver->getOut_L();
	m_pMainBuffer_R = m_pAudioDriver->getOut_R();

	audioEngine_setupLadspaFX(m_pAudioDriver->getBufferSize());

	unsigned start = currentTime();

	audioEngine_seek( 0, false );

	res = m_pAudioDriver->connect();
	if (res != 0) {
		errorLog("[exportSong] Error starting disk writer driver [DiskWriterDriver::connect()]");
	}
}



void Hydrogen::stopExportSong()
{
	if ( m_pAudioDriver->getClassName() != "DiskWriterDriver" ) {
		return;
	}

//	audioEngine_stopAudioDrivers();
	m_pAudioDriver->disconnect();

	m_audioEngineState = STATE_INITIALIZED;
	delete m_pAudioDriver;
	m_pAudioDriver = NULL;

	m_pMainBuffer_L = NULL;
	m_pMainBuffer_R = NULL;

	m_bUseTrackOuts = m_bOldUseTrackOuts;
	m_pSong->setMode( m_oldEngineMode );
	m_pSong->setLoopEnabled( m_bOldLoopEnabled );

	m_nSongPos = -1;
	m_nPatternTickPosition = 0;
	audioEngine_startAudioDrivers();

	if (m_pAudioDriver) {
		m_pAudioDriver->setBpm( m_pSong->m_fBPM );
	}
	else {
		errorLog( "[exportSong] m_pAudioDriver = NULL" );
	}
}



unsigned Hydrogen::getSongNotesQueue()
{
	return m_songNoteQueue.size();
}



/// Used to display audio driver info
GenericDriver* Hydrogen::getAudioDriver()
{
	return m_pAudioDriver;
}



/// Used to display midi driver info
MidiDriver* Hydrogen::getMidiDriver()
{
	return m_pMidiDriver;
}



void Hydrogen::setMasterPeak_L(float value) {
	m_fMasterPeak_L = value;
}



void Hydrogen::setMasterPeak_R(float value) {
	m_fMasterPeak_R = value;
}



int Hydrogen::getState() {
	return m_audioEngineState;
}



void Hydrogen::setCurrentPatternList(PatternList *pPatternList) {
	audioEngine_lock( "Hydrogen::setCurrentPatternList" );
	m_pPlayingPatterns = pPatternList;
	EventQueue::getInstance()->pushEvent( EVENT_PATTERN_CHANGED, -1 );
	audioEngine_unlock();
}






/// Lock the audio engine
void Hydrogen::lockEngine(const std::string& sLocker) {
	audioEngine_lock(sLocker);
}



/// Unlock the audio engine
void Hydrogen::unlockEngine() {
	audioEngine_unlock();
}



float Hydrogen::getProcessTime() {
	return m_fProcessTime;
}



float Hydrogen::getMaxProcessTime() {
	return m_fMaxProcessTime;
}



int Hydrogen::loadDrumkit( DrumkitInfo *drumkitInfo )
{
	infoLog( "[loadDrumkit] " + drumkitInfo->getName() );
	LocalFileMng fileMng;
	string sDrumkitPath = fileMng.getDrumkitDirectory( drumkitInfo->getName() );

	InstrumentList *songInstrList = m_pSong->getInstrumentList();
	InstrumentList *pDrumkitInstrList = drumkitInfo->getInstrumentList();
	for (unsigned i = 0; i < pDrumkitInstrList->getSize(); ++i ) {
		Instrument *pInstr = songInstrList->get( i );
		if ( ! pInstr->m_bIsLocked ) {
			Instrument *pNewInstr = pDrumkitInstrList->get( i );
			infoLog( "[loadDrumkit] Loading instrument (" + toString( i ) + " of " + toString( pDrumkitInstrList->getSize() ) + ") [ " + pNewInstr->m_sName + " ]" );
			// creo i nuovi layer in base al nuovo strumento
			for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
				InstrumentLayer *pNewLayer = pNewInstr->getLayer( nLayer );
				if (pNewLayer != NULL) {
					Sample *pNewSample = pNewLayer->m_pSample;
					string sSampleFilename = sDrumkitPath + drumkitInfo->getName() + "/" + pNewSample->m_sFilename;
					infoLog( "[loadDrumkit]    |-> Loading layer [ " + sSampleFilename + " ]" );

					// carico il nuovo sample e creo il nuovo layer
					Sample *pSample = Sample::load( sSampleFilename );
	//				pSample->setFilename( pNewSample->getFilename() );	// riuso il path del nuovo sample (perche' e' gia relativo al path del drumkit)
					if (pSample == NULL) {
						errorLog( "[loadDrumkit] Error Loading drumkit: NULL sample, now using /emptySample.wav" );
						pSample->m_sFilename = string(DataPath::getDataPath() ).append( "/emptySample.wav" );
						pSample = Sample::load( pSample->m_sFilename );
					}
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->m_fStartVelocity = pNewLayer->m_fStartVelocity;
					pLayer->m_fEndVelocity = pNewLayer->m_fEndVelocity;
					pLayer->m_fGain = pNewLayer->m_fGain;

					InstrumentLayer *pOldLayer = pInstr->getLayer(nLayer);
					audioEngine_lock( "Hydrogen::loadDrumkit" );
					pInstr->setLayer( pLayer, nLayer );	// set the new layer
					audioEngine_unlock();
					delete pOldLayer;		// delete the old layer

				}
				else {
					InstrumentLayer *pOldLayer = pInstr->getLayer(nLayer);
					audioEngine_lock( "Hydrogen::loadDrumkit" );
					pInstr->setLayer( NULL, nLayer );
					audioEngine_unlock();
					delete pOldLayer;		// delete the old layer
				}

			}
			audioEngine_lock( "Hydrogen::loadDrumkit" );
			// update instrument properties
			pInstr->m_sName = pNewInstr->m_sName;
			pInstr->m_fPan_L = pNewInstr->m_fPan_L;
			pInstr->m_fPan_R = pNewInstr->m_fPan_R;
			pInstr->m_fVolume = pNewInstr->m_fVolume;
			pInstr->m_sDrumkitName = pNewInstr->m_sDrumkitName;
			pInstr->m_bIsMuted = pNewInstr->m_bIsMuted;
			pInstr->m_fRandomPitchFactor = pNewInstr->m_fRandomPitchFactor;
			pInstr->m_pADSR = new ADSR( *( pNewInstr->m_pADSR ) );
			pInstr->m_bFilterActive = pNewInstr->m_bFilterActive;
			pInstr->m_fCutoff = pNewInstr->m_fCutoff;
			pInstr->m_fResonance = pNewInstr->m_fResonance;
			pInstr->m_excludeVectId.clear();
			for (unsigned i = 0; i < pNewInstr->m_excludeVectId.size(); ++i) {
				pInstr->m_excludeVectId.push_back( pNewInstr->m_excludeVectId[ i ] );
			}
			pInstr->m_excludeVect.clear();

			audioEngine_unlock();
		}
	}

	audioEngine_lock("Hydrogen::loadDrumkit");
	// rebuild the exclude vector
	for (unsigned nInstr = 0; nInstr < songInstrList->getSize(); ++nInstr) {
		Instrument* pInstr = songInstrList->get( nInstr );
		for (unsigned i = 0; i < pInstr->m_excludeVectId.size(); ++i) {
			int id = pInstr->m_excludeVectId[ i ];
			Instrument* pExcluded = songInstrList->get( id );
			pInstr->m_excludeVect.push_back( pExcluded );
		}
	}
	audioEngine_unlock();

	return 0;	//ok
}



void Hydrogen::raiseError( unsigned nErrorCode )
{
	audioEngine_raiseError( nErrorCode );
}


unsigned long Hydrogen::getTotalFrames()
{
	return m_pAudioDriver->m_transport.m_nFrames;
}



/// Set the position in the song
void Hydrogen::setPatternPos( int pos )
{
	audioEngine_lock( "Hydrogen::setPatternPos" );

	int nPatternGroups = (m_pSong->getPatternGroupVector())->size();
	if ( pos >= nPatternGroups ) {
		hydrogenInstance->warningLog( "[Hydrogen::setPatternPos()] patternPos > nPatternGroups. pos: " + toString(pos) + ", nPatternGroups: " + toString(nPatternGroups) );
		audioEngine_unlock();
		return;
	}

	vector<PatternList*> *pColumns = m_pSong->getPatternGroupVector();
	int totalTick = 0;
	int nPatternSize;
	Pattern *pPattern = NULL;
	for ( int i = 0; i < pos; ++i ) {
		PatternList *pColumn = (*pColumns)[ i ];
		pPattern = pColumn->get(0);	// prendo solo il primo. I pattern nel gruppo devono avere la stessa lunghezza
		if ( pPattern ) {
			nPatternSize = pPattern->m_nSize;
		}
		else {
			nPatternSize = MAX_NOTES;
		}

		totalTick += nPatternSize;
	}

	if (getState() != STATE_PLAYING) {
		// find pattern immediately when not playing
		int dummy;
		m_nSongPos = findPatternInTick( totalTick, m_pSong->isLoopEnabled(), &dummy);
	}

	m_pAudioDriver->locate( (int) (totalTick * m_pAudioDriver->m_transport.m_nTickSize) );


	audioEngine_unlock();
}



/// Preview, usa solo il primo layer
void Hydrogen::previewSample( Sample *pSample )
{
	audioEngine_lock( "Hydrogen::previewSample" );

	InstrumentLayer *pLayer = m_pPreviewInstrument->getLayer(0);

	Sample *pOldSample = pLayer->m_pSample;
	pLayer->m_pSample = pSample;
	delete pOldSample;

	Note *previewNote = new Note( m_pPreviewInstrument, 0, 1.0, 1.0, 1.0, MAX_NOTES, 0 );
	audioEngine_noteOn( previewNote );

	audioEngine_unlock();
}



void Hydrogen::getLadspaFXPeak( int nFX, float *fL, float *fR )
{
#ifdef LADSPA_SUPPORT
	(*fL) = m_fFXPeak_L[nFX];
	(*fR) = m_fFXPeak_R[nFX];
#else
	(*fL) = 0;
	(*fR) = 0;
#endif
}



void Hydrogen::setLadspaFXPeak( int nFX, float fL, float fR )
{
#ifdef LADSPA_SUPPORT
	m_fFXPeak_L[nFX] = fL;
	m_fFXPeak_R[nFX] = fR;
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

	if ( fabs(fOldBpm1 - fBPM) > 20 ) {	// troppa differenza, niente media
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

	fBPM = (fBPM + fOldBpm1 + fOldBpm2 + fOldBpm3 + fOldBpm4 + fOldBpm5 + fOldBpm6 + fOldBpm7 + fOldBpm8) / 9.0;


	infoLog( "[setTapTempo] avg BPM = " + toString(fBPM) );
	fOldBpm8 = fOldBpm7;
	fOldBpm7 = fOldBpm6;
	fOldBpm6 = fOldBpm5;
	fOldBpm5 = fOldBpm4;
	fOldBpm4 = fOldBpm3;
	fOldBpm3 = fOldBpm2;
	fOldBpm2 = fOldBpm1;
	fOldBpm1 = fBPM;

	lockEngine("Hydrogen::setTapTempo");

// 	m_pAudioDriver->setBpm( fBPM );
// 	m_pSong->setBpm( fBPM );

	setBPM( fBPM );

	unlockEngine();
}



void Hydrogen::setBPM( float fBPM )
{
	if (m_pAudioDriver && m_pSong) {
		m_pAudioDriver->setBpm( fBPM );
		m_pSong->m_fBPM = fBPM;
//		audioEngine_process_checkBPMChanged();
	}
}



void Hydrogen::restartLadspaFX() {
	if (m_pAudioDriver) {
		lockEngine("Hydrogen::restartLadspaFX");
		audioEngine_setupLadspaFX( m_pAudioDriver->getBufferSize() );
		unlockEngine();
	}
	else {
		errorLog( "[restartLadspaFX] m_pAudioDriver = NULL" );
	}
}



int Hydrogen::getSelectedPatternNumber()
{
	return m_nSelectedPatternNumber;
}



void Hydrogen::setSelectedPatternNumber(int nPat)
{
	// FIXME: controllare se e' valido..
	if (nPat == m_nSelectedPatternNumber)	return;

	m_nSelectedPatternNumber = nPat;

	EventQueue::getInstance()->pushEvent( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}



int Hydrogen::getSelectedInstrumentNumber()
{
	return m_nSelectedInstrumentNumber;
}



void Hydrogen::setSelectedInstrumentNumber( int nInstrument )
{
	if (m_nSelectedInstrumentNumber == nInstrument)	return;

	m_nSelectedInstrumentNumber = nInstrument;
	EventQueue::getInstance()->pushEvent( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

