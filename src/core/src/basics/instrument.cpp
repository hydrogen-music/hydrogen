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
	, __sample_selection_alg( VELOCITY )
	, __active( true )
	, __soloed( false )
	, __muted( false )
	, __mute_group( -1 )
	, __queued( 0 )
	, __hihat_grp( -1 )
	, __lower_cc( 0 )
	, __higher_cc( 127 )
	, __components( nullptr )
	, __is_preview_instrument(false)
	, __is_metronome_instrument(false)
	, __apply_velocity( true )
	, __current_instr_for_export(false)
	, m_bHasMissingSamples( false )
{
	if ( __adsr == nullptr ) {
		__adsr = new ADSR();
	}

    if( __midi_out_note < MIDI_OUT_NOTE_MIN ){
		__midi_out_note = MIDI_OUT_NOTE_MIN;	
	}
	
	if( __midi_out_note > MIDI_OUT_NOTE_MAX ){
		__midi_out_note = MIDI_OUT_NOTE_MAX;	
	}
	
	for ( int i=0; i<MAX_FX; i++ ) {
		__fx_level[i] = 0.0;
	}
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
	, __sample_selection_alg( other->sample_selection_alg() )
	, __active( other->is_active() )
	, __soloed( other->is_soloed() )
	, __muted( other->is_muted() )
	, __mute_group( other->get_mute_group() )
	, __queued( other->is_queued() )
	, __hihat_grp( other->get_hihat_grp() )
	, __lower_cc( other->get_lower_cc() )
	, __higher_cc( other->get_higher_cc() )
	, __components( nullptr )
	, __is_preview_instrument(false)
	, __is_metronome_instrument(false)
	, __apply_velocity( other->get_apply_velocity() )
	, __current_instr_for_export(false)
{
	for ( int i=0; i<MAX_FX; i++ ) {
		__fx_level[i] = other->get_fx_level( i );
	}

	__components = new std::vector<InstrumentComponent*> ();
	for (auto it = other->get_components()->begin(); it != other->get_components()->end(); ++it) {
		__components->push_back(new InstrumentComponent(*it));
	}
}

Instrument::~Instrument()
{
	for(auto& pComponent : *this->get_components()){
		delete pComponent;
	}	

	delete __components;

	delete __adsr;
	__adsr = nullptr;
}

Instrument* Instrument::load_instrument( const QString& drumkit_name, const QString& instrument_name )
{
	Instrument* pInstrument = new Instrument();
	pInstrument->load_from( drumkit_name, instrument_name, false );
	return pInstrument;
}

void Instrument::load_from( Drumkit* pDrumkit, Instrument* pInstrument, bool is_live )
{
	if ( is_live ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
	}
	
	for(auto& pComponent : *this->get_components()){
		delete pComponent;
	}
	
	this->get_components()->clear();
	
	if ( is_live ) {
		AudioEngine::get_instance()->unlock();
	}

	set_missing_samples( false );

	for (std::vector<InstrumentComponent*>::iterator it = pInstrument->get_components()->begin() ; it != pInstrument->get_components()->end(); ++it) {
		InstrumentComponent* pSrcComponent = *it;

		InstrumentComponent* pMyComponent = new InstrumentComponent( pSrcComponent->get_drumkit_componentID() );
		pMyComponent->set_gain( pSrcComponent->get_gain() );

		this->get_components()->push_back( pMyComponent );

		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			InstrumentLayer* src_layer = pSrcComponent->get_layer( i );
			InstrumentLayer* my_layer = pMyComponent->get_layer( i );

			if( src_layer == nullptr ) {
				if ( is_live ) {
					AudioEngine::get_instance()->lock( RIGHT_HERE );
				}
				pMyComponent->set_layer( nullptr, i );
				if ( is_live ) {
					AudioEngine::get_instance()->unlock();
				}
			} else {
				QString sample_path =  pDrumkit->get_path() + "/" + src_layer->get_sample()->get_filename();
				Sample* pSample = Sample::load( sample_path );
				if ( pSample == nullptr ) {
					_ERRORLOG( QString( "Error loading sample %1. Creating a new empty layer." ).arg( sample_path ) );
					set_missing_samples( true );
					if ( is_live ) {
						AudioEngine::get_instance()->lock( RIGHT_HERE );
					}
					pMyComponent->set_layer( nullptr, i );
					
					if ( is_live ) {
						AudioEngine::get_instance()->unlock();
					}
				} else {
					if ( is_live ) {
						AudioEngine::get_instance()->lock( RIGHT_HERE );
					}
					pMyComponent->set_layer( new InstrumentLayer( src_layer, pSample ), i );
					if ( is_live ) {
						AudioEngine::get_instance()->unlock();
					}
				}
			}
			delete my_layer;
		}
	}
	if ( is_live ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
	}

	this->set_id( pInstrument->get_id() );
	this->set_name( pInstrument->get_name() );
	this->set_drumkit_name( pDrumkit->get_name() );
	this->set_gain( pInstrument->get_gain() );
	this->set_volume( pInstrument->get_volume() );
	this->set_pan_l( pInstrument->get_pan_l() );
	this->set_pan_r( pInstrument->get_pan_r() );
	this->set_adsr( new ADSR( *( pInstrument->get_adsr() ) ) );
	this->set_filter_active( pInstrument->is_filter_active() );
	this->set_filter_cutoff( pInstrument->get_filter_cutoff() );
	this->set_filter_resonance( pInstrument->get_filter_resonance() );
	this->set_random_pitch_factor( pInstrument->get_random_pitch_factor() );
	this->set_muted( pInstrument->is_muted() );
	this->set_mute_group( pInstrument->get_mute_group() );
	this->set_midi_out_channel( pInstrument->get_midi_out_channel() );
	this->set_midi_out_note( pInstrument->get_midi_out_note() );
	this->set_stop_notes( pInstrument->is_stop_notes() );
	this->set_sample_selection_alg( pInstrument->sample_selection_alg() );
	this->set_hihat_grp( pInstrument->get_hihat_grp() );
	this->set_lower_cc( pInstrument->get_lower_cc() );
	this->set_higher_cc( pInstrument->get_higher_cc() );
	this->set_apply_velocity ( pInstrument->get_apply_velocity() );
	
	if ( is_live ) {
		AudioEngine::get_instance()->unlock();
	}
}

