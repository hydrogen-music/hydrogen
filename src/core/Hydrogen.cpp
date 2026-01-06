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

#include <pthread.h>
#include <cassert>
#include <cstdio>
#include <deque>
#include <queue>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include "Midi/Midi.h"

#include <core/Hydrogen.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/FX/LadspaFX.h>
#include <core/H2Exception.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/TimeHelper.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/AudioOutput.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/CoreMidiDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/FakeAudioDriver.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/JackMidiDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/IO/NullDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/PortMidiDriver.h>
#include <core/IO/PulseAudioDriver.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>


#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#include "OscServer.h"
#endif

namespace H2Core
{
//----------------------------------------------------------------------------
//
// Implementation of Hydrogen class
//
//----------------------------------------------------------------------------

Hydrogen* Hydrogen::__instance = nullptr;

Hydrogen::Hydrogen() : m_fBeatCounterBeatLength( 1 )
					 , m_nBeatCounterTotalBeats( 4 )
					 , m_nBeatCounterEventCount( 1 )
					 , m_nBeatCounterBeatCount( 1 )
					 , m_lastBeatCounterTimePoint( TimePoint() )
					 , m_lastTapTempoTimePoint( TimePoint() )
					 , m_fTapTempoAverageBpm( MIN_BPM )
					 , m_nTapTempoEventsAveraged( 0 )
					 , m_nSelectedInstrumentNumber( 0 )
					 , m_nSelectedPatternNumber( 0 )
					 , m_bExportSessionIsActive( false )
					 , m_GUIState( GUIState::startup )
					 , m_lastMidiEvent( MidiEvent::Type::Null )
					 , m_lastMidiEventParameter( Midi::ParameterInvalid )
					 , m_oldEngineMode( Song::Mode::Song )
					 , m_bOldLoopEnabled( false )
					 , m_nLastRecordedMIDINoteTick( 0 )
					 , m_bRecordEnabled( false )
					 , m_bSessionIsExported( false )
					 , m_hihatOpenness( Midi::ParameterMaximum )
{
	if ( __instance ) {
		ERRORLOG( "Hydrogen audio engine is already running" );
		throw H2Exception( "Hydrogen audio engine is already running" );
	}

	auto pPref = Preferences::get_instance();
	m_nBeatCounterDriftCompensation = pPref->m_nBeatCounterDriftCompensation;
	m_nBeatCounterStartOffset = pPref->m_nBeatCounterStartOffset;
	m_beatCounterDiffs.resize( 16 );

	m_pSoundLibraryDatabase = std::make_shared<SoundLibraryDatabase>();
	m_pSong = Song::getEmptySong( m_pSoundLibraryDatabase );

	m_pTimeline = std::make_shared<Timeline>();

	m_pAudioEngine = new AudioEngine();
	m_pMidiActionManager = std::make_shared<MidiActionManager>();
	m_pPlaylist = std::make_shared<Playlist>();
	m_pTimeHelper = std::make_shared<TimeHelper>();

	// Prevent double creation caused by calls from MIDI thread
	__instance = this;

	m_pAudioEngine->startAudioDriver( Event::Trigger::Default );
	m_pAudioEngine->startMidiDriver( Event::Trigger::Default );

	if ( pPref->getOscServerEnabled() ) {
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

	m_pAudioEngine->lock( RIGHT_HERE );
	m_pAudioEngine->prepare( Event::Trigger::Suppress );
	m_pAudioEngine->unlock();

	killInstruments();

	delete m_pAudioEngine;

	__instance = nullptr;
}

void Hydrogen::create_instance( int nOscPort )
{
	// Create all the other instances that we need
	// ....and in the right order
	Logger::create_instance();
	Preferences::create_instance();
	EventQueue::create_instance();

#ifdef H2CORE_HAVE_OSC
	NsmClient::create_instance();
	OscServer::create_instance( nOscPort );
#endif

	if ( __instance == nullptr ) {
		__instance = new Hydrogen;
	}
}

/// Start the internal sequencer
void Hydrogen::sequencerPlay()
{
	m_pAudioEngine->play();
}

/// Stop the internal sequencer
void Hydrogen::sequencerStop()
{
	CoreActionController::sendAllNoteOffMessages();

	m_pAudioEngine->stop();
	CoreActionController::activateRecordMode( false );

	// Delete redundant instruments still alive after switching the
	// drumkit to a smaller one.
	killInstruments();
}

Song::PlaybackTrack Hydrogen::getPlaybackTrackState() const {

	if ( m_pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return Song::PlaybackTrack::None;
	}

	return m_pSong->getPlaybackTrackState();
}
	
void Hydrogen::mutePlaybackTrack( const bool bMuted )
{
	if ( m_pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	m_pSong->setPlaybackTrackEnabled( bMuted );

	EventQueue::get_instance()->pushEvent( Event::Type::PlaybackTrackChanged, 0 );
}

void Hydrogen::loadPlaybackTrack( const QString& sFileName )
{
	if ( m_pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	if ( ! sFileName.isEmpty() &&
		 ! Filesystem::file_exists( sFileName, true ) || sFileName.isEmpty() ) {
		ERRORLOG( QString( "Invalid playback track filename [%1]. File does not exist or is empty." )
				  .arg( sFileName ) );
		m_pSong->setPlaybackTrackFileName( "" );
		INFOLOG( "Disabling playback track" );
		m_pSong->setPlaybackTrackEnabled( false );
	}
	else {
		m_pSong->setPlaybackTrackFileName( sFileName );
		m_pSong->setPlaybackTrackEnabled( true );
	}

	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();
	
	EventQueue::get_instance()->pushEvent( Event::Type::PlaybackTrackChanged, 0 );
}

void Hydrogen::setSong( std::shared_ptr<Song> pSong )
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
	setSelectedPatternNumber( 0, false, Event::Trigger::Suppress );

	if ( pCurrentSong != nullptr ) {
		if ( isUnderSessionManagement() ) {
#ifdef H2CORE_HAVE_OSC
			if ( pCurrentSong->getFileName().contains(
					 NsmClient::get_instance()->getSessionFolderPath() ) ) {
				// When under session management Hydrogen is only allowed to
				// replace the content of the session song but not to write to a
				// different location.
				if ( pSong != nullptr ) {
					pSong->setFileName( pCurrentSong->getFileName() );
				}
			}
#endif
		}
		m_pAudioEngine->prepare( Event::Trigger::Suppress );

		if ( pCurrentSong->getDrumkit() != nullptr ) {
			pCurrentSong->getDrumkit()->unloadSamples();
		}
	}

	renameJackPorts( pSong, m_pSong != nullptr ? m_pSong->getDrumkit() : nullptr );

	// In order to allow functions like audioEngine_setupLadspaFX() to
	// load the settings of the new song, like whether the LADSPA FX
	// are activated, m_pSong has to be set prior to the call of
	// AudioEngine::setSong().
	m_pSong = pSong;
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		pSong->getDrumkit()->loadSamples();
	}

	// Ensure the selected instrument is within the range of new
	// instrument list.
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 m_nSelectedInstrumentNumber >=
		 m_pSong->getDrumkit()->getInstruments()->size() ) {
		m_nSelectedInstrumentNumber =
			std::max( m_pSong->getDrumkit()->getInstruments()->size() - 1, 0 );
	}

	// Update the audio engine to work with the new song.
	m_pAudioEngine->setSong( pSong );

	// load new playback track information
	m_pAudioEngine->getSampler()->reinitializePlaybackTrack();

	m_pAudioEngine->unlock();

	// Push current state of Hydrogen to attached control interfaces,
	// like OSC clients.
	CoreActionController::initExternalControlInterfaces();
}

void Hydrogen::midiNoteOn( std::shared_ptr<Note> pNote )
{
	m_pAudioEngine->noteOn( pNote );
}

bool Hydrogen::addRealtimeNote(
	int nInstrument,
	float fVelocity,
	bool bNoteOff,
	Midi::Note note
)
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	auto pSampler = pAudioEngine->getSampler();
	const auto pPref = Preferences::get_instance();
	unsigned int nRealColumn = 0;
	unsigned res = pPref->getPatternEditorGridResolution();
	int nBase = pPref->isPatternEditorUsingTriplets() ? 3 : 4;
	const bool bPlaySelectedInstrument = pPref->getMidiInstrumentMap()->getInput() ==
		MidiInstrumentMap::Input::SelectedInstrument;
	int scalar = ( 4 * 4 * H2Core::nTicksPerQuarter ) / ( res * nBase );
	int currentPatternNumber;

