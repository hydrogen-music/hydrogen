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

#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/sample.h>

namespace H2Core
{

const char* InstrumentLayer::__class_name = "InstrumentLayer";

InstrumentLayer::InstrumentLayer( Sample *sample )
		: Object( __class_name )
		, __start_velocity( 0.0 )
		, __end_velocity( 1.0 )
		, __pitch( 0.0 )
		, __gain( 1.0 )
		, __sample( sample )
{
	//infoLog( "INIT" );
}



InstrumentLayer::~InstrumentLayer()
{
	delete __sample;
	__sample = NULL;
	//infoLog( "DESTROY" );
}

};


