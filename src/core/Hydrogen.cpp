/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/AudioEngine/Transport.h>
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
#include <core/H2Exception.h>
#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>

#include <atomic>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <core/Helpers/TimeHelper.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/AudioDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/CoreMidiDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/JackDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/IO/StubAudioDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/PortMidiDriver.h>
#include <core/IO/PulseAudioDriver.h>
#include <core/IO/SoftwareDriver.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Timeline.h>

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

Hydrogen::Hydrogen(
	std::shared_ptr<Preferences> pPref,
	ProcessMode processMode,
	int nOscPort
)
	: m_fBeatCounterBeatLength( 1 ),
	  m_nBeatCounterTotalBeats( 4 ),
	  m_nBeatCounterEventCount( 1 ),
	  m_nBeatCounterBeatCount( 1 ),
	  m_lastBeatCounterTimePoint( TimePoint() ),
	  m_lastTapTempoTimePoint( TimePoint() ),
	  m_fTapTempoAverageBpm( MIN_BPM ),
	  m_nTapTempoEventsAveraged( 0 ),
	  m_nSelectedInstrumentNumber( 0 ),
	  m_nSelectedPatternNumber( 0 ),
	  m_bExportSessionIsActive( false ),
	  m_ProcessMode( processMode ),
	  m_bIsFullyOperational( false ),
	  m_lastMidiEvent( MidiEvent::Type::Null ),
	  m_lastMidiEventParameter( Midi::ParameterInvalid ),
	  m_oldEngineMode( Song::Mode::Song ),
	  m_bOldLoopEnabled( false ),
	  m_nLastRecordedMIDINoteTick( 0 ),
	  m_bRecordEnabled( false ),
	  m_hihatOpenness( Midi::ParameterMaximum ),
	  m_pPreferences( pPref ),
	  m_interpolateModeOverride( Interpolation::InterpolateMode::Linear ),
	  m_bUseInterpolateModeOverride( false ),
	  m_pEventQueue( nullptr ),
	  m_bCachedUnderSessionManagement( false ),
	  m_bCachedUnderPluginHost( false ),
	  m_bInstanceLoggerSpawned( false )
{
	// This instance owns its Preferences and EventQueue (ADR 0015); no
	// process-wide singleton is involved.
	m_pEventQueue = new EventQueue( this );

	// Per-instance Logger (ADR 0015, T1.6): its own queue/worker/log file, with
	// a path made unique per process+instance (pid + counter). Mirrors the
	// process-default logger's stdout/colour settings so console verbosity is
	// unchanged; the default logger remains the unscoped/static fallback. The
	// instance entry points (e.g. the audio process callback) wrap work in a
	// Logger::Scope( getLogger() ) so logging routes here.
	static std::atomic<int> nInstanceCounter { 0 };
	const QFileInfo defaultLogInfo( Filesystem::logFilePath() );
	if ( pPref->m_audioDriver == Preferences::AudioDriver::Plugin ) {
		// Only in case Hydrogen was instantiated by a plugin host, we use a
		// custom log file.
		const QString sInstanceLogPath =
			defaultLogInfo.absolutePath() + "/" +
			defaultLogInfo.completeBaseName() +
			QString( "_%1_%2." )
				.arg( QCoreApplication::applicationPid() )
				.arg( nInstanceCounter++ ) +
			defaultLogInfo.suffix();
		// No Scope is active during construction, so currentLogger() resolves
		// to the process-default logger — read its settings to mirror console
		// verbosity.
		const auto pDefaultLogger = Logger::currentLogger();
		m_pLogger = Logger::createInstanceLogger(
			sInstanceLogPath,
			pDefaultLogger != nullptr ? pDefaultLogger->getUseStdout() : false,
			false,
			pDefaultLogger != nullptr ? pDefaultLogger->getLogColors() : true
		);
		INFOLOG( QString( "Spawning instance logger backed by [%1]" )
					 .arg( sInstanceLogPath ) );
		m_bInstanceLoggerSpawned = true;
	}
	else {
		m_pLogger = Logger::currentLogger();
	}

#ifdef H2CORE_HAVE_OSC
	// OSC server + NSM client are owned per-instance (ADR 0015). The OscServer
	// only binds a port when OSC is enabled in this instance's Preferences, so
	// multiple instances coexist (disabled ones never touch the network).
	m_pNsmClient = new NsmClient( this );
	m_pOscServer = new OscServer( this, nOscPort );
#endif

	m_nBeatCounterDriftCompensation = pPref->m_nBeatCounterDriftCompensation;
	m_nBeatCounterStartOffset = pPref->m_nBeatCounterStartOffset;
	m_beatCounterDiffs.resize( 16 );

	m_pSoundLibraryDatabase = std::make_shared<SoundLibraryDatabase>( this );
	m_pSong = Song::getEmptySong( this, m_pSoundLibraryDatabase );

	m_pAudioEngine = new AudioEngine( this );
	m_pMidiActionManager = std::make_shared<MidiActionManager>( this );
	m_pCoreActionController = std::make_shared<CoreActionController>( this );
	m_pPlaylist = std::make_shared<Playlist>();
	m_pTimeHelper = std::make_shared<TimeHelper>();

	m_pAudioEngine->startAudioDriver( Event::Trigger::Default );
	m_pAudioEngine->startMidiDriver( Event::Trigger::Default );

	// The OSC server is disabled when running as a plugin: the host owns control
	// surfaces and network endpoints (ADR 0026).
	if ( pPref->getOscServerEnabled() && ! isUnderPluginHost() ) {
		toggleOscServer( true );
	}
}

