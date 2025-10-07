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
#include <core/AudioEngine/TransportPosition.h>
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
#include <core/SoundLibrary/SoundLibraryDatabase.h>

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
					 , m_lastMidiEvent( MidiMessage::Event::Null )
					 , m_nLastMidiEventParameter( 0 )
					 , m_CurrentTime( {0,0} )
					 , m_oldEngineMode( Song::Mode::Song ) 
					 , m_bOldLoopEnabled( false )
					 , m_nLastRecordedMIDINoteTick( 0 )
					 , m_bSessionDrumkitNeedsRelinking( false )
					 , m_bSessionIsExported( false )
					 , m_nHihatOpenness( 127 )
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

	m_pSoundLibraryDatabase = new SoundLibraryDatabase();
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

	m_pAudioEngine->lock( RIGHT_HERE );
	removeSong();
	m_pAudioEngine->unlock();
	
	__kill_instruments();

	delete m_pSoundLibraryDatabase;
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
	if ( pSong != nullptr ) {
		pSong->getPatternList()->set_to_old();
	}
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

	// Delete redundant instruments still alive after switching the
	// drumkit to a smaller one.
	__kill_instruments();
}

Song::PlaybackTrack Hydrogen::getPlaybackTrackState() const {

	if ( __song == nullptr ) {
		ERRORLOG( "No song set yet" );
		return Song::PlaybackTrack::None;
	}

	return __song->getPlaybackTrackState();
}
	
