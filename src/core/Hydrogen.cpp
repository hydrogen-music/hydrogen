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
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportInfo.h>
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

#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>
#include "MidiMap.h"

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

Hydrogen::Hydrogen() : m_nSelectedInstrumentNumber( 0 )
					 , m_nSelectedPatternNumber( 0 )
					 , m_bExportSessionIsActive( false )
					 , m_GUIState( GUIState::unavailable )
{
	if ( __instance ) {
		ERRORLOG( "Hydrogen audio engine is already running" );
		throw H2Exception( "Hydrogen audio engine is already running" );
	}

	INFOLOG( "[Hydrogen]" );

	__song = nullptr;

	m_pTimeline = std::make_shared<Timeline>();
	m_pCoreActionController = new CoreActionController();

	initBeatcounter();
	InstrumentComponent::setMaxLayers( Preferences::get_instance()->getMaxLayers() );
	
	m_pAudioEngine = new AudioEngine();
	Playlist::create_instance();

	EventQueue::get_instance()->push_event( EVENT_STATE, static_cast<int>(AudioEngine::State::Initialized) );

	// Prevent double creation caused by calls from MIDI thread
	__instance = this;

	m_pAudioEngine->startAudioDrivers();
	
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
	
	removeSong();
	
	__kill_instruments();

	delete m_pCoreActionController;
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
	std::shared_ptr<Song> pSong = getSong();
	pSong->getPatternList()->set_to_old();
	m_pAudioEngine->play();
}

/// Stop the internal sequencer
void Hydrogen::sequencer_stop()
{
	if( Hydrogen::get_instance()->getMidiOutput() != nullptr ){
		Hydrogen::get_instance()->getMidiOutput()->handleQueueAllNoteOff();
	}

	m_pAudioEngine->stop();
	Preferences::get_instance()->setRecordEvents(false);
}

bool Hydrogen::setPlaybackTrackState( const bool state )
{
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	return pSong->setPlaybackTrackEnabled(state);
}

void Hydrogen::loadPlaybackTrack( const QString filename )
{
	std::shared_ptr<Song> pSong = getSong();
	pSong->setPlaybackTrackFilename(filename);

	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();
}

void Hydrogen::setSong( std::shared_ptr<Song> pSong )
{
	assert ( pSong );
	
	// Move to the beginning.
	setSelectedPatternNumber( 0 );

	std::shared_ptr<Song> pCurrentSong = getSong();
	if ( pSong == pCurrentSong ) {
		DEBUGLOG( "pSong == pCurrentSong" );
		return;
	}

	if ( pCurrentSong != nullptr ) {
		/* NOTE: 
		 *       - this is actually some kind of cleanup 
		 *       - removeSong cares itself for acquiring a lock
		 */
		
		if ( isUnderSessionManagement() ) {
			// When under session management Hydrogen is only allowed
			// to replace the content of the session song but not to
			// write to a different location.
			pSong->setFilename( pCurrentSong->getFilename() );
		}
		removeSong();
		// delete pCurrentSong;
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
		NsmClient::linkDrumkit( NsmClient::get_instance()->m_sSessionFolderPath, true );
#endif
	} else {		
		Preferences::get_instance()->setLastSongFilename( pSong->getFilename() );
	}
	
	EventQueue::get_instance()->push_event( EVENT_SONG_MODE_ACTIVATION,
											( pSong->getMode() == Song::Mode::Song) ? 1 : 0 );
	EventQueue::get_instance()->push_event( EVENT_TIMELINE_ACTIVATION,
											static_cast<int>( pSong->getIsTimelineActivated() ) );
}

/* Mean: remove current song from memory */
void Hydrogen::removeSong()
{
	m_pAudioEngine->removeSong();
	__song = nullptr;
}

void Hydrogen::midi_noteOn( Note *note )
{
	m_pAudioEngine->noteOn( note );
}

