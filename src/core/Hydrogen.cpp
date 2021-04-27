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
#include <core/config.h>

#ifdef WIN32
#    include "core/Timehelper.h"
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
#include <thread>
#include <chrono>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <core/EventQueue.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/H2Exception.h>
#include <core/AudioEngine.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/Basics/AutomationPath.h>
#include <core/Hydrogen.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Helpers/Filesystem.h>
#include <core/FX/LadspaFX.h>
#include <core/FX/Effects.h>

#include <core/Preferences.h>
#include <core/Sampler/Sampler.h>
#include "MidiMap.h"
#include <core/Timeline.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#include "OscServer.h"
#endif

#include <core/IO/AudioOutput.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/NullDriver.h>
#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>
#include <core/IO/CoreMidiDriver.h>
#include <core/IO/TransportInfo.h>
#include <core/IO/OssDriver.h>
#include <core/IO/FakeDriver.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/JackMidiDriver.h>
#include <core/IO/PortMidiDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/PulseAudioDriver.h>

namespace H2Core
{
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
	m_pNextSong = nullptr;

	m_bExportSessionIsActive = false;
	m_pTimeline = new Timeline();
	m_pCoreActionController = new CoreActionController();
	m_GUIState = GUIState::unavailable;
	m_nMaxTimeHumanize = 2000;
	m_fNewBpmJTM = 120;
	m_nSelectedInstrumentNumber =  0;

	initBeatcounter();
	InstrumentComponent::setMaxLayers( Preferences::get_instance()->getMaxLayers() );
	
	m_pAudioEngine = new AudioEngine();
	Playlist::create_instance();

	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_INITIALIZED );

	// Prevent double creation caused by calls from MIDI thread
	__instance = this;

	// When under session management and using JACK as audio driver,
	// it is crucial for Hydrogen to activate the JACK client _after_
	// the initial Song was set. Else the per track outputs will not
	// be registered in time and the session software won't be able to
	// rewire them properly. Therefore, the audio driver is started in
	// the callback function for opening a Song in nsm_open_cb().
	//
	// But the presence of the environmental variable NSM_URL does not
	// guarantee for a session management to be present (and at this
	// early point of initialization it's basically impossible to
	// tell). As a fallback the main() function will check for the
	// presence of the audio driver after creating both the Hydrogen
	// and NsmClient instance and prior to the creation of the GUI. If
	// absent, the starting of the audio driver will be triggered.
	if ( ! getenv( "NSM_URL" ) ){
		m_pAudioEngine->startAudioDrivers();
	}
	
	for(int i = 0; i< MAX_INSTRUMENTS; i++){
		m_nInstrumentLookupTable[i] = i;
	}

	if ( Preferences::get_instance()->getOscServerEnabled() ) {
		toggleOscServer( true );
	}
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

	if ( getState() == STATE_PLAYING ) {
		m_pAudioEngine->stop();
	}
	removeSong();
	
	
	m_pAudioEngine->stopAudioDrivers();
	m_pAudioEngine->destroy();
	__kill_instruments();

	delete m_pCoreActionController;
	delete m_pTimeline;
	delete m_pAudioEngine;

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
	pSong->getPatternList()->set_to_old();
	m_pAudioEngine->getAudioDriver()->play();
}

/// Stop the internal sequencer
void Hydrogen::sequencer_stop()
{
	if( Hydrogen::get_instance()->getMidiOutput() != nullptr ){
		Hydrogen::get_instance()->getMidiOutput()->handleQueueAllNoteOff();
	}

	m_pAudioEngine->getAudioDriver()->stop();
	Preferences::get_instance()->setRecordEvents(false);
}

bool Hydrogen::setPlaybackTrackState( const bool state )
{
	Song* pSong = getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	return pSong->setPlaybackTrackEnabled(state);
}

void Hydrogen::loadPlaybackTrack( const QString filename )
{
	Song* pSong = getSong();
	pSong->setPlaybackTrackFilename(filename);

	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();
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

	if ( pCurrentSong != nullptr ) {
		/* NOTE: 
		 *       - this is actually some kind of cleanup 
		 *       - removeSong cares itself for acquiring a lock
		 */
		removeSong();
		delete pCurrentSong;
	}

	if ( m_GUIState != GUIState::unavailable ) {
		/* Reset GUI */
		EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
		EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	}
	
	// In order to allow functions like audioEngine_setupLadspaFX() to
	// load the settings of the new song, like whether the LADSPA FX
	// are activated, __song has to be set prior to the call of
	// audioEngine_setSong().
	__song = pSong;

	// Update the audio engine to work with the new song.
	m_pAudioEngine->setSong( pSong );

	// load new playback track information
	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();

	// Push current state of Hydrogen to attached control interfaces,
	// like OSC clients.
	m_pCoreActionController->initExternalControlInterfaces();

	if ( isUnderSessionManagement() ) {
#ifdef H2CORE_HAVE_OSC
		NsmClient::linkDrumkit( NsmClient::get_instance()->m_sSessionFolderPath.toLocal8Bit().data(), true );
#endif
	} else {		
		Preferences::get_instance()->setLastSongFilename( pSong->getFilename() );
	}
}

/* Mean: remove current song from memory */
void Hydrogen::removeSong()
{
	__song = nullptr;
	m_pAudioEngine->removeSong();
}