	std::shared_ptr<Song> pSong = getSong();

	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	m_pAudioEngine->lock( RIGHT_HERE );
	
	if ( nInstrument >= pSong->getDrumkit()->getInstruments()->size() ||
	     nInstrument < 0 ) {
          ERRORLOG( QString("Provided instrument number [%1] out of bound [0,%2]")
                  .arg(nInstrument)
                  .arg( pSong->getDrumkit()->getInstruments()->size() ) );
	  pAudioEngine->unlock();
	  return false;
	}
	auto pInstrument = pSong->getDrumkit()->getInstruments()->get( nInstrument );
    if ( pInstrument == nullptr ) {
      ERRORLOG( QString( "Unable to obtain instrument [%1]" ).arg( nInstrument ) );
      pAudioEngine->unlock();
      return false;
    }
	const auto instrumentId = pInstrument->getId();

	// Get current pattern and column
	std::shared_ptr<Pattern> pCurrentPattern = nullptr;
	long nTickInPattern = 0;
	const float fPan = 0;

	bool bDoRecord = m_bRecordEnabled;
	if ( getMode() == Song::Mode::Song && bDoRecord &&
		 pAudioEngine->getState() == AudioEngine::State::Playing ) {

		// Recording + song playback mode + actually playing
		auto pPatternList = pSong->getPatternList();
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
		auto pColumn = ( *pColumns )[ nColumn ];
		currentPatternNumber = -1;
		for ( int n = 0; n < pColumn->size(); n++ ) {
			auto pPattern = pColumn->get( n );
			int nIndex = pPatternList->index( pPattern );
			if ( nIndex > currentPatternNumber ) {
				currentPatternNumber = nIndex;
				pCurrentPattern = pPattern;
			}
		}

		// Cancel recording if punch area disagrees
		bDoRecord = pPref->inPunchArea( nColumn );

	}
	else { // Not song-record mode
		auto pPatternList = pSong->getPatternList();

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
		if ( qcolumn == pCurrentPattern->getLength() ){
			qcolumn = 0;
		}
		nTickInPattern = qcolumn;
	}

