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
		 * Load the sample stored in #m_sFilepath into
		 * #m_data_L and #m_data_R.
		 *
		 * It uses libsndfile for reading both the content and
		 * the metadata of the sample file. The latter is
		 * stored in #m_nFrames and #m_nSampleRate.
		 *
		 * Hydrogen does only support up to #SAMPLE_CHANNELS
		 * (two per default) channels in the audio file. If
		 * there are more, Hydrogen will _NOT_ downmix its
		 * content but simply extract the first two channels
		 * and display a warning message. For mono file the
		 * same content will be assigned to both the left
		 * (#m_data_L) and right channel (#m_data_R).
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
		const QString& getFilepath() const;
		/** \return Filename part of #m_sFilepath */
		void setFilepath( const QString& sPath );
		QString getFilename() const;
		/** \param filename Filename part of #m_sFilepath*/
		void setFilename( const QString& filename );

		/** \return #m_nFrames accessor */
		int getFrames() const;
		/** \return #m_nSampleRate */
		int getSampleRate() const;

		/** \return data size, which is calculated by
		 * #m_nFrames time sizeof( float ) * 2
		 */
		int getSize() const;
		/** \return #m_data_L*/
		float* getData_L() const;
		/** \return #m_data_R*/
		float* getData_R() const;
		/**
		 * #m_bIsModified setter
		 * \param value the new value for #m_bIsModified
		 */
		void setIsModified( bool value );
		/** \return #m_bIsModified */
		bool getIsModified() const;
		/** \return #m_panEnvelope */
		const PanEnvelope& getPanEnvelope() const;
		/** \return #m_velocityEnvelope */
		const VelocityEnvelope& getVelocityEnvelope() const;
		/** \return #m_loops parameters */
		const Loops& getLoops() const;
		/** \return #m_rubberband parameters */
		const Rubberband& getRubberband() const;
	void setPanEnvelope( const PanEnvelope& envelope );
	void setVelocityEnvelope( const VelocityEnvelope& envelope );
	void setLoops( const Loops& loops );
	void setRubberband( const Rubberband& rubberband );

	const License& getLicense() const;
	void setLicense( const License& license );
	
		/**
		 * parse the given string and rturn the corresponding loop_mode
		 * \param string the loop mode text to be parsed
		 */
		static Loops::LoopMode parseLoopMode( const QString& string );
		/** \return mode member of #m_loops as a string */
		QString getLoopModeString() const;
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
		/** \return sample duration in seconds */
		double getSampleDuration() const;

		/**
		 * apply #m_loops transformation to the sample
		 */
		bool applyLoops();
		/**
		 * apply #m_velocityEnvelope transformation to the sample
		 */
		void applyVelocity();
		/**
		 * apply #m_panEnvelope transformation to the sample
		 * \param p the pan vector
		 */
		void applyPan();
		/**
		 * apply #m_rubberband transformation to the sample
		 * \param fBpm tempo the Rubberband transformation will target
		 */
		void applyRubberband( float fBpm );
		/**
		 * call rubberband cli to modify the sample using #m_rubberband
		 * \param fBpm tempo the Rubberband transformation will target
		 */
		bool execRubberbandCli( float fBpm );

		/** Convenience variable not written to disk. */
		bool				m_bIsLoaded;
		QString				m_sFilepath;          ///< filepath of the sample
		int					m_nFrames;            ///< number of frames in this sample
		int					m_nSampleRate;       ///< samplerate for this sample
		float*				m_data_L;            ///< left channel data
		float*				m_data_R;            ///< right channel data
		bool				m_bIsModified;       ///< true if sample is modified
		PanEnvelope			m_panEnvelope;      ///< pan envelope vector
		VelocityEnvelope	m_velocityEnvelope; ///< velocity envelope vector
		Loops				m_loops;             ///< set of loop parameters
		Rubberband			m_rubberband;        ///< set of rubberband parameters
		/** loop modes string */
		static const std::vector<QString> m_loopModes;

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

inline const QString& Sample::getFilepath() const {
	return m_sFilepath;
}

inline void Sample::setFilepath( const QString& sPath ) {
	m_sFilepath = sPath;
}

inline QString Sample::getFilename() const
{
	return m_sFilepath.section( "/", -1 );
}

inline int Sample::getFrames() const
{
	return m_nFrames;
}

inline int Sample::getSampleRate() const
{
	return m_nSampleRate;
}

inline double Sample::getSampleDuration() const
{
	return static_cast<double>(m_nFrames) / static_cast<double>(m_nSampleRate);
}

inline int Sample::getSize() const
{
	return m_nFrames * sizeof( float ) * 2;
}

inline float* Sample::getData_L() const
{
	return m_data_L;
}

inline float* Sample::getData_R() const
{
	return m_data_R;
}

inline void Sample::setIsModified( bool is_modified )
{
	m_bIsModified = is_modified;
}

inline bool Sample::getIsModified() const
{
	return m_bIsModified;
}

inline QString Sample::getLoopModeString() const
{
	return std::move( m_loopModes.at(m_loops.mode) );
}

inline const Sample::PanEnvelope& Sample::getPanEnvelope() const
{
	return m_panEnvelope;
}

inline const Sample::VelocityEnvelope& Sample::getVelocityEnvelope() const
{
	return m_velocityEnvelope;
}

inline const Sample::Loops& Sample::getLoops() const
{
	return m_loops;
}

inline const Sample::Rubberband& Sample::getRubberband() const
{
	return m_rubberband;
}
inline void Sample::setPanEnvelope( const PanEnvelope& envelope ) {
	m_panEnvelope = envelope;
}
inline void Sample::setVelocityEnvelope( const VelocityEnvelope& envelope ) {
	m_velocityEnvelope = envelope;
}
inline void Sample::setLoops( const Loops& loops ) {
	m_loops = loops;
}
inline void Sample::setRubberband( const Rubberband& rubberband ) {
	m_rubberband = rubberband;
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