void Hydrogen::mutePlaybackTrack( const bool bMuted )
{
	if ( __song == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	__song->setPlaybackTrackEnabled( bMuted );

	EventQueue::get_instance()->push_event( EVENT_PLAYBACK_TRACK_CHANGED, 0 );
}

void Hydrogen::loadPlaybackTrack( QString sFilename )
{
	if ( __song == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	if ( ! sFilename.isEmpty() &&
		 ! Filesystem::file_exists( sFilename, true ) ) {
		ERRORLOG( QString( "Invalid playback track filename [%1]. File does not exist." )
				  .arg( sFilename ) );
		sFilename = "";
	}

	if ( sFilename.isEmpty() ) {
		INFOLOG( "Disable playback track" );
		__song->setPlaybackTrackEnabled( false );
	}
	
	__song->setPlaybackTrackFilename( sFilename );

	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();
	
	EventQueue::get_instance()->push_event( EVENT_PLAYBACK_TRACK_CHANGED, 0 );
}

void Hydrogen::setSong( std::shared_ptr<Song> pSong, bool bRelinking )
{
	if ( pSong == nullptr ) {
		WARNINGLOG( "setting nullptr!" );
	}

	std::shared_ptr<Song> pCurrentSong = getSong();
	if ( pSong == pCurrentSong ) {
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );

	// Move to the beginning.
	setSelectedPatternNumber( 0, false );

	if ( pCurrentSong != nullptr ) {
		/* NOTE: 
		 *       - this is actually some kind of cleanup 
		 *       - removeSong cares itself for acquiring a lock
		 */
		
		if ( isUnderSessionManagement() ) {
			// When under session management Hydrogen is only allowed
			// to replace the content of the session song but not to
			// write to a different location.
			if ( pSong != nullptr ) {
				pSong->setFilename( pCurrentSong->getFilename() );
			}
		}
		removeSong();
	}

	// In order to allow functions like audioEngine_setupLadspaFX() to
	// load the settings of the new song, like whether the LADSPA FX
	// are activated, __song has to be set prior to the call of
	// audioEngine_setSong().
	__song = pSong;

	// Ensure the selected instrument is within the range of new
	// instrument list.
	if ( pSong != nullptr &&
		 m_nSelectedInstrumentNumber >= __song->getInstrumentList()->size() ) {
		m_nSelectedInstrumentNumber =
			std::max( __song->getInstrumentList()->size() - 1, 0 );
	}

	// Update the audio engine to work with the new song.
	m_pAudioEngine->setSong( pSong );

	// load new playback track information
	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();

	m_pAudioEngine->unlock();

	// Push current state of Hydrogen to attached control interfaces,
	// like OSC clients.
	m_pCoreActionController->initExternalControlInterfaces();

#ifdef H2CORE_HAVE_OSC
	if ( isUnderSessionManagement() && bRelinking ) {
		setSessionDrumkitNeedsRelinking( true );
	}
#endif
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

bool Hydrogen::addRealtimeNote(	int		nInstrument,
								float	fVelocity,
								bool	bNoteOff,
								int		nNote )
{
	
	AudioEngine* pAudioEngine = m_pAudioEngine;
	auto pSampler = pAudioEngine->getSampler();
	Preferences *pPref = Preferences::get_instance();
	unsigned int nRealColumn = 0;
	unsigned res = pPref->getPatternEditorGridResolution();
	int nBase = pPref->isPatternEditorUsingTriplets() ? 3 : 4;
	bool bPlaySelectedInstrument = pPref->__playselectedinstrument;
	int scalar = ( 4 * MAX_NOTES ) / ( res * nBase );
	int currentPatternNumber;

	std::shared_ptr<Song> pSong = getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	m_pAudioEngine->lock( RIGHT_HERE );
	
	if ( ! bPlaySelectedInstrument ) {
		if ( nInstrument >= ( int ) pSong->getInstrumentList()->size() ) {
			// unused instrument
			ERRORLOG( QString( "Provided instrument [%1] not found" )
					  .arg( nInstrument ) );
			pAudioEngine->unlock();
			return false;
		}
	}

	// Get current pattern and column
	const Pattern* pCurrentPattern = nullptr;
	long nTickInPattern = 0;
	const float fPan = 0;

	bool doRecord = pPref->getRecordEvents();
	if ( getMode() == Song::Mode::Song && doRecord &&
		 pAudioEngine->getState() == AudioEngine::State::Playing ) {

		// Recording + song playback mode + actually playing
		PatternList* pPatternList = pSong->getPatternList();
		auto pColumns = pSong->getPatternGroupVector();
		int nColumn = pAudioEngine->getTransportPosition()->getColumn(); // current column
		// or pattern group
		if ( nColumn < 0 || nColumn >= pColumns->size() ) {
			pAudioEngine->unlock(); // unlock the audio engine
			ERRORLOG( QString( "Provided column [%1] out of bound [%2,%3)" )
					  .arg( nColumn ).arg( 0 )
					  .arg( pColumns->size() ) );
			return false;
		}
		// Locate nTickInPattern -- may need to jump back one column
		nTickInPattern = pAudioEngine->getTransportPosition()->getPatternTickPosition();

		// Capture new notes in the bottom-most pattern (if not already done above)
		PatternList *pColumn = ( *pColumns )[ nColumn ];
		currentPatternNumber = -1;
		for ( int n = 0; n < pColumn->size(); n++ ) {
			Pattern *pPattern = pColumn->get( n );
			int nIndex = pPatternList->index( pPattern );
			if ( nIndex > currentPatternNumber ) {
				currentPatternNumber = nIndex;
				pCurrentPattern = pPattern;
			}
		}

		// Cancel recording if punch area disagrees
		doRecord = pPref->inPunchArea( nColumn );

	}
	else { // Not song-record mode
		PatternList *pPatternList = pSong->getPatternList();

		if ( ( m_nSelectedPatternNumber != -1 )
			 && ( m_nSelectedPatternNumber < ( int )pPatternList->size() ) )
		{
			pCurrentPattern = pPatternList->get( m_nSelectedPatternNumber );
			currentPatternNumber = m_nSelectedPatternNumber;
		}

		if ( ! pCurrentPattern ) {
			ERRORLOG( "Current pattern invalid" );
			pAudioEngine->unlock(); // unlock the audio engine
			return false;
		}

		// Locate nTickInPattern -- may need to wrap around end of pattern
		nTickInPattern = pAudioEngine->getTransportPosition()->getPatternTickPosition();
	}

	if ( pCurrentPattern && pPref->getQuantizeEvents() ) {
		// quantize it to scale
		unsigned qcolumn = ( unsigned )::round( nTickInPattern / ( double )scalar ) * scalar;

		//we have to make sure that no beat is added on the last displayed note in a bar
		//for example: if the pattern has 4 beats, the editor displays 5 beats, so we should avoid adding beats an note 5.
		if ( qcolumn == pCurrentPattern->get_length() ){
			qcolumn = 0;
		}
		nTickInPattern = qcolumn;
	}

	auto pInstrumentList = pSong->getInstrumentList();
	int nInstrumentNumber;
	if ( bPlaySelectedInstrument ) {
		nInstrumentNumber = getSelectedInstrumentNumber();
	} else {
		nInstrumentNumber = m_nInstrumentLookupTable[ nInstrument ];
	}
	auto pInstr = pInstrumentList->get( nInstrumentNumber );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieved instrument [%1]. Plays selected instrument: [%2]" )
				  .arg( nInstrumentNumber )
				  .arg( bPlaySelectedInstrument ) );
		pAudioEngine->unlock();
		return false;
	}

	// Record note
	if ( pCurrentPattern != nullptr &&
		 pAudioEngine->getState() == AudioEngine::State::Playing &&
		 doRecord ) {

		INFOLOG( QString( "Recording [%1] to pattern: %2 (%3), tick: [%4/%5]." )
				 .arg( bNoteOff ? "NoteOff" : "NoteOn")
				 .arg( currentPatternNumber ).arg( pCurrentPattern->get_name() )
				 .arg( nTickInPattern ).arg( pCurrentPattern->get_length() ) );

		bool bIsModified = false;

		if ( bNoteOff ) {
			
			int nPatternSize = pCurrentPattern->get_length();
			int nNoteLength =
				static_cast<int>(pAudioEngine->getTransportPosition()->getPatternTickPosition()) -
				m_nLastRecordedMIDINoteTick;

			if ( bPlaySelectedInstrument ) {
				nNoteLength =
					static_cast<int>(static_cast<double>(nNoteLength) *
									 Note::pitchToFrequency( nNote ));
			}

			for ( unsigned nNote = 0; nNote < nPatternSize; nNote++ ) {
				const Pattern::notes_t* notes = pCurrentPattern->get_notes();
				FOREACH_NOTE_CST_IT_BOUND_LENGTH( notes, it, nNote, pCurrentPattern ) {
					Note *pNote = it->second;
					if ( pNote != nullptr &&
						 pNote->get_position() == m_nLastRecordedMIDINoteTick &&
						 pInstr == pNote->get_instrument() ) {
						
						if ( m_nLastRecordedMIDINoteTick + nNoteLength > nPatternSize ) {
							nNoteLength = nPatternSize - m_nLastRecordedMIDINoteTick;
						}
						pNote->set_length( nNoteLength );
						bIsModified = true;
					}
				}
			}

		}
		else { // note on
			EventQueue::AddMidiNoteVector noteAction;
			noteAction.m_column = nTickInPattern;
			noteAction.m_row = nInstrumentNumber;
			noteAction.m_pattern = currentPatternNumber;
			noteAction.f_velocity = fVelocity;
			noteAction.f_pan = fPan;
			noteAction.m_length = -1;
			noteAction.b_isMidi = true;

			if ( bPlaySelectedInstrument ) {
				int divider = nNote / 12;
				noteAction.no_octaveKeyVal = (Note::Octave)(divider -3);
				noteAction.nk_noteKeyVal = (Note::Key)(nNote - (12 * divider));
				noteAction.b_isInstrumentMode = true;
			} else {
				noteAction.no_octaveKeyVal = (Note::Octave)0;
				noteAction.nk_noteKeyVal = (Note::Key)0;
				noteAction.b_isInstrumentMode = false;
			}

			EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);

			m_nLastRecordedMIDINoteTick = nTickInPattern;
			
			bIsModified = true;
		}
		
		if ( bIsModified ) {
			EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, -1 );
			setIsModified( true );
		}
	}

	// Play back the note.
	if ( ! pInstr->hasSamples() ) {
		pAudioEngine->unlock();
		return true;
	}
	
	if ( bPlaySelectedInstrument ) {
		if ( bNoteOff ) {
			if ( pSampler->isInstrumentPlaying( pInstr ) ) {
				pSampler->midiKeyboardNoteOff( nNote );
			}
		}
		else { // note on
			Note *pNote2 = new Note( pInstr, nRealColumn, fVelocity, fPan );

			int divider = nNote / 12;
			Note::Octave octave = (Note::Octave)(divider -3);
			Note::Key notehigh = (Note::Key)(nNote - (12 * divider));

			pNote2->set_midi_info( notehigh, octave, nNote );
			midi_noteOn( pNote2 );
		}
	}
	else {
		if ( bNoteOff ) {
			if ( pSampler->isInstrumentPlaying( pInstr ) ) {
				Note *pNoteOff = new Note( pInstr );
				pNoteOff->set_note_off( true );
				midi_noteOn( pNoteOff );
			}
		}
		else { // note on
			Note *pNote2 = new Note( pInstr, nRealColumn, fVelocity, fPan );
			midi_noteOn( pNote2 );
		}
	}

	m_pAudioEngine->unlock(); // unlock the audio engine
	return true;
}


