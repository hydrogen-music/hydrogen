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

#define KEY_MIN              0
#define KEY_MAX             11
#define OCTAVE_MIN          -3
#define OCTAVE_MAX           3
#define OCTAVE_OFFSET        3
#define OCTAVE_DEFAULT       0
#define KEYS_PER_OCTAVE     12

#define MIDI_FACTOR         127

#define VELOCITY_MIN        0.0f
#define VELOCITY_MAX        1.0f
#define PAN_MIN             0.0f
#define PAN_MAX             0.5f
#define LEAD_LAG_MIN        -1.0f
#define LEAD_LAG_MAX        1.0f

namespace H2Core {

class XMLNode;
class ADSR;
class Instrument;
class InstrumentList;

/**
 * A note plays an associated instrument with a velocity left and right pan
 */
class Note : public Object {
    H2_OBJECT
    public:
        // TODO use Key
        /** possible keys */
        //enum Key { C=KEY_MIN, Cs, D, Ef, E, F, Fs, G, Af, A, Bf, B };
        // TODO use Octave
        /** possible octaves */
        //enum Octave { P8Z=-3, P8Y=-2, P8X=-1, P8=0, P8A=1, P8B=2, P8C=3 };

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

        /** copy constructor */
        Note( Note* other );
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
        static Note* load_from( XMLNode* node, InstrumentList* instruments=0 );

        /** output details through logger with DEBUG severity */
        void dump();

        void map_instrument( InstrumentList* instruments );                 ///< find the corresponding instrument and point to it, or an empty instrument
        void set_instrument( Instrument* instrument );                      ///< set the instrument
        Instrument* get_instrument()        { return __instrument; }        ///< get the instrument
        int get_instrument_id() const       { return __instrument_id; }     ///< get the instrument id
        bool has_instrument() const         { return __instrument!=0; }     ///< returns true of the note is linked to an instrument
        void set_position( int position )   { __position = position; }      ///< set the position of the note
        int get_position() const            { return __position; }          ///< get the position of the note
        void set_velocity( float velocity );                                ///< set the velocity of the note
        float get_velocity() const          { return __velocity; }          ///< get the velocity of the note
        void set_pan_l( float pan );                                        ///< set the left pan of the note
        float get_pan_l() const             { return __pan_l; }             ///< get the left pan of the note
        void set_pan_r( float pan );                                        ///< set the right pan of the note
        float get_pan_r() const             { return __pan_r; }             ///< get the right pan of the note
        void set_lead_lag( float lead_lag );                                ///< set the lead lag of the note
        float get_lead_lag() const          { return __lead_lag; }          ///< get the lead lag of the note
        void set_length( int length )       { __length = length; }          ///< set length of the note
        int get_length() const              { return __length; }            ///< get length of the note
        void set_pitch( float pitch )       { __pitch = pitch; }            ///< set pitch of the note
        float get_pitch() const             { return __pitch; }             ///< get pitch of the note
        void set_note_off( bool note_off )  { __note_off = note_off; }      ///< set note_off of the note
        bool get_note_off() const           { return __note_off; }          ///< get note_off of the note
        //void set_midi_msg( int midi_msg )   { __midi_msg = midi_msg; }      ///< set midi message of the note
        int get_midi_msg() const            { return __midi_msg; }          ///< get midi message of the note
        void set_pattern_idx( int idx )     { __pattern_idx = idx; }        ///< set pattern index of the note
        int get_pattern_idx() const         { return __pattern_idx; }       ///< get pattern index of the note
        void set_just_recorded( bool val )  { __just_recorded = val; }      ///< set just recorded
        bool get_just_recorded() const      { return __just_recorded; }     ///< get just recorded
        float get_sample_position() const   { return __sample_position; }   ///< get sample position
        void set_humanize_delay( int delay ) { __humanize_delay = delay; }  ///< set humanize delay
        int get_humanize_delay() const      { return __humanize_delay; }    ///< get humanize delay
        float get_cut_off() const           { return __cut_off; }           ///< get cut off
        float get_resonance() const         { return __resonance; }         ///< get resonance
        float get_bpfb_l() const            { return __bpfb_l; }            ///< get left band pass filter buffer
        float get_bpfb_r() const            { return __bpfb_r; }            ///< get right band pass filter buffer
        float get_lpfb_l() const            { return __lpfb_l; }            ///< get left low pass filter buffer
        float get_lpfb_r() const            { return __lpfb_r; }            ///< get right low pass filter buffer
        int get_key()                       { return __key; }               ///< get key
        int get_octave()                    { return __octave; }            ///< get octave
        int get_midi_key() const;                                           ///< return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET !!!
        int get_midi_velocity() const       { return __velocity * MIDI_FACTOR; }                        ///< return scaled velocity for midi output
        float get_notekey_pitch() const     { return __octave * KEYS_PER_OCTAVE + __key; }              ///< returns octave*12 + key
        float get_total_pitch() const       { return __octave * KEYS_PER_OCTAVE + __key + __pitch; }    ///< returns octave*12+key+pitch

