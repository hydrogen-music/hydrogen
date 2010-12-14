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

#include <sndfile.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include "gui/src/HydrogenApp.h"

using namespace std;

namespace H2Core
{

Sample::Sample( unsigned frames,
		const QString& filename, 
		unsigned sample_rate,
		float* data_l,
		float* data_r,
		bool sample_is_modified,
		const QString& sample_mode,
		unsigned start_frame,
		unsigned loop_frame,
		int repeats,
		unsigned end_frame,
		SampleVeloPan velopan,
		bool use_rubber_band,
		float rubberband_divider,
		int rubberband_C_settings,
		float rubberband_pitch)


		: Object( "Sample" )
		, __data_l( data_l )
		, __data_r( data_r )
		, __sample_rate( sample_rate )
		, __filename( filename )
		, __n_frames( frames )
		, __sample_is_modified( sample_is_modified )
		, __sample_mode( sample_mode )
		, __start_frame( start_frame )
		, __loop_frame( loop_frame )
		, __repeats( repeats )
		, __end_frame( end_frame )
		, __velo_pan( velopan )
		, __use_rubber( use_rubber_band )
		, __rubber_divider( rubberband_divider )
		, __rubber_C_settings( rubberband_C_settings )
		, __rubber_pitch( rubberband_pitch )
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

        return load_sndfile( filename );

}


Sample* Sample::load_sndfile( const QString& filename )
{
	// file exists?
	if ( QFile( filename ).exists() == false ) {
		_ERRORLOG( QString( "[Sample::load] Load sample: File %1 not found" ).arg( filename ) );
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( filename.toLocal8Bit(), SFM_READ, &soundInfo );
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

	Sample *pSample = new Sample( soundInfo.frames, filename, soundInfo.samplerate, data_l, data_r );
//	pSample->reverse_sample( pSample ); // test reverse
	return pSample;
}


Sample* Sample::load_edit_sndfile( const QString& filename,
                                   const unsigned startframe,
                                   const unsigned loopframe,
                                   const unsigned endframe,
                                   const int loops,
                                   const QString loopmode,
                                   bool use_rubberband,
                                   float rubber_divider,
                                   int rubberbandCsettings,
                                   float rubber_pitch)
{
	//set the path to rubberband-cli
	QString program = Preferences::get_instance()->m_rubberBandCLIexecutable;
	//test the path. if test fails return NULL
	if ( QFile( program ).exists() == false && use_rubberband) {
		_ERRORLOG( QString( "Rubberband executable: File %1 not found" ).arg( program ) );
		return NULL;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();

	// file exists?
	if ( QFile( filename ).exists() == false ) {
		_ERRORLOG( QString( "[Sample::load] Load sample: File %1 not found" ).arg( filename ) );
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( filename.toLocal8Bit(), SFM_READ, &soundInfo );
	if ( !file ) {
                _INFOLOG( QString( "[Sample::load] File not: %1" ).arg( filename ) );
	}

	unsigned onesamplelength =  endframe - startframe;
        unsigned looplength =  endframe - loopframe;
	unsigned repeatslength = looplength * loops;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * loops ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}

	float *origdata_l;
	float *origdata_r;

        unsigned samplerate = 0;
        float *pTmpBuffer = new float[ soundInfo.frames * soundInfo.channels ];
	
        //int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
        sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
        sf_close( file );
        samplerate = soundInfo.samplerate;
        origdata_l = new float[ soundInfo.frames ];
        origdata_r = new float[ soundInfo.frames ];
	
	
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

        long int z = loopframe;
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

        if ( loopmode == "reverse" && loops > 0 && startframe == loopframe ){
		reverse( tempdata_l, tempdata_l + onesamplelength );
		reverse( tempdata_r, tempdata_r + onesamplelength );		
		}

        if ( loopmode == "pingpong" &&  startframe == loopframe){
		reverse(looptempdata_l, looptempdata_l + looplength);
		reverse(looptempdata_r, looptempdata_r + looplength);
	}
	
	for ( int i = 0; i< loops ;i++){
			
		unsigned tempdataend = onesamplelength + ( looplength * i );
                if ( startframe == loopframe ){
			copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
			copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );
		}
		if ( loopmode == "pingpong" && loops > 1){
			reverse(looptempdata_l, looptempdata_l + looplength);
			reverse(looptempdata_r, looptempdata_r + looplength);
		}
                if ( startframe != loopframe ){
			copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
			copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );
		}

	}
	
	if ( loops == 0 && loopmode == "reverse" ){
                reverse( tempdata_l + loopframe, tempdata_l + newlength);
                reverse( tempdata_r + loopframe, tempdata_r + newlength);
		}

	//create new sample
	Sample *pSample = new Sample( newlength, filename, samplerate );
	

	//check for volume vector
	if ( (pEngine->m_volumen.size() > 2 )|| ( pEngine->m_volumen.size() == 2 &&  (pEngine->m_volumen[0].m_hyvalue > 0 || pEngine->m_volumen[1].m_hyvalue > 0 ))){

		//1. write velopan into sample
		SampleVeloPan::SampleVeloVector velovec;
		pSample->__velo_pan.m_Samplevolumen.clear();
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

			for ( int z = static_cast<int>(deltastartframe) ; z < static_cast<int>(deltaendframe); z++){			
				tempdata_l[z] = tempdata_l[z] * y;
				tempdata_r[z] = tempdata_r[z] * y;
				y = y - subtract;
			}
		}
		
	}

	//check for pan vector
	if ( (pEngine->m_pan.size() > 2 )|| ( pEngine->m_pan.size() == 2 &&  (pEngine->m_pan[0].m_hyvalue != 45 || pEngine->m_pan[1].m_hyvalue != 45 ))){
		//first step write velopan into sample
		SampleVeloPan::SamplePanVector panvec;
		pSample->__velo_pan.m_SamplePan.clear();
		for (int i = 0; i < static_cast<int>(pEngine->m_pan.size()); i++){		
			panvec.m_SamplePanframe = pEngine->m_pan[i].m_hxframe;
			panvec.m_SamplePanvalue = pEngine->m_pan[i].m_hyvalue;
			pSample->__velo_pan.m_SamplePan.push_back( panvec );
		}

		float divider = newlength / 841.0F;
		for (int i = 1; i  < static_cast<int>(pEngine->m_pan.size()); i++){
			
			double y =  (45 - static_cast<int>(pEngine->m_pan[i - 1].m_hyvalue))/45.0F;
			double k = (45 - static_cast<int>(pEngine->m_pan[i].m_hyvalue))/45.0F;

			unsigned deltastartframe = pEngine->m_pan[i - 1].m_hxframe * divider;
			unsigned deltaendframe = pEngine->m_pan[i].m_hxframe * divider;

			if ( i == static_cast<int>(pEngine->m_pan.size()) -1) deltaendframe = newlength;
			unsigned deltaIdiff = deltaendframe - deltastartframe ;
			double subtract = 0.0F;

			
			if ( y > k ){
				subtract = (y - k) / deltaIdiff;
			}else
			{
				subtract = ( k - y) / deltaIdiff * (-1);
			}

			for ( int z = static_cast<int>(deltastartframe) ; z < static_cast<int>(deltaendframe); z++){
				if( y < 0 ){
					double k = 1 + y;
					tempdata_l[z] = tempdata_l[z] * k;
					tempdata_r[z] = tempdata_r[z];
				}
				else if(y > 0){
					double k = 1 - y;
					tempdata_l[z] = tempdata_l[z];
					tempdata_r[z] = tempdata_r[z] * k;
				}
				else if(y == 0){
					tempdata_l[z] = tempdata_l[z];
					tempdata_r[z] = tempdata_r[z];
				}
				y = y - subtract;	
			}
		}
		
	}

