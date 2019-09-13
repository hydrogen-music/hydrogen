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

#ifndef H2C_NOTE_H
#define H2C_NOTE_H

#include <hydrogen/object.h>
#include <hydrogen/basics/instrument.h>

#define KEY_MIN                 0
#define KEY_MAX                 11
#define OCTAVE_MIN              -3
#define OCTAVE_MAX              3
#define OCTAVE_OFFSET           3
#define OCTAVE_DEFAULT          0
#define KEYS_PER_OCTAVE         12

#define MIDI_MIDDLE_C           60
#define MIDI_FACTOR             127

#define VELOCITY_MIN            0.0f
#define VELOCITY_MAX            1.0f
#define PAN_MIN                 0.0f
#define PAN_MAX                 0.5f
#define LEAD_LAG_MIN            -1.0f
#define LEAD_LAG_MAX            1.0f

/* Should equal (default __octave + OCTAVE_OFFSET) * KEYS_PER_OCTAVE + default __key */
#define MIDI_DEFAULT_OFFSET     36

namespace H2Core
{

class XMLNode;
class ADSR;
class Instrument;
class InstrumentList;

struct SelectedLayerInfo {
	int SelectedLayer;		///< selected layer during layer selection
	float SamplePosition;	///< place marker for overlapping process() cycles
};

/**
 * A note plays an associated instrument with a velocity left and right pan
 */
class Note : public H2Core::Object
{
		H2_OBJECT
	public:
		/** possible keys */
		enum Key { C=KEY_MIN, Cs, D, Ef, E, F, Fs, G, Af, A, Bf, B };
		/** possible octaves */
		enum Octave { P8Z=-3, P8Y=-2, P8X=-1, P8=OCTAVE_DEFAULT, P8A=1, P8B=2, P8C=3 };

		/**
		 * constructor
		 * \param instrument the instrument played by this note
		 * \param position the position of the note within the pattern
		 * \param velocity it's velocity
		 * \param pan_l left pan
		 * \param pan_r right pan
		 * \param length it's length
		 * \param pitch it's pitch
		 */
		Note( Instrument* instrument, int position, float velocity, float pan_l, float pan_r, int length, float pitch );

		/**
		 * copy constructor with an optional parameter
		 * \param other 
		 * \param instrument if set will be used as note instrument
		 */
		Note( Note* other, Instrument* instrument=nullptr );
		/** destructor */
		~Note();

		/*
		 * save the note within the given XMLNode
		 * \param node the XMLNode to feed
		 */
		void save_to( XMLNode* node );
		/**
		 * load a note from an XMLNode
		 * \param node the XMLDode to read from
		 * \param instruments the current instrument list to search instrument into
		 * \return a new Note instance
		 */
		static Note* load_from( XMLNode* node, InstrumentList* instruments );

		/** output details through logger with DEBUG severity */
		void dump();

		/**
		 * find the corresponding instrument and point to it, or an empty instrument
		 * \param instruments the list of instrument to look into
		 */
		void map_instrument( InstrumentList* instruments );
		/** #__instrument accessor */
		Instrument* get_instrument();
		/** return true if #__instrument is set */
		bool has_instrument() const;
		/**
		 * #__instrument_id setter
		 * \param value the new value
		 */
		void set_instrument_id( int value );
		/** #__instrument_id accessor */
		int get_instrument_id() const;
		/**
		 * #__specific_compo_id setter
		 * \param value the new value
		 */
		void set_specific_compo_id( int value );
		/** #__specific_compo_id accessor */
		int get_specific_compo_id() const;
		/**
		 * #__position setter
		 * \param value the new value
		 */
		void set_position( int value );
		/** #__position accessor */
		int get_position() const;
		/**
		 * #__velocity setter
		 * \param value the new value
		 */
		void set_velocity( float value );
		/** #__velocity accessor */
		float get_velocity() const;
		/**
		 * #__pan_l setter
		 * \param value the new value
		 */
		void set_pan_l( float value );
		/** #__pan_l accessor */
		float get_pan_l() const;
		/**
		 * #__pan_r setter
		 * \param value the new value
		 */
		void set_pan_r( float value );
		/** #__pan_r accessor */
		float get_pan_r() const;
		/**
		 * #__lead_lag setter
		 * \param value the new value
		 */
		void set_lead_lag( float value );
		/** #__lead_lag accessor */
		float get_lead_lag() const;
		/**
		 * #__length setter
		 * \param value the new value
		 */
		void set_length( int value );
		/** #__length accessor */
		int get_length() const;
		/**
		 * #__pitch setter
		 * \param value the new value
		 */
		void set_pitch( float value );
		/** #__pitch accessor */
		float get_pitch() const;
		/**
		 * #__note_off setter
		 * \param value the new value
		 */
		void set_note_off( bool value );
		/** #__note_off accessor */
		bool get_note_off() const;
		/** #__midi_msg accessor */
		int get_midi_msg() const;
		/**
		 * #__pattern_idx setter
		 * \param value the new value
		 */
		void set_pattern_idx( int value );
		/** #__pattern_idx accessor */
		int get_pattern_idx() const;
		/**
		 * #__just_recorded setter
		 * \param value the new value
		 */
		void set_just_recorded( bool value );
		/** #__just_recorded accessor */
		bool get_just_recorded() const;