Hydrogen::~Hydrogen()
{
	INFOLOG( "[~Hydrogen]" );

	// We reuse this member to indicate shutdown as well.
	m_bIsFullyOperational = false;

#ifdef H2CORE_HAVE_OSC
	// This instance owns its OSC server and NSM client (ADR 0015).
	if ( m_pNsmClient != nullptr ) {
		m_pNsmClient->shutdown();
		delete m_pNsmClient;
		m_pNsmClient = nullptr;
	}
	if ( m_pOscServer != nullptr ) {
		delete m_pOscServer;
		m_pOscServer = nullptr;
	}
#endif

	m_pAudioEngine->lock( RIGHT_HERE );
	m_pAudioEngine->prepare( Event::Trigger::Suppress );
	m_pAudioEngine->unlock();

	killInstruments();

	delete m_pAudioEngine;

	// This instance owns its EventQueue; tear it down last (after the engine,
	// which may still emit events during teardown).
	delete m_pEventQueue;
	m_pEventQueue = nullptr;

	// In case we created a logger for this very instance, we also have to tear
	// down its custom Logger instance. We also need to clean up the custom log
	// file. Since the process id is used in the file name, we would clutter up
	// disk space otherwise.
	if ( m_bInstanceLoggerSpawned ) {
		const auto sInstanceLogFile = m_pLogger->getLogFile();
		delete m_pLogger;
		Filesystem::rm( sInstanceLogFile );
	}
}

Hydrogen* Hydrogen::create_instance(
	int nOscPort,
	std::shared_ptr<Preferences> pPreferences,
	ProcessMode processMode
)
{
	// Standalone factory (ADR 0015): construct a Hydrogen owning the provided
	// Preferences. The caller owns the returned instance and is responsible for
	// deleting it. No process-wide singleton is registered.
	Logger::create_instance();
	return new Hydrogen( pPreferences, processMode, nOscPort );
}

/// Start the internal sequencer
void Hydrogen::sequencerPlay()
{
	m_pAudioEngine->play();
}

/// Stop the internal sequencer
void Hydrogen::sequencerStop()
{
	m_pAudioEngine->stop();
	m_pCoreActionController->activateRecordMode( false );

	// Delete redundant instruments still alive after switching the
	// drumkit to a smaller one.
	killInstruments();
}

void Hydrogen::loadPlaybackTrack( const QString& sFileName )
{
	if ( m_pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	const auto pSample = Sample::load( sFileName );
	if ( pSample == nullptr ) {
		ERRORLOG(
			QString( "Failed to load [%1]. Could not update playback track." )
				.arg( sFileName )
		);
		return;
	}

	const auto pInstrument = Instrument::from( pSample, this );
	if ( pInstrument != nullptr ) {
		pInstrument->setName( "PlaybackTrack" );
		pInstrument->setId( Instrument::PlaybackTrackId );
		pInstrument->loadSamples( m_pAudioEngine->getPlayhead()->getBpm(),
								  getPreferences().get() );
	}

	m_pSong->setPlaybackTrackInstrument( pInstrument );

	m_pEventQueue->pushEvent(
		Event::Type::PlaybackTrackChanged, 0
	);
}

void Hydrogen::setSong( std::shared_ptr<Song> pSong )
{
	if ( pSong == nullptr ) {
		WARNINGLOG( "setting nullptr!" );
	}

	std::shared_ptr<Song> pCurrentSong = getSong();

	m_pAudioEngine->lock( RIGHT_HERE );

	// Move to the beginning.
	setSelectedPatternNumber( 0, false, Event::Trigger::Suppress );

	if ( pCurrentSong != nullptr ) {
		if ( isUnderSessionManagement() ) {
#ifdef H2CORE_HAVE_OSC
			if ( pCurrentSong->getPath().contains(
					 m_pNsmClient->getSessionFolderPath() ) ) {
				// When under session management Hydrogen is only allowed to
				// replace the content of the session song but not to write to a
				// different location.
				if ( pSong != nullptr ) {
					pSong->setPath( pCurrentSong->getPath() );
				}
			}
#endif
		}
		m_pAudioEngine->prepare( Event::Trigger::Suppress );

		if ( pCurrentSong->getDrumkit() != nullptr ) {
			if ( ! ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
					 pCurrentSong->getDrumkit()->getUuid() ==
					 pSong->getDrumkit()->getUuid() ) ) {
				// Only unload samples in case we do load a new drumkit.
				pCurrentSong->getDrumkit()->unloadSamples();
			}
		}
	}

	renamePerTrackJackAudioPorts( pSong, m_pSong != nullptr ? m_pSong->getDrumkit() : nullptr );

	m_pSong = pSong;
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 !pSong->getDrumkit()->areSamplesLoaded() ) {
		pSong->getDrumkit()->loadSamples( 120, getPreferences().get() );
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

	m_pAudioEngine->unlock();

	// Push current state of Hydrogen to attached control interfaces,
	// like OSC clients.
	m_pCoreActionController->initExternalControlInterfaces();
}

