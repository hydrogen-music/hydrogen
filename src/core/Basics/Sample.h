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

#ifndef H2C_SAMPLE_H
#define H2C_SAMPLE_H

#include <memory>
#include <vector>
#include <sndfile.h>

#include <core/License.h>
#include <core/Object.h>

namespace H2Core
{

/**
 * A container for a sample, being able to apply modifications on it
 */

/** an envelope point within a frame */
/** \ingroup docCore docAudioDriver */
class EnvelopePoint : public H2Core::Object<EnvelopePoint>
{
		H2_OBJECT(EnvelopePoint)
	public:
		int frame;  ///< frame index
		int value;  ///< value
		/** to be able to sort velocity points vectors */
		struct Comparator {
			bool operator()( const EnvelopePoint& a, const EnvelopePoint& b ) const
			{
				return a.frame < b.frame;
			}
		};
		/** default constructor */
		EnvelopePoint();
		/**
		 * constructor
		 * \param f the frame index
		 * \param v the value associated with the frame
		 */
		EnvelopePoint( int f, int v );
		/** copy constructor */
		EnvelopePoint( const EnvelopePoint& other );
};

class Sample : public H2Core::Object<Sample>
{
		H2_OBJECT(Sample)
	public:

		/** define the type used to store pan envelope points */
		using PanEnvelope = std::vector<EnvelopePoint>;
		/** define the type used to store velocity envelope points */
		using VelocityEnvelope = std::vector<EnvelopePoint>;
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
				QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
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
				QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
		};

	static QString sndfileFormatToQString( int nFormat );

		/**
		 * Sample constructor
		 * \param filepath the path to the sample
		 * \param license associated with the sample
		 * \param frames the number of frames per channel in the sample
		 * \param sample_rate the sample rate of the sample
		 * \param data_l the left channel array of data
		 * \param data_r the right channel array of data
		 */
		Sample( const QString& filepath, const License& license = License(), int frames=0, int sample_rate=0, float* data_l=nullptr, float* data_r=nullptr );
		/** copy constructor */
		Sample( std::shared_ptr<Sample> other );
		/** destructor */
		~Sample();


		/**
		 * write sample to a file
		 * \param path the path to write the sample to
		 * \param format the format of the output
		 */
		bool write( const QString& path,
					int format= ( SF_FORMAT_WAV|SF_FORMAT_PCM_16 ) ) const;
	
		/**
		 * Load a sample from a file.
		 *
		 * This function checks whether the @a filepath is
		 * readable, initializes a new Sample, and calls the
		 * load() member on it.
		 *
		 * \param filepath the file to load audio data from
		 * \param license associated with the sample
		 *
		 * \return Pointer to the newly initialized Sample. If
		 * the provided @a filepath is not readable, a nullptr
		 * is returned instead.
		 *
		 * \fn load(const QString& filepath)
		 */
	static std::shared_ptr<Sample> load( const QString& filepath, const License& license = License() );

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
		 * After successfully loading, the function applies all loop,
		 * rubberband, and envelope modifications in case they were
		 * set by the user.
		 *
		 * \fn load()
		 */
		bool load( float fBpm = 120 );
		/**
		 * Flush the current content of the left and right
		 * channel and the current metadata.
		 */
		void unload();

		/** \return true if the associated sample file was loaded */
		bool isLoaded() const;
		const QString& get_filepath() const;
		/** \return Filename part of #__filepath */
		QString get_filename() const;
		/** \param filename Filename part of #__filepath*/
		void set_filename( const QString& filename );
		/** \param filename sets #__filepath*/
		void set_filepath( const QString& sFilepath );
		/**
		 * #__frames setter
		 * \param value the new value for #__frames
		 */
		void set_frames( int value );
		/** \return #__frames accessor */
		int get_frames() const;
		/**
		 * \param sampleRate Sets #__sample_rate.
		 */
		void set_sample_rate( const int sampleRate );
		/** \return #__sample_rate */
		int get_sample_rate() const;
		/** \return sample duration in seconds */
		double get_sample_duration( ) const;
	
