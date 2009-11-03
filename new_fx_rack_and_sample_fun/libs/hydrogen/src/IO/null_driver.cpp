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

#include <hydrogen/IO/NullDriver.h>
#include <hydrogen/globals.h>


namespace H2Core
{

NullDriver::NullDriver( audioProcessCallback processCallback )
		: AudioOutput( "NullDriver" )
{
	UNUSED( processCallback );
//	INFOLOG( "INIT" );
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
	return NULL;
}


float* NullDriver::getOut_R()
{
	INFOLOG( "not implemented yet" );
	return NULL;
}


void NullDriver::play()
{
	INFOLOG( "not implemented" );
}

void NullDriver::stop()
{
	INFOLOG( "not implemented" );
}

void NullDriver::locate( unsigned long nFrame )
{
	UNUSED( nFrame );
	INFOLOG( "not implemented" );
}

void NullDriver::updateTransportInfo()
{
	INFOLOG( "not implemented" );
}

void NullDriver::setBpm( float fBPM )
{
	UNUSED( fBPM );
	ERRORLOG( "not implemented yet" );
}

};