void Instrument::load_from( const QString& dk_name, const QString& instrument_name, bool is_live )
{
	Drumkit* pDrumkit = Drumkit::load_by_name( dk_name );
	if ( !pDrumkit ) {
		return;
	}

	assert( pDrumkit );

	Instrument* pInstrument = pDrumkit->get_instruments()->find( instrument_name );
	if ( pInstrument!=nullptr ) {
		load_from( pDrumkit, pInstrument, is_live );
	}
	
	delete pDrumkit;
}

Instrument* Instrument::load_from( XMLNode* node, const QString& dk_path, const QString& dk_name )
{
	int id = node->read_int( "id", EMPTY_INSTR_ID, false, false );
	if ( id == EMPTY_INSTR_ID ) {
		return nullptr;
	}

	Instrument* pInstrument = new Instrument( id, node->read_string( "name", "" ), nullptr );
	pInstrument->set_drumkit_name( dk_name );
	pInstrument->set_volume( node->read_float( "volume", 1.0f ) );
	pInstrument->set_muted( node->read_bool( "isMuted", false ) );
	pInstrument->set_pan_l( node->read_float( "pan_L", 1.0f ) );
	pInstrument->set_pan_r( node->read_float( "pan_R", 1.0f ) );
	// may not exist, but can't be empty
	pInstrument->set_apply_velocity( node->read_bool( "applyVelocity", true, false ) );
	pInstrument->set_filter_active( node->read_bool( "filterActive", true, false ) );
	pInstrument->set_filter_cutoff( node->read_float( "filterCutoff", 1.0f, true, false ) );
	pInstrument->set_filter_resonance( node->read_float( "filterResonance", 0.0f, true, false ) );
	pInstrument->set_random_pitch_factor( node->read_float( "randomPitchFactor", 0.0f, true, false ) );
	float attack = node->read_float( "Attack", 0.0f, true, false );
	float decay = node->read_float( "Decay", 0.0f, true, false  );
	float sustain = node->read_float( "Sustain", 1.0f, true, false );
	float release = node->read_float( "Release", 1000.0f, true, false );
	pInstrument->set_adsr( new ADSR( attack, decay, sustain, release ) );
	pInstrument->set_gain( node->read_float( "gain", 1.0f, true, false ) );
	pInstrument->set_mute_group( node->read_int( "muteGroup", -1, true, false ) );
	pInstrument->set_midi_out_channel( node->read_int( "midiOutChannel", -1, true, false ) );
	pInstrument->set_midi_out_note( node->read_int( "midiOutNote", pInstrument->__midi_out_note, true, false ) );
	pInstrument->set_stop_notes( node->read_bool( "isStopNote", true,false ) );

	QString sRead_sample_select_algo = node->read_string( "sampleSelectionAlgo", "VELOCITY" );
	if ( sRead_sample_select_algo.compare("VELOCITY") == 0 ) {
		pInstrument->set_sample_selection_alg( VELOCITY );
	}
	else if ( sRead_sample_select_algo.compare("ROUND_ROBIN") == 0 ) {
		pInstrument->set_sample_selection_alg( ROUND_ROBIN );
	}
	else if ( sRead_sample_select_algo.compare("RANDOM") == 0 ) {
		pInstrument->set_sample_selection_alg( RANDOM );
	}

	pInstrument->set_hihat_grp( node->read_int( "isHihat", -1, true ) );
	pInstrument->set_lower_cc( node->read_int( "lower_cc", 0, true ) );
	pInstrument->set_higher_cc( node->read_int( "higher_cc", 127, true ) );

	for ( int i=0; i<MAX_FX; i++ ) {
		pInstrument->set_fx_level( node->read_float( QString( "FX%1Level" ).arg( i+1 ), 0.0 ), i );
	}

	XMLNode ComponentNode = node->firstChildElement( "instrumentComponent" );
	while ( !ComponentNode.isNull() ) {
		pInstrument->get_components()->push_back( InstrumentComponent::load_from( &ComponentNode, dk_path ) );
		ComponentNode = ComponentNode.nextSiblingElement( "instrumentComponent" );
	}
	return pInstrument;
}

