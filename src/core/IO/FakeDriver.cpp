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

#include <core/IO/FakeDriver.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

namespace H2Core
{

FakeDriver::FakeDriver( audioProcessCallback processCallback )
		: AudioOutput()
		, m_processCallback( processCallback )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_nBufferSize( 0 )
		, m_nSampleRate( 44100 ) {
}


FakeDriver::~FakeDriver() {
}


int FakeDriver::init( unsigned nBufferSize )
{
	INFOLOG( QString( "Init, %1 samples" ).arg( nBufferSize ) );

	m_nBufferSize = nBufferSize;
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
	m_pOut_L = new float[nBufferSize];
	m_pOut_R = new float[nBufferSize];

	return 0;
}


int FakeDriver::connect()
{
	INFOLOG( "connect" );

	// always rolling, no user interaction
	Hydrogen::get_instance()->getAudioEngine()->setNextState( AudioEngine::State::Playing );

	return 0;
}


void FakeDriver::disconnect()
{
	INFOLOG( "disconnect" );

	delete[] m_pOut_L;
	m_pOut_L = nullptr;

	delete[] m_pOut_R;
	m_pOut_R = nullptr;
}


unsigned FakeDriver::getSampleRate()
{
	return m_nSampleRate;
}

float* FakeDriver::getOut_L()
{
	return m_pOut_L;
}


float* FakeDriver::getOut_R()
{
	return m_pOut_R;
}


void FakeDriver::processCallback()
{
	while ( m_processCallback( m_nBufferSize, nullptr ) == 0 ) {
		// process...
	}
}

QString FakeDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[FakeDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nBufferSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBufferSize ) )
			.append( QString( "%1%2m_nSampleRate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleRate ) );
	} else {
		sOutput = QString( "[FakeDriver]" )
			.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
			.append( QString( ", m_nSampleRate: %1" ).arg( m_nSampleRate ) );
	}

	return sOutput;
}

};
