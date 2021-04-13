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

#include <core/Basics/DrumkitComponent.h>

#include <cassert>

#include <core/AudioEngine.h>

#include <core/Helpers/Xml.h>
#include <core/Helpers/Filesystem.h>

#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>

namespace H2Core
{

const char* DrumkitComponent::__class_name = "DrumkitComponent";

DrumkitComponent::DrumkitComponent( const int id, const QString& name )
	: Object( __class_name )
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
	: Object( __class_name )
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

QString DrumkitComponent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Object::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[DrumkitComponent]\n" ).arg( sPrefix )
			.append( QString( "%1%2id: %3\n" ).arg( sPrefix ).arg( s ).arg( __id ) )
			.append( QString( "%1%2name: %3\n" ).arg( sPrefix ).arg( s ).arg( __name ) )
			.append( QString( "%1%2volume: %3\n" ).arg( sPrefix ).arg( s ).arg( __volume ) )
			.append( QString( "%1%2muted: %3\n" ).arg( sPrefix ).arg( s ).arg( __muted ) )
			.append( QString( "%1%2soloed: %3\n" ).arg( sPrefix ).arg( s ).arg( __soloed ) )
			.append( QString( "%1%2peak_l: %3\n" ).arg( sPrefix ).arg( s ).arg( __peak_l ) )
			.append( QString( "%1%2peak_r: %3\n" ).arg( sPrefix ).arg( s ).arg( __peak_r ) );
	} else {

		sOutput = QString( "[DrumkitComponent]" )
			.append( QString( " id: %1" ).arg( __id ) )
			.append( QString( ", name: %1" ).arg( __name ) )
			.append( QString( ", volume: %1" ).arg( __volume ) )
			.append( QString( ", muted: %1" ).arg( __muted ) )
			.append( QString( ", soloed: %1" ).arg( __soloed ) )
			.append( QString( ", peak_l: %1" ).arg( __peak_l ) )
			.append( QString( ", peak_r: %1" ).arg( __peak_r ) );
	}
	return sOutput;
}

};
