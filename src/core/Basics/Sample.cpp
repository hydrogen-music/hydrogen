/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */



#include <limits>
#include <memory>

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Helpers/Filesystem.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Note.h>

#if defined(H2CORE_HAVE_RUBBERBAND) || _DOXYGEN_
#include <rubberband/RubberBandStretcher.h>
#define RUBBERBAND_BUFFER_OVERSIZE  500
#define RUBBERBAND_DEBUG            0
#endif

namespace H2Core
{

const std::vector<QString> Sample::__loop_modes = { "forward", "reverse", "pingpong" };

#if defined(H2CORE_HAVE_RUBBERBAND) || _DOXYGEN_
static double compute_pitch_scale( const Sample::Rubberband& r );
static RubberBand::RubberBandStretcher::Options compute_rubberband_options( const Sample::Rubberband& r );
#endif


/* EnvelopePoint */
EnvelopePoint::EnvelopePoint() : frame( 0 ), value( 0 )
{
}

EnvelopePoint::EnvelopePoint( int f, int v ) : frame( f ), value( v )
{
}

EnvelopePoint::EnvelopePoint( const EnvelopePoint& other ) : Object(other), frame ( other.frame ), value ( other.value )
{
}
/* EnvelopePoint */


Sample::Sample( const QString& filepath, const License& license, int frames, int sample_rate, float* data_l, float* data_r ) 
  : m_bIsLoaded( false ),
	__filepath( filepath ),
	__frames( frames ),
	__sample_rate( sample_rate ),
	__data_l( data_l ),
	__data_r( data_r ),
	__is_modified( false ),
	m_license( license )
{
	if ( filepath.lastIndexOf( "/" ) <= 0 ) {
		WARNINGLOG( QString( "Provided filepath [%1] does not seem like an absolute path. Sample will most probably be unable to load." ) );
	}
}

Sample::Sample( std::shared_ptr<Sample> pOther ) :
	Object( *pOther ),
	m_bIsLoaded( pOther->m_bIsLoaded ),
	__filepath( pOther->get_filepath() ),
	__frames( pOther->get_frames() ),
	__sample_rate( pOther->get_sample_rate() ),
	__data_l( nullptr ),
	__data_r( nullptr ),
	__is_modified( pOther->get_is_modified() ),
	__loops( pOther->__loops ),
	__rubberband( pOther->__rubberband ),
	m_license( pOther->m_license )
{

	__data_l = new float[__frames];
	__data_r = new float[__frames];
	
	// Since the third argument of memcpy takes the number of bytes,
	// which are about to be copied, and the data is given in float,
	// which are  four bytes each, the number of copied frames
	// `__frames` has to be multiplied by four.
	memcpy( __data_l, pOther->get_data_l(), __frames * 4 );
	memcpy( __data_r, pOther->get_data_r(), __frames * 4 );
	
	auto pPan = pOther->get_pan_envelope();
	for( int i=0; i<pPan.size(); i++ ) {
		__pan_envelope.push_back( pPan.at(i) );
	}

	auto pVelocity = pOther->get_velocity_envelope();
	for( int i=0; i<pVelocity.size(); i++ ) {
		__velocity_envelope.push_back( pVelocity.at(i) );
	}
}

Sample::~Sample()
{
	if ( __data_l != nullptr ) {
		delete[] __data_l;
	}
	if ( __data_r != nullptr ) {
		delete[] __data_r;
	}
}

void Sample::set_filename( const QString& filename )
{
	QFileInfo Filename = QFileInfo( filename );
	QFileInfo Dest = QFileInfo( get_filepath() );
	__filepath = QDir(Dest.absolutePath()).filePath( Filename.fileName() );
}


const QString& Sample::get_filepath() const
{
	return __filepath;
}

std::shared_ptr<Sample> Sample::load( const QString& sFilepath, const License& license )
{
	std::shared_ptr<Sample> pSample;
	
	if( !Filesystem::file_readable( sFilepath ) ) {
		ERRORLOG( QString( "Unable to read %1" ).arg( sFilepath ) );
		return nullptr;
	}

	pSample = std::make_shared<Sample>( sFilepath, license );

	// Samples loaded this way have no loops, rubberband, or envelopes
	// set. Therefore, we do not have to pass a tempo in here.
	if( !pSample->load() ) {
		return nullptr;
	}
	
	return pSample;
}

bool Sample::load( float fBpm )
{
	// Will contain a bunch of metadata about the loaded sample.
	SF_INFO sound_info = {0};

	// Opens file in read-only mode.
#ifdef WIN32
	// On Windows we use a special version of sf_open to ensure we get all
	// characters of the filename entered in the GUI right. No matter which
	// encoding was used locally.
	// We have to terminate the string using a null character ourselves.
	QString sPath( get_filepath() );
	const QString sPaddedPath = sPath.append( '\0' );
	wchar_t* encodedFilename = new wchar_t[ sPaddedPath.size() ];

	sPaddedPath.toWCharArray( encodedFilename );

	SNDFILE* file = sf_wchar_open( encodedFilename, SFM_READ,
								   &sound_info );
	delete encodedFilename;
#else
	SNDFILE* file = sf_open( get_filepath().toLocal8Bit(), SFM_READ,
							 &sound_info );
#endif
	if ( file == nullptr ) {
		ERRORLOG( QString( "Error loading file [%1] with format [%2]: %3" )
				  .arg( get_filepath() )
				  .arg( sndfileFormatToQString( sound_info.format ) )
				  .arg( sf_strerror( file ) ) );
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
		WARNINGLOG( QString( "%1 is an empty sample" ).arg( get_filepath() ) );
	}
	
	// Deallocate the handler.
	if ( sf_close( file ) != 0 ){
		WARNINGLOG( QString( "Unable to close sample file %1" ).arg( get_filepath() ) );
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
			__data_l[i] = buffer[i * SAMPLE_CHANNELS ];
			__data_r[i] = buffer[i * SAMPLE_CHANNELS + 1 ];
		}
	}
	delete[] buffer;