///rubberband
	if( use_rubberband ){

		unsigned rubberoutframes = 0;
		double ratio = 1.0;
		double durationtime = 60.0 / pEngine->getNewBpmJTM() * rubber_divider/*beats*/;	
		double induration = (double) newlength / (double) samplerate;
		if (induration != 0.0) ratio = durationtime / induration;
		rubberoutframes = int(newlength * ratio + 0.1);
//		_INFOLOG(QString("ratio: %1, rubberoutframes: %2, rubberinframes: %3").arg( ratio ).arg ( rubberoutframes ).arg ( newlength ));
	
		//create new sample

		SF_INFO rubbersoundInfo;
		rubbersoundInfo.samplerate = samplerate;
		rubbersoundInfo.frames = rubberoutframes;
		rubbersoundInfo.channels = 2;
		rubbersoundInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	
		if ( !sf_format_check( &rubbersoundInfo ) ) {
			_ERRORLOG( "Error in soundInfo" );
			return 0;
		}
		QString outfilePath = Preferences::get_instance()->getDataDirectory() + "/tmp_rb_outfile.wav";
		SNDFILE* m_file = sf_open( outfilePath.toLocal8Bit(), SFM_WRITE, &rubbersoundInfo);

		float *infobf = new float[rubbersoundInfo.channels * newlength];
			for (int i = 0; i < newlength; ++i) {
			float value_l = tempdata_l[i];
			float value_r = tempdata_r[i];
			if (value_l > 1.f) value_l = 1.f;
			if (value_l < -1.f) value_l = -1.f;
			if (value_r > 1.f) value_r = 1.f;
			if (value_r < -1.f) value_r = -1.f;
			infobf[i * rubbersoundInfo.channels + 0] = value_l;
			infobf[i * rubbersoundInfo.channels + 1] = value_r;
		}

		int res = sf_writef_float(m_file, infobf, newlength );
		sf_close( m_file );
		delete[] infobf;


		QObject *parent = 0;

		QProcess *rubberband = new QProcess(parent);

		QStringList arguments;

		QString rCs = QString(" %1").arg(rubberbandCsettings);
		float pitch = pow( 1.0594630943593, ( double)rubber_pitch );
		QString rPs = QString(" %1").arg(pitch);

		QString rubberResultPath = Preferences::get_instance()->getDataDirectory() + "/tmp_rb_result_file.wav";
		arguments << "-D" << QString(" %1").arg( durationtime ) 	//stretch or squash to make output file X seconds long
			  << "--threads"					//assume multi-CPU even if only one CPU is identified
			  << "-P"						//aim for minimal time distortion
			  << "-f" << rPs					//pitch
			  << "-c" << rCs					//"crispness" levels
			  << outfilePath 					//infile
			  << rubberResultPath;					//outfile

		rubberband->start(program, arguments);

		while( !rubberband->waitForFinished() ){
			//_ERRORLOG( QString( "prozessing" ));	
		}

		//open the new rubberband created file
		// file exists?
		if ( QFile( rubberResultPath ).exists() == false ) {
			_ERRORLOG( QString( "Rubberband reimporter File %1 not found" ).arg( rubberResultPath ) );
			return NULL;
		}
		
		SF_INFO soundInfoRI;
		SNDFILE* fileRI = sf_open( rubberResultPath.toLocal8Bit(), SFM_READ, &soundInfoRI);
		if ( !fileRI ) {
			_ERRORLOG( QString( "[Sample::load] Error loading file %1" ).arg( rubberResultPath ) );
		}
		
		float *pTmpBufferRI = new float[ soundInfoRI.frames * soundInfoRI.channels ];
	
		//int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
		sf_read_float( fileRI, pTmpBufferRI, soundInfoRI.frames * soundInfoRI.channels );
		sf_close( fileRI );
	
		float *dataRI_l = new float[ soundInfoRI.frames ];
		float *dataRI_r = new float[ soundInfoRI.frames ];
	
	
		if ( soundInfoRI.channels == 1 ) {	// MONO sample
			for ( long int i = 0; i < soundInfo.frames; i++ ) {
				dataRI_l[i] = pTmpBufferRI[i];
				dataRI_r[i] = pTmpBufferRI[i];
			}
		} else if ( soundInfoRI.channels == 2 ) { // STEREO sample
			for ( long int i = 0; i < soundInfoRI.frames; i++ ) {
				dataRI_l[i] = pTmpBufferRI[i * 2];
				dataRI_r[i] = pTmpBufferRI[i * 2 + 1];
			}
		}
		delete[] pTmpBufferRI;

		pSample->set_new_sample_length_frames( soundInfoRI.frames );
		pSample->__data_l = dataRI_l;
		pSample->__data_r = dataRI_r;
	
		pSample->__sample_rate = soundInfoRI.samplerate;	
		pSample->__sample_is_modified = true;
		pSample->__sample_mode = loopmode;
		pSample->__start_frame = startframe;
                pSample->__loop_frame = loopframe;
		pSample->__end_frame = endframe;
		pSample->__repeats = loops;
		pSample->__use_rubber = true;
		pSample->__rubber_divider = rubber_divider;
		pSample->__rubber_C_settings = rubberbandCsettings;
		pSample->__rubber_pitch = rubber_pitch;

		//delete the tmp files
		if( QFile( outfilePath ).remove() ); 
//			_INFOLOG("remove outfile");
		if( QFile( rubberResultPath ).remove() );
//			_INFOLOG("remove rubberResultFile");

	}else///~rubberband
	{
		pSample->__data_l = tempdata_l;
		pSample->__data_r = tempdata_r;
	
		pSample->__sample_rate = samplerate;	
		pSample->__sample_is_modified = true;
		pSample->__sample_mode = loopmode;
		pSample->__start_frame = startframe;
                pSample->__loop_frame = loopframe;
		pSample->__end_frame = endframe;
		pSample->__repeats = loops;
	
	}
		return pSample;
}
};

