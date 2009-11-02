/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://hydrogen.sourceforge.net
 *
 * CoreAudio Driver for Hydrogen
 * Copyright(c) 2005 by Jonathan Dempsey [jonathandempsey@fastmail.fm]
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
 * Rewrote CoreAudio driver, now using AUHAL (2005/03/02 Jonathan Dempsey)
 * Set Hydrogen to detect hardware device buffer size (2005/03/06 Jonathan Dempsey)
 * Cleaned up the code a bit (2005/11/29 Jonathan Dempsey)
 * More cleaning . . . (2005/12/28 Jonathan Dempsey)
 */

#include "CoreAudioDriver.h"

#ifdef Q_OS_MACX

#include "CoreServices/CoreServices.h"
///
/// The Render Callback
///
static OSStatus renderProc(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData
)
{
	H2Core::CoreAudioDriver* pDriver = ( H2Core::CoreAudioDriver * )inRefCon;
	pDriver->mProcessCallback( pDriver->m_nBufferSize, NULL );
	for ( unsigned i = 0; i < ioData->mNumberBuffers; i++ ) {
		AudioBuffer &outData = ioData->mBuffers[ i ];
		Float32* pOutData = ( float* )( outData.mData );

		float *pAudioSource;
		if ( i == 0 ) {
			pAudioSource = pDriver->m_pOut_L;
		} else {
			pAudioSource = pDriver->m_pOut_R;
		}
		for ( unsigned j = 0; j < inNumberFrames; ++j ) {
			pOutData[ j ] = pAudioSource[ j ];
		}
	}

	return noErr;
}