	// Apply modifiers (if present/altered).
	if ( ! apply_loops() ) {
		WARNINGLOG( "Unable to apply loops" );
	}
	apply_velocity();
	apply_pan();
#ifdef H2CORE_HAVE_RUBBERBAND
	apply_rubberband( fBpm );
#else
	if ( ! exec_rubberband_cli( fBpm ) ) {
		WARNINGLOG( "Unable to apply rubberband" );
	}
#endif

	m_bIsLoaded = true;

	return true;
}

void Sample::unload()
{
	if ( __data_l != nullptr ) {
		delete [] __data_l;
	}

	if ( __data_r != nullptr ) {
		delete [] __data_r;
	}
	__frames = __sample_rate = 0;
	/** #__is_modified = false; leave this unchanged as pan,
	    velocity, loop and rubberband are kept unchanged */

	__data_l = __data_r = nullptr;

	m_bIsLoaded = false;
}

bool Sample::apply_loops()
{
	if( __loops.start_frame == 0 && __loops.loop_frame == 0 &&
		__loops.end_frame == 0 && __loops.count == 0 ) {
		// Default parameters. No looping was set by the
		// user. Skipping.
		return true;
	}
	
	if( __loops.start_frame<0 ) {
		ERRORLOG( QString( "start_frame %1 < 0 is not allowed" ).arg( __loops.start_frame ) );
		return false;
	}
	if( __loops.loop_frame<__loops.start_frame ) {
		ERRORLOG( QString( "loop_frame %1 < start_frame %2 is not allowed" ).arg( __loops.loop_frame ).arg( __loops.start_frame ) );
		return false;
	}
	if( __loops.end_frame<__loops.loop_frame ) {
		ERRORLOG( QString( "end_frame %1 < loop_frame %2 is not allowed" ).arg( __loops.end_frame ).arg( __loops.loop_frame ) );
		return false;
	}
	if( __loops.end_frame>__frames ) {
		ERRORLOG( QString( "end_frame %1 > __frames %2 is not allowed" ).arg( __loops.end_frame ).arg( __frames ) );
		return false;
	}
	if( __loops.count<0 ) {
		ERRORLOG( QString( "count %1 < 0 is not allowed" ).arg( __loops.count ) );
		return false;
	}
	//if( lo == __loops ) return true;

	bool full_loop = __loops.start_frame==__loops.loop_frame;
	int full_length =  __loops.end_frame - __loops.start_frame;
	int loop_length =  __loops.end_frame - __loops.loop_frame;
	int new_length = full_length + loop_length * __loops.count;

	float* new_data_l = new float[ new_length ];
	float* new_data_r = new float[ new_length ];

	// copy full_length frames to new_data
	if ( __loops.mode==Loops::REVERSE && ( __loops.count==0 || full_loop ) ) {
		if( full_loop ) {
			// copy end => start
			for( int i=0, j=__loops.end_frame; i<full_length; i++, j-- ) {
				new_data_l[i]=__data_l[j];
			}
			for( int i=0, j=__loops.end_frame; i<full_length; i++, j-- ) {
				new_data_r[i]=__data_r[j];
			}
		} else {
			// copy start => loop
			int to_loop = __loops.loop_frame - __loops.start_frame;
			memcpy( new_data_l, __data_l+__loops.start_frame, sizeof( float )*to_loop );
			memcpy( new_data_r, __data_r+__loops.start_frame, sizeof( float )*to_loop );
			// copy end => loop
			for( int i=to_loop, j=__loops.end_frame; i<full_length; i++, j-- ) {
				new_data_l[i]=__data_l[j];
			}
			for( int i=to_loop, j=__loops.end_frame; i<full_length; i++, j-- ) {
				new_data_r[i]=__data_r[j];
			}
		}
	} else {
		// copy start => end
		memcpy( new_data_l, __data_l+__loops.start_frame, sizeof( float )*full_length );
		memcpy( new_data_r, __data_r+__loops.start_frame, sizeof( float )*full_length );
	}
	// copy the loops
	if( __loops.count>0 ) {
		int x = full_length;
		bool forward = ( __loops.mode==Loops::FORWARD );
		bool ping_pong = ( __loops.mode==Loops::PINGPONG );
		for( int i=0; i<__loops.count; i++ ) {
			if ( forward ) {
				// copy loop => end
				memcpy( &new_data_l[x], __data_l+__loops.loop_frame, sizeof( float )*loop_length );
				memcpy( &new_data_r[x], __data_r+__loops.loop_frame, sizeof( float )*loop_length );
			} else {
				// copy end => loop
				for( int i=__loops.end_frame, y=x; i>__loops.loop_frame; i--, y++ ) {
					new_data_l[y]=__data_l[i];
				}
				for( int i=__loops.end_frame, y=x; i>__loops.loop_frame; i--, y++ ) {
					new_data_r[y]=__data_r[i];
				}
			}
			x+=loop_length;
			if( ping_pong ) {
				forward=!forward;
			}
		}
		assert( x==new_length );
	}
	delete[] __data_l;
	delete[] __data_r;
	__data_l = new_data_l;
	__data_r = new_data_r;
	__frames = new_length;
	__is_modified = true;
	
	return true;
}

