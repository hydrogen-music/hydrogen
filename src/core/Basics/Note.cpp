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

const char* Note::m_keyStr[] = { "C", "Cs", "D", "Ef", "E", "F", "Fs", "G", "Af", "A", "Bf", "B" };

Note::Note( std::shared_ptr<Instrument> pInstrument, int nPosition,
			float fVelocity, float fPan, int nLength, float fPitch )
	: m_nInstrumentId( EMPTY_INSTR_ID ),
	  m_sType( "" ),
	  m_nPosition( nPosition ),
	  m_fVelocity( fVelocity ),
	  m_nLength( nLength ),
	  m_fPitch( fPitch ),
	  m_key( static_cast<Note::Key>(KEY_MIN) ),
	  m_octave( static_cast<Note::Octave>(OCTAVE_DEFAULT) ),
	  m_pAdsr( nullptr ),
	  m_fLeadLag( LEAD_LAG_DEFAULT ),
	  m_nHumanizeDelay( 0 ),
	  m_fBpfbL( 0.0 ),
	  m_fBpfbR( 0.0 ),
	  m_fLpfbL( 0.0 ),
	  m_fLpfbR( 0.0 ),
	  m_nMidiMsg( -1 ),
	  m_bNoteOff( false ),
	  m_fProbability( PROBABILITY_DEFAULT ),
	  m_nNoteStart( 0 ),
	  m_fUsedTickSize( std::nan("") ),
	  m_nSpecificCompoIdx( -1 ),
	  m_pInstrument( pInstrument )
{
	if ( pInstrument != nullptr ) {
		m_pAdsr = pInstrument->copyAdsr();
		m_nInstrumentId = pInstrument->getId();
		m_sType = pInstrument->getType();

		m_layersSelected.resize( m_pInstrument->getComponents()->size() );
		for ( int ii = 0; ii < pInstrument->getComponents()->size(); ++ii ) {
			const auto pCompo = pInstrument->getComponent( ii );
			if ( pCompo != nullptr ) {
				std::shared_ptr<SelectedLayerInfo> pSampleInfo =
					std::make_shared<SelectedLayerInfo>();
				pSampleInfo->nSelectedLayer = -1;
				pSampleInfo->fSamplePosition = 0;
				pSampleInfo->nNoteLength = LENGTH_ENTIRE_SAMPLE;

				m_layersSelected[ ii ] = pSampleInfo;
			}
			else {
				m_layersSelected[ ii ] = nullptr;
			}
		}
	}

	setPan( fPan ); // this checks the boundaries
}