void Hydrogen::midi_noteOn( Note *note )
{
	m_pAudioEngine->noteOn( note );
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
	
	AudioEngine* pAudioEngine = m_pAudioEngine;
	Preferences *pPreferences = Preferences::get_instance();
	unsigned int nRealColumn = 0;
	unsigned res = pPreferences->getPatternEditorGridResolution();
	int nBase = pPreferences->getPatternEditorGridTupletNumerator();
	int nTupletDenominator = pPreferences->getPatternEditorGridTupletDenominator();
	int scalar = ( 4 * MAX_NOTES ) / ( res * nBase ); // TODO float? TupletDenominator??
	bool hearnote = forcePlay;
	int currentPatternNumber;

	m_pAudioEngine->lock( RIGHT_HERE );

	Song *pSong = getSong();
	if ( !pPreferences->__playselectedinstrument ) {
		if ( instrument >= ( int ) pSong->getInstrumentList()->size() ) {
			// unused instrument
			pAudioEngine->unlock();
			return;
		}
	}

	// Get current partern and column, compensating for "lookahead" if required
	const Pattern* currentPattern = nullptr;
	unsigned int column = 0;
	float fTickSize = pAudioEngine->getAudioDriver()->m_transport.m_fTickSize;
	unsigned int lookaheadTicks = calculateLookahead( fTickSize ) / fTickSize;
	bool doRecord = pPreferences->getRecordEvents();
	if ( pSong->getMode() == Song::SONG_MODE && doRecord &&
		 pAudioEngine->getState() == STATE_PLAYING )
	{

		// Recording + song playback mode + actually playing
		PatternList *pPatternList = pSong->getPatternList();
		int ipattern = getPatternPos(); // playlist index
		if ( ipattern < 0 || ipattern >= (int) pPatternList->size() ) {
			pAudioEngine->unlock(); // unlock the audio engine
			return;
		}
		// Locate column -- may need to jump back in the pattern list
		column = getTickPosition();
		while ( column < lookaheadTicks ) {
			ipattern -= 1;
			if ( ipattern < 0 || ipattern >= (int) pPatternList->size() ) {
				pAudioEngine->unlock(); // unlock the audio engine
				return;
			}

			// Convert from playlist index to actual pattern index
			std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
			PatternList *pColumn = ( *pColumns )[ ipattern ];
			currentPatternNumber = -1;
			for ( int n = 0; n < pColumn->size(); n++ ) {
				Pattern *pPattern = pColumn->get( n );
				int nIndex = pPatternList->index( pPattern );
				if ( nIndex > currentPatternNumber ) {
					currentPatternNumber = nIndex;
					currentPattern = pPattern;
				}
			}
			column = column + (*pColumns)[ipattern]->longest_pattern_length();
			// WARNINGLOG( "Undoing lookahead: corrected (" + to_string( ipattern+1 ) +
			// "," + to_string( (int) ( column - currentPattern->get_length() ) -
			// (int) lookaheadTicks ) + ") -> (" + to_string(ipattern) +
			// "," + to_string( (int) column - (int) lookaheadTicks ) + ")." );
		}
		column -= lookaheadTicks;
		// Convert from playlist index to actual pattern index (if not already done above)
		if ( currentPattern == nullptr ) {
			std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
			PatternList *pColumn = ( *pColumns )[ ipattern ];
			currentPatternNumber = -1;
			for ( int n = 0; n < pColumn->size(); n++ ) {
				Pattern *pPattern = pColumn->get( n );
				int nIndex = pPatternList->index( pPattern );
				if ( nIndex > currentPatternNumber ) {
					currentPatternNumber = nIndex;
					currentPattern = pPattern;
				}
			}
		}

		// Cancel recording if punch area disagrees
		doRecord = pPreferences->inPunchArea( ipattern );

	} else { // Not song-record mode
		PatternList *pPatternList = pSong->getPatternList();

		int selectedPatternNumber = pAudioEngine->getSelectedPatternNumber();
		if ( ( selectedPatternNumber != -1 )
			 && ( selectedPatternNumber < ( int )pPatternList->size() ) )
		{
			currentPattern = pPatternList->get( selectedPatternNumber );
			currentPatternNumber = selectedPatternNumber;
		}

		if ( ! currentPattern ) {
			pAudioEngine->unlock(); // unlock the audio engine
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

	if ( currentPattern && pPreferences->getQuantizeEvents() ) {
		// quantize it to scale
		unsigned qcolumn = ( unsigned )::round( column / ( double )scalar ) * scalar; //TODO TupletDenominator??

		//we have to make sure that no beat is added on the last displayed note in a bar
		//for example: if the pattern has 4 beats, the editor displays 5 beats, so we should avoid adding beats an note 5.
		if ( qcolumn == currentPattern->get_length() ) qcolumn = 0;
		column = qcolumn;
	}

	unsigned position = column;
	pAudioEngine->setAddRealtimeNoteTickPosition( column );

	Instrument *instrRef = nullptr;
	if ( pSong ) {
		//getlookuptable index = instrument+36, ziel wert = der entprechende wert -36
		instrRef = pSong->getInstrumentList()->get( m_nInstrumentLookupTable[ instrument ] );
	}

	if ( currentPattern && ( getState() == STATE_PLAYING ) ) {
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
				instrRef = pSong->getInstrumentList()->get( getSelectedInstrumentNumber() );
				int divider = msg1 / 12;
				noteAction.m_row = getSelectedInstrumentNumber();
				noteAction.no_octaveKeyVal = (Note::Octave)(divider -3);
				noteAction.nk_noteKeyVal = (Note::Key)(msg1 - (12 * divider));
				noteAction.b_isInstrumentMode = true;
			} else {
				instrRef = pSong->getInstrumentList()->get( m_nInstrumentLookupTable[ instrument ] );
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
		}/* if doRecord */
	} else if ( pPreferences->getHearNewNotes() ) {
			hearnote = true;
	} /* if .. STATE_PLAYING */

	if ( !pPreferences->__playselectedinstrument ) {
		if ( hearnote && instrRef ) {
			Note *pNote2 = new Note( instrRef, nRealColumn, velocity, pan_L, pan_R, -1, 0 );
			midi_noteOn( pNote2 );
		}
	} else if ( hearnote  ) {
		Instrument* pInstr = pSong->getInstrumentList()->get( getSelectedInstrumentNumber() );
		Note *pNote2 = new Note( pInstr, nRealColumn, velocity, pan_L, pan_R, -1, 0 );

		int divider = msg1 / 12;
		Note::Octave octave = (Note::Octave)(divider -3);
		Note::Key notehigh = (Note::Key)(msg1 - (12 * divider));

		//ERRORLOG( QString( "octave: %1, note: %2, instrument %3" ).arg( octave ).arg(notehigh).arg(instrument));
		pNote2->set_midi_info( notehigh, octave, msg1 );
		midi_noteOn( pNote2 );
	}

	m_pAudioEngine->unlock(); // unlock the audio engine
}

unsigned long Hydrogen::getTickPosition()
{
	return m_pAudioEngine->getPatternTickPosition();
}

unsigned long Hydrogen::getRealtimeTickPosition()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	// Get the realtime transport position in frames and convert
	// it into ticks.
	unsigned int initTick = ( unsigned int )( getRealtimeFrames() /
						  pAudioEngine->getAudioDriver()->m_transport.m_fTickSize );
	unsigned long retTick;

	struct timeval currtime;
	struct timeval deltatime;

	double sampleRate = ( double ) pAudioEngine->getAudioDriver()->getSampleRate();
	gettimeofday ( &currtime, nullptr );

	// Definition macro from timehelper.h calculating the time
	// difference between `currtime` and `m_currentTickTime`
	// (`currtime`-`m_currentTickTime`) and storing the results in
	// `deltatime`. It uses both the .tv_sec (seconds) and
	// .tv_usec (microseconds) members of the timeval struct.
	timersub( &currtime, &pAudioEngine->getCurrentTickTime(), &deltatime );

	// add a buffers worth for jitter resistance
	double deltaSec =
			( double ) deltatime.tv_sec
			+ ( deltatime.tv_usec / 1000000.0 )
			+ ( pAudioEngine->getAudioDriver()->getBufferSize() / ( double )sampleRate );

	retTick = ( unsigned long ) ( ( sampleRate / ( double ) pAudioEngine->getAudioDriver()->m_transport.m_fTickSize ) * deltaSec );

	retTick += initTick;

	return retTick;
}

