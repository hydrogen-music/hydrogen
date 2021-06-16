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

#include <core/Basics/Instrument.h>

#include <cassert>

#include <core/Hydrogen.h>
#include <core/AudioEngine.h>

#include <core/Helpers/Xml.h>

#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Sampler/Sampler.h>

namespace H2Core
{

const char* Instrument::__class_name = "Instrument";

Instrument::Instrument( const int id, const QString& name, std::shared_ptr<ADSR> adsr )
	: Object( __class_name )
	, __id( id )
	, __name( name )
	, __gain( 1.0 )
	, __volume( 1.0 )
	, m_fPan( 0.f )
	, __peak_l( 0.0 )
	, __peak_r( 0.0 )
	, __adsr( adsr )
	, __filter_active( false )
	, __filter_cutoff( 1.0 )
	, __filter_resonance( 0.0 )
	, __pitch_offset( 0.0 )
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
		__adsr = std::make_shared<ADSR>();
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
	__components = new std::vector<std::shared_ptr<InstrumentComponent>>();
}

Instrument::Instrument( std::shared_ptr<Instrument> other )
	: Object( __class_name )
	, __id( other->get_id() )
	, __name( other->get_name() )
	, __gain( other->__gain )
	, __volume( other->get_volume() )
	, m_fPan( other->getPan() )
	, __peak_l( other->get_peak_l() )
	, __peak_r( other->get_peak_r() )
	, __adsr( std::make_shared<ADSR>( *( other->get_adsr() ) ) )
	, __filter_active( other->is_filter_active() )
	, __filter_cutoff( other->get_filter_cutoff() )
	, __filter_resonance( other->get_filter_resonance() )
	, __pitch_offset( other->get_pitch_offset() )
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

	__components = new std::vector<std::shared_ptr<InstrumentComponent>>();
	for ( auto& pComponent : *other->get_components() ) {
		__components->push_back( std::make_shared<InstrumentComponent>( pComponent ) );
	}
}

Instrument::~Instrument()
{
	delete __components;
}

std::shared_ptr<Instrument> Instrument::load_instrument( const QString& drumkit_name, const QString& instrument_name, Filesystem::Lookup lookup )
{
	auto pInstrument = std::make_shared<Instrument>();
	pInstrument->load_from( drumkit_name, instrument_name, false, lookup );
	return pInstrument;
}

void Instrument::load_from( Drumkit* pDrumkit, std::shared_ptr<Instrument> pInstrument, bool is_live )
{
	AudioEngine* pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	if ( is_live ) {
		pAudioEngine->lock( RIGHT_HERE );
	}
	
	this->get_components()->clear();
	
	if ( is_live ) {
		pAudioEngine->unlock();
	}

	set_missing_samples( false );

	for ( const auto& pSrcComponent : *pInstrument->get_components() ) {
		auto pMyComponent = std::make_shared<InstrumentComponent>( pSrcComponent->get_drumkit_componentID() );
		pMyComponent->set_gain( pSrcComponent->get_gain() );

		this->get_components()->push_back( pMyComponent );

		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			auto src_layer = pSrcComponent->get_layer( i );
			auto my_layer = pMyComponent->get_layer( i );

			if( src_layer == nullptr ) {
				if ( is_live ) {
					pAudioEngine->lock( RIGHT_HERE );
				}
				pMyComponent->set_layer( nullptr, i );
				if ( is_live ) {
					pAudioEngine->unlock();
				}
			} else {
				QString sample_path =  pDrumkit->get_path() + "/" + src_layer->get_sample()->get_filename();
				auto pSample = Sample::load( sample_path );
				if ( pSample == nullptr ) {
					_ERRORLOG( QString( "Error loading sample %1. Creating a new empty layer." ).arg( sample_path ) );
					set_missing_samples( true );
					if ( is_live ) {
						pAudioEngine->lock( RIGHT_HERE );
					}
					pMyComponent->set_layer( nullptr, i );
					
					if ( is_live ) {
						pAudioEngine->unlock();
					}
				} else {
					if ( is_live ) {
						pAudioEngine->lock( RIGHT_HERE );
					}
					pMyComponent->set_layer( std::make_shared<InstrumentLayer>( src_layer, pSample ), i );
					if ( is_live ) {
						pAudioEngine->unlock();
					}
				}
			}
			my_layer = nullptr;
		}
	}
	if ( is_live ) {
		pAudioEngine->lock( RIGHT_HERE );
	}

	this->set_id( pInstrument->get_id() );
	this->set_name( pInstrument->get_name() );
	this->set_drumkit_name( pDrumkit->get_name() );
	this->set_gain( pInstrument->get_gain() );
	this->set_volume( pInstrument->get_volume() );
	this->setPan( pInstrument->getPan() );
	this->set_adsr( std::make_shared<ADSR>( *( pInstrument->get_adsr() ) ) );
	this->set_filter_active( pInstrument->is_filter_active() );
	this->set_filter_cutoff( pInstrument->get_filter_cutoff() );
	this->set_filter_resonance( pInstrument->get_filter_resonance() );
	this->set_pitch_offset( pInstrument->get_pitch_offset() );
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
		pAudioEngine->unlock();
	}
}