void Hydrogen::midiNoteOn( std::shared_ptr<Note> pNote )
{
	ASSERT_NO_EDITOR_MODE( this );

	m_pAudioEngine->noteOn( pNote );
}

bool Hydrogen::addRealtimeNote(
	int nInstrument,
	float fVelocity,
	bool bNoteOff,
	Midi::Note note
)
{
	ASSERT_NO_EDITOR_MODE( this );

	AudioEngine* pAudioEngine = m_pAudioEngine;
	auto pSampler = pAudioEngine->getSampler();
	const auto pPref = m_pPreferences;
	unsigned int nRealColumn = 0;
	unsigned res = pPref->getPatternEditorGridResolution();
	int nBase = pPref->isPatternEditorUsingTriplets() ? 3 : 4;
	const bool bPlaySelectedInstrument = pPref->getMidiInstrumentMap()->getInput() ==
		MidiInstrumentMap::Input::SelectedInstrument;
	int scalar = ( 4 * 4 * H2Core::nTicksPerQuarter ) / ( res * nBase );
	int nCurrentPatternNumber;

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
		int nColumn = pAudioEngine->getPlayhead()->getColumn(); // current column
		// or pattern group
		if ( nColumn < 0 || nColumn >= pColumns->size() ) {
			pAudioEngine->unlock(); // unlock the audio engine
			ERRORLOG( QString( "Provided column [%1] out of bound [%2,%3)" )
					  .arg( nColumn ).arg( 0 )
					  .arg( pColumns->size() ) );
			return false;
		}
		// Locate nTickInPattern -- may need to jump back one column
		nTickInPattern = pAudioEngine->getPlayhead()->getPatternTickPosition();

		// Capture new notes in the bottom-most pattern (if not already done above)
		auto pColumn = ( *pColumns )[ nColumn ];
		nCurrentPatternNumber = -1;
		for ( int n = 0; n < pColumn->size(); n++ ) {
			auto pPattern = pColumn->get( n );
			int nIndex = pPatternList->index( pPattern );
			if ( nIndex > nCurrentPatternNumber ) {
				nCurrentPatternNumber = nIndex;
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
			nCurrentPatternNumber = m_nSelectedPatternNumber;
		}

		if ( ! pCurrentPattern ) {
			ERRORLOG( "Current pattern invalid" );
			pAudioEngine->unlock(); // unlock the audio engine
			return false;
		}

		// Locate nTickInPattern -- may need to wrap around end of pattern
		nTickInPattern = pAudioEngine->getPlayhead()->getPatternTickPosition();
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
				 .arg( nCurrentPatternNumber ).arg( pCurrentPattern->getName() )
				 .arg( nTickInPattern ).arg( pCurrentPattern->getLength() ) );

		if ( bNoteOff ) {
            // Handle the Note-Off event corresponding to the previous Note-On.
            // This is used to record notes of custom lengths.
			const int nPatternSize = pCurrentPattern->getLength();
			const int nCurrentTick = static_cast<int>(
				pAudioEngine->getPlayhead()->getPatternTickPosition()
			);

			int nNoteLength;
			if ( nCurrentTick < m_nLastRecordedMIDINoteTick ) {
				// BUG: We passed the boundary between to patterns or
				// transported got looped. As we do not support the notion of
				// custom note lengths reaching from one pattern into the next
				// one, we trim it at the end of the pattern instead.
				nNoteLength = nPatternSize - m_nLastRecordedMIDINoteTick;

				// We also omit pitch-related rescaling as we do not know the
                // true length of the note (transport could have been wrapped
                // multiple times).
			}
			else {
				nNoteLength =
					static_cast<int>( pAudioEngine->getPlayhead()
										  ->getPatternTickPosition() ) -
					m_nLastRecordedMIDINoteTick;
			}

			bool bPatternModified = false;
			for ( unsigned nnNote = 0; nnNote < nPatternSize; nnNote++ ) {
				const Pattern::notes_t* notes = pCurrentPattern->getNotes();
				FOREACH_NOTE_CST_IT_BOUND_LENGTH(
					notes, it, nnNote, pCurrentPattern
				)
				{
					auto pNote = it->second;
					if ( pNote != nullptr &&
						 pNote->getPosition() == m_nLastRecordedMIDINoteTick &&
						 sameObject( pInstrument, pNote->getInstrument() ) ) {
						int nNewNoteLength = nNoteLength;
						if ( m_nLastRecordedMIDINoteTick + nNoteLength >
							 nPatternSize ) {
							nNewNoteLength =
								nPatternSize - m_nLastRecordedMIDINoteTick;
						}
						pNote->setLength( nNewNoteLength );
						bPatternModified = true;
					}
				}
			}

			if ( bPatternModified && ! pCurrentPattern->getIsModified() ) {
				m_pEventQueue->pushEvent(
					Event::Type::PatternChanged, -1
				);
				setPatternModified( true, nCurrentPatternNumber );
			}
		}
		else { // note on
			EventQueue::AddMidiNoteVector noteAction;
			noteAction.nColumn = nTickInPattern;
			noteAction.id = instrumentId;
			noteAction.nPattern = nCurrentPatternNumber;
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

			m_pEventQueue->m_addMidiNoteVector.push_back(noteAction);

			m_nLastRecordedMIDINoteTick = nTickInPattern;
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
		m_pEventQueue->pushEvent( Event::Type::NextPatternsChanged, 0 );

	} else {
		ERRORLOG( "can't set next pattern in song mode" );
	}
}