void Hydrogen::sequencer_setNextPattern( int pos )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	pAudioEngine->lock( RIGHT_HERE );
	Song* pSong = getSong();
	if ( pSong && pSong->getMode() == Song::PATTERN_MODE ) {
		PatternList* pPatternList = pSong->getPatternList();
		
		// Check whether `pos` is in range of the pattern list.
		if ( ( pos >= 0 ) && ( pos < ( int )pPatternList->size() ) ) {
			Pattern* pPattern = pPatternList->get( pos );
			
			// If the pattern is already in the `AudioEngine::m_pNextPatterns`, it
			// will be removed from the latter and its `del()` method
			// will return a pointer to the very pattern. The if
			// clause is therefore only entered if the `pPattern` was
			// not already present.
			if ( pAudioEngine->getNextPatterns()->del( pPattern ) == nullptr ) {
				pAudioEngine->getNextPatterns()->add( pPattern );
			}
		} else {
			ERRORLOG( QString( "pos not in patternList range. pos=%1 patternListSize=%2" )
					  .arg( pos ).arg( pPatternList->size() ) );
			pAudioEngine->getNextPatterns()->clear();
		}
	} else {
		ERRORLOG( "can't set next pattern in song mode" );
		pAudioEngine->getNextPatterns()->clear();
	}

	pAudioEngine->unlock();
}

void Hydrogen::sequencer_setOnlyNextPattern( int pos )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;	
	pAudioEngine->lock( RIGHT_HERE );
	
	Song* pSong = getSong();
	if ( pSong && pSong->getMode() == Song::PATTERN_MODE ) {
		PatternList* pPatternList = pSong->getPatternList();
		
		// Clear the list of all patterns scheduled to be processed
		// next and fill them with those currently played.
		pAudioEngine->getNextPatterns()->clear( );
		Pattern* pPattern;
		for ( int nPattern = 0 ; nPattern < (int) pAudioEngine->getPlayingPatterns()->size() ; ++nPattern ) {
			pPattern = pAudioEngine->getPlayingPatterns()->get( nPattern );
			pAudioEngine->getNextPatterns()->add( pPattern );
		}
		
		// Appending the requested pattern.
		pPattern = pPatternList->get( pos );
		pAudioEngine->getNextPatterns()->add( pPattern );
	} else {
		ERRORLOG( "can't set next pattern in song mode" );
		pAudioEngine->getNextPatterns()->clear();
	}
	
	pAudioEngine->unlock();
}

