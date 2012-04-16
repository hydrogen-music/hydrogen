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

#ifndef H2C_INSTRUMENT_H
#define H2C_INSTRUMENT_H

#include <cassert>

#include <hydrogen/object.h>
#include <hydrogen/basics/adsr.h>

#define EMPTY_INSTR_ID          -1
#define METRONOME_INSTR_ID      -2

namespace H2Core
{

class XMLNode;
class ADSR;
class Drumkit;
class InstrumentLayer;

/**
Instrument class
*/
class Instrument : public H2Core::Object
{
        H2_OBJECT
    public:
        /**
         * constructor
         * \param id the id of this instrument
         * \param name the name of the instrument
         * \param adsr attack decay sustain release instance
         */
        Instrument( const int id=EMPTY_INSTR_ID, const QString& name="Empty Instrument", ADSR* adsr=0 );
        /** copy constructor */
        Instrument( Instrument* other );
        /** destructor */
        ~Instrument();

        /**
         * creates a new Instrument, loads samples from a given instrument within a given drumkit
         * \param drumkit_name the drumkit to search the instrument in
         * \param instrument_name the instrument within the drumkit to load samples from
         * \return a new Instrument instance
         */
        static Instrument* load_instrument( const QString& drumkit_name, const QString& instrument_name );

        /**
         * loads instrument from a given instrument within a given drumkit into a `live` Instrument object.
         * \param drumkit_name the drumkit to search the instrument in
         * \param instrument_name the instrument within the drumkit to load samples from
         * \param is_live is it performed while playing
         */
        void load_from( const QString& drumkit_name, const QString& instrument_name, bool is_live = true );

        /**
         * loads instrument from a given instrument into a `live` Instrument object.
         * \param drumkit the drumkit the instrument belongs to
         * \param instrument to load samples and members from
         * \param is_live is it performed while playing
         */
        void load_from( Drumkit* drumkit, Instrument* instrument, bool is_live = true );

        /**
         * load samples data
         */
        void load_samples();
        /*
         * unload instrument samples
         */
        void unload_samples();

        /*
         * save the intrument within the given XMLNode
         * \param node the XMLNode to feed
         */
        void save_to( XMLNode* node );
        /**
         * load an instrument from an XMLNode
         * \param node the XMLDode to read from
         * \param dk_path the directory holding the drumkit data
         * \param dk_name the name of the drumkit
         * \return a new Instrument instance
         */
        static Instrument* load_from( XMLNode* node, const QString& dk_path, const QString& dk_name );

        /**
         * get an instrument layer from  the list
         * \param idx the index to get the layer from
         * \return a pointer to the layer
         */
        InstrumentLayer* operator[]( int idx );
        /**
         * get an instrument layer from  the list
         * \param idx the index to get the layer from
         * \return a pointer to the layer
         */
        InstrumentLayer* get_layer( int idx );

        /**
         * set a layer within the instrument's layer list
         * \param layer the layer to be set
         * \param idx the index within the list
         */
        void set_layer( InstrumentLayer* layer, int idx );

        ///< set the name of the instrument
        void set_name( const QString& name );
        ///< get the name of the instrument
        const QString& get_name() const;

        ///< set the id of the instrument
        void set_id( const int id );
        ///< get the id of the instrument
        int get_id() const;

        /** set the ADSR of the instrument */
        void set_adsr( ADSR* adsr );
        /** get the ADSR of the instrument */
        ADSR* get_adsr() const;
        /** get a copy of the ADSR of the instrument */
        ADSR* copy_adsr() const;

        /** set the mute group of the instrument */
        void set_mute_group( int group );
        /** get the mute group of the instrument */
        int get_mute_group() const;

        /** set the midi out channel of the instrument */
        void set_midi_out_channel( int channel );
        /** get the midi out channel of the instrument */
        int get_midi_out_channel() const;

        /** set the midi out note of the instrument */
        void set_midi_out_note( int note );
        /** get the midi out note of the instrument */
        int get_midi_out_note() const;

        /** set muted status of the instrument */
        void set_muted( bool muted );
        /** get muted status of the instrument */
        bool is_muted() const;

        /** set left pan of the instrument */
        void set_pan_l( float val );
        /** get left pan of the instrument */
        float get_pan_l() const;

        /** set right pan of the instrument */
        void set_pan_r( float val );
        /** get right pan of the instrument */
        float get_pan_r() const;