void Hydrogen::toggleNextPattern( int nPatternNumber ) {
	if ( __song != nullptr && getMode() == Song::Mode::Pattern ) {
		m_pAudioEngine->lock( RIGHT_HERE );
		m_pAudioEngine->toggleNextPattern( nPatternNumber );
		m_pAudioEngine->unlock();
		EventQueue::get_instance()->push_event( EVENT_NEXT_PATTERNS_CHANGED, 0 );

	} else {
		ERRORLOG( "can't set next pattern in song mode" );
	}
}

bool Hydrogen::flushAndAddNextPattern( int nPatternNumber ) {
	if ( __song != nullptr && getMode() == Song::Mode::Pattern ) {
		m_pAudioEngine->lock( RIGHT_HERE );
		m_pAudioEngine->flushAndAddNextPattern( nPatternNumber );
		m_pAudioEngine->unlock();
		EventQueue::get_instance()->push_event( EVENT_NEXT_PATTERNS_CHANGED, 0 );

		return true;

	} else {
		ERRORLOG( "can't set next pattern in song mode" );
	}

	return false;
}

void Hydrogen::restartDrivers()
{
	m_pAudioEngine->restartAudioDrivers();
}

bool Hydrogen::startExportSession( int nSampleRate, int nSampleDepth,
								   double fCompressionLevel )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
		sequencer_stop();
	}

	std::shared_ptr<Song> pSong = getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	m_oldEngineMode = pSong->getMode();
	m_bOldLoopEnabled = pSong->isLoopEnabled();

	pSong->setMode( Song::Mode::Song );
	pSong->setLoopMode( Song::LoopMode::Disabled );
	
	/*
	 * Currently an audio driver is loaded
	 * which is not the DiskWriter driver.
	 * Stop the current driver and fire up the DiskWriter.
	 */
	pAudioEngine->stopAudioDrivers();

	AudioOutput* pDriver = pAudioEngine->createAudioDriver(
		Preferences::AudioDriver::Disk );

	DiskWriterDriver* pDiskWriterDriver = dynamic_cast<DiskWriterDriver*>( pDriver );
	if ( pDriver == nullptr || pDiskWriterDriver == nullptr ) {
		ERRORLOG( "Unable to start up DiskWriterDriver" );

		if ( pDriver != nullptr ) {
			delete pDriver;
		}
		return false;
	}
	
	pDiskWriterDriver->setSampleRate( static_cast<unsigned>(nSampleRate) );
	pDiskWriterDriver->setSampleDepth( nSampleDepth );
	pDiskWriterDriver->setCompressionLevel( fCompressionLevel );

	m_bExportSessionIsActive = true;

	return true;
}

