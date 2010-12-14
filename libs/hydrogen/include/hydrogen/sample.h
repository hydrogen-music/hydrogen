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

#ifndef SAMPLE_BUFFER_H
#define SAMPLE_BUFFER_H

#include <string>
#include <hydrogen/globals.h>
#include <hydrogen/Object.h>

namespace H2Core
{

/**
\ingroup H2CORE
*/

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


class Sample : public Object
{
public:
	Sample( unsigned frames,
		const QString& filename, 
		unsigned sample_rate,
		float* data_L = NULL,
		float* data_R = NULL,
		bool sample_is_modified = false,
		const QString& sample_mode = "forward",
		unsigned start_frame = 0,
		unsigned loop_frame = 0,
		int repeats = 0,
		unsigned end_frame = 0,
		SampleVeloPan velopan = SampleVeloPan(),
		bool use_rubber = false,
		float use_rubber_divider = 1.0,
		int use_rubber_c_settings = 4,
		float rubber_pitch = 0.0 );
		

	~Sample();

	float* get_data_l() {
		return __data_l;
	}
	float* get_data_r() {
		return __data_r;
	}

	unsigned get_sample_rate() {
		return __sample_rate;
	}

	const QString get_filename() {
		return __filename;
	}


	/// Returns the bytes number ( 2 channels )
	unsigned get_size() {
		return __n_frames * sizeof( float ) * 2;
	}

	/// Loads a sample from disk
	static Sample* load( const QString& filename );

	/// Loads an modified sample
        static Sample* load_edit_sndfile( const QString& filename,
				const unsigned startframe,
				const unsigned loppframe,
				const unsigned endframe,
				const int loops,
				const QString loopmode,
				const bool use_rubberband,
				const float rubber_divider,
				const int rubber_c_settings,
				const float rubber_pitch );


	void set_new_sample_length_frames( unsigned new_sample_length) {
		__n_frames = new_sample_length;
	}

	unsigned get_n_frames() {
		return __n_frames;
	}

	///beginn of sample edit 

	void set_sample_is_modified( bool is_modified ) {
		__sample_is_modified = is_modified;
	}
	bool get_sample_is_modified() const {
		return __sample_is_modified;
	}

	void set_sample_mode( QString sample_mode ) {
		__sample_mode = sample_mode;
	}
	QString get_sample_mode() const {
		return __sample_mode;
	}

	void set_start_frame( unsigned start_frame ) {
		__start_frame = start_frame;
	}
	unsigned get_start_frame() const {
		return __start_frame;
	}

	void set_loop_frame( unsigned loop_frame ) {
		 __loop_frame = loop_frame;
	}
	unsigned get_loop_frame() const {
		return __loop_frame;
	}

	void set_repeats( int repeats ) {
		__repeats = repeats;
	}
	int get_repeats() const {
		return __repeats;
	}

	void set_end_frame( unsigned end_frame ) {
		__end_frame = end_frame;
	}
	unsigned get_end_frame() const {
		return __end_frame;
	}

	void set_use_rubber( bool use_rubber ) {
		__use_rubber = use_rubber;
	}
	bool get_use_rubber() const {
		return __use_rubber;
	}

	void set_rubber_divider( float use_rubber_divider ) {
		__rubber_divider = use_rubber_divider;
	}
	float get_rubber_divider() const {
		return __rubber_divider;
	}

	void set_rubber_C_settings( int use_rubber_c_settings) {
		__rubber_C_settings = use_rubber_c_settings;
	}
	float get_rubber_C_settings() const {
		return __rubber_C_settings;
	}

	void set_rubber_pitch( float rubber_pitch ) {
		__rubber_pitch = rubber_pitch;
	}
	float get_rubber_pitch() const {
		return __rubber_pitch;
	}

	void sampleEditProzess( Sample* Sample );
	void setmod();

	SampleVeloPan __velo_pan;	///< volume and pan vector

private:

	float *__data_l;	///< Left channel data
	float *__data_r;	///< Right channel data
	unsigned __sample_rate;		///< samplerate for this sample
	QString __filename;		///< filename associated with this sample
	unsigned __n_frames;		///< Total number of frames in this sample.
	bool __sample_is_modified;	///< true if sample is modified
	QString __sample_mode;		///< loop mode
	unsigned __start_frame;		///< start frame
	unsigned __loop_frame;		///< beginn of the loop section
	int __repeats;			///< repeats from the loop section
	unsigned __end_frame; 		///< sample end frame
	bool __use_rubber;		///< use the rubberband bin
	float __rubber_divider;		///< the divider to calculate the ratio
	int __rubber_C_settings;	///< the rubberband "crispness" levels
	float __rubber_pitch;		///< rubberband pitch 


	//static int __total_used_bytes;

        /// loads a file
        static Sample* load_sndfile( const QString& filename );


};

};

#endif
