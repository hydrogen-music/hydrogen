/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>

namespace H2Core
{

const char* Note::__key_str[] = { "C", "Cs", "D", "Ef", "E", "F", "Fs", "G", "Af", "A", "Bf", "B" };

Note::Note( std::shared_ptr<Instrument> pInstrument, int nPosition,
			float fVelocity, float fPan, int nLength, float fPitch )
	: __instrument_id( EMPTY_INSTR_ID ),
	  m_sType( "" ),
	  __position( nPosition ),
	  __velocity( fVelocity ),
	  __length( nLength ),
	  __pitch( fPitch ),
	  __key( static_cast<Note::Key>(KEY_MIN) ),
	  __octave( static_cast<Note::Octave>(OCTAVE_DEFAULT) ),
	  __adsr( nullptr ),
	  __lead_lag( LEAD_LAG_DEFAULT ),
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
	  __probability( PROBABILITY_DEFAULT ),
	  m_nNoteStart( 0 ),
	  m_fUsedTickSize( std::nan("") ),
	  m_nSpecificCompoIdx( -1 ),
	  __instrument( pInstrument )
{
	if ( pInstrument != nullptr ) {
		__adsr = pInstrument->copy_adsr();
		__instrument_id = pInstrument->get_id();
		m_sType = pInstrument->getType();

		__layers_selected.resize( __instrument->get_components()->size() );
		for ( int ii = 0; ii < pInstrument->get_components()->size(); ++ii ) {
			const auto pCompo = pInstrument->get_component( ii );
			if ( pCompo != nullptr ) {
				std::shared_ptr<SelectedLayerInfo> pSampleInfo =
					std::make_shared<SelectedLayerInfo>();
				pSampleInfo->nSelectedLayer = -1;
				pSampleInfo->fSamplePosition = 0;
				pSampleInfo->nNoteLength = LENGTH_ENTIRE_SAMPLE;

				__layers_selected[ ii ] = pSampleInfo;
			}
			else {
				__layers_selected[ ii ] = nullptr;
			}
		}
	}

	setPan( fPan ); // this checks the boundaries
}

Note::Note( std::shared_ptr<Note> pOther, std::shared_ptr<Instrument> pInstrument )
	: __instrument_id( EMPTY_INSTR_ID ),
	  m_sType( pOther->getType() ),
	  __position( pOther->get_position() ),
	  __velocity( pOther->get_velocity() ),
	  m_fPan( pOther->getPan() ),
	  __length( pOther->get_length() ),
	  __pitch( pOther->get_pitch() ),
	  __key( pOther->get_key() ),
	  __octave( pOther->get_octave() ),
	  __adsr( nullptr ),
	  __lead_lag( pOther->get_lead_lag() ),
	  __cut_off( pOther->get_cut_off() ),
	  __resonance( pOther->get_resonance() ),
	  __humanize_delay( pOther->get_humanize_delay() ),
	  __bpfb_l( pOther->get_bpfb_l() ),
	  __bpfb_r( pOther->get_bpfb_r() ),
	  __lpfb_l( pOther->get_lpfb_l() ),
	  __lpfb_r( pOther->get_lpfb_r() ),
	  __pattern_idx( pOther->get_pattern_idx() ),
	  __midi_msg( pOther->get_midi_msg() ),
	  __note_off( pOther->get_note_off() ),
	  __just_recorded( pOther->get_just_recorded() ),
	  __probability( pOther->get_probability() ),
	  m_nNoteStart( pOther->getNoteStart() ),
	  m_fUsedTickSize( pOther->getUsedTickSize() ),
	  m_nSpecificCompoIdx( pOther->m_nSpecificCompoIdx ),
	  __instrument( pOther->get_instrument() )
{
	if ( pInstrument != nullptr ) {
		__instrument = pInstrument;
	}
	if ( __instrument != nullptr ) {
		__adsr = __instrument->copy_adsr();
		__instrument_id = __instrument->get_id();

		__layers_selected.resize( __instrument->get_components()->size() );
		for ( int ii = 0; ii < __instrument->get_components()->size(); ++ii ) {
			const auto ppSelectedLayerInfo = pOther->__layers_selected[ ii ];
			if ( ppSelectedLayerInfo != nullptr ) {
				std::shared_ptr<SelectedLayerInfo> pSampleInfo =
					std::make_shared<SelectedLayerInfo>();
				pSampleInfo->nSelectedLayer = ppSelectedLayerInfo->nSelectedLayer;
				pSampleInfo->fSamplePosition = ppSelectedLayerInfo->fSamplePosition;
				pSampleInfo->nNoteLength = ppSelectedLayerInfo->nNoteLength;
		
				__layers_selected[ ii ] = pSampleInfo;
			}
			else {
				__layers_selected[ ii ] = nullptr;
			}
		}
	}
}

Note::~Note() {
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
	m_fPan = check_boundary( val, PAN_MIN, PAN_MAX );
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

void Note::mapTo( std::shared_ptr<Drumkit> pDrumkit,
				  std::shared_ptr<Drumkit> pOldDrumkit )
{
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}
	const auto pDrumkitMap = pDrumkit->toDrumkitMap();

	std::shared_ptr<Instrument> pInstrument = nullptr;
	// In case drumkit and note feature a type string, we use this one to
	// retrieve the matching instrument. Else we restore to "historical" loading
	// using instrument IDs. This is used both for patterns created prior to
	// version 2.0 of Hydrogen and patterns created with a kit with missing
	// types (either legacy one or freshly created instrument).
	if ( ! m_sType.isEmpty() && pDrumkitMap->getAllTypes().size() > 0 ) {
		bool bFound;
		const int nId = pDrumkitMap->getId( m_sType, &bFound );
		if ( bFound ) {
			pInstrument = pDrumkit->getInstruments()->find( nId );
		}

		if ( pOldDrumkit != nullptr ) {
			// Check whether we deal with the same kit and the type of an
			// instrument was changed. If so, we have to change the type of all
			// associated notes too.
			const auto pOldDrumkitMap = pOldDrumkit->toDrumkitMap();
			const int nOldId = pOldDrumkitMap->getId( m_sType, &bFound );
			if ( pDrumkit->getPath() == pOldDrumkit->getPath() &&
				 pDrumkit->getName() == pOldDrumkit->getName() &&
				 bFound && nId != nOldId ) {
				pInstrument = pDrumkit->getInstruments()->find( nOldId );
				if ( pInstrument != nullptr ) {
					m_sType = pInstrument->getType();
				}
			}
		}
	}
	else {
		pInstrument = pDrumkit->getInstruments()->find( __instrument_id );

		if ( pOldDrumkit != nullptr && pInstrument != nullptr &&
			 pDrumkit->getPath() == pOldDrumkit->getPath() &&
			 pDrumkit->getName() == pOldDrumkit->getName() &&
			 ! pInstrument->getType().isEmpty() ) {
			// The note was associated with an instrument, which did not hold a
			// type in the old version of the kit but was just assigned a new
			// one. Since this was an explicit user interaction we propagate
			// those type changes to all contained notes as well.
			m_sType = pInstrument->getType();
		}
	}

	if ( pInstrument != nullptr ) {
		__instrument = pInstrument;
		__adsr = pInstrument->copy_adsr();
		__instrument_id = pInstrument->get_id();

		__layers_selected.clear();
		__layers_selected.resize( pInstrument->get_components()->size() );
		for ( int ii = 0; ii < pInstrument->get_components()->size(); ++ii ) {
			const auto pCompo = pInstrument->get_component( ii );
			if ( pCompo != nullptr ) {
				std::shared_ptr<SelectedLayerInfo> sampleInfo =
					std::make_shared<SelectedLayerInfo>();
				sampleInfo->nSelectedLayer = -1;
				sampleInfo->fSamplePosition = 0;
				sampleInfo->nNoteLength = LENGTH_ENTIRE_SAMPLE;

				__layers_selected[ ii ] = sampleInfo;
			}
			else {
				__layers_selected[ ii ] = nullptr;
			}
		}
	}
	else {
		INFOLOG( QString( "No instrument was found for type [%1] and ID [%2]." )
				 .arg( m_sType ).arg( __instrument_id ) );
		__instrument = nullptr;
		__adsr = nullptr;
		__layers_selected.clear();

		// In case no matching instrument was found, we reset the instrument ID.
		// But we only do so in case the note has an associated instrument type.
		// Else, there would be no way to map it back to the previous
		// instrument.
		if ( ! m_sType.isEmpty() ) {
			__instrument_id = EMPTY_INSTR_ID;
		}
	}
}

QString Note::key_to_string() const
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