void Sample::apply_velocity()
{
	// TODO frame width (841) and height (91) should go out of here
	// the VelocityEnvelope should be processed within TargetWaveDisplay
	// so that we here have ( int frame_idx, float scale ) points
	// but that will break the xml storage
	if ( __velocity_envelope.size() == 0 ) {
		return;
	}
	
	float inv_resolution = __frames / 841.0F;
	for ( int i = 1; i < __velocity_envelope.size(); i++ ) {
		float y = ( 91 - __velocity_envelope[i - 1].value ) / 91.0F;
		float k = ( 91 - __velocity_envelope[i].value ) / 91.0F;
		int start_frame = __velocity_envelope[i - 1].frame * inv_resolution;
		int end_frame = __velocity_envelope[i].frame * inv_resolution;
		if ( i == __velocity_envelope.size() -1 ) {
			end_frame = __frames;
		}
		int length = end_frame - start_frame ;
		float step = ( y - k ) / length;;
		for ( int z = start_frame ; z < end_frame; z++ ) {
			__data_l[z] = __data_l[z] * y;
			__data_r[z] = __data_r[z] * y;
			y-=step;
		}
	}

	__is_modified = true;
}

void Sample::apply_pan()
{
	if( __pan_envelope.size() == 0 ) {
		return;
	}
	
	float inv_resolution = __frames / 841.0F;
	for ( int i = 1; i < __pan_envelope.size(); i++ ) {
		float y = ( 45 - __pan_envelope[i - 1].value ) / 45.0F;
		float k = ( 45 - __pan_envelope[i].value ) / 45.0F;
		int start_frame = __pan_envelope[i - 1].frame * inv_resolution;
		int end_frame = __pan_envelope[i].frame * inv_resolution;
		if ( i == __pan_envelope.size() -1 ) {
			end_frame = __frames;
		}
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

	__is_modified = true;
}

void Sample::apply_rubberband( float fBpm ) {
	// TODO see Rubberband declaration in sample.h
#ifdef H2CORE_HAVE_RUBBERBAND
	if( ! __rubberband.use ){
		// Default behavior
		return;
	}
	
	// compute rubberband options
	double output_duration = 60.0 / fBpm * __rubberband.divider;
	double time_ratio = output_duration / get_sample_duration();
	RubberBand::RubberBandStretcher::Options options =
		compute_rubberband_options( __rubberband );
	double pitch_scale = compute_pitch_scale( __rubberband );
	// output buffer
	//
	// Sometimes the Rubber Band result is _way_ larger than expected,
	// e.g. `out_buffer_size` = 1837 and retrieved frames = 11444. No
	// idea what is going on there but it would make Hydrogen crash if
	// not accounting for resizing the output buffer. The +10 is in
	// place to cover the more frequent situations of a difference of
	// just one frame.
	int out_buffer_size = static_cast<int>( __frames * time_ratio + 0.1 + 10 );
	// instantiate rubberband
	RubberBand::RubberBandStretcher rubber = RubberBand::RubberBandStretcher( __sample_rate, 2, options, time_ratio, pitch_scale );
	rubber.setDebugLevel( RUBBERBAND_DEBUG );
	// This option will be ignored in real-time processing.
	rubber.setExpectedInputDuration( __frames );

	int retrieved = 0;
	//int buffer_free = out_buffer_size;
	float* out_data_l = new float[ out_buffer_size ];
	float* out_data_r = new float[ out_buffer_size ];
	float* out_data_l_tmp;
	float* out_data_r_tmp;

	DEBUGLOG( QString( "on %1\n\toptions\t\t: %2\n\ttime ratio\t: %3\n\tpitch\t\t: %4" ).arg( get_filename() ).arg( options ).arg( time_ratio ).arg( pitch_scale ) );

	float* ibuf[2];
	int block_size = MAX_BUFFER_SIZE;

	// If the RUB button in the player control is activated and
	// Hydrogen is told to apply Rubber Band to samples on-the-fly
	// when encountering tempo changes, we will use Rubber Band's
	// real-time processing mode.
	if ( !Preferences::get_instance()->getRubberBandBatchMode() ) {
		ibuf[0] = __data_l;
		ibuf[1] = __data_r;
		rubber.study( ibuf, __frames, true );
	} else {
		rubber.setMaxProcessSize( block_size );
	}

	// retrieve data
	float* obuf[2];
	int processed = 0;
	int available = 0;
	int nRequired = 0;

	while( processed < __frames ) {

		if ( !Preferences::get_instance()->getRubberBandBatchMode() ) {
			// Ask Rubber Band how many samples it requires to produce
			// further output.
			nRequired = rubber.getSamplesRequired();
		} else {
			nRequired = block_size;
		}
		bool final = (processed + nRequired >= __frames);
		int ibs = (final ? (__frames-processed) : nRequired );
		float tempIbufL[ibs];
		float tempIbufR[ibs];
		for(int i = 0 ; i < ibs; i++) {
			tempIbufL[i] = __data_l[i + processed];
			tempIbufR[i] = __data_r[i + processed];
		}
		ibuf[0] = tempIbufL;
		ibuf[1] = tempIbufR;
		rubber.process( ibuf, ibs, final );
		processed += ibs;

		// .available() == 0 does indicate that Rubber Band requires
		// more input samples in order to produce more output. Whether
		// the stretching is complete will be checked after the parent
		// while loop.
		while( (available=rubber.available()) > 0 ) {
				
			if ( retrieved + available > out_buffer_size ) {
				// The buffers defined above are too small.
				int nNewBufferSize = static_cast<int>( ( retrieved + available ) * 1.2 );
				WARNINGLOG( QString( "Unexpected output size of stretched Rubber Band sample. Increasing output buffer from [%1] to [%2]" )
							.arg( out_buffer_size )
							.arg( nNewBufferSize ) );
				out_data_l_tmp = new float[ out_buffer_size ];
				out_data_r_tmp = new float[ out_buffer_size ];
				memcpy( out_data_l_tmp, out_data_l, out_buffer_size * sizeof( float ) );
				memcpy( out_data_r_tmp, out_data_r, out_buffer_size * sizeof( float ) );
				delete [] out_data_l;
				delete [] out_data_r;
				out_data_l = new float[ nNewBufferSize ];
				out_data_r = new float[ nNewBufferSize ];
				memcpy( out_data_l, out_data_l_tmp, out_buffer_size * sizeof( float ) );
				memcpy( out_data_r, out_data_r_tmp, out_buffer_size * sizeof( float ) );
				delete [] out_data_l_tmp;
				delete [] out_data_r_tmp;
			}
				
			obuf[0] = &out_data_l[retrieved];
			obuf[1] = &out_data_r[retrieved];
			int n = rubber.retrieve( obuf, available);

			retrieved += n;
		}
		
		if( final ){
			break;
		}
	   
	}

	// second run of stretcher to retrieve all last
	// frames until stretcher returns -1.
	while( (available=rubber.available())!= -1) {
			
		if ( retrieved + available > out_buffer_size ) {
			// The buffers defined above are too small.
			int nNewBufferSize = static_cast<int>( ( retrieved + available ) * 1.5 );
			WARNINGLOG( QString( "Unexpected output size of stretched Rubber Band sample. Increasing output buffer from [%1] to [%2[" )
						.arg( out_buffer_size )
						.arg( nNewBufferSize ) );
			out_data_l_tmp = new float[ out_buffer_size ];
			out_data_r_tmp = new float[ out_buffer_size ];
			memcpy( out_data_l_tmp, out_data_l, out_buffer_size * sizeof( float ) );
			memcpy( out_data_r_tmp, out_data_r, out_buffer_size * sizeof( float ) ); 
			delete [] out_data_l;
			delete [] out_data_r;
			out_data_l = new float[ nNewBufferSize ];
			out_data_r = new float[ nNewBufferSize ];
			memcpy( out_data_l, out_data_l_tmp, out_buffer_size * sizeof( float ) );
			memcpy( out_data_r, out_data_r_tmp, out_buffer_size * sizeof( float ) );
			delete [] out_data_l_tmp;
			delete [] out_data_r_tmp;

			out_buffer_size = nNewBufferSize;
		}
	
		obuf[0] = &out_data_l[retrieved];
		obuf[1] = &out_data_r[retrieved];
		int n = rubber.retrieve( obuf, available);

		retrieved += n;
	}
	
	delete [] __data_l;
	delete [] __data_r;
	__data_l = new float[ retrieved ];
	__data_r = new float[ retrieved ];
	memcpy( __data_l, out_data_l, retrieved*sizeof( float ) );
	memcpy( __data_r, out_data_r, retrieved*sizeof( float ) );
	delete [] out_data_l;
	delete [] out_data_r;

	// update sample
	__frames = retrieved;
	__is_modified = true;
#endif
}

bool Sample::exec_rubberband_cli( float fBpm )
{
	if ( ! __rubberband.use ) {
		// Default behavior
		return true;
	}
	
	//set the path to rubberband-cli
	QString program = Preferences::get_instance()->m_sRubberBandCLIexecutable;
	//test the path. if test fails return NULL
	if ( QFile( program ).exists() == false && __rubberband.use ) {
		ERRORLOG( QString( "Rubberband executable: File %1 not found" ).arg( program ) );
		return false;
	}

	QString outfilePath =  QDir::tempPath() + "/tmp_rb_outfile.wav";
	if( !write( outfilePath ) ) {
		ERRORLOG( "unable to write sample" );
		return false;
	};

	unsigned rubberoutframes = 0;
	double ratio = 1.0;
	double durationtime = 60.0 / fBpm * __rubberband.divider/*beats*/;
	double induration = get_sample_duration();
	if ( induration != 0.0 ) {
		ratio = durationtime / induration;
	}

	rubberoutframes = int( __frames * ratio + 0.1 );
	_INFOLOG( QString( "ratio: %1, rubberoutframes: %2, rubberinframes: %3" ).arg( ratio ).arg ( rubberoutframes ).arg ( __frames ) );

	QObject*	pParent = nullptr;
	QProcess*	pRubberbandProc = new QProcess( pParent );

	QStringList arguments;
	QString rCs = QString( " %1" ).arg( __rubberband.c_settings );
	float fFrequency = Note::pitchToFrequency( ( double )__rubberband.pitch );
	QString rFs = QString( " %1" ).arg( fFrequency );
	QString rubberResultPath = QDir::tempPath() + "/tmp_rb_result_file.wav";

	arguments << "-D" << QString( " %1" ).arg( durationtime ) 	//stretch or squash to make output file X seconds long
			  << "--threads"					//assume multi-CPU even if only one CPU is identified
			  << "-P"						//aim for minimal time distortion
			  << "-f" << rFs					//frequency
			  << "-c" << rCs					//"crispness" levels
			  << outfilePath 					//infile
			  << rubberResultPath;					//outfile

	pRubberbandProc->start( program, arguments );

	while( pRubberbandProc->state() != QProcess::NotRunning 
		   && !pRubberbandProc->waitForFinished() ) {
		//_ERRORLOG( QString( "processing" ));
	}

	delete pRubberbandProc;
	if ( QFile( rubberResultPath ).exists() == false ) {
		_ERRORLOG( QString( "Rubberband reimporter File %1 not found" ).arg( rubberResultPath ) );
		return false;
	}

	auto p_Rubberbanded = Sample::load( rubberResultPath );
	if( p_Rubberbanded == nullptr ) {
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
	
	return true;
}

Sample::Loops::LoopMode Sample::parse_loop_mode( const QString& sMode )
{
	if ( sMode == "forward" ) {
		return Loops::FORWARD;
	} else if ( sMode == "reverse" ) {
		return Loops::REVERSE;
	} else if ( sMode == "pingpong" ) {
		return Loops::PINGPONG;
	}
	
	return Loops::FORWARD;
}

bool Sample::write( const QString& path, int format ) const
{
	float* obuf = new float[ SAMPLE_CHANNELS * __frames ];
	for ( int i = 0; i < __frames; ++i ) {
		float value_l = __data_l[i];
		float value_r = __data_r[i];
		
		if ( value_l > 1.f ) {
			value_l = 1.f;
		} else if ( value_l < -1.f ) {
			value_l = -1.f;
		} else if ( value_r > 1.f ) {
			value_r = 1.f;
		} else if ( value_r < -1.f ) {
			value_r = -1.f;
		}
		
		obuf[ i* SAMPLE_CHANNELS + 0 ] = value_l;
		obuf[ i* SAMPLE_CHANNELS + 1 ] = value_r;
	}
	SF_INFO sf_info;
	sf_info.channels = SAMPLE_CHANNELS;
	sf_info.frames = __frames;
	sf_info.samplerate = __sample_rate;
	sf_info.format = format;
	if ( !sf_format_check( &sf_info ) ) {
		ERRORLOG( "SF_INFO error" );
		delete[] obuf;
		return false;
	}

#ifdef WIN32
	// On Windows we use a special version of sf_open to ensure we get all
	// characters of the filename entered in the GUI right. No matter which
	// encoding was used locally.
	// We have to terminate the string using a null character ourselves.
	QString sPaddedPath = QString( path ).append( '\0' );
	wchar_t* encodedFilename = new wchar_t[ sPaddedPath.size() ];

	sPaddedPath.toWCharArray( encodedFilename );
	
	SNDFILE* sf_file = sf_wchar_open( encodedFilename, SFM_WRITE,
								   &sf_info );
	delete encodedFilename;
#else
	const auto sPathLocal8Bit = path.toLocal8Bit();
	SNDFILE* sf_file = sf_open( sPathLocal8Bit.data(), SFM_WRITE, &sf_info );
#endif

	if ( sf_file == nullptr ) {
		ERRORLOG( QString( "Unable to create file [%1] with format [%2]: %3" )
				  .arg( path )
				  .arg( sndfileFormatToQString( format ) )
				  .arg( sf_strerror( sf_file ) ) );
		sf_close( sf_file );
		delete[] obuf;
		return false;
	}

	sf_count_t res = sf_writef_float( sf_file, obuf, __frames );

	if ( res<=0 ) {
		ERRORLOG( QString( "sf_writef_float error : %1" ).arg( sf_strerror( sf_file ) ) );
		sf_close( sf_file );
		delete[] obuf;
		return false;
	}

	sf_close( sf_file );
	delete[] obuf;
	return true;
}

QString Sample::Loops::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Loops]\n" ).arg( sPrefix )
			.append( QString( "%1%2start_frame: %3\n" ).arg( sPrefix ).arg( s ).arg( start_frame ) )
			.append( QString( "%1%2loop_frame: %3\n" ).arg( sPrefix ).arg( s ).arg( loop_frame ) )
			.append( QString( "%1%2end_frame: %3\n" ).arg( sPrefix ).arg( s ).arg( end_frame ) )
			.append( QString( "%1%2count: %3\n" ).arg( sPrefix ).arg( s ).arg( count ) )
			.append( QString( "%1%2mode: %3\n" ).arg( sPrefix ).arg( s ).arg( mode ) );
	} else {
		sOutput = QString( "[Loops]" )
			.append( QString( " start_frame: %1" ).arg( start_frame ) )
			.append( QString( ", loop_frame: %1" ).arg( loop_frame ) )
			.append( QString( ", end_frame: %1" ).arg( end_frame ) )
			.append( QString( ", count: %1" ).arg( count ) )
			.append( QString( ", mode: %1" ).arg( mode ) );
	}

	return sOutput;
}