// TODO: make variable name and getter/setter consistent.
int Hydrogen::getPatternPos()
{
	return m_pAudioEngine->getSongPos();
}

/* Return pattern for selected song tick position */
int Hydrogen::getPosForTick( unsigned long TickPos, int* nPatternStartTick )
{
	Song* pSong = getSong();
	if ( pSong == nullptr ) {
		return 0;
	}

	return m_pAudioEngine->findPatternInTick( TickPos, pSong->getIsLoopEnabled(), nPatternStartTick );
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
	m_pAudioEngine->restartAudioDrivers();
}

void Hydrogen::startExportSession(int sampleRate, int sampleDepth )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( getState() == STATE_PLAYING ) {
		sequencer_stop();
	}
	
	unsigned nSamplerate = (unsigned) sampleRate;
	
	pAudioEngine->getSampler()->stopPlayingNotes();

	Song* pSong = getSong();
	
	m_oldEngineMode = pSong->getMode();
	m_bOldLoopEnabled = pSong->getIsLoopEnabled();

	pSong->setMode( Song::SONG_MODE );
	pSong->setIsLoopEnabled( true );
	
	/*
	 * Currently an audio driver is loaded
	 * which is not the DiskWriter driver.
	 * Stop the current driver and fire up the DiskWriter.
	 */
	pAudioEngine->stopAudioDrivers();

	pAudioEngine->setAudioDriver( new DiskWriterDriver( AudioEngine::audioEngine_process, nSamplerate, sampleDepth ) );
	
	m_bExportSessionIsActive = true;
}

void Hydrogen::stopExportSession()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	m_bExportSessionIsActive = false;
	
 	pAudioEngine->stopAudioDrivers();
	
	delete pAudioEngine->getAudioDriver();
	pAudioEngine->setAudioDriver( nullptr );
	
	Song* pSong = getSong();
	pSong->setMode( m_oldEngineMode );
	pSong->setIsLoopEnabled( m_bOldLoopEnabled );
	
	pAudioEngine->startAudioDrivers();

	if ( pAudioEngine->getAudioDriver() ) {
		pAudioEngine->getAudioDriver()->setBpm( pSong->getBpm() );
	} else {
		ERRORLOG( "pAudioEngine->getAudioDriver() = nullptr" );
	}
}

/// Export a song to a wav file
void Hydrogen::startExportSong( const QString& filename)
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	// reset
	pAudioEngine->getAudioDriver()->m_transport.m_nFrames = 0; // reset total frames
	// TODO: not -1 instead?
	pAudioEngine->setSongPos( 0 );
	pAudioEngine->setPatternTickPosition( 0 );
	pAudioEngine->setState( STATE_PLAYING );
	pAudioEngine->setPatternStartTick( -1 );

	Preferences *pPref = Preferences::get_instance();

	int res = pAudioEngine->getAudioDriver()->init( pPref->m_nBufferSize );
	if ( res != 0 ) {
		ERRORLOG( "Error starting disk writer driver [DiskWriterDriver::init()]" );
	}

	pAudioEngine->setMainBuffer_L( pAudioEngine->getAudioDriver()->getOut_L() );
	pAudioEngine->setMainBuffer_R( pAudioEngine->getAudioDriver()->getOut_R() );

	pAudioEngine->setupLadspaFX( pAudioEngine->getAudioDriver()->getBufferSize() );

	pAudioEngine->seek( 0, false );

	DiskWriterDriver* pDiskWriterDriver = (DiskWriterDriver*) pAudioEngine->getAudioDriver();
	pDiskWriterDriver->setFileName( filename );
	
	res = pDiskWriterDriver->connect();
	if ( res != 0 ) {
		ERRORLOG( "Error starting disk writer driver [DiskWriterDriver::connect()]" );
	}
}

void Hydrogen::stopExportSong()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( pAudioEngine->getAudioDriver()->class_name() != DiskWriterDriver::class_name() ) {
		return;
	}

	pAudioEngine->getSampler()->stopPlayingNotes();
	
	pAudioEngine->getAudioDriver()->disconnect();

	pAudioEngine->setSongPos( -1 );
	pAudioEngine->setPatternTickPosition( 0 );
}

/// Used to display audio driver info
AudioOutput* Hydrogen::getAudioOutput() const
{
	AudioEngine* pAudioEngine = m_pAudioEngine;

	return pAudioEngine->getAudioDriver();
}

/// Used to display midi driver info
MidiInput* Hydrogen::getMidiInput() const 
{
	return m_pAudioEngine->getMidiDriver();
}

MidiOutput* Hydrogen::getMidiOutput() const
{
	return m_pAudioEngine->getMidiOutDriver();
}

int Hydrogen::getState() const
{
	return m_pAudioEngine->getState();
}

