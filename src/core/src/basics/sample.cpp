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

#include <hydrogen/basics/sample.h>

#include <limits>
#include <algorithm>
#include <vector>
#include <cmath>
#include <sndfile.h>

#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/helpers/filesystem.h>

namespace H2Core {

const char* Sample::__class_name = "Sample";
const char* Sample::__loop_modes[] = { "forward", "reverse", "pingpong" };

Sample::Sample( const QString& filename,  int frames, int sample_rate, float* data_l, float* data_r ) : Object( __class_name ),
    __filename( filename ),
    __frames( frames ),
    __sample_rate( sample_rate ),
    __data_l( data_l ),
    __data_r( data_r ),
    __is_modified( false )
{
    //assert( __filename.lastIndexOf( "/" )<0 );
}

Sample::Sample( Sample* other ): Object( __class_name ),
    __filename( other->get_filename() ),
    __frames( other->get_frames() ),
    __sample_rate( other->get_sample_rate() ),
    __data_l( 0 ),
    __data_r( 0 ),
    __is_modified( other->get_is_modified() ),
    __velo_pan( other->__velo_pan ),
    __loops( other->__loops ),
    __rubberband( other->__rubberband )
{
    __data_l = new float[__frames];
    __data_r = new float[__frames];
    memcpy( __data_l, other->get_data_l(), __frames );
    memcpy( __data_r, other->get_data_r(), __frames );
}

Sample::~Sample() {
    if( __data_l!=0 ) delete[] __data_l;
    if( __data_r!=0 ) delete[] __data_r;
}

Sample* Sample::load( const QString& filepath ) {
    if( !Filesystem::file_readable( filepath ) ) {
        ERRORLOG( QString( "Unable to read %1" ).arg( filepath ) );
        return 0;
    }
    return libsndfile_load( filepath );
}

/*
Sample* Sample::load ( const QString& filepath, const Loops& loop_options, const Rubberband& rubber_options ) {
    Sample* sample = Sample::load( filepath );
    if ( sample==0 ) return 0;
    if( !sample->apply_loops( loop_options ) ) {
        // TODO
    }
    return sample;
}
*/

Sample* Sample::libsndfile_load( const QString& filepath ) {
    SF_INFO sound_info;
    SNDFILE* file = sf_open( filepath.toLocal8Bit(), SFM_READ, &sound_info );
    if ( !file ) {
        ERRORLOG( QString( "[Sample::load] Error loading file %1" ).arg( filepath ) );
        return 0;
    }
    if ( sound_info.channels>2 ) {
        WARNINGLOG( QString( "can't handle %1 channels, only 2 will be used" ).arg( sound_info.channels ) );
        sound_info.channels = 2;
    }
    if ( sound_info.frames > ( std::numeric_limits<int>::max()/sound_info.channels ) ) {
        WARNINGLOG( QString( "sample frames count (%1) and channels (%2) are too much, truncate it." ).arg( sound_info.frames ).arg( sound_info.channels ) );
        sound_info.frames = ( std::numeric_limits<int>::max()/sound_info.channels );
    }

    float* buffer = new float[ sound_info.frames * sound_info.channels ];
    memset( buffer, 0, sound_info.frames *sound_info.channels );
    sf_count_t count = sf_read_float( file, buffer, sound_info.frames * sound_info.channels );
    sf_close( file );
    if( count==0 ) WARNINGLOG( QString( "%1 is an empty sample" ).arg( filepath ) );

    float* data_l = new float[ sound_info.frames ];
    float* data_r = new float[ sound_info.frames ];

    if ( sound_info.channels == 1 ) {
        memcpy( data_l,buffer,sound_info.frames*sizeof( float ) );
        memcpy( data_r,buffer,sound_info.frames*sizeof( float ) );
    } else if ( sound_info.channels == 2 ) {
        for ( int i = 0; i < sound_info.frames; i++ ) {
            data_l[i] = buffer[i * 2];
            data_r[i] = buffer[i * 2 + 1];
        }
    }
    delete[] buffer;

    //int idx = filepath.lastIndexOf( "/" );
    //QString filename( ( idx>=0 ) ? filepath.right( filepath.size()-1-filepath.lastIndexOf( "/" ) ) : filepath );
    Sample* sample = new Sample( filepath, sound_info.frames, sound_info.samplerate, data_l, data_r );
    return sample;
}

Sample* Sample::load_edit_sndfile( const QString& filepath, const Loops& lo, const Rubberband& ro )
{
	//set the path to rubberband-cli
	QString program = Preferences::get_instance()->m_rubberBandCLIexecutable;
	//test the path. if test fails return NULL
	if ( QFile( program ).exists() == false && ro.use) {
		_ERRORLOG( QString( "Rubberband executable: File %1 not found" ).arg( program ) );
		return NULL;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();

	// file exists?
	if ( QFile( filepath ).exists() == false ) {
		_ERRORLOG( QString( "[Sample::load] Load sample: File %1 not found" ).arg( filepath ) );
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( filepath.toLocal8Bit(), SFM_READ, &soundInfo );
	if ( !file ) {
                _INFOLOG( QString( "[Sample::load] File not: %1" ).arg( filepath ) );
	}

	unsigned onesamplelength =  lo.end_frame - lo.start_frame;
    unsigned looplength =  lo.end_frame - lo.loop_frame;
	unsigned repeatslength = looplength * lo.count;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * lo.count ;
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

        long int z = lo.loop_frame;
	long int y = lo.start_frame;

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

		
	if ( lo.mode == Loops::REVERSE ){
		reverse(looptempdata_l, looptempdata_l + looplength);
		reverse(looptempdata_r, looptempdata_r + looplength);
	}

        if ( lo.mode == Loops::REVERSE && lo.count > 0 && lo.start_frame == lo.loop_frame ){
		reverse( tempdata_l, tempdata_l + onesamplelength );
		reverse( tempdata_r, tempdata_r + onesamplelength );		
		}

        if ( lo.mode == Loops::PINGPONG &&  lo.start_frame == lo.loop_frame){
		reverse(looptempdata_l, looptempdata_l + looplength);
		reverse(looptempdata_r, looptempdata_r + looplength);
	}
	
	for ( int i = 0; i< lo.count ;i++){
			
		unsigned tempdataend = onesamplelength + ( looplength * i );
                if ( lo.start_frame == lo.loop_frame ){
			copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
			copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );
		}
		if ( lo.mode == Loops::PINGPONG && lo.count > 1){
			reverse(looptempdata_l, looptempdata_l + looplength);
			reverse(looptempdata_r, looptempdata_r + looplength);
		}
                if ( lo.start_frame != lo.loop_frame ){
			copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
			copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );
		}

	}
	
	if ( lo.count == 0 && lo.mode == Loops::REVERSE ){
                reverse( tempdata_l + lo.loop_frame, tempdata_l + newlength);
                reverse( tempdata_r + lo.loop_frame, tempdata_r + newlength);
		}

	//create new sample
	Sample *pSample = new Sample( filepath, newlength, samplerate );
	

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
	if( ro.use ){

		unsigned rubberoutframes = 0;
		double ratio = 1.0;
		double durationtime = 60.0 / pEngine->getNewBpmJTM() * ro.divider/*beats*/;	
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
		QString outfilePath = QDir::tempPath() + "/tmp_rb_outfile.wav";
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

		QString rCs = QString(" %1").arg(ro.c_settings);
		float pitch = pow( 1.0594630943593, ( double)ro.pitch );
		QString rPs = QString(" %1").arg(pitch);

		QString rubberResultPath = QDir::tempPath() + "/tmp_rb_result_file.wav";
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

		pSample->set_frames( soundInfoRI.frames );
		pSample->__data_l = dataRI_l;
		pSample->__data_r = dataRI_r;
	
		pSample->__sample_rate = soundInfoRI.samplerate;	
		pSample->set_is_modified( true );
		pSample->__loops = lo;
		pSample->__rubberband = ro;

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
		pSample->set_is_modified( true );
		pSample->__loops = lo;
	
	}
		return pSample;
}

Sample::Loops::LoopMode Sample::parse_loop_mode( const QString& string ) {
    char* mode = string.toLocal8Bit().data();
    for( int i=Loops::FORWARD; i<Loops::PINGPONG; i++ ) {
        if( 0 == strncasecmp( mode, __loop_modes[i], sizeof( __loop_modes[i] ) ) ) return ( Loops::LoopMode )i;
    }
    return Loops::FORWARD;
}

};

