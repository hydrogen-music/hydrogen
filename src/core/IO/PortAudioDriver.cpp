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

#include <core/IO/PortAudioDriver.h>
#if defined(H2CORE_HAVE_PORTAUDIO) || _DOXYGEN_

#include <iostream>

#include <core/Preferences/Preferences.h>
namespace H2Core
{

int portAudioCallback(
	const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData )
{
	float *out = ( float* )outputBuffer;
	PortAudioDriver *pDriver = ( PortAudioDriver* )userData;
	if ( pDriver == nullptr ) {
		___ERRORLOG( "Invalid driver pointer" );
		return 1;
	}

	while ( framesPerBuffer > 0 ) {
		unsigned long nFrames = std::min( (unsigned long) MAX_BUFFER_SIZE, framesPerBuffer );
		pDriver->m_processCallback( nFrames, nullptr );

		for ( unsigned i = 0; i < nFrames; i++ ) {
			*out++ = pDriver->m_pOut_L[ i ];
			*out++ = pDriver->m_pOut_R[ i ];
		}
		framesPerBuffer -= nFrames;
	}
	return 0;
}


bool PortAudioDriver::m_bInitialised = false;


PortAudioDriver::PortAudioDriver( audioProcessCallback processCallback )
		: AudioOutput()
		, m_processCallback( processCallback )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_pStream( nullptr )
{
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
	m_sDevice = Preferences::get_instance()->m_sPortAudioDevice;
}


PortAudioDriver::~PortAudioDriver() {
}

int PortAudioDriver::init( unsigned nBufferSize )
{
	return 0;
}


// String list of API names
QStringList PortAudioDriver::getHostAPIs()
{
	if ( ! m_bInitialised ) {
		Pa_Initialize();
		m_bInitialised = true;
	}

	QStringList hostAPIs;
	int nHostAPIs = Pa_GetHostApiCount();
	for ( int n = 0; n < nHostAPIs; n++ ) {
		const PaHostApiInfo *pHostApiInfo = Pa_GetHostApiInfo( (PaHostApiIndex)n );
		if ( pHostApiInfo == nullptr ) {
			ERRORLOG( QString( "Invalid host API [%1]" ).arg( n ) );
			continue;
		}
		hostAPIs.push_back( pHostApiInfo->name );
	}

	return hostAPIs;
}
	
// List devices
QStringList PortAudioDriver::getDevices( QString HostAPI ) {
	if ( ! m_bInitialised ) {
		Pa_Initialize();
		m_bInitialised = true;
	}

	QStringList devices;
	if ( HostAPI.isNull() || HostAPI == "" ) {
		WARNINGLOG( "Using default HostAPI" );
		auto pInfo = Pa_GetHostApiInfo( Pa_GetDefaultHostApi() );
		if ( pInfo == nullptr ) {
			ERRORLOG( "Unable to obtain default Host API" );
			return devices;
		}
		
		HostAPI = pInfo->name;
	}

	int nDevices = Pa_GetDeviceCount();
	for ( int nDevice = 0; nDevice < nDevices; nDevice++ ) {
		const PaDeviceInfo *pDeviceInfo = Pa_GetDeviceInfo( nDevice );
		if ( pDeviceInfo == nullptr ) {
			continue;
		}

		// Filter by API
		auto pInfo = Pa_GetHostApiInfo( pDeviceInfo->hostApi );
		if ( pInfo == nullptr || pInfo->name != HostAPI ) {
			continue;
		}
		if ( pDeviceInfo->maxOutputChannels >= 2 ) {
			devices.push_back( QString( pDeviceInfo->name ) );
		}
	}

	return devices;
}

QStringList PortAudioDriver::getDevices() {
	Preferences *pPreferences = Preferences::get_instance();
	return getDevices( pPreferences->m_sPortAudioHostAPI );
}

//
// Connect
// return 0: Ok
// return 1: Generic error
//
int PortAudioDriver::connect()
{
	bool bUseDefaultStream = true;
	Preferences *pPreferences = Preferences::get_instance();
	INFOLOG( "[connect]" );

	m_pOut_L = new float[ MAX_BUFFER_SIZE ];
	m_pOut_R = new float[ MAX_BUFFER_SIZE ];

	// Reset buffers to avoid noise during startup (e.g. in case the callback
	// was not able to obtain the audio engine lock, the arbitrary numbers
	// filling the buffer after creation will be passed to the audio output).
	for ( int ii = 0; ii < MAX_BUFFER_SIZE; ii++ ) {
		m_pOut_L[ ii ] = 0;
		m_pOut_R[ ii ] = 0;
	}

	int err;
	if ( ! m_bInitialised ) {
		err = Pa_Initialize();

		if ( err != paNoError ) {
			ERRORLOG( "Portaudio error in Pa_Initialize: " + QString( Pa_GetErrorText( err ) ) );
			return 1;
		}
		m_bInitialised = true;

	}


	// Find device to use
	int nDevices = Pa_GetDeviceCount();
	const PaDeviceInfo *pDeviceInfo;
	for ( int nDevice = 0; nDevice < nDevices; nDevice++ ) {
		pDeviceInfo = Pa_GetDeviceInfo( nDevice );
		if ( pDeviceInfo == nullptr ) {
			continue;
		}
		
		// Filter by HostAPI
		if ( ! pPreferences->m_sPortAudioHostAPI.isNull() || pPreferences->m_sPortAudioHostAPI != "" ) {
			auto pInfo = Pa_GetHostApiInfo( pDeviceInfo->hostApi );
			if ( pInfo == nullptr || pInfo->name != pPreferences->m_sPortAudioHostAPI ) {
				continue;
			}
		}

		if ( pDeviceInfo->maxOutputChannels >= 2
			 && ( QString::compare( m_sDevice,  pDeviceInfo->name, Qt::CaseInsensitive ) == 0 ||
			      m_sDevice.isNull() || m_sDevice == "" ) ) {
			PaStreamParameters outputParameters;
			memset( &outputParameters, '\0', sizeof( outputParameters ) );
			outputParameters.channelCount = 2;
			outputParameters.device = nDevice;
			outputParameters.hostApiSpecificStreamInfo = nullptr;
			outputParameters.sampleFormat = paFloat32;

			// Use the same latency setting as Pa_OpenDefaultStream() -- defaulting to the high suggested
			// latency. This should probably be an option.
			outputParameters.suggestedLatency =
				Pa_GetDeviceInfo( nDevice )->defaultHighInputLatency;
			if ( pPreferences->m_nLatencyTarget > 0 ) {
				outputParameters.suggestedLatency = pPreferences->m_nLatencyTarget * 1.0 / getSampleRate();
			}

			err = Pa_OpenStream( &m_pStream,
								 nullptr, /* No input stream */
								 &outputParameters,
								 m_nSampleRate, paFramesPerBufferUnspecified, paNoFlag,
								 portAudioCallback, this );
			if ( err != paNoError ) {
				ERRORLOG( QString( "Found but can't open device '%1' (max %3 in, %4 out): %2" )
						  .arg( m_sDevice ).arg( Pa_GetErrorText( err ) )
						  .arg( pDeviceInfo->maxInputChannels ).arg( pDeviceInfo->maxOutputChannels ) );
				// Use the default stream
				break;
			}
			INFOLOG( QString( "Opened device '%1'" ).arg( m_sDevice ) );
			bUseDefaultStream = false;
			break;
		}

		if ( bUseDefaultStream ) {
			ERRORLOG( QString( "Can't use device '%1', using default stream" )
					  .arg( m_sDevice ) );
		}
	}

	if ( bUseDefaultStream ) {
		// Failed to open the request device. Use the default device.
		// Less than desirably, this will also use the default latency settings.
		err = Pa_OpenDefaultStream(
					&m_pStream,        /* passes back stream pointer */
					0,              /* no input channels */
					2,              /* stereo output */
					paFloat32,      /* 32 bit floating point output */
					m_nSampleRate,          // sample rate
					paFramesPerBufferUnspecified, // frames per buffer
					portAudioCallback, /* specify our custom callback */
					this );        /* pass our data through to callback */
	}

	if ( err != paNoError ) {
		ERRORLOG(  "Portaudio error in Pa_OpenDefaultStream: " + QString( Pa_GetErrorText( err ) ) );
		return 1;
	}

	if ( m_pStream == nullptr ) {
		ERRORLOG( "Invalid stream." );
		return 1;
	}

	const PaStreamInfo *pStreamInfo = Pa_GetStreamInfo( m_pStream );
	if ( pStreamInfo == nullptr ) {
		ERRORLOG( "Invalid stream info." );
		return 1;
	}
	
	if ( (unsigned) pStreamInfo->sampleRate != m_nSampleRate ) {
		ERRORLOG( QString( "Couldn't get sample rate %d, using %d instead" ).arg( m_nSampleRate ).arg( pStreamInfo->sampleRate ) );
		m_nSampleRate = (unsigned) pStreamInfo->sampleRate;
	}
	INFOLOG( QString( "PortAudio outpot latency: %1 s" ).arg( pStreamInfo->outputLatency ) );

	err = Pa_StartStream( m_pStream );


	if ( err != paNoError ) {
		ERRORLOG(  "Portaudio error in Pa_StartStream: " + QString( Pa_GetErrorText( err ) ) );
		return 1;
	}
	return 0;
}

void PortAudioDriver::disconnect()
{
	if ( m_pStream != nullptr ) {
		
		int err = Pa_StopStream( m_pStream );
		if ( err != paNoError ) {
			ERRORLOG( "Err: " + QString( Pa_GetErrorText( err ) ) );
		}

		err = Pa_CloseStream( m_pStream );
		if ( err != paNoError ) {
			ERRORLOG( "Err: " + QString( Pa_GetErrorText( err ) ) );
		}
	}

	m_bInitialised = false;
	Pa_Terminate();

	delete[] m_pOut_L;
	m_pOut_L = nullptr;

	delete[] m_pOut_R;
	m_pOut_R = nullptr;
}

unsigned PortAudioDriver::getBufferSize()
{
	return MAX_BUFFER_SIZE;
}

unsigned PortAudioDriver::getSampleRate()
{
	return m_nSampleRate;
}

int PortAudioDriver::getLatency()
{
	if ( m_pStream == nullptr ) {
		return 0;
	}
	
	const PaStreamInfo *pStreamInfo = Pa_GetStreamInfo( m_pStream );
	if ( pStreamInfo == nullptr ) {
		ERRORLOG( "Invalid stream info" );
		return 0;
	}
	
	return std::max( static_cast<int>( pStreamInfo->outputLatency * getSampleRate() ),
					 0 );
}

float* PortAudioDriver::getOut_L()
{
	return m_pOut_L;
}

float* PortAudioDriver::getOut_R()
{
	return m_pOut_R;
}

};

#endif