	// Record note
	if ( pCurrentPattern != nullptr &&
		 pAudioEngine->getState() == AudioEngine::State::Playing &&
		 bDoRecord ) {

		INFOLOG( QString( "Recording [%1] to pattern: %2 (%3), tick: [%4/%5]." )
				 .arg( bNoteOff ? "NoteOff" : "NoteOn")
				 .arg( currentPatternNumber ).arg( pCurrentPattern->getName() )
				 .arg( nTickInPattern ).arg( pCurrentPattern->getLength() ) );

		bool bIsModified = false;

		if ( bNoteOff ) {
            // Handle the NOTE_OFF event corresponding to the previous NOTE_ON.
            // This is used to record notes of custom lengths.
			int nPatternSize = pCurrentPattern->getLength();
			int nNoteLength =
				static_cast<int>(pAudioEngine->getTransportPosition()->getPatternTickPosition()) -
				m_nLastRecordedMIDINoteTick;

			if ( bPlaySelectedInstrument ) {
				nNoteLength = static_cast<int>(
					static_cast<double>( nNoteLength ) *
					Note::Pitch::fromMidiNote( note ).toFrequency()
				);
			}

			for ( unsigned nnNote = 0; nnNote < nPatternSize; nnNote++ ) {
				const Pattern::notes_t* notes = pCurrentPattern->getNotes();
				FOREACH_NOTE_CST_IT_BOUND_LENGTH( notes, it, nnNote, pCurrentPattern ) {
					auto pNote = it->second;
					if ( pNote != nullptr &&
						 pNote->getPosition() == m_nLastRecordedMIDINoteTick &&
						 pInstrument == pNote->getInstrument() ) {
						
						if ( m_nLastRecordedMIDINoteTick + nNoteLength > nPatternSize ) {
							nNoteLength = nPatternSize - m_nLastRecordedMIDINoteTick;
						}
						pNote->setLength( nNoteLength );
						bIsModified = true;
					}
				}
			}

		}
		else { // note on
			EventQueue::AddMidiNoteVector noteAction;
			noteAction.nColumn = nTickInPattern;
			noteAction.id = instrumentId;
			noteAction.nPattern = currentPatternNumber;
			noteAction.fVelocity = fVelocity;
			noteAction.fPan = fPan;
			noteAction.nLength = -1;

			if ( bPlaySelectedInstrument && note != Midi::NoteInvalid ) {
				noteAction.octave = Note::octaveFrom( note );
				noteAction.key = Note::keyFrom( note );
			}
			else {
				noteAction.octave = Note::OctaveDefault;
				noteAction.key = Note::KeyDefault;
			}

			EventQueue::get_instance()->m_addMidiNoteVector.push_back(noteAction);

			m_nLastRecordedMIDINoteTick = nTickInPattern;
			
			bIsModified = true;
		}
		
		if ( bIsModified ) {
			EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, -1 );
			setIsModified( true );
		}
	}

	// Play back the note.
	if ( ! pInstrument->hasSamples() ) {
		pAudioEngine->unlock();
		return true;
	}
	
	if ( bPlaySelectedInstrument && note != Midi::NoteInvalid ) {
		if ( bNoteOff ) {
			if ( pSampler->isInstrumentPlaying( pInstrument ) ) {
				pSampler->midiKeyboardNoteOff(
					pInstrument, Note::keyFrom( note ), Note::octaveFrom( note )
				);
			}
		}
		else { // note on
			auto pNote2 = std::make_shared<Note>(
				pInstrument, nRealColumn, fVelocity, fPan
			);

			pNote2->setKey( Note::keyFrom( note ) );
			pNote2->setOctave( Note::octaveFrom( note ) );
			midiNoteOn( pNote2 );
		}
	}
	else {
		if ( bNoteOff ) {
			if ( pSampler->isInstrumentPlaying( pInstrument ) ) {
				auto pNoteOff = std::make_shared<Note>( pInstrument );
				pNoteOff->setNoteOff( true );
				midiNoteOn( pNoteOff );
			}
		}
		else { // note on
			auto pNote2 = std::make_shared<Note>(
				pInstrument, nRealColumn, fVelocity, fPan );
			midiNoteOn( pNote2 );
		}
	}

	m_pAudioEngine->unlock(); // unlock the audio engine
	return true;
}

void Hydrogen::toggleNextPattern( int nPatternNumber ) {
	if ( m_pSong != nullptr && getMode() == Song::Mode::Pattern ) {
		m_pAudioEngine->lock( RIGHT_HERE );
		m_pAudioEngine->toggleNextPattern( nPatternNumber );
		m_pAudioEngine->unlock();
		EventQueue::get_instance()->pushEvent( Event::Type::NextPatternsChanged, 0 );

	} else {
		ERRORLOG( "can't set next pattern in song mode" );
	}
}

bool Hydrogen::flushAndAddNextPattern( int nPatternNumber ) {
	if ( m_pSong != nullptr && getMode() == Song::Mode::Pattern ) {
		m_pAudioEngine->lock( RIGHT_HERE );
		m_pAudioEngine->flushAndAddNextPattern( nPatternNumber );
		m_pAudioEngine->unlock();
		EventQueue::get_instance()->pushEvent( Event::Type::NextPatternsChanged, 0 );

		return true;

	} else {
		ERRORLOG( "can't set next pattern in song mode" );
	}

	return false;
}

void Hydrogen::restartAudioDriver() {
	const bool bWasPlaying =
		m_pAudioEngine->getState() == AudioEngine::State::Playing;

	m_pAudioEngine->stopAudioDriver( Event::Trigger::Suppress );
	m_pAudioEngine->startAudioDriver( Event::Trigger::Default );

	if ( bWasPlaying ) {
		m_pAudioEngine->startPlayback();
	}
}

void Hydrogen::restartMidiDriver() {
	m_pAudioEngine->stopMidiDriver( Event::Trigger::Suppress );
	m_pAudioEngine->startMidiDriver( Event::Trigger::Default );
}

bool Hydrogen::startExportSession( int nSampleRate, int nSampleDepth,
								   double fCompressionLevel )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
		sequencerStop();
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
	
	/* Currently an audio driver is loaded which is not the DiskWriter driver.
	 * Stop the current driver and fire up the DiskWriter. */
	pAudioEngine->stopAudioDriver( Event::Trigger::Suppress );
	// We do not want to have any MIDI feedback or note on/off event while
	// exporting audio to file.
	pAudioEngine->stopMidiDriver( Event::Trigger::Default );

	AudioOutput* pDriver = pAudioEngine->createAudioDriver(
		Preferences::AudioDriver::Disk, Event::Trigger::Default );

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
void Hydrogen::startExportSong( const QString& sFileName)
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	CoreActionController::locateToTick( 0 );
	pAudioEngine->play();
	pAudioEngine->getSampler()->stopPlayingNotes();

	DiskWriterDriver* pDiskWriterDriver = static_cast<DiskWriterDriver*>(pAudioEngine->getAudioDriver());
	pDiskWriterDriver->setFileName( sFileName );
	pDiskWriterDriver->write();
}

