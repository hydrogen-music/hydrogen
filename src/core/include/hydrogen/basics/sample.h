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

#ifndef H2C_SAMPLE_H
#define H2C_SAMPLE_H

#include <vector>
#include <sndfile.h>

#include <hydrogen/object.h>

namespace H2Core
{

/**
 * A container for a sample, beeing able to apply modifications on it
 */
class Sample : public H2Core::Object
{
		H2_OBJECT
	public:
		/** an envelope point within a frame */
		class EnvelopePoint
		{
			public:
				int frame;  ///< frame index
				int value;  ///< value
				/** to be able to sort velocity points vectors */
				struct Comparator {
					bool operator()( Sample::EnvelopePoint const& a, Sample::EnvelopePoint const& b )
					{
						return a.frame < b.frame;
					}
				};
				/** default constructor */
				EnvelopePoint() : frame( 0 ), value( 0 ) { };
				/**
				 * constructor
				 * \param f the frame index
				 * \param v the value associated with the frame
				 */
				EnvelopePoint( int f, int v ) : frame( f ), value( v ) { };
				/** copy constructor */
				EnvelopePoint( EnvelopePoint* other )
				{
					frame = other->frame;
					value = other->value;
				};
		};
		/** define the type used to store pan enveloppe points */
		typedef std::vector<EnvelopePoint> PanEnvelope;
		/** define the type used to store velocity enveloppe points */
		typedef std::vector<EnvelopePoint> VelocityEnvelope;
		/** set of loop configuration flags */
		class Loops
		{
			public:
				/** possible sample editing loop mode */
				enum LoopMode {
					FORWARD=0,
					REVERSE,
					PINGPONG
				};
				int start_frame;        ///< the frame index where to start the new sample from
				int loop_frame;         ///< the frame index where to start the loop from
				int end_frame;          ///< the frame index where to end the new sample to
				int count;              ///< the counts of loops to apply
				LoopMode mode;          ///< one of the possible loop modes
				/** constructor */
				Loops() : start_frame( 0 ), loop_frame( 0 ), end_frame( 0 ), count( 0 ), mode( FORWARD ) { };
				/** copy constructor */
				Loops( const Loops* other ) :
					start_frame( other->start_frame ),
					loop_frame( other->loop_frame ),
					end_frame( other->end_frame ),
					count( other->count ),
					mode( other->mode ) { };
				/** equal to operator */
				bool operator ==( const Loops& b ) const
				{
					return ( start_frame==b.start_frame && loop_frame==b.loop_frame && end_frame==b.end_frame && count==b.count && mode==b.mode );
				}
		};

		/** set of rubberband configuration flags */
		class Rubberband
		{
			public:
				bool use;               ///< is rubberband enabled
				float divider;          ///< TODO should be ratio : desired time ratio
				float pitch;            ///< desired pitch
				int c_settings;        ///< TODO should be crispness, see rubberband -h
				/** constructor */
				Rubberband() : use( false ), divider ( 1.0 ), pitch( 1.0 ), c_settings( 4 ) { };
				/** copy constructor */
				Rubberband( const Rubberband* other ) :
					use( other->use ),
					divider ( other->divider ),
					c_settings( other->c_settings ),
					pitch( other->pitch ) { };
				/** equal to operator */
				bool operator ==( const Rubberband& b ) const
				{
					return ( use==b.use && divider==b.divider && c_settings==b.c_settings && pitch==b.pitch );
				}
		};

		/**
		 * Sample constructor
		 * \param filepath the path to the sample
		 * \param frames the number of frames per channel in the sample
		 * \param sample_rate the sample rate of the sample
		 * \param data_l the left channel array of data
		 * \param data_r the right channel array of data
		 */
		Sample( const QString& filepath, int frames=0, int sample_rate=0, float* data_l=0, float* data_r=0 );
		/** copy constructor */
		Sample( Sample* other );
		/** destructor */
		~Sample();


		/**
		 * write sample to a file
		 * \param path the path to write the sample to
		 * \param format the format of the output
		 */
		bool write( const QString& path, int format= ( SF_FORMAT_WAV|SF_FORMAT_PCM_16 ) );