QString Sample::Rubberband::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Rubberband]\n" ).arg( sPrefix )
			.append( QString( "%1%2use: %3\n" ).arg( sPrefix ).arg( s ).arg( use ) )
			.append( QString( "%1%2divider: %3\n" ).arg( sPrefix ).arg( s ).arg( divider ) )
			.append( QString( "%1%2pitch: %3\n" ).arg( sPrefix ).arg( s ).arg( pitch ) )
			.append( QString( "%1%2c_settings: %3\n" ).arg( sPrefix ).arg( s ).arg( c_settings ) );
	} else {
		sOutput = QString( "[Rubberband]" )
			.append( QString( " use: %1" ).arg( use ) )
			.append( QString( ", divider: %1" ).arg( divider ) )
			.append( QString( ", pitch: %1" ).arg( pitch ) )
			.append( QString( ", c_settings: %1" ).arg( c_settings ) );
	}
	return sOutput;
}

QString Sample::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Sample]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bIsLoaded: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsLoaded ) )
			.append( QString( "%1%2filepath: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __filepath ) )
			.append( QString( "%1%2frames: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __frames ) )
			.append( QString( "%1%2sample_rate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __sample_rate ) )
			.append( QString( "%1%2is_modified: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __is_modified ) )
			.append( QString( "%1%2__pan_envelope: [\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppoint : __pan_envelope ) {
			sOutput.append( QString( "%1%2%2frame: %3, value: %4" ).arg( sPrefix ).arg( s )
							.arg( ppoint.frame ).arg( ppoint.value ) );
		}
		sOutput.append( QString( "]\n%1%2__velocity_envelope: [\n" )
						.arg( sPrefix ).arg( s ) );
		for ( const auto& ppoint : __velocity_envelope ) {
			sOutput.append( QString( "%1%2%2frame: %3, value: %4" ).arg( sPrefix ).arg( s )
							.arg( ppoint.frame ).arg( ppoint.value ) );
		}
			sOutput.append( QString( "]\n%1" )
					 .arg( __loops.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1" )
					 .arg( __loops.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1" )
					 .arg( __rubberband.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_license: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_license.toQString( sPrefix + s, bShort ) ) );
	}
	else {
		sOutput = QString( "[Sample]" )
			.append( QString( " m_bIsLoaded: %1" ).arg( m_bIsLoaded ) )
			.append( QString( ", filepath: %1" ).arg( __filepath ) )
			.append( QString( ", frames: %1" ).arg( __frames ) )
			.append( QString( ", sample_rate: %1" ).arg( __sample_rate ) )
			.append( QString( ", is_modified: %1" ).arg( __is_modified ) )
			.append( ", __pan_envelope: [" );
		for ( const auto& ppoint : __pan_envelope ) {
			sOutput.append( QString( "[frame: %1, value: %2] " )
							.arg( ppoint.frame ).arg( ppoint.value ) );
		}
		sOutput.append( "], __velocity_envelope: [" );
		for ( const auto& ppoint : __velocity_envelope ) {
			sOutput.append( QString( "[frame: %1, value: %2] " )
							.arg( ppoint.frame ).arg( ppoint.value ) );
		}
		sOutput.append( QString( "], [%1]" )
					 .arg( __loops.toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", [%1]\n" )
					 .arg( __rubberband.toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", m_license: %1" )
					 .arg( m_license.toQString( sPrefix, bShort ) ) );
	}
	
	return sOutput;
}

QString Sample::sndfileFormatToQString( int nFormat ) {
	QString sFormat;
	if ( nFormat & 0x010000 ) {
		sFormat = "Microsoft WAV format (little endian)";
	} else if ( nFormat & 0x020000 ) {
		sFormat = "Apple/SGI AIFF format (big endian)";
	} else if ( nFormat & 0x030000 ) {
		sFormat = "Sun/NeXT AU format (big endian)";
	} else if ( nFormat & 0x040000 ) {
		sFormat = "RAW PCM data";
	} else if ( nFormat & 0x050000 ) {
		sFormat = "Ensoniq PARIS file format";
	} else if ( nFormat & 0x060000 ) {
		sFormat = "Amiga IFF / SVX8 / SV16 format";
	} else if ( nFormat & 0x070000 ) {
		sFormat = "Sphere NIST format";
	} else if ( nFormat & 0x080000 ) {
		sFormat = "VOC files";
	} else if ( nFormat & 0x0A0000 ) {
		sFormat = "Berkeley/IRCAM/CARL";
	} else if ( nFormat & 0x0B0000 ) {
		sFormat = "Sonic Foundry's 64 bit RIFF/WAV";
	} else if ( nFormat & 0x0C0000 ) {
		sFormat = "Matlab (tm) V4.2 / GNU Octave 2.0";
	} else if ( nFormat & 0x0D0000 ) {
		sFormat = "Matlab (tm) V5.0 / GNU Octave 2.1";
	} else if ( nFormat & 0x0E0000 ) {
		sFormat = "Portable Voice Format";
	} else if ( nFormat & 0x0F0000 ) {
		sFormat = "Fasttracker 2 Extended Instrument";
	} else if ( nFormat & 0x100000 ) {
		sFormat = "HMM Tool Kit format";
	} else if ( nFormat & 0x110000 ) {
		sFormat = "Midi Sample Dump Standard";
	} else if ( nFormat & 0x120000 ) {
		sFormat = "Audio Visual Research";
	} else if ( nFormat & 0x130000 ) {
		sFormat = "MS WAVE with WAVEFORMATEX";
	} else if ( nFormat & 0x160000 ) {
		sFormat = "Sound Designer 2";
	} else if ( nFormat & 0x170000 ) {
		sFormat = "FLAC lossless file format";
	} else if ( nFormat & 0x180000 ) {
		sFormat = "Core Audio File format";
	} else if ( nFormat & 0x190000 ) {
		sFormat = "Psion WVE format";
	} else if ( nFormat & 0x200000 ) {
		sFormat = "Xiph OGG container";
	} else if ( nFormat & 0x210000 ) {
		sFormat = "Akai MPC 2000 sampler";
	} else if ( nFormat & 0x220000 ) {
		sFormat = "RF64 WAV file";
	} else if ( nFormat & 0x230000 ) {
		sFormat = "MPEG-1/2 audio stream_FORMAT_OGG";
	} else {
		return QString( "Unknown format [%1]" ).arg( nFormat );
	}

	QString sSubType;
	if ( nFormat & 0x0001 ) {
		sSubType = "Signed 8 bit data";
	} else if ( nFormat & 0x0002 ) {
		sSubType = "Signed 16 bit data";
	} else if ( nFormat & 0x0003 ) {
		sSubType = "Signed 24 bit data";
	} else if ( nFormat & 0x0004 ) {
		sSubType = "Signed 32 bit data";
	} else if ( nFormat & 0x0005 ) {
		sSubType = "Unsigned 8 bit data (WAV and RAW only)";
	} else if ( nFormat & 0x0006 ) {
		sSubType = "32 bit float data";
	} else if ( nFormat & 0x0007 ) {
		sSubType = "64 bit float data";
	} else if ( nFormat & 0x0010 ) {
		sSubType = "U-Law encoded";
	} else if ( nFormat & 0x0011 ) {
		sSubType = "A-Law encoded";
	} else if ( nFormat & 0x0012 ) {
		sSubType = "IMA ADPCM";
	} else if ( nFormat & 0x0013 ) {
		sSubType = "Microsoft ADPCM";
	} else if ( nFormat & 0x0020 ) {
		sSubType = "GSM 6.10 encoding";
	} else if ( nFormat & 0x0021 ) {
		sSubType = "OKI / Dialogix ADPCM";
	} else if ( nFormat & 0x0022 ) {
		sSubType = "16kbs NMS G721-variant encoding";
	} else if ( nFormat & 0x0023 ) {
		sSubType = "24kbs NMS G721-variant encoding";
	} else if ( nFormat & 0x0024 ) {
		sSubType = "32kbs NMS G721-variant encoding";
	} else if ( nFormat & 0x0030 ) {
		sSubType = "32kbs G721 ADPCM encoding";
	} else if ( nFormat & 0x0031 ) {
		sSubType = "24kbs G723 ADPCM encoding";
	} else if ( nFormat & 0x0032 ) {
		sSubType = "40kbs G723 ADPCM encoding";
	} else if ( nFormat & 0x0040 ) {
		sSubType = "12 bit Delta Width Variable Word encoding";
	} else if ( nFormat & 0x0041 ) {
		sSubType = "16 bit Delta Width Variable Word encoding";
	} else if ( nFormat & 0x0042 ) {
		sSubType = "24 bit Delta Width Variable Word encoding";
	} else if ( nFormat & 0x0043 ) {
		sSubType = "N bit Delta Width Variable Word encoding";
	} else if ( nFormat & 0x0050 ) {
		sSubType = "8 bit differential PCM (XI only)";
	} else if ( nFormat & 0x0051 ) {
		sSubType = "16 bit differential PCM (XI only)";
	} else if ( nFormat & 0x0060 ) {
		sSubType = "Xiph Vorbis encoding";
	} else if ( nFormat & 0x0064 ) {
		sSubType = "Xiph/Skype Opus encoding";
	} else if ( nFormat & 0x0070 ) {
		sSubType = "Apple Lossless Audio Codec (16 bit)";
	} else if ( nFormat & 0x0071 ) {
		sSubType = "Apple Lossless Audio Codec (20 bit)";
	} else if ( nFormat & 0x0072 ) {
		sSubType = "Apple Lossless Audio Codec (24 bit)";
	} else if ( nFormat & 0x0073 ) {
		sSubType = "Apple Lossless Audio Codec (32 bit)";
	} else if ( nFormat & 0x0080 ) {
		sSubType = "MPEG-1 Audio Layer I";
	} else if ( nFormat & 0x0081 ) {
		sSubType = "MPEG-1 Audio Layer II";
	} else if ( nFormat & 0x0082 ) {
		sSubType = "MPEG-2 Audio Layer III";
	} else {
		INFOLOG( QString( "Unknown subtype [%1]" ).arg( nFormat ) );
	}

	QString sEndianness;
	if ( nFormat & 0x00000000 ) {
		sEndianness = "Default file endian-ness";
	} else if ( nFormat & 0x10000000 ) {
		sEndianness = "Force little endian-ness";
	} else if ( nFormat & 0x20000000 ) {
		sEndianness = "Force big endian-ness";
	} else if ( nFormat & 0x30000000 ) {
		sEndianness = "Force CPU endian-ness";
	}
	
	if ( ! sSubType.isEmpty() ) {
		sFormat.append( QString( " - %1" ).arg( sSubType ) );
	}
	if ( ! sEndianness.isEmpty() ) {
		sFormat.append( QString( " - %1" ).arg( sEndianness ) );
	}

	return sFormat;
}

#ifdef H2CORE_HAVE_RUBBERBAND
static double compute_pitch_scale( const Sample::Rubberband& rb )
{
	return Note::pitchToFrequency( rb.pitch );
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

	if ( Preferences::get_instance()->getRubberBandBatchMode() ) {
		options |= RubberBand::RubberBandStretcher::OptionProcessRealTime;
	} else {
		options |= RubberBand::RubberBandStretcher::OptionProcessOffline;
	}

	if ( !lamination ) options |= RubberBand::RubberBandStretcher::OptionPhaseIndependent;
	if ( longwin )     options |= RubberBand::RubberBandStretcher::OptionWindowLong;
	if ( shortwin )    options |= RubberBand::RubberBandStretcher::OptionWindowShort;
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
