/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/Basics/Note.h>

#include <cassert>

#include <core/Helpers/Xml.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>

namespace H2Core
{

const char* Note::__key_str[] = { "C", "Cs", "D", "Ef", "E", "F", "Fs", "G", "Af", "A", "Bf", "B" };

Note::Note( std::shared_ptr<Instrument> instrument, int position, float velocity, float pan, int length, float pitch )
	: __instrument( instrument ),
	  __instrument_id( 0 ),
	  __specific_compo_id( -1 ),
	  __position( position ),
	  __velocity( velocity ),
	  __length( length ),
	  __pitch( pitch ),
	  __key( C ),
	  __octave( P8 ),
	  __adsr( nullptr ),
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
	  __probability( 1.0f ),
	  m_nNoteStart( 0 ),
	  m_fUsedTickSize( std::nan("") )
{
	if ( __instrument != nullptr ) {
		__adsr = __instrument->copy_adsr();
		__instrument_id = __instrument->get_id();

		for ( const auto& pCompo : *__instrument->get_components() ) {

			std::shared_ptr<SelectedLayerInfo> sampleInfo = std::make_shared<SelectedLayerInfo>();
			sampleInfo->SelectedLayer = -1;
			sampleInfo->SamplePosition = 0;

			__layers_selected[ pCompo->get_drumkit_componentID() ] = sampleInfo;
		}
	}

	setPan( pan ); // this checks the boundaries
}

Note::Note( Note* other, std::shared_ptr<Instrument> instrument )
	: Object( *other ),
	  __instrument( other->get_instrument() ),
	  __instrument_id( 0 ),
	  __specific_compo_id( -1 ),
	  __position( other->get_position() ),
	  __velocity( other->get_velocity() ),
	  m_fPan( other->getPan() ),
	  __length( other->get_length() ),
	  __pitch( other->get_pitch() ),
	  __key( other->get_key() ),
	  __octave( other->get_octave() ),
	  __adsr( nullptr ),
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
	  __probability( other->get_probability() ),
	  m_nNoteStart( other->getNoteStart() ),
	  m_fUsedTickSize( other->getUsedTickSize() )
{
	// Either use the provided instrument or fall back to the one
	// stored in the other Note
	if ( instrument != nullptr ) {
		__instrument = instrument;
	}

	if ( __instrument != nullptr ) {
		__adsr = __instrument->copy_adsr();
		__instrument_id = __instrument->get_id();
	}

	for ( const auto& mm : other->__layers_selected ) {
		std::shared_ptr<SelectedLayerInfo> pSampleInfo = std::make_shared<SelectedLayerInfo>();
		pSampleInfo->SelectedLayer = mm.second->SelectedLayer;
		pSampleInfo->SamplePosition = mm.second->SamplePosition;
		
		__layers_selected[ mm.first ] = pSampleInfo;
	}
}

Note::~Note()
{
}

static inline float check_boundary( float v, float min, float max )
{
	if ( v > max ) {
		return max;
	}
	if ( v < min ) {
		return min;
	}
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

void Note::setPan( float val ) {
	m_fPan = check_boundary( val, -1.0f, 1.0f );
}

void Note::map_instrument( InstrumentList* instruments )
{
	assert( instruments );
	auto instr = instruments->find( __instrument_id );
	if( !instr ) {
		ERRORLOG( QString( "Instrument with ID: '%1' not found. Using empty instrument." ).arg( __instrument_id ) );
		__instrument = std::make_shared<Instrument>();
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

bool Note::isPartiallyRendered() const {
	bool bRes = false;

	for ( auto ll : __layers_selected ) {
		if ( ll.second->SamplePosition > 0 ) {
			bRes = true;
			break;
		}
	}

	return bRes;
}

void Note::computeNoteStart() {
	// Notes not inserted via the audio engine but directly, using
	// e.g. the GUI, will be insert at position 0 and don't require a
	// specific start position.
	if ( __position == 0 ) {
		return;
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	double fTickMismatch;
	m_nNoteStart =
		pAudioEngine->computeFrameFromTick( __position, &fTickMismatch );
		
	// If there is a negative Humanize delay, take into account so
	// we don't miss the time slice.  ignore positive delay, or we
	// might end the queue processing prematurely based on NoteQueue
	// placement.  the sampler handles positive delay.
	if ( __humanize_delay < 0 ) {
		m_nNoteStart += __humanize_delay;
	}
	
	if ( pHydrogen->isTimelineEnabled() ) {
		m_fUsedTickSize = -1;
	} else {
		m_fUsedTickSize = pAudioEngine->getTickSize();
	}
}

std::shared_ptr<Sample> Note::getSample( int nComponentID, int nSelectedLayer ) {

	std::shared_ptr<Sample> pSample;
	
	if ( __instrument == nullptr ) {
		ERRORLOG( "Sample does not hold an instrument" );
		return nullptr;
	}

	auto pInstrCompo = __instrument->get_component( nComponentID );
	if ( pInstrCompo == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1] of instrument [%2]" )
				  .arg( nComponentID ).arg( __instrument->get_name() ) );
		return nullptr;
	}
	
	auto pSelectedLayer = get_layer_selected( nComponentID );
	if ( pSelectedLayer == nullptr ) {
		WARNINGLOG( QString( "No SelectedLayer for component ID [%1] of instrument [%2]" )
					.arg( nComponentID ).arg( __instrument->get_name() ) );
		return nullptr;
	}

	if( pSelectedLayer->SelectedLayer != -1 ||
		nSelectedLayer != -1 ) {
		// This function was already called for this note and a
		// specific layer the sample will be taken from was already
		// selected or it is provided as an input argument.

		int nLayer = pSelectedLayer->SelectedLayer != -1 ?
			pSelectedLayer->SelectedLayer : nSelectedLayer;

		if ( pSelectedLayer->SelectedLayer != -1 &&
			 nSelectedLayer != -1 &&
			 pSelectedLayer->SelectedLayer != nSelectedLayer ) {
			WARNINGLOG( QString( "Previously selected layer [%1] and requested layer [%2] differ. The previous one will be used." )
						.arg( pSelectedLayer->SelectedLayer )
						.arg( nSelectedLayer ) );
		}
		
		auto pLayer = pInstrCompo->get_layer( nLayer );
		if ( pLayer == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve layer [%1] selected for component [%2] of instrument [%3]" )
					  .arg( nLayer )
					  .arg( nComponentID ).arg( __instrument->get_name() ) );
			return nullptr;
		}
		
		pSample = pLayer->get_sample();
			
	} else {
		// Select an instrument layer.
		std::vector<int> possibleLayersVector;
		float fRoundRobinID;
		auto pSong = Hydrogen::get_instance()->getSong();
		
		for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); ++nLayer ) {
			auto pLayer = pInstrCompo->get_layer( nLayer );
			if ( pLayer == nullptr ) {
				continue;
			}

			if ( ( __velocity >= pLayer->get_start_velocity() ) &&
				 ( __velocity <= pLayer->get_end_velocity() ) ) {

				possibleLayersVector.push_back( nLayer );
				if ( __instrument->sample_selection_alg() == Instrument::VELOCITY ) {
					break;
				} else if ( __instrument->sample_selection_alg() == Instrument::ROUND_ROBIN ) {
					fRoundRobinID = pLayer->get_start_velocity();
				}
			}
		}

		// In some instruments the start and end velocities of a layer
		// are not set perfectly giving rise to some 'holes'.
		// Occasionally the velocity of a note can fall into it
		// causing the sampler to just skip it. Instead, we will
		// search for the nearest sample and play this one instead.
		if ( possibleLayersVector.size() == 0 ){
			WARNINGLOG( QString( "Velocity did fall into a hole between the instrument layers for component [%1] of instrument [%2]." )
						.arg( nComponentID ).arg( __instrument->get_name() ) );
			
			float shortestDistance = 1.0f;
			int nearestLayer = -1;
			for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); ++nLayer ){
				auto pLayer = pInstrCompo->get_layer( nLayer );
				if ( pLayer == nullptr ){
					continue;
				}
							
				if ( std::min( abs( pLayer->get_start_velocity() - __velocity ),
							   abs( pLayer->get_start_velocity() - __velocity ) ) <
					 shortestDistance ){
					shortestDistance =
						std::min( abs( pLayer->get_start_velocity() - __velocity ),
								  abs( pLayer->get_start_velocity() - __velocity ) );
					nearestLayer = nLayer;
				}
			}