        /** set gain of the instrument */
        void set_gain( float gain );
        /** get gain of the instrument */
        float get_gain() const;
        /** set the volume of the instrument */
        void set_volume( float volume );
        /** get the volume of the instrument */
        float get_volume() const;

        /** activate the filter of the instrument */
        void set_filter_active( bool active );
        /** get the status of the filter of the instrument */
        bool is_filter_active() const;

        /** set the filter resonance of the instrument */
        void set_filter_resonance( float val );
        /** get the filter resonance of the instrument */
        float get_filter_resonance() const;

        /** set the filter cutoff of the instrument */
        void set_filter_cutoff( float val );
        /** get the filter cutoff of the instrument */
        float get_filter_cutoff() const;

        /** set the left peak of the instrument */
        void set_peak_l( float val );
        /** get the left peak of the instrument */
        float get_peak_l() const;
        /** set the right peak of the instrument */
        void set_peak_r( float val );
        /** get the right peak of the instrument */
        float get_peak_r() const;

        /** set the fx level of the instrument */
        void set_fx_level( float level, int index );
        /** get the fx level of the instrument */
        float get_fx_level( int index ) const;

        /** set the random pitch factor of the instrument */
        void set_random_pitch_factor( float val );
        /** get the random pitch factor of the instrument */
        float get_random_pitch_factor() const;

        /** set the active status of the instrument */
        void set_active( bool active );
        /** get the active status of the instrument */
        bool is_active() const;

        /** set the soloed status of the instrument */
        void set_soloed( bool soloed );
        /** get the soloed status of the instrument */
        bool is_soloed() const;

        /** enqueue the instrument */
        void enqueue();
        /** dequeue the instrument */
        void dequeue();
        /** get the queued status of the instrument */
        bool is_queued() const;

        /** set the stop notes status of the instrument */
        void set_stop_notes( bool stopnotes );
        /** get the stop notes of the instrument */
        bool is_stop_notes() const;

        ///< set the name of the related drumkit
        void set_drumkit_name( const QString& name );
        ///< get the name of the related drumkits
        const QString& get_drumkit_name() const;