/// Export a song to a wav file
void Hydrogen::startExportSong( const QString& filename)
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	getCoreActionController()->locateToTick( 0 );
	pAudioEngine->play();
	pAudioEngine->getSampler()->stopPlayingNotes();

	DiskWriterDriver* pDiskWriterDriver = static_cast<DiskWriterDriver*>(pAudioEngine->getAudioDriver());
	pDiskWriterDriver->setFileName( filename );
	pDiskWriterDriver->write();
}

void Hydrogen::stopExportSong()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	pAudioEngine->getSampler()->stopPlayingNotes();
	getCoreActionController()->locateToTick( 0 );
}

void Hydrogen::stopExportSession()
{
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong == nullptr ) {
		return;
	}

	pSong->setMode( m_oldEngineMode );
	if ( m_bOldLoopEnabled ) {
		pSong->setLoopMode( Song::LoopMode::Enabled );
	} else {
		pSong->setLoopMode( Song::LoopMode::Disabled );
	}
	
	AudioEngine* pAudioEngine = m_pAudioEngine;

	pAudioEngine->stop();
 	pAudioEngine->restartAudioDrivers();
	if ( pAudioEngine->getAudioDriver() == nullptr ) {
		ERRORLOG( "Unable to restart previous audio driver after exporting song." );
	}
	m_bExportSessionIsActive = false;
}

