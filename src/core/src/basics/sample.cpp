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



#include <limits>
#include <memory>

#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/basics/sample.h>

#if defined(H2CORE_HAVE_RUBBERBAND) || _DOXYGEN_
#include <rubberband/RubberBandStretcher.h>
#define RUBBERBAND_BUFFER_OVERSIZE  500
#define RUBBERBAND_DEBUG            0
#endif

namespace H2Core
{

const char* Sample::__class_name = "Sample";
const char* EnvelopePoint::__class_name = "EnvolopePoint";

const char* Sample::__loop_modes[] = { "forward", "reverse", "pingpong" };

#if defined(H2CORE_HAVE_RUBBERBAND) || _DOXYGEN_
static double compute_pitch_scale( const Sample::Rubberband& r );
static RubberBand::RubberBandStretcher::Options compute_rubberband_options( const Sample::Rubberband& r );
#endif


/* EnvelopePoint */
EnvelopePoint::EnvelopePoint() : Object( EnvelopePoint::__class_name ), frame( 0 ), value( 0 ) 
{
}

EnvelopePoint::EnvelopePoint( int f, int v ) : Object( EnvelopePoint::__class_name ), frame( f ), value( v ) 
{
}

EnvelopePoint::EnvelopePoint( EnvelopePoint* other ) : Object( EnvelopePoint::__class_name )
{
	frame = other->frame;
	value = other->value;
}
/* EnvelopePoint */


Sample::Sample( const QString& filepath,  int frames, int sample_rate, float* data_l, float* data_r ) : Object( Sample::__class_name ),
	__filepath( filepath ),
	__frames( frames ),
	__sample_rate( sample_rate ),
	__data_l( data_l ),
	__data_r( data_r ),
	__is_modified( false )
{
	assert( filepath.lastIndexOf( "/" ) >0 );
}

Sample::Sample( Sample* pOther ): Object( __class_name ),
	__filepath( pOther->get_filepath() ),
	__frames( pOther->get_frames() ),
	__sample_rate( pOther->get_sample_rate() ),
	__data_l( nullptr ),
	__data_r( nullptr ),
	__is_modified( pOther->get_is_modified() ),
	__loops( pOther->__loops ),
	__rubberband( pOther->__rubberband )
{
	__data_l = new float[__frames];
	__data_r = new float[__frames];
	
	// Since the third argument of memcpy takes the number of bytes,
	// which are about to be copied, and the data is given in float,
	// which are  four bytes each, the number of copied frames
	// `__frames` has to be multiplied by four.
	memcpy( __data_l, pOther->get_data_l(), __frames * 4 );
	memcpy( __data_r, pOther->get_data_r(), __frames * 4 );

	PanEnvelope* pPan = pOther->get_pan_envelope();
	for( int i=0; i<pPan->size(); i++ ) {
		__pan_envelope.push_back( std::make_unique<EnvelopePoint>( pPan->at(i)->value, pPan->at(i)->frame ) );
	}

	PanEnvelope* pVelocity = pOther->get_velocity_envelope();
	for( int i=0; i<pVelocity->size(); i++ ) {
		__velocity_envelope.push_back( std::make_unique<EnvelopePoint>( pVelocity->at(i)->value, pVelocity->at(i)->frame ) );
	}
}

Sample::~Sample()
{
	if( __data_l!=nullptr ) delete[] __data_l;
	if( __data_r!=nullptr ) delete[] __data_r;
}

void Sample::set_filename( const QString& filename )
{
	QFileInfo Filename = QFileInfo( filename );
	QFileInfo Dest = QFileInfo( __filepath );
	__filepath = QDir(Dest.absolutePath()).filePath( Filename.fileName() );
}


Sample* Sample::load( const QString& filepath )
{
	Sample* pSample = nullptr;
	
	if( !Filesystem::file_readable( filepath ) ) {
		ERRORLOG( QString( "Unable to read %1" ).arg( filepath ) );
	} else {
		pSample = new Sample( filepath );
		
		if( !pSample->load() )
		{
			delete pSample;
			pSample = nullptr;
		}
	}
	
	return pSample;
}

Sample* Sample::load( const QString& filepath, const Loops& loops, const Rubberband& rubber, const VelocityEnvelope& velocity, const PanEnvelope& pan )
{
	Sample* pSample = Sample::load( filepath );
	
	if( pSample ){
		pSample->apply( loops, rubber, velocity, pan );
	}

	return pSample;
}

void Sample::apply( const Loops& loops, const Rubberband& rubber, const VelocityEnvelope& velocity, const PanEnvelope& pan )
{
	apply_loops( loops );
	apply_velocity( velocity );
	apply_pan( pan );
#ifdef H2CORE_HAVE_RUBBERBAND
	apply_rubberband( rubber );
#else
	exec_rubberband_cli( rubber );
#endif
}

bool Sample::load()
{
	// Will contain a bunch of metadata about the loaded sample.
	SF_INFO sound_info;
	
	// Opens file in read-only mode.
	SNDFILE* file = sf_open( __filepath.toLocal8Bit(), SFM_READ, &sound_info );
	if ( !file ) {
		ERRORLOG( QString( "[Sample::load] Error loading file %1" ).arg( __filepath ) );
		return false;
	}
	
	// Sanity check. SAMPLE_CHANNELS is defined in
	// core/include/hydrogen/globals.h and set to 2.
	if ( sound_info.channels > SAMPLE_CHANNELS ) {
		WARNINGLOG( QString( "can't handle %1 channels, only 2 will be used" ).arg( sound_info.channels ) );
		sound_info.channels = SAMPLE_CHANNELS;
	}
	if ( sound_info.frames > ( std::numeric_limits<int>::max()/sound_info.channels ) ) {
		WARNINGLOG( QString( "sample frames count (%1) and channels (%2) are too much, truncate it." ).arg( sound_info.frames ).arg( sound_info.channels ) );
		sound_info.frames = ( std::numeric_limits<int>::max()/sound_info.channels );
	}

	// Create an array, which will hold the block of samples read
	// from file.
	float* buffer = new float[ sound_info.frames * sound_info.channels ];
	
	//memset( buffer, 0, sound_info.frames *sound_info.channels );
	
	// Read all frames into `buffer'. Libsndfile does seamlessly
	// convert the format of the underlying data on the fly. The
	// output will be an array of floats regardless of file's
	// encoding (e.g. 16 bit PCM).
	sf_count_t count = sf_read_float( file, buffer, sound_info.frames * sound_info.channels );
	if( count==0 ){
		WARNINGLOG( QString( "%1 is an empty sample" ).arg( __filepath ) );
	}
	
	// Deallocate the handler.
	if ( sf_close( file ) != 0 ){
		WARNINGLOG( QString( "Unable to close sample file %1" ).arg( __filepath ) );
	}
	
	// Flush the current content of the left and right channel and
	// the current metadata.
	unload();
	
	// Save the metadata of the loaded file into private members
	// of the Sample class.
	__frames = sound_info.frames;
	__sample_rate = sound_info.samplerate;

	// Split the loaded frames into left and right channel. 
	// If only one channels was present in the underlying data,
	// duplicate its content.
	__data_l = new float[ sound_info.frames ];
	__data_r = new float[ sound_info.frames ];
	if ( sound_info.channels == 1 ) {
		memcpy( __data_l, buffer, __frames * sizeof( float ) );
		memcpy( __data_r, buffer, __frames * sizeof( float ) );
	} else if ( sound_info.channels == SAMPLE_CHANNELS ) {
		for ( int i = 0; i < __frames; i++ ) {
			__data_l[i] = buffer[i * SAMPLE_CHANNELS];
			__data_r[i] = buffer[i * SAMPLE_CHANNELS + 1];
		}
	}
	delete[] buffer;

	return true;
}

bool Sample::apply_loops( const Loops& lo )
{
	if( __loops == lo ) return true;
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
	//if( lo == __loops ) return true;

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
	delete [] __data_l;
	delete [] __data_r;
	__data_l = new_data_l;
	__data_r = new_data_r;
	__frames = new_length;
	__is_modified = true;
	return true;
}

void Sample::apply_velocity( const VelocityEnvelope& v )
{
	// TODO frame width (841) and height (91) should go out of here
	// the VelocityEnvelope should be processed within TargetWaveDisplay
	// so that we here have ( int frame_idx, float scale ) points
	// but that will break the xml storage
	if( v.empty() && __velocity_envelope.empty() ) 
	{
		return;
	}
	
	__velocity_envelope.clear();
	if ( v.size() > 0 ) {
		float inv_resolution = __frames / 841.0F;
		for ( int i = 1; i < v.size(); i++ ) {
			float y = ( 91 - v[i - 1]->value ) / 91.0F;
			float k = ( 91 - v[i]->value ) / 91.0F;
			int start_frame = v[i - 1]->frame * inv_resolution;
			int end_frame = v[i]->frame * inv_resolution;
			if ( i == v.size() -1 ) end_frame = __frames;
			int length = end_frame - start_frame ;
			float step = ( y - k ) / length;;
			for ( int z = start_frame ; z < end_frame; z++ ) {
				__data_l[z] = __data_l[z] * y;
				__data_r[z] = __data_r[z] * y;
				y-=step;
			}
		}
		
		for(auto& pEnvPtr : v){
			__velocity_envelope.emplace_back( std::make_unique<EnvelopePoint>( pEnvPtr->value, pEnvPtr->frame ) );
		}
	}
	__is_modified = true;
}

void Sample::apply_pan( const PanEnvelope& p )
{
	// TODO see apply_velocity
	if( p.empty() && __pan_envelope.empty() )
	{
		return;
	}
	
	__pan_envelope.clear();
	if ( p.size() > 0 ) {
		float inv_resolution = __frames / 841.0F;
		for ( int i = 1; i < p.size(); i++ ) {
			float y = ( 45 - p[i - 1]->value ) / 45.0F;
			float k = ( 45 - p[i]->value ) / 45.0F;
			int start_frame = p[i - 1]->frame * inv_resolution;
			int end_frame = p[i]->frame * inv_resolution;
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
		
		for(auto& pEnvPtr : p){
			__pan_envelope.emplace_back( std::make_unique<EnvelopePoint>( pEnvPtr->value, pEnvPtr->frame ) );
		}
	}
	__is_modified = true;
}

void Sample::apply_rubberband( const Rubberband& rb )
{
	// TODO see Rubberband declaration in sample.h
#ifdef H2CORE_HAVE_RUBBERBAND
	//if( __rubberband == rb ) return;
	if( !rb.use ) return;
	// compute rubberband options
	double output_duration = 60.0 / Hydrogen::get_instance()->getNewBpmJTM() * rb.divider;
	double time_ratio = output_duration / get_sample_duration();
	RubberBand::RubberBandStretcher::Options options = compute_rubberband_options( rb );
	double pitch_scale = compute_pitch_scale( rb );
	// output buffer
	int out_buffer_size = ( int )( __frames* time_ratio + 0.1 );
	// instantiate rubberband
	RubberBand::RubberBandStretcher* rubber = new RubberBand::RubberBandStretcher( __sample_rate, 2, options, time_ratio, pitch_scale );
	rubber->setDebugLevel( RUBBERBAND_DEBUG );
	rubber->setExpectedInputDuration( __frames );

	//DEBUGLOG( QString( "on %1\n\toptions\t\t: %2\n\ttime ratio\t: %3\n\tpitch\t\t: %4" ).arg( get_filename() ).arg( options ).arg( time_ratio ).arg( pitch_scale ) );

	int block_size = Hydrogen::get_instance()->getAudioOutput()->getBufferSize();
	float* ibuf[2];
	int studied = 0;

	while( studied < __frames ) {
		bool final = (studied + block_size >= __frames);
		int ibs = (final ? (__frames-studied) : block_size );
		//___DEBUGLOG( QString(" ibs : %1").arg( ibs ) );
		float tempIbufL[ibs];
		float tempIbufR[ibs];
		for(int i = 0 ; i < ibs; i++) {
			tempIbufL[i] = __data_l[i + studied];
			tempIbufR[i] = __data_r[i + studied];
		}
		ibuf[0] = tempIbufL;
		ibuf[1] = tempIbufR;
		rubber->study( ibuf, ibs, final );
		studied += ibs;
		if( final ) break;
	}

	//int buffer_free = out_buffer_size;
	float* out_data_l= new float[ out_buffer_size ];
	float* out_data_r = new float[ out_buffer_size ];
	// retrieve data
	float* obuf[2];
	int processed = 0;
	int available = 0;
	int retrieved = 0;
	while( processed < __frames ) {
		bool final = (processed + block_size >= __frames);
		int ibs = (final ? (__frames-processed) : block_size );
		float tempIbufL[ibs];
		float tempIbufR[ibs];
		for(int i = 0 ; i < ibs; i++) {
			tempIbufL[i] = __data_l[i + processed];
			tempIbufR[i] = __data_r[i + processed];
		}
		ibuf[0] = tempIbufL;
		ibuf[1] = tempIbufR;
		rubber->process( ibuf, ibs, final );
		processed += ibs;

		// first run of stretcher.retrieve() frames.
		// we retrieve audio frames from stretcher until
		// available is >0. but important here is that stretcher
		// is not finished processing even available is 0.
		// stretcher finished with -1. but this status we
		// cannot reach here, because stretcher becomes new
		// inputbuffer in this while loop.
		// we need a final run of stretcher after input processing loop
		// is finished
		while( (available=rubber->available())>0) {
			obuf[0] = &out_data_l[retrieved];
			obuf[1] = &out_data_r[retrieved];
			int n = rubber->retrieve( obuf, available);

			retrieved += n;
			//buffer_free -= n;
		}
		if( final ) break;
	}

	// second run of stretcher to retrieve all last
	// frames until stretcher returns -1.
	while( (available=rubber->available())!= -1) {
		obuf[0] = &out_data_l[retrieved];
		obuf[1] = &out_data_r[retrieved];
		int n = rubber->retrieve( obuf, available);

		retrieved += n;
		//buffer_free -= n;
	}

//    qDebug()<<"outputbuffersize"<<out_buffer_size;
//    qDebug()<<"retrieved frames"<<retrieved;
//    qDebug()<<"stretcher status"<<rubber->available();

	// DEBUGLOG( QString( "%1 frames processed, %2 frames retrieved" ).arg( __frames ).arg( retrieved ) );
	// final data buffers
	delete [] __data_l;
	delete [] __data_r;
	__data_l = new float[ retrieved ];
	__data_r = new float[ retrieved ];
	memcpy( __data_l, out_data_l, retrieved*sizeof( float ) );
	memcpy( __data_r, out_data_r, retrieved*sizeof( float ) );
	delete [] out_data_l;
	delete [] out_data_r;
	// update sample
	__rubberband = rb;
	__frames = retrieved;
	__is_modified = true;
#endif
}

bool Sample::exec_rubberband_cli( const Rubberband& rb )
{
	//set the path to rubberband-cli
	QString program = Preferences::get_instance()->m_rubberBandCLIexecutable;
	//test the path. if test fails return NULL
	if ( QFile( program ).exists() == false && rb.use ) {
		ERRORLOG( QString( "Rubberband executable: File %1 not found" ).arg( program ) );
		return false;
	}

	if( rb.use ) {
		QString outfilePath =  QDir::tempPath() + "/tmp_rb_outfile.wav";
		if( !write( outfilePath ) ) {
			ERRORLOG( "unable to write sample" );
			return false;
		};

		unsigned rubberoutframes = 0;
		double ratio = 1.0;
		double durationtime = 60.0 / Hydrogen::get_instance()->getNewBpmJTM() * rb.divider/*beats*/;
		double induration = get_sample_duration();
		if ( induration != 0.0 ) ratio = durationtime / induration;

		rubberoutframes = int( __frames * ratio + 0.1 );
		_INFOLOG( QString( "ratio: %1, rubberoutframes: %2, rubberinframes: %3" ).arg( ratio ).arg ( rubberoutframes ).arg ( __frames ) );

		QObject*	pParent = nullptr;
		QProcess*	pRrubberbandProc = new QProcess( pParent );

		QStringList arguments;
		QString rCs = QString( " %1" ).arg( rb.c_settings );
		float pitch = pow( 1.0594630943593, ( double )rb.pitch );
		QString rPs = QString( " %1" ).arg( pitch );
		QString rubberResultPath = QDir::tempPath() + "/tmp_rb_result_file.wav";

		arguments << "-D" << QString( " %1" ).arg( durationtime ) 	//stretch or squash to make output file X seconds long
		          << "--threads"					//assume multi-CPU even if only one CPU is identified
		          << "-P"						//aim for minimal time distortion
		          << "-f" << rPs					//pitch
		          << "-c" << rCs					//"crispness" levels
		          << outfilePath 					//infile
		          << rubberResultPath;					//outfile

		pRrubberbandProc->start( program, arguments );

		while( !	pRrubberbandProc->waitForFinished() ) {
			//_ERRORLOG( QString( "prozessing" ));
		}
		if ( QFile( rubberResultPath ).exists() == false ) {
			_ERRORLOG( QString( "Rubberband reimporter File %1 not found" ).arg( rubberResultPath ) );
			return false;
		}

		Sample* p_Rubberbanded = Sample::load( rubberResultPath.toLocal8Bit() );
		if( p_Rubberbanded==nullptr ) {
			return false;
		}

		QFile( outfilePath ).remove();

		QFile( rubberResultPath ).remove();

		__frames = p_Rubberbanded->get_frames();
		__data_l = p_Rubberbanded->get_data_l();
		__data_r = p_Rubberbanded->get_data_r();
		p_Rubberbanded->__data_l = nullptr;
		p_Rubberbanded->__data_r = nullptr;
		__is_modified = true;
		__rubberband = rb;
		delete p_Rubberbanded;
	}
	return true;
}

Sample::Loops::LoopMode Sample::parse_loop_mode( const QString& string )
{
	QByteArray byteArray =	string.toLocal8Bit();
	char* mode = byteArray.data();
	for( int i=Loops::FORWARD; i<=Loops::PINGPONG; i++ ) {
		if( 0 == strncasecmp( mode, __loop_modes[i], sizeof( __loop_modes[i] ) ) ) return ( Loops::LoopMode )i;
	}
	return Loops::FORWARD;
}

bool Sample::write( const QString& path, int format )
{
	float* obuf = new float[ SAMPLE_CHANNELS * __frames ];
	for ( int i = 0; i < __frames; ++i ) {
		float value_l = __data_l[i];
		float value_r = __data_r[i];
		if ( value_l > 1.f ) value_l = 1.f;
		else if ( value_l < -1.f ) value_l = -1.f;
		else if ( value_r > 1.f ) value_r = 1.f;
		else if ( value_r < -1.f ) value_r = -1.f;
		obuf[ i* SAMPLE_CHANNELS + 0 ] = value_l;
		obuf[ i* SAMPLE_CHANNELS + 1 ] = value_r;
	}
	SF_INFO sf_info;
	sf_info.channels = SAMPLE_CHANNELS;
	sf_info.frames = __frames;
	sf_info.samplerate = __sample_rate;
	sf_info.format = format;
	if ( !sf_format_check( &sf_info ) ) {
		___ERRORLOG( "SF_INFO error" );
		delete[] obuf;
		return false;
	}

	SNDFILE* sf_file = sf_open( path.toLocal8Bit().data(), SFM_WRITE, &sf_info ) ;

	if ( sf_file==nullptr ) {
		___ERRORLOG( QString( "sf_open error : %1" ).arg( sf_strerror( sf_file ) ) );
		delete[] obuf;
		return false;
	}

	sf_count_t res = sf_writef_float( sf_file, obuf, __frames );

	if ( res<=0 ) {
		___ERRORLOG( QString( "sf_writef_float error : %1" ).arg( sf_strerror( sf_file ) ) );
		delete[] obuf;
		return false;
	}

	sf_close( sf_file );

	delete[] obuf;
	return true;
}

#ifdef H2CORE_HAVE_RUBBERBAND
static double compute_pitch_scale( const Sample::Rubberband& rb )
{
	double pitchshift = rb.pitch;
	double frequencyshift = 1.0;
	if ( pitchshift != 0.0 ) {
		frequencyshift *= pow( 2.0, pitchshift / 12 );
	}
	//float pitch = pow( 1.0594630943593, ( double )rb.pitch );
	return frequencyshift;
}

static RubberBand::RubberBandStretcher::Options compute_rubberband_options( const Sample::Rubberband& rb )
{
	// default settings
	enum {
		CompoundDetector,
		PercussiveDetector,
		SoftDetector
	} detector = CompoundDetector;
	enum {
		NoTransients,
		BandLimitedTransients,
		Transients
	} transients = Transients;
	bool lamination = true;
	bool longwin = false;
	bool shortwin = false;
	RubberBand::RubberBandStretcher::Options options = RubberBand::RubberBandStretcher::DefaultOptions;
	// apply our settings
	int crispness = rb.c_settings;
	// compute result options
	switch ( crispness ) {
	case -1:
		crispness = 5;
		break;
	case 0:
		detector = CompoundDetector;
		transients = NoTransients;
		lamination = false;
		longwin = true;
		shortwin = false;
		break;
	case 1:
		detector = SoftDetector;
		transients = Transients;
		lamination = false;
		longwin = true;
		shortwin = false;
		break;
	case 2:
		detector = CompoundDetector;
		transients = NoTransients;
		lamination = false;
		longwin = false;
		shortwin = false;
		break;
	case 3:
		detector = CompoundDetector;
		transients = NoTransients;
		lamination = true;
		longwin = false;
		shortwin = false;
		break;
	case 4:
		detector = CompoundDetector;
		transients = BandLimitedTransients;
		lamination = true;
		longwin = false;
		shortwin = false;
		break;
	case 5:
		detector = CompoundDetector;
		transients = Transients;
		lamination = true;
		longwin = false;
		shortwin = false;
		break;
	case 6:
		detector = CompoundDetector;
		transients = Transients;
		lamination = false;
		longwin = false;
		shortwin = true;
		break;
	};
	//if (realtime)    options |= RubberBand::RubberBandStretcher::OptionProcessRealTime;
	//if (precise)     options |= RubberBand::RubberBandStretcher::OptionStretchPrecise;

	if ( !lamination ) options |= RubberBand::RubberBandStretcher::OptionPhaseIndependent;
	if ( longwin )     options |= RubberBand::RubberBandStretcher::OptionWindowLong;
	if ( shortwin )    options |= RubberBand::RubberBandStretcher::OptionWindowShort;
	options |= RubberBand::RubberBandStretcher::OptionProcessOffline;
	options |= RubberBand::RubberBandStretcher::OptionStretchPrecise;
	//if (smoothing)   options |= RubberBand::RubberBandStretcher::OptionSmoothingOn;
	//if (formant)     options |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
	//if (hqpitch)     options |= RubberBand::RubberBandStretcher::OptionPitchHighQuality;
	options |= RubberBand::RubberBandStretcher::OptionPitchHighQuality;
	/*
	switch (threading) {
	case 0:
		options |= RubberBand::RubberBandStretcher::OptionThreadingAuto;
		break;
	case 1:
		options |= RubberBand::RubberBandStretcher::OptionThreadingNever;
		break;
	case 2:
		options |= RubberBand::RubberBandStretcher::OptionThreadingAlways;
		break;
	}
	*/
	switch ( transients ) {
	case NoTransients:
		options |= RubberBand::RubberBandStretcher::OptionTransientsSmooth;
		break;
	case BandLimitedTransients:
		options |= RubberBand::RubberBandStretcher::OptionTransientsMixed;
		break;
	case Transients:
		options |= RubberBand::RubberBandStretcher::OptionTransientsCrisp;
		break;
	}
	/*
	switch (detector) {
	case CompoundDetector:
		options |= RubberBand::RubberBandStretcher::OptionDetectorCompound;
		break;
	case PercussiveDetector:
		options |= RubberBand::RubberBandStretcher::OptionDetectorPercussive;
		break;
	case SoftDetector:
		options |= RubberBand::RubberBandStretcher::OptionDetectorSoft;
		break;
	}
	*/
	return options;
}
#endif

};

/* vim: set softtabstop=4 noexpandtab: */