bool Hydrogen::flushAndAddNextPattern( int nPatternNumber ) {
	if ( m_pSong != nullptr && getMode() == Song::Mode::Pattern ) {
		m_pAudioEngine->lock( RIGHT_HERE );
		m_pAudioEngine->flushAndAddNextPattern( nPatternNumber );
		m_pAudioEngine->unlock();
		m_pEventQueue->pushEvent( Event::Type::NextPatternsChanged, 0 );

		return true;

	} else {
		ERRORLOG( "can't set next pattern in song mode" );
	}

	return false;
}

void Hydrogen::restartAudioDriver() {
	const bool bWasPlaying =
		m_pAudioEngine->getState() == AudioEngine::State::Playing;

	bool bCombinedDriver = false;
#ifdef H2CORE_HAVE_JACK
	{
		// Ensure this shared pointer instance does not outlive
		// stopAudioDriver().
		auto pJackDriver = std::dynamic_pointer_cast<JackDriver>(
			m_pAudioEngine->getAudioDriver()
		);
		if ( pJackDriver != nullptr &&
			 pJackDriver->getMode() == JackDriver::Mode::Combined ) {
			bCombinedDriver = true;
		}
	}
#endif

	m_pAudioEngine->stopAudioDriver( Event::Trigger::Suppress );
	m_pAudioEngine->startAudioDriver( Event::Trigger::Default );

	if ( bCombinedDriver && m_pPreferences->m_audioDriver !=
		 Preferences::AudioDriver::Jack ) {
		// We stopped a combined MIDI and audio driver. But the user shows to
		// use separate ones instead. We have to ensure the audio driver is
		// properly restarted.
		m_pAudioEngine->startMidiDriver( Event::Trigger::Default );
	}

	if ( bWasPlaying ) {
		m_pAudioEngine->startPlayback();
	}
}

void Hydrogen::restartMidiDriver() {
	bool bCombinedDriver = false;
#ifdef H2CORE_HAVE_JACK
	{
		// Ensure this shared pointer instance does not outlive
		// stopMidiDriver().
		auto pJackDriver = std::dynamic_pointer_cast<JackDriver>(
			m_pAudioEngine->getMidiDriver()
		);
		if ( pJackDriver != nullptr &&
			 pJackDriver->getMode() == JackDriver::Mode::Combined ) {
			bCombinedDriver = true;
		}
	}
#endif

	m_pAudioEngine->stopMidiDriver( Event::Trigger::Suppress );
	m_pAudioEngine->startMidiDriver( Event::Trigger::Default );

	if ( bCombinedDriver && m_pPreferences->m_midiDriver !=
		 Preferences::MidiDriver::Jack ) {
		// We stopped a combined MIDI and audio driver. But the user shows to
		// use separate ones instead. We have to ensure the audio driver is
		// properly restarted.
		m_pAudioEngine->startAudioDriver( Event::Trigger::Default );
	}
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

	auto pDriver = pAudioEngine->createAudioDriver(
		Preferences::AudioDriver::Disk, Event::Trigger::Default );

	auto pDiskWriterDriver =
		std::dynamic_pointer_cast<DiskWriterDriver>( pDriver );
	if ( pDriver == nullptr || pDiskWriterDriver == nullptr ) {
		ERRORLOG( "Unable to start up DiskWriterDriver" );
		return false;
	}

	pDiskWriterDriver->setSampleRate( static_cast<unsigned>(nSampleRate) );
	pDiskWriterDriver->setSampleDepth( nSampleDepth );
	pDiskWriterDriver->setCompressionLevel( fCompressionLevel );

	m_bExportSessionIsActive = true;

	return true;
}

