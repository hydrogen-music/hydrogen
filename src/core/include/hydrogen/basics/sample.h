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
#include <QtCore/QString>

#include <hydrogen/object.h>

namespace H2Core {

class SampleVeloPan
{
public:
	struct SampleVeloVector
	{
		int m_SampleVeloframe;
		int m_SampleVelovalue;
	};

	std::vector<SampleVeloVector> m_Samplevolumen;

	struct SamplePanVector
	{
		int m_SamplePanframe;
		int m_SamplePanvalue;
	};

	std::vector<SamplePanVector> m_SamplePan;

	SampleVeloPan() {
		SampleVeloVector velovector;
		velovector.m_SampleVeloframe = -1;
		velovector.m_SampleVelovalue = -1;
		m_Samplevolumen.push_back( velovector );
		SamplePanVector panvector;
		panvector.m_SamplePanframe = -1;	
		panvector.m_SamplePanvalue = -1;
		m_SamplePan.push_back( panvector );
	}

	SampleVeloPan( const SampleVeloPan& velopan ) {
		m_Samplevolumen = velopan.m_Samplevolumen;
		m_SamplePan = velopan.m_SamplePan;
	}

};

/**
 * A container for a sample, beeing able to apply modifications on it
 */
class Sample : public Object {
        H2_OBJECT
    public:
        /** possible sample editing loop mode */
        enum LoopMode {
            FORWARD=0,
            REVERSE,
            PINGPONG
        };

        /** set of loop options */
        class LoopOptions {
            public:
                int start_frame;        ///< the frame index where to start the new sample from
                int loop_frame;         ///< the frame index where to start the loop from
                int end_frame;          ///< the frame index where to end the new sample to
                int loops;              ///< the counts of loops to apply
                LoopMode mode;          ///< one of the possible loop modes
                /** constructor */
                LoopOptions() {
                    start_frame = loop_frame = end_frame = loops = 0;
                    mode = FORWARD;
                };
                /** copy constructor */
                LoopOptions( const LoopOptions* other) {
                    start_frame = other->start_frame;
                    loop_frame = other->loop_frame;
                    end_frame = other->end_frame;
                    loops = other->loops;
                    mode = other->mode;
                };
        };

        /** set of rubberband options */
        class RubberBandOptions {
            public:
                bool use;               ///< TODO
                float divider;          ///< TODO
                int c_settings;         ///< TODO
                float pitch;            ///< TODO
                /** constructor */
                RubberBandOptions() {
                    use = false;
                    divider = 1.0;
                    c_settings = 4;
                    pitch = 0.0;
                };
                /** copy constructor */
                RubberBandOptions( const RubberBandOptions* other) {
                    use = other->use;
                    divider = other->divider;
                    c_settings = other->c_settings;
                    pitch = other->pitch;
                };
        };

        /**
         * Sample constructor
         * \param filename the sample file name
         * \param frames the number of frames per channel in the sample
         * \param sample_rate the sample rate of the sample
         * \param data_l the left channel array of data
         * \param data_l the right channel array of data
         */
        Sample( const QString& filename, int frames, int sample_rate, float* data_l=0, float* data_r=0 );
        /** copy constructor */
        Sample( Sample* other );
        /** destructor */
        ~Sample();

        /**
         * load a sample from a file
         * \param filepath the file to load audio data from
         */
        static Sample* load( const QString& filepath );

        static Sample* load_edit_sndfile( const QString& filepath,
                                   const unsigned startframe,
                                   const unsigned loopframe,
                                   const unsigned endframe,
                                   const int loops,
                                   const QString loopmode,
                                   bool use_rubberband,
                                   float rubber_divider,
                                   int rubberbandCsettings,
                                   float rubber_pitch);

        /** return true if both data channels are null pointers */
        bool is_empty() const;
        /** __filename accessor */
        const QString get_filename() const;
        /**
         * __frames setter
         * \parama value the new value for __frames
         */
        void set_frames( int value );
        /** __frames accessor */
        int get_frames() const;
        /**
         * __sample_rate setter
         * \parama value the new value for __sample_rate
         */
        void set_sample_rate( int value );
        /** __sample_rate accessor */
        int get_sample_rate() const;