			// Check whether the search was successful and assign the results.
			if ( nearestLayer > -1 ){
				possibleLayersVector.push_back( nearestLayer );
				if ( __instrument->sample_selection_alg() == Instrument::ROUND_ROBIN ) {
					fRoundRobinID =
						pInstrCompo->get_layer( nearestLayer )->get_start_velocity();
				}
			} else {
				ERRORLOG( QString( "No sample found for component [%1] of instrument [%2]" )
						  .arg( nComponentID ).arg( __instrument->get_name() ) );
				return nullptr;
			}
		}

		if( possibleLayersVector.size() > 0 ) {

			int nLayerPicked;
			switch ( __instrument->sample_selection_alg() ) {
			case Instrument::VELOCITY: 
				nLayerPicked = possibleLayersVector[ 0 ];
				break;
				
			case Instrument::RANDOM:
				nLayerPicked = possibleLayersVector[ rand() %
													 possibleLayersVector.size() ];
				break;

			case Instrument::ROUND_ROBIN: {
				fRoundRobinID = __instrument->get_id() * 10 + fRoundRobinID;
				int nIndex = pSong->getLatestRoundRobin( fRoundRobinID ) + 1;
				if ( nIndex >= possibleLayersVector.size() ) {
					nIndex = 0;
				}

				pSong->setLatestRoundRobin( fRoundRobinID, nIndex );
				nLayerPicked = possibleLayersVector[ nIndex ];
				break;
			}
				
			default:
				ERRORLOG( QString( "Unknown selection algorithm [%1] for instrument [%2]" )
						  .arg( __instrument->sample_selection_alg() )
						  .arg( __instrument->get_name() ) );
				return nullptr;
			} 

			pSelectedLayer->SelectedLayer = nLayerPicked;
			auto pLayer = pInstrCompo->get_layer( nLayerPicked );
			pSample = pLayer->get_sample();

		} else {
			ERRORLOG( "No samples found during random layer selection. This is a bug and shoul dn't happen!" );
		}
	}

	return pSample;
}