namespace H2Core
{

CoreAudioDriver::CoreAudioDriver( audioProcessCallback processCallback )
		: H2Core::AudioOutput( "CoreAudioDriver" )
		, m_bIsRunning( false )
		, mProcessCallback( processCallback )
		, m_pOut_L( NULL )
		, m_pOut_R( NULL )
{
	//INFOLOG( "INIT" );
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
	//  m_nBufferSize = Preferences::get_instance()->m_nBufferSize;
	//  BufferSize is currently set to match the default audio device.

	OSStatus err;

	UInt32 size = sizeof( AudioDeviceID );
	err = AudioHardwareGetProperty(
	          kAudioHardwarePropertyDefaultOutputDevice,
	          &size,
	          &m_outputDevice
	      );
	if ( err != noErr ) {
		ERRORLOG( "Could not get Default Output Device" );
	}

	UInt32 dataSize = sizeof( m_nBufferSize );
	err = AudioDeviceGetProperty(
	          m_outputDevice,
	          0,
	          false,
	          kAudioDevicePropertyBufferFrameSize,
	          &dataSize,
	          ( void * )&m_nBufferSize
	      );

	if ( err != noErr ) {
		ERRORLOG( "get BufferSize error" );
	}
	INFOLOG( QString( "Buffersize: %1" ).arg( m_nBufferSize ) );


	// print some info
	AudioStreamBasicDescription outputStreamBasicDescription;
	UInt32 propertySize = sizeof( outputStreamBasicDescription );
	err = AudioDeviceGetProperty( m_outputDevice, 0, 0, kAudioDevicePropertyStreamFormat, &propertySize, &outputStreamBasicDescription );
	if ( err ) {
		printf( "AudioDeviceGetProperty: returned %d when getting kAudioDevicePropertyStreamFormat", err );
	}

	INFOLOG( QString("SampleRate: %1").arg( outputStreamBasicDescription.mSampleRate ) );
	INFOLOG( QString("BytesPerPacket: %1").arg( outputStreamBasicDescription.mBytesPerPacket ) );
	INFOLOG( QString("FramesPerPacket: %1").arg( outputStreamBasicDescription.mFramesPerPacket ) );
	INFOLOG( QString("BytesPerFrame: %1").arg( outputStreamBasicDescription.mBytesPerFrame ) );
	INFOLOG( QString("ChannelsPerFrame: %1").arg( outputStreamBasicDescription.mChannelsPerFrame ) );
	INFOLOG( QString("BitsPerChannel: %1").arg( outputStreamBasicDescription.mBitsPerChannel ) );
}



CoreAudioDriver::~CoreAudioDriver()
{
	//INFOLOG( "DESTROY" );
	disconnect();
}



int CoreAudioDriver::init( unsigned bufferSize )
{
	OSStatus err = noErr;

	m_pOut_L = new float[ m_nBufferSize ];
	m_pOut_R = new float[ m_nBufferSize ];

	memset ( m_pOut_L, 0, m_nBufferSize * sizeof( float ) );
	memset ( m_pOut_R, 0, m_nBufferSize * sizeof( float ) );

// Get Component
        Component compOutput;
        ComponentDescription descAUHAL;

	descAUHAL.componentType = kAudioUnitType_Output;
	descAUHAL.componentSubType = kAudioUnitSubType_HALOutput;
	descAUHAL.componentManufacturer = kAudioUnitManufacturer_Apple;
	descAUHAL.componentFlags = 0;
	descAUHAL.componentFlagsMask = 0;

	compOutput = FindNextComponent( NULL, &descAUHAL );
	if ( compOutput == NULL ) {
		ERRORLOG( "Error in FindNextComponent" );
		//exit (-1);
	}

	err = OpenAComponent( compOutput, &m_outputUnit );
	if ( err != noErr ) {
		ERRORLOG( "Error Opening Component" );
	}

// Get Current Output Device
	UInt32 size = sizeof( AudioDeviceID );
	err = AudioHardwareGetProperty(
	          kAudioHardwarePropertyDefaultOutputDevice,
	          &size,
	          &m_outputDevice
	      );
	if ( err != noErr ) {
		ERRORLOG( "Could not get Default Output Device" );
	}

// Set AUHAL to Current Device
	err = AudioUnitSetProperty(
	          m_outputUnit,
	          kAudioOutputUnitProperty_CurrentDevice,
	          kAudioUnitScope_Global,
	          0,
	          &m_outputDevice,
	          sizeof( m_outputDevice )
	      );
	if ( err != noErr ) {
		ERRORLOG( "Could not set Current Device" );
	}

//	UInt32 dataSize = sizeof(cBufferSize);

	/*	err =   AudioDeviceGetProperty(outputDevice, 0, false, kAudioDevicePropertyBufferFrameSize,
				&dataSize, (void *) &cBufferSize);
				if (err != noErr ) {
				printf( "Coud not get BufferSize" );
				}
				else{
				fprintf(stderr, "%lu\n", cBufferSize);
				}
	*/

	AudioStreamBasicDescription asbdesc;
	asbdesc.mSampleRate = ( Float64 )m_nSampleRate;
	asbdesc.mFormatID = kAudioFormatLinearPCM;
	asbdesc.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
	asbdesc.mBytesPerPacket = sizeof( Float32 );
	asbdesc.mFramesPerPacket = 1;
	asbdesc.mBytesPerFrame = sizeof( Float32 );
	asbdesc.mChannelsPerFrame = 2;	// comix: was set to 1
	asbdesc.mBitsPerChannel = 32;



	err = AudioUnitSetProperty(
	          m_outputUnit,
	          kAudioUnitProperty_StreamFormat,
	          kAudioUnitScope_Input,
	          0,
	          &asbdesc,
	          sizeof( AudioStreamBasicDescription )
	      );

// Set Render Callback
	AURenderCallbackStruct out;
	out.inputProc = renderProc;
	out.inputProcRefCon = ( void * )this;

	err = AudioUnitSetProperty(
	          m_outputUnit,
	          kAudioUnitProperty_SetRenderCallback,
	          kAudioUnitScope_Global,
	          0,
	          &out,
	          sizeof( out )
	      );
	if ( err != noErr ) {
		ERRORLOG( "Could not Set Render Callback" );
	}

//Initialize AUHAL
	err = AudioUnitInitialize( m_outputUnit );
	if ( err != noErr ) {
		ERRORLOG( "Could not Initialize AudioUnit" );
	}

	return 0;
}



int CoreAudioDriver::connect()
{
	OSStatus err;
	err = AudioOutputUnitStart( m_outputUnit );
	if ( err != noErr ) {
		ERRORLOG( "Could not start AudioUnit" );
	}

	m_bIsRunning = true;
	return 0;
}



void CoreAudioDriver::disconnect()
{
	OSStatus err = noErr;
	err = AudioOutputUnitStop( m_outputUnit );
	err = AudioUnitUninitialize( m_outputUnit );
	err = CloseComponent( m_outputUnit );
}



void CoreAudioDriver::play()
{
	//INFOLOG( "play" );
	m_transport.m_status = TransportInfo::ROLLING;
}



void CoreAudioDriver::stop()
{
	//INFOLOG( "stop" );
	m_transport.m_status = TransportInfo::STOPPED;
}



float* CoreAudioDriver::getOut_L()
{
	return m_pOut_L;
}



float* CoreAudioDriver::getOut_R()
{
	return m_pOut_R;
}



unsigned CoreAudioDriver::getBufferSize()
{
	return m_nBufferSize;
}



unsigned CoreAudioDriver::getSampleRate()
{
	return m_nSampleRate;
}



void CoreAudioDriver::updateTransportInfo()
{
	// INFOLOG( "nothing");
}



void CoreAudioDriver::locate( unsigned long nFrame )
{
	//INFOLOG( "locate: " + to_string( nFrame ) );
	m_transport.m_nFrames = nFrame;
	//fprintf ( stderr, "m_transport.m_nFrames = %lu\n", m_transport.m_nFrames );
}



void CoreAudioDriver::setBpm( float fBPM )
{
	//INFOLOG( "[setBpm]" + to_string( fBPM ));
	m_transport.m_nBPM = fBPM;
}

}

#endif // Q_OS_MACX