void Hydrogen::addRealtimeNote(	int		instrument,
								float	velocity,
								float	fPan,
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
	int nBase = pPreferences->isPatternEditorUsingTriplets() ? 3 : 4;
	int scalar = ( 4 * MAX_NOTES ) / ( res * nBase );
	bool hearnote = forcePlay;
	int currentPatternNumber;

	m_pAudioEngine->lock( RIGHT_HERE );

	std::shared_ptr<Song> pSong = getSong();
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
	float fTickSize = pAudioEngine->getTickSize();
	unsigned int lookaheadTicks = pAudioEngine->calculateLookahead( fTickSize ) / fTickSize;
	bool doRecord = pPreferences->getRecordEvents();
	if ( getMode() == Song::Mode::Song && doRecord &&
		 pAudioEngine->getState() == AudioEngine::State::Playing )
	{

		// Recording + song playback mode + actually playing
		PatternList *pPatternList = pSong->getPatternList();
		int ipattern = pAudioEngine->getColumn(); // current column
												   // or pattern group
		if ( ipattern < 0 || ipattern >= (int) pPatternList->size() ) {
			pAudioEngine->unlock(); // unlock the audio engine
			return;
		}
		// Locate column -- may need to jump back in the pattern list
		column = pAudioEngine->getPatternTickPosition();
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

		if ( ( m_nSelectedPatternNumber != -1 )
			 && ( m_nSelectedPatternNumber < ( int )pPatternList->size() ) )
		{
			currentPattern = pPatternList->get( m_nSelectedPatternNumber );
			currentPatternNumber = m_nSelectedPatternNumber;
		}

		if ( ! currentPattern ) {
			pAudioEngine->unlock(); // unlock the audio engine
			return;
		}

		// Locate column -- may need to wrap around end of pattern
		column = pAudioEngine->getPatternTickPosition();
		if ( column >= lookaheadTicks ) {
			column -= lookaheadTicks;
		} else {
			lookaheadTicks %= currentPattern->get_length();
			column = (column + currentPattern->get_length() - lookaheadTicks)
					% currentPattern->get_length();
		}
	}

	if ( currentPattern && pPreferences->getQuantizeEvents() ) {
		// quantize it to scale
		unsigned qcolumn = ( unsigned )::round( column / ( double )scalar ) * scalar;

		//we have to make sure that no beat is added on the last displayed note in a bar
		//for example: if the pattern has 4 beats, the editor displays 5 beats, so we should avoid adding beats an note 5.
		if ( qcolumn == currentPattern->get_length() ) qcolumn = 0;
		column = qcolumn;
	}

	unsigned position = column;
	pAudioEngine->setAddRealtimeNoteTickPosition( column );

	std::shared_ptr<Instrument> instrRef = nullptr;
	if ( pSong ) {
		//getlookuptable index = instrument+36, ziel wert = der entprechende wert -36
		instrRef = pSong->getInstrumentList()->get( m_nInstrumentLookupTable[ instrument ] );
	}

	if ( currentPattern && ( pAudioEngine->getState() == AudioEngine::State::Playing ) ) {
		assert( currentPattern );
		if ( doRecord ) {
			EventQueue::AddMidiNoteVector noteAction;
			noteAction.m_column = column;
			noteAction.m_pattern = currentPatternNumber;
			noteAction.f_velocity = velocity;
			noteAction.f_pan = fPan;
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
			if ( pPreferences->getHearNewNotes() && position <= pAudioEngine->getPatternTickPosition() ) {
				hearnote = true;
			}
		}/* if doRecord */
	} else if ( pPreferences->getHearNewNotes() ) {
			hearnote = true;
	} /* if .. AudioEngine::State::Playing */

	if ( !pPreferences->__playselectedinstrument ) {
		if ( hearnote && instrRef ) {
			Note *pNote2 = new Note( instrRef, nRealColumn, velocity, fPan, -1, 0 );
			midi_noteOn( pNote2 );
		}
	} else if ( hearnote  ) {
		auto pInstr = pSong->getInstrumentList()->get( getSelectedInstrumentNumber() );
		Note *pNote2 = new Note( pInstr, nRealColumn, velocity, fPan, -1, 0 );

		int divider = msg1 / 12;
		Note::Octave octave = (Note::Octave)(divider -3);
		Note::Key notehigh = (Note::Key)(msg1 - (12 * divider));

		//ERRORLOG( QString( "octave: %1, note: %2, instrument %3" ).arg( octave ).arg(notehigh).arg(instrument));
		pNote2->set_midi_info( notehigh, octave, msg1 );
		midi_noteOn( pNote2 );
	}

	m_pAudioEngine->unlock(); // unlock the audio engine
}


