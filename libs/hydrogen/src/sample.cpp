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

#include "gui/src/HydrogenApp.h"
#include "gui/src/SampleEditor/SampleEditor.h"

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


Sample* Sample::load_edit_wave( const QString& filename,
				const unsigned startframe,
				const unsigned loppframe,
				const unsigned endframe,
				const int loops,
				const QString loopmode)
{
	_INFOLOG( QString( "mode: " + loopmode) );
	_INFOLOG( QString( "loops: " ).arg( loops ) );
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

	unsigned onesamplelength =  endframe - startframe;
	unsigned looplength =  endframe - loppframe;
	unsigned repeatslength = looplength * loops;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * loops ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}

	float *pTmpBuffer = new float[ soundInfo.frames * soundInfo.channels ];

	//int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_close( file );

	float *origdata_l = new float[ soundInfo.frames ];
	float *origdata_r = new float[ soundInfo.frames ];


	if ( soundInfo.channels == 1 ) {	// MONO sample
		for ( long int i = 0; i < soundInfo.frames; i++ ) {
			origdata_l[i] = pTmpBuffer[i];
			origdata_r[i] = pTmpBuffer[i];
		}
	} else if ( soundInfo.channels == 2 ) { // STEREO sample
		for ( long int i = 0; i < soundInfo.frames; i++ ) {
			origdata_l[i] = pTmpBuffer[i * 2];
			origdata_r[i] = pTmpBuffer[i * 2 + 1];
		}
	}
	delete[] pTmpBuffer;

	float *tempdata_l = new float[ newlength ];
	float *tempdata_r = new float[ newlength ];

	float *looptempdata_l = new float[ looplength ];
	float *looptempdata_r = new float[ looplength ];

	long int z = loppframe;
	long int y = startframe;

	for ( unsigned i = 0; i < newlength; i++){ //first vector

		tempdata_l[i] = 0;
		tempdata_r[i] = 0;
	}

	for ( unsigned i = 0; i < onesamplelength; i++, y++){ //first vector

		tempdata_l[i] = origdata_l[y];
		tempdata_r[i] = origdata_r[y];
	}

	for ( unsigned i = 0; i < looplength; i++, z++){ //loop vector

		looptempdata_l[i] = origdata_l[z];
		looptempdata_r[i] = origdata_r[z];
	}

		
	if ( loopmode == "reverse" ){
		reverse(looptempdata_l, looptempdata_l + looplength);
		reverse(looptempdata_r, looptempdata_r + looplength);
	}

	if ( loopmode == "reverse" && loops > 0 && startframe == loppframe ){
		reverse( tempdata_l, tempdata_l + onesamplelength );
		reverse( tempdata_r, tempdata_r + onesamplelength );		
		}

	
	for ( int i = 0; i< loops ;i++){

		unsigned tempdataend = onesamplelength + ( looplength * i );
		copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
		copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );
		if ( loopmode == "pingpong" ){
			reverse(looptempdata_l, looptempdata_l + looplength);
			reverse(looptempdata_r, looptempdata_r + looplength);
		}

	}

	
	if ( loops == 0 && loopmode == "reverse" ){
		reverse( tempdata_l + loppframe, tempdata_l + newlength);
		reverse( tempdata_r + loppframe, tempdata_r + newlength);		
		}


	Sample *pSample = new Sample( newlength, filename );
	pSample->__data_l = tempdata_l;
	pSample->__data_r = tempdata_r;
	pSample->__sample_rate = soundInfo.samplerate;
//	pSample->reverse_sample( pSample ); // test reverse
	return pSample;


}
};

