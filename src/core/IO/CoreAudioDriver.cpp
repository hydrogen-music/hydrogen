/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 * Rewrote CoreAudio driver, now using AUHAL (2005/03/02 Jonathan Dempsey)
 * Set Hydrogen to detect hardware device buffer size (2005/03/06 Jonathan Dempsey)
 * Cleaned up the code a bit (2005/11/29 Jonathan Dempsey)
 * More cleaning . . . (2005/12/28 Jonathan Dempsey)
 */

#include <core/IO/CoreAudioDriver.h>

#if defined(H2CORE_HAVE_COREAUDIO) || _DOXYGEN_

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
	assert( ioData->mNumberBuffers > 0 && ioData->mNumberBuffers <= 2 );
	pDriver->m_pOut_L =  static_cast< float *>( ioData->mBuffers[ 0 ].mData );
	pDriver->m_pOut_R =  static_cast< float *>( ioData->mBuffers[ 1 ].mData );
	pDriver->mProcessCallback( inNumberFrames, NULL );
	pDriver->m_pOut_L = nullptr;
	pDriver->m_pOut_R = nullptr;
	return noErr;
}



namespace H2Core
{

int CoreAudioDriver::getLatency() {
	// Calculate the overall latency as the device latency + the stream latency.
	OSStatus err;
	UInt32 nSize;

	AudioObjectPropertyAddress propertyAddress = {
		kAudioDevicePropertyLatency,
		kAudioDevicePropertyScopeInput,
		0
	};
	UInt32 nDeviceLatency;
	nSize = sizeof( nDeviceLatency );
	err = AudioObjectGetPropertyData( m_outputDevice, &propertyAddress, 0, NULL, &nSize, &nDeviceLatency );
	if ( err != noErr ) {
		ERRORLOG( "Couldn't get device latency" );
		return -1;
	}

	// Find the stream ID for the output stream, then find the latency
	AudioStreamID streamID;
	nSize = sizeof( streamID );
	propertyAddress = {
		kAudioDevicePropertyStreams,
		kAudioDevicePropertyScopeOutput,
		0
	};
	err = AudioObjectGetPropertyData( m_outputDevice, &propertyAddress, 0, NULL, &nSize, &streamID );
	if ( err != noErr ) {
		ERRORLOG( "Couldn't get stream for output device" );
	}

	UInt32 nStreamLatency;
	nSize = sizeof(nStreamLatency);
	propertyAddress = {
		kAudioStreamPropertyLatency,
		kAudioObjectPropertyScopeOutput,
		0
	};
	err = AudioObjectGetPropertyData( streamID, &propertyAddress, 0, NULL, &nSize, &nStreamLatency );
	if ( err != noErr ) {
		ERRORLOG( QString("Couldn't get stream latency") );
	}

	return nDeviceLatency + nStreamLatency + m_nBufferSize;
}

QString CoreAudioDriver::deviceName( AudioDeviceID deviceID )
{
	OSStatus err;
	CFStringRef deviceNameRef;
	UInt32 size = sizeof( deviceNameRef );
	AudioObjectPropertyAddress propertyAddress = {
		kAudioDevicePropertyDeviceNameCFString,
		kAudioDevicePropertyScopeOutput,
		0
	};
	err = AudioObjectGetPropertyData( deviceID, &propertyAddress, 0, NULL, &size, &deviceNameRef );
	if ( err != noErr ) {
		ERRORLOG( QString( "Couldn't get name for device %1" ).arg( deviceID ) );
		return QString();
	}
	UInt32 nBufferSize = CFStringGetMaximumSizeForEncoding( CFStringGetLength( deviceNameRef ), kCFStringEncodingUTF8 );
	char buffer[ nBufferSize + 1 ];
	CFStringGetCString( deviceNameRef, buffer, nBufferSize + 1, kCFStringEncodingUTF8 );
	CFRelease( deviceNameRef );

	return QString( buffer );

}

std::vector< AudioDeviceID > CoreAudioDriver::outputDeviceIDs()
{
	std::vector< AudioDeviceID > outputDeviceIDs;
	QStringList res;
	UInt32 dataSize;
	OSStatus err;

	// Read the 'Devices' system property

	AudioObjectPropertyAddress propertyAddress = {
		kAudioHardwarePropertyDevices,
		kAudioObjectPropertyScopeGlobal,
		kAudioObjectPropertyElementMaster
	};

	err = AudioObjectGetPropertyDataSize( kAudioObjectSystemObject,
										  &propertyAddress, 0, NULL, &dataSize );
	if ( err != noErr ) {
		ERRORLOG( "Couldn't get size for devices list" );
		return outputDeviceIDs;
	}

	int nDevices = dataSize / sizeof( AudioDeviceID );
	AudioDeviceID deviceIDs[ nDevices ];

	err = AudioObjectGetPropertyData( kAudioObjectSystemObject,
									  &propertyAddress, 0, NULL, &dataSize, deviceIDs );
	if ( err != noErr ) {
		ERRORLOG( "Couldn't read device IDs" );
		return outputDeviceIDs;
	}

	// Find suitable output devices

	for ( int i = 0; i < nDevices; i++ ) {
		UInt32 nBufferListSize = 0;
		AudioObjectPropertyAddress propertyAddress = {
			kAudioDevicePropertyStreamConfiguration,
			kAudioDevicePropertyScopeOutput,
			0
		};
		err = AudioObjectGetPropertyDataSize( deviceIDs[ i ], &propertyAddress, 0, NULL, &nBufferListSize );
		if ( err != noErr ) {
			ERRORLOG( "Couldn't get device config size" );
			continue;
		}
		AudioBufferList *pBufferList = (AudioBufferList *) alloca( nBufferListSize );
		err = AudioObjectGetPropertyData( deviceIDs[ i ], &propertyAddress, 0, NULL, &nBufferListSize, pBufferList );

		int nChannels = 0;
		for ( int nBuffer = 0; nBuffer < pBufferList->mNumberBuffers; nBuffer++ ) {
			nChannels += pBufferList->mBuffers[ nBuffer ].mNumberChannels;
		}
		if ( nChannels < 2 ) {
			// Skip input devices and any mono outputs
			if ( nChannels == 1 ) {
				INFOLOG( QString( "Skipping mono output device %1" ).arg( deviceIDs[ i ] ) );
			}
			continue;
		}

		outputDeviceIDs.push_back( deviceIDs[ i ] );
	}

	return outputDeviceIDs;
}


QStringList CoreAudioDriver::getDevices()
{
	QStringList res;
	res.push_back( "default" );
	for ( AudioDeviceID device : outputDeviceIDs() ) {
		res.push_back( deviceName( device ) );
	}
	return res;
}

AudioDeviceID CoreAudioDriver::preferredOutputDevice()
{
	QString sPreferredDeviceName = Preferences::get_instance()->m_sCoreAudioDevice;

	if ( sPreferredDeviceName.isNull()
		 || QString::compare( sPreferredDeviceName, "default", Qt::CaseInsensitive ) == 0 ) {
		INFOLOG( "Using default device" );
		return defaultOutputDevice();
	}
	for ( AudioDeviceID device : outputDeviceIDs() ) {
		QString sDeviceName = deviceName( device );
		if ( QString::compare( sDeviceName, sPreferredDeviceName, Qt::CaseInsensitive ) == 0 ) {
			// Found it.
			INFOLOG( QString( "Found device '%1' (%2) for preference '%3'" )
					 .arg( sDeviceName ).arg( (int)device ).arg( sPreferredDeviceName ) );
			return device;
		}
	}
	ERRORLOG( QString( "Couldn't find device '%1', falling back to default" ).arg( sPreferredDeviceName ) );
	return defaultOutputDevice();
}

AudioDeviceID CoreAudioDriver::defaultOutputDevice(void)
{
	getDevices();
	UInt32 dataSize = 0;
	OSStatus err = 0;
	AudioDeviceID device;

	AudioObjectPropertyAddress propertyAddress = {
		kAudioHardwarePropertyDefaultOutputDevice,
		kAudioObjectPropertyScopeGlobal,
		kAudioObjectPropertyElementMaster
	};

	dataSize = sizeof(AudioDeviceID);
	err = AudioObjectGetPropertyData(kAudioObjectSystemObject,
											&propertyAddress,
											0,
											NULL,
											&dataSize,
											&device);

	if ( err != noErr ) {
		ERRORLOG( "Could not get Default Output Device" );
	}
	return device;
}

void CoreAudioDriver::retrieveBufferSize(void)
{
	UInt32 dataSize = 0;
	OSStatus err = 0;

	AudioObjectPropertyAddress propertyAddress = {
		kAudioDevicePropertyBufferFrameSize,
		kAudioObjectPropertyScopeGlobal,
		kAudioObjectPropertyElementMaster
	};

	dataSize = sizeof( m_nBufferSize );

	err = AudioObjectGetPropertyData(m_outputDevice,
							&propertyAddress,
							0,
							NULL,
							&dataSize,
							( void * )&m_nBufferSize
						);

	if ( err != noErr ) {
		ERRORLOG( "get BufferSize error" );
	}
	INFOLOG( QString( "Buffersize: %1" ).arg( m_nBufferSize ) );
}

void CoreAudioDriver::printStreamInfo(void)
{
	AudioStreamBasicDescription outputStreamBasicDescription;
	UInt32 propertySize = sizeof( outputStreamBasicDescription );
	OSStatus err = 0;

	AudioObjectPropertyAddress propertyAddress = {
		kAudioDevicePropertyStreamFormat,
		kAudioObjectPropertyScopeGlobal,
		kAudioObjectPropertyElementMaster
	};

	err = AudioObjectGetPropertyData(m_outputDevice,
							&propertyAddress,
							0,
							NULL,
							&propertySize,
							&outputStreamBasicDescription
						);

	if ( err ) {
		ERRORLOG( QString("AudioDeviceGetProperty: returned %1 when getting kAudioDevicePropertyStreamFormat").arg(err) );
	}

	INFOLOG( QString("SampleRate: %1").arg( outputStreamBasicDescription.mSampleRate ) );
	INFOLOG( QString("BytesPerPacket: %1").arg( outputStreamBasicDescription.mBytesPerPacket ) );
	INFOLOG( QString("FramesPerPacket: %1").arg( outputStreamBasicDescription.mFramesPerPacket ) );
	INFOLOG( QString("BytesPerFrame: %1").arg( outputStreamBasicDescription.mBytesPerFrame ) );
	INFOLOG( QString("ChannelsPerFrame: %1").arg( outputStreamBasicDescription.mChannelsPerFrame ) );
	INFOLOG( QString("BitsPerChannel: %1").arg( outputStreamBasicDescription.mBitsPerChannel ) );
}


CoreAudioDriver::CoreAudioDriver( audioProcessCallback processCallback )
		: H2Core::AudioDriver()
		, H2Core::Object<CoreAudioDriver>()
		, m_bIsRunning( false )
		, mProcessCallback( processCallback )
		, m_pOut_L( NULL )
		, m_pOut_R( NULL )
{
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;

	//Get the default playback device and store it in m_outputDevice
	m_outputDevice = preferredOutputDevice();

	//Get the buffer size of the previously detected device and store it in m_nBufferSize
	retrieveBufferSize();

	// print some info
	printStreamInfo();
}



CoreAudioDriver::~CoreAudioDriver()
{
	disconnect();
}



int CoreAudioDriver::init( unsigned nBufferSize )
{
	OSStatus err = noErr;

	// Get Component
	AudioComponent compOutput;
	AudioComponentDescription descAUHAL;

	descAUHAL.componentType = kAudioUnitType_Output;
	descAUHAL.componentSubType = kAudioUnitSubType_HALOutput;
	descAUHAL.componentManufacturer = kAudioUnitManufacturer_Apple;
	descAUHAL.componentFlags = 0;
	descAUHAL.componentFlagsMask = 0;

	compOutput = AudioComponentFindNext( NULL, &descAUHAL );
	if ( compOutput == NULL ) {
		ERRORLOG( "Error in FindNextComponent" );
		//exit (-1);
	}

	err = AudioComponentInstanceNew( compOutput, &m_outputUnit );
	if ( err != noErr ) {
		ERRORLOG( "Error Opening Component" );
	}

	// Get Current Output Device
	m_outputDevice = preferredOutputDevice();

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

	// Set buffer size
	INFOLOG( QString( "Setting buffer size to %1" ).arg( nBufferSize ) );
	AudioObjectPropertyAddress propertyAddress = {
		kAudioDevicePropertyBufferFrameSize,
		kAudioObjectPropertyScopeGlobal,
		kAudioObjectPropertyElementMaster
	};

	err = AudioObjectSetPropertyData( m_outputDevice,
									  &propertyAddress,
									  0,
									  NULL,
                                      sizeof(UInt32), &nBufferSize);

	if ( err != noErr ) {
		ERRORLOG( QString( "Could not set buffer size to %1" ).arg( nBufferSize ) );
	}

	retrieveBufferSize();

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
	err = AudioComponentInstanceDispose( m_outputUnit );
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

}

#endif // H2CORE_HAVE_COREAUDIO
