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

#include <core/Basics/InstrumentComponent.h>

#include <cassert>

#include <core/AudioEngine.h>

#include <core/Helpers/Xml.h>
#include <core/Helpers/Filesystem.h>

#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>

namespace H2Core
{

const char* InstrumentComponent::__class_name = "InstrumentComponent";

int InstrumentComponent::m_nMaxLayers;

InstrumentComponent::InstrumentComponent( int related_drumkit_componentID )
	: Object( __class_name )
	, __related_drumkit_componentID( related_drumkit_componentID )
	, __gain( 1.0 )
{
	__layers.resize( m_nMaxLayers );
	for ( int i = 0; i < m_nMaxLayers; i++ ) {
		__layers[i] = nullptr;
	}
}

InstrumentComponent::InstrumentComponent( InstrumentComponent* other )
	: Object( __class_name )
	, __related_drumkit_componentID( other->__related_drumkit_componentID )
	, __gain( other->__gain )
{
	__layers.resize( m_nMaxLayers );
	for ( int i = 0; i < m_nMaxLayers; i++ ) {
		InstrumentLayer* other_layer = other->get_layer( i );
		if ( other_layer ) {
			__layers[i] = new InstrumentLayer( other_layer );
		} else {
			__layers[i] = nullptr;
		}
	}
}

InstrumentComponent::~InstrumentComponent()
{
	for ( int i = 0; i < m_nMaxLayers; i++ ) {
		delete __layers[i];
		__layers[i] = nullptr;
	}
}

void InstrumentComponent::set_layer( InstrumentLayer* layer, int idx )
{
	assert( idx >= 0 && idx < m_nMaxLayers );
	if ( __layers[ idx ] ) {
		delete __layers[ idx ];
	}
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

InstrumentComponent* InstrumentComponent::load_from( XMLNode* node, const QString& dk_path )
{
	int id = node->read_int( "component_id", EMPTY_INSTR_ID, false, false );
	if ( id==EMPTY_INSTR_ID ) {
		return nullptr;
	}

	InstrumentComponent* pInstrumentComponent = new InstrumentComponent( id );
	pInstrumentComponent->set_gain( node->read_float( "gain", 1.0f, true, false ) );
	XMLNode layer_node = node->firstChildElement( "layer" );
	int n = 0;
	while ( !layer_node.isNull() ) {
		if ( n >= m_nMaxLayers ) {
			ERRORLOG( QString( "n (%1) >= m_nMaxLayers (%2)" ).arg( n ).arg( m_nMaxLayers ) );
			break;
		}
		pInstrumentComponent->set_layer( InstrumentLayer::load_from( &layer_node, dk_path ), n );
		n++;
		layer_node = layer_node.nextSiblingElement( "layer" );
	}
	return pInstrumentComponent;
}

void InstrumentComponent::save_to( XMLNode* node, int component_id )
{
	XMLNode component_node;
	if( component_id == -1 ) {
		component_node = node->createNode( "instrumentComponent" );
		component_node.write_int( "component_id", __related_drumkit_componentID );
		component_node.write_float( "gain", __gain );
	}
	for ( int n = 0; n < m_nMaxLayers; n++ ) {
		InstrumentLayer* pLayer = get_layer( n );
		if( pLayer ) {
			if( component_id == -1 ) {
				pLayer->save_to( &component_node );
			} else {
				pLayer->save_to( node );
			}
		}
	}
}

};

/* vim: set softtabstop=4 noexpandtab: */