/// Used to display audio driver info
AudioOutput* Hydrogen::getAudioOutput() const
{
	return m_pAudioEngine->getAudioDriver();
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

// This will check if an instrument has any notes
bool Hydrogen::instrumentHasNotes( std::shared_ptr<Instrument> pInst )
{
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	PatternList* pPatternList = pSong->getPatternList();

	for ( int nPattern = 0 ; nPattern < (int)pPatternList->size() ; ++nPattern )
	{
		if( pPatternList->get( nPattern )->references( pInst ) )
		{
			INFOLOG("Instrument " + pInst->get_name() + " has notes" );
			return true;
		}
	}

	// no notes for this instrument
	return false;
}

void Hydrogen::removeInstrument( int nInstrumentNumber ) {
	auto pSong = getSong();
	if ( pSong != nullptr ) {

		m_pAudioEngine->lock( RIGHT_HERE );

		pSong->removeInstrument( nInstrumentNumber, false );
		
		if ( nInstrumentNumber == m_nSelectedInstrumentNumber ) {
			setSelectedInstrumentNumber( std::max( 0, nInstrumentNumber - 1 ) );
		} else if ( m_nSelectedInstrumentNumber >=
					pSong->getInstrumentList()->size() ) {
			setSelectedInstrumentNumber( std::max( 0, pSong->getInstrumentList()->size() - 1 ) );
		}
		m_pAudioEngine->unlock();
		
		setIsModified( true );
	}
}

void Hydrogen::raiseError( unsigned nErrorCode )
{
	m_pAudioEngine->raiseError( nErrorCode );
}

void Hydrogen::onTapTempoAccelEvent()
{
	if ( getTempoSource() != Tempo::Song ) {
		return;
	}
#ifndef WIN32
	INFOLOG( "tap tempo" );
	static timeval oldTimeVal;

	struct timeval now;
	gettimeofday(&now, nullptr);

	float fInterval =
			(now.tv_sec - oldTimeVal.tv_sec) * 1000.0
			+ (now.tv_usec - oldTimeVal.tv_usec) / 1000.0;

	oldTimeVal = now;

	// We multiply by a factor of two in order to allow for tempi
	// smaller than the minimum one enter the calculation of the
	// average. Else the minimum one could not be reached via tap
	// tempo and it is clambed anyway.
	if ( fInterval < 60000.0 * 2 / static_cast<float>(MIN_BPM) ) {
		setTapTempo( fInterval );
	}
#endif
}

void Hydrogen::setTapTempo( float fInterval ) {

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
	m_pAudioEngine->setNextBpm( fBPM );
	m_pAudioEngine->unlock();
	
	// Store it's value in the .h2song file.
	if ( __song != nullptr ) {
		__song->setBpm( fBPM );
	}
	
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

void Hydrogen::updateSelectedPattern( bool bNeedsLock ) {
	if ( isPatternEditorLocked() ) {
		if ( bNeedsLock ) {
			m_pAudioEngine->lock( RIGHT_HERE );
		}
		m_pAudioEngine->handleSelectedPattern();
		if ( bNeedsLock ) {
			m_pAudioEngine->unlock();
		}
	}
}

void Hydrogen::setSelectedPatternNumber( int nPat, bool bNeedsLock, bool bForce )
{
	if ( nPat == m_nSelectedPatternNumber ) {
		if ( bForce ) {
			EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
		}
		return;
	}

	if ( getPatternMode() == Song::PatternMode::Selected ) {
		if ( bNeedsLock ) {
			m_pAudioEngine->lock( RIGHT_HERE );
		}
		
		m_nSelectedPatternNumber = nPat;
		// The specific values provided are not important since we a
		// in selected pattern mode.
		m_pAudioEngine->updatePlayingPatterns();

		if ( bNeedsLock ) {
			m_pAudioEngine->unlock();
		}
	} else {
		m_nSelectedPatternNumber = nPat;
	}

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}

void Hydrogen::setSelectedInstrumentNumber( int nInstrument, bool bTriggerEvent )
{
	if ( m_nSelectedInstrumentNumber == nInstrument ) {
		return;
	}

	m_nSelectedInstrumentNumber = nInstrument;

	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	}
}

void Hydrogen::renameJackPorts( std::shared_ptr<Song> pSong )
{
#ifdef H2CORE_HAVE_JACK
	if ( pSong == nullptr ) {
		return;
	}
	
	if ( Preferences::get_instance()->m_bJackTrackOuts == true &&
		hasJackAudioDriver() && pSong != nullptr ) {

		// When restarting the audio driver after loading a new song under
		// Non session management all ports have to be registered _prior_
		// to the activation of the client.
		if ( isUnderSessionManagement() &&
			 getGUIState() != Hydrogen::GUIState::ready ) {
			return;
		}

		m_pAudioEngine->makeTrackPorts( pSong );
	}
#endif
}

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

bool Hydrogen::handleBeatCounter()
{
	if ( getTempoSource() != Tempo::Song ) {
		return false;
	}
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
		return false;
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
			

			m_pAudioEngine->lock( RIGHT_HERE );
			m_pAudioEngine->setNextBpm( fBeatCountBpm );
			m_pAudioEngine->unlock();

			if ( __song != nullptr ) {
				__song->setBpm( fBeatCountBpm );
			}
	
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
				return true;
			}
		}
		else {
			m_nBeatCount ++;
		}
	}
	else {
		return false;
	}
	return true;
}
// ~ m_nBeatCounter

void Hydrogen::releaseJackTimebaseControl()
{
#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( hasJackTransport() ) {
		static_cast< JackAudioDriver* >( pAudioEngine->getAudioDriver() )->releaseTimebaseControl();
	}
#endif
}

void Hydrogen::initJackTimebaseControl()
{
#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( hasJackTransport() ) {
		static_cast< JackAudioDriver* >( pAudioEngine->getAudioDriver() )->initTimebaseControl();
	}
#endif
}

void Hydrogen::addInstrumentToDeathRow( std::shared_ptr<Instrument> pInstr ) {
	__instrument_death_row.push_back( pInstr );
	__kill_instruments();
}

