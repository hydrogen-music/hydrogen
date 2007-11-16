/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/Sample.h>

#include <hydrogen/Preferences.h>
#include "flac_file.h"
#include <sndfile.h>
#include <iostream>
#include <fstream>

using namespace std;

namespace H2Core {

Sample::Sample(unsigned frames, const string& filename, float* data_l, float* data_r )
 : Object( "Sample" )
 , __data_l( data_l )
 , __data_r( data_r )
 , __sample_rate( 44100 )
 , __filename( filename )
 , __n_frames( frames )
{
//	infoLog("INIT " + m_sFilename + ". nFrames: " + toString( nFrames ) );
}



Sample::~Sample()
{
	delete[] __data_l;
	delete[] __data_r;
//	infoLog( "DESTROY " + m_sFilename);
}




Sample* Sample::load(const string& filename)
{
	// is a flac file?
	string ext = filename.substr( filename.length() - 4, filename.length() );
	if ( ( ext == "flac" ) || ( ext == "FLAC" ) ) {
		return load_flac( filename );
	}
	else {
		return load_wave( filename );
	}
}



/// load a FLAC file
Sample* Sample::load_flac( const string& filename )
{
#ifdef FLAC_SUPPORT
	FLACFile file;
	return file.load( filename );
#else
	std::cerr << "[loadFLAC] FLAC support was disabled during compilation" << std::endl;
	return NULL;
#endif
}



Sample* Sample::load_wave( const string& filename )
{
	// file exists?
	std::ifstream verify( filename.c_str() , std::ios::in | std::ios::binary);
	if (verify == NULL){
		cerr << "[Sample::load] Load sample: File " + filename + " not found." << endl;
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( filename.c_str(), SFM_READ, &soundInfo );
	if (!file) {
		cerr << "[Sample::load] Error loading file " << filename << endl;
	}

//	cout << "frames = " << soundInfo.frames << endl;
//	cout << "samplerate = " << soundInfo.samplerate << endl;
//	cout << "channels = " << soundInfo.channels << endl;

	float *pTmpBuffer = new float[ soundInfo.frames * soundInfo.channels ];

	//int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_close( file );

	float *data_l = new float[ soundInfo.frames ];
	float *data_r = new float[ soundInfo.frames ];

	if ( soundInfo.channels == 1) {	// MONO sample
		for (long int i = 0; i < soundInfo.frames; i++) {
			data_l[i] = pTmpBuffer[i];
			data_r[i] = pTmpBuffer[i];
		}
	}
	else if (soundInfo.channels == 2) { // STEREO sample
		for (long int i = 0; i < soundInfo.frames; i++) {
			data_l[i] = pTmpBuffer[i * 2];
			data_r[i] = pTmpBuffer[i * 2 + 1];
		}
	}
	delete[] pTmpBuffer;


	Sample *pSample = new Sample( soundInfo.frames, filename );
	pSample->__data_l = data_l;
	pSample->__data_r = data_r;
	pSample->__sample_rate = soundInfo.samplerate;
	return pSample;
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