void Note::save_to( XMLNode* node )
{
	node->write_int( "position", __position );
	node->write_float( "leadlag", __lead_lag );
	node->write_float( "velocity", __velocity );
	node->write_float( "pan", m_fPan );
	node->write_float( "pitch", __pitch );
	node->write_string( "key", key_to_string() );
	node->write_int( "length", __length );
	node->write_int( "instrument", get_instrument()->get_id() );
	node->write_bool( "note_off", __note_off );
	node->write_float( "probability", __probability );
}

Note* Note::load_from( XMLNode* node, InstrumentList* instruments )
{
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

	Note* note = new Note(
		nullptr,
		node->read_int( "position", 0 ),
		node->read_float( "velocity", 0.8f ),
		fPan,
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

QString Note::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Note]\n" ).arg( sPrefix )
			.append( QString( "%1%2instrument_id: %3\n" ).arg( sPrefix ).arg( s ).arg( __instrument_id ) )
			.append( QString( "%1%2specific_compo_id: %3\n" ).arg( sPrefix ).arg( s ).arg( __specific_compo_id ) )
			.append( QString( "%1%2position: %3\n" ).arg( sPrefix ).arg( s ).arg( __position ) )
			.append( QString( "%1%2m_nNoteStart: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nNoteStart ) )
			.append( QString( "%1%2m_fUsedTickSize: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fUsedTickSize ) )
			.append( QString( "%1%2velocity: %3\n" ).arg( sPrefix ).arg( s ).arg( __velocity ) )
			.append( QString( "%1%2pan: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fPan ) )
			.append( QString( "%1%2length: %3\n" ).arg( sPrefix ).arg( s ).arg( __length ) )
			.append( QString( "%1%2pitch: %3\n" ).arg( sPrefix ).arg( s ).arg( __pitch ) )
			.append( QString( "%1%2key: %3\n" ).arg( sPrefix ).arg( s ).arg( __key ) )
			.append( QString( "%1%2octave: %3\n" ).arg( sPrefix ).arg( s ).arg( __octave ) );
		if ( __adsr != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( __adsr->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2adsr: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
			
		sOutput.append( QString( "%1%2lead_lag: %3\n" ).arg( sPrefix ).arg( s ).arg( __lead_lag ) )
			.append( QString( "%1%2cut_off: %3\n" ).arg( sPrefix ).arg( s ).arg( __cut_off ) )
			.append( QString( "%1%2resonance: %3\n" ).arg( sPrefix ).arg( s ).arg( __resonance ) )
			.append( QString( "%1%2humanize_delay: %3\n" ).arg( sPrefix ).arg( s ).arg( __humanize_delay ) )
			.append( QString( "%1%2key: %3\n" ).arg( sPrefix ).arg( s ).arg( __key ) )
			.append( QString( "%1%2bpfb_l: %3\n" ).arg( sPrefix ).arg( s ).arg( __bpfb_l ) )
			.append( QString( "%1%2bpfb_r: %3\n" ).arg( sPrefix ).arg( s ).arg( __bpfb_r ) )
			.append( QString( "%1%2lpfb_l: %3\n" ).arg( sPrefix ).arg( s ).arg( __lpfb_l ) )
			.append( QString( "%1%2lpfb_r: %3\n" ).arg( sPrefix ).arg( s ).arg( __lpfb_r ) )
			.append( QString( "%1%2pattern_idx: %3\n" ).arg( sPrefix ).arg( s ).arg( __pattern_idx ) )
			.append( QString( "%1%2midi_msg: %3\n" ).arg( sPrefix ).arg( s ).arg( __midi_msg ) )
			.append( QString( "%1%2note_off: %3\n" ).arg( sPrefix ).arg( s ).arg( __note_off ) )
			.append( QString( "%1%2just_recorded: %3\n" ).arg( sPrefix ).arg( s ).arg( __just_recorded ) )
			.append( QString( "%1%2probability: %3\n" ).arg( sPrefix ).arg( s ).arg( __probability ) )
			.append( QString( "%1" ).arg( __instrument->toQString( sPrefix + s, bShort ) ) );
		sOutput.append( QString( "%1%2layers_selected:\n" )
						.arg( sPrefix ).arg( s ) );
		for ( auto ll : __layers_selected ) {
			sOutput.append( QString( "%1%2[component: %3, selected layer: %4, sample position: %5]\n" )
							.arg( sPrefix ).arg( s + s )
							.arg( ll.first )
							.arg( ll.second->SelectedLayer )
							.arg( ll.second->SamplePosition ) );
		}
	} else {

		sOutput = QString( "[Note]" )
			.append( QString( ", instrument_id: %1" ).arg( __instrument_id ) )
			.append( QString( ", specific_compo_id: %1" ).arg( __specific_compo_id ) )
			.append( QString( ", position: %1" ).arg( __position ) )
			.append( QString( ", m_nNoteStart: %1" ).arg( m_nNoteStart ) )
			.append( QString( ", m_fUsedTickSize: %1" ).arg( m_fUsedTickSize ) )
			.append( QString( ", velocity: %1" ).arg( __velocity ) )
			.append( QString( ", pan: %1" ).arg( m_fPan ) )
			.append( QString( ", length: %1" ).arg( __length ) )
			.append( QString( ", pitch: %1" ).arg( __pitch ) )
			.append( QString( ", key: %1" ).arg( __key ) )
			.append( QString( ", octave: %1" ).arg( __octave ) );
		if ( __adsr != nullptr ) {
			sOutput.append( QString( ", [%1" )
							.arg( __adsr->toQString( sPrefix + s, bShort )
								  .replace( "\n", "]" ) ) );
		} else {
			sOutput.append( ", adsr: nullptr" );
		}
			
		sOutput.append( QString( ", lead_lag: %1" ).arg( __lead_lag ) )
			.append( QString( ", cut_off: %1" ).arg( __cut_off ) )
			.append( QString( ", resonance: %1" ).arg( __resonance ) )
			.append( QString( ", humanize_delay: %1" ).arg( __humanize_delay ) )
			.append( QString( ", key: %1" ).arg( __key ) )
			.append( QString( ", bpfb_l: %1" ).arg( __bpfb_l ) )
			.append( QString( ", bpfb_r: %1" ).arg( __bpfb_r ) )
			.append( QString( ", lpfb_l: %1" ).arg( __lpfb_l ) )
			.append( QString( ", lpfb_r: %1" ).arg( __lpfb_r ) )
			.append( QString( ", pattern_idx: %1" ).arg( __pattern_idx ) )
			.append( QString( ", midi_msg: %1" ).arg( __midi_msg ) )
			.append( QString( ", note_off: %1" ).arg( __note_off ) )
			.append( QString( ", just_recorded: %1" ).arg( __just_recorded ) )
			.append( QString( ", probability: %1" ).arg( __probability ) )
			.append( QString( ", instrument: %1" ).arg( __instrument->get_name() ) )
			.append( QString( ", layers_selected: " ) );
		for ( auto ll : __layers_selected ) {
			sOutput.append( QString( "[component: %1, selected layer: %2, sample position: %3] " )
							.arg( ll.first )
							.arg( ll.second->SelectedLayer )
							.arg( ll.second->SamplePosition ) );
		}
	}
	return sOutput;
}

QString Note::KeyToQString( Key key ) {
	QString s;

	switch( key ) {
	case Key::C:
		s = QString( "C" );
		break;
	case Key::Cs:
		s = QString( "Cs" );
		break;
	case Key::D:
		s = QString( "D" );
		break;
	case Key::Ef:
		s = QString( "Ef" );
		break;
	case Key::E:
		s = QString( "E" );
		break;
	case Key::F:
		s = QString( "F" );
		break;
	case Key::Fs:
		s = QString( "Fs" );
		break;
	case Key::G:
		s = QString( "G" );
		break;
	case Key::Af:
		s = QString( "Af" );
		break;
	case Key::A:
		s = QString( "A" );
		break;
	case Key::Bf:
		s = QString( "Bf" );
		break;
	case Key::B:
		s = QString( "B" );
		break;
	default:
		ERRORLOG(QString( "Unknown Key value [%1]" ).arg( key ) );
	}

	return s;
}
};

/* vim: set softtabstop=4 noexpandtab: */
