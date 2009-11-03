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

#include "FakeDriver.h"

namespace H2Core
{

FakeDriver::FakeDriver( audioProcessCallback processCallback )
		: AudioOutput( "FakeDriver" )
		, m_processCallback( processCallback )
		, m_pOut_L( NULL )
		, m_pOut_R( NULL )
{
	INFOLOG( "INIT" );
}


FakeDriver::~FakeDriver()
{
	INFOLOG( "DESTROY" );
}


int FakeDriver::init( unsigned nBufferSize )
{
	INFOLOG( QString( "Init, %1 samples" ).arg( nBufferSize ) );

	m_nBufferSize = nBufferSize;
	m_pOut_L = new float[nBufferSize];
	m_pOut_R = new float[nBufferSize];

	return 0;
}


int FakeDriver::connect()
{
	INFOLOG( "connect" );

	// 	// always rolling, no user interaction
	m_transport.m_status = TransportInfo::ROLLING;

	return 0;
}


void FakeDriver::disconnect()
{
	INFOLOG( "disconnect" );

	delete[] m_pOut_L;
	m_pOut_L = NULL;

	delete[] m_pOut_R;
	m_pOut_R = NULL;
}


unsigned FakeDriver::getSampleRate()
{
	return 44100;
}

float* FakeDriver::getOut_L()
{
	return m_pOut_L;
}


float* FakeDriver::getOut_R()
{
	return m_pOut_R;
}


void FakeDriver::play()
{
	m_transport.m_status = TransportInfo::ROLLING;

	while ( m_processCallback( m_nBufferSize, NULL ) == 0 ) {
		// process...
	}
}

void FakeDriver::stop()
{
	m_transport.m_status = TransportInfo::STOPPED;
}

void FakeDriver::locate( unsigned long nFrame )
{
	m_transport.m_nFrames = nFrame;
}

void FakeDriver::updateTransportInfo()
{
	// not used
}

void FakeDriver::setBpm( float fBPM )
{
	m_transport.m_nBPM = fBPM;
}

};
