/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: NullDriver.cpp,v 1.7 2005/05/01 19:51:40 comix Exp $
 *
 */

#include "NullDriver.h"

NullDriver::NullDriver( audioProcessCallback processCallback )
 : GenericDriver( "NullDriver" )
{
	infoLog( "INIT" );
}


NullDriver::~NullDriver() {
	infoLog( "DESTROY" );
}


int NullDriver::init(unsigned nBufferSize)
{
	return 0;
}


int NullDriver::connect() {
	infoLog( "connect" );
	return 0;
}


void NullDriver::disconnect() {
	infoLog( "disconnect" );
}


unsigned NullDriver::getBufferSize() {
//	infoLog( "[getBufferSize()] not implemented yet");
	return 0;
}


unsigned NullDriver::getSampleRate() {
//	infoLog("[getSampleRate()] not implemented yet");
	return 0;
}

float* NullDriver::getOut_L()
{
	infoLog("[getOut_L()] not implemented yet");
	return NULL;
}


float* NullDriver::getOut_R()
{
	infoLog("[getOut_R()] not implemented yet");
	return NULL;
}


void NullDriver::play()
{
	infoLog( "[play] not implemented" );
}

void NullDriver::stop()
{
	infoLog( "[stop] not implemented" );
}

void NullDriver::locate( unsigned long nFrame )
{
	infoLog( "[locate] not implemented" );
}

void NullDriver::updateTransportInfo()
{
	infoLog( "[updateTransportInfo] not implemented" );
}

void NullDriver::setBpm(float fBPM)
{
	errorLog( "[setBpm] not implemented yet" );
}

