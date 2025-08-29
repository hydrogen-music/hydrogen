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

#include <core/IO/FakeAudioDriver.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

namespace H2Core {

FakeAudioDriver::FakeAudioDriver( audioProcessCallback processCallback )
		: AudioOutput()
		, m_processCallback( processCallback )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_bActive( false )
		, m_processInterval( 10 )
		, m_nBufferSize( 0 )
		, m_nSampleRate( 44100 ) {
}

FakeAudioDriver::~FakeAudioDriver() {
}

int FakeAudioDriver::init( unsigned nBufferSize ) {

	m_nBufferSize = nBufferSize;
	m_nSampleRate = Preferences::get_instance()->m_nSampleRate;
	m_pOut_L = new float[ nBufferSize ];
	m_pOut_R = new float[ nBufferSize ];

	m_processInterval = std::chrono::duration<float>(
		static_cast<float>(m_nBufferSize) /
		static_cast<float>(m_nSampleRate) );

	INFOLOG( QString( "nBufferSize: [%1], nSampleRate: [%2], m_processInterval: [%3]" )
			 .arg( m_nBufferSize ).arg( m_nSampleRate )
			 .arg( m_processInterval.count() ) );

	return 0;
}

int FakeAudioDriver::connect() {
	if ( m_pCallbackHandler != nullptr ) {
		m_bActive = false;
		m_pCallbackHandler->join();
		m_pCallbackHandler = nullptr;
	}

	m_bActive = true;
	m_pCallbackHandler = std::make_shared< std::thread >(
		FakeAudioDriver::processCallback, ( void* )this );

	return 0;
}

void FakeAudioDriver::disconnect() {
	m_bActive = false;

	if ( m_pCallbackHandler != nullptr ) {
		m_pCallbackHandler->join();
		m_pCallbackHandler = nullptr;
	}

	delete[] m_pOut_L;
	m_pOut_L = nullptr;

	delete[] m_pOut_R;
	m_pOut_R = nullptr;
}

unsigned FakeAudioDriver::getSampleRate() {
	return m_nSampleRate;
}

float* FakeAudioDriver::getOut_L() {
	return m_pOut_L;
}

float* FakeAudioDriver::getOut_R() {
	return m_pOut_R;
}

QString FakeAudioDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[FakeAudioDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_processInterval: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_processInterval.count() ) )
			.append( QString( "%1%2m_nBufferSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBufferSize ) )
			.append( QString( "%1%2m_nSampleRate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleRate ) );
	} else {
		sOutput = QString( "[FakeAudioDriver]" )
			.append( QString( ", m_processIntercal: %1" )
					 .arg( m_processInterval.count() ) )
			.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
			.append( QString( ", m_nSampleRate: %1" ).arg( m_nSampleRate ) );
	}

	return sOutput;
}

void FakeAudioDriver::processCallback( void* pInstance ) {
	auto pDriver = static_cast<FakeAudioDriver*>( pInstance );
	if ( pDriver == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	while ( pDriver->m_bActive ) {
		// process...
		auto start = Clock::now();

		if ( pDriver->m_lastRun != TimePoint() ) {
			if ( start - pDriver->m_lastRun >= pDriver->m_processInterval ) {
				WARNINGLOG( QString( "Audio could not be processed in time. Duration: [%1], Interval: [%2]" )
							.arg( ( pDriver->m_lastRun - start ).count() )
							.arg( pDriver->m_processInterval.count() ) );
			}
			else {
				// Simulate real-life audio driver.
				std::this_thread::sleep_for(
					pDriver->m_processInterval - ( start - pDriver->m_lastRun ) );
			}
		}

		if ( pDriver->m_processCallback(
				 pDriver->getBufferSize(), nullptr ) != 0 ) {
			pDriver->m_bActive = false;
			return;
		}

		pDriver->m_lastRun = Clock::now();
	}
}
};
