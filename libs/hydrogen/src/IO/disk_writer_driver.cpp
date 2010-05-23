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
#include "DiskWriterDriver.h"

#include <hydrogen/Preferences.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Pattern.h>

#include <pthread.h>
#include <cassert>

namespace H2Core
{

pthread_t diskWriterDriverThread;

void* diskWriterDriver_thread( void* param )
{
	DiskWriterDriver *pDriver = ( DiskWriterDriver* )param;
	_INFOLOG( "DiskWriterDriver thread start" );

	// always rolling, no user interaction
	pDriver->m_transport.m_status = TransportInfo::ROLLING;

	SF_INFO soundInfo;
	soundInfo.samplerate = pDriver->m_nSampleRate;
//	soundInfo.frames = -1;//getNFrames();		///\todo: da terminare
	soundInfo.channels = 2;
	//default format
	int sfformat = 0x010000; //wav format (default)
	int bits = 0x0002; //16 bit PCM (default)
	//sf_format switch
	if( pDriver->m_sFilename.endsWith(".aiff") || pDriver->m_sFilename.endsWith(".AIFF") ){
		sfformat =  0x020000; //Apple/SGI AIFF format (big endian)
	}
	if( pDriver->m_sFilename.endsWith(".flac") || pDriver->m_sFilename.endsWith(".FLAC") ){
                sfformat =  0x170000; //FLAC lossless file format
	}
	if( ( pDriver->m_nSampleDepth == 8 ) && ( pDriver->m_sFilename.endsWith(".aiff") || pDriver->m_sFilename.endsWith(".AIFF") ) ){ 
		bits = 0x0001; //Signed 8 bit data works with aiff
	}
	if( ( pDriver->m_nSampleDepth == 8 ) && ( pDriver->m_sFilename.endsWith(".wav") || pDriver->m_sFilename.endsWith(".WAV") ) ){ 
		bits = 0x0005; //Unsigned 8 bit data needed for Microsoft WAV format
	}
	if( pDriver->m_nSampleDepth == 16 ){
		bits = 0x0002; //Signed 16 bit data
	}
	if( pDriver->m_nSampleDepth == 24 ){
		bits = 0x0003; //Signed 24 bit data
	}
	if( pDriver->m_nSampleDepth == 32 ){
		bits = 0x0004; ////Signed 32 bit data
	}

	soundInfo.format =  sfformat|bits;

	//ogg vorbis option
	if( pDriver->m_sFilename.endsWith( ".ogg" ) | pDriver->m_sFilename.endsWith( ".OGG" ) )
		soundInfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;


///formats
//          SF_FORMAT_WAV          = 0x010000,     /* Microsoft WAV format (little endian). */
//          SF_FORMAT_AIFF         = 0x020000,     /* Apple/SGI AIFF format (big endian). */
//          SF_FORMAT_AU           = 0x030000,     /* Sun/NeXT AU format (big endian). */
//          SF_FORMAT_RAW          = 0x040000,     /* RAW PCM data. */
//          SF_FORMAT_PAF          = 0x050000,     /* Ensoniq PARIS file format. */
//          SF_FORMAT_SVX          = 0x060000,     /* Amiga IFF / SVX8 / SV16 format. */
//          SF_FORMAT_NIST         = 0x070000,     /* Sphere NIST format. */
//          SF_FORMAT_VOC          = 0x080000,     /* VOC files. */
//          SF_FORMAT_IRCAM        = 0x0A0000,     /* Berkeley/IRCAM/CARL */
//          SF_FORMAT_W64          = 0x0B0000,     /* Sonic Foundry's 64 bit RIFF/WAV */
//          SF_FORMAT_MAT4         = 0x0C0000,     /* Matlab (tm) V4.2 / GNU Octave 2.0 */
//          SF_FORMAT_MAT5         = 0x0D0000,     /* Matlab (tm) V5.0 / GNU Octave 2.1 */
//          SF_FORMAT_PVF          = 0x0E0000,     /* Portable Voice Format */
//          SF_FORMAT_XI           = 0x0F0000,     /* Fasttracker 2 Extended Instrument */
//          SF_FORMAT_HTK          = 0x100000,     /* HMM Tool Kit format */
//          SF_FORMAT_SDS          = 0x110000,     /* Midi Sample Dump Standard */
//          SF_FORMAT_AVR          = 0x120000,     /* Audio Visual Research */
//          SF_FORMAT_WAVEX        = 0x130000,     /* MS WAVE with WAVEFORMATEX */
//          SF_FORMAT_SD2          = 0x160000,     /* Sound Designer 2 */
//          SF_FORMAT_FLAC         = 0x170000,     /* FLAC lossless file format */
//          SF_FORMAT_CAF          = 0x180000,     /* Core Audio File format */
//	    SF_FORMAT_OGG
///bits
//          SF_FORMAT_PCM_S8       = 0x0001,       /* Signed 8 bit data */
//          SF_FORMAT_PCM_16       = 0x0002,       /* Signed 16 bit data */
//          SF_FORMAT_PCM_24       = 0x0003,       /* Signed 24 bit data */
//          SF_FORMAT_PCM_32       = 0x0004,       /* Signed 32 bit data */
///used for ogg
//          SF_FORMAT_VORBIS

	if ( !sf_format_check( &soundInfo ) ) {
		_ERRORLOG( "Error in soundInfo" );
		return 0;
	}


	SNDFILE* m_file = sf_open( pDriver->m_sFilename.toLocal8Bit(), SFM_WRITE, &soundInfo );

	float *pData = new float[ pDriver->m_nBufferSize * 2 ];	// always stereo

	float *pData_L = pDriver->m_pOut_L;
	float *pData_R = pDriver->m_pOut_R;


	//calculate exact song length in frames (no loops).
	std::vector<PatternList*> *pPatternColumns = Hydrogen::get_instance()->getSong()->get_pattern_group_vector();
	int nColumns = pPatternColumns->size();

	int nPatternSize;
	int nSongSize = 0;
	for ( int i = 0; i < nColumns; ++i ) {
		PatternList *pColumn = ( *pPatternColumns )[ i ];
		if ( pColumn->get_size() != 0 ) {
			nPatternSize = pColumn->get( 0 )->get_length();
		} else {
			nPatternSize = MAX_NOTES;
		}
		nSongSize += nPatternSize;
	}

	float ticksize = pDriver->m_nSampleRate * 60.0 /  Hydrogen::get_instance()->getSong()->__bpm / 192 *4;
	 //here we have the song length in frames dependent from bpm and samplerate
	unsigned songLengthInFrames = ticksize * nSongSize;

	unsigned frameNumber = 0;
	int lastRun = 0;
	while ( frameNumber < songLengthInFrames ) {

		int usedBuffer = pDriver->m_nBufferSize;

		//this will calculate the the size from -last- (end of song) used frame buffer,
		//which is mostly smaller than pDriver->m_nBufferSize
		if( songLengthInFrames - frameNumber <  pDriver->m_nBufferSize ){
			lastRun = songLengthInFrames - frameNumber;
			usedBuffer = lastRun;
		};

		frameNumber += usedBuffer;
		int ret = pDriver->m_processCallback( usedBuffer, NULL );

		for ( unsigned i = 0; i < usedBuffer; i++ ) {
			if(pData_L[i] > 1){
				pData[i * 2] = 1;
			}
			else if(pData_L[i] < -1){
				pData[i * 2] = -1;
			}else
			{ 
				pData[i * 2] = pData_L[i];
			}

			if(pData_R[i] > 1){
				pData[i * 2 + 1] = 1;
			}
			else if(pData_R[i] < -1){
				pData[i * 2 + 1] = -1;
			}else
			{ 
				pData[i * 2 + 1] = pData_R[i];
			}
		}
		int res = sf_writef_float( m_file, pData, usedBuffer );
		if ( res != ( int )usedBuffer ) {
			_ERRORLOG( "Error during sf_write_float" );
		}

		float fPercent = ( float ) frameNumber / ( float )songLengthInFrames * 100.0;
		EventQueue::get_instance()->push_event( EVENT_PROGRESS, ( int )fPercent );
//		frameNuber += lastrun;
	}

	delete[] pData;
	pData = NULL;

	sf_close( m_file );

	_INFOLOG( "DiskWriterDriver thread end" );

	pthread_exit( NULL );

//	Hydrogen::get_instance()->stopExportSong();
	return NULL;
}




DiskWriterDriver::DiskWriterDriver( audioProcessCallback processCallback, unsigned nSamplerate, const QString& sFilename, int nSampleDepth )
		: AudioOutput( "DiskWriterDriver" )
		, m_nSampleRate( nSamplerate )
		, m_sFilename( sFilename )
		, m_nSampleDepth ( nSampleDepth )
		, m_processCallback( processCallback )
{
	INFOLOG( "INIT" );
}



DiskWriterDriver::~DiskWriterDriver()
{
	INFOLOG( "DESTROY" );
}



int DiskWriterDriver::init( unsigned nBufferSize )
{
	INFOLOG( QString( "Init, %1 samples" ).arg( nBufferSize ) );

	m_nBufferSize = nBufferSize;
	m_pOut_L = new float[nBufferSize];
	m_pOut_R = new float[nBufferSize];

	return 0;
}



///
/// Connect
/// return 0: Ok
///
int DiskWriterDriver::connect()
{
	INFOLOG( "[connect]" );

	pthread_attr_t attr;
	pthread_attr_init( &attr );

	pthread_create( &diskWriterDriverThread, &attr, diskWriterDriver_thread, this );

	return 0;
}



/// disconnect
void DiskWriterDriver::disconnect()
{
	INFOLOG( "[disconnect]" );

	delete[] m_pOut_L;
	m_pOut_L = NULL;

	delete[] m_pOut_R;
	m_pOut_R = NULL;
}



unsigned DiskWriterDriver::getSampleRate()
{
	return m_nSampleRate;
}



void DiskWriterDriver::play()
{
	m_transport.m_status = TransportInfo::ROLLING;
}



void DiskWriterDriver::stop()
{
	m_transport.m_status = TransportInfo::STOPPED;
}



void DiskWriterDriver::locate( unsigned long nFrame )
{
	INFOLOG( QString( "Locate: %1" ).arg( nFrame ) );
	m_transport.m_nFrames = nFrame;
}



void DiskWriterDriver::updateTransportInfo()
{
//	errorLog( "[updateTransportInfo] not implemented yet" );
}



void DiskWriterDriver::setBpm( float fBPM )
{
	INFOLOG( QString( "SetBpm: %1" ).arg( fBPM ) );
	m_transport.m_nBPM = fBPM;
}


};
