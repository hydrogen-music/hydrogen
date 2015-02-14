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

#include <hydrogen/basics/instrument.h>

#include <cassert>

#include <hydrogen/audio_engine.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>

#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_layer.h>

namespace H2Core
{

const char* Instrument::__class_name = "Instrument";

Instrument::Instrument( const int id, const QString& name, ADSR* adsr )
	: Object( __class_name )
	, __id( id )
	, __name( name )
	, __gain( 1.0 )
	, __volume( 1.0 )
	, __pan_l( 1.0 )
	, __pan_r( 1.0 )
	, __peak_l( 0.0 )
	, __peak_r( 0.0 )
	, __adsr( adsr )
	, __filter_active( false )
	, __filter_cutoff( 1.0 )
	, __filter_resonance( 0.0 )
	, __random_pitch_factor( 0.0 )
	, __midi_out_note( 36 + id )
	, __midi_out_channel( -1 )
	, __stop_notes( false )
	, __active( true )
	, __soloed( false )
	, __muted( false )
	, __mute_group( -1 )
	, __queued( 0 )
	, __hihat( false )
	, __lower_cc( 0 )
	, __higher_cc( 127 )
	, __components( NULL )
{
	if ( __adsr==0 ) __adsr = new ADSR();
	for ( int i=0; i<MAX_FX; i++ ) __fx_level[i] = 0.0;
	__components = new std::vector<InstrumentComponent*> ();
}

Instrument::Instrument( Instrument* other )
	: Object( __class_name )
	, __id( other->get_id() )
	, __name( other->get_name() )
	, __gain( other->__gain )
	, __volume( other->get_volume() )
	, __pan_l( other->get_pan_l() )
	, __pan_r( other->get_pan_r() )
	, __peak_l( other->get_peak_l() )
	, __peak_r( other->get_peak_r() )
	, __adsr( new ADSR( *( other->get_adsr() ) ) )
	, __filter_active( other->is_filter_active() )
	, __filter_cutoff( other->get_filter_cutoff() )
	, __filter_resonance( other->get_filter_resonance() )
	, __random_pitch_factor( other->get_random_pitch_factor() )
	, __midi_out_note( other->get_midi_out_note() )
	, __midi_out_channel( other->get_midi_out_channel() )
	, __stop_notes( other->is_stop_notes() )
	, __active( other->is_active() )
	, __soloed( other->is_soloed() )
	, __muted( other->is_muted() )
	, __mute_group( other->get_mute_group() )
	, __queued( other->is_queued() )
	, __hihat( other->is_hihat() )
	, __lower_cc( other->get_lower_cc() )
	, __higher_cc( other->get_higher_cc() )
	, __components( NULL )
{
	for ( int i=0; i<MAX_FX; i++ ) __fx_level[i] = other->get_fx_level( i );

	__components = new std::vector<InstrumentComponent*> ();
    __components->assign( other->get_components()->begin(), other->get_components()->end() );
}

Instrument::~Instrument()
{
	__components->clear();
	delete __components;

	delete __adsr;
	__adsr = 0;
}

Instrument* Instrument::load_instrument( const QString& drumkit_name, const QString& instrument_name )
{
	Instrument* i = new Instrument();
	i->load_from( drumkit_name, instrument_name, false );
	return i;
}

void Instrument::load_from( Drumkit* drumkit, Instrument* instrument, bool is_live )
{
    this->get_components()->clear();

    for (std::vector<InstrumentComponent*>::iterator it = instrument->get_components()->begin() ; it != instrument->get_components()->end(); ++it) {
        InstrumentComponent* src_component = *it;

        InstrumentComponent* my_component = new InstrumentComponent( src_component->get_drumkit_componentID() );
        my_component->set_gain( src_component->get_gain() );

        this->get_components()->push_back( my_component );

        for ( int i=0; i<MAX_LAYERS; i++ ) {
            InstrumentLayer* src_layer = src_component->get_layer( i );
            InstrumentLayer* my_layer = my_component->get_layer( i );

            if( src_layer==0 ) {
                if ( is_live )
                    AudioEngine::get_instance()->lock( RIGHT_HERE );
                my_component->set_layer( NULL, i );
                if ( is_live )
                    AudioEngine::get_instance()->unlock();
            } else {
                QString sample_path =  drumkit->get_path() + "/" + src_layer->get_sample()->get_filename();
                Sample* sample = Sample::load( sample_path );
                if ( sample==0 ) {
                    _ERRORLOG( QString( "Error loading sample %1. Creating a new empty layer." ).arg( sample_path ) );
                    if ( is_live )
                        AudioEngine::get_instance()->lock( RIGHT_HERE );
                    my_component->set_layer( NULL, i );
                    if ( is_live )
                        AudioEngine::get_instance()->unlock();
                } else {
                    if ( is_live )
                        AudioEngine::get_instance()->lock( RIGHT_HERE );
                    my_component->set_layer( new InstrumentLayer( src_layer, sample ), i );
                    if ( is_live )
                        AudioEngine::get_instance()->unlock();
                }
            }
            delete my_layer;
        }
	}
	if ( is_live )
		AudioEngine::get_instance()->lock( RIGHT_HERE );

	this->set_id( instrument->get_id() );
	this->set_name( instrument->get_name() );
	this->set_drumkit_name( drumkit->get_name() );
	this->set_gain( instrument->get_gain() );
	this->set_volume( instrument->get_volume() );
	this->set_pan_l( instrument->get_pan_l() );
	this->set_pan_r( instrument->get_pan_r() );
	this->set_adsr( new ADSR( *( instrument->get_adsr() ) ) );
	this->set_filter_active( instrument->is_filter_active() );
	this->set_filter_cutoff( instrument->get_filter_cutoff() );
	this->set_filter_resonance( instrument->get_filter_resonance() );
	this->set_random_pitch_factor( instrument->get_random_pitch_factor() );
	this->set_muted( instrument->is_muted() );
	this->set_mute_group( instrument->get_mute_group() );
	this->set_midi_out_channel( instrument->get_midi_out_channel() );
	this->set_midi_out_note( instrument->get_midi_out_note() );
	this->set_stop_notes( instrument->is_stop_notes() );
	this->set_hihat( instrument->is_hihat() );
	this->set_lower_cc( instrument->get_lower_cc() );
	this->set_higher_cc( instrument->get_higher_cc() );
	if ( is_live )
		AudioEngine::get_instance()->unlock();
}

void Instrument::load_from( const QString& dk_name, const QString& instrument_name, bool is_live )
{
	Drumkit* drumkit = Drumkit::load_by_name( dk_name );
	if ( ! drumkit ) return;
	assert( drumkit );
	Instrument* instrument = drumkit->get_instruments()->find( instrument_name );
	if ( instrument!=0 ) {
		load_from( drumkit, instrument, is_live );
	}
	delete drumkit;
}

Instrument* Instrument::load_from( XMLNode* node, const QString& dk_path, const QString& dk_name )
{
	int id = node->read_int( "id", EMPTY_INSTR_ID, false, false );
	if ( id==EMPTY_INSTR_ID ) return 0;
	Instrument* instrument = new Instrument( id, node->read_string( "name", "" ), 0 );
	instrument->set_drumkit_name( dk_name );
	instrument->set_volume( node->read_float( "volume", 1.0f ) );
	instrument->set_muted( node->read_bool( "isMuted", false ) );
	instrument->set_pan_l( node->read_float( "pan_L", 1.0f ) );
	instrument->set_pan_r( node->read_float( "pan_R", 1.0f ) );
	// may not exist, but can't be empty
	instrument->set_filter_active( node->read_bool( "filterActive", true, false ) );
	instrument->set_filter_cutoff( node->read_float( "filterCutoff", 1.0f, true, false ) );
	instrument->set_filter_resonance( node->read_float( "filterResonance", 0.0f, true, false ) );
	instrument->set_random_pitch_factor( node->read_float( "randomPitchFactor", 0.0f, true, false ) );
	float attack = node->read_float( "Attack", 0.0f, true, false );
	float decay = node->read_float( "Decay", 0.0f, true, false  );
	float sustain = node->read_float( "Sustain", 1.0f, true, false );
	float release = node->read_float( "Release", 1000.0f, true, false );
	instrument->set_adsr( new ADSR( attack, decay, sustain, release ) );
	instrument->set_gain( node->read_float( "gain", 1.0f, true, false ) );
	instrument->set_mute_group( node->read_int( "muteGroup", -1, true, false ) );
	instrument->set_midi_out_channel( node->read_int( "midiOutChannel", -1, true, false ) );
	instrument->set_midi_out_note( node->read_int( "midiOutNote", instrument->__midi_out_note, true, false ) );
	instrument->set_stop_notes( node->read_bool( "isStopNote", true ,false ) );
	instrument->set_hihat( node->read_bool( "isHihat", false, true ) );
	instrument->set_lower_cc( node->read_int( "lower_cc", 0, true ) );
	instrument->set_higher_cc( node->read_int( "higher_cc", 127, true ) );

	for ( int i=0; i<MAX_FX; i++ ) {
		instrument->set_fx_level( node->read_float( QString( "FX%1Level" ).arg( i+1 ), 0.0 ), i );
	}
	XMLNode component_node = node->firstChildElement( "instrumentComponent" );
	while ( !component_node.isNull() ) {
		instrument->get_components()->push_back( InstrumentComponent::load_from( &component_node, dk_path ) );
        component_node = component_node.nextSiblingElement( "instrumentComponent" );
	}
	return instrument;
}

void Instrument::load_samples()
{
    for (std::vector<InstrumentComponent*>::iterator it = get_components()->begin() ; it != get_components()->end(); ++it) {
        InstrumentComponent* component = *it;
        for ( int i=0; i<MAX_LAYERS; i++ ) {
            InstrumentLayer* layer = component->get_layer( i );
                if( layer ) layer->load_sample( );
        }
    }
}

void Instrument::unload_samples()
{
    for (std::vector<InstrumentComponent*>::iterator it = get_components()->begin() ; it != get_components()->end(); ++it) {
        InstrumentComponent* component = *it;
        for ( int i=0; i<MAX_LAYERS; i++ ) {
            InstrumentLayer* layer = component->get_layer( i );
            if( layer ) layer->unload_sample();
        }
    }
}

void Instrument::save_to( XMLNode* node )
{
	XMLNode instrument_node = node->ownerDocument().createElement( "instrument" );
	instrument_node.write_int( "id", __id );
	instrument_node.write_string( "name", __name );
	instrument_node.write_float( "volume", __volume );
	instrument_node.write_bool( "isMuted", __muted );
	instrument_node.write_float( "pan_L", __pan_l );
	instrument_node.write_float( "pan_R", __pan_r );
	instrument_node.write_float( "randomPitchFactor", __random_pitch_factor );
	instrument_node.write_float( "gain", __gain );
	instrument_node.write_bool( "filterActive", __filter_active );
	instrument_node.write_float( "filterCutoff", __filter_cutoff );
	instrument_node.write_float( "filterResonance", __filter_resonance );
	instrument_node.write_float( "Attack", __adsr->get_attack() );
	instrument_node.write_float( "Decay", __adsr->get_decay() );
	instrument_node.write_float( "Sustain", __adsr->get_sustain() );
	instrument_node.write_float( "Release", __adsr->get_release() );
	instrument_node.write_int( "muteGroup", __mute_group );
	instrument_node.write_int( "midiOutChannel", __midi_out_channel );
	instrument_node.write_int( "midiOutNote", __midi_out_note );
	instrument_node.write_bool( "isStopNote", __stop_notes );
	instrument_node.write_bool( "isHihat", __hihat );
	instrument_node.write_int( "lower_cc", __lower_cc );
	instrument_node.write_int( "higher_cc", __higher_cc );
	for ( int i=0; i<MAX_FX; i++ ) {
		instrument_node.write_float( QString( "FX%1Level" ).arg( i+1 ), __fx_level[i] );
	}
	for (std::vector<InstrumentComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
        InstrumentComponent* component = *it;
		component->save_to( &instrument_node );

	}
	node->appendChild( instrument_node );
}

void Instrument::set_adsr( ADSR* adsr )
{
	if( __adsr ) delete __adsr;
	__adsr = adsr;
}

InstrumentComponent* Instrument::get_component( int DrumkitComponentID )
{
    for (std::vector<InstrumentComponent*>::iterator it = get_components()->begin() ; it != get_components()->end(); ++it) {
         if( (*it)->get_drumkit_componentID() == DrumkitComponentID )
             return *it;
    }

    return NULL;
}

};

/* vim: set softtabstop=4 expandtab: */
