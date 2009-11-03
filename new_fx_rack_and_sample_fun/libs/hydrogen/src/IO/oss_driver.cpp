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

#include "OssDriver.h"

// check if OSS support is enabled
#ifdef OSS_SUPPORT

#include <hydrogen/Preferences.h>

#include <pthread.h>

namespace H2Core
{

audioProcessCallback ossDriver_audioProcessCallback;
bool ossDriver_running;
pthread_t ossDriverThread;
int oss_driver_bufferSize = -1;
OssDriver *m_pOssDriverInstance = NULL;

unsigned nNextFrames = 0;

void* ossDriver_processCaller( void* param )
{
	// stolen from amSynth
	struct sched_param sched;
	sched.sched_priority = 50;
	int res = sched_setscheduler( 0, SCHED_FIFO, &sched );
	sched_getparam( 0, &sched );
	if ( res ) {
		_WARNINGLOG( "Can't set realtime scheduling for OSS Driver" );
	}
	_INFOLOG( QString( "Scheduling priority = %1" ).arg( sched.sched_priority ) );

	OssDriver *ossDriver = ( OssDriver* )param;

	sleep( 1 );

	while ( ossDriver_running ) {
		ossDriver_audioProcessCallback( oss_driver_bufferSize, NULL );
		ossDriver->write();
	}

	pthread_exit( NULL );
	return NULL;
}




OssDriver::OssDriver( audioProcessCallback processCallback )
		: AudioOutput( "OssDriver" )
{
	INFOLOG( "INIT" );
	audioBuffer = NULL;
	ossDriver_running = false;
	this->processCallback = processCallback;
	ossDriver_audioProcessCallback = processCallback;
	m_pOssDriverInstance = this;
}






OssDriver::~OssDriver()
{
	INFOLOG( "DESTROY" );
}



int OssDriver::init( unsigned nBufferSize )
{
	oss_driver_bufferSize = nBufferSize;

	delete[] audioBuffer;
	audioBuffer = NULL;

	audioBuffer = new short[nBufferSize * 2];

	out_L = new float[nBufferSize];
	out_R = new float[nBufferSize];

	// clear buffers
	memset( out_L, 0, nBufferSize * sizeof( float ) );
	memset( out_R, 0, nBufferSize * sizeof( float ) );

	return 0;
}



/// Connect
/// return 0: Ok
/// return 1: Generic error
int OssDriver::connect()
{
	INFOLOG( "connect" );

	Preferences *preferencesMng = Preferences::get_instance();

	// initialize OSS
	int bits = 16;
	int speed = preferencesMng->m_nSampleRate;
	int stereo = 1;
	int bs;

	QString audioDevice;
#ifdef __NetBSD__
	audioDevice = "/dev/audio";
#else
	audioDevice = preferencesMng->m_sOSSDevice;
#endif

	// Non blocking OSS open code stolen from GLAME
	fd = open( audioDevice.toLocal8Bit(), O_WRONLY | O_NONBLOCK );	// test with non blocking open
	int arg = fcntl( fd, F_GETFL, 0 );
	if ( arg != -1 ) {	// back to blocking mode...
		fcntl( fd, F_SETFL, arg & ~O_NONBLOCK );
	}

	if ( fd == -1 ) {
		ERRORLOG( "DSP ERROR_OPEN" );
		return 1;
	}
	if ( ioctl( fd, SNDCTL_DSP_SYNC, NULL ) < 0 ) {
		ERRORLOG( "ERROR_IOCTL" );
		close( fd );
		return 1;
	}
	if ( ioctl( fd, SNDCTL_DSP_SAMPLESIZE, &bits ) < 0 ) {
		ERRORLOG( "ERROR_IOCTL" );
		close( fd );
		return 1;
	}
	if ( ioctl( fd, SNDCTL_DSP_SPEED, &speed ) < 0 ) {
		ERRORLOG( "ERROR_IOCTL" );
		close( fd );
		return 1;
	}
	if ( ioctl( fd, SNDCTL_DSP_STEREO, &stereo ) < 0 ) {
		ERRORLOG( "ERROR_IOCTL" );
		close( fd );
		return 1;
	}

	unsigned bufferBits = log2( speed / 60 );
	int fragSize = 0x00200000 | bufferBits;

	ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &fragSize );

	if ( ioctl( fd, SNDCTL_DSP_GETBLKSIZE, &bs ) < 0 ) {
		ERRORLOG( "ERROR_IOCTL" );
		close( fd );
		return 1;
	}

	INFOLOG( QString( "Blocksize audio = %1" ).arg( bs ) );

	if ( bs != ( 1 << bufferBits ) ) {
		ERRORLOG( "ERROR_IOCTL: unable to set BlockSize" );
		close( fd );
		return 1;
	}

	int format = AFMT_S16_LE;
	if ( ioctl( fd, SNDCTL_DSP_SETFMT, &format ) == -1 ) {
		ERRORLOG( "ERROR_IOCTL unable to set format" );
		close( fd );
		return 1;
	}

	// start main thread
	ossDriver_running = true;
	pthread_attr_t attr;
	pthread_attr_init( &attr );

	pthread_create( &ossDriverThread, &attr, ossDriver_processCaller, this );

	return 0;
}




void OssDriver::disconnect()
{
	INFOLOG( "disconnect" );

	ossDriver_running = false;

	// join ossDriverThread
	pthread_join( ossDriverThread, NULL );

	if ( fd != -1 ) {
		if ( close( fd ) ) {
			ERRORLOG( "Error closing audio device" );
		}
	}

	delete [] out_L;
	out_L = NULL;

	delete [] out_R;
	out_R = NULL;

	delete[] audioBuffer;
	audioBuffer = NULL;
}




/// Write the audio data
void OssDriver::write()
{
//	infoLog("write");
	unsigned size = oss_driver_bufferSize * 2;

	// prepare the 2-channel array of short
	for ( unsigned i = 0; i < ( unsigned )oss_driver_bufferSize; ++i ) {
		audioBuffer[i * 2] = ( short )( out_L[i] * 32768.0 );
		audioBuffer[i * 2 + 1] = ( short )( out_R[i] * 32768.0 );
	}

	unsigned long written = ::write( fd, audioBuffer, size * 2 );

	if ( written != ( size * 2 ) ) {
		ERRORLOG( "OssDriver: Error writing samples to audio device." );
//		std::cerr << "written = " << written << " of " << (size*2) << endl;
	}
}





int OssDriver::log2( int n )
{
	int result = 0;
	while ( ( n >>= 1 ) > 0 )
		result++;
	return result;
}




unsigned OssDriver::getBufferSize()
{
	return oss_driver_bufferSize;
}


unsigned OssDriver::getSampleRate()
{
	Preferences *preferencesMng = Preferences::get_instance();
	return preferencesMng->m_nSampleRate;
}


float* OssDriver::getOut_L()
{
	return out_L;
}
float* OssDriver::getOut_R()
{
	return out_R;
}


void OssDriver::play()
{
	m_transport.m_status = TransportInfo::ROLLING;
}

void OssDriver::stop()
{
	m_transport.m_status = TransportInfo::STOPPED;
}

void OssDriver::locate( unsigned long nFrame )
{
	m_transport.m_nFrames = nFrame;
}


void OssDriver::updateTransportInfo()
{
	// not used
}

void OssDriver::setBpm( float fBPM )
{
	INFOLOG( QString( "setBpm: %1" ).arg( fBPM ) );
	m_transport.m_nBPM = fBPM;
}

};

#endif // OSS support