void Hydrogen::__kill_instruments()
{
	if ( __instrument_death_row.size() > 0 ) {
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
		}
		if ( __instrument_death_row.size() ) {
			pInstr = __instrument_death_row.front();
			INFOLOG( QString( "Instrument %1 still has %2 active notes. "
							  "Delaying 'delete instrument' operation." )
					 . arg( pInstr->get_name() )
					 . arg( pInstr->is_queued() ) );
		}
	}
}



void Hydrogen::__panic()
{
	m_pAudioEngine->lock( RIGHT_HERE );
	sequencer_stop();
	m_pAudioEngine->getSampler()->stopPlayingNotes();
	m_pAudioEngine->unlock();
}

bool Hydrogen::hasJackAudioDriver() const {
#ifdef H2CORE_HAVE_JACK
	if ( m_pAudioEngine->getAudioDriver() != nullptr ) {
		if ( dynamic_cast<JackAudioDriver*>(m_pAudioEngine->getAudioDriver()) != nullptr ) {
			return true;
		}
	}
	return false;
#else
	return false;
#endif	
}

bool Hydrogen::hasJackTransport() const {
#ifdef H2CORE_HAVE_JACK
	if ( m_pAudioEngine->getAudioDriver() != nullptr ) {
		if ( dynamic_cast<JackAudioDriver*>(m_pAudioEngine->getAudioDriver()) != nullptr &&
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

float Hydrogen::getJackTimebaseControllerBpm() const {
#ifdef H2CORE_HAVE_JACK
  if ( m_pAudioEngine->getAudioDriver() != nullptr ) {
	  if ( dynamic_cast<JackAudioDriver*>(m_pAudioEngine->getAudioDriver()) != nullptr ) {
		  return static_cast<JackAudioDriver*>(m_pAudioEngine->getAudioDriver())->getTimebaseControllerBpm();
	  } else {
		  ERRORLOG("No JACK driver");
		  return std::nan("");
	  }
  } else {
	  ERRORLOG("No audio driver");
	  return std::nan("");
  }
#else
  ERRORLOG("No JACK support");
  return std::nan("");
#endif
}

JackAudioDriver::Timebase Hydrogen::getJackTimebaseState() const {
#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	if ( hasJackTransport() ) {
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
	if ( __song != nullptr && __song->getIsTimelineActivated() &&
		 getMode() == Song::Mode::Song &&
		 getJackTimebaseState() != JackAudioDriver::Timebase::Listener ) {
		return true;
	}

	return false;
}

bool Hydrogen::isPatternEditorLocked() const {
	if ( getMode() == Song::Mode::Song &&
		 __song != nullptr ) {
		if ( __song->getIsPatternEditorLocked() ) {
			return true;
		}
	}

	return false;
}

void Hydrogen::setIsPatternEditorLocked( bool bValue ) {
	if ( __song != nullptr &&
		 bValue != __song->getIsPatternEditorLocked() ) {
		__song->setIsPatternEditorLocked( bValue );
		__song->setIsModified( true );

		updateSelectedPattern();
			
		EventQueue::get_instance()->push_event( EVENT_PATTERN_EDITOR_LOCKED,
												bValue );
	}
}

Song::Mode Hydrogen::getMode() const {
	if ( __song != nullptr ) {
		return __song->getMode();
	}

	return Song::Mode::None;
}

void Hydrogen::setMode( Song::Mode mode ) {
	if ( __song != nullptr && mode != __song->getMode() ) {
		__song->setMode( mode );
		EventQueue::get_instance()->push_event( EVENT_SONG_MODE_ACTIVATION,
												( mode == Song::Mode::Song) ? 1 : 0 );
	}
}

Song::ActionMode Hydrogen::getActionMode() const {
	if ( __song != nullptr ) {
		return __song->getActionMode();
	}
	return Song::ActionMode::None;
}

void Hydrogen::setActionMode( Song::ActionMode mode ) {
	if ( __song != nullptr ) {
		__song->setActionMode( mode );
		EventQueue::get_instance()->push_event( EVENT_ACTION_MODE_CHANGE,
												( mode == Song::ActionMode::drawMode ) ? 1 : 0 );
	}
}

Song::PatternMode Hydrogen::getPatternMode() const {
	if ( __song != nullptr && getMode() == Song::Mode::Pattern ) {
		return __song->getPatternMode();
	}
	return Song::PatternMode::None;
}

void Hydrogen::setPatternMode( Song::PatternMode mode )
{
	if ( __song != nullptr &&
		 getPatternMode() != mode ) {
		m_pAudioEngine->lock( RIGHT_HERE );

		__song->setPatternMode( mode );
		setIsModified( true );
		
		if ( m_pAudioEngine->getState() != AudioEngine::State::Playing ||
			 mode == Song::PatternMode::Selected ) {
			// Only update the playing patterns in selected pattern
			// mode or if transport is not rolling. In stacked pattern
			// mode with transport rolling
			// AudioEngine::updatePatternTransportPosition() will call
			// the functions and activate the next patterns once the
			// current ones are looped.
			m_pAudioEngine->updatePlayingPatterns();
			m_pAudioEngine->clearNextPatterns();
		}

		m_pAudioEngine->unlock();
		EventQueue::get_instance()->push_event( EVENT_STACKED_MODE_ACTIVATION,
												( mode == Song::PatternMode::Selected ) ? 1 : 0 );
	}
}

Hydrogen::Tempo Hydrogen::getTempoSource() const {
	if ( getJackTimebaseState() == JackAudioDriver::Timebase::Listener ) {
		return Tempo::Jack;
	}
	else if ( getMode() == Song::Mode::Song &&
			  __song != nullptr && __song->getIsTimelineActivated() ) {
		return Tempo::Timeline;
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
										auto pNewSample = std::make_shared<Sample>( pSample );
										
										if ( ! pNewSample->load( fBpm ) ){
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
		if ( getSong()->getIsModified() != bIsModified ) {
			getSong()->setIsModified( bIsModified );
		}
	}
}
bool Hydrogen::getIsModified() const {
	if ( getSong() != nullptr ) {
		return getSong()->getIsModified();
	}
	return false;
}

QString Hydrogen::getLastLoadedDrumkitPath() const {
	if ( getSong() != nullptr ) {
		return getSong()->getLastLoadedDrumkitPath();
	}
	ERRORLOG( "no song set yet" );

	return "";
}

QString Hydrogen::getLastLoadedDrumkitName() const {
	if ( getSong() != nullptr ) {
		return getSong()->getLastLoadedDrumkitName();
	}
	ERRORLOG( "no song set yet" );

	return "";
}

void Hydrogen::setIsTimelineActivated( bool bEnabled ) {
	if ( getSong() != nullptr ) {
		auto pPref = Preferences::get_instance();
		auto pAudioEngine = getAudioEngine();

		if ( bEnabled != getSong()->getIsTimelineActivated() ) {
			
			pAudioEngine->lock( RIGHT_HERE );
			
			// DEBUGLOG( QString( "bEnabled: %1, getSong()->getIsTimelineActivated(): %2" )
			// 		  .arg( bEnabled )
			// 		  .arg( getSong()->getIsTimelineActivated()) );
		
			pPref->setUseTimelineBpm( bEnabled );
			getSong()->setIsTimelineActivated( bEnabled );

			if ( bEnabled ) {
				getTimeline()->activate();
			} else {
				getTimeline()->deactivate();
			}

			pAudioEngine->handleTimelineChange();
			pAudioEngine->unlock();

			EventQueue::get_instance()->push_event( EVENT_TIMELINE_ACTIVATION, static_cast<int>( bEnabled ) );
		}
	}
}

int Hydrogen::getColumnForTick( long nTick, bool bLoopMode, long* pPatternStartTick ) const
{
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong == nullptr ) {
		// Fallback
		const int nPatternSize = MAX_NOTES;
		const int nColumn = static_cast<int>(
			std::floor( static_cast<float>( nTick ) /
						static_cast<float>( nPatternSize ) ) );
		*pPatternStartTick = static_cast<long>(nColumn * nPatternSize);
		return nColumn;
	}

	long nTotalTick = 0;

	std::vector<PatternList*> *pPatternColumns = pSong->getPatternGroupVector();
	int nColumns = pPatternColumns->size();

	if ( nColumns == 0 ) {
		// There are no patterns in the current song.
		*pPatternStartTick = 0;
		return 0;
	}

	// Sum the lengths of all pattern columns and use the macro
	// MAX_NOTES in case some of them are of size zero. If the
	// supplied value nTick is bigger than this and doesn't belong to
	// the next pattern column, we just found the pattern list we were
	// searching for.
	int nPatternSize;
	for ( int i = 0; i < nColumns; ++i ) {
		PatternList *pColumn = ( *pPatternColumns )[ i ];
		if ( pColumn->size() != 0 ) {
			nPatternSize = pColumn->longest_pattern_length();
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
		long nLoopTick = 0;
		// nTotalTicks is now the same as m_nSongSizeInTicks
		if ( nTotalTick != 0 ) {
			nLoopTick = nTick % nTotalTick;
		}
		nTotalTick = 0;
		for ( int i = 0; i < nColumns; ++i ) {
			PatternList *pColumn = ( *pPatternColumns )[ i ];
			if ( pColumn->size() != 0 ) {
				nPatternSize = pColumn->longest_pattern_length();
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

	( *pPatternStartTick ) = 0;
	return -1;
}

long Hydrogen::getTickForColumn( int nColumn ) const
{
	auto pSong = getSong();
	if ( pSong == nullptr ) {
		// Fallback
		return static_cast<long>(nColumn * MAX_NOTES);
	}

	const int nPatternGroups = pSong->getPatternGroupVector()->size();
	if ( nPatternGroups == 0 ) {
		// No patterns in song.
		return 0;
	}

	if ( nColumn >= nPatternGroups ) {
		// The position is beyond the end of the Song, we
		// set periodic boundary conditions or return the
		// beginning of the Song as a fallback.
		if ( pSong->isLoopEnabled() ) {
			nColumn = nColumn % nPatternGroups;
		} else {
			WARNINGLOG( QString( "Provided column [%1] is larger than the available number [%2]")
						.arg( nColumn ) .arg(  nPatternGroups )
						);
			return -1;
		}
	}

	std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
	long totalTick = 0;
	int nPatternSize;
	Pattern *pPattern = nullptr;
	
	for ( int i = 0; i < nColumn; ++i ) {
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

void Hydrogen::updateSongSize() {
	getAudioEngine()->updateSongSize();
}

std::shared_ptr<Instrument> Hydrogen::getSelectedInstrument() const {

	std::shared_ptr<Instrument> pInstrument = nullptr;
	
	if ( __song != nullptr ) {
		
		m_pAudioEngine->lock( RIGHT_HERE );

		int nSelectedInstrumentNumber = m_nSelectedInstrumentNumber;
		auto pInstrList = __song->getInstrumentList();
		if ( nSelectedInstrumentNumber >= pInstrList->size() ) {
			nSelectedInstrumentNumber = -1;
		}

		if ( nSelectedInstrumentNumber != -1 ) {
			pInstrument = pInstrList->get( nSelectedInstrumentNumber );
		}
		
		m_pAudioEngine->unlock();
	}

	return pInstrument;
}

void Hydrogen::updateVirtualPatterns() {

	if ( __song == nullptr ) {
		ERRORLOG( "no song" );
		return;
	}
	PatternList *pPatternList = __song->getPatternList();
	if ( pPatternList == nullptr ) {
		ERRORLOG( "no pattern list");
		return;
	}
	
	pPatternList->flattened_virtual_patterns_compute();

	m_pAudioEngine->lock( RIGHT_HERE );
	m_pAudioEngine->updateVirtualPatterns();
	m_pAudioEngine->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, 0 );
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
		sOutput.append( QString( "%1%2__instrument_death_row:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto const& ii : __instrument_death_row ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			} else {
				sOutput.append( QString( "nullptr\n" ) );
			}
		}
		sOutput.append( QString( "%1%2m_nSelectedInstrumentNumber: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nSelectedInstrumentNumber ) )
			.append( QString( "%1%2m_pAudioEngine:\n" ).arg( sPrefix ).arg( s ) );
		if ( m_pAudioEngine != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pAudioEngine->toQString( sPrefix + s + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2lastMidiEvent: %3\n" ).arg( sPrefix ).arg( s )
						.arg( MidiMessage::EventToQString( m_lastMidiEvent ) ) )
			.append( QString( "%1%2lastMidiEventParameter: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nLastMidiEventParameter ) )
			.append( QString( "%1%2m_nHihatOpenness: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nHihatOpenness ) )
			.append( QString( "%1%2m_nInstrumentLookupTable: [ %3 ... %4 ]\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nInstrumentLookupTable[ 0 ] ).arg( m_nInstrumentLookupTable[ MAX_INSTRUMENTS -1 ] ) );
	}
	else {
		
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
		sOutput.append( QString( ", __instrument_death_row: [" ) );
		for ( auto const& ii : __instrument_death_row ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			} else {
				sOutput.append( QString( " nullptr" ) );
			}
		}
		sOutput.append( QString( ", m_nSelectedInstrumentNumber: %1" ).arg( m_nSelectedInstrumentNumber ) )
			.append( ", m_pAudioEngine:" );
		if ( m_pAudioEngine != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pAudioEngine->toQString( sPrefix, bShort ) ) );
		} else {
			sOutput.append( QString( " nullptr" ) );
		}
		sOutput.append( QString( ", lastMidiEvent: %1" )
						.arg( MidiMessage::EventToQString( m_lastMidiEvent ) ) )
			.append( QString( ", lastMidiEventParameter: %1" )
					 .arg( m_nLastMidiEventParameter ) )
			.append( QString( ", m_nHihatOpenness: %1" ).arg( m_nHihatOpenness ) )
			.append( QString( ", m_nInstrumentLookupTable: [ %1 ... %2 ]" )
					 .arg( m_nInstrumentLookupTable[ 0 ] ).arg( m_nInstrumentLookupTable[ MAX_INSTRUMENTS -1 ] ) );
	}
		
	return sOutput;
}

}; /* Namespace */
