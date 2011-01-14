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

#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/helpers/filesystem.h>

namespace H2Core {

const char* Sample::__class_name = "Sample";
const char* Sample::__loop_modes[] = { "forward", "reverse", "pingpong" };

Sample::Sample( const QString& filepath,  int frames, int sample_rate, float* data_l, float* data_r ) : Object( __class_name ),
    __filepath( filepath ),
    __frames( frames ),
    __sample_rate( sample_rate ),
    __data_l( data_l ),
    __data_r( data_r ),
    __is_modified( false ) {
    assert( filepath.lastIndexOf( "/" ) >0 );
}

Sample::Sample( Sample* other ): Object( __class_name ),
    __filepath( other->get_filepath() ),
    __frames( other->get_frames() ),
    __sample_rate( other->get_sample_rate() ),
    __data_l( 0 ),
    __data_r( 0 ),
    __is_modified( other->get_is_modified() ),
    __loops( other->__loops ),
    __rubberband( other->__rubberband ) {
    __data_l = new float[__frames];
    __data_r = new float[__frames];
    memcpy( __data_l, other->get_data_l(), __frames );
    memcpy( __data_r, other->get_data_r(), __frames );
    EnvelopePoint pt;
    PanEnvelope* pan = other->get_pan_envelope();
    for( int i=0; i<pan->size(); i++ ) __pan_envelope.push_back( pan->at( i ) );
    PanEnvelope* velocity = other->get_velocity_envelope();
    for( int i=0; i<velocity->size(); i++ ) __velocity_envelope.push_back( velocity->at( i ) );

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
    // TODO should replace the other one when rubberband is done
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
    if ( sound_info.channels > SAMPLE_CHANNELS ) {
        WARNINGLOG( QString( "can't handle %1 channels, only 2 will be used" ).arg( sound_info.channels ) );
        sound_info.channels = SAMPLE_CHANNELS;
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
    } else if ( sound_info.channels == SAMPLE_CHANNELS ) {
        for ( int i = 0; i < sound_info.frames; i++ ) {
            data_l[i] = buffer[i * SAMPLE_CHANNELS];
            data_r[i] = buffer[i * SAMPLE_CHANNELS + 1];
        }
    }
    delete[] buffer;

    Sample* sample = new Sample( filepath, sound_info.frames, sound_info.samplerate, data_l, data_r );
    return sample;
}

bool Sample::apply_loops( const Loops& lo ) {
    if( lo.start_frame<0 ) {
        ERRORLOG( QString( "start_frame %1 < 0 is not allowed" ).arg( lo.start_frame ) );
        return false;
    }
    if( lo.loop_frame<lo.start_frame ) {
        ERRORLOG( QString( "loop_frame %1 < start_frame %2 is not allowed" ).arg( lo.loop_frame ).arg( lo.start_frame ) );
        return false;
    }
    if( lo.end_frame<lo.loop_frame ) {
        ERRORLOG( QString( "end_frame %1 < loop_frame %2 is not allowed" ).arg( lo.end_frame ).arg( lo.loop_frame ) );
        return false;
    }
    if( lo.end_frame>__frames ) {
        ERRORLOG( QString( "end_frame %1 > __frames %2 is not allowed" ).arg( lo.end_frame ).arg( __frames ) );
        return false;
    }
    if( lo.count<0 ) {
        ERRORLOG( QString( "count %1 < 0 is not allowed" ).arg( lo.count ) );
        return false;
    }

    bool full_loop = lo.start_frame==lo.loop_frame;
    int full_length =  lo.end_frame - lo.start_frame;
    int loop_length =  lo.end_frame - lo.loop_frame;
    int new_length = full_length + loop_length * lo.count;

    float* new_data_l = new float[ new_length ];
    float* new_data_r = new float[ new_length ];

    // copy full_length frames to new_data
    if ( lo.mode==Loops::REVERSE && ( lo.count==0 || full_loop ) ) {
        if( full_loop ) {
            // copy end => start
            for( int i=0, j=lo.end_frame; i<full_length; i++, j-- ) new_data_l[i]=__data_l[j];
            for( int i=0, j=lo.end_frame; i<full_length; i++, j-- ) new_data_r[i]=__data_r[j];
        } else {
            // copy start => loop
            int to_loop = lo.loop_frame - lo.start_frame;
            memcpy( new_data_l, __data_l+lo.start_frame, sizeof( float )*to_loop );
            memcpy( new_data_r, __data_r+lo.start_frame, sizeof( float )*to_loop );
            // copy end => loop
            for( int i=to_loop, j=lo.end_frame; i<full_length; i++, j-- ) new_data_l[i]=__data_l[j];
            for( int i=to_loop, j=lo.end_frame; i<full_length; i++, j-- ) new_data_r[i]=__data_r[j];
        }
    } else {
        // copy start => end
        memcpy( new_data_l, __data_l+lo.start_frame, sizeof( float )*full_length );
        memcpy( new_data_r, __data_r+lo.start_frame, sizeof( float )*full_length );
    }
    // copy the loops
    if( lo.count>0 ) {
        int x = full_length;
        bool forward = ( lo.mode==Loops::FORWARD );
        bool ping_pong = ( lo.mode==Loops::PINGPONG );
        for( int i=0; i<lo.count; i++ ) {
            if ( forward ) {
                // copy loop => end
                memcpy( &new_data_l[x], __data_l+lo.loop_frame, sizeof( float )*loop_length );
                memcpy( &new_data_r[x], __data_r+lo.loop_frame, sizeof( float )*loop_length );
            } else {
                // copy end => loop
                for( int i=lo.end_frame, y=x; i>lo.loop_frame; i--, y++ ) new_data_l[y]=__data_l[i];
                for( int i=lo.end_frame, y=x; i>lo.loop_frame; i--, y++ ) new_data_r[y]=__data_r[i];
            }
            x+=loop_length;
            if( ping_pong ) forward=!forward;
        }
        assert( x==new_length );
    }
    __loops = lo;
    delete __data_l;
    delete __data_r;
    __data_l = new_data_l;
    __data_r = new_data_r;
    __frames = new_length;
    return true;
}

void Sample::apply_velocity( const VelocityEnvelope& v ) {
    // TODO frame width (841) and height (91) should go out of here
    // the VelocityEnvelope should be processed within TargetWaveDisplay
    // so that we here have ( int frame_idx, float scale ) points
    // but that will break the xml storage
    __velocity_envelope.clear();
    if ( v.size() > 0 ) {
        float inv_resolution = __frames / 841.0F;
        for ( int i = 1; i < v.size(); i++ ) {
            float y = ( 91 - v[i - 1].value ) / 91.0F;
            float k = ( 91 - v[i].value ) / 91.0F;
            int start_frame = v[i - 1].frame * inv_resolution;
            int end_frame = v[i].frame * inv_resolution;
            if ( i == v.size() -1 ) end_frame = __frames;
            int length = end_frame - start_frame ;
            float step = ( y - k ) / length;;
            for ( int z = start_frame ; z < end_frame; z++ ) {
                __data_l[z] = __data_l[z] * y;
                __data_r[z] = __data_r[z] * y;
                y-=step;
            }
        }
        __velocity_envelope = v;
    }
}

void Sample::apply_pan( const PanEnvelope& p ) {
    // TODO see apply_velocity
    __pan_envelope.clear();
    if ( p.size() > 0 ) {
        float inv_resolution = __frames / 841.0F;
        for ( int i = 1; i < p.size(); i++ ) {
            float y = ( 45 - p[i - 1].value ) / 45.0F;
            float k = ( 45 - p[i].value ) / 45.0F;
            int start_frame = p[i - 1].frame * inv_resolution;
            int end_frame = p[i].frame * inv_resolution;
            if ( i == p.size() -1 ) end_frame = __frames;
            int length = end_frame - start_frame ;
            float step = ( y - k ) / length;;
            for ( int z = start_frame ; z < end_frame; z++ ) {
                // seems wrong to modify only one channel ?!?!
                if( y < 0 ) {
                    float k = 1 + y;
                    __data_l[z] = __data_l[z] * k;
                    __data_r[z] = __data_r[z];
                } else if ( y > 0 ) {
                    float k = 1 - y;
                    __data_l[z] = __data_l[z];
                    __data_r[z] = __data_r[z] * k;
                } else if( y==0 ) {
                    __data_l[z] = __data_l[z];
                    __data_r[z] = __data_r[z];
                }
                y-=step;
            }
        }
        __pan_envelope = p;
    }
}

Sample* Sample::load( const QString& filepath, const Loops& loops, const Rubberband& rubber, const VelocityEnvelope& velocity, const PanEnvelope& pan ) {
    //set the path to rubberband-cli
    QString program = Preferences::get_instance()->m_rubberBandCLIexecutable;
    //test the path. if test fails return NULL
    if ( QFile( program ).exists() == false && rubber.use ) {
        _ERRORLOG( QString( "Rubberband executable: File %1 not found" ).arg( program ) );
        return NULL;
    }


    Sample* sample = load( filepath );
    if( sample==0 ) return 0;

    sample->apply_loops( loops );
    sample->apply_velocity( velocity );
    sample->apply_pan( pan );

    Hydrogen* pEngine = Hydrogen::get_instance();

    float* data_l = sample->get_data_l();
    float* data_r = sample->get_data_r();
///rubberband
    if( rubber.use ) {

        unsigned rubberoutframes = 0;
        double ratio = 1.0;
        double durationtime = 60.0 / pEngine->getNewBpmJTM() * rubber.divider/*beats*/;
        double induration = ( double ) sample->get_frames() / ( double ) sample->get_sample_rate();
        if ( induration != 0.0 ) ratio = durationtime / induration;
        rubberoutframes = int( sample->get_frames() * ratio + 0.1 );
//		_INFOLOG(QString("ratio: %1, rubberoutframes: %2, rubberinframes: %3").arg( ratio ).arg ( rubberoutframes ).arg ( sample->get_frames() ));

        //create new sample

        SF_INFO rubbersoundInfo;
        rubbersoundInfo.samplerate = sample->get_sample_rate();
        rubbersoundInfo.frames = rubberoutframes;
        rubbersoundInfo.channels = SAMPLE_CHANNELS;
        rubbersoundInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

        if ( !sf_format_check( &rubbersoundInfo ) ) {
            _ERRORLOG( "Error in soundInfo" );
            return 0;
        }
        QString outfilePath = QDir::tempPath() + "/tmp_rb_outfile.wav";
        SNDFILE* m_file = sf_open( outfilePath.toLocal8Bit(), SFM_WRITE, &rubbersoundInfo );

        float* infobf = new float[rubbersoundInfo.channels * sample->get_frames()];
        for ( int i = 0; i < sample->get_frames(); ++i ) {
            float value_l = data_l[i];
            float value_r = data_r[i];
            if ( value_l > 1.f ) value_l = 1.f;
            if ( value_l < -1.f ) value_l = -1.f;
            if ( value_r > 1.f ) value_r = 1.f;
            if ( value_r < -1.f ) value_r = -1.f;
            infobf[i* rubbersoundInfo.channels + 0] = value_l;
            infobf[i* rubbersoundInfo.channels + 1] = value_r;
        }

        int res = sf_writef_float( m_file, infobf, sample->get_frames() );
        sf_close( m_file );
        delete[] infobf;


        QObject* parent = 0;

        QProcess* rubberband = new QProcess( parent );

        QStringList arguments;

        QString rCs = QString( " %1" ).arg( rubber.c_settings );
        float pitch = pow( 1.0594630943593, ( double )rubber.pitch );
        QString rPs = QString( " %1" ).arg( pitch );

        QString rubberResultPath = QDir::tempPath() + "/tmp_rb_result_file.wav";
        arguments << "-D" << QString( " %1" ).arg( durationtime ) 	//stretch or squash to make output file X seconds long
                  << "--threads"					//assume multi-CPU even if only one CPU is identified
                  << "-P"						//aim for minimal time distortion
                  << "-f" << rPs					//pitch
                  << "-c" << rCs					//"crispness" levels
                  << outfilePath 					//infile
                  << rubberResultPath;					//outfile

        rubberband->start( program, arguments );

        while( !rubberband->waitForFinished() ) {
            //_ERRORLOG( QString( "prozessing" ));
        }

        //open the new rubberband created file
        // file exists?
        if ( QFile( rubberResultPath ).exists() == false ) {
            _ERRORLOG( QString( "Rubberband reimporter File %1 not found" ).arg( rubberResultPath ) );
            return NULL;
        }

        SF_INFO soundInfoRI;
        SNDFILE* fileRI = sf_open( rubberResultPath.toLocal8Bit(), SFM_READ, &soundInfoRI );
        if ( !fileRI ) {
            _ERRORLOG( QString( "[Sample::load] Error loading file %1" ).arg( rubberResultPath ) );
        }

        float* pTmpBufferRI = new float[ soundInfoRI.frames * soundInfoRI.channels ];

        //int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
        sf_read_float( fileRI, pTmpBufferRI, soundInfoRI.frames * soundInfoRI.channels );
        sf_close( fileRI );

        float* dataRI_l = new float[ soundInfoRI.frames ];
        float* dataRI_r = new float[ soundInfoRI.frames ];


        if ( soundInfoRI.channels == 1 ) {	// MONO sample
            for ( long int i = 0; i < soundInfoRI.frames; i++ ) {
                dataRI_l[i] = pTmpBufferRI[i];
                dataRI_r[i] = pTmpBufferRI[i];
            }
        } else if ( soundInfoRI.channels == SAMPLE_CHANNELS ) { // STEREO sample
            for ( long int i = 0; i < soundInfoRI.frames; i++ ) {
                dataRI_l[i] = pTmpBufferRI[i * SAMPLE_CHANNELS];
                dataRI_r[i] = pTmpBufferRI[i * SAMPLE_CHANNELS + 1];
            }
        }
        delete[] pTmpBufferRI;

        sample->set_frames( soundInfoRI.frames );
        sample->__data_l = dataRI_l;
        sample->__data_r = dataRI_r;

        sample->__sample_rate = soundInfoRI.samplerate;
        sample->set_is_modified( true );
        sample->__loops = loops;
        sample->__rubberband = rubber;

        //delete the tmp files
        if( QFile( outfilePath ).remove() );
//			_INFOLOG("remove outfile");
        if( QFile( rubberResultPath ).remove() );
//			_INFOLOG("remove rubberResultFile");

    }
    sample->set_is_modified( true );
    return sample;
}

Sample::Loops::LoopMode Sample::parse_loop_mode( const QString& string ) {
    char* mode = string.toLocal8Bit().data();
    for( int i=Loops::FORWARD; i<Loops::PINGPONG; i++ ) {
        if( 0 == strncasecmp( mode, __loop_modes[i], sizeof( __loop_modes[i] ) ) ) return ( Loops::LoopMode )i;
    }
    return Loops::FORWARD;
}

bool Sample::write( const QString& path, int format ) {
    float* obuf = new float[ SAMPLE_CHANNELS * __frames ];
    for ( int i = 0; i < __frames; ++i ) {
        float value_l = __data_l[i];
        float value_r = __data_r[i];
        if ( value_l > 1.f ) value_l = 1.f;
        else if ( value_l < -1.f ) value_l = -1.f;
        else if ( value_r > 1.f ) value_r = 1.f;
        else if ( value_r < -1.f ) value_r = -1.f;
        obuf[ i * SAMPLE_CHANNELS + 0 ] = value_l;
        obuf[ i * SAMPLE_CHANNELS + 1 ] = value_r;
    }
    SF_INFO sf_info;
    sf_info.channels = SAMPLE_CHANNELS;
    sf_info.frames = __frames;
    sf_info.samplerate = __sample_rate;
    sf_info.format = format;
    if ( !sf_format_check( &sf_info ) ) {
        ___ERRORLOG( "SF_INFO error" );
        return false;
    }
    SNDFILE* sf_file = sf_open( path.toLocal8Bit().data(), SFM_WRITE, &sf_info ) ;
    if ( sf_file==0 ) {
        ___ERRORLOG( QString("sf_open error : %1").arg( sf_strerror( sf_file ) ) );
        return false;
    }
    sf_count_t res = sf_writef_float( sf_file, obuf, __frames );
    if ( res<=0 ) {
        ___ERRORLOG( QString("sf_writef_float error : %1").arg( sf_strerror( sf_file ) ) );
        return false;
    }
    sf_close( sf_file );
    delete[] obuf;
    return true;
}

};

/* vim: set softtabstop=4 expandtab: */
