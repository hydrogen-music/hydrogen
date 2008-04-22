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
#include <hydrogen/IO/TransportInfo.h>
#include <cstdio>

namespace H2Core
{

TransportInfo::TransportInfo()
		: Object( "TransportInfo" )
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
	switch ( m_status ) {
	case STOPPED:
		INFOLOG( "status = STOPPED" );
		break;

	case ROLLING:
		INFOLOG( "status = ROLLING" );
		break;

	case BAD:
		INFOLOG( "status = BAD" );
		break;

	default:
		ERRORLOG( "status = unknown" );
	}
	INFOLOG( QString( "frames = %1" ).arg( m_nFrames ) );
	INFOLOG( QString( "tickSize = %1" ).arg( m_nTickSize ) );
}

};