void Hydrogen::sequencer_setNextPattern( int pos )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	pAudioEngine->lock( RIGHT_HERE );
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong != nullptr && getMode() == Song::Mode::Pattern ) {
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
	
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong != nullptr && getMode() == Song::Mode::Pattern ) {
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

void Hydrogen::restartDrivers()
{
	m_pAudioEngine->restartAudioDrivers();
}

bool Hydrogen::startExportSession(int sampleRate, int sampleDepth )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
		sequencer_stop();
	}
	
	unsigned nSamplerate = (unsigned) sampleRate;

	std::shared_ptr<Song> pSong = getSong();
	
	m_oldEngineMode = getMode();
	m_bOldLoopEnabled = pSong->getIsLoopEnabled();

	pSong->setMode( Song::Mode::Song );
	pSong->setIsLoopEnabled( true );
	
	/*
	 * Currently an audio driver is loaded
	 * which is not the DiskWriter driver.
	 * Stop the current driver and fire up the DiskWriter.
	 */
	pAudioEngine->stopAudioDrivers();

	DiskWriterDriver* pNewDriver = new DiskWriterDriver( AudioEngine::audioEngine_process, nSamplerate, sampleDepth );
	int nRes = pNewDriver->init( Preferences::get_instance()->m_nBufferSize );
	if ( nRes != 0 ) {
		ERRORLOG( "Unable to initialize disk writer driver." );
		return false;
	}
	m_bExportSessionIsActive = true;
	
	pAudioEngine->setAudioDriver( pNewDriver );
	pAudioEngine->setupLadspaFX();

	return true;
}

/// Export a song to a wav file
void Hydrogen::startExportSong( const QString& filename)
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	pAudioEngine->reset();
	pAudioEngine->play();
	getCoreActionController()->locateToFrame( 0 );
	pAudioEngine->getSampler()->stopPlayingNotes();

	DiskWriterDriver* pDiskWriterDriver = static_cast<DiskWriterDriver*>(pAudioEngine->getAudioDriver());
	pDiskWriterDriver->setFileName( filename );
	pDiskWriterDriver->write();
}

void Hydrogen::stopExportSong()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	pAudioEngine->getSampler()->stopPlayingNotes();
	pAudioEngine->reset();
}

void Hydrogen::stopExportSession()
{
	std::shared_ptr<Song> pSong = getSong();
	pSong->setMode( m_oldEngineMode );
	pSong->setIsLoopEnabled( m_bOldLoopEnabled );
	
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
 	pAudioEngine->stopAudioDrivers();
	
	pAudioEngine->startAudioDrivers();
	if ( pAudioEngine->getAudioDriver() == nullptr ) {
		ERRORLOG( "Unable to restart previous audio driver after exporting song." );
	}
	m_bExportSessionIsActive = false;
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

// Setting conditional to true will keep instruments that have notes if new kit has less instruments than the old one
int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo )
{
	return loadDrumkit( pDrumkitInfo, true );
}

