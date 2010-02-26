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
        //default format = waf 16bit
	int sfformat = 0x010000;
	int bits = 0x0002;

	soundInfo.format =  sfformat|bits;

	if ( !sf_format_check( &soundInfo ) ) {
		_ERRORLOG( "Error in soundInfo" );
		return 0;
	}


	SNDFILE* m_file = sf_open( pDriver->m_sFilename.toLocal8Bit(), SFM_WRITE, &soundInfo );

	float *pData = new float[ pDriver->m_nBufferSize * 2 ];	// always stereo

	float *pData_L = pDriver->m_pOut_L;
	float *pData_R = pDriver->m_pOut_R;


	//calculate exaxt song frames.
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
	unsigned songLengthinFrames = ticksize * nSongSize; 

	unsigned frameNumber = 0;
	int lastRun = 0;
	while ( frameNumber < songLengthinFrames ) {

		int usedBuffer = pDriver->m_nBufferSize;

		if( songLengthinFrames - frameNumber <  pDriver->m_nBufferSize ){
			lastRun = songLengthinFrames - frameNumber;
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

		float fPercent = ( float ) frameNumber / ( float )songLengthinFrames * 100.0;
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
	// not used
}



void DiskWriterDriver::setBpm( float fBPM )
{
	INFOLOG( QString( "SetBpm: %1" ).arg( fBPM ) );
	m_transport.m_nBPM = fBPM;
}


};