void Hydrogen::stopExportSong()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	pAudioEngine->getSampler()->stopPlayingNotes();
	CoreActionController::locateToTick( 0 );
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
	pAudioEngine->stopAudioDriver( Event::Trigger::Suppress );
	pAudioEngine->startAudioDriver( Event::Trigger::Default );
	if ( pAudioEngine->getAudioDriver() == nullptr ) {
		ERRORLOG( "Unable to restart previous audio driver after exporting song." );
	}
	pAudioEngine->startMidiDriver( Event::Trigger::Default );
	if ( pAudioEngine->getMidiDriver() == nullptr ) {
		ERRORLOG( "Unable to restart MIDI driver after exporting song." );
	}
	m_bExportSessionIsActive = false;
}

/// Used to display audio driver info
AudioOutput* Hydrogen::getAudioOutput() const
{
	return m_pAudioEngine->getAudioDriver();
}

/// Used to display midi driver info
std::shared_ptr<MidiBaseDriver> Hydrogen::getMidiDriver() const {
	return m_pAudioEngine->getMidiDriver();
}

void Hydrogen::onTapTempoAccelEvent( TimePoint start ) {
	if ( getTempoSource() != Tempo::Song ) {
		return;
	}

	auto now = start;
	if ( now == TimePoint() ) {
		// Default value. No time stamp was provided.
		now = Clock::now();
	}

	const double fInterval = std::chrono::duration_cast<std::chrono::microseconds>(
		now - m_lastTapTempoTimePoint ).count() / 1000.0 / 1000.0;

	m_lastTapTempoTimePoint = now;

	const float fBpm = 60.0 / fInterval;

	// We divide by a factor of two in order to allow for tempi smaller than
	// the minimum one enter the calculation of the average. Else the minimum
	// one could not be reached via tap tempo and it is clambed anyway.
	//
	// This also covers the initial tap tempo handling with
	// m_lastTapTempoTimePoint being initialized to TimePoint().
	if ( fBpm <= static_cast<float>(MIN_BPM) / 2.0 ) {
		// Reset the average.
		m_nTapTempoEventsAveraged = 0;
		return;
	}

	if ( std::abs( fBpm - m_fTapTempoAverageBpm ) > Hydrogen::nTapTempoMaxDiff ) {
		// New speed diverges too much. We reset the tempo instead.
		m_nTapTempoEventsAveraged = 0;
	}

	if ( m_nTapTempoEventsAveraged == 0 ) {
		m_fTapTempoAverageBpm = fBpm;
	}
	else {
		m_fTapTempoAverageBpm =
			( fBpm + static_cast<float>(m_nTapTempoEventsAveraged) *
			  m_fTapTempoAverageBpm ) /
			static_cast<float>(m_nTapTempoEventsAveraged + 1);
	}

	++m_nTapTempoEventsAveraged;

	CoreActionController::setBpm( m_fTapTempoAverageBpm );
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

void Hydrogen::setSelectedPatternNumber( int nPat, bool bNeedsLock,
										 Event::Trigger trigger )
{
	if ( nPat == m_nSelectedPatternNumber ) {
		if ( trigger == Event::Trigger::Force ) {
			EventQueue::get_instance()->pushEvent(
				Event::Type::SelectedPatternChanged, -1 );
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
		m_pAudioEngine->updatePlayingPatterns( Event::Trigger::Default );

		if ( bNeedsLock ) {
			m_pAudioEngine->unlock();
		}
	} else {
		m_nSelectedPatternNumber = nPat;
	}

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent( Event::Type::SelectedPatternChanged, -1 );
	}
}

void Hydrogen::setSelectedInstrumentNumber( int nInstrument,
											Event::Trigger trigger )
{
	// In case no instrument is selected (-1), we still perform an update since
	// another type-only row might be selected in the GUI.
	if ( m_nSelectedInstrumentNumber == nInstrument ) {
		if ( trigger == Event::Trigger::Force ) {
			EventQueue::get_instance()->pushEvent(
				Event::Type::SelectedInstrumentChanged, -1 );
		}
		return;
	}

	if ( m_pSong != nullptr && m_pSong->getDrumkit() != nullptr &&
		 nInstrument < m_pSong->getDrumkit()->getInstruments()->size() &&
		 nInstrument >= 0 ) {
		m_nSelectedInstrumentNumber = nInstrument;
	}
	else {
		m_nSelectedInstrumentNumber = -1;
	}

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::SelectedInstrumentChanged, -1 );
	}
}

void Hydrogen::renameJackPorts( std::shared_ptr<Song> pSong,
								std::shared_ptr<Drumkit> pOldDrumkit )
{
#ifdef H2CORE_HAVE_JACK
	if ( pSong == nullptr ) {
		return;
	}
	
	if ( Preferences::get_instance()->m_bJackTrackOuts == true &&
		hasJackAudioDriver() ) {

		// When restarting the audio driver after loading a new song under
		// Non session management all ports have to be registered _prior_
		// to the activation of the client.
		if ( isUnderSessionManagement() &&
			 getGUIState() != Hydrogen::GUIState::ready ) {
			return;
		}

		m_pAudioEngine->makeTrackPorts( pSong, pOldDrumkit );
	}
#endif
}

void Hydrogen::setBeatCounterTotalBeats( int nBeatsToCount ) {
	if ( m_nBeatCounterTotalBeats != nBeatsToCount ) {
		m_nBeatCounterTotalBeats = nBeatsToCount;
		EventQueue::get_instance()->pushEvent( Event::Type::BeatCounter, 0 );
	}
}

