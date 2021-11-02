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

#include <core/IO/AlsaAudioDriver.h>

#if defined(H2CORE_HAVE_ALSA) || _DOXYGEN_

#include <pthread.h>
#include <iostream>
#include <core/Preferences/Preferences.h>

namespace H2Core
{

pthread_t alsaAudioDriverThread;

static int alsa_xrun_recovery( snd_pcm_t *handle, int err )
{
	if ( err == -EPIPE ) {  /* under-run */
		err = snd_pcm_prepare( handle );
	} else if ( err == -ESTRPIPE ) {
		while ( ( err = snd_pcm_resume( handle ) ) == -EAGAIN ) {
			sleep( 1 );     /* wait until the suspend flag is released */
		}
		if ( err < 0 ) {
			err = snd_pcm_prepare( handle );
			if ( err < 0 ) {
				std::cerr << "Can't recover from suspend, prepare failed: " << snd_strerror( err ) << std::endl;
			}
		}
		return 0;
	}
	return err;
}

void* alsaAudioDriver_processCaller( void* param )
{
	Base *__object = (Base*)param;
	AlsaAudioDriver *pDriver = ( AlsaAudioDriver* )param;

	// stolen from amSynth
	struct sched_param sched;
	sched.sched_priority = 50;
	int res = sched_setscheduler( 0, SCHED_FIFO, &sched );
	sched_getparam( 0, &sched );
	if ( res ) {
		__ERRORLOG( "Can't set realtime scheduling for ALSA Driver" );
	}
	__INFOLOG( QString( "Scheduling priority = %1" ).arg( sched.sched_priority ) );

	sleep( 1 );

	int err;
	if ( ( err = snd_pcm_prepare( pDriver->m_pPlayback_handle ) ) < 0 ) {
		__ERRORLOG( QString( "Cannot prepare audio interface for use: %1" ).arg( snd_strerror ( err ) ) );
	}

	int nFrames = pDriver->m_nBufferSize;
	__INFOLOG( QString( "nFrames: %1" ).arg( nFrames ) );
	short pBuffer[ nFrames * 2 ];

	float *pOut_L = pDriver->m_pOut_L;
	float *pOut_R = pDriver->m_pOut_R;

	while ( pDriver->m_bIsRunning ) {
		// prepare the audio data
		pDriver->m_processCallback( nFrames, nullptr );

		for ( int i = 0; i < nFrames; ++i ) {
			pBuffer[ i * 2 ] = ( short )( pOut_L[ i ] * 32768.0 );
			pBuffer[ i * 2 + 1 ] = ( short )( pOut_R[ i ] * 32768.0 );
		}

		if ( ( err = snd_pcm_writei( pDriver->m_pPlayback_handle, pBuffer, nFrames ) ) < 0 ) {
			__ERRORLOG( "XRUN" );

			if ( alsa_xrun_recovery( pDriver->m_pPlayback_handle, err ) < 0 ) {
				__ERRORLOG( "Can't recover from XRUN" );
			}
			// retry
			if ( ( err = snd_pcm_writei( pDriver->m_pPlayback_handle, pBuffer, nFrames ) ) < 0 ) {
				__ERRORLOG( "XRUN 2" );
				if ( alsa_xrun_recovery( pDriver->m_pPlayback_handle, err ) < 0 ) {
					__ERRORLOG( "Can't recover from XRUN" );
				}
			}

			pDriver->m_nXRuns++;
		}
	}
	return nullptr;
}


/// Use the name hints to build a list of potential device names.
QStringList AlsaAudioDriver::getDevices()
{
	QStringList result;
	void **pHints, **pHint;

	if ( snd_device_name_hint( -1, "pcm", &pHints) < 0) {
		ERRORLOG( "Couldn't get device hints" );
		return result;
	}

	for ( pHint = pHints; *pHint != nullptr; pHint++) {
		const char *sName = snd_device_name_get_hint( *pHint, "NAME"),
			*sIOID = snd_device_name_get_hint( *pHint, "IOID");

		if ( sIOID && QString( sIOID ) != "Output") {
			continue;
		}

		QString sDev = QString( sName );
		if ( sName ) {
			free( (void *)sName );
		}
		if ( sIOID ) {
			free( (void *)sIOID );
		}
		result.push_back( sDev );
	}
	snd_device_name_free_hint( pHints );
	return result;
}

AlsaAudioDriver::AlsaAudioDriver( audioProcessCallback processCallback )
		: AudioOutput()
		, m_bIsRunning( false )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_nXRuns( 0 )
		, m_nBufferSize( 0 )
		, m_pPlayback_handle( nullptr )
		, m_processCallback( processCallback )
{
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
	m_sAlsaAudioDevice = Preferences::get_instance()->m_sAlsaAudioDevice;
}

AlsaAudioDriver::~AlsaAudioDriver()
{
	if ( m_nXRuns > 0 ) {
		WARNINGLOG( QString( "%1 xruns" ).arg( m_nXRuns ) );
	}
}


int AlsaAudioDriver::init( unsigned nBufferSize )
{
	m_nBufferSize = nBufferSize;

	return 0;	// ok
}


int AlsaAudioDriver::connect()
{
	INFOLOG( "alsa device: " + m_sAlsaAudioDevice );
	int nChannels = 2;

	int err;

	// provo ad aprire il device per verificare se e' libero ( non bloccante )
	if ( ( err = snd_pcm_open( &m_pPlayback_handle, m_sAlsaAudioDevice.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK ) ) < 0 ) {
		ERRORLOG( QString( "ALSA: cannot open audio device %1:%2" ).arg( m_sAlsaAudioDevice ).arg( snd_strerror( err ) ) );

		// il dispositivo e' occupato..provo con "default"
		m_sAlsaAudioDevice = "default";
		if ( ( err = snd_pcm_open( &m_pPlayback_handle, m_sAlsaAudioDevice.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK ) ) < 0 ) {
			ERRORLOG( QString( "ALSA: cannot open audio device %1:%2" ).arg( m_sAlsaAudioDevice ) .arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
			return 1;
		}
		WARNINGLOG( "Using alsa device: " + m_sAlsaAudioDevice );
	}
	snd_pcm_close( m_pPlayback_handle );

	// Apro il device ( bloccante )
	if ( ( err = snd_pcm_open( &m_pPlayback_handle, m_sAlsaAudioDevice.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, 0 ) ) < 0 ) {
		ERRORLOG( QString( "ALSA: cannot open audio device %1:%2" ).arg( m_sAlsaAudioDevice ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_alloca( &hw_params );
	if ( hw_params == nullptr ) {
		ERRORLOG( "error in snd_pcm_hw_params_alloca" );
		return 1;
	}

	if ( ( err = snd_pcm_hw_params_any( m_pPlayback_handle, hw_params ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_any: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}
//	snd_pcm_hw_params_set_access( m_pPlayback_handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED  );

	if ( ( err = snd_pcm_hw_params_set_access( m_pPlayback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_access: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	if ( ( err = snd_pcm_hw_params_set_format( m_pPlayback_handle, hw_params, SND_PCM_FORMAT_S16_LE ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_format: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	snd_pcm_hw_params_set_rate_near( m_pPlayback_handle, hw_params, &m_nSampleRate, nullptr );

	if ( ( err = snd_pcm_hw_params_set_channels( m_pPlayback_handle, hw_params, nChannels ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_channels: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	// Configure buffer size, periods and period size. The user
	// "BufferSize" setting defines the user's intention for the
	// number of frames processed in a callback period. In ALSA, this
	// is the "period", whereas the actual buffer (as reported by
	// *_get_buffer_size) is sized to keep at least 2 periods' worth
	// of data.
	//
	unsigned nPeriods = 2;
	if ( ( err = snd_pcm_hw_params_set_periods_near( m_pPlayback_handle, hw_params, &nPeriods, nullptr ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_periods: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}
	INFOLOG( QString( "nPeriods: %1" ).arg( nPeriods ) );

	snd_pcm_uframes_t period_size = m_nBufferSize;

	if ( ( err = snd_pcm_hw_params_set_period_size_near( m_pPlayback_handle, hw_params, &period_size, nullptr ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_period_size: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}
	m_nBufferSize = period_size;

	if ( ( err = snd_pcm_hw_params( m_pPlayback_handle, hw_params ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	snd_pcm_hw_params_get_rate( hw_params, &m_nSampleRate, nullptr );

	INFOLOG( QString( "*** PERIOD SIZE: %1" ).arg( period_size ) );
	INFOLOG( QString( "*** SAMPLE RATE: %1" ).arg( m_nSampleRate ) );
	INFOLOG( QString( "*** BUFFER SIZE: %1" ).arg( nPeriods * m_nBufferSize ) );

	//snd_pcm_hw_params_free( hw_params );

	m_pOut_L = new float[ m_nBufferSize ];
	m_pOut_R = new float[ m_nBufferSize ];

	memset( m_pOut_L, 0, m_nBufferSize * sizeof( float ) );
	memset( m_pOut_R, 0, m_nBufferSize * sizeof( float ) );

	m_bIsRunning = true;

	// start the main thread
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_create( &alsaAudioDriverThread, &attr, alsaAudioDriver_processCaller, this );

	return 0;	// OK
}




void AlsaAudioDriver::disconnect()
{
	INFOLOG( "[disconnect]" );

	m_bIsRunning = false;

	pthread_join( alsaAudioDriverThread, nullptr );

	snd_pcm_close( m_pPlayback_handle );

	delete[] m_pOut_L;
	m_pOut_L = nullptr;

	delete[] m_pOut_R;
	m_pOut_R = nullptr;
}

unsigned AlsaAudioDriver::getBufferSize()
{
	return m_nBufferSize;
}

unsigned AlsaAudioDriver::getSampleRate()
{
	return m_nSampleRate;
}

float* AlsaAudioDriver::getOut_L()
{
	return m_pOut_L;
}

float* AlsaAudioDriver::getOut_R()
{
	return m_pOut_R;
}

};

#endif // H2CORE_HAVE_ALSA