Note::Note( std::shared_ptr<Note> pOther, std::shared_ptr<Instrument> pInstrument )
	: m_nInstrumentId( EMPTY_INSTR_ID ),
	  m_sType( pOther->getType() ),
	  m_nPosition( pOther->getPosition() ),
	  m_fVelocity( pOther->getVelocity() ),
	  m_fPan( pOther->getPan() ),
	  m_nLength( pOther->getLength() ),
	  m_fPitch( pOther->getPitch() ),
	  m_key( pOther->getKey() ),
	  m_octave( pOther->getOctave() ),
	  m_pAdsr( nullptr ),
	  m_fLeadLag( pOther->getLeadLag() ),
	  m_nHumanizeDelay( pOther->getHumanizeDelay() ),
	  m_fBpfbL( pOther->m_fBpfbL ),
	  m_fBpfbR( pOther->m_fBpfbR ),
	  m_fLpfbL( pOther->m_fLpfbL ),
	  m_fLpfbR( pOther->m_fLpfbR ),
	  m_nMidiMsg( pOther->getMidiMsg() ),
	  m_bNoteOff( pOther->getNoteOff() ),
	  m_fProbability( pOther->getProbability() ),
	  m_nNoteStart( pOther->getNoteStart() ),
	  m_fUsedTickSize( pOther->getUsedTickSize() ),
	  m_nSpecificCompoIdx( pOther->m_nSpecificCompoIdx ),
	  m_pInstrument( pOther->getInstrument() )
{
	if ( pInstrument != nullptr ) {
		m_pInstrument = pInstrument;
	}
	if ( m_pInstrument != nullptr ) {
		m_pAdsr = m_pInstrument->copyAdsr();
		m_nInstrumentId = m_pInstrument->getId();

		m_layersSelected.resize( m_pInstrument->getComponents()->size() );
		for ( int ii = 0; ii < m_pInstrument->getComponents()->size(); ++ii ) {
			const auto ppSelectedLayerInfo = pOther->m_layersSelected[ ii ];
			if ( ppSelectedLayerInfo != nullptr ) {
				std::shared_ptr<SelectedLayerInfo> pSampleInfo =
					std::make_shared<SelectedLayerInfo>();
				pSampleInfo->nSelectedLayer = ppSelectedLayerInfo->nSelectedLayer;
				pSampleInfo->fSamplePosition = ppSelectedLayerInfo->fSamplePosition;
				pSampleInfo->nNoteLength = ppSelectedLayerInfo->nNoteLength;
		
				m_layersSelected[ ii ] = pSampleInfo;
			}
			else {
				m_layersSelected[ ii ] = nullptr;
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

void Note::setVelocity( float velocity )
{
	m_fVelocity = check_boundary( velocity, VELOCITY_MIN, VELOCITY_MAX );
}

void Note::setLeadLag( float lead_lag )
{
	m_fLeadLag = check_boundary( lead_lag, LEAD_LAG_MIN, LEAD_LAG_MAX );
}

void Note::setPan( float val ) {
	m_fPan = check_boundary( val, PAN_MIN, PAN_MAX );
}

void Note::setHumanizeDelay( int nValue )
{
	// We do not perform bound checks with
	// AudioEngine::nMaxTimeHumanize in here as different contribution
	// could push the value first beyond and then within the bounds
	// again. The clamping will be done in computeNoteStart() instead.
	if ( nValue != m_nHumanizeDelay ) {
		m_nHumanizeDelay = nValue;
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
		pInstrument = pDrumkit->getInstruments()->find( m_nInstrumentId );

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
		m_pInstrument = pInstrument;
		m_pAdsr = pInstrument->copyAdsr();
		m_nInstrumentId = pInstrument->getId();

		m_layersSelected.clear();
		m_layersSelected.resize( pInstrument->getComponents()->size() );
		for ( int ii = 0; ii < pInstrument->getComponents()->size(); ++ii ) {
			const auto pCompo = pInstrument->getComponent( ii );
			if ( pCompo != nullptr ) {
				std::shared_ptr<SelectedLayerInfo> sampleInfo =
					std::make_shared<SelectedLayerInfo>();
				sampleInfo->nSelectedLayer = -1;
				sampleInfo->fSamplePosition = 0;
				sampleInfo->nNoteLength = LENGTH_ENTIRE_SAMPLE;

				m_layersSelected[ ii ] = sampleInfo;
			}
			else {
				m_layersSelected[ ii ] = nullptr;
			}
		}
	}
	else {
		INFOLOG( QString( "No instrument was found for type [%1] and ID [%2]." )
				 .arg( m_sType ).arg( m_nInstrumentId ) );
		m_pInstrument = nullptr;
		m_pAdsr = nullptr;
		m_layersSelected.clear();

		// In case no matching instrument was found, we reset the instrument ID.
		// But we only do so in case the note has an associated instrument type.
		// Else, there would be no way to map it back to the previous
		// instrument.
		if ( ! m_sType.isEmpty() ) {
			m_nInstrumentId = EMPTY_INSTR_ID;
		}
	}
}

void Note::setKeyOctave( const QString& str )
{
	int l = str.length();
	QString s_key = str.left( l-1 );
	QString s_oct = str.mid( l-1, l );
	if ( s_key.endsWith( "-" ) ) {
		s_key.replace( "-", "" );
		s_oct.insert( 0, "-" );
	}
	m_octave = ( Octave )s_oct.toInt();
	for( int i=KEY_MIN; i<=KEY_MAX; i++ ) {
		if( m_keyStr[i]==s_key ) {
			m_key = ( Key )i;
			return;
		}
	}
	___ERRORLOG( "Unhandled key: " + s_key );
}

bool Note::isPartiallyRendered() const {
	bool bRes = false;

	for ( const auto& ll : m_layersSelected ) {
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
		TransportPosition::computeFrameFromTick( m_nPosition, &fTickMismatch );

	m_nNoteStart += std::clamp( m_nHumanizeDelay,
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
	
	if ( m_pInstrument == nullptr ) {
		ERRORLOG( "Sample does not hold an instrument" );
		return nullptr;
	}

	auto pComponent = m_pInstrument->getComponent( nComponentIdx );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1] of instrument [%2]" )
				  .arg( nComponentIdx ).arg( m_pInstrument->getName() ) );
		return nullptr;
	}
	
	auto pSelectedLayer = getLayerSelected( nComponentIdx );
	if ( pSelectedLayer == nullptr ) {
		WARNINGLOG( QString( "No SelectedLayer for component [%1] of instrument [%2]" )
					.arg( pComponent->getName() ).arg( m_pInstrument->getName() ) );
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
		
		auto pLayer = pComponent->getLayer( nLayer );
		if ( pLayer == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve layer [%1] selected for component [%2] of instrument [%3]" )
					  .arg( nLayer ).arg( pComponent->getName() )
					  .arg( m_pInstrument->getName() ) );
			return nullptr;
		}
		
		pSample = pLayer->getSample();
			
	}
	else {
		// Select an instrument layer.
		std::vector<int> possibleLayersVector;
		int nLayersEncountered = 0;
		float fRoundRobinID;
		auto pSong = Hydrogen::get_instance()->getSong();
		
		for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); ++nLayer ) {
			auto pLayer = pComponent->getLayer( nLayer );
			if ( pLayer == nullptr ) {
				continue;
			}
			++nLayersEncountered;

			if ( ( m_fVelocity >= pLayer->getStartVelocity() ) &&
				 ( m_fVelocity <= pLayer->getEndVelocity() ) ) {

				possibleLayersVector.push_back( nLayer );
				if ( pComponent->getSelection() ==
					 InstrumentComponent::Selection::Velocity ) {
					break;
				}
				else if ( pComponent->getSelection() ==
						  InstrumentComponent::Selection::RoundRobin ) {
					fRoundRobinID = pLayer->getStartVelocity();
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
						.arg( m_fVelocity ).arg( pComponent->getName() )
						.arg( m_pInstrument->getName() ) );
			
			float shortestDistance = 1.0f;
			int nearestLayer = -1;
			for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); ++nLayer ){
				auto pLayer = pComponent->getLayer( nLayer );
				if ( pLayer == nullptr ){
					continue;
				}
							
				if ( std::min( abs( pLayer->getStartVelocity() - m_fVelocity ),
							   abs( pLayer->getStartVelocity() - m_fVelocity ) ) <
					 shortestDistance ){
					shortestDistance =
						std::min( abs( pLayer->getStartVelocity() - m_fVelocity ),
								  abs( pLayer->getStartVelocity() - m_fVelocity ) );
					nearestLayer = nLayer;
				}
			}

			// Check whether the search was successful and assign the results.
			if ( nearestLayer > -1 ){
				possibleLayersVector.push_back( nearestLayer );
				if ( pComponent->getSelection() ==
					 InstrumentComponent::Selection::RoundRobin ) {
					fRoundRobinID =
						pComponent->getLayer( nearestLayer )->getStartVelocity();
				}
			} else {
				ERRORLOG( QString( "No sample found for component [%1] of instrument [%2]" )
						  .arg( pComponent->getName() )
						  .arg( m_pInstrument->getName() ) );
				return nullptr;
			}
		}

		if ( possibleLayersVector.size() > 0 ) {

			int nLayerPicked;
			switch ( pComponent->getSelection() ) {
			case InstrumentComponent::Selection::Velocity:
				nLayerPicked = possibleLayersVector[ 0 ];
				break;
				
			case InstrumentComponent::Selection::Random:
				nLayerPicked = possibleLayersVector[ rand() %
													 possibleLayersVector.size() ];
				break;

			case InstrumentComponent::Selection::RoundRobin: {
				fRoundRobinID = m_pInstrument->getId() * 10 + fRoundRobinID;
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
						  .arg( InstrumentComponent::SelectionToQString(
									pComponent->getSelection() ) )
						  .arg( m_pInstrument->getName() ) );
				return nullptr;
			} 

			pSelectedLayer->nSelectedLayer = nLayerPicked;
			auto pLayer = pComponent->getLayer( nLayerPicked );
			pSample = pLayer->getSample();

		} else {
			ERRORLOG( "No samples found during random layer selection. This is a bug and shoul dn't happen!" );
		}
	}

	return pSample;
}