		/*
		 * selected sample
		 * */
		SelectedLayerInfo* get_layer_selected( int CompoID );


		void set_probability( float value );
		float get_probability() const;

		/**
		 * #__humanize_delay setter
		 * \param value the new value
		 */
		void set_humanize_delay( int value );
		/** #__humanize_delay accessor */
		int get_humanize_delay() const;
		/** #__cut_off accessor */
		float get_cut_off() const;
		/** #__resonance accessor */
		float get_resonance() const;
		/** #__bpfb_l accessor */
		float get_bpfb_l() const;
		/** #__bpfb_r accessor */
		float get_bpfb_r() const;
		/** #__lpfb_l accessor */
		float get_lpfb_l() const;
		/** #__lpfb_r accessor */
		float get_lpfb_r() const;
		/** #__key accessor */
		Key get_key();
		/** #__octave accessor */
		Octave get_octave();
		/** return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET !!! */
		int get_midi_key() const;
		/** midi velocity accessor 
		 * \code{.cpp}
		 * __velocity * #MIDI_FACTOR
		 * \endcode */
		int get_midi_velocity() const;
		/** note key pitch accessor
		 * \code{.cpp}
		 * __octave * KEYS_PER_OCTAVE + __key
		 * \endcode */
		float get_notekey_pitch() const;
	        /** returns
		 * \code{.cpp}
		 * __octave * 12 + __key + __pitch 
		 * \endcode*/ 
		float get_total_pitch() const;

		/** return a string representation of key-octave */
		QString key_to_string();
		/**
		 * parse str and set #__key and #__octave
		 * \param str the string to be parsed
		 */
		void set_key_octave( const QString& str );
		/**
		 * set #__key and #__octave only if within acceptable range
		 * \param key the key to set
		 * \param octave the octave to be set
		 */
		void set_key_octave( Key key, Octave octave );
		/**
		 * set #__key, #__octave and #__midi_msg only if within acceptable range
		 * \param key the key to set
		 * \param octave the octave to be set
		 * \param msg
		 */
		void set_midi_info( Key key, Octave octave, int msg );

		/** get the ADSR of the note */
		ADSR* get_adsr() const;
		/** call release on adsr */
		//float release_adsr() const              { return __adsr->release(); }
		/** call get value on adsr */
		//float get_adsr_value(float v) const     { return __adsr->get_value( v ); }

		/** return true if instrument, key and octave matches with internal
		 * \param instrument the instrument to match with #__instrument
		 * \param key the key to match with #__key
		 * \param octave the octave to match with #__octave
		 */
		bool match( Instrument* instrument, Key key, Octave octave ) const;

		/**
		 * compute left and right output based on filters
		 * \param val_l the left channel value
		 * \param val_r the right channel value
		 */
		void compute_lr_values( float* val_l, float* val_r );