void Instrument::load_samples()
{
	for (std::vector<InstrumentComponent*>::iterator it = get_components()->begin() ; it != get_components()->end(); ++it) {
		InstrumentComponent* pComponent = *it;
		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			InstrumentLayer* pLayer = pComponent->get_layer( i );
			if( pLayer ) {
				pLayer->load_sample();
			}
		}
	}
}

void Instrument::unload_samples()
{
	for (std::vector<InstrumentComponent*>::iterator it = get_components()->begin() ; it != get_components()->end(); ++it) {
		InstrumentComponent* pComponent = *it;
		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			InstrumentLayer* pLayer = pComponent->get_layer( i );
			if( pLayer ){
				pLayer->unload_sample();
			}
		}
	}
}

void Instrument::save_to( XMLNode* node, int component_id )
{
	XMLNode InstrumentNode = node->createNode( "instrument" );
	InstrumentNode.write_int( "id", __id );
	InstrumentNode.write_string( "name", __name );
	InstrumentNode.write_float( "volume", __volume );
	InstrumentNode.write_bool( "isMuted", __muted );
	InstrumentNode.write_float( "pan_L", __pan_l );
	InstrumentNode.write_float( "pan_R", __pan_r );
	InstrumentNode.write_float( "randomPitchFactor", __random_pitch_factor );
	InstrumentNode.write_float( "gain", __gain );
	InstrumentNode.write_bool( "applyVelocity", __apply_velocity );
	InstrumentNode.write_bool( "filterActive", __filter_active );
	InstrumentNode.write_float( "filterCutoff", __filter_cutoff );
	InstrumentNode.write_float( "filterResonance", __filter_resonance );
	InstrumentNode.write_float( "Attack", __adsr->get_attack() );
	InstrumentNode.write_float( "Decay", __adsr->get_decay() );
	InstrumentNode.write_float( "Sustain", __adsr->get_sustain() );
	InstrumentNode.write_float( "Release", __adsr->get_release() );
	InstrumentNode.write_int( "muteGroup", __mute_group );
	InstrumentNode.write_int( "midiOutChannel", __midi_out_channel );
	InstrumentNode.write_int( "midiOutNote", __midi_out_note );
	InstrumentNode.write_bool( "isStopNote", __stop_notes );

	switch ( __sample_selection_alg ) {
	case VELOCITY:
		InstrumentNode.write_string( "sampleSelectionAlgo", "VELOCITY" );
		break;
	case RANDOM:
		InstrumentNode.write_string( "sampleSelectionAlgo", "RANDOM" );
		break;
	case ROUND_ROBIN:
		InstrumentNode.write_string( "sampleSelectionAlgo", "ROUND_ROBIN" );
		break;
	}

	InstrumentNode.write_int( "isHihat", __hihat_grp );
	InstrumentNode.write_int( "lower_cc", __lower_cc );
	InstrumentNode.write_int( "higher_cc", __higher_cc );

	for ( int i=0; i<MAX_FX; i++ ) {
		InstrumentNode.write_float( QString( "FX%1Level" ).arg( i+1 ), __fx_level[i] );
	}
	for (std::vector<InstrumentComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
		InstrumentComponent* pComponent = *it;
		if( component_id == -1 || pComponent->get_drumkit_componentID() == component_id ) {
			pComponent->save_to( &InstrumentNode, component_id );
		}
	}
}

void Instrument::set_adsr( ADSR* adsr )
{
	if( __adsr ) {
		delete __adsr;
	}
	__adsr = adsr;
}

InstrumentComponent* Instrument::get_component( int DrumkitComponentID )
{
	for (std::vector<InstrumentComponent*>::iterator it = get_components()->begin() ; it != get_components()->end(); ++it) {
		if( (*it)->get_drumkit_componentID() == DrumkitComponentID ) {
			return *it;
		}
	}

	return nullptr;
}

};

/* vim: set softtabstop=4 noexpandtab: */
