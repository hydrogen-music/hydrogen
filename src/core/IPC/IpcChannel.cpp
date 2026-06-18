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

#include <core/IPC/IpcChannel.h>

#include <QtNetwork/QLocalSocket>

namespace H2Core {

IpcChannel::IpcChannel( QLocalSocket* pSocket, QObject* pParent )
	: QObject( pParent )
	, m_pSocket( pSocket ) {
	if ( m_pSocket != nullptr ) {
		m_pSocket->setParent( this );
		connect( m_pSocket, &QLocalSocket::readyRead,
				 this, &IpcChannel::onReadyRead );
		connect( m_pSocket, &QLocalSocket::disconnected,
				 this, &IpcChannel::disconnected );
	}
}

IpcChannel::~IpcChannel() = default;

IpcChannel* IpcChannel::connectToServer( const QString& sName, int nTimeoutMs,
										 QObject* pParent ) {
	auto* pSocket = new QLocalSocket();
	pSocket->connectToServer( sName );
	if ( ! pSocket->waitForConnected( nTimeoutMs ) ) {
		delete pSocket;
		return nullptr;
	}
	return new IpcChannel( pSocket, pParent );
}

bool IpcChannel::isConnected() const {
	return m_pSocket != nullptr &&
		m_pSocket->state() == QLocalSocket::ConnectedState;
}

bool IpcChannel::send( const IpcMessage& msg ) {
	if ( m_pSocket == nullptr ) {
		return false;
	}
	const QByteArray frame = msg.encode();
	const qint64 nWritten = m_pSocket->write( frame );
	m_pSocket->flush();
	return nWritten == static_cast<qint64>( frame.size() );
}

void IpcChannel::pump() {
	if ( m_pSocket == nullptr ) {
		return;
	}
	const QByteArray data = m_pSocket->readAll();
	if ( ! data.isEmpty() ) {
		m_reader.append( data );
	}
	IpcMessage msg;
	while ( m_reader.next( msg ) ) {
		m_pending.push( msg );
		emit messageReceived( msg );
	}
}

void IpcChannel::onReadyRead() {
	pump();
}

bool IpcChannel::receive( IpcMessage& out, int nTimeoutMs ) {
	if ( ! m_pending.empty() ) {
		out = m_pending.front();
		m_pending.pop();
		return true;
	}
	while ( m_pending.empty() ) {
		if ( m_pSocket == nullptr ) {
			return false;
		}
		// waitForReadyRead pumps the socket and (in Qt) emits readyRead, which
		// runs onReadyRead()/pump(); call pump() again to cover the case where
		// it does not.
		if ( ! m_pSocket->waitForReadyRead( nTimeoutMs ) ) {
			pump(); // last chance: bytes may have arrived with the connect
			if ( m_pending.empty() ) {
				return false;
			}
			break;
		}
		pump();
	}
	out = m_pending.front();
	m_pending.pop();
	return true;
}

};