	private:
		Instrument*		__instrument;   ///< the instrument to be played by this note
		int				__instrument_id;        ///< the id of the instrument played by this note
		int				__specific_compo_id;    ///< play a specific component, -1 if playing all
		int				__position;             ///< note position inside the pattern
		float			__velocity;           ///< velocity (intensity) of the note [0;1]
		float			__pan_l;              ///< pan of the note (left volume) [0;0.5]
		float			__pan_r;              ///< pan of the note (right volume) [0;0.5]
		int				__length;               ///< the length of the note
		float			__pitch;              ///< the frequency of the note
		Key				__key;                  ///< the key, [0;11]==[C;B]
		Octave			 __octave;            ///< the octave [-3;3]
		ADSR*			__adsr;               ///< attack decay sustain release
		float			__lead_lag;           ///< lead or lag offset of the note
		float			__cut_off;            ///< filter cutoff [0;1]
		float			__resonance;          ///< filter resonant frequency [0;1]
		int				__humanize_delay;       ///< used in "humanize" function
		std::map< int, SelectedLayerInfo* > __layers_selected;
		float			__bpfb_l;             ///< left band pass filter buffer
		float			__bpfb_r;             ///< right band pass filter buffer
		float			__lpfb_l;             ///< left low pass filter buffer
		float			__lpfb_r;             ///< right low pass filter buffer
		int				__pattern_idx;          ///< index of the pattern holding this note for undo actions
		int				__midi_msg;             ///< TODO
		bool			__note_off;            ///< note type on|off
		bool			__just_recorded;       ///< used in record+delete
		float			__probability;        ///< note probability
		static const char* __key_str[]; ///< used to build QString from #__key an #__octave
};

// DEFINITIONS

inline ADSR* Note::get_adsr() const
{
	return __adsr;
}

inline Instrument* Note::get_instrument()
{
	return __instrument;
}

inline bool Note::has_instrument() const
{
	return __instrument!=nullptr;
}

inline void Note::set_instrument_id( int value )
{
	__instrument_id = value;
}

inline int Note::get_instrument_id() const
{
	return __instrument_id;
}

inline void Note::set_specific_compo_id( int value )
{
	__specific_compo_id = value;
}

inline int Note::get_specific_compo_id() const
{
	return __specific_compo_id;
}

inline void Note::set_position( int value )
{
	__position = value;
}

inline int Note::get_position() const
{
	return __position;
}

inline float Note::get_velocity() const
{
	return __velocity;
}

inline float Note::get_pan_l() const
{
	return __pan_l;
}

inline float Note::get_pan_r() const
{
	return __pan_r;
}

inline float Note::get_lead_lag() const
{
	return __lead_lag;
}

inline void Note::set_length( int value )
{
	__length = value;
}

inline int Note::get_length() const
{
	return __length;
}

inline void Note::set_pitch( float value )
{
	__pitch = value;
}

inline float Note::get_pitch() const
{
	return __pitch;
}

inline void Note::set_note_off( bool value )
{
	__note_off = value;
}

inline bool Note::get_note_off() const
{
	return __note_off;
}

inline int Note::get_midi_msg() const
{
	return __midi_msg;
}

inline void Note::set_pattern_idx( int value )
{
	__pattern_idx = value;
}

inline int Note::get_pattern_idx() const
{
	return __pattern_idx;
}

inline void Note::set_just_recorded( bool value )
{
	__just_recorded = value;
}

inline bool Note::get_just_recorded() const
{
	return __just_recorded;
}

inline float Note::get_probability() const
{
	return __probability;
}

inline void Note::set_probability( float value )
{
	__probability = value;
}

inline SelectedLayerInfo* Note::get_layer_selected( int CompoID )
{
	return __layers_selected[ CompoID ];
}

inline void Note::set_humanize_delay( int value )
{
	__humanize_delay = value;
}

inline int Note::get_humanize_delay() const
{
	return __humanize_delay;
}

inline float Note::get_cut_off() const
{
	return __cut_off;
}

inline float Note::get_resonance() const
{
	return __resonance;
}

inline float Note::get_bpfb_l() const
{
	return __bpfb_l;
}

inline float Note::get_bpfb_r() const
{
	return __bpfb_r;
}

inline float Note::get_lpfb_l() const
{
	return __lpfb_l;
}

inline float Note::get_lpfb_r() const
{
	return __lpfb_r;
}

inline Note::Key Note::get_key()
{
	return __key;
}

inline Note::Octave Note::get_octave()
{
	return __octave;
}

inline int Note::get_midi_key() const
{
	/* TODO ???
	if( !has_instrument() ) { return (__octave + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE + __key; }
	*/
	return ( __octave + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE + __key + __instrument->get_midi_out_note() - MIDI_DEFAULT_OFFSET;
}

inline int Note::get_midi_velocity() const
{
	return __velocity * MIDI_FACTOR;
}

inline float Note::get_notekey_pitch() const
{
	return __octave * KEYS_PER_OCTAVE + __key;
}

inline float Note::get_total_pitch() const
{
	return __octave * KEYS_PER_OCTAVE + __key + __pitch;
}

inline void Note::set_key_octave( Key key, Octave octave )
{
	if( key>=KEY_MIN && key<=KEY_MAX ) __key = key;
	if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) __octave = octave;
}

inline void Note::set_midi_info( Key key, Octave octave, int msg )
{
	if( key>=KEY_MIN && key<=KEY_MAX ) __key = key;
	if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) __octave = octave;
	__midi_msg = msg;
}

inline bool Note::match( Instrument* instrument, Key key, Octave octave ) const
{
	return ( ( __instrument==instrument ) && ( __key==key ) && ( __octave==octave ) );
}

inline void Note::compute_lr_values( float* val_l, float* val_r )
{
	/* TODO ???
	if( !has_instrument() ) {
		*val_l = 0.0f;
		*val_r = 0.0f;
		return;
	}
	*/
	float cut_off = __instrument->get_filter_cutoff();
	float resonance = __instrument->get_filter_resonance();
	__bpfb_l  =  resonance * __bpfb_l  + cut_off * ( *val_l - __lpfb_l );
	__lpfb_l +=  cut_off   * __bpfb_l;
	__bpfb_r  =  resonance * __bpfb_r  + cut_off * ( *val_r - __lpfb_r );
	__lpfb_r +=  cut_off   * __bpfb_r;
	*val_l = __lpfb_l;
	*val_r = __lpfb_r;
}

};

#endif // H2C_NOTE_H

/* vim: set softtabstop=4 noexpandtab: */