	for ( const auto& ll : __layers_selected ) {
		if ( ll != nullptr && ll->fSamplePosition > 0 ) {
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

std::shared_ptr<Sample> Note::getSample( int nComponentIdx, int nSelectedLayer ) const {

	std::shared_ptr<Sample> pSample;
	
	if ( __instrument == nullptr ) {
		ERRORLOG( "Sample does not hold an instrument" );
		return nullptr;
	}

	auto pInstrCompo = __instrument->get_component( nComponentIdx );
	if ( pInstrCompo == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1] of instrument [%2]" )
				  .arg( nComponentIdx ).arg( __instrument->get_name() ) );
		return nullptr;
	}
	
	auto pSelectedLayer = get_layer_selected( nComponentIdx );
	if ( pSelectedLayer == nullptr ) {
		WARNINGLOG( QString( "No SelectedLayer for component [%1] of instrument [%2]" )
					.arg( pInstrCompo->getName() ).arg( __instrument->get_name() ) );
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
		
		auto pLayer = pInstrCompo->getLayer( nLayer );
		if ( pLayer == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve layer [%1] selected for component [%2] of instrument [%3]" )
					  .arg( nLayer ).arg( pInstrCompo->getName() )
					  .arg( __instrument->get_name() ) );
			return nullptr;
		}
		
		pSample = pLayer->get_sample();
			
	}
	else {
		// Select an instrument layer.
		std::vector<int> possibleLayersVector;
		int nLayersEncountered = 0;
		float fRoundRobinID;
		auto pSong = Hydrogen::get_instance()->getSong();
		
		for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); ++nLayer ) {
			auto pLayer = pInstrCompo->getLayer( nLayer );
			if ( pLayer == nullptr ) {
				continue;
			}
			++nLayersEncountered;

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

		if ( nLayersEncountered == 0 ) {
			return nullptr;
		}

		// In some instruments the start and end velocities of a layer
		// are not set perfectly giving rise to some 'holes'.
		// Occasionally the velocity of a note can fall into it
		// causing the sampler to just skip it. Instead, we will
		// search for the nearest sample and play this one instead.
		if ( possibleLayersVector.size() == 0 ){
			WARNINGLOG( QString( "Velocity [%1] did fall into a hole between the instrument layers for component [%2] of instrument [%3]." )
						.arg( __velocity ).arg( pInstrCompo->getName() )
						.arg( __instrument->get_name() ) );
			
			float shortestDistance = 1.0f;
			int nearestLayer = -1;
			for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); ++nLayer ){
				auto pLayer = pInstrCompo->getLayer( nLayer );
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
						pInstrCompo->getLayer( nearestLayer )->get_start_velocity();
				}
			} else {
				ERRORLOG( QString( "No sample found for component [%1] of instrument [%2]" )
						  .arg( pInstrCompo->getName() )
						  .arg( __instrument->get_name() ) );
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
			auto pLayer = pInstrCompo->getLayer( nLayerPicked );
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

void Note::save_to( XMLNode& node ) const
{
	node.write_int( "position", __position );
	node.write_float( "leadlag", __lead_lag );
	node.write_float( "velocity", __velocity );
	node.write_float( "pan", m_fPan );
	node.write_float( "pitch", __pitch );
	node.write_string( "key", key_to_string() );
	node.write_int( "length", __length );
	node.write_int( "instrument", __instrument_id );
	node.write_string( "type", m_sType );
	node.write_bool( "note_off", __note_off );
	node.write_float( "probability", __probability );
}

std::shared_ptr<Note> Note::load_from( const XMLNode& node, bool bSilent )
{
	bool bFound, bFound2;
	float fPan = node.read_float( "pan", PAN_DEFAULT, &bFound, true, false, true );
	if ( !bFound ) {
		// check if pan is expressed in the old fashion (version <=
		// 1.1 ) with the pair (pan_L, pan_R)
		float fPanL = node.read_float( "pan_L", 1.f, &bFound, false, false, bSilent );
		float fPanR = node.read_float( "pan_R", 1.f, &bFound2, false, false, bSilent );
		if ( bFound && bFound2 ) {
			fPan = Sampler::getRatioPan( fPanL, fPanR );  // convert to single pan parameter
		} else {
			WARNINGLOG( QString( "Neither `pan` nor `pan_L` and `pan_R` were found. Falling back to `pan = 0`" ) );
		}
	}

	auto pNote = std::make_shared<Note>(
		nullptr,
		node.read_int( "position", 0, false, false, bSilent ),
		node.read_float( "velocity", VELOCITY_DEFAULT, false, false, bSilent ),
		fPan,
		node.read_int( "length", LENGTH_ENTIRE_SAMPLE, true, false, bSilent ),
		node.read_float( "pitch", PITCH_DEFAULT, false, false, bSilent )
	);
	pNote->set_lead_lag(
		node.read_float( "leadlag", LEAD_LAG_DEFAULT, false, false, bSilent ) );
	pNote->set_key_octave( node.read_string( "key", "C0", false, false, bSilent ) );
	pNote->set_note_off( node.read_bool( "note_off", false, false, false, bSilent ) );
	pNote->set_instrument_id( node.read_int( "instrument", EMPTY_INSTR_ID, false, false, bSilent ) );
	pNote->setType( node.read_string( "type", "", true, true, bSilent ) );
	pNote->set_probability(
		node.read_float( "probability", PROBABILITY_DEFAULT, false, false, bSilent ));

	return pNote;
}

QString Note::prettyName() const {
	QString sInstrument, sPattern;

	if ( __instrument != nullptr ) {
		sInstrument = QString( "instr: [%1]" ).arg( __instrument->get_name() );
	} else {
		sInstrument = QString( "type: [%1]" ).arg( m_sType );
	}

	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		const auto pPattern = pSong->getPatternList()->get( __pattern_idx );
		if ( pPattern != nullptr ) {
			sPattern = QString( "Pat: [%1]" ).arg( pPattern->getName() );
		}
	}
	if ( sPattern.isEmpty() ) {
		sPattern = "No pat";
	}

	return QString( "%1, %2, pos: %3, key: %4, octave: %5" )
		.arg( sPattern ).arg( sInstrument ).arg( __position )
		.arg( KeyToQString( __key ) )
		.arg( OctaveToQString( __octave ) );
}

