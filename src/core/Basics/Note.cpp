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
#include "Midi/Midi.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Random.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>

namespace H2Core {

Note::Pitch Note::Pitch::Default = Note::Pitch( 0 );
Note::Pitch Note::Pitch::Invalid = Note::Pitch( 666 );
Note::Pitch Note::Pitch::Maximum = Note::Pitch(
	static_cast<float>( KEYS_PER_OCTAVE ) *
		static_cast<float>( Note::OctaveMax ) +
	static_cast<float>( Note::KeyMax )
);
Note::Pitch Note::Pitch::Minimum = Note::Pitch(
	static_cast<float>( KEYS_PER_OCTAVE ) *
		static_cast<float>( Note::OctaveMin ) +
	static_cast<float>( Note::KeyMin )
);

SelectedLayerInfo::SelectedLayerInfo()
	: pLayer( nullptr ),
	  fSamplePosition( 0.0 ),
	  nNoteLength( LENGTH_ENTIRE_SAMPLE )
{
}
SelectedLayerInfo::~SelectedLayerInfo()
{
}

QString SelectedLayerInfo::toQString( const QString& sPrefix, bool bShort )
	const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[SelectedLayerInfo]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2pLayer: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( pLayer->toQString( "", bShort ) ) )
					  .append( QString( "%1%2fSamplePosition: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( fSamplePosition ) )
					  .append( QString( "%1%2nNoteLength: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nNoteLength ) );
	}
	else {
		sOutput =
			QString( "[SelectedLayerInfo] " )
				.append( QString( "pLayer: %1" )
							 .arg( pLayer->toQString( "", bShort ) ) )
				.append(
					QString( ", fSamplePosition: %1" ).arg( fSamplePosition )
				)
				.append( QString( ", nNoteLength: %1" ).arg( nNoteLength ) );
	}

	return sOutput;
}

Note::Note(
	std::shared_ptr<Instrument> pInstrument,
	int nPosition,
	float fVelocity,
	float fPan,
	int nLength,
	float fPitch
)
	: m_instrumentId( Instrument::EmptyId ),
	  m_sType( "" ),
	  m_nPosition( nPosition ),
	  m_fVelocity( fVelocity ),
	  m_nLength( nLength ),
	  m_fPitch( fPitch ),
	  m_key( Note::KeyDefault ),
	  m_octave( Note::OctaveDefault ),
	  m_pAdsr( nullptr ),
	  m_fLeadLag( LEAD_LAG_DEFAULT ),
	  m_nHumanizeDelay( 0 ),
	  m_fBpfbL( 0.0 ),
	  m_fBpfbR( 0.0 ),
	  m_fLpfbL( 0.0 ),
	  m_fLpfbR( 0.0 ),
	  m_bNoteOff( false ),
	  m_fProbability( PROBABILITY_DEFAULT ),
	  m_nNoteStart( 0 ),
	  m_fUsedTickSize( std::nan( "" ) ),
	  m_pInstrument( pInstrument )
{
	if ( pInstrument != nullptr ) {
		m_pAdsr = pInstrument->copyAdsr();
		m_instrumentId = pInstrument->getId();
		m_sType = pInstrument->getType();
	}

	setPan( fPan );	 // this checks the boundaries
}

Note::Note( std::shared_ptr<Note> pOther )
	: m_instrumentId( pOther->getInstrumentId() ),
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
	  m_bNoteOff( pOther->getNoteOff() ),
	  m_fProbability( pOther->getProbability() ),
	  m_nNoteStart( pOther->getNoteStart() ),
	  m_fUsedTickSize( pOther->getUsedTickSize() ),
	  m_pInstrument( pOther->getInstrument() )
{
	if ( m_pInstrument != nullptr ) {
		m_pAdsr = m_pInstrument->copyAdsr();
		m_instrumentId = m_pInstrument->getId();

		for ( const auto& [ppOtherComponent, ppOtherSelectedLayerInfo] :
			  pOther->m_selectedLayerInfoMap ) {
			if ( ppOtherComponent != nullptr &&
				 ppOtherSelectedLayerInfo != nullptr ) {
				// We took a deep copy of the instrument and have to ensure we
				// point to the right component.
				auto pComponent = m_pInstrument->getComponent(
					pOther->m_pInstrument->index( ppOtherComponent )
				);
				if ( pComponent == nullptr ) {
					continue;
				}

				auto pSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
				pSelectedLayerInfo->pLayer = ppOtherSelectedLayerInfo->pLayer;
				pSelectedLayerInfo->fSamplePosition =
					ppOtherSelectedLayerInfo->fSamplePosition;
				pSelectedLayerInfo->nNoteLength =
					ppOtherSelectedLayerInfo->nNoteLength;

				setSelectedLayerInfo( pSelectedLayerInfo, pComponent );
			}
		}
	}
}

Note::~Note()
{
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

void Note::setPan( float val )
{
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

bool Note::isPartiallyRendered() const
{
	for ( const auto& [_, ppSelectedLayerInfo] : m_selectedLayerInfoMap ) {
		if ( ppSelectedLayerInfo != nullptr &&
			 ppSelectedLayerInfo->fSamplePosition > 0 ) {
			return true;
		}
	}

	return false;
}

void Note::computeNoteStart()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	double fTickMismatch;
	m_nNoteStart =
		TransportPosition::computeFrameFromTick( m_nPosition, &fTickMismatch );

	m_nNoteStart += std::clamp(
		m_nHumanizeDelay, -1 * AudioEngine::nMaxTimeHumanize,
		AudioEngine::nMaxTimeHumanize
	);

	// No note can start before the beginning of the song.
	if ( m_nNoteStart < 0 ) {
		m_nNoteStart = 0;
	}

	if ( pHydrogen->isTimelineEnabled() ) {
		m_fUsedTickSize = -1;
	}
	else {
		// This is used for triggering recalculation in case the tempo
		// changes where manually applied by the user. They are not
		// dependent on a particular position of the transport (as
		// Timeline is not activated).
		m_fUsedTickSize = pAudioEngine->getTransportPosition()->getTickSize();
	}
}

bool Note::layersAlreadySelected() const
{
	for ( const auto& [_, ppSelectedLayerInfo] : m_selectedLayerInfoMap ) {
		if ( ppSelectedLayerInfo != nullptr &&
			 ppSelectedLayerInfo->pLayer != nullptr ) {
			return true;
		}
	}

	return false;
}

void Note::selectLayers( const std::map<
						 std::shared_ptr<InstrumentComponent>,
						 std::shared_ptr<InstrumentLayer> >& lastUsedLayers )
{
	std::shared_ptr<Sample> pSample;

	if ( m_pInstrument == nullptr ) {
		ERRORLOG( "Sample does not hold an instrument" );
		return;
	}

	auto selectLayer = [=]( std::shared_ptr<InstrumentComponent> pComponent ) {
		// Aggregate all layers covering the velocity of this note.
		std::vector<std::shared_ptr<InstrumentLayer> > possibleLayersVector;
		int nLayersEncountered = 0;
		const bool bLayersSoloed = pComponent->isAnyLayerSoloed();
		auto pSong = Hydrogen::get_instance()->getSong();

		for ( const auto& ppLayer : pComponent->getLayers() ) {
			if ( ppLayer == nullptr || ppLayer->getIsMuted() ||
				 ( bLayersSoloed && !ppLayer->getIsSoloed() ) ) {
				continue;
			}
			++nLayersEncountered;

			if ( m_fVelocity >= ppLayer->getStartVelocity() &&
				 m_fVelocity <= ppLayer->getEndVelocity() ) {
				possibleLayersVector.push_back( ppLayer );
				if ( pComponent->getSelection() ==
					 InstrumentComponent::Selection::Velocity ) {
					// In case of "First in velocity", we select the first layer
					// encountered. The order in
					// InstrumentComponent::getLayers() corresponds to the order
					// shown in the ComponentView.
					return ppLayer;
				}
			}
		}

		if ( nLayersEncountered == 0 ) {
			// Not returning `nullptr` directly because the return type
			// detection of the lambda functions fails.
			std::shared_ptr<InstrumentLayer> pNoLayer = nullptr;
			return pNoLayer;
		}

		// In some instruments the start and end velocities of a layer
		// are not set perfectly giving rise to some 'holes'.
		// Occasionally the velocity of a note can fall into it
		// causing the sampler to just skip it. Instead, we will
		// search for the nearest sample and play this one instead.
		if ( possibleLayersVector.size() == 0 ) {
			WARNINGLOG(
				QString(
					"Velocity [%1] did fall into a hole between the instrument "
					"layers for component [%2] of instrument [%3]."
				)
					.arg( m_fVelocity )
					.arg( pComponent->getName() )
					.arg( m_pInstrument->getName() )
			);

			float fShortestDistance = 1.0f;
			std::shared_ptr<InstrumentLayer> pNearestLayer = nullptr;
			for ( const auto& ppLayer : pComponent->getLayers() ) {
				if ( ppLayer == nullptr || ppLayer->getIsMuted() ||
					 ( bLayersSoloed && !ppLayer->getIsSoloed() ) ) {
					continue;
				}

				if ( std::min(
						 abs( ppLayer->getStartVelocity() - m_fVelocity ),
						 abs( ppLayer->getStartVelocity() - m_fVelocity )
					 ) < fShortestDistance ) {
					fShortestDistance = std::min(
						abs( ppLayer->getStartVelocity() - m_fVelocity ),
						abs( ppLayer->getStartVelocity() - m_fVelocity )
					);
					pNearestLayer = ppLayer;
				}
			}

			// Check whether the search was successful and assign the results.
			if ( pNearestLayer != nullptr ) {
				possibleLayersVector.push_back( pNearestLayer );
			}
			else {
				ERRORLOG(
					QString(
						"No sample found for component [%1] of instrument [%2]"
					)
						.arg( pComponent->getName() )
						.arg( m_pInstrument->getName() )
				);
				std::shared_ptr<InstrumentLayer> pNoLayer = nullptr;
				return pNoLayer;
			}
		}

		// Select a layer.
		if ( possibleLayersVector.size() == 0 ) {
			ERRORLOG(
				"No samples found during random layer selection. This is a bug "
				"and shouldn't happen!"
			);
			std::shared_ptr<InstrumentLayer> pNoLayer = nullptr;
			return pNoLayer;
		}

		std::shared_ptr<InstrumentLayer> pSelectedLayer = nullptr;
		switch ( pComponent->getSelection() ) {
			case InstrumentComponent::Selection::Velocity:
				pSelectedLayer = possibleLayersVector[0];
				break;

			case InstrumentComponent::Selection::Random:
				pSelectedLayer =
					possibleLayersVector[rand() % possibleLayersVector.size()];
				break;

			case InstrumentComponent::Selection::RoundRobin:
				// We check whether there was already a layer for this
				// component. If so, we use the next one.
				if ( lastUsedLayers.find( pComponent ) !=
					 lastUsedLayers.end() ) {
					auto pLastLayer = lastUsedLayers.at( pComponent );
					if ( pLastLayer != nullptr ) {
						// Is the last layer among the possible ones?
						int nNextLayerIndex = -1;
						for ( int ii = 0; ii < possibleLayersVector.size();
							  ++ii ) {
							if ( pLastLayer == possibleLayersVector[ii] ) {
								// Found. We select the next layer.
								nNextLayerIndex = ii + 1;
								break;
							}
						}

						if ( nNextLayerIndex >= 0 &&
							 nNextLayerIndex < possibleLayersVector.size() ) {
							pSelectedLayer =
								possibleLayersVector[nNextLayerIndex];
						}
						else if ( nNextLayerIndex ==
								  possibleLayersVector.size() ) {
							// We use periodic boundary conditions and start
							// over with the first layer.
							pSelectedLayer = possibleLayersVector[0];
						}
					}
				}

				if ( pSelectedLayer == nullptr ) {
					// Cache miss. We start again at the top.
					pSelectedLayer = possibleLayersVector[0];
				}
				break;

			default:
				ERRORLOG(
					QString(
						"Unknown selection algorithm [%1] for instrument [%2]"
					)
						.arg( InstrumentComponent::SelectionToQString(
							pComponent->getSelection()
						) )
						.arg( m_pInstrument->getName() )
				);
				break;
		}

		return pSelectedLayer;
	};

	if ( m_selectedLayerInfoMap.size() > 0 ) {
		// Some components have been manually set. We only use selection for
		// those with invalid selection info or layer.
		for ( const auto& [ppComponent, ppSelectedLayerInfo] :
			  m_selectedLayerInfoMap ) {
			if ( ppComponent == nullptr ) {
				continue;
			}

			if ( ppSelectedLayerInfo == nullptr ||
				 ppSelectedLayerInfo->pLayer == nullptr ) {
				auto pNewSelectedLayerInfo =
					std::make_shared<SelectedLayerInfo>();
				pNewSelectedLayerInfo->pLayer = selectLayer( ppComponent );
				m_selectedLayerInfoMap[ppComponent] = pNewSelectedLayerInfo;
			}
		}
	}
	else {
		// Select layers for all components
		for ( const auto& ppComponent : *m_pInstrument->getComponents() ) {
			auto pNewSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
			pNewSelectedLayerInfo->pLayer = selectLayer( ppComponent );
			m_selectedLayerInfoMap[ppComponent] = pNewSelectedLayerInfo;
		}
	}
}

std::shared_ptr<SelectedLayerInfo> Note::getSelecterLayerInfo(
	std::shared_ptr<InstrumentComponent> pComponent
) const
{
	if ( m_selectedLayerInfoMap.find( pComponent ) !=
		 m_selectedLayerInfoMap.end() ) {
		return m_selectedLayerInfoMap.at( pComponent );
	}

	return nullptr;
}
void Note::setSelectedLayerInfo(
	std::shared_ptr<SelectedLayerInfo> pInfo,
	std::shared_ptr<InstrumentComponent> pComponent
)
{
	if ( pComponent == nullptr ) {
		ERRORLOG( "Invalid component" );
		return;
	}

	m_selectedLayerInfoMap[pComponent] = pInfo;
}

void Note::mapToInstrument( std::shared_ptr<Instrument> pInstrument )
{
	if ( pInstrument != nullptr ) {
		m_pInstrument = pInstrument;
		m_pAdsr = pInstrument->copyAdsr();
		m_instrumentId = pInstrument->getId();
	}
	else {
		m_pInstrument = nullptr;
		m_pAdsr = nullptr;
		// The instrument ID will be kept to avoid any loss of information.
	}

	m_selectedLayerInfoMap.clear();
}

float Note::getTotalPitch() const
{
	float fNotePitch = static_cast<int>( m_octave ) * KEYS_PER_OCTAVE +
					   static_cast<int>( m_key ) + m_fPitch;

	if ( m_pInstrument != nullptr ) {
		fNotePitch += m_pInstrument->getPitchOffset();
	}
	return fNotePitch;
}

void Note::humanize()
{
	// Due to the nature of the Gaussian distribution, the factors
	// will also scale the standard deviations of the generated random
	// variables.
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		const float fRandomVelocityFactor = pSong->getHumanizeVelocityValue();
		if ( fRandomVelocityFactor != 0 ) {
			setVelocity(
				m_fVelocity +
				fRandomVelocityFactor *
					Random::getGaussian( AudioEngine::fHumanizeVelocitySD )
			);
		}

		const float fRandomTimeFactor = pSong->getHumanizeTimeValue();
		if ( fRandomTimeFactor != 0 ) {
			setHumanizeDelay(
				m_nHumanizeDelay +
				fRandomTimeFactor * AudioEngine::nMaxTimeHumanize *
					Random::getGaussian( AudioEngine::fHumanizeTimingSD )
			);
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

void Note::swing()
{
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
				  m_nPosition + H2Core::nTicksPerQuarter / 8., &fTickMismatch
			  ) -
			  TransportPosition::computeFrameFromTick(
				  m_nPosition, &fTickMismatch
			  ) ) *
				pSong->getSwingFactor()
		);
	}
}

void Note::saveTo( XMLNode& node ) const
{
	node.write_int( "position", m_nPosition );
	node.write_float( "leadlag", m_fLeadLag );
	node.write_float( "velocity", m_fVelocity );
	node.write_float( "pan", m_fPan );
	node.write_float( "pitch", m_fPitch );
	node.write_string(
		"key", QString( "%1%2" )
				   .arg( Note::KeyToQString( m_key ) )
				   .arg( static_cast<int>( m_octave ) )
	);
	node.write_int( "length", m_nLength );
	node.write_int( "instrument", static_cast<int>( m_instrumentId ) );
	node.write_string( "type", m_sType );
	node.write_bool( "note_off", m_bNoteOff );
	node.write_float( "probability", m_fProbability );
}

std::shared_ptr<Note> Note::loadFrom( const XMLNode& node, bool bSilent )
{
	auto pNote = std::make_shared<Note>();

	bool bFound, bFound2;
	float fPan =
		node.read_float( "pan", pNote->getPan(), &bFound, true, false, true );
	if ( !bFound ) {
		// check if pan is expressed in the old fashion (version <=
		// 1.1 ) with the pair (pan_L, pan_R)
		float fPanL =
			node.read_float( "pan_L", 1.f, &bFound, false, false, bSilent );
		float fPanR =
			node.read_float( "pan_R", 1.f, &bFound2, false, false, bSilent );
		if ( bFound && bFound2 ) {
			fPan = Sampler::getRatioPan(
				fPanL, fPanR
			);	// convert to single pan parameter
		}
		else {
			WARNINGLOG(
				QString( "Neither `pan` nor `pan_L` and `pan_R` were found. "
						 "Falling back to `pan = 0`" )
			);
		}
	}
	pNote->setPan( fPan );

	pNote->setPosition(
		node.read_int( "position", pNote->getPosition(), false, false, bSilent )
	);
	pNote->setVelocity( node.read_float(
		"velocity", pNote->getVelocity(), false, false, bSilent
	) );
	pNote->setLength(
		node.read_int( "length", pNote->getLength(), true, false, bSilent )
	);
	pNote->m_fPitch =
		node.read_float( "pitch", pNote->getPitch(), false, false, bSilent );
	pNote->setLeadLag(
		node.read_float( "leadlag", pNote->getLeadLag(), false, false, bSilent )
	);
	const QString sKeyOctave =
		node.read_string( "key", "C0", false, false, bSilent );
	const int nKeyOctaveLength = sKeyOctave.length();
	QString sKey = sKeyOctave.left( nKeyOctaveLength - 1 );
	QString sOctave = sKeyOctave.mid( nKeyOctaveLength - 1, nKeyOctaveLength );
	if ( sKey.endsWith( "-" ) ) {
		sKey.replace( "-", "" );
		sOctave.insert( 0, "-" );
	}

	const int nOctave = sOctave.toInt();
	auto octave = Note::OctaveDefault;
	if ( nOctave >= static_cast<int>( Note::OctaveMin ) &&
		 nOctave <= static_cast<int>( Note::OctaveMax ) ) {
		octave = static_cast<Note::Octave>( nOctave );
	}
	else {
		ERRORLOG( QString( "Octave value [%1] out of bound" ).arg( nOctave ) );
	}
	auto key = Note::QStringToKey( sKey );
	if ( key == Key::Invalid ) {
		ERRORLOG( QString( "Invalid key [%1]" ).arg( sKey ) );
		key = Note::KeyDefault;
	}
	pNote->setKeyOctave( key, octave );
	pNote->setNoteOff(
		node.read_bool( "note_off", pNote->getNoteOff(), false, false, bSilent )
	);
	pNote->setInstrumentId( static_cast<Instrument::Id>( node.read_int(
		"instrument", static_cast<int>( pNote->getInstrumentId() ), false,
		false, bSilent
	) ) );
	pNote->setType(
		node.read_string( "type", pNote->getType(), true, true, bSilent )
	);
	pNote->setProbability( node.read_float(
		"probability", pNote->getProbability(), false, false, bSilent
	) );

	return pNote;
}

QString Note::prettyName() const
{
	QString sInstrument;
	if ( m_pInstrument != nullptr ) {
		sInstrument = QString( "instr: [%1]" ).arg( m_pInstrument->getName() );
	}
	else {
		sInstrument = QString( "type: [%1]" ).arg( m_sType );
	}

	return QString( "%1, pos: %2, key: %3, octave: %4" )
		.arg( sInstrument )
		.arg( m_nPosition )
		.arg( KeyToQString( m_key ) )
		.arg( OctaveToQString( m_octave ) );
}

QString Note::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[Note]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2m_nInstrumentId: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( static_cast<int>( m_instrumentId ) ) )
					  .append( QString( "%1%2m_sType: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_sType ) )
					  .append( QString( "%1%2m_nPosition: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_nPosition ) )
					  .append( QString( "%1%2m_fVelocity: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_fVelocity ) )
					  .append( QString( "%1%2m_fPan: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_fPan ) )
					  .append( QString( "%1%2m_nLength: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_nLength ) )
					  .append( QString( "%1%2m_fPitch: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_fPitch ) )
					  .append( QString( "%1%2m_key: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( KeyToQString( m_key ) ) )
					  .append( QString( "%1%2m_octave: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( OctaveToQString( m_octave ) ) );
		if ( m_pAdsr != nullptr ) {
			sOutput.append(
				QString( "%1" ).arg( m_pAdsr->toQString( sPrefix + s, bShort ) )
			);
		}
		else {
			sOutput.append(
				QString( "%1%2m_pAdsr: nullptr\n" ).arg( sPrefix ).arg( s )
			);
		}
		sOutput
			.append( QString( "%1%2m_fLeadLag: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fLeadLag ) )
			.append( QString( "%1%2m_nHumanizeDelay: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nHumanizeDelay ) )
			.append( QString( "%1%2m_fBpfbL: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fBpfbL ) )
			.append( QString( "%1%2m_fBpfbR: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fBpfbR ) )
			.append( QString( "%1%2m_fLpfbL: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fLpfbL ) )
			.append( QString( "%1%2m_fLpfbR: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fLpfbR ) )
			.append( QString( "%1%2m_bNoteOff: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bNoteOff ) )
			.append( QString( "%1%2m_fProbability: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fProbability ) )
			.append( QString( "%1%2m_nNoteStart: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nNoteStart ) )
			.append( QString( "%1%2m_fUsedTickSize: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fUsedTickSize ) )
			.append( QString( "%1%2m_selectedLayerInfoMap:\n" )
						 .arg( sPrefix )
						 .arg( s ) );
		for ( const auto& [ppComponent, ppSelectedLayerInfo] :
			  m_selectedLayerInfoMap ) {
			sOutput.append(
				QString( "%1%2%2[%3]: %4" )
					.arg( sPrefix )
					.arg( s )
					.arg(
						ppComponent != nullptr ? ppComponent->getName()
											   : "nullptr"
					)
					.arg(
						ppSelectedLayerInfo != nullptr
							? ppSelectedLayerInfo->toQString( "", true )
							: "nullptr"
					)
			);
		}
		if ( m_pInstrument != nullptr ) {
			sOutput.append( QString( "%1" ).arg(
				m_pInstrument->toQString( sPrefix + s, bShort )
			) );
		}
		else {
			sOutput.append( QString( "%1%2m_pInstrument: nullptr\n" )
								.arg( sPrefix )
								.arg( s ) );
		}
	}
	else {
		sOutput =
			QString( "[Note]" )
				.append( QString( ", m_instrumentId: %1" )
							 .arg( static_cast<int>( m_instrumentId ) ) )
				.append( QString( ", m_sType: %1" ).arg( m_sType ) )
				.append( QString( ", m_nPosition: %1" ).arg( m_nPosition ) )
				.append( QString( ", m_fVelocity: %1" ).arg( m_fVelocity ) )
				.append( QString( ", m_fPan: %1" ).arg( m_fPan ) )
				.append( QString( ", m_nLength: %1" ).arg( m_nLength ) )
				.append( QString( ", m_fPitch: %1" ).arg( m_fPitch ) )
				.append( QString( ", m_key: %1" ).arg( KeyToQString( m_key ) ) )
				.append( QString( ", m_octave: %1" )
							 .arg( OctaveToQString( m_octave ) ) );
		if ( m_pAdsr != nullptr ) {
			sOutput.append( QString( ", [%1" ).arg(
				m_pAdsr->toQString( sPrefix + s, bShort ).replace( "\n", "]" )
			) );
		}
		else {
			sOutput.append( ", m_pAdsr: nullptr" );
		}

		sOutput.append( QString( ", m_fLeadLag: %1" ).arg( m_fLeadLag ) )
			.append( QString( ", m_nHumanizeDelay: %1" ).arg( m_nHumanizeDelay )
			)
			.append( QString( ", m_fBpfbL: %1" ).arg( m_fBpfbL ) )
			.append( QString( ", m_fBpfbR: %1" ).arg( m_fBpfbR ) )
			.append( QString( ", m_fLlpfbL: %1" ).arg( m_fLpfbL ) )
			.append( QString( ", m_fLpfbR: %1" ).arg( m_fLpfbR ) )
			.append( QString( ", m_bNoteOff: %1" ).arg( m_bNoteOff ) )
			.append( QString( ", m_fProbability: %1" ).arg( m_fProbability ) )
			.append( QString( ", m_nNoteStart: %1" ).arg( m_nNoteStart ) )
			.append( QString( ", m_fUsedTickSize: %1" ).arg( m_fUsedTickSize ) )
			.append( QString( ", m_selectedLayerInfoMap: [" ) );
		QStringList selectedLayerInfos;
		for ( const auto& [ppComponent, ppSelectedLayerInfo] :
			  m_selectedLayerInfoMap ) {
			selectedLayerInfos
				<< QString( "[%1]: %2" )
					   .arg(
						   ppComponent != nullptr ? ppComponent->getName()
												  : "nullptr"
					   )
					   .arg(
						   ppSelectedLayerInfo != nullptr
							   ? ppSelectedLayerInfo->toQString( "", true )
							   : "nullptr"
					   );
		}
		sOutput.append( selectedLayerInfos.join( ", " ) ).append( "]" );
		if ( m_pInstrument != nullptr ) {
			sOutput.append(
				QString( ", m_pInstrument: %1" ).arg( m_pInstrument->getName() )
			);
		}
		else {
			sOutput.append( QString( ", m_pInstrument: nullptr" ) );
		}
	}
	return sOutput;
}

QString Note::KeyToQString( const Key& key )
{
	switch ( key ) {
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
		case Key::Invalid:
			return "Invalid";
		default:
			return QString( "Unknown Key value [%1]" )
				.arg( static_cast<int>( key ) );
	}
}

Note::Key Note::QStringToKey( const QString& sKey )
{
	if ( sKey == "C" ) {
		return Key::C;
	}
	else if ( sKey == "Cs" ) {
		return Key::Cs;
	}
	else if ( sKey == "D" ) {
		return Key::D;
	}
	else if ( sKey == "Ef" ) {
		return Key::Ef;
	}
	else if ( sKey == "E" ) {
		return Key::E;
	}
	else if ( sKey == "F" ) {
		return Key::F;
	}
	else if ( sKey == "Fs" ) {
		return Key::Fs;
	}
	else if ( sKey == "G" ) {
		return Key::G;
	}
	else if ( sKey == "Af" ) {
		return Key::Af;
	}
	else if ( sKey == "A" ) {
		return Key::A;
	}
	else if ( sKey == "Bf" ) {
		return Key::Bf;
	}
	else if ( sKey == "B" ) {
		return Key::B;
	}
	else {
		return Key::Invalid;
	}
}

Note::Key Note::keyFromInt( int nKey )
{
	if ( nKey >= static_cast<int>( KeyMin ) &&
		 nKey <= static_cast<int>( KeyMax ) ) {
		return static_cast<Key>( nKey );
	}
	else {
		return Key::Invalid;
	}
}

Note::Key Note::keyFromIntClamp( int nKey )
{
	return static_cast<Key>( std::clamp(
		nKey, static_cast<int>( KeyMin ), static_cast<int>( KeyMax )
	) );
}

Note::Key Note::keyFrom( Midi::Note note )
{
	if ( note == Midi::NoteInvalid ) {
		return Key::Invalid;
	}
	const int nDivider = static_cast<int>( note ) / KEYS_PER_OCTAVE;
	return Note::keyFromIntClamp(
		static_cast<int>( note ) - ( KEYS_PER_OCTAVE * nDivider )
	);
}

QString Note::OctaveToQString( const Octave& octave )
{
	switch ( octave ) {
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
		case Octave::Invalid:
		default:
			return QString( "Unknown octave value [%1]" )
				.arg( static_cast<int>( octave ) );
	}
}

Note::Octave Note::octaveFromInt( int nOctave )
{
	if ( nOctave >= static_cast<int>( OctaveMin ) &&
		 nOctave <= static_cast<int>( OctaveMax ) ) {
		return static_cast<Octave>( nOctave );
	}
	else {
		return Octave::Invalid;
	}
}

Note::Octave Note::octaveFromIntClamp( int nOctave )
{
	return static_cast<Octave>( std::clamp(
		nOctave, static_cast<int>( OctaveMin ), static_cast<int>( OctaveMax )
	) );
}

Note::Octave Note::octaveFrom( Midi::Note note )
{
	if ( note == Midi::NoteInvalid ) {
		return Octave::Invalid;
	}
	const int nDivider = static_cast<int>( note ) / KEYS_PER_OCTAVE;
	return Note::octaveFromIntClamp( nDivider - OCTAVE_OFFSET );
}

};	// namespace H2Core

/* vim: set softtabstop=4 noexpandtab: */