void Hydrogen::setCurrentPatternList( PatternList *pPatternList )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	pAudioEngine->lock( RIGHT_HERE );
	if ( pAudioEngine->getPlayingPatterns() ) {
		pAudioEngine->getPlayingPatterns()->setNeedsLock( false );
	}

	(*pAudioEngine->getPlayingPatterns()) = pPatternList;
	pPatternList->setNeedsLock( true );
	
	EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	
	pAudioEngine->unlock();
}

// Setting conditional to true will keep instruments that have notes if new kit has less instruments than the old one
int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo )
{
	return loadDrumkit( pDrumkitInfo, true );
}

int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo, bool conditional )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	assert ( pDrumkitInfo );

	int old_ae_state = getState();
	if( getState() >= STATE_READY ) {
		pAudioEngine->setState( STATE_PREPARED );
	}

	INFOLOG( pDrumkitInfo->get_name() );
	m_sCurrentDrumkitName = pDrumkitInfo->get_name();
	if ( pDrumkitInfo->isUserDrumkit() ) {
		m_currentDrumkitLookup = Filesystem::Lookup::user;
	} else {
		m_currentDrumkitLookup = Filesystem::Lookup::system;
	}

	std::vector<DrumkitComponent*>* pSongCompoList= getSong()->getComponents();
	std::vector<DrumkitComponent*>* pDrumkitCompoList = pDrumkitInfo->get_components();
	
	pAudioEngine->lock( RIGHT_HERE );	
	for( auto &pComponent : *pSongCompoList ){
		delete pComponent;
	}
	pSongCompoList->clear();
	pAudioEngine->unlock();
	
	for (std::vector<DrumkitComponent*>::iterator it = pDrumkitCompoList->begin() ; it != pDrumkitCompoList->end(); ++it) {
		DrumkitComponent* pSrcComponent = *it;
		DrumkitComponent* pNewComponent = new DrumkitComponent( pSrcComponent->get_id(), pSrcComponent->get_name() );
		pNewComponent->load_from( pSrcComponent );

		pSongCompoList->push_back( pNewComponent );
	}

	//current instrument list
	InstrumentList *pSongInstrList = getSong()->getInstrumentList();
	
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
	int nMaxID = -1;
	
	for ( unsigned nInstr = 0; nInstr < pDrumkitInstrList->size(); ++nInstr ) {
		Instrument *pInstr = nullptr;
		if ( nInstr < pSongInstrList->size() ) {
			//instrument exists already
			pInstr = pSongInstrList->get( nInstr );
			assert( pInstr );
		} else {
			pInstr = new Instrument();
			// The instrument isn't playing yet; no need for locking
			// :-) - Jakob Lund.  m_pAudioEngine->lock(
			// "Hydrogen::loadDrumkit" );
			pSongInstrList->add( pInstr );
			// m_pAudioEngine->unlock();
		}

		Instrument *pNewInstr = pDrumkitInstrList->get( nInstr );
		assert( pNewInstr );
		INFOLOG( QString( "Loading instrument (%1 of %2) [%3]" )
				 .arg( nInstr + 1 )
				 .arg( pDrumkitInstrList->size() )
				 .arg( pNewInstr->get_name() ) );

		// Preserve instrument IDs. Where the new drumkit has more instruments than the song does, new
		// instruments need new ids.
		int nID = pInstr->get_id();
		if ( nID == EMPTY_INSTR_ID ) {
			nID = nMaxID + 1;
		}
		nMaxID = std::max( nID, nMaxID );

		// Moved code from here right into the Instrument class - Jakob Lund.
		pInstr->load_from( pDrumkitInfo, pNewInstr );
		pInstr->set_id( nID );
	}

	//wolke: new delete function
	if ( instrumentDiff >= 0 ) {
		for ( int i = 0; i < instrumentDiff ; i++ ){
			removeInstrument(
						getSong()->getInstrumentList()->size() - 1,
						conditional
						);
		}
	}

#ifdef H2CORE_HAVE_JACK
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->renameJackPorts( getSong() );
	pAudioEngine->unlock();
#endif

	pAudioEngine->setState( old_ae_state );
	
	m_pCoreActionController->initExternalControlInterfaces();
	
	// Create a symbolic link in the session folder when under session
	// management.
	if ( isUnderSessionManagement() ) {
#ifdef H2CORE_HAVE_OSC
		NsmClient::linkDrumkit( NsmClient::get_instance()->m_sSessionFolderPath.toLocal8Bit().data(), false );
#endif
	}

	return 0;	//ok
}

// This will check if an instrument has any notes
bool Hydrogen::instrumentHasNotes( Instrument *pInst )
{
	Song* pSong = getSong();
	PatternList* pPatternList = pSong->getPatternList();

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
	Instrument *pInstr = pSong->getInstrumentList()->get( instrumentNumber );
	PatternList* pPatternList = pSong->getPatternList();

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
		getSong()->purgeInstrument( pInstr );
	}

	InstrumentList* pList = pSong->getInstrumentList();
	if ( pList->size()==1 ){
		m_pAudioEngine->lock( RIGHT_HERE );
		Instrument* pInstr = pList->get( 0 );
		pInstr->set_name( (QString( "Instrument 1" )) );
		for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			InstrumentComponent* pCompo = *it;
			// remove all layers
			for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
				pCompo->set_layer( nullptr, nLayer );
			}
		}
		m_pAudioEngine->unlock();
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
		INFOLOG("clear last instrument to empty instrument 1 instead delete the last instrument");
		return;
	}

	// if the instrument was the last on the instruments list, select the
	// next-last
	if ( instrumentNumber >= (int)getSong()->getInstrumentList()->size() - 1 ) {
		Hydrogen::get_instance()->setSelectedInstrumentNumber(
					std::max(0, instrumentNumber - 1 )
					);
	}
	//
	// delete the instrument from the instruments list
	m_pAudioEngine->lock( RIGHT_HERE );
	getSong()->getInstrumentList()->del( instrumentNumber );
	getSong()->setIsModified( true );
	m_pAudioEngine->unlock();

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
	m_pAudioEngine->raiseError( nErrorCode );
}

