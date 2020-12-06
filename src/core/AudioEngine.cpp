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

#include <core/AudioEngine.h>

#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Basics/Song.h>
#include <core/Sampler/Sampler.h>

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


AudioEngine* AudioEngine::__instance = nullptr;
const char* AudioEngine::__class_name = "AudioEngine";


void AudioEngine::create_instance()
{
	if( __instance == nullptr ) {
		__instance = new AudioEngine;
	}
}

AudioEngine::AudioEngine()
		: Object( __class_name )
		, __sampler( nullptr )
		, __synth( nullptr )
		, m_fElapsedTime( 0 )
		, m_pMainBuffer_L( nullptr )
		, m_pMainBuffer_R( nullptr )
		, m_pAudioDriver( nullptr )
		, m_pMidiDriver( nullptr )
		, m_pMidiDriverOut( nullptr )
		, m_State( STATE_INITIALIZED )
{
	__instance = this;
	INFOLOG( "INIT" );

	__sampler = new Sampler;
	__synth = new Synth;
	
	m_pEventQueue = EventQueue::get_instance();

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
	delete __sampler;
	delete __synth;
}

Sampler* AudioEngine::get_sampler()
{
	assert(__sampler);
	return __sampler;
}

Synth* AudioEngine::get_synth()
{
	assert(__synth);
	return __synth;
}

void AudioEngine::lock( const char* file, unsigned int line, const char* function )
{
	__engine_mutex.lock();
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_lockingThread = std::this_thread::get_id();
}

bool AudioEngine::try_lock( const char* file, unsigned int line, const char* function )
{
	bool res = __engine_mutex.try_lock();
	if ( !res ) {
		// Lock not obtained
		return false;
	}
	__locker.file = file;
	__locker.line = line;
	__locker.function = function;
	m_lockingThread = std::this_thread::get_id();
	return true;
}

bool AudioEngine::try_lock_for( std::chrono::microseconds duration, const char* file, unsigned int line, const char* function )
{
	bool res = __engine_mutex.try_lock_for( duration );
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
	m_lockingThread = std::this_thread::get_id();
	return true;
}

void AudioEngine::unlock()
{
	// Leave "__locker" dirty.
	m_lockingThread = std::thread::id();
	__engine_mutex.unlock();
}


float AudioEngine::compute_tick_size( const int nSampleRate, const float fBpm, const int nResolution)
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
	
	if ( !Preferences::get_instance()->getUseTimelineBpm() ){
		
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

		auto tempoMarkers = pHydrogen->getTimeline()->getAllTempoMarkers();
		
		// TODO: how to handle the BPM before the first marker?
		fPreviousTickSize = compute_tick_size( static_cast<int>(sampleRate), 
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

			fPreviousTickSize = compute_tick_size( static_cast<int>(sampleRate), 
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
	AudioEngine::get_instance()->calculateElapsedTime( pDriver->getSampleRate(),
													   nFrame,
													   pHydrogen->getSong()->__resolution );
}

void AudioEngine::clearAudioBuffers( uint32_t nFrames )
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

	if ( sDriver == "Oss" ) {
		pDriver = new OssDriver( m_AudioProcessCallback );
		if ( pDriver->class_name() == NullDriver::class_name() ) {
			delete pDriver;
			pDriver = nullptr;
		}
	} else if ( sDriver == "Jack" ) {
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
	} else if ( sDriver == "Alsa" ) {
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
	QMutexLocker mx(&mutex_OutputPointer);

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
		if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == nullptr ) {
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == nullptr ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == nullptr ) {
					if ( ( m_pAudioDriver = createDriver( "PortAudio" ) ) == nullptr ) {
						if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == nullptr ) {
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
			if ( ( m_pAudioDriver = createDriver( "Alsa" ) ) == nullptr ) {
				if ( ( m_pAudioDriver = createDriver( "CoreAudio" ) ) == nullptr ) {
					if ( ( m_pAudioDriver = createDriver( "Jack" ) ) == nullptr ) {
						if ( ( m_pAudioDriver = createDriver( "Oss" ) ) == nullptr ) {
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
		m_State = STATE_READY;
		m_pAudioDriver->setBpm( pSong->__bpm );
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

		if ( ( m_pMainBuffer_L = m_pAudioDriver->getOut_L() ) == nullptr ) {
			___ERRORLOG( "m_pMainBuffer_L == NULL" );
		}
		if ( ( m_pMainBuffer_R = m_pAudioDriver->getOut_R() ) == nullptr ) {
			___ERRORLOG( "m_pMainBuffer_R == NULL" );
		}

#ifdef H2CORE_HAVE_JACK
		renameJackPorts( pSong );
#endif

		setupLadspaFX( m_pAudioDriver->getBufferSize() );
	}
}

void AudioEngine::stopAudioDrivers()
{
	___INFOLOG( "[audioEngine_stopAudioDrivers]" );

	// check current state
	if ( m_State == STATE_PLAYING ) {
		//audioEngine_stop(); //TODO SMO: Re-enable
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

void AudioEngine::process_checkBPMChanged(Song* pSong)
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

void AudioEngine::setupLadspaFX( unsigned nBufferSize )
{
	//___INFOLOG( "buffersize=" + to_string(nBufferSize) );

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
	if ( ! pSong ) {
		return;
	}

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

void AudioEngine::renameJackPorts(Song * pSong)
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


}; // namespace H2Core