		/**
		 * Load a sample from a file.
		 *
		 * This function checks whether the @a filepath is
		 * readable, initializes a new Sample, and calls the
		 * load( const bool checkSampleRate ) member on it.
		 *
		 * \param filepath the file to load audio data from
		 *
		 * \param checkSampleRate If set to true, the sample
		 * rate of the loaded file will be compared to the
		 * one used by the audio driver and a warning will be
		 * displayed in case they do not match.
		 *
		 * \return Pointer to the newly initialized Sample. If
		 * the provided @a filepath is not readable, a nullptr
		 * is returned instead.
		 *
		 * \fn load( const QString& filepath, const bool checkSampleRate )
		 */
	        static Sample* load( const QString& filepath, const bool checkSampleRate );
	
		/**
		 * Load a sample from a file and apply the
		 * transformations to the sample data.
		 *
		 * Wrapper around load( const QString& filepath, const
		 * bool checkSampleRate ), which calls apply() with @a
		 * loops, @a rubber, @a velocity, and @a pan as
		 * arguments after successfully loading the sample.
		 *
		 * \param filepath the file to load audio data from
		 * \param loops transformation parameters
		 * \param rubber band transformation parameters
		 * \param velocity envelope points
		 * \param pan envelope points
		 *
		 * \return Pointer to the newly initialized Sample. If
		 * the provided @a filepath is not readable, a nullptr
		 * is returned instead.
		 *
		 * \fn load( const QString& filepath, const Loops& loops, const Rubberband& rubber, const VelocityEnvelope& velocity, const PanEnvelope& pan, const bool checkSampleRate )
		 */
                static Sample* load( const QString& filepath, const Loops& loops, const Rubberband& rubber, const VelocityEnvelope& velocity, const PanEnvelope& pan, const bool checkSampleRate );

		/**
		 * Load the sample stored in #__filepath into
		 * #__data_l and #__data_r.
		 *
		 * It uses libsndfile for reading both the content and
		 * the metadata of the sample file. The latter is
		 * stored in #__frames and #__sample_rate.
		 *
		 * Hydrogen does only support up to #SAMPLE_CHANNELS
		 * (two per default) channels in the audio file. If
		 * there are more, Hydrogen will _NOT_ downmix its
		 * content but simply extract the first two channels
		 * and display a warning message. For mono file the
		 * same content will be assigned to both the left
		 * (#__data_l) and right channel (#__data_r).
		 *
		 * If the total number of frames in the file is larger
		 * than the maximum value of an `int', the content is
		 * truncated and a warning log message will be
		 * displayed.
		 *
		 * \param checkSampleRate If set to true, the sample
		 * rate of the loaded file will be compared to the
		 * one used by the audio driver and a warning will be
		 * displayed in case they do not match.
		 *
		 * \fn load( const bool checkSampleRate )
		 */
		void load( const bool checkSampleRate );
		/**
		 * Flush the current content of the left and right
		 * channel and the current metadata.
		 */
		void unload();

		/**
		 * Apply transformations to the sample data.
		 *
		 * The function is a wrapper around a specific apply_*
		 * functions.
		 *
		 * \param loops Loops transformation parameters handed
		 * over to apply_loops().
		 * \param rubber Rubber Band transformation parameters
		 * handed over to apply_rubberband() in case Hydrogen
		 * was compiled to use the Rubber Band library for
		 * audio time-stretching and pitch-shifting
		 * (#H2CORE_HAVE_RUBBERBAND) or exec_rubberband_cli()
		 * if is wasn't.
		 * \param velocity Velocity envelope points handed
		 * over to apply_velocity().
		 * \param pan Pan envelope points handed over to
		 * apply_pan().
		 */
		void apply( const Loops& loops, const Rubberband& rubber, const VelocityEnvelope& velocity, const PanEnvelope& pan );
		/**
		 * aplly loop transformation to the sample
		 * \param lo loops parameters
		 */
		bool apply_loops( const Loops& lo );
		/**
		 * aplly velocity transformation to the sample
		 * \param v the velocity vector
		 */
		void apply_velocity( const VelocityEnvelope& v );
		/**
		 * aplly velocity transformation to the sample
		 * \param p the pan vector
		 */
		void apply_pan( const PanEnvelope& p );
		/**
		 * aplly rubberband transformation to the sample
		 * \param rb rubberband parameters
		 */
		void apply_rubberband( const Rubberband& rb );
		/**
		 * call rubberband cli to modify the sample
		 * \param rb rubberband parameters
		 */
		bool exec_rubberband_cli( const Rubberband& rb );