void Hydrogen::setBeatCounterBeatLength( float fBeatLength ) {
	if ( m_fBeatCounterBeatLength != fBeatLength ) {
		m_fBeatCounterBeatLength = fBeatLength;
		EventQueue::get_instance()->pushEvent( Event::Type::BeatCounter, 0 );
	}
}

bool Hydrogen::handleBeatCounter( TimePoint start )
{
	if ( getTempoSource() != Tempo::Song ) {
		return false;
	}

	auto pEventQueue = EventQueue::get_instance();

	auto now = start;
	if ( now == TimePoint() ) {
		// Default value. No time stamp was provided.
		now = Clock::now();
	}
	double fTimeDeltaSeconds;
	if ( m_nBeatCounterBeatCount == 1 ) {
		// Reset or initialize
		m_lastBeatCounterTimePoint = now;
		fTimeDeltaSeconds = 0;
	}
	else {
		fTimeDeltaSeconds = std::chrono::duration_cast<std::chrono::microseconds>(
			now - m_lastBeatCounterTimePoint -
			std::chrono::duration<double, std::milli>(m_nBeatCounterDriftCompensation)
			).count() / 1000.0 / 1000.0;
	}

	m_nBeatCounterEventCount++;
	m_lastBeatCounterTimePoint = now;

	// In case of too big differences, we reset the beatconter. If the user
	// waits long enough, she can start anew.
	if ( fTimeDeltaSeconds > 3.001 * 1/m_fBeatCounterBeatLength ) {
		m_nBeatCounterEventCount = 1;
		m_nBeatCounterBeatCount = 1;

		pEventQueue->pushEvent( Event::Type::BeatCounter, 0 );
		return false;
	}

	// Only accept differences big enough
	if ( m_nBeatCounterBeatCount != 1 && fTimeDeltaSeconds <= .001 ) {
		pEventQueue->pushEvent( Event::Type::BeatCounter, 0 );
		return false;
	}

	// Store the difference for later usage.
	if ( m_nBeatCounterBeatCount > 1 &&
		 m_nBeatCounterBeatCount <= m_beatCounterDiffs.size() ) {
		m_beatCounterDiffs[ m_nBeatCounterBeatCount - 2 ] = fTimeDeltaSeconds;
	}

	// Compute and reset
	double fAverageTime;
	bool bTempoSet = false;
	if ( m_nBeatCounterBeatCount == m_nBeatCounterTotalBeats ){
		double fTotalDiffs = 0;
		for ( const auto& ffDiff : m_beatCounterDiffs ) {
			fTotalDiffs += ffDiff;
		}

		// Time between the beats / beat counter activations.
		fAverageTime = fTotalDiffs /
			( m_nBeatCounterBeatCount - 1 ) * m_fBeatCounterBeatLength;
		const float fBeatCountBpm =
			static_cast<float>(std::floor( 60 / fAverageTime * 100 ) / 100);
			
		if ( CoreActionController::setBpm( fBeatCountBpm ) ) {
			bTempoSet = true;
		}

		m_nBeatCounterBeatCount = 1;
		m_nBeatCounterEventCount = 1;
	}
	else {
		m_nBeatCounterBeatCount++;
	}

	// Update counter numbers before starting playback. Else the user could
	// experience visual delays in the BpmTap.
	pEventQueue->pushEvent( Event::Type::BeatCounter, 0 );

	if ( bTempoSet && Preferences::get_instance()->m_beatCounter ==
		 Preferences::BeatCounter::TapAndPlay &&
		 m_pAudioEngine->getState() != AudioEngine::State::Playing ) {
		const int nSampleRate =
			m_pAudioEngine->getAudioDriver()->getSampleRate();
		long nRtStartFrame = 0;
		if ( m_fBeatCounterBeatLength <= 1 ){
			nRtStartFrame =
				nSampleRate * fAverageTime * ( 1/ m_fBeatCounterBeatLength );
		}
		else {
			nRtStartFrame =
				nSampleRate * fAverageTime / m_fBeatCounterBeatLength ;
		}

		const int nSleepTime =
			static_cast<int>( static_cast<float>(nRtStartFrame) * 1000 /
							  static_cast<float>(nSampleRate) ) +
			m_nBeatCounterDriftCompensation + m_nBeatCounterStartOffset;
		std::this_thread::sleep_for( std::chrono::milliseconds( nSleepTime ) );

		sequencerPlay();
	}

	return true;
}

void Hydrogen::updateBeatCounterSettings() {
	const auto pPreferences = Preferences::get_instance();

	m_nBeatCounterDriftCompensation =
		pPreferences->m_nBeatCounterDriftCompensation;
	m_nBeatCounterStartOffset = pPreferences->m_nBeatCounterStartOffset;

	EventQueue::get_instance()->pushEvent( Event::Type::BeatCounter, 0 );
}

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
	m_instrumentDeathRow.push_back( pInstr );
	killInstruments();
}

void Hydrogen::removeInstrumentFromDeathRow( std::shared_ptr<Instrument> pInstr ) {
	for ( auto it = m_instrumentDeathRow.begin();
		  it != m_instrumentDeathRow.end(); ) {
        if ( *it == pInstr ) {
            it = m_instrumentDeathRow.erase( it );
		} else {
            ++it;
		}
    }
}

