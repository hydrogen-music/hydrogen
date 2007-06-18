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
 * $Id: TransportInfo.cpp,v 1.5 2005/05/01 19:51:40 comix Exp $
 *
 */
#include "TransportInfo.h"
#include <stdio.h>

TransportInfo::TransportInfo() : Object( "TransportInfo" )
{
//	infoLog( "INIT" );
	m_status = STOPPED;
	m_nFrames = 0;
	m_nTickSize = 0;
	m_nBPM = 120;
}


TransportInfo::~TransportInfo()
{
//	infoLog( "DESTROY" );
}


void TransportInfo::printInfo()
{
	switch (m_status) {
		case STOPPED:
			infoLog( "[transport] status = STOPPED" );
			break;

		case ROLLING:
			infoLog( "[transport] status = ROLLING" );
			break;

		case BAD:
			errorLog( "[transport] status = BAD" );
			break;

		default:
			errorLog( "[transport] status = unknown" );
	}
	infoLog( "[transport] frames = " + toString(m_nFrames) );
	infoLog( "[transport] tickSize = " + toString(m_nTickSize) );
}