int Hydrogen::loadDrumkit( Drumkit *pDrumkitInfo, bool conditional )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	assert ( pDrumkitInfo );

	AudioEngine::State oldAudioEngineState = pAudioEngine->getState();
	if( pAudioEngine->getState() == AudioEngine::State::Ready ||
		pAudioEngine->getState() == AudioEngine::State::Playing ) {
		pAudioEngine->setState( AudioEngine::State::Prepared );
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
		std::shared_ptr<Instrument> pInstr = nullptr;
		if ( nInstr < pSongInstrList->size() ) {
			//instrument exists already
			pInstr = pSongInstrList->get( nInstr );
			assert( pInstr );
		} else {
			pInstr = std::make_shared<Instrument>();
			// The instrument isn't playing yet; no need for locking
			// :-) - Jakob Lund.  m_pAudioEngine->lock(
			// "Hydrogen::loadDrumkit" );
			pSongInstrList->add( pInstr );
			// m_pAudioEngine->unlock();
		}

		auto pNewInstr = pDrumkitInstrList->get( nInstr );
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
	renameJackPorts( getSong() );
	pAudioEngine->unlock();
#endif

	setIsModified( true );

	pAudioEngine->setState( oldAudioEngineState );
	
	m_pCoreActionController->initExternalControlInterfaces();
	
	// Create a symbolic link in the session folder when under session
	// management.
	if ( isUnderSessionManagement() ) {
#ifdef H2CORE_HAVE_OSC
		NsmClient::linkDrumkit( NsmClient::get_instance()->m_sSessionFolderPath, false );
#endif
	}

	return 0;	//ok
}

// This will check if an instrument has any notes
bool Hydrogen::instrumentHasNotes( std::shared_ptr<Instrument> pInst )
{
	std::shared_ptr<Song> pSong = getSong();
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
	std::shared_ptr<Song> pSong = getSong();
	auto pInstr = pSong->getInstrumentList()->get( instrumentNumber );
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
		auto pInstr = pList->get( 0 );
		pInstr->set_name( (QString( "Instrument 1" )) );
		for ( auto& pCompo : *pInstr->get_components() ) {
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
		setSelectedInstrumentNumber( std::max(0, instrumentNumber - 1 ) );
	}
	//
	// delete the instrument from the instruments list
	m_pAudioEngine->lock( RIGHT_HERE );
	getSong()->getInstrumentList()->del( instrumentNumber );
	setIsModified( true );
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

	m_pAudioEngine->setNextBpm( fBPM );
	// Store it's value in the .h2song file.
	getSong()->setBpm( fBPM );
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );
}

void Hydrogen::restartLadspaFX()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( pAudioEngine->getAudioDriver() ) {
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setupLadspaFX();
		pAudioEngine->unlock();
	} else {
		ERRORLOG( "m_pAudioDriver = NULL" );
	}
}


