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

#include <core/IO/NullDriver.h>
#include <core/Globals.h>


namespace H2Core
{

NullDriver::NullDriver( audioProcessCallback processCallback )
		: AudioDriver()
{
	UNUSED( processCallback );
//	
}


NullDriver::~NullDriver()
{
//	INFOLOG( "DESTROY" );
}


int NullDriver::init( unsigned nBufferSize )
{
	UNUSED( nBufferSize );
	return 0;
}


int NullDriver::connect()
{
	INFOLOG( "connect" );
	return 0;
}


void NullDriver::disconnect()
{
	INFOLOG( "disconnect" );
}


unsigned NullDriver::getBufferSize()
{
//	infoLog( "[getBufferSize()] not implemented yet");
	return 0;
}


unsigned NullDriver::getSampleRate()
{
//	infoLog("[getSampleRate()] not implemented yet");
	return 0;
}

float* NullDriver::getOut_L()
{
	INFOLOG( "not implemented yet" );
	return nullptr;
}


float* NullDriver::getOut_R()
{
	INFOLOG( "not implemented yet" );
	return nullptr;
}

QString NullDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[NullDriver]" ).arg( sPrefix );
	} else {
		sOutput = "[NullDriver]";
	}

	return sOutput;
}
};
