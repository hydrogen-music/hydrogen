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

#include <hydrogen/basics/drumkit_component.h>

#include <cassert>

#include <hydrogen/audio_engine.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>

#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>

namespace H2Core
{

const char* DrumkitComponent::m_sClassName = "DrumkitComponent";

DrumkitComponent::DrumkitComponent( const int id, const QString& name )
	: Object( m_sClassName )
	, __id( id )
	, __name( name )
	, __volume( 1.0 )
	, __muted( false )
	, __soloed( false )
	, __out_L( nullptr )
	, __out_R( nullptr )
	, __peak_l( 0.0 )
	, __peak_r( 0.0 )
{
	__out_L = new float[ MAX_BUFFER_SIZE ];
	__out_R = new float[ MAX_BUFFER_SIZE ];
}

DrumkitComponent::DrumkitComponent( DrumkitComponent* other )
	: Object( m_sClassName )
	, __id( other->get_id() )
	, __name( other->get_name() )
	, __volume( other->__volume )
	, __muted( other->__muted )
	, __soloed( other->__soloed )
	, __out_L( nullptr )
	, __out_R( nullptr )
	, __peak_l( 0.0 )
	, __peak_r( 0.0 )
{
	__out_L = new float[ MAX_BUFFER_SIZE ];
	__out_R = new float[ MAX_BUFFER_SIZE ];
}

DrumkitComponent::~DrumkitComponent()
{
	delete[] __out_L;
	delete[] __out_R;
}

void DrumkitComponent::reset_outs( uint32_t nFrames )
{
	memset( __out_L, 0, nFrames * sizeof( float ) );
	memset( __out_R, 0, nFrames * sizeof( float ) );
}

void DrumkitComponent::set_outs( int nBufferPos, float valL, float valR )
{
	__out_L[nBufferPos] += valL;
	__out_R[nBufferPos] += valR;
}

float DrumkitComponent::get_out_L( int nBufferPos )
{
	return __out_L[nBufferPos];
}

float DrumkitComponent::get_out_R( int nBufferPos )
{
	return __out_R[nBufferPos];
}

void DrumkitComponent::load_from( DrumkitComponent* component, bool is_live )
{
	if ( is_live ) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
	}

	this->set_id( component->get_id() );
	this->set_name( component->get_name() );
	this->set_muted( component->is_muted() );
	this->set_volume( component->get_volume() );

	if ( is_live ) {
		AudioEngine::get_instance()->unlock();
	}
}

DrumkitComponent* DrumkitComponent::load_from( XMLNode* node, const QString& dk_path )
{
	int id = node->read_int( "id", EMPTY_INSTR_ID, false, false );
	if ( id==EMPTY_INSTR_ID ) {
		return nullptr;
	}

	DrumkitComponent* pDrumkitComponent = new DrumkitComponent( id, node->read_string( "name", "" ) );
	pDrumkitComponent->set_volume( node->read_float( "volume", 1.0f, true, false ) );

	return pDrumkitComponent;
}

void DrumkitComponent::save_to( XMLNode* node )
{
	XMLNode ComponentNode = node->createNode( "drumkitComponent" );
	ComponentNode.write_int( "id", __id );
	ComponentNode.write_string( "name", __name );
	ComponentNode.write_float( "volume", __volume );
}

};