/// Export a song to a wav file
void Hydrogen::startExportSong( const QString& sFileName,
								const std::vector<Uuid>& excludedInstruments )
{
	AudioEngine* pAudioEngine = m_pAudioEngine;

	// Arm the instruments for this export (ADR 0027): every instrument is
	// exported unless its identity is in the exclusion list (per-instrument /
	// track-out exports). The Sampler honours this only while an export session
	// is active.
	auto pSong = getSong();
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		auto pInstrumentList = pSong->getDrumkit()->getInstruments();
		for ( int ii = 0; ii < pInstrumentList->size(); ++ii ) {
			auto pInstr = pInstrumentList->get( ii );
			if ( pInstr == nullptr ) {
				continue;
			}
			const bool bExcluded = std::find(
				excludedInstruments.begin(), excludedInstruments.end(),
				pInstr->getUuid() ) != excludedInstruments.end();
			pInstr->setCurrentlyExported( ! bExcluded );
		}
	}

	m_pCoreActionController->locateToTick( 0 );
	pAudioEngine->play();
	pAudioEngine->getSampler()->stopPlayingNotes();

	auto pDiskWriterDriver =
		std::dynamic_pointer_cast<DiskWriterDriver>( pAudioEngine->getAudioDriver() );
	pDiskWriterDriver->setFileName( sFileName );
	pDiskWriterDriver->write();
}

void Hydrogen::stopExportSong()
{
	AudioEngine* pAudioEngine = m_pAudioEngine;
	pAudioEngine->getSampler()->stopPlayingNotes();
	m_pCoreActionController->locateToTick( 0 );
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
std::shared_ptr<AudioDriver> Hydrogen::getAudioDriver() const
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

	m_pCoreActionController->setBpm( m_fTapTempoAverageBpm );
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
			m_pEventQueue->pushEvent(
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
		m_pEventQueue->pushEvent( Event::Type::SelectedPatternChanged, -1 );
	}
}