float Note::getTotalPitch() const
{
	float fNotePitch = m_octave * KEYS_PER_OCTAVE + m_key + m_fPitch;

	if ( m_pInstrument != nullptr ) {
		fNotePitch += m_pInstrument->getPitchOffset();
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
			setVelocity( m_fVelocity + fRandomVelocityFactor *
						  Random::getGaussian( AudioEngine::fHumanizeVelocitySD ) );
		}

		const float fRandomTimeFactor = pSong->getHumanizeTimeValue();
		if ( fRandomTimeFactor != 0 ) {
			setHumanizeDelay( m_nHumanizeDelay + fRandomTimeFactor *
								AudioEngine::nMaxTimeHumanize *
								Random::getGaussian( AudioEngine::fHumanizeTimingSD ) );
		}
	}

	if ( m_pInstrument != nullptr ) {
		const float fRandomPitchFactor = m_pInstrument->getRandomPitchFactor();
		if ( fRandomPitchFactor != 0 ) {
			m_fPitch += Random::getGaussian( AudioEngine::fHumanizePitchSD ) *
				fRandomPitchFactor;
			}
	}
}

void Note::swing() {
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr && pSong->getSwingFactor() > 0 ) {
		// If the Timeline is activated, the tick size may change at
		// any point. Therefore, the length in frames of a 16-th note
		// offset has to be calculated for a particular transport
		// position and is not generally applicable.
		double fTickMismatch;
		setHumanizeDelay(
			m_nHumanizeDelay +
			( TransportPosition::computeFrameFromTick(
				m_nPosition + H2Core::nTicksPerQuarter / 8., &fTickMismatch ) -
			  TransportPosition::computeFrameFromTick(
				  m_nPosition, &fTickMismatch ) ) * pSong->getSwingFactor() );
	}
}

