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
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>

namespace H2Core
{

const char* Note::__class_name = "Note";
const char* Note::__key_str[] = { "C", "Cs", "D", "Ef", "E", "F", "Fs", "G", "Af", "A", "Bf", "B" };

Note::Note( Instrument* instrument, int position, float velocity, float pan_l, float pan_r, int length, float pitch )
	: Object( __class_name ),
	  __instrument( instrument ),
	  __instrument_id( 0 ),
	  __specific_compo_id( -1 ),
	  __position( position ),
	  __velocity( velocity ),
	  __pan_l( PAN_MAX ),
	  __pan_r( PAN_MAX ),
	  __length( length ),
	  __pitch( pitch ),
	  __key( C ),
	  __octave( P8 ),
	  __adsr( 0 ),
	  __lead_lag( 0.0 ),
	  __cut_off( 1.0 ),
	  __resonance( 0.0 ),
	  __humanize_delay( 0 ),
	  __bpfb_l( 0.0 ),
	  __bpfb_r( 0.0 ),
	  __lpfb_l( 0.0 ),
	  __lpfb_r( 0.0 ),
	  __pattern_idx( 0 ),
	  __midi_msg( -1 ),
	  __note_off( false ),
	  __just_recorded( false ),
	  __probability( 1.0f )
{
	if ( __instrument != 0 ) {
		__adsr = __instrument->copy_adsr();
		__instrument_id = __instrument->get_id();

		for (std::vector<InstrumentComponent*>::iterator it = __instrument->get_components()->begin() ; it !=__instrument->get_components()->end(); ++it) {
			InstrumentComponent *pCompo = *it;

			SelectedLayerInfo *sampleInfo = new SelectedLayerInfo;
			sampleInfo->SelectedLayer = -1;
			sampleInfo->SamplePosition = 0;

			__layers_selected[ pCompo->get_drumkit_componentID() ] = sampleInfo;
		}
	}

	set_pan_l(pan_l);
	set_pan_r(pan_r);
}

Note::Note( Note* other, Instrument* instrument )
	: Object( __class_name ),
	  __instrument( other->get_instrument() ),
	  __instrument_id( 0 ),
	  __specific_compo_id( -1 ),
	  __position( other->get_position() ),
	  __velocity( other->get_velocity() ),
	  __pan_l( other->get_pan_l() ),
	  __pan_r( other->get_pan_r() ),
	  __length( other->get_length() ),
	  __pitch( other->get_pitch() ),
	  __key( other->get_key() ),
	  __octave( other->get_octave() ),
	  __adsr( 0 ),
	  __lead_lag( other->get_lead_lag() ),
	  __cut_off( other->get_cut_off() ),
	  __resonance( other->get_resonance() ),
	  __humanize_delay( other->get_humanize_delay() ),
	  __bpfb_l( other->get_bpfb_l() ),
	  __bpfb_r( other->get_bpfb_r() ),
	  __lpfb_l( other->get_lpfb_l() ),
	  __lpfb_r( other->get_lpfb_r() ),
	  __pattern_idx( other->get_pattern_idx() ),
	  __midi_msg( other->get_midi_msg() ),
	  __note_off( other->get_note_off() ),
	  __just_recorded( other->get_just_recorded() ),
	  __probability( other->get_probability() )
{
	if ( instrument != 0 ) __instrument = instrument;
	if ( __instrument != 0 ) {
		__adsr = __instrument->copy_adsr();
		__instrument_id = __instrument->get_id();

		for (std::vector<InstrumentComponent*>::iterator it = __instrument->get_components()->begin() ; it !=__instrument->get_components()->end(); ++it) {
			InstrumentComponent *pCompo = *it;

			SelectedLayerInfo *sampleInfo = new SelectedLayerInfo;
			sampleInfo->SelectedLayer = -1;
			sampleInfo->SamplePosition = 0;

			__layers_selected[ pCompo->get_drumkit_componentID() ] = sampleInfo;
		}
	}
}

Note::~Note()
{
	delete __adsr;
	__adsr = 0;
}

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

void Note::set_probability( float probability ){
	__probability = check_boundary( probability, PROBABILITY_MIN, PROBABILITY_MAX );
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
		if( __key_str[i]==s_key ) {
			__key = ( Key )i;
			return;
		}
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
	node->write_float( "probability", __probability );
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
	note->set_instrument_id( node->read_int( "instrument", EMPTY_INSTR_ID ) );
	note->map_instrument( instruments );
	note->set_probability( node->read_float( "probability", 1.0f ));

	return note;
}
/**
 * Returns a pointer to a string version of the internally used
 * H2Core::NotePropertiesMode enumeration object. In case non of the
 * provided enumerators do match this class, which should not happen,
 * a reference to the string "UNRECOGNIZED" will be returned.
 * /param notePropertiesMode Enumeration determining the property of
 * the note, which was altered during the last action.
 */
const char* convertNotePropertiesModeToString( NotePropertiesMode notePropertiesMode ){
	switch( notePropertiesMode )
		{
		case NotePropertiesMode::VELOCITY :
			return NotePropertiesModeStrings[ 0 ];
		case NotePropertiesMode::PAN :
			return NotePropertiesModeStrings[ 1 ];
		case NotePropertiesMode::LEADLAG :
			return NotePropertiesModeStrings[ 2 ];
		case NotePropertiesMode::NOTEKEY :
			return NotePropertiesModeStrings[ 3 ];
		case NotePropertiesMode::PROBABILITY :
			return NotePropertiesModeStrings[ 4 ];
		default :
			return NotePropertiesModeStrings[ 5 ];
		}
}
};

/* vim: set softtabstop=4 noexpandtab: */
