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

#include <core/IO/PluginAudioDriver.h>

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <algorithm>
#include <cstring>

namespace H2Core {

PluginAudioDriver::PluginAudioDriver( Hydrogen* pHydrogen,
									  audioProcessCallback processCallback )
		: AudioDriver( pHydrogen )
		, m_processCallback( processCallback )
		, m_nBufferSize( 0 )
		, m_nSampleRate( 0 )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr ) {
}

PluginAudioDriver::~PluginAudioDriver() {
}

int PluginAudioDriver::init( unsigned nBufferSize ) {
	// The host dictates buffers and rate; we just seed plausible values from
	// the preferences so the engine has a sane sample rate / block size until
	// the host hands us its first block via setHostBuffers()/setSampleRate().
	m_nBufferSize = nBufferSize;
	m_nSampleRate = m_pHydrogen->getPreferences()->m_nSampleRate;

	INFOLOG( QString( "nBufferSize: [%1], nSampleRate: [%2]" )
			 .arg( m_nBufferSize ).arg( m_nSampleRate ) );

	return 0;
}

int PluginAudioDriver::connect() {
	// Nothing to open: the host is always connected.
	return 0;
}

void PluginAudioDriver::disconnect() {
	// Nothing to close; the host owns the buffers.
	m_pOut_L = nullptr;
	m_pOut_R = nullptr;
}

unsigned PluginAudioDriver::getBufferSize() {
	return m_nBufferSize;
}

unsigned PluginAudioDriver::getSampleRate() {
	return m_nSampleRate;
}

float* PluginAudioDriver::getOut_L() {
	return m_pOut_L;
}

float* PluginAudioDriver::getOut_R() {
	return m_pOut_R;
}

void PluginAudioDriver::setHostBuffers( float* pOut_L, float* pOut_R,
										unsigned nFrames ) {
	m_pOut_L = pOut_L;
	m_pOut_R = pOut_R;
	m_nBufferSize = nFrames;
}

void PluginAudioDriver::setSampleRate( unsigned nSampleRate ) {
	m_nSampleRate = nSampleRate;
}

void PluginAudioDriver::setHostTransport( bool bRolling, double fBpm,
										  long long nFrame ) {
	m_hostTransport.bRolling = bRolling;
	m_hostTransport.fBpm = fBpm;
	m_hostTransport.nFrame = nFrame;
	m_hostTransport.bValid = true;
}

const PluginAudioDriver::HostTransport&
PluginAudioDriver::getHostTransport() const {
	return m_hostTransport;
}

void PluginAudioDriver::setBusBuffers( const std::vector<float*>& busOut_L,
									   const std::vector<float*>& busOut_R ) {
	m_busOut_L = busOut_L;
	m_busOut_R = busOut_R;
}

int PluginAudioDriver::getBusCount() const {
	return static_cast<int>(
		std::min( m_busOut_L.size(), m_busOut_R.size() ) );
}

float* PluginAudioDriver::getBusBuffer_L( int nBus ) const {
	if ( nBus < 0 || nBus >= static_cast<int>( m_busOut_L.size() ) ) {
		return nullptr;
	}
	return m_busOut_L[ nBus ];
}

float* PluginAudioDriver::getBusBuffer_R( int nBus ) const {
	if ( nBus < 0 || nBus >= static_cast<int>( m_busOut_R.size() ) ) {
		return nullptr;
	}
	return m_busOut_R[ nBus ];
}

void PluginAudioDriver::clearBusBuffers( unsigned nFrames ) {
	for ( auto pBus : m_busOut_L ) {
		if ( pBus != nullptr ) {
			memset( pBus, 0, nFrames * sizeof( float ) );
		}
	}
	for ( auto pBus : m_busOut_R ) {
		if ( pBus != nullptr ) {
			memset( pBus, 0, nFrames * sizeof( float ) );
		}
	}
}

QString PluginAudioDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[PluginAudioDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nBufferSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBufferSize ) )
			.append( QString( "%1%2m_nSampleRate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleRate ) );
	} else {
		sOutput = QString( "[PluginAudioDriver]" )
			.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
			.append( QString( ", m_nSampleRate: %1" ).arg( m_nSampleRate ) );
	}

	return sOutput;
}

};
