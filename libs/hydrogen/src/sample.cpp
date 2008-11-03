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

#include <hydrogen/sample.h>

#include <hydrogen/Preferences.h>
#include "flac_file.h"
#include <sndfile.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>


using namespace std;

namespace H2Core
{

Sample::Sample( unsigned frames,
		const QString& filename, 
		float* data_l,
		float* data_r,
		bool sample_is_modified,
		const QString& sample_mode,
		unsigned start_frame,
		unsigned loop_frame,
		int repeats,
		unsigned end_frame,
		unsigned fade_out_startframe,
		int fade_out_type  )

		: Object( "Sample" )
		, __data_l( data_l )
		, __data_r( data_r )
		, __sample_rate( 44100 )
		, __filename( filename )
		, __n_frames( frames )
		, __sample_is_modified( sample_is_modified )
		, __sample_mode( sample_mode )
		, __start_frame( start_frame )
		, __loop_frame( loop_frame )
		, __repeats( repeats )
		, __end_frame( end_frame )
		, __fade_out_startframe( fade_out_startframe )
		, __fade_out_type( fade_out_type )
{
		//INFOLOG("INIT " + m_sFilename + ". nFrames: " + toString( nFrames ) );
}



Sample::~Sample()
{
	delete[] __data_l;
	delete[] __data_r;
	//INFOLOG( "DESTROY " + m_sFilename);
}




Sample* Sample::load( const QString& filename )
{
	// is it a flac file?
	if ( ( filename.endsWith( "flac") ) || ( filename.endsWith( "FLAC" )) ) {
		return load_flac( filename );
	} else {
		return load_wave( filename );
	}
}



/// load a FLAC file
Sample* Sample::load_flac( const QString& filename )
{
#ifdef FLAC_SUPPORT
	FLACFile file;
	return file.load( filename );
#else
	_ERRORLOG("[loadFLAC] FLAC support was disabled during compilation");
	return NULL;
#endif
}



Sample* Sample::load_wave( const QString& filename )
{
	// file exists?
	if ( QFile( filename ).exists() == false ) {
		_ERRORLOG( QString( "[Sample::load] Load sample: File %1 not found" ).arg( filename ) );
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( filename.toAscii(), SFM_READ, &soundInfo );
	if ( !file ) {
		_ERRORLOG( QString( "[Sample::load] Error loading file %1" ).arg( filename ) );
	}


	float *pTmpBuffer = new float[ soundInfo.frames * soundInfo.channels ];

	//int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_close( file );

	float *data_l = new float[ soundInfo.frames ];
	float *data_r = new float[ soundInfo.frames ];

	if ( soundInfo.channels == 1 ) {	// MONO sample
		for ( long int i = 0; i < soundInfo.frames; i++ ) {
			data_l[i] = pTmpBuffer[i];
			data_r[i] = pTmpBuffer[i];
		}
	} else if ( soundInfo.channels == 2 ) { // STEREO sample
		for ( long int i = 0; i < soundInfo.frames; i++ ) {
			data_l[i] = pTmpBuffer[i * 2];
			data_r[i] = pTmpBuffer[i * 2 + 1];
		}
	}
	delete[] pTmpBuffer;

	Sample *pSample = new Sample( soundInfo.frames, filename );
	pSample->__data_l = data_l;
	pSample->__data_r = data_r;
	pSample->__sample_rate = soundInfo.samplerate;
//	pSample->reverse_sample( pSample ); // test reverse
	return pSample;
}

/// sample_editor functions process
/// also prozess must run if a song will load

//simple reverse example 
void Sample::sampleEditProzess( Sample* Sample )
{

	unsigned onesamplelength =  __end_frame - __start_frame;
	unsigned looplength = __end_frame - __loop_frame ;
	unsigned repeatslength = looplength * __repeats;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * __repeats ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}
/*
	if ( __repeats == 0 )__repeats = 1;
	float *tempdata_l = new float[ newlength ];
	float *tempdata_r = new float[ newlength ];
	float *looptempdata_l = new float[ looplength ];
	float *looptempdata_r = new float[ looplength ];

	long int z = __loop_frame;
	long int y = __start_frame;
	for (int i = 0; i < __end_frame - __start_frame; i++){ //first vector
		
		tempdata_l[i] = Sample->__data_l[y];
		tempdata_r[i] = Sample->__data_r[y];
		y++;
	}

	for (int i = 0; i < __end_frame - __loop_frame; i++){ //loop vector
		
		looptempdata_l[i] = Sample->__data_l[z];
		looptempdata_r[i] = Sample->__data_r[z];
		z++;
	}


	for ( int i = 0; i< __repeats;i++){
		unsigned tempdataend = onesamplelength * (i+1);
		copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
	}
*/
	ERRORLOG( QString("beginlang: %1").arg(onesamplelength) );
	ERRORLOG( QString("looplang: %1").arg(looplength) );	
	ERRORLOG( QString("newlength: %1").arg(newlength) );


	
	


//	Sample->__data_l = tempdata_l;
//	Sample->__data_r = tempdata_r;
//	Sample->__n_frames = newlength;
	
/*
	float *data_l = new float[ Sample->get_n_frames() ];
	float *data_r = new float[ Sample->get_n_frames() ];
	data_l = Sample->__data_l;
	data_r = Sample->__data_r;
	reverse(data_l, data_l + Sample->get_n_frames());
	reverse(data_r, data_r + Sample->get_n_frames());
	Sample->__data_l = data_l;
	Sample->__data_r = data_r;
*/
}


/*
void Sample::save( const string& sFilename )
{
	errorLog( "[save] not implemented yet" );
	infoLog( "saving " + sFilename );

	SF_INFO soundInfo;
	soundInfo.samplerate = m_nSampleRate;
	soundInfo.channels = 2;
	soundInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	SNDFILE* file = sf_open( sFilename.c_str(), SFM_WRITE, &soundInfo );

	float *pData = new float[ getNFrames() * 2 ];	// always stereo

	// prepare the interleaved buffer
	for ( unsigned i = 0; i < getNFrames(); i++ ) {
		pData[ i * 2 ] = m_pData_L[ i ];
		pData[ i * 2 + 1 ] = m_pData_R[ i ];
	}

	sf_writef_float( file, pData, getNFrames() );

	sf_close( file );

	delete[] pData;
}
*/



};