void Note::saveTo( XMLNode& node ) const
{
	node.write_int( "position", m_nPosition );
	node.write_float( "leadlag", m_fLeadLag );
	node.write_float( "velocity", m_fVelocity );
	node.write_float( "pan", m_fPan );
	node.write_float( "pitch", m_fPitch );
	node.write_string( "key", QString( "%1%2" )
					   .arg( m_keyStr[m_key] ).arg( m_octave ) );
	node.write_int( "length", m_nLength );
	node.write_int( "instrument", m_nInstrumentId );
	node.write_string( "type", m_sType );
	node.write_bool( "note_off", m_bNoteOff );
	node.write_float( "probability", m_fProbability );
}

std::shared_ptr<Note> Note::loadFrom( const XMLNode& node, bool bSilent )
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
	pNote->setLeadLag(
		node.read_float( "leadlag", LEAD_LAG_DEFAULT, false, false, bSilent ) );
	pNote->setKeyOctave( node.read_string( "key", "C0", false, false, bSilent ) );
	pNote->setNoteOff( node.read_bool( "note_off", false, false, false, bSilent ) );
	pNote->setInstrumentId( node.read_int( "instrument", EMPTY_INSTR_ID, false, false, bSilent ) );
	pNote->setType( node.read_string( "type", "", true, true, bSilent ) );
	pNote->setProbability(
		node.read_float( "probability", PROBABILITY_DEFAULT, false, false, bSilent ));

	return pNote;
}

QString Note::prettyName() const {
	QString sInstrument;
	if ( m_pInstrument != nullptr ) {
		sInstrument = QString( "instr: [%1]" ).arg( m_pInstrument->getName() );
	} else {
		sInstrument = QString( "type: [%1]" ).arg( m_sType );
	}

	return QString( "%1, pos: %2, key: %3, octave: %4" )
		.arg( sInstrument ).arg( m_nPosition )
		.arg( KeyToQString( m_key ) ).arg( OctaveToQString( m_octave ) );
}

