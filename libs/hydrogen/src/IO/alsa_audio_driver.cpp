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
#ifdef ALSA_SUPPORT

#include "AlsaAudioDriver.h"
#include <pthread.h>
#include <iostream>
#include <hydrogen/Preferences.h>

namespace H2Core
{

pthread_t alsaAudioDriverThread;

static int alsa_xrun_recovery( snd_pcm_t *handle, int err )
{
	if ( err == -EPIPE ) {  /* under-run */
		err = snd_pcm_prepare( handle );
	} else if ( err == -ESTRPIPE ) {
		while ( ( err = snd_pcm_resume( handle ) ) == -EAGAIN )
			sleep( 1 );     /* wait until the suspend flag is released */
		if ( err < 0 ) {
			err = snd_pcm_prepare( handle );
			if ( err < 0 )
				std::cerr << "Can't recovery from suspend, prepare failed: " << snd_strerror( err ) << std::endl;
		}
		return 0;
	}
	return err;
}

void* alsaAudioDriver_processCaller( void* param )
{
	AlsaAudioDriver *pDriver = ( AlsaAudioDriver* )param;

	// stolen from amSynth
	struct sched_param sched;
	sched.sched_priority = 50;
	int res = sched_setscheduler( 0, SCHED_FIFO, &sched );
	sched_getparam( 0, &sched );
	if ( res ) {
		_ERRORLOG( "Can't set realtime scheduling for ALSA Driver" );
	}
	_INFOLOG( QString( "Scheduling priority = %1" ).arg( sched.sched_priority ) );

	sleep( 1 );

	int err;
	if ( ( err = snd_pcm_prepare( pDriver->m_pPlayback_handle ) ) < 0 ) {
		_ERRORLOG( QString( "Cannot prepare audio interface for use: %1" ).arg( snd_strerror ( err ) ) );
	}

	int nFrames = pDriver->m_nBufferSize;
// 	_INFOLOG( "nFrames: " + to_string( nFrames ) );
	short pBuffer[ nFrames * 2 ];

	float *pOut_L = pDriver->m_pOut_L;
	float *pOut_R = pDriver->m_pOut_R;

	while ( pDriver->m_bIsRunning ) {
		// prepare the audio data
		pDriver->m_processCallback( nFrames, NULL );

		for ( int i = 0; i < nFrames; ++i ) {
			pBuffer[ i * 2 ] = ( short )( pOut_L[ i ] * 32768.0 );
			pBuffer[ i * 2 + 1 ] = ( short )( pOut_R[ i ] * 32768.0 );
		}

		if ( ( err = snd_pcm_writei( pDriver->m_pPlayback_handle, pBuffer, nFrames ) ) < 0 ) {
			_ERRORLOG( "XRUN" );

			if ( alsa_xrun_recovery( pDriver->m_pPlayback_handle, err ) < 0 ) {
				_ERRORLOG( "Can't recovery from XRUN" );
			}
			// retry
			if ( ( err = snd_pcm_writei( pDriver->m_pPlayback_handle, pBuffer, nFrames ) ) < 0 ) {
				_ERRORLOG( "XRUN 2" );
				if ( alsa_xrun_recovery( pDriver->m_pPlayback_handle, err ) < 0 ) {
					_ERRORLOG( "Can't recovery from XRUN" );
				}
			}

			pDriver->m_nXRuns++;
		}
	}
	return 0;
}



AlsaAudioDriver::AlsaAudioDriver( audioProcessCallback processCallback )
		: AudioOutput( "AlsaAudioDriver" )
		, m_bIsRunning( false )
		, m_pOut_L( NULL )
		, m_pOut_R( NULL )
		, m_nXRuns( 0 )
		, m_processCallback( processCallback )
{
	INFOLOG( "INIT" );
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
	m_sAlsaAudioDevice = Preferences::get_instance()->m_sAlsaAudioDevice;
}

AlsaAudioDriver::~AlsaAudioDriver()
{
	if ( m_nXRuns > 0 ) {
		WARNINGLOG( QString( "%1 xruns" ).arg( m_nXRuns ) );
	}
	INFOLOG( "DESTROY" );
}


int AlsaAudioDriver::init( unsigned nBufferSize )
{
	INFOLOG( "init" );
	m_nBufferSize = nBufferSize;

	return 0;	// ok
}


int AlsaAudioDriver::connect()
{
	INFOLOG( "alsa device: " + m_sAlsaAudioDevice );
	int nChannels = 2;
	int period_size = m_nBufferSize / 2;

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
	if ( hw_params == NULL ) {
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

	snd_pcm_hw_params_set_rate_near( m_pPlayback_handle, hw_params, &m_nSampleRate, 0 );

	if ( ( err = snd_pcm_hw_params_set_channels( m_pPlayback_handle, hw_params, nChannels ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_channels: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	unsigned nPeriods = 2;
	if ( ( err = snd_pcm_hw_params_set_periods_near( m_pPlayback_handle, hw_params, &nPeriods, 0 ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_periods: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}
	INFOLOG( QString( "nPeriods: %1" ).arg( nPeriods ) );

	// Set buffer size (in frames). The resulting latency is given by
	// latency = periodsize * periods / (rate * bytes_per_frame)
//	m_nBufferSize = period_size * 2;
//	infoLog( "buffer size preferita:" + to_string( m_nBufferSize ) );

//	if ( ( err = snd_pcm_hw_params_set_buffer_size_near( m_pPlayback_handle, hw_params, &m_nBufferSize ) ) < 0 ) {
//		errorLog( "[connect] error in snd_pcm_hw_params_set_buffer_size: " + string( QString::fromLocal8Bit(snd_strerror(err)) ) );
//		return 1;
//	}
//	infoLog( "buffer size scelta:" + to_string( m_nBufferSize ) );

	if ( ( err = snd_pcm_hw_params_set_period_size( m_pPlayback_handle, hw_params, period_size, 0 ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params_set_period_size: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
	}


	if ( ( err = snd_pcm_hw_params( m_pPlayback_handle, hw_params ) ) < 0 ) {
		ERRORLOG( QString( "error in snd_pcm_hw_params: %1" ).arg( QString::fromLocal8Bit(snd_strerror(err)) ) );
		return 1;
	}

	snd_pcm_hw_params_get_rate( hw_params, &m_nSampleRate, 0 );
	snd_pcm_hw_params_get_buffer_size( hw_params, &m_nBufferSize );

	INFOLOG( QString( "*** PERIOD SIZE: %1" ).arg( period_size ) );
	INFOLOG( QString( "*** SAMPLE RATE: %1" ).arg( m_nSampleRate ) );
	INFOLOG( QString( "*** BUFFER SIZE: %1" ).arg( m_nBufferSize ) );

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

	pthread_join( alsaAudioDriverThread, NULL );

	snd_pcm_close( m_pPlayback_handle );

	delete[] m_pOut_L;
	m_pOut_L = NULL;

	delete[] m_pOut_R;
	m_pOut_R = NULL;
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


void AlsaAudioDriver::updateTransportInfo()
{
	//errorLog( "[updateTransportInfo] not implemented yet" );
}

void AlsaAudioDriver::play()
{
	INFOLOG( "play" );
	m_transport.m_status = TransportInfo::ROLLING;
}

void AlsaAudioDriver::stop()
{
	INFOLOG( "stop" );
	m_transport.m_status = TransportInfo::STOPPED;
}

void AlsaAudioDriver::locate( unsigned long nFrame )
{
//	infoLog( "[locate] " + to_string( nFrame ) );
	m_transport.m_nFrames = nFrame;
//	m_transport.printInfo();
}

void AlsaAudioDriver::setBpm( float fBPM )
{
//	warningLog( "[setBpm] " + to_string(fBPM) );
	m_transport.m_nBPM = fBPM;
}

};

#endif // ALSA_SUPPORT