void Hydrogen::setSelectedInstrumentNumber( int nInstrument,
											Event::Trigger trigger )
{
	// In case no instrument is selected (-1), we still perform an update since
	// another type-only row might be selected in the GUI.
	if ( m_nSelectedInstrumentNumber == nInstrument ) {
		if ( trigger == Event::Trigger::Force ) {
			m_pEventQueue->pushEvent(
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
		m_pEventQueue->pushEvent(
			Event::Type::SelectedInstrumentChanged, -1 );
	}
}

void Hydrogen::renamePerTrackJackAudioPorts( std::shared_ptr<Song> pSong,
								std::shared_ptr<Drumkit> pOldDrumkit )
{
#ifdef H2CORE_HAVE_JACK
	if ( pSong == nullptr ) {
		return;
	}

	if ( m_pPreferences->m_bJackTrackOuts == true && hasJackDriver() ) {
		// When restarting the audio driver after loading a new song under
		// Non session management all ports have to be registered _prior_
		// to the activation of the client.
		if ( isUnderSessionManagement() && !isFullyOperational() ) {
			return;
		}

		m_pAudioEngine->createPerTrackJackAudioPorts( pSong, pOldDrumkit );
	}
#endif
}

void Hydrogen::setBeatCounterTotalBeats( int nBeatsToCount ) {
	if ( m_nBeatCounterTotalBeats != nBeatsToCount ) {
		m_nBeatCounterTotalBeats = nBeatsToCount;
		m_pEventQueue->pushEvent( Event::Type::BeatCounter, 0 );
	}
}

void Hydrogen::setBeatCounterBeatLength( float fBeatLength ) {
	if ( m_fBeatCounterBeatLength != fBeatLength ) {
		m_fBeatCounterBeatLength = fBeatLength;
		m_pEventQueue->pushEvent( Event::Type::BeatCounter, 0 );
	}
}

bool Hydrogen::handleBeatCounter( TimePoint start )
{
	if ( getTempoSource() != Tempo::Song ) {
		return false;
	}

	auto pEventQueue = m_pEventQueue;

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
			
		if ( m_pCoreActionController->setBpm( fBeatCountBpm ) ) {
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

	if ( bTempoSet && m_pPreferences->m_beatCounter ==
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
	const auto pPreferences = m_pPreferences;

	m_nBeatCounterDriftCompensation =
		pPreferences->m_nBeatCounterDriftCompensation;
	m_nBeatCounterStartOffset = pPreferences->m_nBeatCounterStartOffset;

	m_pEventQueue->pushEvent( Event::Type::BeatCounter, 0 );
}

void Hydrogen::addInstrumentToDeathRow( std::shared_ptr<Instrument> pInstr )
{
	m_instrumentDeathRow.push_back( pInstr );
	killInstruments();
}

void Hydrogen::removeInstrumentFromDeathRow( std::shared_ptr<Instrument> pInstr ) {
	for ( auto it = m_instrumentDeathRow.begin();
		  it != m_instrumentDeathRow.end(); ) {
        if ( sameObject( *it, pInstr ) ) {
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

	if ( m_ProcessMode == ProcessMode::Editor ) {
		// The mirror engine has no access to the actual audio drivers.
		return;
	}

#ifdef H2CORE_HAVE_JACK
	if ( hasJackDriver() ) {
		auto pJackDriver = std::dynamic_pointer_cast<JackDriver>(
			m_pAudioEngine->getAudioDriver()
		);
		if ( pJackDriver != nullptr ) {
			pJackDriver->cleanUpPerTrackAudioPorts();
		}
	}
#endif
}

bool Hydrogen::hasJackDriver() const
{
	// In editor mode the mirror has no real audio driver; consult the
	// IPC-cached AudioDriverInfo instead (ADR 0029).
	if ( m_ProcessMode == ProcessMode::Editor ) {
		return m_cachedAudioDriverInfo.kind == Preferences::AudioDriver::Jack;
	}

#ifdef H2CORE_HAVE_JACK
	if ( m_pAudioEngine->getAudioDriver() != nullptr ) {
		if ( std::dynamic_pointer_cast<JackDriver>(
				 m_pAudioEngine->getAudioDriver()
			 ) != nullptr ) {
			return true;
		}
	}
	return false;
#else
	return false;
#endif
}

bool Hydrogen::hasJackTransport() const
{
	// In editor mode the mirror has no real audio driver; consult the
	// IPC-cached AudioDriverInfo instead (ADR 0029).
	if ( m_ProcessMode == ProcessMode::Editor ) {
		return m_cachedAudioDriverInfo.jackTransportEnabled;
	}

#ifdef H2CORE_HAVE_JACK
	if ( m_pAudioEngine->getAudioDriver() != nullptr ) {
		if ( std::dynamic_pointer_cast<JackDriver>(
				 m_pAudioEngine->getAudioDriver()
			 ) != nullptr &&
			 m_pPreferences->m_nJackTransportMode ==
				 Preferences::USE_JACK_TRANSPORT ) {
			return true;
		}
	}
	return false;
#else
	return false;
#endif
}

JackDriver::Timebase Hydrogen::getJackTimebaseState() const
{
	// In editor mode the mirror has no real audio driver; consult the
	// IPC-cached AudioDriverInfo instead (ADR 0029).
	if ( m_ProcessMode == ProcessMode::Editor ) {
		return m_cachedAudioDriverInfo.timebaseState;
	}

#ifdef H2CORE_HAVE_JACK
	AudioEngine* pAudioEngine = m_pAudioEngine;
	if ( hasJackTransport() ) {
		return std::dynamic_pointer_cast<JackDriver>(
				   pAudioEngine->getAudioDriver()
		)
			->getTimebaseState();
	}
	return JackDriver::Timebase::None;
#else
	return JackDriver::Timebase::None;
#endif
}

AudioDriverInfo Hydrogen::getAudioDriverInfo() const {
	AudioDriverInfo info;
	const auto pDriver = m_pAudioEngine->getAudioDriver();
	if ( pDriver == nullptr ) {
		return info;
	}
	info.isPresent = true;
	info.isRunning =
		std::dynamic_pointer_cast<StubAudioDriver>( pDriver ) == nullptr;

	// The consolidated software driver (ADR 0031) clocks the engine but may be
	// headless (no real output). Its producesAudio flag is the authoritative
	// "running" signal: false → headless (former Null), reported as kind Null.
	if ( const auto pSw = std::dynamic_pointer_cast<SoftwareDriver>( pDriver ) ) {
		info.isRunning = pSw->getProducesAudio();
		info.kind = pSw->getProducesAudio() ?
			Preferences::AudioDriver::Fake : Preferences::AudioDriver::Null;
		return info;
	}

	// Resolve the concrete kind. Each cast is guarded by the same H2CORE_HAVE_*
	// flag the GUI used, so a selected-but-not-compiled backend falls through to
	// Null below.
#ifdef H2CORE_HAVE_JACK
	if ( std::dynamic_pointer_cast<JackDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::Jack;
		info.timebaseState = getJackTimebaseState();
		info.jackTransportEnabled =
			m_pPreferences->m_nJackTransportMode ==
				Preferences::USE_JACK_TRANSPORT;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_ALSA
	if ( const auto pAlsa =
			 std::dynamic_pointer_cast<AlsaAudioDriver>( pDriver ) ) {
		info.kind = Preferences::AudioDriver::Alsa;
		info.connectedDevice = pAlsa->m_sAlsaAudioDevice;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_PORTAUDIO
	if ( std::dynamic_pointer_cast<PortAudioDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::PortAudio;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_COREAUDIO
	if ( std::dynamic_pointer_cast<CoreAudioDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::CoreAudio;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_PULSEAUDIO
	if ( std::dynamic_pointer_cast<PulseAudioDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::PulseAudio;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_OSS
	if ( std::dynamic_pointer_cast<OssDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::Oss;
		return info;
	}
#endif
	if ( std::dynamic_pointer_cast<DiskWriterDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::Disk;
		return info;
	}

	// Present but unmatched: the StubAudioDriver fallback (or a backend not
	// surfaced in the UI, e.g. the plugin/fake drivers).
	info.kind = Preferences::AudioDriver::Null;
	return info;
}

const AudioDriverInfo& Hydrogen::getCachedAudioDriverInfo() const {
	return m_cachedAudioDriverInfo;
}

void Hydrogen::setCachedAudioDriverInfo( const AudioDriverInfo& info ) {
	m_cachedAudioDriverInfo = info;
}

MidiDriverInfo Hydrogen::getMidiDriverInfo() const {
	MidiDriverInfo info;
	const auto pDriver = m_pAudioEngine->getMidiDriver();
	if ( pDriver == nullptr ) {
		return info;
	}
	info.isPresent = true;
	info.isInputActive = pDriver->isInputActive();
	info.isOutputActive = pDriver->isOutputActive();
	return info;
}

const MidiDriverInfo& Hydrogen::getCachedMidiDriverInfo() const {
	return m_cachedMidiDriverInfo;
}

void Hydrogen::setCachedMidiDriverInfo( const MidiDriverInfo& info ) {
	m_cachedMidiDriverInfo = info;
}

bool Hydrogen::isUnderSessionManagement() const
{
	// In editor mode the mirror has no NsmClient; return the IPC-cached
	// value (ADR 0029).
	if ( m_ProcessMode == ProcessMode::Editor ) {
		return m_bCachedUnderSessionManagement;
	}

#ifdef H2CORE_HAVE_OSC
	ASSERT_NO_EDITOR_MODE( this );
	if ( m_pNsmClient != nullptr ) {
		if ( m_pNsmClient->getUnderSessionManagement() ) {
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

void Hydrogen::setCachedUnderSessionManagement( bool bValue ) {
	m_bCachedUnderSessionManagement = bValue;
}

bool Hydrogen::isTimelineEnabled() const {
	// In editor mode this intentionally returns true when the Timeline is
	// activated in Song mode. Even though tempo is controlled by the remote
	// engine (Tempo::Remote), the mirror needs Timeline-based frame/tick
	// conversion (Transport.cpp) and Timeline-aware GUI rendering
	// (SongEditorPositionRuler) to stay consistent with the authoritative
	// engine. The mirror has the Timeline data (synced via SetSong IPC).
	if ( m_pSong != nullptr && m_pSong->getIsTimelineActivated() &&
		 getMode() == Song::Mode::Song &&
		 ! m_pPreferences->getMidiClockInputHandling() &&
		 getJackTimebaseState() != JackDriver::Timebase::Listener ) {
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
		setSongModified( true );

		updateSelectedPattern();
			
		m_pEventQueue->pushEvent( Event::Type::PatternEditorLocked,
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
			m_pEventQueue->pushEvent(
				Event::Type::SongModeActivation, ( mode == Song::Mode::Song) ? 1 : 0 );
		}
	}
	else if ( trigger == Event::Trigger::Force ) {
		m_pEventQueue->pushEvent(
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
		m_pEventQueue->pushEvent( Event::Type::ActionModeChanged,
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
		setSongModified( true );
		
		if ( m_pAudioEngine->getState() != AudioEngine::State::Playing ||
			 mode == Song::PatternMode::Selected ) {
			// Only update the playing patterns in selected pattern
			// mode or if transport is not rolling. In stacked pattern
			// mode with transport rolling
			// AudioEngine::updatePatternTransport() will call
			// the functions and activate the next patterns once the
			// current ones are looped.
			m_pAudioEngine->updatePlayingPatterns( Event::Trigger::Suppress );
			m_pAudioEngine->clearNextPatterns();
		}

		m_pAudioEngine->unlock();
		m_pEventQueue->pushEvent( Event::Type::StackedModeActivation,
												( mode == Song::PatternMode::Selected ) ? 1 : 0 );
	}
}

bool Hydrogen::isUnderPluginHost() const {
	// In editor mode the mirror's audio driver is Null (forced by
	// configureMirrorPreferences); return the IPC-cached value instead
	// (ADR 0029).
	if ( m_ProcessMode == ProcessMode::Editor ) {
		return m_bCachedUnderPluginHost;
	}

	ASSERT_NO_EDITOR_MODE( this );
	return m_pPreferences != nullptr &&
		m_pPreferences->m_audioDriver == Preferences::AudioDriver::Plugin;
}

void Hydrogen::setCachedUnderPluginHost( bool bValue ) {
	m_bCachedUnderPluginHost = bValue;
}

Interpolation::InterpolateMode Hydrogen::getInterpolateMode() const {
	if ( m_bUseInterpolateModeOverride ) {
		return m_interpolateModeOverride;
	}
	if ( m_pPreferences != nullptr ) {
		return m_pPreferences->m_interpolateMode;
	}
	return Interpolation::InterpolateMode::Linear;
}

void Hydrogen::setInterpolateModeOverride( Interpolation::InterpolateMode mode ) {
	m_interpolateModeOverride = mode;
	m_bUseInterpolateModeOverride = true;
}

void Hydrogen::clearInterpolateModeOverride() {
	m_bUseInterpolateModeOverride = false;
}

Hydrogen::Tempo Hydrogen::getTempoSource( bool bAuthoritativeEngine ) const {
	if ( ! bAuthoritativeEngine &&
		 getProcessMode() == H2Core::ProcessMode::Editor ) {
		// The mirror engine does not own any audio/MIDI drivers or external
		// tempo sources. Tempo is controlled by the authoritative engine and
		// followed via telemetry + IPC events (ADR 0016/0030).
		return Tempo::Remote;
	}

	if ( isUnderPluginHost() ) {
		// The host transport owns tempo; it wins over the Timeline (ADR 0013).
		return Tempo::Plugin;
	}
	if ( getJackTimebaseState() == JackDriver::Timebase::Listener ) {
		return Tempo::Jack;
	}
	else if ( m_pPreferences->getMidiClockInputHandling() ) {
		return Tempo::Midi;
	}
	else if ( getMode() == Song::Mode::Song &&
			  m_pSong != nullptr && m_pSong->getIsTimelineActivated() ) {
		return Tempo::Timeline;
	}

	return Tempo::Song;
}

void Hydrogen::toggleOscServer( bool bEnable ) {
	// No OSC server under a plugin host (ADR 0026): ignore enable requests.
	if ( isUnderPluginHost() || m_ProcessMode == ProcessMode::Editor ) {
		return;
	}
#ifdef H2CORE_HAVE_OSC
	if ( bEnable ) {
		m_pOscServer->start();
	} else {
		m_pOscServer->stop();
	}
#endif
}

void Hydrogen::recreateOscServer() {
#ifdef H2CORE_HAVE_OSC
	if ( m_pOscServer != nullptr ) {
		delete m_pOscServer;
	}

	// This function is called in response to altering the OSC port in the
	// preferences dialog and pressing Ok/apply. We want the specified port to
	// be set and overwrite a potential value the user might have provided as
	// CLI argument.
	m_pOscServer = new OscServer( this, -1 );

	if ( m_pPreferences->getOscServerEnabled() ) {
		toggleOscServer( true );
	}
#endif
}

void Hydrogen::setDrumkitModified( bool bIsModified )
{
	if ( m_pSong == nullptr || m_pSong->getDrumkit() == nullptr ) {
		return;
	}

	if ( bIsModified && ! m_pSong->getIsModified() ) {
		setSongModified( true );
	}

	if ( m_pSong->getDrumkit()->getIsModified() == bIsModified ) {
		return;
	}

	m_pSong->getDrumkit()->setIsModified( bIsModified );

	m_pEventQueue->pushEvent( Event::Type::DrumkitIsModified, -1 );
}

void Hydrogen::setPatternModified( bool bIsModified, int nIndex )
{
	if ( m_pSong == nullptr ) {
		return;
	}

	auto pPattern = m_pSong->getPatternList()->get( nIndex );
	if ( pPattern == nullptr ) {
		return;
	}

	if ( bIsModified && ! m_pSong->getIsModified() ) {
		setSongModified( true );
	}

	if ( pPattern->getIsModified() == bIsModified ) {
		return;
	}

	pPattern->setIsModified( bIsModified );

	m_pEventQueue->pushEvent(
		Event::Type::PatternIsModified, nIndex
	);
}

void Hydrogen::setSongModified( bool bIsModified )
{
	if ( m_pSong == nullptr || m_pSong->getIsModified() == bIsModified ) {
		return;
	}

	m_pSong->setIsModified( bIsModified );

	m_pEventQueue->pushEvent( Event::Type::SongIsModified, -1 );

#ifdef H2CORE_HAVE_OSC
	if ( isUnderSessionManagement() ) {
		// If Hydrogen is under session management (NSM), tell the
		// NSM server that the Song was modified.
		m_pNsmClient->sendDirtyState( bIsModified );
	}
#endif
}

bool Hydrogen::getSongModified() const {
	if ( m_pSong != nullptr ) {
		return m_pSong->getIsModified();
	}
	return false;
}

void Hydrogen::setIsTimelineActivated( bool bEnabled ) {
	if ( getSong() == nullptr ) {
        return;
    }

	auto pPref = m_pPreferences;
	auto pAudioEngine = getAudioEngine();

	if ( bEnabled != getSong()->getIsTimelineActivated() ) {
		pAudioEngine->lock( RIGHT_HERE );

		m_pSong->setIsTimelineActivated( bEnabled );

		if ( bEnabled ) {
			m_pSong->getTimeline()->activate( this );
		}
		else {
			m_pSong->getTimeline()->deactivate();
		}

		pAudioEngine->handleTimelineChange();
		pAudioEngine->unlock();

		m_pEventQueue->pushEvent(
			Event::Type::TimelineActivation, static_cast<int>( bEnabled )
		);
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
		m_pEventQueue->pushEvent(
			Event::Type::PatternChanged, 0
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
			.append( QString( "%1%2m_ProcessMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( ProcessModeToQString( m_ProcessMode ) ) )
			.append( QString( "%1%2m_bIsFullyOperational: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsFullyOperational ) )
			.append( QString( "%1%2m_instrumentDeathRow:\n" ).arg( sPrefix ).arg( s ) );
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
			.append( QString( ", m_ProcessMode: %1" ).
					 arg( ProcessModeToQString( m_ProcessMode ) ) )
			.append( QString( ", m_bIsFullyOperational: %1" )
					 .arg( m_bIsFullyOperational ) )
			.append( QString( ", m_instrumentDeathRow: [" ) );
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
}; /* Namespace */
