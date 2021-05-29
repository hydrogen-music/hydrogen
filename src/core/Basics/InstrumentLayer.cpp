/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Basics/InstrumentLayer.h>

#include <core/Helpers/Xml.h>
#include <core/Basics/Sample.h>

namespace H2Core
{

const char* InstrumentLayer::__class_name = "InstrumentLayer";

InstrumentLayer::InstrumentLayer( std::shared_ptr<Sample> sample ) : Object( __class_name ),
	__start_velocity( 0.0 ),
	__end_velocity( 1.0 ),
	__pitch( 0.0 ),
	__gain( 1.0 ),
	__sample( sample )
{
}

InstrumentLayer::InstrumentLayer( InstrumentLayer* other ) : Object( __class_name ),
	__start_velocity( other->get_start_velocity() ),
	__end_velocity( other->get_end_velocity() ),
	__pitch( other->get_pitch() ),
	__gain( other->get_gain() ),
	__sample( other->get_sample() )
{
}

InstrumentLayer::InstrumentLayer( InstrumentLayer* other, std::shared_ptr<Sample> sample ) : Object( __class_name ),
	__start_velocity( other->get_start_velocity() ),
	__end_velocity( other->get_end_velocity() ),
	__pitch( other->get_pitch() ),
	__gain( other->get_gain() ),
	__sample( sample )
{
}

InstrumentLayer::~InstrumentLayer()
{
}

void InstrumentLayer::set_sample( std::shared_ptr<Sample> sample )
{
	__sample = sample;
}

void InstrumentLayer::load_sample()
{
	if( __sample ) {
		__sample->load();
	}
}

void InstrumentLayer::unload_sample()
{
	if( __sample ) {
		__sample->unload();
	}
}

InstrumentLayer* InstrumentLayer::load_from( XMLNode* node, const QString& dk_path )
{
	auto pSample = std::make_shared<Sample>( dk_path+"/"+node->read_string( "filename", "" ) );
	InstrumentLayer* layer = new InstrumentLayer( pSample );
	layer->set_start_velocity( node->read_float( "min", 0.0 ) );
	layer->set_end_velocity( node->read_float( "max", 1.0 ) );
	layer->set_gain( node->read_float( "gain", 1.0, true, false ) );
	layer->set_pitch( node->read_float( "pitch", 0.0, true, false ) );
	return layer;
}

void InstrumentLayer::save_to( XMLNode* node )
{
	XMLNode layer_node = node->createNode( "layer" );
	layer_node.write_string( "filename", get_sample()->get_filename() );
	layer_node.write_float( "min", __start_velocity );
	layer_node.write_float( "max", __end_velocity );
	layer_node.write_float( "gain", __gain );
	layer_node.write_float( "pitch", __pitch );
}

QString InstrumentLayer::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Object::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentLayer]\n" ).arg( sPrefix )
			.append( QString( "%1%2gain: %3\n" ).arg( sPrefix ).arg( s ).arg( __gain ) )
			.append( QString( "%1%2pitch: %3\n" ).arg( sPrefix ).arg( s ).arg( __pitch ) )
			.append( QString( "%1%2start_velocity: %3\n" ).arg( sPrefix ).arg( s ).arg( __start_velocity ) )
			.append( QString( "%1%2end_velocity: %3\n" ).arg( sPrefix ).arg( s ).arg( __end_velocity ) )
			.append( QString( "%1" ).arg( __sample->toQString( sPrefix + s, bShort ) ) );
	} else {
		sOutput = QString( "[InstrumentLayer]" )
			.append( QString( " gain: %1" ).arg( __gain ) )
			.append( QString( ", pitch: %1" ).arg( __pitch ) )
			.append( QString( ", start_velocity: %1" ).arg( __start_velocity ) )
			.append( QString( ", end_velocity: %1" ).arg( __end_velocity ) )
			.append( QString( ", sample: %1\n" ).arg( __sample->get_filepath() ) );
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
