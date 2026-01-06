/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Helpers/Random.h>
#include <core/Helpers/Xml.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
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

Note::Note( std::shared_ptr<Instrument> pInstrument, int nPosition, float fVelocity, float fPan, int nLength, float fPitch )
	: __instrument( pInstrument ),
	  __instrument_id( 0 ),
	  __specific_compo_id( -1 ),
	  __position( nPosition ),
	  __velocity( fVelocity ),
	  __length( nLength ),
	  __pitch( fPitch ),
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
	if ( pInstrument != nullptr ) {
		__adsr = pInstrument->copy_adsr();
		__instrument_id = pInstrument->get_id();

		for ( const auto& pCompo : *pInstrument->get_components() ) {
			std::shared_ptr<SelectedLayerInfo> pSampleInfo = std::make_shared<SelectedLayerInfo>();
			pSampleInfo->nSelectedLayer = -1;
			pSampleInfo->fSamplePosition = 0;
			pSampleInfo->nNoteLength = -1;

			__layers_selected[ pCompo->get_drumkit_componentID() ] = pSampleInfo;
		}
	}

	setPan( fPan ); // this checks the boundaries
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
	if ( instrument != nullptr ) __instrument = instrument;
	if ( __instrument != nullptr ) {
		__adsr = __instrument->copy_adsr();
		__instrument_id = __instrument->get_id();
	}

	for ( const auto& mm : other->__layers_selected ) {
		std::shared_ptr<SelectedLayerInfo> pSampleInfo = std::make_shared<SelectedLayerInfo>();
		pSampleInfo->nSelectedLayer = mm.second->nSelectedLayer;
		pSampleInfo->fSamplePosition = mm.second->fSamplePosition;
		pSampleInfo->nNoteLength = mm.second->nNoteLength;
		
		__layers_selected[ mm.first ] = pSampleInfo;
	}
}

Note::~Note()
{
}

static inline float check_boundary( float fValue, float fMin, float fMax )
{
	return std::clamp( fValue, fMin, fMax );
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

void Note::set_humanize_delay( int nValue )
{
	// We do not perform bound checks with
	// AudioEngine::nMaxTimeHumanize in here as different contribution
	// could push the value first beyond and then within the bounds
	// again. The clamping will be done in computeNoteStart() instead.
	if ( nValue != __humanize_delay ) {
		__humanize_delay = nValue;
	}
}

void Note::map_instrument( std::shared_ptr<InstrumentList> pInstrumentList )
{
	if ( pInstrumentList == nullptr ) {
		assert( pInstrumentList );
		ERRORLOG( "Invalid instrument list" );
		return;
	}
	
	auto pInstr = pInstrumentList->find( __instrument_id );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Instrument with ID [%1] not found. Using empty instrument." )
				  .arg( __instrument_id ) );
		__instrument = std::make_shared<Instrument>();
	}
	else {
		__instrument = pInstr;
		__adsr = pInstr->copy_adsr();

		for ( const auto& ppCompo : *pInstr->get_components() ) {
			std::shared_ptr<SelectedLayerInfo> sampleInfo = std::make_shared<SelectedLayerInfo>();
			sampleInfo->nSelectedLayer = -1;
			sampleInfo->fSamplePosition = 0;
			sampleInfo->nNoteLength = -1;

			__layers_selected[ ppCompo->get_drumkit_componentID() ] = sampleInfo;
		}
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
		if ( ll.second->fSamplePosition > 0 ) {
			bRes = true;
			break;
		}
	}

	return bRes;
}