unsigned long Hydrogen::getTotalFrames()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	return pAudioEngine->getAudioDriver()->m_transport.m_nFrames;
}

void Hydrogen::setRealtimeFrames( unsigned long frames )
{
	m_pAudioEngine->setRealtimeFrames( frames );
}

unsigned long Hydrogen::getRealtimeFrames()
{
	return m_pAudioEngine->getRealtimeFrames();
}


long Hydrogen::getTickForPosition( int pos )
{
	Song* pSong = getSong();	

	int nPatternGroups = pSong->getPatternGroupVector()->size();
	if ( nPatternGroups == 0 ) {
		return -1;
	}

	if ( pos >= nPatternGroups ) {
		// The position is beyond the end of the Song, we
		// set periodic boundary conditions or return the
		// beginning of the Song as a fallback.
		if ( pSong->getIsLoopEnabled() ) {
			pos = pos % nPatternGroups;
		} else {
			WARNINGLOG( QString( "patternPos > nPatternGroups. pos:"
								 " %1, nPatternGroups: %2")
						.arg( pos ) .arg(  nPatternGroups )
						);
			return -1;
		}
	}

	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
	long totalTick = 0;
	int nPatternSize;
	Pattern *pPattern = nullptr;
	
	for ( int i = 0; i < pos; ++i ) {
		PatternList *pColumn = ( *pColumns )[ i ];
		
		if( pColumn->size() > 0)
		{
			nPatternSize = pColumn->longest_pattern_length();
		} else {
			nPatternSize = MAX_NOTES;
		}
		totalTick += nPatternSize;
	}
	
	return totalTick;
}

void Hydrogen::setPatternPos( int nPatternNumber )
{
	if ( nPatternNumber < -1 ) {
		nPatternNumber = -1;
	}
	
	auto pAudioEngine = m_pAudioEngine;
	
	pAudioEngine->lock( RIGHT_HERE );
	// TODO: why?
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
	long totalTick = getTickForPosition( nPatternNumber );
	if ( totalTick < 0 ) {
		pAudioEngine->unlock();
		return;
	}

	if ( getState() != STATE_PLAYING ) {
		// find pattern immediately when not playing
		//		int dummy;
		// 		m_nSongPos = findPatternInTick( totalTick,
		//					        pSong->getIsLoopEnabled(),
		//					        &dummy );
		pAudioEngine->setSongPos( nPatternNumber );
		pAudioEngine->setPatternTickPosition( 0 );
	}
	INFOLOG( "relocate" );

	pAudioEngine->locate( static_cast<int>( totalTick * pAudioEngine->getAudioDriver()->m_transport.m_fTickSize ));

	pAudioEngine->unlock();
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

	m_pAudioEngine->lock( RIGHT_HERE );

	setBPM( fBPM );

	m_pAudioEngine->unlock();
}

void Hydrogen::setBPM( float fBPM )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;	
	Song* pSong = getSong();
	if ( ! pAudioEngine->getAudioDriver() || ! pSong ){
		return;
	}
	
	if ( fBPM > MAX_BPM ) {
		fBPM = MAX_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
					.arg( fBPM ).arg( MAX_BPM ) );
	} else if ( fBPM < MIN_BPM ) {
		fBPM = MIN_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
					.arg( fBPM ).arg( MIN_BPM ) );
	}

	if ( getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
		ERRORLOG( "Unable to change tempo directly in the presence of an external JACK timebase master. Press 'J.MASTER' get tempo control." );
		return;
	}
	
	pAudioEngine->getAudioDriver()->setBpm( fBPM );
	pSong->setBpm( fBPM );
	setNewBpmJTM ( fBPM );
}

void Hydrogen::restartLadspaFX()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( pAudioEngine->getAudioDriver() ) {
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setupLadspaFX( pAudioEngine->getAudioDriver()->getBufferSize() );
		pAudioEngine->unlock();
	} else {
		ERRORLOG( "m_pAudioDriver = NULL" );
	}
}

int Hydrogen::getSelectedPatternNumber()
{
	return m_pAudioEngine->getSelectedPatternNumber();
}


void Hydrogen::setSelectedPatternNumber( int nPat )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;	
	
	if ( nPat == pAudioEngine->getSelectedPatternNumber() )	return;


	if ( Preferences::get_instance()->patternModePlaysSelected() ) {
		pAudioEngine->lock( RIGHT_HERE );
		
		pAudioEngine->setSelectedPatternNumber( nPat );

		pAudioEngine->unlock();
	} else {
		pAudioEngine->setSelectedPatternNumber( nPat );
	}

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}