		/** \return true if both data channels are null pointers */
		bool is_empty() const;
		/** __filepath accessor */
		const QString get_filepath() const;
		/** return filename part of __filepath */
		const QString get_filename() const;
		/** set the filename part of __filepath*/
		void set_filename( const QString& filename );
		/**
		 * __frames setter
		 * \param value the new value for __frames
		 */
		void set_frames( int value );
		/** \return __frames accessor */
		int get_frames() const;
		/**
		 * __sample_rate setter
		 * \param value the new value for __sample_rate
		 */
		void set_sample_rate( const int sampleRate );
		/** \return __sample_rate accessor */
		int get_sample_rate() const;
		/** \return sample duration in seconds */
		double get_sample_duration( ) const;

		/** \return data size, which is calculated by
		 * #__frames time sizeof( float ) * 2 
		 */
		int get_size() const;
		/** \return __data_l accessor */
		float* get_data_l() const;
		/** \return __data_r accessor */
		float* get_data_r() const;
		/**
		 * __is_modified setter
		 * \param value the new value for __is_modified
		 */
		void set_is_modified( bool value );
		/** \return __is_modified accessor */
		bool get_is_modified() const;
		/** \return __pan_envelope accessor */
		PanEnvelope* get_pan_envelope();
		/** \return __velocity_envelope accessor */
		VelocityEnvelope* get_velocity_envelope();
		/** \return __loops parameters accessor */
		Loops get_loops() const;
		/** \return __rubberband parameters accessor */
		Rubberband get_rubberband() const;
		/**
		 * parse the given string and rturn the corresponding lopp_mode
		 * \param string the loop mode text to be parsed
		 */
		static Loops::LoopMode parse_loop_mode( const QString& string );
		/** \return __loops.mode as a string */
		QString get_loop_mode_string() const;

	private:
		QString __filepath;                     ///< filepath of the sample
		int __frames;                           ///< number of frames in this sample
		int __sample_rate;                      ///< samplerate for this sample
		float* __data_l;                        ///< left channel data
		float* __data_r;                        ///< right channel data
		bool __is_modified;                     ///< true if sample is modified
		PanEnvelope __pan_envelope;             ///< pan envelope vector
		VelocityEnvelope __velocity_envelope;   ///< velocity envelope vector
		Loops __loops;                          ///< set of loop parameters
		Rubberband __rubberband;                ///< set of rubberband parameters
		/** loop modes string */
		static const char* __loop_modes[];
};

// DEFINITIONS

inline void Sample::unload()
{
	if( __data_l ) delete __data_l;
	if( __data_r ) delete __data_r;
	__frames = __sample_rate = 0;
	__data_l = __data_r = 0;
	// __is_modified = false; leave this unchanged as pan, velocity, loop and rubberband are kept unchanged
}

inline bool Sample::is_empty() const
{
	return ( __data_l==__data_r==0 );
}

inline const QString Sample::get_filepath() const
{
	return __filepath;
}

inline const QString Sample::get_filename() const
{
	return __filepath.section( "/", -1 );
}

inline void Sample::Sample::set_frames( int frames )
{
	__frames = frames;
}

inline int Sample::get_frames() const
{
	return __frames;
}

inline int Sample::get_sample_rate() const
{
	return __sample_rate;
}

inline void Sample::set_sample_rate( const int sampleRate )
{
	__sample_rate = sampleRate;
}

inline double Sample::get_sample_duration() const
{
	return ( double )__frames / ( double )__sample_rate;
}

inline int Sample::get_size() const
{
	return __frames * sizeof( float ) * 2;
}

inline float* Sample::get_data_l() const
{
	return __data_l;
}

inline float* Sample::get_data_r() const
{
	return __data_r;
}

inline void Sample::set_is_modified( bool is_modified )
{
	__is_modified = is_modified;
}

inline bool Sample::get_is_modified() const
{
	return __is_modified;
}

inline QString Sample::get_loop_mode_string() const
{
	return __loop_modes[__loops.mode];
}

inline Sample::PanEnvelope* Sample::get_pan_envelope()
{
	return &__pan_envelope;
}

inline Sample::VelocityEnvelope* Sample::get_velocity_envelope()
{
	return &__velocity_envelope;
}

inline Sample::Loops Sample::get_loops() const
{
	return __loops;
}

inline Sample::Rubberband Sample::get_rubberband() const
{
	return __rubberband;
}

};

#endif // H2C_SAMPLE_H

/* vim: set softtabstop=4 noexpandtab: */