void Hydrogen::killInstruments() {
	std::shared_ptr<Instrument> pInstr;

	while ( m_instrumentDeathRow.size() > 0 &&
			( m_instrumentDeathRow.front() == nullptr ||
			  ( m_instrumentDeathRow.front() != nullptr &&
				m_instrumentDeathRow.front()->isQueued() == 0 ) ) ) {
		pInstr = m_instrumentDeathRow.front();
		m_instrumentDeathRow.pop_front();

		if ( pInstr != nullptr  ) {
			pInstr->unloadSamples();
		}
	}

	if ( m_instrumentDeathRow.size() > 0 ) {
		pInstr = m_instrumentDeathRow.front();
		if ( pInstr != nullptr ) {
			INFOLOG( QString( "Instrument [%1] still has active notes:\n\t%2 " )
					 .arg( pInstr->getName() )
					 .arg( pInstr->getEnqueuedBy().join( "\n\t" ) ) );
		}
	}

#ifdef H2CORE_HAVE_JACK
	if ( hasJackAudioDriver() ) {
		auto pJackAudioDriver = dynamic_cast<JackAudioDriver*>(
			m_pAudioEngine->getAudioDriver());
		if ( pJackAudioDriver != nullptr ) {
			pJackAudioDriver->cleanupPerTrackPorts();
		}
	}
#endif
}



void Hydrogen::panic()
{
	m_pAudioEngine->lock( RIGHT_HERE );
	sequencerStop();
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
			 Preferences::get_instance()->m_nJackTransportMode ==
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
	if ( m_pSong != nullptr && m_pSong->getIsTimelineActivated() &&
		 getMode() == Song::Mode::Song &&
		 ! Preferences::get_instance()->getMidiClockInputHandling() &&
		 getJackTimebaseState() != JackAudioDriver::Timebase::Listener ) {
		return true;
	}

	return false;
}

bool Hydrogen::isPatternEditorLocked() const {
	if ( getMode() == Song::Mode::Song &&
		 m_pSong != nullptr ) {
		if ( m_pSong->getIsPatternEditorLocked() ) {
			return true;
		}
	}

	return false;
}

void Hydrogen::setIsPatternEditorLocked( bool bValue ) {
	if ( m_pSong != nullptr &&
		 bValue != m_pSong->getIsPatternEditorLocked() ) {
		m_pSong->setIsPatternEditorLocked( bValue );
		m_pSong->setIsModified( true );

		updateSelectedPattern();
			
		EventQueue::get_instance()->pushEvent( Event::Type::PatternEditorLocked,
												bValue );
	}
}

Song::Mode Hydrogen::getMode() const {
	if ( m_pSong != nullptr ) {
		return m_pSong->getMode();
	}

	return Song::Mode::None;
}

void Hydrogen::setMode( const Song::Mode& mode, Event::Trigger trigger ) {
	if ( m_pSong != nullptr && mode != m_pSong->getMode() ) {
		m_pSong->setMode( mode );
		if ( trigger != Event::Trigger::Suppress ) {
			EventQueue::get_instance()->pushEvent(
				Event::Type::SongModeActivation, ( mode == Song::Mode::Song) ? 1 : 0 );
		}
	}
	else if ( trigger == Event::Trigger::Force ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::SongModeActivation, ( mode == Song::Mode::Song) ? 1 : 0 );

	}
}

Song::ActionMode Hydrogen::getActionMode() const {
	if ( m_pSong != nullptr ) {
		return m_pSong->getActionMode();
	}
	return Song::ActionMode::None;
}

void Hydrogen::setActionMode( const Song::ActionMode& mode ) {
	if ( m_pSong != nullptr ) {
		m_pSong->setActionMode( mode );
		EventQueue::get_instance()->pushEvent( Event::Type::ActionModeChanged,
												( mode == Song::ActionMode::drawMode ) ? 1 : 0 );
	}
}

Song::PatternMode Hydrogen::getPatternMode() const {
	if ( m_pSong != nullptr && getMode() == Song::Mode::Pattern ) {
		return m_pSong->getPatternMode();
	}
	return Song::PatternMode::None;
}

void Hydrogen::setPatternMode( const Song::PatternMode& mode )
{
	if ( m_pSong != nullptr &&
		 getPatternMode() != mode ) {
		m_pAudioEngine->lock( RIGHT_HERE );

		m_pSong->setPatternMode( mode );
		setIsModified( true );
		
		if ( m_pAudioEngine->getState() != AudioEngine::State::Playing ||
			 mode == Song::PatternMode::Selected ) {
			// Only update the playing patterns in selected pattern
			// mode or if transport is not rolling. In stacked pattern
			// mode with transport rolling
			// AudioEngine::updatePatternTransportPosition() will call
			// the functions and activate the next patterns once the
			// current ones are looped.
			m_pAudioEngine->updatePlayingPatterns( Event::Trigger::Suppress );
			m_pAudioEngine->clearNextPatterns();
		}

		m_pAudioEngine->unlock();
		EventQueue::get_instance()->pushEvent( Event::Type::StackedModeActivation,
												( mode == Song::PatternMode::Selected ) ? 1 : 0 );
	}
}

