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
	// Clear any stale socket file left by a crashed previous run.
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
