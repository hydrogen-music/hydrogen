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

#include <core/IPC/IpcServer.h>

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

namespace H2Core {

namespace {

// Whether anything currently accepts connections on the endpoint.
//
// A failed connect alone does not prove the endpoint is dead: a live
// server whose accept backlog is momentarily full leaves the probe
// connecting without any error set (Unix EAGAIN) or reports a pipe error
// (Windows: all instances busy, access denied). Only a provably absent
// endpoint - no socket file (ServerNotFound) or a stale file nothing
// accepts on (ConnectionRefused) - may be treated as not live; anything
// else must count as live, or listen() below would unlink a busy
// endpoint's socket file.
bool isEndpointLive( const QString& sName ) {
	QLocalSocket probe;
	probe.connectToServer( sName );
	if ( probe.waitForConnected( 100 ) ) {
		return true;
	}

	switch ( probe.error() ) {
	case QLocalSocket::ServerNotFoundError:
	case QLocalSocket::ConnectionRefusedError:
		return false;
	default:
		return true;
	}
}

} // namespace

IpcServer::IpcServer( QObject* pParent )
	: QObject( pParent )
	, m_pServer( new QLocalServer( this ) ) {
	// Notify (do not auto-accept) so this never competes with waitForChannel().
	connect( m_pServer, &QLocalServer::newConnection,
			 this, &IpcServer::connectionPending );

	IPCLOG( "" );
}

IpcServer::~IpcServer() = default;

bool IpcServer::listen( const QString& sName ) {
	// Never bind over a live endpoint. On Linux QLocalServer::listen()
	// fails with AddressInUse on its own, but Windows named pipes are
	// multi-instance: a second listen() on a live name silently succeeds
	// and both servers would race for the same endpoint. Refusing
	// explicitly keeps the no-steal contract on every platform.
	if ( isEndpointLive( sName ) ) {
		IPCLOG( QString( "Endpoint [%1] is already live; not binding over it" )
				.arg( sName ) );
		return false;
	}

	// Clear any socket file left behind by a crashed previous run: Qt
	// reports AddressInUse for any existing file and never clears it
	// itself. (On Windows removeServer() is a no-op.)
	QLocalServer::removeServer( sName );

	IPCLOG( QString( "Listening to [%1]" ).arg( sName ) );

	return m_pServer->listen( sName );
}

QString IpcServer::serverName() const {
	return m_pServer->serverName();
}

IpcChannel* IpcServer::nextPendingChannel() {
	QLocalSocket* pSocket = m_pServer->nextPendingConnection();
	if ( pSocket == nullptr ) {
		return nullptr;
	}
	// Server side: this accepted channel's sends (initial state, replies,
	// forwarded events) run on the engine's event-loop-less serve thread, so they
	// must be pushed to completion. See IpcChannel's constructor.
	return new IpcChannel( pSocket, this, /*bPushWrites=*/true );
}

IpcChannel* IpcServer::waitForChannel( int nTimeoutMs ) {
	if ( ! m_pServer->hasPendingConnections() ) {
		if ( ! m_pServer->waitForNewConnection( nTimeoutMs ) ) {
			return nullptr;
		}
	}
	return nextPendingChannel();
}

};
