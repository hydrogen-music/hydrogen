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

#include <hydrogen/basics/instrument_component.h>

#include <cassert>

#include <hydrogen/audio_engine.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>

#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>

namespace H2Core
{

const char* InstrumentComponent::__class_name = "InstrumentComponent";

int InstrumentComponent::maxLayers;

InstrumentComponent::InstrumentComponent( int related_drumkit_componentID )
	: Object( __class_name )
	, __related_drumkit_componentID( related_drumkit_componentID )
	, __gain( 1.0 )
{
	__layers.resize( maxLayers );
	for ( int i = 0; i < maxLayers; i++ ) {
		__layers[i] = NULL;
	}
}

InstrumentComponent::InstrumentComponent( InstrumentComponent* other )
	: Object( __class_name )
	, __related_drumkit_componentID( other->__related_drumkit_componentID )
	, __gain( other->__gain )
{
	__layers.resize( maxLayers );
	for ( int i = 0; i < maxLayers; i++ ) {
		InstrumentLayer* other_layer = other->get_layer( i );
		if ( other_layer ) {
			__layers[i] = new InstrumentLayer( other_layer, other_layer->get_sample());
		} else {
			__layers[i] = 0;
		}
	}
}

InstrumentComponent::~InstrumentComponent()
{
	for ( int i = 0; i < maxLayers; i++ ) {
		delete __layers[i];
		__layers[i] = 0;
	}
}

void InstrumentComponent::setMaxLayers( int layers )
{
	maxLayers = layers;
}

int InstrumentComponent::getMaxLayers()
{
	return maxLayers;
}

InstrumentComponent* InstrumentComponent::load_from( XMLNode* node, const QString& dk_path )
{
	int id = node->read_int( "component_id", EMPTY_INSTR_ID, false, false );
	if ( id==EMPTY_INSTR_ID ) return 0;

	InstrumentComponent* instrument_component = new InstrumentComponent( id );
	instrument_component->set_gain( node->read_float( "gain", 1.0f, true, false ) );
	XMLNode layer_node = node->firstChildElement( "layer" );
	int n = 0;
	while ( !layer_node.isNull() ) {
		if ( n >= maxLayers ) {
			ERRORLOG( QString( "n (%1) >= maxLayers (%2)" ).arg( n ).arg( maxLayers ) );
			break;
		}
		instrument_component->set_layer( InstrumentLayer::load_from( &layer_node, dk_path ), n );
		n++;
		layer_node = layer_node.nextSiblingElement( "layer" );
	}
	return instrument_component;
}

void InstrumentComponent::save_to( XMLNode* node, int component_id )
{
	XMLNode component_node;
	if( component_id == -1 ) {
		component_node = node->ownerDocument().createElement( "instrumentComponent" );
		component_node.write_int( "component_id", __related_drumkit_componentID );
		component_node.write_float( "gain", __gain );
	}
	for ( int n = 0; n < maxLayers; n++ ) {
		InstrumentLayer* layer = get_layer( n );
		if( layer ) {
			if( component_id == -1 )
				layer->save_to( &component_node );
			else
				layer->save_to( node );
		}
	}
	if( component_id == -1 )
		node->appendChild( component_node );
}

};

/* vim: set softtabstop=4 noexpandtab: */
