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

#include <core/Basics/InstrumentComponent.h>

#include <cassert>
#include <set>

#include <core/Basics/InstrumentLayer.h>
#include <core/Helpers/Xml.h>


namespace H2Core
{

InstrumentComponent::InstrumentComponent( const QString& sName, float fGain )
	: m_sName( sName )
	, m_fGain( fGain )
	, m_bIsMuted( false )
	, m_bIsSoloed( false )
	, m_selection( Selection::Velocity )
{
	/*: Name assigned to an InstrumentComponent of a fresh instrument. */
	const QString sComponentName =
		QT_TRANSLATE_NOOP( "InstrumentComponent", "Main");

	if ( sName.isEmpty() ) {
		m_sName = sComponentName;
	}
}

InstrumentComponent::InstrumentComponent( std::shared_ptr<InstrumentComponent> other )
	: m_sName( other->m_sName )
	, m_fGain( other->m_fGain )
	, m_bIsMuted( other->m_bIsMuted )
	, m_bIsSoloed( other->m_bIsSoloed )
	, m_selection( other->m_selection )
{
	for ( const auto& ppLayer : other->m_layers ) {
		if ( ppLayer != nullptr ) {
			m_layers.push_back( std::make_shared<InstrumentLayer>( ppLayer ) );
		}
	}
}

InstrumentComponent::~InstrumentComponent()
{
}

void InstrumentComponent::addLayer(
	std::shared_ptr<InstrumentLayer> pLayer,
	int nIndex
)
{
	if ( pLayer == nullptr ) {
		return;
	}

	if ( nIndex == -1 || nIndex == m_layers.size() ) {
      // Append
		m_layers.push_back( pLayer );
	}
	else if ( nIndex >= 0 && nIndex < m_layers.size() ) {
	  // Insert
      m_layers.insert( std::next( m_layers.begin(), nIndex ), pLayer );
	}
	else {
		ERRORLOG( QString( "Index [%1] out of bound [0, %2]" )
					  .arg( nIndex )
					  .arg( m_layers.size() ) );
	}
}

void InstrumentComponent::setLayer( std::shared_ptr<InstrumentLayer> pLayer, int nIndex )
{
	if ( nIndex < 0 || nIndex >= m_layers.size() ) {
		ERRORLOG( QString( "Index [%1] out of bound [0,%2]" )
					  .arg( nIndex )
					  .arg( m_layers.size() ) );
	  return;
	}
	m_layers[ nIndex ] = pLayer;
}

void InstrumentComponent::removeLayer( int nIndex )
{
	if ( nIndex < 0 || nIndex >= m_layers.size() ) {
		ERRORLOG( QString( "Index [%1] out of bound [0,%2]" )
					  .arg( nIndex )
					  .arg( m_layers.size() ) );
	  return;
	}
	m_layers.erase( std::next( m_layers.begin(), nIndex ) );
}

std::shared_ptr<InstrumentComponent> InstrumentComponent::loadFrom(
	const XMLNode& node,
	const QString& sDrumkitPath,
	const QString& sSongPath,
	const License& drumkitLicense,
	bool bSilent )
{
	auto pInstrumentComponent = std::make_shared<InstrumentComponent>();
	pInstrumentComponent->m_sName = node.read_string(
		"name", pInstrumentComponent->m_sName, true, false, true );
	pInstrumentComponent->m_fGain = node.read_float(
		"gain", pInstrumentComponent->m_fGain, true, false, bSilent );
	pInstrumentComponent->m_bIsMuted = node.read_bool(
		"isMuted", pInstrumentComponent->m_bIsMuted, true, false, true );
	pInstrumentComponent->m_bIsSoloed = node.read_bool(
		"isSoloed", pInstrumentComponent->m_bIsSoloed, true, false, true );
	const QString sSelection = node.read_string(
		"sampleSelectionAlgo", "", true, true, bSilent  );
	if ( sSelection.compare( "VELOCITY" ) == 0 ) {
		pInstrumentComponent->m_selection = Selection::Velocity;
	}
	else if ( sSelection.compare( "ROUND_ROBIN" ) == 0 ) {
		pInstrumentComponent->m_selection = Selection::RoundRobin;
	}
	else if ( sSelection.compare( "RANDOM" ) == 0 ) {
		pInstrumentComponent->m_selection = Selection::Random;
	}
	else {
		// Default option
		pInstrumentComponent->m_selection = Selection::Velocity;
	}

	XMLNode layer_node = node.firstChildElement( "layer" );
	while ( ! layer_node.isNull() ) {
		auto pLayer = InstrumentLayer::loadFrom(
			layer_node, sDrumkitPath, sSongPath, drumkitLicense, bSilent );
		if ( pLayer != nullptr ) {
			pInstrumentComponent->addLayer( pLayer, -1 );
		}
		layer_node = layer_node.nextSiblingElement( "layer" );
	}
	
	return pInstrumentComponent;
}

void InstrumentComponent::saveTo( XMLNode& node, bool bSongKit,
								 bool bKeepMissingSamples, bool bSilent )
{
	XMLNode component_node;
	component_node = node.createNode( "instrumentComponent" );
	component_node.write_string( "name", m_sName );
	component_node.write_float( "gain", m_fGain );
	component_node.write_bool( "isMuted", m_bIsMuted );
	component_node.write_bool( "isSoloed", m_bIsSoloed );

	switch ( m_selection ) {
	case Selection::Velocity:
		component_node.write_string( "sampleSelectionAlgo", "VELOCITY" );
		break;
	case Selection::Random:
		component_node.write_string( "sampleSelectionAlgo", "RANDOM" );
		break;
	case Selection::RoundRobin:
		component_node.write_string( "sampleSelectionAlgo", "ROUND_ROBIN" );
		break;
	}

	std::set<int> indicesToRemove;
	for ( int nn = 0; nn < m_layers.size(); nn++ ) {
		auto pLayer = m_layers[ nn ];
		if ( pLayer != nullptr ) {
			if ( pLayer->getSample() != nullptr || bKeepMissingSamples ) {
				pLayer->saveTo( component_node, bSongKit );
			}
			else {
				if ( ! bSilent ) {
					INFOLOG( QString( "Discarding layer of missing sample [%1]" )
							.arg( pLayer->getFallbackSampleFileName() ) );
				}
				else {
					indicesToRemove.insert( nn );
				}
				// Remove the layer missing a sample from the component.
			}
		}
	}

	if ( ! bKeepMissingSamples && indicesToRemove.size() > 0 ) {
		std::vector< std::shared_ptr<InstrumentLayer> > newLayers;
		for ( int nn = 0; nn < m_layers.size(); ++nn ) {
			if ( indicesToRemove.find( nn ) == indicesToRemove.end() ) {
				newLayers.push_back( m_layers[ nn ] );
			}
		}

		m_layers = newLayers;
	}
}

bool InstrumentComponent::hasSamples() const {
	for ( const auto& pLayer : m_layers ) {
		if ( pLayer != nullptr ) {
			if ( pLayer->getSample() != nullptr ) {
				return true;
			}
		}
	}

	return false;
}

bool InstrumentComponent::isAnyLayerSoloed() const {
	for ( const auto& ppLayer : m_layers ) {
		if ( ppLayer != nullptr && ppLayer->getIsSoloed() ) {
			return true;
		}
	}

	return false;
}

void InstrumentComponent::setAutoVelocity()
{
	if ( m_layers.size() == 0 ) {
		return;
	}

	const float fVelocityRange = 1.0 / static_cast<float>( m_layers.size() );

	int nLayer = 0;
	for ( auto& ppLayer : m_layers ) {
		if ( ppLayer != nullptr ) {
			ppLayer->setStartVelocity( nLayer * fVelocityRange );
			ppLayer->setEndVelocity( nLayer * fVelocityRange + fVelocityRange );

			++nLayer;
		}
	}
}

QString InstrumentComponent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentComponent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sName: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sName ) )
			.append( QString( "%1%2m_fGain: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fGain ) )
			.append( QString( "%1%2m_bIsMuted: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsMuted ) )
			.append( QString( "%1%2m_bIsSoloed: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsSoloed ) )
			.append( QString( "%1%2m_selection: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( SelectionToQString( m_selection ) ) )
			.append( QString( "%1%2m_layers:\n" ).arg( sPrefix ).arg( s ) );
	
		for ( const auto& ll : m_layers ) {
			if ( ll != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ll->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	} else {
		sOutput = QString( "[InstrumentComponent] " )
			.append( QString( "m_sName: %1" ).arg( m_sName ) )
			.append( QString( ", m_fGain: %1" ).arg( m_fGain ) )
			.append( QString( ", m_bIsMuted: %1" ).arg( m_bIsMuted ) )
			.append( QString( ", m_bIsSoloed: %1" ).arg( m_bIsSoloed ) )
			.append( QString( ", m_selection: %1" )
					 .arg( SelectionToQString( m_selection ) ) )
			.append( QString( ", m_layers: [" ) );
	
		for ( const auto& ll : m_layers ) {
			if ( ll != nullptr ) {
				sOutput.append( QString( " [%1" ).arg( ll->toQString( sPrefix + s + s, bShort ).replace( "\n", "]" ) ) );
			}
		}

		sOutput.append( "]\n" );

	}
	
	return sOutput;
}

QString InstrumentComponent::SelectionToQString( const Selection& selection ) {
	switch( selection ) {
	case InstrumentComponent::Selection::Velocity:
		return "Velocity";
	case InstrumentComponent::Selection::RoundRobin:
		return "Round Robin";
	case InstrumentComponent::Selection::Random:
		return "Random";
	default:
		return QString( "Unknown Selection [%1]" )
			.arg( static_cast<int>(selection) );
	}
}


const std::vector<std::shared_ptr<InstrumentLayer>> InstrumentComponent::getLayers() const {
	std::vector<std::shared_ptr<InstrumentLayer>> layersUsed;
	for ( const auto& layer : m_layers ) {
		if ( layer != nullptr ) {
			layersUsed.push_back( layer );
		}
	}
	
	return layersUsed;
}

int InstrumentComponent::index( std::shared_ptr<InstrumentLayer> pLayer ) const {
	for( int ii = 0; ii < m_layers.size(); ii++ ) {
		if ( m_layers.at( ii ) == pLayer ) {
			return ii;
		}
	}
	return -1;
}

std::vector<std::shared_ptr<InstrumentLayer>>::iterator InstrumentComponent::begin() {
	return m_layers.begin();
}

std::vector<std::shared_ptr<InstrumentLayer>>::iterator InstrumentComponent::end() {
	return m_layers.end();
}
};

/* vim: set softtabstop=4 noexpandtab: */