void Instrument::load_from( const QString& dk_name, const QString& instrument_name, bool is_live, Filesystem::Lookup lookup )
{
	Drumkit* pDrumkit = Drumkit::load_by_name( dk_name, false, lookup );
	if ( !pDrumkit ) {
		return;
	}

	assert( pDrumkit );

	auto pInstrument = pDrumkit->get_instruments()->find( instrument_name );
	if ( pInstrument!=nullptr ) {
		load_from( pDrumkit, pInstrument, is_live );
	}
	
	delete pDrumkit;
}

std::shared_ptr<Instrument> Instrument::load_from( XMLNode* node, const QString& dk_path, const QString& dk_name )
{
	int id = node->read_int( "id", EMPTY_INSTR_ID, false, false );
	if ( id == EMPTY_INSTR_ID ) {
		return nullptr;
	}

	auto pInstrument = std::make_shared<Instrument>( id, node->read_string( "name", "" ), nullptr );
	pInstrument->set_drumkit_name( dk_name );
	pInstrument->set_volume( node->read_float( "volume", 1.0f ) );
	pInstrument->set_muted( node->read_bool( "isMuted", false ) );
	bool bFound, bFound2;
	float fPan = node->read_float( "pan", 0.f, &bFound );
	if ( !bFound ) {
		// check if pan is expressed in the old fashion (version <= 1.1 ) with the pair (pan_L, pan_R)
		float fPanL = node->read_float( "pan_L", 1.f, &bFound );
		float fPanR = node->read_float( "pan_R", 1.f, &bFound2 );
		if ( bFound == true && bFound2 == true ) { // found nodes pan_L and pan_R
			fPan = Sampler::getRatioPan( fPanL, fPanR );  // convert to single pan parameter
		}
	}
	pInstrument->setPan( fPan );
	
	// may not exist, but can't be empty
	pInstrument->set_apply_velocity( node->read_bool( "applyVelocity", true, false ) );
	pInstrument->set_filter_active( node->read_bool( "filterActive", true, false ) );
	pInstrument->set_filter_cutoff( node->read_float( "filterCutoff", 1.0f, true, false ) );
	pInstrument->set_filter_resonance( node->read_float( "filterResonance", 0.0f, true, false ) );
	pInstrument->set_pitch_offset( node->read_float( "pitchOffset", 0.0f, true, false ) );
	pInstrument->set_random_pitch_factor( node->read_float( "randomPitchFactor", 0.0f, true, false ) );
	float attack = node->read_float( "Attack", 0.0f, true, false );
	float decay = node->read_float( "Decay", 0.0f, true, false  );
	float sustain = node->read_float( "Sustain", 1.0f, true, false );
	float release = node->read_float( "Release", 1000.0f, true, false );
	pInstrument->set_adsr( std::make_shared<ADSR>( attack, decay, sustain, release ) );
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
	for ( auto& pComponent : *get_components() ) {
		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			auto pLayer = pComponent->get_layer( i );
			if( pLayer ) {
				pLayer->load_sample();
			}
		}
	}
}

