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
#include <hydrogen/hydrogen.h>
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
		int fade_out_type,
		SampleVeloPan velopan )

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
		, __velo_pan( velopan )
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
				const QString loopmode,
 				const unsigned fadeoutstartframe,
				const int fadeouttype)
{

	Hydrogen *pEngine = Hydrogen::get_instance();

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

	if ( loopmode == "pingpong" &&  startframe == loppframe){
		reverse(looptempdata_l, looptempdata_l + looplength);
		reverse(looptempdata_r, looptempdata_r + looplength);
	}
	
	for ( int i = 0; i< loops ;i++){
			
		unsigned tempdataend = onesamplelength + ( looplength * i );
		copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
		copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );
		if ( loopmode == "pingpong" && loops > 1){
			reverse(looptempdata_l, looptempdata_l + looplength);
			reverse(looptempdata_r, looptempdata_r + looplength);
		}

	}

	
	if ( loops == 0 && loopmode == "reverse" ){
		reverse( tempdata_l + loppframe, tempdata_l + newlength);
		reverse( tempdata_r + loppframe, tempdata_r + newlength);		
		}



	//create new sample
	Sample *pSample = new Sample( newlength, filename );

	//check for volume vector
	if ( (pEngine->m_volumen.size() > 2 )|| ( pEngine->m_volumen.size() == 2 &&  (pEngine->m_volumen[0].m_hyvalue > 0 || pEngine->m_volumen[1].m_hyvalue > 0 ))){

		//1. write velopan into sample
		SampleVeloPan::SampleVeloVector velovec;
		for (int i = 0; i < static_cast<int>(pEngine->m_volumen.size()); i++){		
			velovec.m_SampleVeloframe = pEngine->m_volumen[i].m_hxframe;
			velovec.m_SampleVelovalue = pEngine->m_volumen[i].m_hyvalue;
			pSample->__velo_pan.m_Samplevolumen.push_back( velovec );
		}
		//2. compute volume
		float divider = newlength / 841.0F;
		for (int i = 1; i  < static_cast<int>(pEngine->m_volumen.size()); i++){
			
			double y =  (91 - static_cast<int>(pEngine->m_volumen[i - 1].m_hyvalue))/91.0F;
			double k = (91 - static_cast<int>(pEngine->m_volumen[i].m_hyvalue))/91.0F;

			unsigned deltastartframe = pEngine->m_volumen[i - 1].m_hxframe * divider;
			unsigned deltaendframe = pEngine->m_volumen[i].m_hxframe * divider;

			if ( i == static_cast<int>(pEngine->m_volumen.size()) -1) deltaendframe = newlength;
			unsigned deltaIdiff = deltaendframe - deltastartframe ;
			double subtract = 0.0F;

			if ( y > k ){
				subtract = (y - k) / deltaIdiff;
			}else
			{
				subtract = ( k - y) / deltaIdiff * (-1);
			}
			//_INFOLOG( QString( "start y %1, end y %2 sub: %3, deltadiff %4" ).arg( y ).arg( k ).arg(subtract).arg(deltaIdiff) );
			for ( int z = static_cast<int>(deltastartframe) ; z < static_cast<int>(deltaendframe); z++){			
//				tempdata_l[z] = tempdata_l[z] * y;
//				tempdata_r[z] = tempdata_r[z] * y;
				y = y - subtract;
			}
		}
		
	}

	//check for pan vector
	if ( (pEngine->m_pan.size() > 2 )|| ( pEngine->m_pan.size() == 2 &&  (pEngine->m_pan[0].m_hyvalue != 45 || pEngine->m_pan[1].m_hyvalue != 45 ))){
		//first step write velopan into sample
		SampleVeloPan::SamplePanVector panvec;
		for (int i = 0; i < static_cast<int>(pEngine->m_pan.size()); i++){		
			panvec.m_SamplePanframe = pEngine->m_pan[i].m_hxframe;
			panvec.m_SamplePanvalue = pEngine->m_pan[i].m_hyvalue;
			pSample->__velo_pan.m_SamplePan.push_back( panvec );
		}
		
	}


///fadeout

	if (fadeouttype == 1){
		double y = 1.0F;
		int differ = newlength - fadeoutstartframe;
		if ( differ <= 0 ) differ = 1;
		double subtract = (double)1.0F / differ;
		for ( unsigned i = fadeoutstartframe; i< newlength; i++, y = y - subtract){			
			tempdata_l[i] = tempdata_l[i] * y;
			tempdata_r[i] = tempdata_r[i] * y;
			//_INFOLOG( QString( "y: %1" ).arg( y ) );
		}
	}


	pSample->__data_l = tempdata_l;
	pSample->__data_r = tempdata_r;
	pSample->__sample_rate = soundInfo.samplerate;
	pSample->__sample_is_modified = true;
	pSample->__sample_mode = loopmode;
	pSample->__start_frame = startframe;
	pSample->__loop_frame = loppframe;
	pSample->__end_frame = endframe;
	pSample->__repeats = loops;
	pSample->__fade_out_startframe = fadeoutstartframe;
	pSample->__fade_out_type = fadeouttype;



	return pSample;

}
};