void Hydrogen::setSelectedPatternNumber( int nPat )
{
	if ( nPat == m_nSelectedPatternNumber ) {
		return;
	}

	if ( Preferences::get_instance()->patternModePlaysSelected() ) {
		getAudioEngine()->lock( RIGHT_HERE );
		
		m_nSelectedPatternNumber = nPat;

		getAudioEngine()->unlock();
	} else {
		m_nSelectedPatternNumber = nPat;
	}

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
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
void Hydrogen::renameJackPorts( std::shared_ptr<Song> pSong )
{
	if ( pSong == nullptr ) {
		return;
	}
	
	if( Preferences::get_instance()->m_bJackTrackOuts == true ){
		if ( haveJackAudioDriver() && pSong != nullptr ) {

			// When restarting the audio driver after loading a new song under
			// Non session management all ports have to be registered _prior_
			// to the activation of the client.
			if ( isUnderSessionManagement() ) {
				return;
			}
			auto pAudioEngine = m_pAudioEngine;

			static_cast< JackAudioDriver* >( m_pAudioEngine->getAudioDriver() )->makeTrackOutputs( pSong );
		}
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
			
			m_pAudioEngine->setNextBpm( fBeatCountBpm );
			getSong()->setBpm( fBeatCountBpm );
	
			EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );
			
			if (Preferences::get_instance()->m_mmcsetplay
					== Preferences::SET_PLAY_OFF) {
				m_nBeatCount = 1;
				m_nEventCount = 1;
			} else {
				if ( pAudioEngine->getState() != AudioEngine::State::Playing ){
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

void Hydrogen::setPlaysSelected( bool bPlaysSelected )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;	
	std::shared_ptr<Song> pSong = getSong();

	if ( getMode() != Song::Mode::Pattern ) {
		return;
	}

	pAudioEngine->lock( RIGHT_HERE );

	Preferences* pPref = Preferences::get_instance();
	bool isPlaysSelected = pPref->patternModePlaysSelected();

	if ( isPlaysSelected && !bPlaysSelected ) {
		pAudioEngine->getPlayingPatterns()->clear();
		Pattern* pSelectedPattern =
				pSong->getPatternList()->get( getSelectedPatternNumber() );
		pAudioEngine->getPlayingPatterns()->add( pSelectedPattern );
	}

	pPref->setPatternModePlaysSelected( bPlaysSelected );
	pAudioEngine->unlock();
}

void Hydrogen::__kill_instruments()
{
	int c = 0;
	std::shared_ptr<Instrument> pInstr = nullptr;
	while ( __instrument_death_row.size()
			&& __instrument_death_row.front()->is_queued() == 0 ) {
		pInstr = __instrument_death_row.front();
		__instrument_death_row.pop_front();
		INFOLOG( QString( "Deleting unused instrument (%1). "
						  "%2 unused remain." )
				 . arg( pInstr->get_name() )
				 . arg( __instrument_death_row.size() ) );
		pInstr = nullptr;
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

bool Hydrogen::haveJackAudioDriver() const {
#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	if ( pAudioEngine->getAudioDriver() != nullptr ) {
		if ( dynamic_cast<JackAudioDriver*>(pAudioEngine->getAudioDriver()) != nullptr ) {
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
		if ( dynamic_cast<JackAudioDriver*>(pAudioEngine->getAudioDriver()) != nullptr &&
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

float Hydrogen::getMasterBpm() const {
#ifdef H2CORE_HAVE_JACK
  if ( m_pAudioEngine->getAudioDriver() != nullptr ) {
	  if ( dynamic_cast<JackAudioDriver*>(m_pAudioEngine->getAudioDriver()) != nullptr ) {
		  return static_cast<JackAudioDriver*>(m_pAudioEngine->getAudioDriver())->getMasterBpm();
	  } else {
		  return std::nan("No JACK driver");
	  }
  } else {
	  return std::nan("No audio driver");
  }
#else
  return std::nan("No JACK support");
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

bool Hydrogen::isTimelineEnabled() const {
	if ( getSong()->getIsTimelineActivated() &&
		 getMode() == Song::Mode::Song &&
		 getJackTimebaseState() != JackAudioDriver::Timebase::Slave ) {
		return true;
	}

	return false;
}

Hydrogen::Tempo Hydrogen::getTempoSource() const {
	if ( getMode() == Song::Mode::Song ) {
		if ( getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
			return Tempo::Jack;
		} else if ( getSong()->getIsTimelineActivated() ) {
			return Tempo::Timeline;
		}
	}

	return Tempo::Song;
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


void Hydrogen::recalculateRubberband( float fBpm ) {
	DEBUGLOG( fBpm );

	if ( !Preferences::get_instance()->getRubberBandBatchMode() ) {
		return;
	}
	
	if ( getSong() != nullptr ) {
		auto pInstrumentList = getSong()->getInstrumentList();
		if ( pInstrumentList != nullptr ) {
			for ( unsigned nnInstr = 0; nnInstr < pInstrumentList->size(); ++nnInstr ) {
				auto pInstr = pInstrumentList->get( nnInstr );
				if ( pInstr == nullptr ) {
					return;
				}
				assert( pInstr );
				if ( pInstr != nullptr ){
					for ( int nnComponent = 0; nnComponent < pInstr->get_components()->size();
						  ++nnComponent ) {
						auto pInstrumentComponent = pInstr->get_component( nnComponent );
						if ( pInstrumentComponent == nullptr ) {
							continue; // regular case when you have a new component empty
						}
				
						for ( int nnLayer = 0; nnLayer < InstrumentComponent::getMaxLayers(); nnLayer++ ) {
							auto pLayer = pInstrumentComponent->get_layer( nnLayer );
							if ( pLayer != nullptr ) {
								auto pSample = pLayer->get_sample();
								if ( pSample != nullptr ) {
									if( pSample->get_rubberband().use ) {
										auto pNewSample = Sample::load(
																	   pSample->get_filepath(),
																	   pSample->get_loops(),
																	   pSample->get_rubberband(),
																	   *pSample->get_velocity_envelope(),
																	   *pSample->get_pan_envelope(),
																	   fBpm
																	   );
										if( pNewSample == nullptr ){
											continue;
										}
								
										// insert new sample from newInstrument
										pLayer->set_sample( pNewSample );
									}
								}
							}
						}
					}
				}
			}
			setIsModified( true );
		} else {
			ERRORLOG( "No InstrumentList present" );
		}
	} else {
		ERRORLOG( "No song set" );
	}
}

void Hydrogen::setIsModified( bool bIsModified ) {
	if ( getSong() != nullptr ) {
		getSong()->setIsModified( bIsModified );
	}
}

void Hydrogen::setMode( Song::Mode mode ) {
	if ( getSong() != nullptr ) {
		getSong()->setMode( mode );
		EventQueue::get_instance()->push_event( EVENT_SONG_MODE_ACTIVATION, ( mode == Song::Mode::Song) ? 1 : 0 );
	}
}

void Hydrogen::setIsTimelineActivated( bool bEnabled ) {
	auto pPref = Preferences::get_instance();

	if ( bEnabled != getSong()->getIsTimelineActivated() ) {
		pPref->setUseTimelineBpm( bEnabled );
		getSong()->setIsTimelineActivated( bEnabled );

		EventQueue::get_instance()->push_event( EVENT_TIMELINE_ACTIVATION, static_cast<int>( bEnabled ) );
	}
}	

QString Hydrogen::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Hydrogen]\n" ).arg( sPrefix )
			.append( QString( "%1%2__song: " ).arg( sPrefix ).arg( s ) );
		if ( __song != nullptr ) {
			sOutput.append( QString( "%1" ).arg( __song->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_ntaktoMeterCompute: %3\n" ).arg( sPrefix ).arg( s ).arg( m_ntaktoMeterCompute ) )
			.append( QString( "%1%2m_nbeatsToCount: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nbeatsToCount ) )
			.append( QString( "%1%2m_nEventCount: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nEventCount ) )
			.append( QString( "%1%2m_nTempoChangeCounter: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nTempoChangeCounter ) )
			.append( QString( "%1%2m_nBeatCount: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nBeatCount ) )
			.append( QString( "%1%2m_nBeatDiffs: [" ).arg( sPrefix ).arg( s ) );
		for ( auto dd : m_nBeatDiffs ) {
			sOutput.append( QString( " %1" ).arg( dd ) );
		}
		sOutput.append( QString( "]\n%1%2m_CurrentTime: %3" ).arg( sPrefix ).arg( s ).arg( static_cast<long>(m_CurrentTime.tv_sec ) ) )
			.append( QString( "%1%2m_nCoutOffset: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nCoutOffset ) )
			.append( QString( "%1%2m_nStartOffset: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nStartOffset ) )
			.append( QString( "%1%2m_oldEngineMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( static_cast<int>(m_oldEngineMode) ) )
			.append( QString( "%1%2m_bOldLoopEnabled: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bOldLoopEnabled ) )
			.append( QString( "%1%2m_bExportSessionIsActive: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bExportSessionIsActive ) )
			.append( QString( "%1%2m_GUIState: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>( m_GUIState ) ) )
			.append( QString( "%1%2m_pTimeline:\n" ).arg( sPrefix ).arg( s ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_sCurrentDrumkitName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sCurrentDrumkitName ) )
			.append( QString( "%1%2m_currentDrumkitLookup: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_currentDrumkitLookup) ) )
			.append( QString( "%1%2__instrument_death_row:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto const& ii : __instrument_death_row ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			} else {
				sOutput.append( QString( "nullptr\n" ) );
			}
		}
		sOutput.append( QString( "%1%2m_nSelectedInstrumentNumber: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nSelectedInstrumentNumber ) )
			.append( QString( "%1%2m_pAudioEngine: \n" ).arg( sPrefix ).arg( s ) )//.arg( m_pAudioEngine ) )
			.append( QString( "%1%2lastMidiEvent: %3\n" ).arg( sPrefix ).arg( s ).arg( lastMidiEvent ) )
			.append( QString( "%1%2lastMidiEventParameter: %3\n" ).arg( sPrefix ).arg( s ).arg( lastMidiEventParameter ) )
			.append( QString( "%1%2m_nInstrumentLookupTable: [ %3 ... %4 ]\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nInstrumentLookupTable[ 0 ] ).arg( m_nInstrumentLookupTable[ MAX_INSTRUMENTS -1 ] ) );
	} else {
		
		sOutput = QString( "%1[Hydrogen]" ).arg( sPrefix )
			.append( QString( ", __song: " ) );
		if ( __song != nullptr ) {
			sOutput.append( QString( "%1" ).arg( __song->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr" ) );
		}
		sOutput.append( QString( ", m_ntaktoMeterCompute: %1" ).arg( m_ntaktoMeterCompute ) )
			.append( QString( ", m_nbeatsToCount: %1" ).arg( m_nbeatsToCount ) )
			.append( QString( ", m_nEventCount: %1" ).arg( m_nEventCount ) )
			.append( QString( ", m_nTempoChangeCounter: %1" ).arg( m_nTempoChangeCounter ) )
			.append( QString( ", m_nBeatCount: %1" ).arg( m_nBeatCount ) )
			.append( QString( ", m_nBeatDiffs: [" ) );
		for ( auto dd : m_nBeatDiffs ) {
			sOutput.append( QString( " %1" ).arg( dd ) );
		}
		sOutput.append( QString( "], m_CurrentTime: %1" ).arg( static_cast<long>( m_CurrentTime.tv_sec ) ) )
			.append( QString( ", m_nCoutOffset: %1" ).arg( m_nCoutOffset ) )
			.append( QString( ", m_nStartOffset: %1" ).arg( m_nStartOffset ) )
			.append( QString( ", m_oldEngineMode: %1" )
					 .arg( static_cast<int>(m_oldEngineMode) ) )
			.append( QString( ", m_bOldLoopEnabled: %1" ).arg( m_bOldLoopEnabled ) )
			.append( QString( ", m_bExportSessionIsActive: %1" ).arg( m_bExportSessionIsActive ) )
			.append( QString( ", m_GUIState: %1" ).arg( static_cast<int>( m_GUIState ) ) );
		sOutput.append( QString( ", m_pTimeline: " ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr" ) );
		}						 
		sOutput.append( QString( ", m_sCurrentDrumkitName: %1" ).arg( m_sCurrentDrumkitName ) )
			.append( QString( ", m_currentDrumkitLookup: %1" ).arg( static_cast<int>(m_currentDrumkitLookup) ) )
			.append( QString( ", __instrument_death_row: [" ) );
		for ( auto const& ii : __instrument_death_row ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			} else {
				sOutput.append( QString( " nullptr" ) );
			}
		}
		sOutput.append( QString( ", m_nSelectedInstrumentNumber: %1" ).arg( m_nSelectedInstrumentNumber ) )
			.append( QString( ", m_pAudioEngine: " ) )// .arg( m_pAudioEngine ) )
			.append( QString( ", lastMidiEvent: %1" ).arg( lastMidiEvent ) )
			.append( QString( ", lastMidiEventParameter: %1" ).arg( lastMidiEventParameter ) )
			.append( QString( ", m_nInstrumentLookupTable: [ %1 ... %2 ]" )
					 .arg( m_nInstrumentLookupTable[ 0 ] ).arg( m_nInstrumentLookupTable[ MAX_INSTRUMENTS -1 ] ) );
	}
		
	return sOutput;
}

}; /* Namespace */