void Instrument::unload_samples()
{
	for ( auto& pComponent : *get_components() ) {
		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			auto pLayer = pComponent->get_layer( i );
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
	InstrumentNode.write_bool( "isSoloed", __soloed );
	InstrumentNode.write_float( "pan", m_fPan );
	InstrumentNode.write_float( "pitchOffset", __pitch_offset );
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
	for ( auto& pComponent : *__components ) {
		if( component_id == -1 || pComponent->get_drumkit_componentID() == component_id ) {
			pComponent->save_to( &InstrumentNode, component_id );
		}
	}
}

void Instrument::set_adsr( std::shared_ptr<ADSR> adsr )
{
	__adsr = adsr;
}

std::shared_ptr<InstrumentComponent> Instrument::get_component( int DrumkitComponentID )
{
	for ( const auto& pComponent : *get_components() ) {
		if( pComponent->get_drumkit_componentID() == DrumkitComponentID ) {
			return pComponent;
		}
	}

	return nullptr;
}

QString Instrument::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Object::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Instrument]\n" ).arg( sPrefix )
			.append( QString( "%1%2id: %3\n" ).arg( sPrefix ).arg( s ).arg( __id ) )
			.append( QString( "%1%2name: %3\n" ).arg( sPrefix ).arg( s ).arg( __name ) )
			.append( QString( "%1%2drumkit_name: %3\n" ).arg( sPrefix ).arg( s ).arg( __drumkit_name ) )
			.append( QString( "%1%2gain: %3\n" ).arg( sPrefix ).arg( s ).arg( __gain ) )
			.append( QString( "%1%2volume: %3\n" ).arg( sPrefix ).arg( s ).arg( __volume ) )
			.append( QString( "%1%2pan: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fPan ) )
			.append( QString( "%1%2peak_l: %3\n" ).arg( sPrefix ).arg( s ).arg( __peak_l ) )
			.append( QString( "%1%2peak_r: %3\n" ).arg( sPrefix ).arg( s ).arg( __peak_r ) )
			.append( QString( "%1" ).arg( __adsr->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2filter_active: %3\n" ).arg( sPrefix ).arg( s ).arg( __filter_active ) )
			.append( QString( "%1%2filter_cutoff: %3\n" ).arg( sPrefix ).arg( s ).arg( __filter_cutoff ) )
			.append( QString( "%1%2filter_resonance: %3\n" ).arg( sPrefix ).arg( s ).arg( __filter_resonance ) )
			.append( QString( "%1%2random_pitch_factor: %3\n" ).arg( sPrefix ).arg( s ).arg( __random_pitch_factor ) )
			.append( QString( "%1%2pitch_offset: %3\n" ).arg( sPrefix ).arg( s ).arg( __pitch_offset ) )
			.append( QString( "%1%2midi_out_note: %3\n" ).arg( sPrefix ).arg( s ).arg( __midi_out_note ) )
			.append( QString( "%1%2midi_out_channel: %3\n" ).arg( sPrefix ).arg( s ).arg( __midi_out_channel ) )
			.append( QString( "%1%2stop_notes: %3\n" ).arg( sPrefix ).arg( s ).arg( __stop_notes ) )
			.append( QString( "%1%2sample_selection_alg: %3\n" ).arg( sPrefix ).arg( s ).arg( __sample_selection_alg ) )
			.append( QString( "%1%2active: %3\n" ).arg( sPrefix ).arg( s ).arg( __active ) )
			.append( QString( "%1%2soloed: %3\n" ).arg( sPrefix ).arg( s ).arg( __soloed ) )
			.append( QString( "%1%2muted: %3\n" ).arg( sPrefix ).arg( s ).arg( __muted ) )
			.append( QString( "%1%2mute_group: %3\n" ).arg( sPrefix ).arg( s ).arg( __mute_group ) )
			.append( QString( "%1%2queued: %3\n" ).arg( sPrefix ).arg( s ).arg( __queued ) ) ;
		sOutput.append( QString( "%1%2fx_level: [ " ).arg( sPrefix ).arg( s ) );
		for ( auto ff : __fx_level ) {
			sOutput.append( QString( "%1 " ).arg( ff ) );
		}
		sOutput.append( QString( "]\n" ) )
			.append( QString( "%1%2hihat_grp: %3\n" ).arg( sPrefix ).arg( s ).arg( __hihat_grp ) )
			.append( QString( "%1%2lower_cc: %3\n" ).arg( sPrefix ).arg( s ).arg( __lower_cc ) )
			.append( QString( "%1%2higher_cc: %3\n" ).arg( sPrefix ).arg( s ).arg( __higher_cc ) )
			.append( QString( "%1%2is_preview_instrument: %3\n" ).arg( sPrefix ).arg( s ).arg( __is_preview_instrument ) )
			.append( QString( "%1%2is_metronome_instrument: %3\n" ).arg( sPrefix ).arg( s ).arg( __is_metronome_instrument ) )
			.append( QString( "%1%2apply_velocity: %3\n" ).arg( sPrefix ).arg( s ).arg( __apply_velocity ) )
			.append( QString( "%1%2current_instr_for_export: %3\n" ).arg( sPrefix ).arg( s ).arg( __current_instr_for_export ) )
			.append( QString( "%1%2m_bHasMissingSamples: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bHasMissingSamples ) )
			.append( QString( "%1%2components:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto cc : *__components ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( "%1" ).arg( cc->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	} else {
		
		sOutput = QString( "[Instrument]" )
			.append( QString( " id: %1" ).arg( __id ) )
			.append( QString( ", name: %1" ).arg( __name ) )
			.append( QString( ", drumkit_name: %1" ).arg( __drumkit_name ) )
			.append( QString( ", gain: %1" ).arg( __gain ) )
			.append( QString( ", volume: %1" ).arg( __volume ) )
			.append( QString( ", pan: %1" ).arg( m_fPan ) )
			.append( QString( ", peak_l: %1" ).arg( __peak_l ) )
			.append( QString( ", peak_r: %1" ).arg( __peak_r ) )
			.append( QString( ", [%1" ).arg( __adsr->toQString( sPrefix + s, bShort ).replace( "\n", "]" ) ) )
			.append( QString( ", filter_active: %1" ).arg( __filter_active ) )
			.append( QString( ", filter_cutoff: %1" ).arg( __filter_cutoff ) )
			.append( QString( ", filter_resonance: %1" ).arg( __filter_resonance ) )
			.append( QString( ", random_pitch_factor: %1" ).arg( __random_pitch_factor ) )
			.append( QString( ", pitch_offset: %1" ).arg( __pitch_offset ) )
			.append( QString( ", midi_out_note: %1" ).arg( __midi_out_note ) )
			.append( QString( ", midi_out_channel: %1" ).arg( __midi_out_channel ) )
			.append( QString( ", stop_notes: %1" ).arg( __stop_notes ) )
			.append( QString( ", sample_selection_alg: %1" ).arg( __sample_selection_alg ) )
			.append( QString( ", active: %1" ).arg( __active ) )
			.append( QString( ", soloed: %1" ).arg( __soloed ) )
			.append( QString( ", muted: %1" ).arg( __muted ) )
			.append( QString( ", mute_group: %1" ).arg( __mute_group ) )
			.append( QString( ", queued: %1" ).arg( __queued ) ) ;
		sOutput.append( QString( ", fx_level: [ " ) );
		for ( auto ff : __fx_level ) {
			sOutput.append( QString( "%1 " ).arg( ff ) );
		}
		sOutput.append( QString( "]" ) )
			.append( QString( ", hihat_grp: %1" ).arg( __hihat_grp ) )
			.append( QString( ", lower_cc: %1" ).arg( __lower_cc ) )
			.append( QString( ", higher_cc: %1" ).arg( __higher_cc ) )
			.append( QString( ", is_preview_instrument: %1" ).arg( __is_preview_instrument ) )
			.append( QString( ", is_metronome_instrument: %1" ).arg( __is_metronome_instrument ) )
			.append( QString( ", apply_velocity: %1" ).arg( __apply_velocity ) )
			.append( QString( ", current_instr_for_export: %1" ).arg( __current_instr_for_export ) )
			.append( QString( ", m_bHasMissingSamples: %1" ).arg( m_bHasMissingSamples ) )
			.append( QString( ", components: [" ) );
		for ( auto cc : *__components ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( " %1" ).arg( cc->get_drumkit_componentID() ) );
			}
		}
		sOutput.append(" ]\n");
	}
		
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
