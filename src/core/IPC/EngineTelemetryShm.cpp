/*
 * Hydrogen
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

#include <core/IPC/EngineTelemetryShm.h>

namespace H2Core {

EngineTelemetryShm::EngineTelemetryShm()
	: m_bOwner( false ) {
}

EngineTelemetryShm::~EngineTelemetryShm() {
	detach();
}

bool EngineTelemetryShm::create( const QString& sKey ) {
	detach();
	m_shm.setKey( sKey );
	// A stale segment from a crashed run would make create() fail with
	// AlreadyExists; attaching-then-detaching releases it on Unix.
	if ( m_shm.attach() ) {
		m_shm.detach();
	}
	if ( ! m_shm.create( sizeof( EngineTelemetry ) ) ) {
		___ERRORLOG( QString( "Unable to create telemetry shm [%1]: %2" )
					 .arg( sKey ).arg( m_shm.errorString() ) );
		return false;
	}
	m_bOwner = true;
	telemetryInit( *static_cast<EngineTelemetry*>( m_shm.data() ) );
	return true;
}

bool EngineTelemetryShm::attach( const QString& sKey ) {
	detach();
	m_shm.setKey( sKey );
	if ( ! m_shm.attach( QSharedMemory::ReadOnly ) ) {
		___ERRORLOG( QString( "Unable to attach telemetry shm [%1]: %2" )
					 .arg( sKey ).arg( m_shm.errorString() ) );
		return false;
	}
	m_bOwner = false;
	return true;
}

void EngineTelemetryShm::detach() {
	if ( m_shm.isAttached() ) {
		m_shm.detach();
	}
	m_bOwner = false;
}

bool EngineTelemetryShm::isValid() const {
	return m_shm.isAttached();
}

bool EngineTelemetryShm::store( const EngineTelemetrySnapshot& snapshot ) {
	if ( ! m_shm.isAttached() || m_shm.data() == nullptr ) {
		return false;
	}
	telemetryStore( *static_cast<EngineTelemetry*>( m_shm.data() ), snapshot );
	return true;
}

bool EngineTelemetryShm::load( EngineTelemetrySnapshot& out ) const {
	if ( ! m_shm.isAttached() || m_shm.constData() == nullptr ) {
		return false;
	}
	return telemetryLoad(
		*static_cast<const EngineTelemetry*>( m_shm.constData() ), out );
}

};