    private:
        int __id;			                    ///< instrument id, should be unique
        QString __name;			                ///< instrument name
        QString __drumkit_name;                         ///< the name of the drumkit this instrument belongs tos
        float __gain;                           ///< gain of the instrument
        float __volume;			                ///< volume of the instrument
        float __pan_l;			                ///< left pan of the instrument
        float __pan_r;			                ///< right pan of the instrument
        float __peak_l;			                ///< left current peak value
        float __peak_r;			                ///< right current peak value
        ADSR* __adsr;                           ///< attack delay sustain release instance
        bool __filter_active;		            ///< is filter active?
        float __filter_cutoff;		            ///< filter cutoff (0..1)
        float __filter_resonance;	            ///< filter resonant frequency (0..1)
        float __random_pitch_factor;            ///< random pitch factor
        int __midi_out_note;		            ///< midi out note
        int __midi_out_channel;		            ///< midi out channel
        bool __stop_notes;		                ///< will the note automatically generate a note off after beeing on
        bool __active;			                ///< is the instrument active?
        bool __soloed;                          ///< is the instrument in solo mode?
        bool __muted;                           ///< is the instrument muted?
        int __mute_group;		                ///< mute group of the instrument
        int __queued;                           ///< count the number of notes queued within Sampler::__playing_notes_queue or std::priority_queue m_songNoteQueue
        float __fx_level[MAX_FX];	            ///< Ladspa FX level array
        InstrumentLayer* __layers[MAX_LAYERS];  ///< InstrumentLayer array
};

// DEFINITIONS

inline void Instrument::set_name( const QString& name )
{
    __name = name;
}

inline const QString& Instrument::get_name() const
{
    return __name;
}

inline void Instrument::set_id( const int id )
{
    __id = id;
}

inline int Instrument::get_id() const
{
    return __id;
}

inline ADSR* Instrument::get_adsr() const
{
    return __adsr;
}

inline ADSR* Instrument::copy_adsr() const
{
    return new ADSR( __adsr );
}

inline void Instrument::set_mute_group( int group )
{
    __mute_group = ( group<-1 ? -1 : group );
}

inline int Instrument::get_mute_group() const
{
    return __mute_group;
}

inline int Instrument::get_midi_out_channel() const
{
    return __midi_out_channel;
}

inline void Instrument::set_midi_out_channel( int channel )
{
    if ( ( channel >= MIDI_OUT_CHANNEL_MIN ) && ( channel <= MIDI_OUT_CHANNEL_MAX ) ) {
        __midi_out_channel = channel;
    } else {
        ERRORLOG( QString( "midi out channel %1 out of bounds" ).arg( channel ) );
    }
}

inline int Instrument::get_midi_out_note() const
{
    return __midi_out_note;
}

inline void Instrument::set_midi_out_note( int note )
{
    if ( ( note >= MIDI_OUT_NOTE_MIN ) && ( note <= MIDI_OUT_NOTE_MAX ) ) {
        __midi_out_note = note;
    } else {
        ERRORLOG( QString( "midi out note %1 out of bounds" ).arg( note ) );
    }
}

inline void Instrument::set_muted( bool muted )
{
    __muted = muted;
}

inline bool Instrument::is_muted() const
{
    return __muted;
}

inline void Instrument::set_pan_l( float val )
{
    __pan_l = val;
}

inline float Instrument::get_pan_l() const
{
    return __pan_l;
}

inline void Instrument::set_pan_r( float val )
{
    __pan_r = val;
}

inline float Instrument::get_pan_r() const
{
    return __pan_r;
}

inline void Instrument::set_gain( float gain )
{
    __gain = gain;
}

inline float Instrument::get_gain() const
{
    return __gain;
}

inline void Instrument::set_volume( float volume )
{
    __volume = volume;
}

inline float Instrument::get_volume() const
{
    return __volume;
}

inline void Instrument::set_filter_active( bool active )
{
    __filter_active = active;
}

inline bool Instrument::is_filter_active() const
{
    return __filter_active;
}

inline void Instrument::set_filter_resonance( float val )
{
    __filter_resonance = val;
}

inline float Instrument::get_filter_resonance() const
{
    return __filter_resonance;
}

inline void Instrument::set_filter_cutoff( float val )
{
    __filter_cutoff = val;
}

inline float Instrument::get_filter_cutoff() const
{
    return __filter_cutoff;
}

inline void Instrument::set_peak_l( float val )
{
    __peak_l = val;
}

inline float Instrument::get_peak_l() const
{
    return __peak_l;
}

inline void Instrument::set_peak_r( float val )
{
    __peak_r = val;
}

inline float Instrument::get_peak_r() const
{
    return __peak_r;
}

inline void Instrument::set_fx_level( float level, int index )
{
    __fx_level[index] = level;
}

inline float Instrument::get_fx_level( int index ) const
{
    return __fx_level[index];
}

inline void Instrument::set_random_pitch_factor( float val )
{
    __random_pitch_factor = val;
}

inline float Instrument::get_random_pitch_factor() const
{
    return __random_pitch_factor;
}

inline void Instrument::set_active( bool active )
{
    __active = active;
}

inline bool Instrument::is_active() const
{
    return __active;
}

inline void Instrument::set_soloed( bool soloed )
{
    __soloed = soloed;
}

inline bool Instrument::is_soloed() const
{
    return __soloed;
}

inline void Instrument::enqueue()
{
    __queued++;
}

inline void Instrument::dequeue()
{
    assert( __queued > 0 );
    __queued--;
}

inline bool Instrument::is_queued() const
{
    return ( __queued > 0 );
}

inline void Instrument::set_stop_notes( bool stopnotes )
{
    __stop_notes = stopnotes;
}

inline bool Instrument::is_stop_notes() const
{
    return __stop_notes;
}

inline InstrumentLayer* Instrument::operator[]( int idx )
{
    assert( idx>=0 && idx <MAX_LAYERS );
    return __layers[ idx ];
}

inline InstrumentLayer* Instrument::Instrument::get_layer( int idx )
{
    assert( idx>=0 && idx <MAX_LAYERS );
    return __layers[ idx ];
}

inline void Instrument::set_layer( InstrumentLayer* layer, int idx )
{
    assert( idx>=0 && idx <MAX_LAYERS );
    __layers[ idx ] = layer;
}

inline void Instrument::set_drumkit_name( const QString& name )
{
    __drumkit_name = name;
}

inline const QString& Instrument::get_drumkit_name() const
{
    return __drumkit_name;
}

};



#endif // H2C_INSTRUMENT_H

/* vim: set softtabstop=4 expandtab:  */
