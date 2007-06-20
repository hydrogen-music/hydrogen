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

Sample::Sample(unsigned nFrames, const string& sFilename, float* pData_L, float* pData_R )
 : Object( "Sample" )
 , m_nFrames( nFrames )
 , m_sFilename( sFilename )
 , m_nSampleRate( 44100 )
 , m_pData_L( pData_L )
 , m_pData_R( pData_R )
{
//	infoLog("INIT " + m_sFilename + ". nFrames: " + toString( nFrames ) );
}



Sample::~Sample()
{
	delete[] m_pData_L;
	delete[] m_pData_R;
//	infoLog( "DESTROY " + m_sFilename);
}




Sample* Sample::load(const string& sFilename)
{
	// is a flac file?
	string ext = sFilename.substr( sFilename.length() - 4, sFilename.length() );
	if ( ( ext == "flac" ) || ( ext == "FLAC" ) ) {
		return loadFLAC( sFilename );
	}
	else {
		return loadWave( sFilename );
	}
}



/// load a FLAC file
Sample* Sample::loadFLAC( const string& sFilename )
{
#ifdef FLAC_SUPPORT
	FLACFile file;
	return file.load( sFilename );
#else
	std::cerr << "[loadFLAC] FLAC support was disabled during compilation" << std::endl;
	return NULL;
#endif
}



Sample* Sample::loadWave( const string& sFilename )
{
	// file exists?
	std::ifstream verify( sFilename.c_str() , std::ios::in | std::ios::binary);
	if (verify == NULL){
		cerr << "[Sample::load] Load sample: File " + sFilename + " not found." << endl;
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( sFilename.c_str(), SFM_READ, &soundInfo );
	if (!file) {
		cerr << "[Sample::load] Error loading file " << sFilename << endl;
	}

//	cout << "frames = " << soundInfo.frames << endl;
//	cout << "samplerate = " << soundInfo.samplerate << endl;
//	cout << "channels = " << soundInfo.channels << endl;

	float *pTmpBuffer = new float[ soundInfo.frames * soundInfo.channels ];

	//int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_close( file );

	float *pData_L = new float[ soundInfo.frames ];
	float *pData_R = new float[ soundInfo.frames ];

	if ( soundInfo.channels == 1) {	// MONO sample
		for (long int i = 0; i < soundInfo.frames; i++) {
			pData_L[i] = pTmpBuffer[i];
			pData_R[i] = pTmpBuffer[i];
		}
	}
	else if (soundInfo.channels == 2) { // STEREO sample
		for (long int i = 0; i < soundInfo.frames; i++) {
			pData_L[i] = pTmpBuffer[i * 2];
			pData_R[i] = pTmpBuffer[i * 2 + 1];
		}
	}
	delete[] pTmpBuffer;


	Sample *pSample = new Sample( soundInfo.frames, sFilename );
	pSample->m_pData_L = pData_L;
	pSample->m_pData_R = pData_R;
	pSample->m_nSampleRate = soundInfo.samplerate;
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

