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

#include <core/AudioEngine/AudioEngine.h>

#include <core/Helpers/Xml.h>
#include <core/Helpers/Filesystem.h>
#include <core/License.h>

#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>

namespace H2Core
{

int InstrumentComponent::m_nMaxLayers;

InstrumentComponent::InstrumentComponent( int related_drumkit_componentID )
	: __related_drumkit_componentID( related_drumkit_componentID )
	, __gain( 1.0 )
{
	__layers.resize( m_nMaxLayers );
	for ( int i = 0; i < m_nMaxLayers; i++ ) {
		__layers[i] = nullptr;
	}
}

InstrumentComponent::InstrumentComponent( std::shared_ptr<InstrumentComponent> other )
	: __related_drumkit_componentID( other->__related_drumkit_componentID )
	, __gain( other->__gain )
{
	__layers.resize( m_nMaxLayers );
	for ( int i = 0; i < m_nMaxLayers; i++ ) {
		std::shared_ptr<InstrumentLayer> other_layer = other->get_layer( i );
		if ( other_layer ) {
			__layers[i] = std::make_shared<InstrumentLayer>( other_layer );
		} else {
			__layers[i] = nullptr;
		}
	}
}

InstrumentComponent::~InstrumentComponent()
{
	for ( int i = 0; i < m_nMaxLayers; i++ ) {
		__layers[i] = nullptr;
	}
}

void InstrumentComponent::set_layer( std::shared_ptr<InstrumentLayer> layer, int idx )
{
	assert( idx >= 0 && idx < m_nMaxLayers );
	__layers[ idx ] = layer;
}

void InstrumentComponent::setMaxLayers( int layers )
{
	m_nMaxLayers = layers;
}

int InstrumentComponent::getMaxLayers()
{
	return m_nMaxLayers;
}

std::shared_ptr<InstrumentComponent> InstrumentComponent::load_from(
	XMLNode* pNode,
	const QString& sDrumkitPath,
	const QString& sSongPath,
	const License& drumkitLicense,
	bool bSilent )
{
	// Component IDs are essential. But in InstrumentComponents created in
	// version >= v2.0 they are not present anymore. Instead, we just fallback
	// to the default value and replace them at a later point of the loading
	// process.
	int nId = pNode->read_int( "component_id", EMPTY_INSTR_ID,
							   true, false, true );

	auto pInstrumentComponent = std::make_shared<InstrumentComponent>( nId );
	pInstrumentComponent->set_gain( pNode->read_float( "gain", 1.0f,
													  true, false, bSilent ) );
	XMLNode layer_node = pNode->firstChildElement( "layer" );
	int nLayer = 0;
	while ( ! layer_node.isNull() ) {
		if ( nLayer >= m_nMaxLayers ) {
			ERRORLOG( QString( "Layer #%1 >= m_nMaxLayers (%2). This as well as all further layers will be omitted." )
					  .arg( nLayer ).arg( m_nMaxLayers ) );
			break;
		}

		auto pLayer = InstrumentLayer::load_from(
			&layer_node, sDrumkitPath, sSongPath, drumkitLicense, bSilent );
		if ( pLayer != nullptr ) {
			pInstrumentComponent->set_layer( pLayer, nLayer );
			nLayer++;
		}
		layer_node = layer_node.nextSiblingElement( "layer" );
	}
	
	return pInstrumentComponent;
}

void InstrumentComponent::save_to( XMLNode* node, int component_id, bool bRecentVersion, bool bFull )
{
	XMLNode component_node;
	if ( bRecentVersion ) {
		component_node = node->createNode( "instrumentComponent" );
		component_node.write_int( "component_id", __related_drumkit_componentID );
		component_node.write_float( "gain", __gain );
	}
	for ( int n = 0; n < m_nMaxLayers; n++ ) {
		auto pLayer = get_layer( n );
		if( pLayer != nullptr ) {
			if( bRecentVersion ) {
				pLayer->save_to( &component_node, bFull );
			} else {
				pLayer->save_to( node, bFull );
			}
		}
	}
}

QString InstrumentComponent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentComponent]\n" ).arg( sPrefix )
			.append( QString( "%1%2related_drumkit_componentID: %3\n" ).arg( sPrefix ).arg( s ).arg( __related_drumkit_componentID ) )
			.append( QString( "%1%2gain: %3\n" ).arg( sPrefix ).arg( s ).arg( __gain ) )
			.append( QString( "%1%2m_nMaxLayers: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nMaxLayers ) )
			.append( QString( "%1%2layers:\n" ).arg( sPrefix ).arg( s ) );
	
		for ( auto ll : __layers ) {
			if ( ll != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ll->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	} else {
		sOutput = QString( "[InstrumentComponent]" )
			.append( QString( " related_drumkit_componentID: %1" ).arg( __related_drumkit_componentID ) )
			.append( QString( ", gain: %1" ).arg( __gain ) )
			.append( QString( ", m_nMaxLayers: %1" ).arg( m_nMaxLayers ) )
			.append( QString( ", [layers:" ) );
	
		for ( auto ll : __layers ) {
			if ( ll != nullptr ) {
				sOutput.append( QString( " [%1" ).arg( ll->toQString( sPrefix + s + s, bShort ).replace( "\n", "]" ) ) );
			}
		}

		sOutput.append( "]\n" );

	}
	
	return sOutput;
}

std::vector<std::shared_ptr<InstrumentLayer>>::iterator InstrumentComponent::begin() {
	return __layers.begin();
}

std::vector<std::shared_ptr<InstrumentLayer>>::iterator InstrumentComponent::end() {
	return __layers.end();
}

};

/* vim: set softtabstop=4 noexpandtab: */