Hydrogen::Tempo Hydrogen::getTempoSource() const {
	if ( getJackTimebaseState() == JackAudioDriver::Timebase::Listener ) {
		return Tempo::Jack;
	}
	else if ( Preferences::get_instance()->getMidiClockInputHandling() ) {
		return Tempo::Midi;
	}
	else if ( getMode() == Song::Mode::Song &&
			  m_pSong != nullptr && m_pSong->getIsTimelineActivated() ) {
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

	// This function is called in response to altering the OSC port in the
	// preferences dialog and pressing Ok/apply. We want the specified port to
	// be set and overwrite a potential value the user might have provided as
	// CLI argument.
	OscServer::create_instance( -1 );
	
	if ( Preferences::get_instance()->getOscServerEnabled() ) {
		toggleOscServer( true );
	}
#endif
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

void Hydrogen::setIsTimelineActivated( bool bEnabled ) {
	if ( getSong() != nullptr ) {
		auto pPref = Preferences::get_instance();
		auto pAudioEngine = getAudioEngine();

		if ( bEnabled != getSong()->getIsTimelineActivated() ) {
			
			pAudioEngine->lock( RIGHT_HERE );
			
			getSong()->setIsTimelineActivated( bEnabled );

			if ( bEnabled ) {
				getTimeline()->activate();
			} else {
				getTimeline()->deactivate();
			}

			pAudioEngine->handleTimelineChange();
			pAudioEngine->unlock();

			EventQueue::get_instance()->pushEvent( Event::Type::TimelineActivation, static_cast<int>( bEnabled ) );
		}
	}
}

int Hydrogen::getColumnForTick( long nTick, bool bLoopMode, long* pPatternStartTick ) const
{
	std::shared_ptr<Song> pSong = getSong();
	if ( pSong == nullptr ) {
		// Fallback
		const int nPatternSize = 4 * H2Core::nTicksPerQuarter;
		const int nColumn = static_cast<int>(
			std::floor( static_cast<float>( nTick ) /
						static_cast<float>( nPatternSize ) ) );
		*pPatternStartTick = static_cast<long>(nColumn * nPatternSize);
		return nColumn;
	}

	long nTotalTick = 0;

	auto pPatternColumns = pSong->getPatternGroupVector();
	int nColumns = pPatternColumns->size();

	if ( nColumns == 0 ) {
		// There are no patterns in the current song.
		*pPatternStartTick = 0;
		return 0;
	}

	// Sum the lengths of all pattern columns and use four quarters in case some
	// of them are of size zero. If the supplied value nTick is bigger than this
	// and doesn't belong to the next pattern column, we just found the pattern
	// list we were searching for.
	int nPatternSize;
	for ( int i = 0; i < nColumns; ++i ) {
		auto pColumn = ( *pPatternColumns )[ i ];
		if ( pColumn->size() != 0 ) {
			nPatternSize = pColumn->longestPatternLength();
		} else {
			nPatternSize = 4 * H2Core::nTicksPerQuarter;
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
			auto pColumn = ( *pPatternColumns )[ i ];
			if ( pColumn->size() != 0 ) {
				nPatternSize = pColumn->longestPatternLength();
			} else {
				nPatternSize = 4 * H2Core::nTicksPerQuarter;
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
		return static_cast<long>(nColumn * 4 * H2Core::nTicksPerQuarter);
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

	auto pColumns = pSong->getPatternGroupVector();
	long totalTick = 0;
	int nPatternSize;

	for ( int i = 0; i < nColumn; ++i ) {
		auto pColumn = ( *pColumns )[ i ];
		
		if ( pColumn->size() > 0 ) {
			nPatternSize = pColumn->longestPatternLength();
		}
		else {
			nPatternSize = 4 * H2Core::nTicksPerQuarter;
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
	
	if ( m_pSong != nullptr && m_pSong->getDrumkit() != nullptr ) {
		
		m_pAudioEngine->lock( RIGHT_HERE );

		int nSelectedInstrumentNumber = m_nSelectedInstrumentNumber;
		auto pInstrList = m_pSong->getDrumkit()->getInstruments();
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

void Hydrogen::updateVirtualPatterns( Event::Trigger trigger ) {

	if ( m_pSong == nullptr ) {
		ERRORLOG( "no song" );
		return;
	}
	auto pPatternList = m_pSong->getPatternList();
	if ( pPatternList == nullptr ) {
		ERRORLOG( "no pattern list");
		return;
	}
	
	pPatternList->flattenedVirtualPatternsCompute();

	m_pAudioEngine->lock( RIGHT_HERE );
	m_pAudioEngine->updateVirtualPatterns();
	m_pAudioEngine->unlock();

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::PatternModified, 0
		);
	}
}

QString Hydrogen::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Hydrogen]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_pSong: " ).arg( sPrefix ).arg( s ) );
		if ( m_pSong != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pSong->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_fBeatCounterBeatLength: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fBeatCounterBeatLength ) )
			.append( QString( "%1%2m_nBeatCounterTotalBeats: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBeatCounterTotalBeats ) )
			.append( QString( "%1%2m_nBeatCounterEventCount: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBeatCounterEventCount ) )
			.append( QString( "%1%2m_nBeatCounterBeatCount: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBeatCounterBeatCount ) )
			.append( QString( "%1%2m_beatCounterDiffs: [" ).arg( sPrefix ).arg( s ) );
		for ( const auto& dd : m_beatCounterDiffs ) {
			sOutput.append( QString( " %1" ).arg( dd ) );
		}
		sOutput.append( QString( "%1%2m_nBeatCounterDriftCompensation: %3\n" ).arg( sPrefix ).arg( s )
						.arg( m_nBeatCounterDriftCompensation ) )
			.append( QString( "%1%2m_nBeatCounterStartOffset: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBeatCounterStartOffset ) )
			.append( QString( "%1%2m_fTapTempoAverageBpm: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fTapTempoAverageBpm ) )
			.append( QString( "%1%2m_nTapTempoEventsAveraged: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nTapTempoEventsAveraged ) )
			.append( QString( "%1%2m_oldEngineMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( Song::ModeToQString( m_oldEngineMode ) ) )
			.append( QString( "%1%2m_bOldLoopEnabled: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bOldLoopEnabled ) )
			.append( QString( "%1%2m_bExportSessionIsActive: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bExportSessionIsActive ) )
			.append( QString( "%1%2m_GUIState: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( GUIStateToQString( m_GUIState ) ) )
			.append( QString( "%1%2m_pTimeline:\n" ).arg( sPrefix ).arg( s ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_instrumentDeathRow:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ii : m_instrumentDeathRow ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			} else {
				sOutput.append( QString( "nullptr\n" ) );
			}
		}
		sOutput.append( QString( "%1%2m_nSelectedInstrumentNumber: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSelectedInstrumentNumber ) )
			.append( QString( "%1%2m_nSelectedPatternNumber: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSelectedPatternNumber ) )
			.append( QString( "%1%2m_bSessionIsExported: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bSessionIsExported ) )
			.append( QString( "%1%2m_nLastRecordedMIDINoteTick: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nLastRecordedMIDINoteTick ) )
			.append( QString( "%1%2m_bRecordEnabled: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bRecordEnabled ) )
			.append( QString( "%1%2m_pAudioEngine:\n" ).arg( sPrefix ).arg( s ) );
		if ( m_pAudioEngine != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pAudioEngine->toQString( sPrefix + s + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput
			.append( QString( "%1%2m_pSoundLibraryDatabase: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg(
							 m_pSoundLibraryDatabase == nullptr
								 ? "nullptr"
								 : m_pSoundLibraryDatabase->toQString(
									   sPrefix + s, bShort
								   )
						 ) )
			.append( QString( "%1%2m_pPlaylist: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg(
							 m_pPlaylist == nullptr
								 ? "nullptr"
								 : m_pPlaylist->toQString( sPrefix + s, bShort )
						 ) )
			.append( QString( "%1%2m_hihatOpenness: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( static_cast<int>( m_hihatOpenness ) ) )
			.append( QString( "%1%2m_lastMidiEvent: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( MidiEvent::TypeToQString( m_lastMidiEvent ) ) )
			.append( QString( "%1%2m_lastMidiEventParameter: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( static_cast<int>( m_lastMidiEventParameter ) ) );
	}
	else {
		
		sOutput = QString( "%1[Hydrogen]" ).arg( sPrefix )
			.append( QString( ", m_pSong: " ) );
		if ( m_pSong != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pSong->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr" ) );
		}
		sOutput.append( QString( ", m_fBeatCounterBeatLength: %1" ).arg( m_fBeatCounterBeatLength ) )
			.append( QString( ", m_nBeatCounterTotalBeats: %1" ).arg( m_nBeatCounterTotalBeats ) )
			.append( QString( ", m_nBeatCounterEventCount: %1" ).arg( m_nBeatCounterEventCount ) )
			.append( QString( ", m_nBeatCounterBeatCount: %1" ).arg( m_nBeatCounterBeatCount ) )
			.append( QString( ", m_beatCounterDiffs: [" ) );
		for ( const auto& dd : m_beatCounterDiffs ) {
			sOutput.append( QString( " %1" ).arg( dd ) );
		}
		sOutput.append( QString( ", m_nBeatCounterDriftCompensation: %1" )
						 .arg( m_nBeatCounterDriftCompensation ) )
			.append( QString( ", m_nBeatCounterStartOffset: %1" )
					 .arg( m_nBeatCounterStartOffset ) )
			.append( QString( ", m_fTapTempoAverageBpm: %1" )
					 .arg( m_fTapTempoAverageBpm ) )
			.append( QString( ", m_nTapTempoEventsAveraged: %1" )
					 .arg( m_nTapTempoEventsAveraged ) )
			.append( QString( ", m_oldEngineMode: %1" )
					 .arg( Song::ModeToQString( m_oldEngineMode ) ) )
			.append( QString( ", m_bOldLoopEnabled: %1" ).arg( m_bOldLoopEnabled ) )
			.append( QString( ", m_bExportSessionIsActive: %1" ).arg( m_bExportSessionIsActive ) )
			.append( QString( ", m_GUIState: %1" ).
					 arg( GUIStateToQString( m_GUIState ) ) );
		sOutput.append( QString( ", m_pTimeline: " ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr" ) );
		}						 
		sOutput.append( QString( ", m_instrumentDeathRow: [" ) );
		for ( const auto& ii : m_instrumentDeathRow ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			} else {
				sOutput.append( QString( " nullptr" ) );
			}
		}
		sOutput.append( QString( ", m_nSelectedInstrumentNumber: %1" )
						.arg( m_nSelectedInstrumentNumber ) )
			.append( QString( ", m_nSelectedPatternNumber: %1" )
						.arg( m_nSelectedPatternNumber ) )
			.append( QString( ", m_bSessionIsExported: %1" )
						.arg( m_bSessionIsExported ) )
			.append( QString( ", m_nLastRecordedMIDINoteTick: %1" )
						.arg( m_nLastRecordedMIDINoteTick ) )
			.append( QString( ", m_bRecordEnabled: %1" )
						.arg( m_bRecordEnabled ) )
			.append( ", m_pAudioEngine:" );
		if ( m_pAudioEngine != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pAudioEngine->toQString( sPrefix, bShort ) ) );
		} else {
			sOutput.append( QString( " nullptr" ) );
		}
		sOutput.append( ", m_pSoundLibraryDatabase: %1" )
			.append(
				m_pSoundLibraryDatabase == nullptr
					? "nullptr"
					: m_pSoundLibraryDatabase->toQString( "", bShort )
			)
			.append( QString( ", m_pPlaylist: %1" )
						 .arg(
							 m_pPlaylist == nullptr
								 ? "nullptr"
								 : m_pPlaylist->toQString( "", bShort )
						 ) )
			.append( QString( ", m_hihatOpenness: %1" )
						 .arg( static_cast<int>( m_hihatOpenness ) ) )
			.append( QString( ", lastMidiEvent: %1" )
						 .arg( MidiEvent::TypeToQString( m_lastMidiEvent ) ) )
			.append( QString( ", lastMidiEventParameter: %1" )
						 .arg( static_cast<int>( m_lastMidiEventParameter ) ) );
	}
		
	return sOutput;
}

QString Hydrogen::GUIStateToQString( const GUIState& state ) {
	switch( state ) {
	case GUIState::startup:
		return "startup";
	case GUIState::headless:
		return "headless";
	case GUIState::ready:
		return "ready";
	case GUIState::shutdown:
		return "shutdown";
	default:
		return QString( "Unknown GUIState [%1]" )
			.arg( static_cast<int>(state) );
	}
}

}; /* Namespace */