int Hydrogen::getSelectedInstrumentNumber()
{
	return m_nSelectedInstrumentNumber;
}

void Hydrogen::setSelectedInstrumentNumber( int nInstrument )
{
	if ( m_nSelectedInstrumentNumber == nInstrument ) {
		return;
	}

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
		m_pAudioEngine->renameJackPorts(pSong);
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
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	// Get first time value:
	if (m_nBeatCount == 1) {
		gettimeofday(&m_CurrentTime,nullptr);
	}

	m_nEventCount++;

	// Set lastTime to m_CurrentTime to remind the time:
	timeval lastTime = m_CurrentTime;

	// Get new time:
	gettimeofday(&m_CurrentTime,nullptr);


	// Build doubled time difference:
	double lastBeatTime = (double)(
				lastTime.tv_sec
				+ (double)(lastTime.tv_usec * US_DIVIDER)
				+ (int)m_nCoutOffset * .0001
				);
	double currentBeatTime = (double)(
				m_CurrentTime.tv_sec
				+ (double)(m_CurrentTime.tv_usec * US_DIVIDER)
				);
	double beatDiff = m_nBeatCount == 1 ? 0 : currentBeatTime - lastBeatTime;

	//if differences are to big reset the beatconter
	if( beatDiff > 3.001 * 1/m_ntaktoMeterCompute ) {
		m_nEventCount = 1;
		m_nBeatCount = 1;
		return;
	}
	// Only accept differences big enough
	if (m_nBeatCount == 1 || beatDiff > .001) {
		if (m_nBeatCount > 1) {
			m_nBeatDiffs[m_nBeatCount - 2] = beatDiff ;
		}
		// Compute and reset:
		if (m_nBeatCount == m_nbeatsToCount){
			//				unsigned long currentframe = getRealtimeFrames();
			double beatTotalDiffs = 0;
			for(int i = 0; i < (m_nbeatsToCount - 1); i++) {
				beatTotalDiffs += m_nBeatDiffs[i];
			}
			double nBeatDiffAverage =
					beatTotalDiffs
					/ (m_nBeatCount - 1)
					* m_ntaktoMeterCompute ;
			float fBeatCountBpm	 =
					(float) ((int) (60 / nBeatDiffAverage * 100))
					/ 100;
			
			m_pAudioEngine->lock( RIGHT_HERE );
			setBPM( fBeatCountBpm );
			m_pAudioEngine->unlock();
			
			if (Preferences::get_instance()->m_mmcsetplay
					== Preferences::SET_PLAY_OFF) {
				m_nBeatCount = 1;
				m_nEventCount = 1;
			}else{
				if ( getState() != STATE_PLAYING ){
					unsigned bcsamplerate =
							pAudioEngine->getAudioDriver()->getSampleRate();
					unsigned long rtstartframe = 0;
					if ( m_ntaktoMeterCompute <= 1){
						rtstartframe =
								bcsamplerate
								* nBeatDiffAverage
								* ( 1/ m_ntaktoMeterCompute );
					}else
					{
						rtstartframe =
								bcsamplerate
								* nBeatDiffAverage
								/ m_ntaktoMeterCompute ;
					}

					int sleeptime =
							( (float) rtstartframe
							  / (float) bcsamplerate
							  * (int) 1000 )
							+ (int)m_nCoutOffset
							+ (int) m_nStartOffset;
					
					std::this_thread::sleep_for( std::chrono::milliseconds( sleeptime ) );

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
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( haveJackTransport() ) {
		static_cast< JackAudioDriver* >( pAudioEngine->getAudioDriver() )->releaseTimebaseMaster();
	}
}

void Hydrogen::onJackMaster()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( haveJackTransport() ) {
		static_cast< JackAudioDriver* >( pAudioEngine->getAudioDriver() )->initTimebaseMaster();
	}
}
#endif

long Hydrogen::getPatternLength( int nPattern )
{
	Song* pSong = getSong();
	if ( pSong == nullptr ){
		return -1;
	}

	std::vector< PatternList* > *pColumns = pSong->getPatternGroupVector();

	int nPatternGroups = pColumns->size();
	if ( nPattern >= nPatternGroups ) {
		if ( pSong->getIsLoopEnabled() ) {
			nPattern = nPattern % nPatternGroups;
		} else {
			return MAX_NOTES;
		}
	}

	if ( nPattern < 1 ){
		return MAX_NOTES;
	}

	PatternList* pPatternList = pColumns->at( nPattern - 1 );
	if ( pPatternList->size() > 0 ) {
		return pPatternList->longest_pattern_length();
	} else {
		return MAX_NOTES;
	}
}

float Hydrogen::getNewBpmJTM() const
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
	AudioEngine* pAudioEngine = m_pAudioEngine;	
	
	// This forces the barline position
	if ( getSong()->getMode() == Song::PATTERN_MODE ) {
		pAudioEngine->setPatternStartTick( -1 );
	}
}

void Hydrogen::togglePlaysSelected()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;	
	Song* pSong = getSong();

	if ( pSong->getMode() != Song::PATTERN_MODE ) {
		return;
	}

	pAudioEngine->lock( RIGHT_HERE );

	Preferences* pPref = Preferences::get_instance();
	bool isPlaysSelected = pPref->patternModePlaysSelected();

	if (isPlaysSelected) {
		pAudioEngine->getPlayingPatterns()->clear();
		Pattern* pSelectedPattern =
				pSong->getPatternList()->get(pAudioEngine->getSelectedPatternNumber());
		pAudioEngine->getPlayingPatterns()->add( pSelectedPattern );
	}

	pPref->setPatternModePlaysSelected( !isPlaysSelected );
	pAudioEngine->unlock();
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
	m_pAudioEngine->getSampler()->stopPlayingNotes();
}

float Hydrogen::getTimelineBpm( int nBar )
{
	Song* pSong = getSong();

	// We need return something
	if ( pSong == nullptr ) {
		return getNewBpmJTM();
	}

	float fBPM = pSong->getBpm();

	// Pattern mode don't use timeline and will have a constant
	// speed.
	if ( pSong->getMode() == Song::PATTERN_MODE ) {
		return fBPM;
	}

	// Check whether the user wants Hydrogen to determine the
	// speed by local setting along the timeline or whether she
	// wants to use a global speed instead.
	if ( ! Preferences::get_instance()->getUseTimelineBpm() ) {
		return fBPM;
	}

	// Determine the speed at the supplied beat.
	float fTimelineBpm = m_pTimeline->getTempoAtBar( nBar, true );
	if ( fTimelineBpm != 0 ) {
		/* TODO: For now the function returns 0 if the bar is
		 * positioned _before_ the first tempo marker. This will be
		 * taken care of with #854. */
		fBPM = fTimelineBpm;
	}

	return fBPM;
}

void Hydrogen::setTimelineBpm()
{
	if ( ! Preferences::get_instance()->getUseTimelineBpm() ||
		 getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
		return;
	}

	Song* pSong = getSong();
	// Obtain the local speed specified for the current Pattern.
	float fBPM = getTimelineBpm( getPatternPos() );

	if ( fBPM != pSong->getBpm() ) {
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
	AudioEngine* pAudioEngine = m_pAudioEngine;
	if ( pAudioEngine->getAudioDriver() != nullptr ) {
		if ( JackAudioDriver::class_name() == pAudioEngine->getAudioDriver()->class_name() ){
			return true;
		}
	}
	return false;
#else
	return false;
#endif	
}

bool Hydrogen::haveJackTransport() const {
#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	if ( pAudioEngine->getAudioDriver() != nullptr ) {
		if ( JackAudioDriver::class_name() == pAudioEngine->getAudioDriver()->class_name() &&
			 Preferences::get_instance()->m_bJackTransportMode ==
			 Preferences::USE_JACK_TRANSPORT ){
			return true;
		}
	}
	return false;
#else
	return false;
#endif	
}

JackAudioDriver::Timebase Hydrogen::getJackTimebaseState() const {
#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	if ( haveJackTransport() ) {
		return static_cast<JackAudioDriver*>(pAudioEngine->getAudioDriver())->getTimebaseState();
	} 
	return JackAudioDriver::Timebase::None;
#else
	return JackAudioDriver::Timebase::None;
#endif	
}

