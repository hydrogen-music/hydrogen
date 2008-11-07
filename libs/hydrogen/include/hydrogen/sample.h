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
class Sample : public Object
{
public:
	Sample( unsigned frames,
		const QString& filename, 
		float* data_L = NULL,
		float* data_R = NULL,
		bool sample_is_modified = false,
		const QString& sample_mode = "forward",
		unsigned start_frame = 0,
		unsigned loop_frame = 0,
		int repeats = 0,
		unsigned end_frame = 0,
		unsigned fade_out_startframe = 0,
		int fade_out_type = 0);
		

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
	static Sample* load_edit_wave( const QString& filename,
				const unsigned startframe,
				const unsigned loppframe,
				const unsigned endframe,
				const int loops,
				const QString loopmode,
 				const unsigned fadeoutstartframe,
				const int fadeouttype);


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

	void set_fade_out_startframe( unsigned fade_out_startframe ) {
		__fade_out_startframe = fade_out_startframe;
	}
	unsigned get_fade_out_startframe() const {
		return __fade_out_startframe;
	}

	void set_fade_out_type( int fade_out_type ) {
		__fade_out_type = fade_out_type;
	}
	int get_fade_out_type() const {
		return __fade_out_type;
	}


	void sampleEditProzess( Sample* Sample );
	void setmod();

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
	unsigned __fade_out_startframe;	///< start frame for fade out
	int __fade_out_type;		///< fade out type 0=off, 1=lin , 2=log


	//static int __total_used_bytes;

	/// loads a wave file
	static Sample* load_wave( const QString& filename );

	/// loads a FLAC file
	static Sample* load_flac( const QString& filename );
	Sample *tempsample;
};

};

#endif
