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

#include <core/IO/PortAudioDriver.h>
#if defined(H2CORE_HAVE_PORTAUDIO) || _DOXYGEN_

#include <iostream>

#include <core/Preferences.h>
namespace H2Core
{

int portAudioCallback(
	const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData
)
{
	PortAudioDriver *pDriver = ( PortAudioDriver* )userData;
	pDriver->m_processCallback( pDriver->m_nBufferSize, nullptr );

	float *out = ( float* )outputBuffer;

	for ( unsigned i = 0; i < framesPerBuffer; i++ ) {
		*out++ = pDriver->m_pOut_L[ i ];
		*out++ = pDriver->m_pOut_R[ i ];
	}
	return 0;
}


const char* PortAudioDriver::__class_name = "PortAudioDriver";

PortAudioDriver::PortAudioDriver( audioProcessCallback processCallback )
		: AudioOutput( __class_name )
		, m_processCallback( processCallback )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_pStream( nullptr )
{
	INFOLOG( "INIT" );
	m_nBufferSize = Preferences::get_instance()->m_nBufferSize;
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
}


PortAudioDriver::~PortAudioDriver()
{
	INFOLOG( "DESTROY" );
}

int PortAudioDriver::init( unsigned nBufferSize )
{
	return 0;
}


//
// Connect
// return 0: Ok
// return 1: Generic error
//
int PortAudioDriver::connect()
{
	INFOLOG( "[connect]" );

	m_pOut_L = new float[ m_nBufferSize ];
	m_pOut_R = new float[ m_nBufferSize ];

	int err = Pa_Initialize();


	if ( err != paNoError ) {
		ERRORLOG( "Portaudio error in Pa_Initialize: " + QString( Pa_GetErrorText( err ) ) );
		return 1;
	}

	err = Pa_OpenDefaultStream(
				&m_pStream,        /* passes back stream pointer */
				0,              /* no input channels */
				2,              /* stereo output */
				paFloat32,      /* 32 bit floating point output */
				m_nSampleRate,          // sample rate
				m_nBufferSize,            // frames per buffer
				portAudioCallback, /* specify our custom callback */
				this );        /* pass our data through to callback */


	if ( err != paNoError ) {
		ERRORLOG(  "Portaudio error in Pa_OpenDefaultStream: " + QString( Pa_GetErrorText( err ) ) );
		return 1;
	}

	err = Pa_StartStream( m_pStream );


	if ( err != paNoError ) {
		ERRORLOG(  "Portaudio error in Pa_StartStream: " + QString( Pa_GetErrorText( err ) ) );
		return 1;
	}
	return 0;
}

void PortAudioDriver::disconnect()
{
	int err = Pa_StopStream( m_pStream );


	if ( err != paNoError ) {
		ERRORLOG( "Err: " + QString( Pa_GetErrorText( err ) ) );
	}

	err = Pa_CloseStream( m_pStream );


	if ( err != paNoError ) {
		ERRORLOG( "Err: " + QString( Pa_GetErrorText( err ) ) );
	}

	Pa_Terminate();

	delete[] m_pOut_L;
	m_pOut_L = nullptr;

	delete[] m_pOut_R;
	m_pOut_R = nullptr;
}

unsigned PortAudioDriver::getBufferSize()
{
	return m_nBufferSize;
}

unsigned PortAudioDriver::getSampleRate()
{
	return m_nSampleRate;
}

float* PortAudioDriver::getOut_L()
{
	return m_pOut_L;
}

float* PortAudioDriver::getOut_R()
{
	return m_pOut_R;
}

void PortAudioDriver::updateTransportInfo()
{
}

void PortAudioDriver::play()
{
	m_transport.m_status = TransportInfo::ROLLING;
}

void PortAudioDriver::stop()
{
	m_transport.m_status = TransportInfo::STOPPED;
}

void PortAudioDriver::locate( unsigned long nFrame )
{
	m_transport.m_nFrames = nFrame;
}

void PortAudioDriver::setBpm( float fBPM )
{
	m_transport.m_fBPM = fBPM;
}

};

#endif

