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

#include <hydrogen/basics/note.h>

#include <cassert>

#include <hydrogen/helpers/xml.h>

#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>

namespace H2Core
{

const char* Note::__class_name = "Note";
const char* Note::__key_str[] = { "C", "Cs", "D", "Ef", "E", "F", "Fs", "G", "Af", "A", "Bf", "B" };

Note::Note( Instrument* instrument, int position, float velocity, float pan_l, float pan_r, int length, float pitch )
    : Object( __class_name ),
      __instrument( instrument ),
      __position( position ),
      __velocity( velocity ),
      __pan_l( pan_l ),
      __pan_r( pan_r ),
      __length( length ),
      __pitch( pitch ),
      __key( C ),
      __octave( P8 ),
//      __adsr( __instrument->get_adsr() ),
      __lead_lag( 0.0 ),
      __cut_off( 1.0 ),
      __resonance( 0.0 ),
      __humanize_delay( 0 ),
      __sample_position( 0.0 ),
      __bpfb_l( 0.0 ),
      __bpfb_r( 0.0 ),
      __lpfb_l( 0.0 ),
      __lpfb_r( 0.0 ),
      __pattern_idx( 0 ),
      __midi_msg( -1 ),
      __note_off( false ),
      __just_recorded( false )
{
    set_ADSR( __instrument );
}

Note::Note( Note* other )
    : Object( __class_name ),
      __instrument( other->get_instrument() ),
      __position( other->get_position() ),
      __velocity( other->get_velocity() ),
      __pan_l( other->get_pan_l() ),
      __pan_r( other->get_pan_r() ),
      __length( other->get_length() ),
      __pitch( other->get_pitch() ),
      __key( other->get_key() ),
      __octave( other->get_octave() ),
      //__adsr( other->get_adsr() ),
      __lead_lag( other->get_lead_lag() ),
      __cut_off( other->get_cut_off() ),
      __resonance( other->get_resonance() ),
      __humanize_delay( other->get_humanize_delay() ),
      __sample_position( other->get_sample_position() ),
      __bpfb_l( other->get_bpfb_l() ),
      __bpfb_r( other->get_bpfb_r() ),
      __lpfb_l( other->get_lpfb_l() ),
      __lpfb_r( other->get_lpfb_r() ),
      __pattern_idx( other->get_pattern_idx() ),
      __midi_msg( other->get_midi_msg() ),
      __note_off( other->get_note_off() ),
      __just_recorded( other->get_just_recorded() )
{
    set_ADSR( other->get_instrument() );
}

Note::~Note() { }

static inline float check_boundary( float v, float min, float max )
{
    if ( v>max ) return max;
    if ( v<min ) return min;
    return v;
}

void Note::set_velocity( float velocity )
{
    __velocity = check_boundary( velocity, VELOCITY_MIN, VELOCITY_MAX );
}

void Note::set_lead_lag( float lead_lag )
{
    __lead_lag = check_boundary( lead_lag, LEAD_LAG_MIN, LEAD_LAG_MAX );
}

void Note::set_pan_l( float pan )
{
    __pan_l = check_boundary( pan, PAN_MIN, PAN_MAX );
}

void Note::set_pan_r( float pan )
{
    __pan_r = check_boundary( pan, PAN_MIN, PAN_MAX );
}

void Note::map_instrument( InstrumentList* instruments )
{
    assert( instruments );
    Instrument* instr = instruments->find( __instrument_id );
    if( !instr ) {
        ERRORLOG( QString( "Instrument with ID: '%1' not found. Using empty instrument." ).arg( __instrument_id ) );
        __instrument = new Instrument();
    } else {
        __instrument = instr;
    }
}


void Note::set_ADSR(   Instrument*   instrument  )
{
    assert( instrument->get_adsr() );
    __adsr = instrument->get_adsr();
}

void Note::set_instrument( Instrument* instrument )
{
    if ( instrument == 0 ) return;
    __instrument = instrument;
    assert( __instrument->get_adsr() );
    __adsr = instrument->get_adsr();
}

QString Note::key_to_string()
{
    return QString( "%1%2" ).arg( __key_str[__key] ).arg( __octave );
}

void Note::set_key_octave( const QString& str )
{
    int l = str.length();
    QString s_key = str.left( l-1 );
    QString s_oct = str.mid( l-1, l );
    if ( s_key.endsWith( "-" ) ) {
        s_key.replace( "-", "" );
        s_oct.insert( 0, "-" );
    }
    __octave = ( Octave )s_oct.toInt();
    for( int i=KEY_MIN; i<=KEY_MAX; i++ ) {
        if( __key_str[i]==s_key )
            __key = ( Key )i;
        return;
    }
    ___ERRORLOG( "Unhandled key: " + s_key );
}

void Note::dump()
{
    INFOLOG( QString( "Note : pos: %1\t humanize offset%2\t instr: %3\t key: %4\t pitch: %5" )
             .arg( __position )
             .arg( __humanize_delay )
             .arg( __instrument->get_name() )
             .arg( key_to_string() )
             .arg( __pitch )
             .arg( __note_off )
           );
}

void Note::save_to( XMLNode* node )
{
    node->write_int( "position", __position );
    node->write_float( "leadlag", __lead_lag );
    node->write_float( "velocity", __velocity );
    node->write_float( "pan_L", __pan_l );
    node->write_float( "pan_R", __pan_r );
    node->write_float( "pitch", __pitch );
    node->write_string( "key", key_to_string() );
    node->write_int( "length", __length );
    node->write_int( "instrument", get_instrument()->get_id() );
    node->write_bool( "note_off", __note_off );
}

Note* Note::load_from( XMLNode* node, InstrumentList* instruments )
{
    Note* note = new Note(
        0,
        node->read_int( "position", 0 ),
        node->read_float( "velocity", 0.8f ),
        node->read_float( "pan_L", 0.5f ),
        node->read_float( "pan_R", 0.5f ),
        node->read_int( "length", -1 ),
        node->read_float( "pitch", 0.0f )
    );
    note->set_lead_lag( node->read_float( "leadlag", 0, false, false ) );
    note->set_key_octave( node->read_string( "key", "C0", false, false ) );
    note->set_note_off( node->read_bool( "note_off", false, false, false ) );
    note->map_instrument( instruments );
    return note;
}

};

/* vim: set softtabstop=4 expandtab: */