        /** return a string representation of key-actove */
        QString key_to_string();
        /**
         * parse str and set __key and __octave
         * \param str the string to be parsed
         */
        void set_key_octave( const QString& str );
        /**
         * set __key and __octave only if within acceptable range
         * \param key the key to set
         * \param octave the octave to be set
         */
        void set_key_octave( int key, int octave )  {
            if( key>=KEY_MIN && key<=KEY_MAX ) __key = key;
            if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) __octave = octave;
        }
        /**
         * set __key, __octave and __midi_msg only if within acceptable range
         * \param key the key to set
         * \param octave the octave to be set
         */
        void set_midi_info( int key, int octave, int msg )  {
            if( key>=KEY_MIN && key<=KEY_MAX ) __key = key;
            if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) __octave = octave;
            __midi_msg = msg;
        }

        /** get the ADSR of the note */
        ADSR* get_adsr() const {
            return __adsr;
        }
        /** call release on adsr */
        //float release_adsr() const              { return __adsr->release(); }
        /** call get value on adsr */
        //float get_adsr_value(float v) const     { return __adsr->get_value( v ); }

        /**
         * update sample_position with increment
         * \param incr the value to add to current sample position
         */
        float update_sample_position( float incr )  {
            __sample_position += incr;
            return __sample_position;
        }

        /** return true if instrument, key and octave matches with internal
         * \param instrument the instrument to match with __instrument
         * \param key the key to match with __key
         * \param octave the octave to match with __octave
         */
        bool match( Instrument* instrument, int key, int octave ) const {
            return ( ( __instrument==instrument ) && ( __key==key ) && ( __octave==octave ) );
        }

        /**
         * compute left and right output based on filters
         * \param val_l the left channel value
         * \param val_r the right channel value
         */
        void compute_lr_values( float* val_l, float* val_r );

    private:
        Instrument* __instrument;   ///< the instrument to be played by this note
        int __instrument_id;        ///< the id of the instrument played by this note
        unsigned __position;		///< note position inside the pattern
        float __velocity;		    ///< velocity (intensity) of the note [0;1]
        float __pan_l;			    ///< pan of the note (left volume) [0;1]
        float __pan_r;			    ///< pan of the note (right volume) [0;1]
        int __length;               ///< the length of the note
        float __pitch;              ///< the frequency of the note
        int __key;                  ///< the key, [0;11]==[C;B]
        int __octave;               ///< the octave [-3;3]
        ADSR* __adsr;               ///< attack decay sustain release
        float __lead_lag;		    ///< lead or lag offset of the note
        float __cut_off;		    ///< filter cutoff [0;1]
        float __resonance;	        ///< filter resonant frequency [0;1]
        int __humanize_delay;       ///< used in "humanize" function
        float __sample_position;    ///< place marker for overlapping process() cycles
        float __bpfb_l;		        ///< left band pass filter buffer
        float __bpfb_r;		        ///< right band pass filter buffer
        float __lpfb_l;		        ///< left low pass filter buffer
        float __lpfb_r;		        ///< right low pass filter buffer
        int __pattern_idx;          ///< index of the pattern holding this note for undo actions
        int __midi_msg;             ///< TODO
        bool __note_off;			///< note type on|off
        bool __just_recorded;       ///< used in record+delete
        static const char* __key_str[]; ///< used to build QString from __key an __octave
};

};

#endif // H2C_NOTE_H

/* vim: set softtabstop=4 expandtab: */