        /** return data size */
        int get_size() const;
        /** __data_l accessor */
        float* get_data_l() const;
        /** __data_r accessor */
        float* get_data_r() const;
        /**
         * __is_modified setter
         * \parama value the new value for __is_modified
         */
        void set_is_modified( bool value );
        /** __is_modified accessor */
        bool get_is_modified() const;
        /**
         * __loop_options.mode setter
         * \parama value the new value for __loop_options.mode
         */
        void set_loop_mode( LoopMode value );
        /** __loop_options.mode accessor */
        LoopMode get_loop_mode() const;
        /** return __loop_options.mode as a string */
        QString get_loop_mode_string() const;
        /**
         * parse the given string and rturn the corresponding lopp_mode
         * \param string the loop mode text to be parsed
         */
        static LoopMode parse_loop_mode( const QString& string );
        /**
         * __loop_options.start_frame setter
         * \parama value the new value for __loop_options.start_frame
         */
        void set_start_frame( int value );
        /** __loop_options.start_frame accessor */
        int get_start_frame() const;
        /**
         * __loop_options.end_frame setter
         * \parama value the new value for __loop_options.end_frame
         */
        void set_end_frame( int value );
        /** __loop_options.end_frame accessor */
        int get_end_frame() const;
        /**
         * __loop_options.loop_frame setter
         * \parama value the new value for __loop_options.loop_frame
         */
        void set_loop_frame( int value );
        /** __loop_options.loop_frame accessor */
        int get_loop_frame() const;
        /**
         * __loop_options.loops setter
         * \parama value the new value for __loop_options.loops
         */
        void set_loops( int value );
        /** __loop_options.loops accessor */
        int get_loops() const;
        /**
         * __rubber_options.use setter
         * \parama value the new value for __rubber_options.use
         */
        void set_use_rubber( bool value );
        /** __rubber_options.use accessor */
        bool get_use_rubber() const;
        /**
         * __rubber_options.pitch setter
         * \parama value the new value for __rubber_options.pitch
         */
        void set_rubber_pitch( float value );
        /** __rubber_options.pitch accessor */
        float get_rubber_pitch() const;
        /**
         * __rubber_options.divider setter
         * \parama value the new value for __rubber_options.divider
         */
        void set_rubber_divider( float value );
        /** __rubber_options.divider accessor */
        float get_rubber_divider() const;
        /**
         * __rubber_options.c_settings setter
         * \parama value the new value for __rubber_options.c_settings
         */
        void set_rubber_c_settings( int value );
        /** __rubber_options.c_settings accessor */
        float get_rubber_c_settings() const;

    private:
        QString __filename;                     ///< filename of the sample, no path information
        int __frames;                           ///< number of frames in this sample
        int __sample_rate;                      ///< samplerate for this sample
        float* __data_l;                        ///< left channel data
        float* __data_r;                        ///< right channel data
        bool __is_modified;                     ///< true if sample is modified
    public:
	    SampleVeloPan __velo_pan;	///< volume and pan vector
    private:
        LoopOptions __loop_options;             ///< set of loop options
        RubberBandOptions __rubber_options;     ///< set of rubberband options
        /** loop modes string */
        static const char* __loop_modes[];
        /**
         * load sample data using libsndfile
         * \param filepath the file to load audio data from
         */
        static Sample* libsndfile_load( const QString& filepath );
};

// DEFINITIONS

inline bool Sample::is_empty() const {
    return ( __data_l==__data_r==0 );
}

inline const QString Sample::get_filename() const {
    return __filename;
}

inline void Sample::Sample::set_frames( int frames ) {
    __frames = frames;
}

inline int Sample::get_frames() const {
    return __frames;
}

inline int Sample::get_sample_rate() const {
    return __sample_rate;
}

inline int Sample::get_size() const {
    return __frames * sizeof( float ) * 2;
}

inline float* Sample::get_data_l() const {
    return __data_l;
}

inline float* Sample::get_data_r() const {
    return __data_r;
}

inline void Sample::set_is_modified( bool is_modified ) {
    __is_modified = is_modified;
}

inline bool Sample::get_is_modified() const {
    return __is_modified;
}

inline void Sample::set_loop_mode( Sample::LoopMode loop_mode ) {
    __loop_options.mode = loop_mode;
}

inline Sample::LoopMode Sample::get_loop_mode() const {
    return __loop_options.mode;
}

inline QString Sample::get_loop_mode_string() const {
    return __loop_modes[__loop_options.mode];
}

inline void Sample::set_start_frame( int start_frame ) {
    __loop_options.start_frame = start_frame;
}

inline int Sample::get_start_frame() const {
    return __loop_options.start_frame;
}

inline void Sample::set_end_frame( int end_frame ) {
    __loop_options.end_frame = end_frame;
}

inline int Sample::get_end_frame() const {
    return __loop_options.end_frame;
}

inline void Sample::set_loop_frame( int loop_frame ) {
    __loop_options.loop_frame = loop_frame;
}

inline int Sample::get_loop_frame() const {
    return __loop_options.loop_frame;
}

inline void Sample::set_loops( int loops ) {
    __loop_options.loops = loops;
}

inline int Sample::get_loops() const {
    return __loop_options.loops;
}

inline void Sample::set_use_rubber( bool use_rubber ) {
    __rubber_options.use = use_rubber;
}

inline bool Sample::get_use_rubber() const {
    return __rubber_options.use;
}

inline void Sample::set_rubber_pitch( float rubber_pitch ) {
    __rubber_options.pitch = rubber_pitch;
}

inline float Sample::get_rubber_pitch() const {
    return __rubber_options.pitch;
}

inline void Sample::set_rubber_divider( float use_rubber_divider ) {
    __rubber_options.divider = use_rubber_divider;
}

inline float Sample::get_rubber_divider() const {
    return __rubber_options.divider;
}

inline void Sample::set_rubber_c_settings( int use_rubber_c_settings ) {
    __rubber_options.c_settings = use_rubber_c_settings;
}

inline float Sample::get_rubber_c_settings() const {
    return __rubber_options.c_settings;
}

};

#endif // H2C_SAMPLE_H

/* vim: set softtabstop=4 expandtab: */