QString Note::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Note]\n" ).arg( sPrefix )
			.append( QString( "%1%2instrument_id: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __instrument_id ) )
			.append( QString( "%1%2m_sType: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sType ) )
			.append( QString( "%1%2position: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __position ) )
			.append( QString( "%1%2velocity: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __velocity ) )
			.append( QString( "%1%2pan: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPan ) )
			.append( QString( "%1%2length: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __length ) )
			.append( QString( "%1%2pitch: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __pitch ) )
			.append( QString( "%1%2key: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( KeyToQString( __key ) ) )
			.append( QString( "%1%2octave: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( OctaveToQString( __octave ) ) );
		if ( __adsr != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( __adsr->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2adsr: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
		sOutput.append( QString( "%1%2lead_lag: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __lead_lag ) )
			.append( QString( "%1%2cut_off: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __cut_off ) )
			.append( QString( "%1%2resonance: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __resonance ) )
			.append( QString( "%1%2humanize_delay: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __humanize_delay ) )
			.append( QString( "%1%2bpfb_l: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __bpfb_l ) )
			.append( QString( "%1%2bpfb_r: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __bpfb_r ) )
			.append( QString( "%1%2lpfb_l: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __lpfb_l ) )
			.append( QString( "%1%2lpfb_r: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __lpfb_r ) )
			.append( QString( "%1%2pattern_idx: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __pattern_idx ) )
			.append( QString( "%1%2midi_msg: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __midi_msg ) )
			.append( QString( "%1%2note_off: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __note_off ) )
			.append( QString( "%1%2just_recorded: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __just_recorded ) )
			.append( QString( "%1%2probability: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( __probability ) )
			.append( QString( "%1%2__key_str: [" ).arg( sPrefix ).arg( s ) );
			for ( int ii = KEY_MIN; ii <= KEY_MAX; ++ii ) {
					 sOutput.append( QString( "%1, " ).arg(
										 QString::fromUtf8( __key_str[ ii ], -1 ) ) );
			}
			sOutput.append( QString( "]\n%1%2m_nNoteStart: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nNoteStart ) )
			.append( QString( "%1%2m_fUsedTickSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fUsedTickSize ) )
			.append( QString( "%1%2m_nSpecificCompoIdx: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSpecificCompoIdx ) )
			.append( QString( "%1%2layers_selected:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppLayer : __layers_selected ) {
			if ( ppLayer != nullptr ) {
				sOutput.append( QString( "%1%2[%3]\n" )
								.arg( sPrefix ).arg( s + s )
								.arg( ppLayer->toQString( "", true ) ) );
			} else {
				sOutput.append( QString( "%1%2[SelectedLayerInfo: nullptr]\n" )
								.arg( sPrefix ).arg( s + s ) );
			}
		}
		if ( __instrument != nullptr ) {
			sOutput.append( QString( "%1" ).arg( __instrument->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2instrument: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
	} else {

		sOutput = QString( "[Note]" )
			.append( QString( ", instrument_id: %1" ).arg( __instrument_id ) )
			.append( QString( ", m_sType: %1" ).arg( m_sType ) )
			.append( QString( ", position: %1" ).arg( __position ) )
			.append( QString( ", velocity: %1" ).arg( __velocity ) )
			.append( QString( ", pan: %1" ).arg( m_fPan ) )
			.append( QString( ", length: %1" ).arg( __length ) )
			.append( QString( ", pitch: %1" ).arg( __pitch ) )
			.append( QString( ", key: %1" )
					 .arg( KeyToQString( __key ) ) )
			.append( QString( ", octave: %1" )
					 .arg( OctaveToQString( __octave ) ) );
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
			.append( QString( ", bpfb_l: %1" ).arg( __bpfb_l ) )
			.append( QString( ", bpfb_r: %1" ).arg( __bpfb_r ) )
			.append( QString( ", lpfb_l: %1" ).arg( __lpfb_l ) )
			.append( QString( ", lpfb_r: %1" ).arg( __lpfb_r ) )
			.append( QString( ", pattern_idx: %1" ).arg( __pattern_idx ) )
			.append( QString( ", midi_msg: %1" ).arg( __midi_msg ) )
			.append( QString( ", note_off: %1" ).arg( __note_off ) )
			.append( QString( ", just_recorded: %1" ).arg( __just_recorded ) )
			.append( QString( ", probability: %1" ).arg( __probability ) )
			.append( ", __key_str: [" );
			for ( int ii = KEY_MIN; ii <= KEY_MAX; ++ii ) {
					 sOutput.append( QString( "%1, " ).arg(
										 QString::fromUtf8( __key_str[ ii ], -1 ) ) );
			}
			sOutput.append( QString( "], m_nNoteStart: %1" ).arg( m_nNoteStart ) )
			.append( QString( ", m_fUsedTickSize: %1" ).arg( m_fUsedTickSize ) )
			.append( QString( ", m_nSpecificCompoIdx: %1" )
					 .arg( m_nSpecificCompoIdx ) )
			.append( QString( ", layers_selected: " ) );
		for ( const auto& ppLayer : __layers_selected ) {
			if ( ppLayer != nullptr ) {
				sOutput.append( QString( "[%1] " )
								.arg( ppLayer->toQString( "", true ) ) );
			} else {
				sOutput.append( "[SelectedLayerInfo: nullptr] " );
			}
		}
		if ( __instrument != nullptr ) {
			sOutput.append( QString( ", instrument: %1" ).arg( __instrument->get_name() ) );
		} else {
			sOutput.append( QString( ", instrument: nullptr" ) );
		}
	}
	return sOutput;
}

QString Note::KeyToQString( const Key& key ) {
	switch( key ) {
	case Key::C:
		return "C";
	case Key::Cs:
		return "Cs";
	case Key::D:
		return "D";
	case Key::Ef:
		return "Ef";
	case Key::E:
		return "E";
	case Key::F:
		return "F";
	case Key::Fs:
		return "Fs";
	case Key::G:
		return "G";
	case Key::Af:
		return "Af";
	case Key::A:
		return "A";
	case Key::Bf:
		return "Bf";
	case Key::B:
		return "B";
	default:
		return QString( "Unknown Key value [%1]" ).arg( key );
	}
}

QString Note::OctaveToQString( const Octave& octave ) {
	switch( octave ) {
	case Octave::P8Z:
		return "P8Z";
	case Octave::P8Y:
		return "P8Y";
	case Octave::P8X:
		return "P8X";
	case Octave::P8:
		return "P8";
	case Octave::P8A:
		return "P8A";
	case Octave::P8B:
		return "P8B";
	case Octave::P8C:
		return "P8C";
	default:
		return QString( "Unknown octave value [%1]" ).arg( octave );
	}
}

QString SelectedLayerInfo::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SelectedLayerInfo]\n" ).arg( sPrefix )
			.append( QString( "%1%2nSelectedLayer: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( nSelectedLayer ) )
			.append( QString( "%1%2fSamplePosition: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( fSamplePosition ) )
			.append( QString( "%1%2nNoteLength: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( nNoteLength ) );
	}
	else {
		sOutput = QString( "[SelectedLayerInfo] " )
			.append( QString( "nSelectedLayer: %1" )
					 .arg( nSelectedLayer ) )
			.append( QString( ", fSamplePosition: %1" )
					 .arg( fSamplePosition ) )
			.append( QString( ", nNoteLength: %1" )
					 .arg( nNoteLength ) );
	}

	return sOutput;
}
};

/* vim: set softtabstop=4 noexpandtab: */