void Note::computeNoteStart() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	double fTickMismatch;
	m_nNoteStart =
		TransportPosition::computeFrameFromTick( __position, &fTickMismatch );

	m_nNoteStart += std::clamp( __humanize_delay,
								-1 * AudioEngine::nMaxTimeHumanize,
								AudioEngine::nMaxTimeHumanize );

	// No note can start before the beginning of the song.
	if ( m_nNoteStart < 0 ) {
		m_nNoteStart = 0;
	}
	
	if ( pHydrogen->isTimelineEnabled() ) {
		m_fUsedTickSize = -1;
	} else {
		// This is used for triggering recalculation in case the tempo
		// changes where manually applied by the user. They are not
		// dependent on a particular position of the transport (as
		// Timeline is not activated).
		m_fUsedTickSize = pAudioEngine->getTransportPosition()->getTickSize();
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

	if ( pSelectedLayer->nSelectedLayer != -1 ||
		 nSelectedLayer != -1 ) {
		// This function was already called for this note and a
		// specific layer the sample will be taken from was already
		// selected or it is provided as an input argument.

		int nLayer = pSelectedLayer->nSelectedLayer != -1 ?
			pSelectedLayer->nSelectedLayer : nSelectedLayer;

		if ( pSelectedLayer->nSelectedLayer != -1 &&
			 nSelectedLayer != -1 &&
			 pSelectedLayer->nSelectedLayer != nSelectedLayer ) {
			WARNINGLOG( QString( "Previously selected layer [%1] and requested layer [%2] differ. The previous one will be used." )
						.arg( pSelectedLayer->nSelectedLayer )
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

		// In some instruments the start and end velocities of a layer are not
		// set perfectly giving rise to some 'holes'. Occasionally, the velocity
		// of a note will fall into it. We have to take care of that by
		// searching for the nearest sample and play this one instead.
		//
		// Apart from that, when having multiple components but not an
		// associated sample for all of them in all instruments (this is
		// especially likely to happen with kits created in Hydrogen version >=
		// 2.0), we might not find a sample in here.
		if ( possibleLayersVector.size() == 0 ){
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

			if ( nearestLayer > -1 ){
				// velocity fell into a hole.
				possibleLayersVector.push_back( nearestLayer );
				if ( __instrument->sample_selection_alg() == Instrument::ROUND_ROBIN ) {
					fRoundRobinID =
						pInstrCompo->get_layer( nearestLayer )->get_start_velocity();
				}
			} else {
				// there is no sample.
				return nullptr;
			}
		}

		if ( possibleLayersVector.size() > 0 ) {

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

			pSelectedLayer->nSelectedLayer = nLayerPicked;
			auto pLayer = pInstrCompo->get_layer( nLayerPicked );
			pSample = pLayer->get_sample();

		} else {
			ERRORLOG( "No samples found during random layer selection. This is a bug and shoul dn't happen!" );
		}
	}

	return pSample;
}

float Note::get_total_pitch() const
{
	float fNotePitch = __octave * KEYS_PER_OCTAVE + __key + __pitch;

	if ( __instrument != nullptr ) {
		fNotePitch += __instrument->get_pitch_offset();
	}
	return fNotePitch;
}

void Note::humanize() {
	// Due to the nature of the Gaussian distribution, the factors
	// will also scale the standard deviations of the generated random
	// variables.
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		const float fRandomVelocityFactor = pSong->getHumanizeVelocityValue();
		if ( fRandomVelocityFactor != 0 ) {
			set_velocity( __velocity + fRandomVelocityFactor *
						  Random::getGaussian( AudioEngine::fHumanizeVelocitySD ) );
		}

		const float fRandomTimeFactor = pSong->getHumanizeTimeValue();
		if ( fRandomTimeFactor != 0 ) {
			set_humanize_delay( __humanize_delay + fRandomTimeFactor *
								AudioEngine::nMaxTimeHumanize *
								Random::getGaussian( AudioEngine::fHumanizeTimingSD ) );
		}
	}

	if ( __instrument != nullptr ) {
		const float fRandomPitchFactor = __instrument->get_random_pitch_factor();
		if ( fRandomPitchFactor != 0 ) {
			__pitch += Random::getGaussian( AudioEngine::fHumanizePitchSD ) *
				fRandomPitchFactor;
			}
	}
}

void Note::swing() {
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr && pSong->getSwingFactor() > 0 ) {
		/* TODO: incorporate the factor MAX_NOTES / 32. either in
		 * Song::m_fSwingFactor or make it a member variable.
		 *
		 * comment by oddtime: 32 depends on the fact that the swing
		 * is applied to the upbeat 16th-notes.  (not to upbeat
		 * 8th-notes as in jazz swing!).  however 32 could be changed
		 * but must be >16, otherwise the max delay is too long and
		 * the swing note could be played after the next downbeat!
		 */
		// If the Timeline is activated, the tick size may change at
		// any point. Therefore, the length in frames of a 16-th note
		// offset has to be calculated for a particular transport
		// position and is not generally applicable.
		double fTickMismatch;
		set_humanize_delay( __humanize_delay +
							( TransportPosition::computeFrameFromTick(
								__position + MAX_NOTES / 32., &fTickMismatch ) -
							  TransportPosition::computeFrameFromTick(
								  __position, &fTickMismatch ) ) *
							pSong->getSwingFactor() );
	}
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

Note* Note::load_from( XMLNode* node, std::shared_ptr<InstrumentList> instruments, bool bSilent )
{
	bool bFound, bFound2;
	float fPan = node->read_float( "pan", 0.f, &bFound, true, false, true );
	if ( !bFound ) {
		// check if pan is expressed in the old fashion (version <=
		// 1.1 ) with the pair (pan_L, pan_R)
		float fPanL = node->read_float( "pan_L", 1.f, &bFound, false, false, bSilent );
		float fPanR = node->read_float( "pan_R", 1.f, &bFound2, false, false, bSilent );
		if ( bFound && bFound2 ) {
			fPan = Sampler::getRatioPan( fPanL, fPanR );  // convert to single pan parameter
		} else {
			WARNINGLOG( QString( "Neither `pan` nor `pan_L` and `pan_R` were found. Falling back to `pan = 0`" ) );
		}
	}

	Note* note = new Note(
		nullptr,
		node->read_int( "position", 0, false, false, bSilent ),
		node->read_float( "velocity", 0.8f, false, false, bSilent ),
		fPan,
		node->read_int( "length", -1, true, false, bSilent ),
        // Starting from version 2.0 of Hydrogen, the "pitch" node will be
        // dropped.
		node->read_float( "pitch", 0.0f, true, false, true )
	);
	note->set_lead_lag( node->read_float( "leadlag", 0, false, false, bSilent ) );
	note->set_key_octave( node->read_string( "key", "C0", false, false, bSilent ) );
	note->set_note_off( node->read_bool( "note_off", false, false, false, bSilent ) );
	note->set_instrument_id( node->read_int( "instrument", EMPTY_INSTR_ID, false, false, bSilent ) );
	note->map_instrument( instruments );
	note->set_probability( node->read_float( "probability", 1.0f, false, false, bSilent ));

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
			.append( QString( "%1%2probability: %3\n" ).arg( sPrefix ).arg( s ).arg( __probability ) );
		if ( __instrument != nullptr ) {		
			sOutput.append( QString( "%1" ).arg( __instrument->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2instrument: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
		sOutput.append( QString( "%1%2layers_selected:\n" )
						.arg( sPrefix ).arg( s ) );
		for ( auto ll : __layers_selected ) {
			if ( ll.second != nullptr ) {
				sOutput.append( QString( "%1%2[component: %3, selected layer: %4, sample position: %5, note length: %6]\n" )
								.arg( sPrefix ).arg( s + s )
								.arg( ll.first )
								.arg( ll.second->nSelectedLayer )
								.arg( ll.second->fSamplePosition )
								.arg( ll.second->nNoteLength ) );
			} else {
				sOutput.append( QString( "%1%2[component: %3, selected layer info: nullptr]\n" )
								.arg( sPrefix ).arg( s + s )
								.arg( ll.first ) );
			}
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
			.append( QString( ", probability: %1" ).arg( __probability ) );
		if ( __instrument != nullptr ) {
			sOutput.append( QString( ", instrument: %1" ).arg( __instrument->get_name() ) );
		} else {
			sOutput.append( QString( ", instrument: nullptr" ) );
		}
		sOutput.append( QString( ", layers_selected: " ) );
		for ( auto ll : __layers_selected ) {
			if ( ll.second != nullptr ) {
				sOutput.append( QString( "[component: %1, selected layer: %2, sample position: %3, note length: %4] " )
								.arg( ll.first )
								.arg( ll.second->nSelectedLayer )
								.arg( ll.second->fSamplePosition )
								.arg( ll.second->nNoteLength ) );
			} else {
				sOutput.append( QString( "[component: %1, selected layer info: nullptr]" )
								.arg( ll.first ) );
			}
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
