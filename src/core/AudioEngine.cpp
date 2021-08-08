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

#include <core/AudioEngine.h>

#ifdef WIN32
#    include "core/Timehelper.h"
#else
#    include <unistd.h>
#    include <sys/time.h>
#endif

#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Basics/Song.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Sampler/Sampler.h>
#include <core/Helpers/Filesystem.h>

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

#include <core/Hydrogen.h>	// TODO: remove this line as soon as possible
#include <core/Preferences.h>
#include <cassert>

namespace H2Core
{


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


/** Gets the current time.
 * \return Current time obtained by gettimeofday()*/
inline timeval currentTime2()
{
	struct timeval now;
	gettimeofday( &now, nullptr );
	return now;
}

AudioEngine::AudioEngine()
		: Object()
		, m_pSampler( nullptr )
		, m_pSynth( nullptr )
		, m_fElapsedTime( 0 )
		, m_pAudioDriver( nullptr )
		, m_pMidiDriver( nullptr )
		, m_pMidiDriverOut( nullptr )
		, m_State( STATE_INITIALIZED )
		, m_pMetronomeInstrument( nullptr )
		, m_nPatternStartTick( -1 )
		, m_nPatternTickPosition( 0 )
		, m_nSongSizeInTicks( 0 )
		, m_nRealtimeFrames( 0 )
		, m_nAddRealtimeNoteTickPosition( 0 )
		, m_nSongPos( -1 )
		, m_nSelectedPatternNumber( 0 )
{
	INFOLOG( "INIT" );

	m_pSampler = new Sampler;
	m_pSynth = new Synth;
	
	m_pEventQueue = EventQueue::get_instance();
	
	m_fMasterPeak_L = 0.0f;		///< Master peak (left channel)
	m_fMasterPeak_R = 0.0f;		///< Master peak (right channel)
	m_fProcessTime = 0.0f;		///< time used in process function
	m_fMaxProcessTime = 0.0f;	///< max ms usable in process with no xrun
	
	srand( time( nullptr ) );

	// Create metronome instrument
	// Get the path to the file of the metronome sound.
	QString sMetronomeFilename = Filesystem::click_file_path();
	m_pMetronomeInstrument = std::make_shared<Instrument>( METRONOME_INSTR_ID, "metronome" );
	
	auto pLayer =  std::make_shared<InstrumentLayer>( Sample::load( sMetronomeFilename ) );
	auto pCompo = std::make_shared<InstrumentComponent>( 0 );
	pCompo->set_layer(pLayer, 0);
	m_pMetronomeInstrument->get_components()->push_back( pCompo );
	m_pMetronomeInstrument->set_is_metronome_instrument(true);
	
	m_pPlayingPatterns = new PatternList();
	m_pPlayingPatterns->setNeedsLock( true );
	m_pNextPatterns = new PatternList();
	m_pNextPatterns->setNeedsLock( true );
	
	m_AudioProcessCallback = &audioEngine_process;

#ifdef H2CORE_HAVE_LADSPA
	Effects::create_instance();
#endif

}

AudioEngine::~AudioEngine()
{
	INFOLOG( "DESTROY" );
#ifdef H2CORE_HAVE_LADSPA
	delete Effects::get_instance();
#endif

//	delete Sequencer::get_instance();
	delete m_pSampler;
	delete m_pSynth;
}

Sampler* AudioEngine::getSampler()
{
	assert(m_pSampler);
	return m_pSampler;
}

Synth* AudioEngine::getSynth()
{
	assert(m_pSynth);
	return m_pSynth;
}

void AudioEngine::lock( const char* file, unsigned int line, const char* function )
{
	m_EngineMutex.lock();
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_LockingThread = std::this_thread::get_id();
}

bool AudioEngine::tryLock( const char* file, unsigned int line, const char* function )
{
	bool res = m_EngineMutex.try_lock();
	if ( !res ) {
		// Lock not obtained
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_LockingThread = std::this_thread::get_id();
	return true;
}

bool AudioEngine::tryLockFor( std::chrono::microseconds duration, const char* file, unsigned int line, const char* function )
{
	bool res = m_EngineMutex.try_lock_for( duration );
	if ( !res ) {
		// Lock not obtained
		WARNINGLOG( QString( "Lock timeout: lock timeout %1:%2%3, lock held by %4:%5:%6" )
					.arg( file ).arg( function ).arg( line )
					.arg( __locker.file ).arg( __locker.function ).arg( __locker.line ));
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_LockingThread = std::this_thread::get_id();
	return true;
}

void AudioEngine::unlock()
{
	// Leave "__locker" dirty.
	m_LockingThread = std::thread::id();
	m_EngineMutex.unlock();
}


void AudioEngine::destroy()
{
	// check current state
	if ( getState() != STATE_INITIALIZED ) {
		___ERRORLOG( "Error the audio engine is not in INITIALIZED state" );
		return;
	}
	m_pSampler->stopPlayingNotes();

	this->lock( RIGHT_HERE );
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
	setState( STATE_UNINITIALIZED );

	EventQueue::get_instance()->push_event( EVENT_STATE, STATE_UNINITIALIZED );

	delete m_pPlayingPatterns;
	m_pPlayingPatterns = nullptr;

	delete m_pNextPatterns;
	m_pNextPatterns = nullptr;

	m_pMetronomeInstrument = nullptr;

	this->unlock();
}

int AudioEngine::start( bool bLockEngine, unsigned nTotalFrames )
{
	if ( bLockEngine ) {
		this->lock( RIGHT_HERE );
	}

	___INFOLOG( "[audioEngine_start]" );

	// check current state
	if ( getState() != STATE_READY ) {
		___ERRORLOG( "Error the audio engine is not in READY state" );
		if ( bLockEngine ) {
			this->unlock();
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
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	m_pAudioDriver->m_transport.m_fTickSize =
		AudioEngine::computeTickSize( static_cast<float>(m_pAudioDriver->getSampleRate()), pSong->getBpm(), pSong->getResolution() );

	// change the current audio engine state
	setState( STATE_PLAYING );
	m_pEventQueue->push_event( EVENT_STATE, STATE_PLAYING );

	if ( bLockEngine ) {
		this->unlock();
	}
	return 0;
}

void AudioEngine::stop( bool bLockEngine )
{
	if ( bLockEngine ) {
		this->lock( RIGHT_HERE );
	}
	___INFOLOG( "[audioEngine_stop]" );

	// check current state
	if ( getState() != STATE_PLAYING ) {
		___ERRORLOG( "Error the audio engine is not in PLAYING state" );
		if ( bLockEngine ) {
			this->unlock();
		}
		return;
	}

	// change the current audio engine state
	setState( STATE_READY );
	m_pEventQueue->push_event( EVENT_STATE, STATE_READY );

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
		this->unlock();
	}
}


float AudioEngine::computeTickSize( const int nSampleRate, const float fBpm, const int nResolution)
{
	float fTickSize = nSampleRate * 60.0 / fBpm / nResolution;
	
	return fTickSize;
}
	
void AudioEngine::calculateElapsedTime( const unsigned sampleRate, const unsigned long nFrame, const int nResolution ) {
	const auto pHydrogen = Hydrogen::get_instance();
	float fTickSize = pHydrogen->getAudioOutput()->m_transport.m_fTickSize;
	
	if ( fTickSize == 0 || sampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		m_fElapsedTime = 0;
		return;
	}
	
	if ( nFrame == 0 ) {
		m_fElapsedTime = 0;
		return;
	}
	
	const unsigned long currentTick = static_cast<unsigned long>(static_cast<float>(nFrame) / fTickSize );

	const auto tempoMarkers = pHydrogen->getTimeline()->getAllTempoMarkers();
	
	if ( !Preferences::get_instance()->getUseTimelineBpm() ||
		 tempoMarkers.size() == 0 ){
		
		int nPatternStartInTicks;
		const int nCurrentPatternNumber = pHydrogen->getPosForTick( currentTick, &nPatternStartInTicks );
		long totalTicks = pHydrogen->getTickForPosition( nCurrentPatternNumber );
		
		// The code above calculates the number of ticks elapsed since
		// the beginning of the Song till the start of the current
		// pattern. The following line covers the remain ticks.
		totalTicks += static_cast<long>(currentTick - nPatternStartInTicks);
		
		m_fElapsedTime = static_cast<float>(totalTicks) * fTickSize / 
			static_cast<float>(sampleRate);
	} else {

		m_fElapsedTime = 0;

		int nPatternStartInTicks;
		long totalTicks;
		long previousTicks = 0;
		float fPreviousTickSize;

		// TODO: how to handle the BPM before the first marker?
		fPreviousTickSize = computeTickSize( static_cast<int>(sampleRate), 
											   tempoMarkers[0]->fBpm, nResolution );
		
		// For each BPM marker on the Timeline we will get the number
		// of ticks since the previous marker/beginning and convert
		// them into time using tick size corresponding to the tempo.
		for ( auto const& mmarker: tempoMarkers ){
			totalTicks = pHydrogen->getTickForPosition( mmarker->nBar );
			    
			if ( totalTicks < currentTick ) {
				m_fElapsedTime += static_cast<float>(totalTicks - previousTicks) * 
					fPreviousTickSize / static_cast<float>(sampleRate);
			} else {
				m_fElapsedTime += static_cast<float>(currentTick - previousTicks) * 
					fPreviousTickSize / static_cast<float>(sampleRate);
				return;
			}

			fPreviousTickSize = computeTickSize( static_cast<int>(sampleRate), 
												   mmarker->fBpm, nResolution );
			previousTicks = totalTicks;
		}
		const int nCurrentPatternNumber = pHydrogen->getPosForTick( currentTick, &nPatternStartInTicks );
		totalTicks = pHydrogen->getTickForPosition( nCurrentPatternNumber );
		
		// The code above calculates the number of ticks elapsed since
		// the beginning of the Song till the start of the current
		// pattern. The following line covers the remain ticks.
		totalTicks += static_cast<long>(currentTick - nPatternStartInTicks);

		m_fElapsedTime += static_cast<float>(totalTicks - previousTicks) * fPreviousTickSize / 
			static_cast<float>(sampleRate);
	}
}

void AudioEngine::updateElapsedTime( const unsigned bufferSize, const unsigned sampleRate ){
	m_fElapsedTime += static_cast<float>(bufferSize) / static_cast<float>(sampleRate);
}

void AudioEngine::locate( const unsigned long nFrame ) {
	
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pDriver = pHydrogen->getAudioOutput();
	pDriver->locate( nFrame );
	calculateElapsedTime( pDriver->getSampleRate(),
						  nFrame,
						  pHydrogen->getSong()->getResolution() );

}

void AudioEngine::clearAudioBuffers( uint32_t nFrames )
{
	QMutexLocker mx( &m_MutexOutputPointer );
	float *pBuffer_L, *pBuffer_R;

	// clear main out Left and Right
	if ( m_pAudioDriver ) {
		pBuffer_L = m_pAudioDriver->getOut_L();
		pBuffer_R = m_pAudioDriver->getOut_R();
		assert( pBuffer_L != nullptr && pBuffer_R != nullptr );
		memset( pBuffer_L, 0, nFrames * sizeof( float ) );
		memset( pBuffer_R, 0, nFrames * sizeof( float ) );
	}

#ifdef H2CORE_HAVE_JACK
	JackAudioDriver * pJackAudioDriver = dynamic_cast<JackAudioDriver*>(m_pAudioDriver);
	
	if( pJackAudioDriver ) {
		pJackAudioDriver->clearPerTrackAudioBuffers( nFrames );
	}
#endif

	mx.unlock();

#ifdef H2CORE_HAVE_LADSPA
	if ( m_State >= STATE_READY ) {
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

AudioOutput* AudioEngine::createDriver( const QString& sDriver )
{
	___INFOLOG( QString( "Driver: '%1'" ).arg( sDriver ) );
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *pDriver = nullptr;

	if ( sDriver == "OSS" ) {
		pDriver = new OssDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "JACK" ) {
		pDriver = new JackAudioDriver( m_AudioProcessCallback );
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
	} else if ( sDriver == "ALSA" ) {
		pDriver = new AlsaAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "PortAudio" ) {
		pDriver = new PortAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	//#ifdef Q_OS_MACX
	else if ( sDriver == "CoreAudio" ) {
		___INFOLOG( "Creating CoreAudioDriver" );
		pDriver = new CoreAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	//#endif
	else if ( sDriver == "PulseAudio" ) {
		pDriver = new PulseAudioDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	}
	else if ( sDriver == "Fake" ) {
		___WARNINGLOG( "*** Using FAKE audio driver ***" );
		pDriver = new FakeDriver( m_AudioProcessCallback );
	} else {
		___ERRORLOG( "Unknown driver " + sDriver );
		raiseError( Hydrogen::UNKNOWN_DRIVER );
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

void AudioEngine::startAudioDrivers()
{
	Preferences *preferencesMng = Preferences::get_instance();

	// Lock both the AudioEngine and the audio output buffers.
	this->lock( RIGHT_HERE );
	QMutexLocker mx(&m_MutexOutputPointer);

	___INFOLOG( "[audioEngine_startAudioDrivers]" );
	
	// check current state
	if ( m_State != STATE_INITIALIZED ) {
		___ERRORLOG( QString( "Error the audio engine is not in INITIALIZED"
							  " state. state=%1" )
					 .arg( m_State ) );
		this->unlock();
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
		if ( ( m_pAudioDriver = createDriver( "JACK" ) ) == nullptr ) {
			if ( ( m_pAudioDriver = createDriver( "ALSA" ) ) == nullptr ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == nullptr ) {
					if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == nullptr ) {
						if ( ( m_pAudioDriver = createDriver( "OSS" ) ) == nullptr ) {
							if ( ( m_pAudioDriver = createDriver( "PulseAudio" ) ) == nullptr ) {
								raiseError( Hydrogen::ERROR_STARTING_DRIVER );
								___ERRORLOG( "Error starting audio driver" );
								___ERRORLOG( "Using the NULL output audio driver" );

								// use the NULL output driver
								m_pAudioDriver = new NullDriver( m_AudioProcessCallback );
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
			if ( ( m_pAudioDriver = createDriver( "ALSA" ) ) == nullptr ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == nullptr ) {
					if ( ( m_pAudioDriver = createDriver( "JACK" ) ) == nullptr ) {
						if ( ( m_pAudioDriver = createDriver( "OSS" ) ) == nullptr ) {
							if ( ( m_pAudioDriver = createDriver( "PulseAudio" ) ) == nullptr ) {
								raiseError( Hydrogen::ERROR_STARTING_DRIVER );
								___ERRORLOG( "Error starting audio driver" );
								___ERRORLOG( "Using the NULL output audio driver" );

								// use the NULL output driver
								m_pAudioDriver = new NullDriver( m_AudioProcessCallback );
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
			raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			___ERRORLOG( "Error starting audio driver" );
			___ERRORLOG( "Using the NULL output audio driver" );

			// use the NULL output driver
			m_pAudioDriver = new NullDriver( m_AudioProcessCallback );
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
	} else if ( preferencesMng->m_sMidiDriver == "CoreMIDI" ) {
#ifdef H2CORE_HAVE_COREMIDI
		CoreMidiDriver *coreMidiDriver = new CoreMidiDriver();
		m_pMidiDriver = coreMidiDriver;
		m_pMidiDriverOut = coreMidiDriver;
		m_pMidiDriver->open();
		m_pMidiDriver->setActive( true );
#endif
	} else if ( preferencesMng->m_sMidiDriver == "JACK-MIDI" ) {
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong ) {
		m_State = STATE_READY;
		m_pAudioDriver->setBpm( pSong->getBpm() );
	} else {
		m_State = STATE_PREPARED;
	}

	if ( m_State == STATE_PREPARED ) {
		m_pEventQueue->push_event( EVENT_STATE, STATE_PREPARED );
	} else if ( m_State == STATE_READY ) {
		m_pEventQueue->push_event( EVENT_STATE, STATE_READY );
	}

	// Unlocking earlier might execute the jack process() callback before we
	// are fully initialized.
	mx.unlock();
	this->unlock();

	if ( m_pAudioDriver ) {
		int res = m_pAudioDriver->connect();
		if ( res != 0 ) {
			raiseError( Hydrogen::ERROR_STARTING_DRIVER );
			___ERRORLOG( "Error starting audio driver [audioDriver::connect()]" );
			___ERRORLOG( "Using the NULL output audio driver" );

			mx.relock();
			delete m_pAudioDriver;
			m_pAudioDriver = new NullDriver( m_AudioProcessCallback );
			mx.unlock();
			m_pAudioDriver->init( 0 );
			m_pAudioDriver->connect();
		}

#ifdef H2CORE_HAVE_JACK
		if ( pSong != nullptr ) {
			renameJackPorts( pSong );
		}
#endif

		setupLadspaFX();
	}
}

void AudioEngine::stopAudioDrivers()
{
	___INFOLOG( "[audioEngine_stopAudioDrivers]" );

	// check current state
	if ( m_State == STATE_PLAYING ) {
		this->stop(); 
	}

	if ( ( m_State != STATE_PREPARED )
		 && ( m_State != STATE_READY ) ) {
		___ERRORLOG( QString( "Error: the audio engine is not in PREPARED"
							  " or READY state. state=%1" )
					 .arg( m_State ) );
		return;
	}

	// change the current audio engine state
	m_State = STATE_INITIALIZED;
	m_pEventQueue->push_event( EVENT_STATE, STATE_INITIALIZED );

	this->lock( RIGHT_HERE );

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
		QMutexLocker mx( &m_MutexOutputPointer );
		delete m_pAudioDriver;
		m_pAudioDriver = nullptr;
		mx.unlock();
	}

	this->unlock();
}

/** 
 * Restart all audio and midi drivers by calling first
 * stopAudioDrivers() and then startAudioDrivers() 
 */
void AudioEngine::restartAudioDrivers()
{
	if ( m_pAudioDriver != nullptr ) {
		stopAudioDrivers();
	}
	startAudioDrivers();
}

void AudioEngine::processCheckBPMChanged(std::shared_ptr<Song> pSong)
{
	if ( m_State != STATE_READY
		 && m_State != STATE_PLAYING ) {
		return;
	}

	long long oldFrame;
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->haveJackTransport() && 
		 m_State != STATE_PLAYING ) {
		oldFrame = static_cast< JackAudioDriver* >( m_pAudioDriver )->m_currentPos;
			
	} else {
		oldFrame = m_pAudioDriver->m_transport.m_nFrames;
	}
#else
	oldFrame = m_pAudioDriver->m_transport.m_nFrames;
#endif
	float fOldTickSize = m_pAudioDriver->m_transport.m_fTickSize;
	float fNewTickSize = AudioEngine::computeTickSize( m_pAudioDriver->getSampleRate(), pSong->getBpm(), pSong->getResolution() );

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

void AudioEngine::setupLadspaFX()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( ! pSong ) {
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

void AudioEngine::renameJackPorts(std::shared_ptr<Song> pSong)
{
#ifdef H2CORE_HAVE_JACK
	// renames jack ports
	if ( ! pSong ) return;

	if ( Hydrogen::get_instance()->haveJackAudioDriver() ) {

		// When restarting the audio driver after loading a new song under
		// Non session management all ports have to be registered _prior_
		// to the activation of the client.
		if ( Hydrogen::get_instance()->isUnderSessionManagement() ) {
			return;
		}
		
		static_cast< JackAudioDriver* >( m_pAudioDriver )->makeTrackOutputs( pSong );
	}
#endif
}

void AudioEngine::raiseError( unsigned nErrorCode )
{
	m_pEventQueue->push_event( EVENT_ERROR, nErrorCode );
}

inline void AudioEngine::processPlayNotes( unsigned long nframes )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	unsigned int framepos;

	if (  getState() == STATE_PLAYING ) {
		framepos = m_pAudioDriver->m_transport.m_nFrames;
	} else {
		// use this to support realtime events when not playing
		framepos = pHydrogen->getRealtimeFrames();
	}

	AutomationPath *vp = pSong->getVelocityAutomationPath();
	

	// reading from m_songNoteQueue
	while ( !m_songNoteQueue.empty() ) {
		Note *pNote = m_songNoteQueue.top();

		float velocity_adjustment = 1.0f;
		if ( pSong->getMode() == Song::SONG_MODE ) {
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

			/* Check if the current note has probability != 1
			 * If yes remove call random function to dequeue or not the note
			 */
			float fNoteProbability = pNote->get_probability();
			if ( fNoteProbability != 1. ) {
				if ( fNoteProbability < (float) rand() / (float) RAND_MAX ) {
					m_songNoteQueue.pop();
					pNote->get_instrument()->dequeue();
					continue;
				}
			}

			if ( pSong->getHumanizeVelocityValue() != 0 ) {
				float random = pSong->getHumanizeVelocityValue() * getGaussian( 0.2 );
				pNote->set_velocity(
							pNote->get_velocity()
							+ ( random
								- ( pSong->getHumanizeVelocityValue() / 2.0 ) )
							);
				if ( pNote->get_velocity() > 1.0 ) {
					pNote->set_velocity( 1.0 );
				} else if ( pNote->get_velocity() < 0.0 ) {
					pNote->set_velocity( 0.0 );
				}
			}

			// Offset + Random Pitch ;)
			float fPitch = pNote->get_pitch() + pNote->get_instrument()->get_pitch_offset();
			/* Check if the current instrument has random picth factor != 0.
			 * If yes add a gaussian perturbation to the pitch
			 */
			float fRandomPitchFactor = pNote->get_instrument()->get_random_pitch_factor();
			if ( fRandomPitchFactor != 0. ) {
				fPitch += getGaussian( 0.4 ) * fRandomPitchFactor;
			}
			pNote->set_pitch( fPitch );


			/*
			 * Check if the current instrument has the property "Stop-Note" set.
			 * If yes, a NoteOff note is generated automatically after each note.
			 */
			auto  noteInstrument = pNote->get_instrument();
			if ( noteInstrument->is_stop_notes() ){
				Note *pOffNote = new Note( noteInstrument,
										   0.0,
										   0.0,
										   0.0,
										   -1,
										   0 );
				pOffNote->set_note_off( true );
				pHydrogen->getAudioEngine()->getSampler()->noteOn( pOffNote );
				delete pOffNote;
			}

			m_pSampler->noteOn( pNote );
			m_songNoteQueue.pop(); // rimuovo la nota dalla lista di note
			pNote->get_instrument()->dequeue();
			// raise noteOn event
			int nInstrument = pSong->getInstrumentList()->index( pNote->get_instrument() );
			if( pNote->get_note_off() ){
				delete pNote;
			}

			m_pEventQueue->push_event( EVENT_NOTEON, nInstrument );
			
			continue;
		} else {
			// this note will not be played
			break;
		}
	}
}


void AudioEngine::seek( long long nFrames, bool bLoopMode )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

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

	bool loop = pSong->getIsLoopEnabled();

	if ( bLoopMode ) {
		loop = true;
	}

	m_nSongPos = findPatternInTick( tickNumber_start, loop, &m_nPatternStartTick );
	//	sprintf(tmp, "[audioEngine_seek()] m_nSongPos = %d", m_nSongPos);
	//	__instance->infoLog(tmp);
	
	clearNoteQueue();
}

inline void AudioEngine::processTransport( unsigned nFrames )
{
	if (   getState() != STATE_READY
		&& getState() != STATE_PLAYING
	) return;

	// Considering JackAudioDriver:
	// Compares the current transport state, speed in bpm, and
	// transport position with a query request to the JACK
	// server. It will only overwrite m_transport.m_nFrames, if
	// the transport position was changed by the user by
	// e.g. clicking on the timeline.
	m_pAudioDriver->updateTransportInfo();

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	// Update the state of the audio engine depending on the
	// status of the audio driver. E.g. if the JACK transport was
	// started by another client, the audio engine has to be
	// started as well.
	switch ( m_pAudioDriver->m_transport.m_status ) {
	case TransportInfo::ROLLING:
		if ( getState() == STATE_READY ) {
			// false == no engine lock. Already locked
			// this should set STATE_PLAYING
			this->start( false, m_pAudioDriver->m_transport.m_nFrames );
		}
		// So, we are not playing even after attempt to start engine
		if ( getState() != STATE_PLAYING ) {
			return;
		}

		/* Now we're playing | Update BPM */
		if ( pSong->getBpm() != m_pAudioDriver->m_transport.m_fBPM ) {
			___INFOLOG( QString( "song bpm: (%1) gets transport bpm: (%2)" )
				.arg( pSong->getBpm() )
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
		if ( getState() == STATE_PLAYING ) {
			// false == no engine lock. Already locked
			this->stop( false );
		}

		// go ahead and increment the realtimeframes by nFrames
		// to support our realtime keyboard and midi event timing
		// TODO: use method like setRealtimeFrames
		m_nRealtimeFrames += nFrames;
		break;
	}
}

void AudioEngine::clearNoteQueue()
{
	//___INFOLOG( "clear notes...");

	// delete all copied notes in the song notes queue
	while (!m_songNoteQueue.empty()) {
		m_songNoteQueue.top()->get_instrument()->dequeue();
		delete m_songNoteQueue.top();
		m_songNoteQueue.pop();
	}

	m_pSampler->stopPlayingNotes();

	// delete all copied notes in the midi notes queue
	for ( unsigned i = 0; i < m_midiNoteQueue.size(); ++i ) {
		delete m_midiNoteQueue[i];
	}
	m_midiNoteQueue.clear();

}

int AudioEngine::audioEngine_process( uint32_t nframes, void* /*arg*/ )
{
	AudioEngine* pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	// ___INFOLOG( QString( "[begin] status: %1, frame: %2, ticksize: %3, bpm: %4" )
	// 	    .arg( m_pAudioDriver->m_transport.m_status )
	// 	    .arg( m_pAudioDriver->m_transport.m_nFrames )
	// 	    .arg( m_pAudioDriver->m_transport.m_fTickSize )
	// 	    .arg( m_pAudioDriver->m_transport.m_fBPM ) );
	timeval startTimeval = currentTime2();

	// Resetting all audio output buffers with zeros.
	pAudioEngine->clearAudioBuffers( nframes );

	// Calculate maximum time to wait for audio engine lock. Using the
	// last calculated processing time as an estimate of the expected
	// processing time for this frame, the amount of slack time that
	// we can afford to wait is: m_fMaxProcessTime - m_fProcessTime.

	float sampleRate = static_cast<float>(pAudioEngine->m_pAudioDriver->getSampleRate());
	pAudioEngine->m_fMaxProcessTime = 1000.0 / ( sampleRate / nframes );
	float fSlackTime = pAudioEngine->m_fMaxProcessTime - pAudioEngine->m_fProcessTime;

	// If we expect to take longer than the available time to process,
	// require immediate locking or not at all: we're bound to drop a
	// buffer anyway.
	if ( fSlackTime < 0.0 ) {
		fSlackTime = 0.0;
	}

	/*
	 * The "try_lock" was introduced for Bug #164 (Deadlock after during
	 * alsa driver shutdown). The try_lock *should* only fail in rare circumstances
	 * (like shutting down drivers). In such cases, it seems to be ok to interrupt
	 * audio processing. Returning the special return value "2" enables the disk 
	 * writer driver to repeat the processing of the current data.
	 */
				
	if ( !pAudioEngine->tryLockFor( std::chrono::microseconds( (int)(1000.0*fSlackTime) ),
							  RIGHT_HERE ) ) {
		___ERRORLOG( QString( "Failed to lock audioEngine in allowed %1 ms, missed buffer" ).arg( fSlackTime ) );

		if ( pAudioEngine->m_pAudioDriver->class_name() == DiskWriterDriver::class_name() ) {
			return 2;	// inform the caller that we could not aquire the lock
		}

		return 0;
	}

	if ( pAudioEngine->getState() < STATE_READY) {
		pAudioEngine->unlock();
		return 0;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	// In case of the JackAudioDriver:
	// Query the JACK server for the current status of the
	// transport, start or stop the audio engine depending the
	// results, update the speed of the current song according to
	// the one used by the JACK server, and adjust the current
	// transport position if it was changed by an user interaction
	// (e.g. clicking on the timeline).
	pAudioEngine->processTransport( nframes );
	

	// ___INFOLOG( QString( "[after process] status: %1, frame: %2, ticksize: %3, bpm: %4" )
	// 	    .arg( m_pAudioDriver->m_transport.m_status )
	// 	    .arg( m_pAudioDriver->m_transport.m_nFrames )
	// 	    .arg( m_pAudioDriver->m_transport.m_fTickSize )
	// 	    .arg( m_pAudioDriver->m_transport.m_fBPM ) );
	// Check whether the tick size has changed.
	pAudioEngine->processCheckBPMChanged(pSong);

	bool bSendPatternChange = false;
	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	int nResNoteQueue = pAudioEngine->updateNoteQueue( nframes );
	if ( nResNoteQueue == -1 ) {	// end of song
		___INFOLOG( "End of song received, calling engine_stop()" );
		pAudioEngine->unlock();
		pAudioEngine->m_pAudioDriver->stop();
		pAudioEngine->locate( 0 ); // locate 0, reposition from start of the song

		if ( (pAudioEngine->m_pAudioDriver->class_name() == DiskWriterDriver::class_name() )
			 || ( pAudioEngine->m_pAudioDriver->class_name() == FakeDriver::class_name() )
			 ) {
			___INFOLOG( "End of song." );
			
			return 1;	// kill the audio AudioDriver thread
		}

		return 0;
	} else if ( nResNoteQueue == 2 ) { // send pattern change
		bSendPatternChange = true;
	}

	// play all notes
	pAudioEngine->processPlayNotes( nframes );

	float *pBuffer_L = pAudioEngine->m_pAudioDriver->getOut_L(),
		*pBuffer_R = pAudioEngine->m_pAudioDriver->getOut_R();
	assert( pBuffer_L != nullptr && pBuffer_R != nullptr );

	// SAMPLER
	pAudioEngine->getSampler()->process( nframes, pSong );
	float* out_L = pAudioEngine->getSampler()->m_pMainOut_L;
	float* out_R = pAudioEngine->getSampler()->m_pMainOut_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

	// SYNTH
	pAudioEngine->getSynth()->process( nframes );
	out_L = pAudioEngine->getSynth()->m_pOut_L;
	out_R = pAudioEngine->getSynth()->m_pOut_R;
	for ( unsigned i = 0; i < nframes; ++i ) {
		pBuffer_L[ i ] += out_L[ i ];
		pBuffer_R[ i ] += out_R[ i ];
	}

	timeval renderTime_end = currentTime2();
	timeval ladspaTime_start = renderTime_end;

#ifdef H2CORE_HAVE_LADSPA
	// Process LADSPA FX
	if ( pAudioEngine->getState() >= STATE_READY ) {
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
					pBuffer_L[ i ] += buf_L[ i ];
					pBuffer_R[ i ] += buf_R[ i ];
					if ( buf_L[ i ] > pAudioEngine->m_fFXPeak_L[nFX] ) {
						pAudioEngine->m_fFXPeak_L[nFX] = buf_L[ i ];
					}

					if ( buf_R[ i ] > pAudioEngine->m_fFXPeak_R[nFX] ) {
						pAudioEngine->m_fFXPeak_R[nFX] = buf_R[ i ];
					}
				}
			}
		}
	}
#endif
	timeval ladspaTime_end = currentTime2();


	// update master peaks
	float val_L, val_R;
	if ( pAudioEngine->getState() >= STATE_READY ) {
		for ( unsigned i = 0; i < nframes; ++i ) {
			val_L = pBuffer_L[i];
			val_R = pBuffer_R[i];

			if ( val_L > pAudioEngine->m_fMasterPeak_L ) {
				pAudioEngine->m_fMasterPeak_L = val_L;
			}

			if ( val_R > pAudioEngine->m_fMasterPeak_R ) {
				pAudioEngine->m_fMasterPeak_R = val_R;
			}

			for (std::vector<DrumkitComponent*>::iterator it = pSong->getComponents()->begin() ; it != pSong->getComponents()->end(); ++it) {
				DrumkitComponent* drumkit_component = *it;

				float compo_val_L = drumkit_component->get_out_L(i);
				float compo_val_R = drumkit_component->get_out_R(i);

				if( compo_val_L > drumkit_component->get_peak_l() ) {
					drumkit_component->set_peak_l( compo_val_L );
				}
				if( compo_val_R > drumkit_component->get_peak_r() ) {
					drumkit_component->set_peak_r( compo_val_R );
				}
			}
		}
	}

	// update total frames number
	if ( pAudioEngine->getState() == STATE_PLAYING ) {
		pAudioEngine->m_pAudioDriver->m_transport.m_nFrames += nframes;
	}

	timeval finishTimeval = currentTime2();
	pAudioEngine->m_fProcessTime =
			( finishTimeval.tv_sec - startTimeval.tv_sec ) * 1000.0
			+ ( finishTimeval.tv_usec - startTimeval.tv_usec ) / 1000.0;

	if ( pAudioEngine->getState() == STATE_PLAYING ) {
		pAudioEngine->updateElapsedTime( nframes, pAudioEngine->m_pAudioDriver->getSampleRate() );
	}

#ifdef CONFIG_DEBUG
	if ( pAudioEngine->m_fProcessTime > m_fMaxProcessTime ) {
		___WARNINGLOG( "" );
		___WARNINGLOG( "----XRUN----" );
		___WARNINGLOG( QString( "XRUN of %1 msec (%2 > %3)" )
					   .arg( ( m_fProcessTime - pAudioEngine->m_fMaxProcessTime ) )
					   .arg( m_fProcessTime ).arg( pAudioEngine->m_fMaxProcessTime ) );
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

	pAudioEngine->unlock();

	if ( bSendPatternChange ) {
		EventQueue::get_instance()->push_event( EVENT_PATTERN_CHANGED, -1 );
	}
	return 0;
}

void AudioEngine::setSong( std::shared_ptr<Song> pNewSong )
{
	___WARNINGLOG( QString( "Set song: %1" ).arg( pNewSong->getName() ) );
	
	this->lock( RIGHT_HERE );

	// check current state
	// should be set by removeSong called earlier
	if ( getState() != STATE_PREPARED ) {
		___ERRORLOG( "Error the audio engine is not in PREPARED state" );
	}

	// setup LADSPA FX
	if ( m_pAudioDriver != nullptr ) {
		setupLadspaFX();
	}

	// update tick size
	processCheckBPMChanged( pNewSong );

	// find the first pattern and set as current
	if ( pNewSong->getPatternList()->size() > 0 ) {
		m_pPlayingPatterns->add( pNewSong->getPatternList()->get( 0 ) );
	}

	renameJackPorts( pNewSong );

	m_pAudioDriver->setBpm( pNewSong->getBpm() );
	m_pAudioDriver->m_transport.m_fTickSize = 
		AudioEngine::computeTickSize( static_cast<int>(m_pAudioDriver->getSampleRate()),
										pNewSong->getBpm(),
										static_cast<int>(pNewSong->getResolution()) );

	// change the current audio engine state
	setState( STATE_READY );

	locate( 0 );

	this->unlock();

	m_pEventQueue->push_event( EVENT_STATE, STATE_READY );
}

void AudioEngine::removeSong()
{
	this->lock( RIGHT_HERE );

	if ( getState() == STATE_PLAYING ) {
		m_pAudioDriver->stop();
		this->stop( false );
	}

	// check current state
	if ( getState() != STATE_READY ) {
		___ERRORLOG( "Error the audio engine is not in READY state" );
		this->unlock();
		return;
	}

	m_pPlayingPatterns->clear();
	m_pNextPatterns->clear();
	clearNoteQueue();

	// change the current audio engine state
	setState( STATE_PREPARED );
	this->unlock();

	m_pEventQueue->push_event( EVENT_STATE, STATE_PREPARED );
}

inline int AudioEngine::updateNoteQueue( unsigned nFrames )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	// Indicates whether the current pattern list changed with respect
	// to the last cycle.
	bool bSendPatternChange = false;
	float fTickSize = m_pAudioDriver->m_transport.m_fTickSize;
	int nLeadLagFactor = pHydrogen->calculateLeadLagFactor( fTickSize );

	unsigned int framepos;
	if ( getState() == STATE_PLAYING ) {
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
		 || ( getState() == STATE_PLAYING
			  && pSong->getMode() == Song::SONG_MODE
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

		if (  getState() != STATE_PLAYING ) {
			// only keep going if we're playing
			continue;
		}
		
		//////////////////////////////////////////////////////////////
		// SONG MODE
		if ( pSong->getMode() == Song::SONG_MODE ) {
			if ( pSong->getPatternGroupVector()->size() == 0 ) {
				// there's no song!!
				___ERRORLOG( "no patterns in song." );
				m_pAudioDriver->stop();
				return -1;
			}
	
			m_nSongPos = findPatternInTick( tick, pSong->getIsLoopEnabled(), &m_nPatternStartTick );

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
				if ( pSong->getIsLoopEnabled() == true ) {
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
			PatternList *pPatternList = ( *( pSong->getPatternGroupVector() ) )[m_nSongPos];
			m_pPlayingPatterns->clear();
			for ( int i=0; i< pPatternList->size(); ++i ) {
				Pattern* pPattern = pPatternList->get(i);
				m_pPlayingPatterns->add( pPattern );
				pPattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}
		}
		
		//////////////////////////////////////////////////////////////
		// PATTERN MODE
		else if ( pSong->getMode() == Song::PATTERN_MODE )	{

			int nPatternSize = MAX_NOTES;

			// If the user chose to playback the pattern she focuses,
			// use it to overwrite `m_pPlayingPatterns`.
			if ( Preferences::get_instance()->patternModePlaysSelected() )
			{
				// TODO: Again, a check whether the pattern did change
				// would be more efficient.
				m_pPlayingPatterns->clear();
				Pattern * pattern = pSong->getPatternList()->get(m_nSelectedPatternNumber);
				m_pPlayingPatterns->add( pattern );
				pattern->extand_with_flattened_virtual_patterns( m_pPlayingPatterns );
			}

			if ( m_pPlayingPatterns->size() != 0 ) {
				nPatternSize = m_pPlayingPatterns->longest_pattern_length();
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
												 0.f, // pan
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

				// Perform a loop over all notes, which are enclose
				// the position of the current tick, using a constant
				// iterator (notes won't be altered!). After some
				// humanization was applied to onset of each note, it
				// will be added to `m_songNoteQueue` for playback.
				FOREACH_NOTE_CST_IT_BOUND(notes,it,m_nPatternTickPosition) {
					Note *pNote = it->second;
					if ( pNote ) {
						pNote->set_just_recorded( false );
						
						/** Time Offset in frames (relative to sample rate)
						*	Sum of 3 components: swing, humanized timing, lead_lag
						*/
						int nOffset = 0;

					   /** Swing 16ths //
						* delay the upbeat 16th-notes by a constant (manual) offset
						*/
						if ( ( ( m_nPatternTickPosition % ( MAX_NOTES / 16 ) ) == 0 )
							 && ( ( m_nPatternTickPosition % ( MAX_NOTES / 8 ) ) != 0 ) ) {
							/* TODO: incorporate the factor MAX_NOTES / 32. either in Song::m_fSwingFactor
							* or make it a member variable.
							* comment by oddtime:
							* 32 depends on the fact that the swing is applied to the upbeat 16th-notes.
							* (not to upbeat 8th-notes as in jazz swing!).
							* however 32 could be changed but must be >16, otherwise the max delay is too long and
							* the swing note could be played after the next downbeat!
							*/
							nOffset += (int) ( ( (float) MAX_NOTES / 32. ) * fTickSize * pSong->getSwingFactor() );
						}

						/* Humanize - Time parameter //
						* Add a random offset to each note. Due to
						* the nature of the Gaussian distribution,
						* the factor Song::__humanize_time_value will
						* also scale the variance of the generated
						* random variable.
						*/
						if ( pSong->getHumanizeTimeValue() != 0 ) {
							nOffset += ( int )(
										getGaussian( 0.3 )
										* pSong->getHumanizeTimeValue()
										* pHydrogen->m_nMaxTimeHumanize
										);
						}

						// Lead or Lag - timing parameter //
						// Add a constant offset to all notes.
						nOffset += (int) ( pNote->get_lead_lag() * nLeadLagFactor );

						// No note is allowed to start prior to the
						// beginning of the song.
						if((tick == 0) && (nOffset < 0)) {
							nOffset = 0;
						}
						
						// Generate a copy of the current note, assign
						// it the new offset, and push it to the list
						// of all notes, which are about to be played
						// back.
						// Why a copy? because it has the new offset (including swing and random timing) in its
						// humanized delay, and tick position is expressed referring to start time (and not pattern).
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

inline int AudioEngine::findPatternInTick( int nTick, bool bLoopMode, int* pPatternStartTick )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	assert( pSong );

	int nTotalTick = 0;
	m_nSongSizeInTicks = 0;

	std::vector<PatternList*> *pPatternColumns = pSong->getPatternGroupVector();
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
		m_nSongSizeInTicks = nTotalTick;
		int nLoopTick = 0;
		if ( m_nSongSizeInTicks != 0 ) {
			nLoopTick = nTick % m_nSongSizeInTicks;
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

	return -1;
}

void AudioEngine::noteOn( Note *note )
{
	// check current state
	if ( ( getState() != STATE_READY )
		 && ( getState() != STATE_PLAYING ) ) {
		___ERRORLOG( "Error the audio engine is not in READY state" );
		delete note;
		return;
	}

	m_midiNoteQueue.push_back( note );
}

bool AudioEngine::compare_pNotes::operator()(Note* pNote1, Note* pNote2)
{
	return (pNote1->get_humanize_delay() +
			pNote1->get_position() *
				Hydrogen::get_instance()->getAudioEngine()->getAudioDriver()->m_transport.m_fTickSize) >
		   (pNote2->get_humanize_delay() +
			pNote2->get_position() *
				Hydrogen::get_instance()->getAudioEngine()->getAudioDriver()->m_transport.m_fTickSize);
}


void AudioEngineLocking::assertAudioEngineLocked() const 
{
#ifndef NDEBUG
		if ( m_bNeedsLock ) {
			H2Core::Hydrogen::get_instance()->getAudioEngine()->assertLocked();
		}
#endif
	}


}; // namespace H2Core