bool Hydrogen::isUnderSessionManagement() const {
#ifdef H2CORE_HAVE_OSC
	if ( NsmClient::get_instance() != nullptr ) {
		if ( NsmClient::get_instance()->getUnderSessionManagement() ) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
#else
	return false;
#endif
}		

void Hydrogen::toggleOscServer( bool bEnable ) {
#ifdef H2CORE_HAVE_OSC
	if ( bEnable ) {
		OscServer::get_instance()->start();
	} else {
		OscServer::get_instance()->stop();
	}
#endif
}

void Hydrogen::recreateOscServer() {
#ifdef H2CORE_HAVE_OSC
	OscServer* pOscServer = OscServer::get_instance();
	if( pOscServer ) {
		delete pOscServer;
	}

	OscServer::create_instance( Preferences::get_instance() );
	
	if ( Preferences::get_instance()->getOscServerEnabled() ) {
		toggleOscServer( true );
	}
#endif
}

void Hydrogen::startNsmClient()
{
#ifdef H2CORE_HAVE_OSC
	//NSM has to be started before jack driver gets created
	NsmClient* pNsmClient = NsmClient::get_instance();

	if(pNsmClient){
		pNsmClient->createInitialClient();
	}
#endif
}

void Hydrogen::setInitialSong( Song *pSong )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;

	// Since the function is only intended to set a Song prior to the
	// initial creation of the audio driver, it will cause the
	// application to get out of sync if used elsewhere. The following
	// checks ensure it is called in the right context.
	if ( pSong == nullptr ) {
		return;
	}
	if ( __song != nullptr ) {
		return;
	}
	if ( pAudioEngine->getAudioDriver() != nullptr ) {
		return;
	}
	
	// Just to be sure.
	pAudioEngine->lock( RIGHT_HERE );

	// Find the first pattern and set as current.
	if ( pSong->getPatternList()->size() > 0 ) {
		m_pAudioEngine->getPlayingPatterns()->add( pSong->getPatternList()->get( 0 ) );
	}

	pAudioEngine->unlock();

	// Move to the beginning.
	setSelectedPatternNumber( 0 );

	__song = pSong;

	// Push current state of Hydrogen to attached control interfaces,
	// like OSC clients.
	m_pCoreActionController->initExternalControlInterfaces();
}

}; /* Namespace */