		/** \return data size, which is calculated by
		 * #__frames time sizeof( float ) * 2 
		 */
		int get_size() const;
		/** \return #__data_l*/
		float* get_data_l() const;
		/** \return #__data_r*/
		float* get_data_r() const;
		/**
		 * #__is_modified setter
		 * \param value the new value for #__is_modified
		 */
		void set_is_modified( bool value );
		/** \return #__is_modified */
		bool get_is_modified() const;
		/** \return #__pan_envelope */
		const PanEnvelope& get_pan_envelope() const;
		/** \return #__velocity_envelope */
		const VelocityEnvelope& get_velocity_envelope() const;
		/** \return #__loops parameters */
		const Loops& get_loops() const;
		/** \return #__rubberband parameters */
		const Rubberband& get_rubberband() const;
	void set_pan_envelope( const PanEnvelope& envelope );
	void set_velocity_envelope( const VelocityEnvelope& envelope );
	void set_loops( const Loops& loops );
	void set_rubberband( const Rubberband& rubberband );

	const License& getLicense() const;
	void setLicense( const License& license );
	
		/**
		 * parse the given string and rturn the corresponding loop_mode
		 * \param string the loop mode text to be parsed
		 */
		static Loops::LoopMode parse_loop_mode( const QString& string );
		/** \return mode member of #__loops as a string */
		QString get_loop_mode_string() const;
		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
	private:
		/**
		 * apply #__loops transformation to the sample
		 */
		bool apply_loops();
		/**
		 * apply #__velocity_envelope transformation to the sample
		 */
		void apply_velocity();
		/**
		 * apply #__pan_envelope transformation to the sample
		 * \param p the pan vector
		 */
		void apply_pan();
		/**
		 * apply #__rubberband transformation to the sample
		 * \param fBpm tempo the Rubberband transformation will target
		 */
		void apply_rubberband( float fBpm );
		/**
		 * call rubberband cli to modify the sample using #__rubberband
		 * \param fBpm tempo the Rubberband transformation will target
		 */
		bool exec_rubberband_cli( float fBpm );

		/** Convenience variable not written to disk. */
		bool				m_bIsLoaded;
		QString				__filepath;          ///< filepath of the sample
		int					__frames;            ///< number of frames in this sample
		int					__sample_rate;       ///< samplerate for this sample
		float*				__data_l;            ///< left channel data
		float*				__data_r;            ///< right channel data
		bool				__is_modified;       ///< true if sample is modified
		PanEnvelope			__pan_envelope;      ///< pan envelope vector
		VelocityEnvelope	__velocity_envelope; ///< velocity envelope vector
		Loops				__loops;             ///< set of loop parameters
		Rubberband			__rubberband;        ///< set of rubberband parameters
		/** loop modes string */
		static const std::vector<QString> __loop_modes;

	/** Transient property indicating the license associated with the
	 * sample.
	 *
	 * This variable is not stored on disk but either derived from the
	 * license of the drumkit containing the sample or specified by
	 * the user when loading the it directly.
	 *
	 * It's value is only important for samples associated with a
	 * drumkit (stored in the InstrumentLayers of a kit). For "free"
	 * ones, like metronome or sound feedback when inserting notes in
	 * the Pattern Editor, it does not have to be specified.
	 */
	License m_license;
};

// DEFINITIONS

inline bool Sample::isLoaded() const {
	return m_bIsLoaded;
}

inline void Sample::set_filepath( const QString& sFilepath )
{
	__filepath = sFilepath;
}

inline QString Sample::get_filename() const
{
	return __filepath.section( "/", -1 );
}

inline void Sample::set_frames( int frames )
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
	return static_cast<double>(__frames) / static_cast<double>(__sample_rate);
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
	return std::move( __loop_modes.at(__loops.mode) );
}

inline const Sample::PanEnvelope& Sample::get_pan_envelope() const
{
	return __pan_envelope;
}

inline const Sample::VelocityEnvelope& Sample::get_velocity_envelope() const
{
	return __velocity_envelope;
}

inline const Sample::Loops& Sample::get_loops() const
{
	return __loops;
}

inline const Sample::Rubberband& Sample::get_rubberband() const
{
	return __rubberband;
}
inline void Sample::set_pan_envelope( const PanEnvelope& envelope ) {
	__pan_envelope = envelope;
}
inline void Sample::set_velocity_envelope( const VelocityEnvelope& envelope ) {
	__velocity_envelope = envelope;
}
inline void Sample::set_loops( const Loops& loops ) {
	__loops = loops;
}
inline void Sample::set_rubberband( const Rubberband& rubberband ) {
	__rubberband = rubberband;
}

inline const License& Sample::getLicense() const {
	return m_license;
}
inline void Sample::setLicense( const License& license ) {
	m_license = license;
}

};

#endif // H2C_SAMPLE_H

/* vim: set softtabstop=4 noexpandtab: */
