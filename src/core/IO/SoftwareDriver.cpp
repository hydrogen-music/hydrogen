/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/IO/SoftwareDriver.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {

SoftwareDriver::SoftwareDriver( Hydrogen* pHydrogen,
								audioProcessCallback processCallback,
								bool bProducesAudio )
		: AudioDriver( pHydrogen )
		, m_processCallback( processCallback )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_bActive( false )
		, m_processInterval( 10 )
		, m_nBufferSize( 0 )
		, m_nSampleRate( 44100 )
		, m_bProducesAudio( bProducesAudio ) {
}

SoftwareDriver::~SoftwareDriver() {
	// Ensure the clock thread is stopped and buffers released even if
	// disconnect() was not called (defensive — a live thread must never outlive
	// the driver, see ADR 0031 / proposal 0004 §10 use-after-free lesson).
	disconnect();
}

int SoftwareDriver::init( unsigned nBufferSize ) {

	m_nBufferSize = nBufferSize;
	m_nSampleRate = m_pHydrogen->getPreferences()->m_nSampleRate;
	m_pOut_L = new float[ nBufferSize ];
	m_pOut_R = new float[ nBufferSize ];

	m_processInterval = std::chrono::duration<float>(
		static_cast<float>(m_nBufferSize) /
		static_cast<float>(m_nSampleRate) );

	INFOLOG( QString( "nBufferSize: [%1], nSampleRate: [%2], m_processInterval: [%3], producesAudio: [%4]" )
			 .arg( m_nBufferSize ).arg( m_nSampleRate )
			 .arg( m_processInterval.count() ).arg( m_bProducesAudio ) );

	return 0;
}

int SoftwareDriver::connect() {
	if ( m_pCallbackHandler != nullptr ) {
		m_bActive = false;
		m_pCallbackHandler->join();
		m_pCallbackHandler = nullptr;
	}

	m_bActive = true;
	m_pCallbackHandler = std::make_shared< std::thread >(
		SoftwareDriver::processCallback, ( void* )this );

	return 0;
}

void SoftwareDriver::disconnect() {
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

unsigned SoftwareDriver::getSampleRate() {
	return m_nSampleRate;
}

float* SoftwareDriver::getOut_L() {
	return m_pOut_L;
}

float* SoftwareDriver::getOut_R() {
	return m_pOut_R;
}

void SoftwareDriver::deactivate() {
	m_bActive = false;

	if ( m_pCallbackHandler != nullptr ) {
		m_pCallbackHandler->join();
		m_pCallbackHandler = nullptr;
	}
}

QString SoftwareDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SoftwareDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bProducesAudio: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bProducesAudio ) )
			.append( QString( "%1%2m_processInterval: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_processInterval.count() ) )
			.append( QString( "%1%2m_lastRun: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( H2Core::timePointToQString( m_lastRun ) ) )
			.append( QString( "%1%2m_nBufferSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBufferSize ) )
			.append( QString( "%1%2m_nSampleRate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleRate ) );
	} else {
		sOutput = QString( "[SoftwareDriver]" )
			.append( QString( ", m_bProducesAudio: %1" ).arg( m_bProducesAudio ) )
			.append( QString( ", m_processInterval: %1" )
					 .arg( m_processInterval.count() ) )
			.append( QString( ", m_lastRun: %1" )
					 .arg( H2Core::timePointToQString( m_lastRun ) ) )
			.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
			.append( QString( ", m_nSampleRate: %1" ).arg( m_nSampleRate ) );
	}

	return sOutput;
}

void SoftwareDriver::processCallback( void* pInstance ) {
	auto pDriver = static_cast<SoftwareDriver*>( pInstance );
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
				// Clock the engine at the configured rate.
				std::this_thread::sleep_for(
					pDriver->m_processInterval - ( start - pDriver->m_lastRun ) );
			}
		}

		if ( pDriver->m_processCallback(
				 pDriver->getBufferSize(), pDriver->getHydrogen() ) != 0 ) {
			pDriver->m_bActive = false;
			return;
		}

		pDriver->m_lastRun = Clock::now();
	}
}
};