QString Note::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Note]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nInstrumentId: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nInstrumentId ) )
			.append( QString( "%1%2m_sType: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sType ) )
			.append( QString( "%1%2m_nPosition: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nPosition ) )
			.append( QString( "%1%2m_fVelocity: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fVelocity ) )
			.append( QString( "%1%2m_fPan: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPan ) )
			.append( QString( "%1%2m_nLength: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nLength ) )
			.append( QString( "%1%2m_fPitch: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPitch ) )
			.append( QString( "%1%2m_key: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( KeyToQString( m_key ) ) )
			.append( QString( "%1%2m_octave: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( OctaveToQString( m_octave ) ) );
		if ( m_pAdsr != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pAdsr->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2m_pAdsr: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
		sOutput.append( QString( "%1%2m_fLeadLag: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fLeadLag ) )
			.append( QString( "%1%2m_nHumanizeDelay: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nHumanizeDelay ) )
			.append( QString( "%1%2m_fBpfbL: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fBpfbL ) )
			.append( QString( "%1%2m_fBpfbR: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fBpfbR ) )
			.append( QString( "%1%2m_fLpfbL: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fLpfbL ) )
			.append( QString( "%1%2m_fLpfbR: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fLpfbR ) )
			.append( QString( "%1%2m_nMidiMsg: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nMidiMsg ) )
			.append( QString( "%1%2m_bNoteOff: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bNoteOff ) )
			.append( QString( "%1%2m_fProbability: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fProbability ) )
			.append( QString( "%1%2m_keyStr: [" ).arg( sPrefix ).arg( s ) );
			for ( int ii = KEY_MIN; ii <= KEY_MAX; ++ii ) {
					 sOutput.append( QString( "%1, " ).arg(
										 QString::fromUtf8( m_keyStr[ ii ], -1 ) ) );
			}
			sOutput.append( QString( "]\n%1%2m_nNoteStart: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nNoteStart ) )
			.append( QString( "%1%2m_fUsedTickSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fUsedTickSize ) )
			.append( QString( "%1%2m_nSpecificCompoIdx: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSpecificCompoIdx ) )
			.append( QString( "%1%2m_layersSelected:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppLayer : m_layersSelected ) {
			if ( ppLayer != nullptr ) {
				sOutput.append( QString( "%1%2[%3]\n" )
								.arg( sPrefix ).arg( s + s )
								.arg( ppLayer->toQString( "", true ) ) );
			} else {
				sOutput.append( QString( "%1%2[SelectedLayerInfo: nullptr]\n" )
								.arg( sPrefix ).arg( s + s ) );
			}
		}
		if ( m_pInstrument != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pInstrument->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2m_pInstrument: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
	} else {

		sOutput = QString( "[Note]" )
			.append( QString( ", m_nInstrumentId: %1" ).arg( m_nInstrumentId ) )
			.append( QString( ", m_sType: %1" ).arg( m_sType ) )
			.append( QString( ", m_nPosition: %1" ).arg( m_nPosition ) )
			.append( QString( ", m_fVelocity: %1" ).arg( m_fVelocity ) )
			.append( QString( ", m_fPan: %1" ).arg( m_fPan ) )
			.append( QString( ", m_nLength: %1" ).arg( m_nLength ) )
			.append( QString( ", m_fPitch: %1" ).arg( m_fPitch ) )
			.append( QString( ", m_key: %1" )
					 .arg( KeyToQString( m_key ) ) )
			.append( QString( ", m_octave: %1" )
					 .arg( OctaveToQString( m_octave ) ) );
		if ( m_pAdsr != nullptr ) {
			sOutput.append( QString( ", [%1" )
							.arg( m_pAdsr->toQString( sPrefix + s, bShort )
								  .replace( "\n", "]" ) ) );
		} else {
			sOutput.append( ", m_pAdsr: nullptr" );
		}

		sOutput.append( QString( ", m_fLeadLag: %1" ).arg( m_fLeadLag ) )
			.append( QString( ", m_nHumanizeDelay: %1" ).arg( m_nHumanizeDelay ) )
			.append( QString( ", m_fBpfbL: %1" ).arg( m_fBpfbL ) )
			.append( QString( ", m_fBpfbR: %1" ).arg( m_fBpfbR ) )
			.append( QString( ", m_fLlpfbL: %1" ).arg( m_fLpfbL ) )
			.append( QString( ", m_fLpfbR: %1" ).arg( m_fLpfbR ) )
			.append( QString( ", m_nMidiMsg: %1" ).arg( m_nMidiMsg ) )
			.append( QString( ", m_bNoteOff: %1" ).arg( m_bNoteOff ) )
			.append( QString( ", m_fProbability: %1" ).arg( m_fProbability ) )
			.append( ", m_keyStr: [" );
			for ( int ii = KEY_MIN; ii <= KEY_MAX; ++ii ) {
					 sOutput.append( QString( "%1, " ).arg(
										 QString::fromUtf8( m_keyStr[ ii ], -1 ) ) );
			}
			sOutput.append( QString( "], m_nNoteStart: %1" ).arg( m_nNoteStart ) )
			.append( QString( ", m_fUsedTickSize: %1" ).arg( m_fUsedTickSize ) )
			.append( QString( ", m_nSpecificCompoIdx: %1" )
					 .arg( m_nSpecificCompoIdx ) )
			.append( QString( ", m_layersSelected: " ) );
		for ( const auto& ppLayer : m_layersSelected ) {
			if ( ppLayer != nullptr ) {
				sOutput.append( QString( "[%1] " )
								.arg( ppLayer->toQString( "", true ) ) );
			} else {
				sOutput.append( "[SelectedLayerInfo: nullptr] " );
			}
		}
		if ( m_pInstrument != nullptr ) {
			sOutput.append( QString( ", m_pInstrument: %1" ).arg( m_pInstrument->getName() ) );
		} else {
			sOutput.append( QString( ", m_pInstrument: nullptr" ) );
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
