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

#include "flac_file.h"
#include <hydrogen/Sample.h>

#include <vector>
#include <fstream>

#ifdef FLAC_SUPPORT

//#include "FLAC/file_decoder.h"
#include <FLAC++/all.h>

namespace H2Core {


#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT < 8
#define LEGACY_FLAC
#else
#undef LEGACY_FLAC
#endif




/// Reads a FLAC file...not optimized yet
class FLACFile_real : public FLAC::Decoder::File, public Object
{
	public:
		FLACFile_real();
		~FLACFile_real();

		void load( string filename );
		Sample* getSample();

	protected:
		virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
		virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);
		virtual void error_callback(::FLAC__StreamDecoderErrorStatus status);

	private:
		std::vector<float> m_audioVect_L;
		std::vector<float> m_audioVect_R;
		string m_sFilename;
};



FLACFile_real::FLACFile_real()
 : Object( "FLACFile_real" )
{
//	infoLog( "INIT" );
}



FLACFile_real::~FLACFile_real()
{
//	infoLog( "DESTROY" );
}



::FLAC__StreamDecoderWriteStatus FLACFile_real::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
//	int nSampleRate = get_sample_rate();
	int nChannelCount = get_channels();
	int nBits = get_bits_per_sample();

	if ( (nChannelCount != 1 ) && (nChannelCount != 2) ) {
		ERRORLOG( "wrong number of channels. nChannelCount=" + to_string( nChannelCount) );
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	unsigned nFrames = frame->header.blocksize;

	if (nBits == 16) {
		if (nChannelCount == 1) {	// mono
			const FLAC__int32* data = buffer[0];

			for ( unsigned i = 0; i < nFrames; i++) {
				m_audioVect_L.push_back( data[i] / 32768.0 );
				m_audioVect_R.push_back( data[i] / 32768.0 );
			}
		}
		else {	// stereo
			const FLAC__int32* data_L = buffer[0];
			const FLAC__int32* data_R = buffer[1];

			for ( unsigned i = 0; i < nFrames; i++) {
				m_audioVect_L.push_back( (float)data_L[i] / 32768.0 );
				m_audioVect_R.push_back( (float)data_R[i] / 32768.0 );
			}
		}
	}
	else if (nBits == 24) {
		if (nChannelCount == 1) {	// mono
			const FLAC__int32* data = buffer[0];

			for ( unsigned i = 0; i < nFrames; i++) {
				m_audioVect_L.push_back( (float)data[i] / 8388608.0 );
				m_audioVect_R.push_back( (float)data[i] / 8388608.0 );
			}
		}
		else {	// stereo
			const FLAC__int32* data_L = buffer[0];
			const FLAC__int32* data_R = buffer[1];

			for ( unsigned i = 0; i < nFrames; i++) {
				m_audioVect_L.push_back( (float)data_L[i] / 8388608.0 );
				m_audioVect_R.push_back( (float)data_R[i] / 8388608.0 );
			}
		}
	}
	else {
		ERRORLOG( "[write_callback] FLAC format error. nBits=" + to_string( nBits ) );
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



void FLACFile_real::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	UNUSED( metadata );
}



void FLACFile_real::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	UNUSED( status );
	ERRORLOG( "[error_callback]" );
}



void FLACFile_real::load( string sFilename )
{
	m_sFilename = sFilename;

	// file exists?
	std::ifstream input(sFilename.c_str() , std::ios::in | std::ios::binary);
	if (!input){
		ERRORLOG( "file " + sFilename + " not found." );
		return;
	}
	else {
		/// \todo: devo chiudere il file?
	}

	set_metadata_ignore_all();

#ifdef LEGACY_FLAC
	set_filename( sFilename.c_str() );

	State s=init();
	if( s != FLAC__FILE_DECODER_OK ) {
#else
	FLAC__StreamDecoderInitStatus s=init(sFilename.c_str() );
	if(s!=FLAC__STREAM_DECODER_INIT_STATUS_OK) {
#endif
		ERRORLOG( "Error in init()" );
	}

#ifdef LEGACY_FLAC
	if ( process_until_end_of_file() == false ) {
		ERRORLOG( "Error in process_until_end_of_file(). Filename: " + m_sFilename );
	}
#else
	if ( process_until_end_of_stream() == false ) {
		ERRORLOG( "[load] Error in process_until_end_of_stream()" );
	}
#endif
}



Sample* FLACFile_real::getSample()
{
	//infoLog( "[getSample]" );
	Sample *pSample = NULL;

	if ( m_audioVect_L.size() == 0 ) {
		// there were errors loading the file
		return NULL;
	}


	int nFrames = m_audioVect_L.size();
	float *data_L = new float[nFrames];
	float *data_R = new float[nFrames];

	memcpy( data_L, &m_audioVect_L[ 0 ], nFrames * sizeof(float) );
	memcpy( data_R, &m_audioVect_R[ 0 ], nFrames * sizeof(float) );
	pSample = new Sample(nFrames, m_sFilename, data_L, data_R );

	return pSample;
}

// :::::::::::::::::::::::::::::




FLACFile::FLACFile()
 : Object( "FLACFile" )
{
	//infoLog( "INIT" );
}


FLACFile::~FLACFile()
{
	//infoLog( "DESTROY" );
}



Sample* FLACFile::load( const std::string& sFilename )
{
	//infoLog( "[load] " + sFilename );

	FLACFile_real *pFile = new FLACFile_real();
	pFile->load( sFilename );
	Sample *pSample = pFile->getSample();
	delete pFile;

	return pSample;
}

};

#endif // FLAC_SUPPORT

