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

#include <QtCore/QElapsedTimer>
#include <QtGlobal>
#include <QtNetwork/QLocalSocket>

#if defined( Q_OS_UNIX )
#include <sys/socket.h>
#endif

namespace H2Core {

namespace {
// Enlarge the OS socket buffers so a whole frame (object payloads — Song /
// Drumkit XML — can be tens of KB) can be handed off in one flush even when the
// peer is not draining concurrently. Without this, a synchronous send()-then-
// receive() on a single thread (as the tests do) deadlocks once the frame
// exceeds the buffer: the write can't complete until the peer reads, but the peer
// only reads after the write. Linux's large default Unix-socket buffers hid this;
// macOS defaults are ~8 KB, smaller than even an empty song's XML. On Windows a
// QLocalSocket is a named pipe (no SO_*BUF); its buffering does not have the same
// limit, so this is a no-op there.
void enlargeSocketBuffers( QLocalSocket* pSocket ) {
#if defined( Q_OS_UNIX )
	const qintptr fd = pSocket->socketDescriptor();
	if ( fd < 0 ) {
		return;
	}
	const int nSize = 8 * 1024 * 1024; // OS clamps to its max if smaller
	setsockopt( static_cast<int>( fd ), SOL_SOCKET, SO_SNDBUF,
				&nSize, sizeof( nSize ) );
	setsockopt( static_cast<int>( fd ), SOL_SOCKET, SO_RCVBUF,
				&nSize, sizeof( nSize ) );
#else
	(void)pSocket;
#endif
}
} // namespace

IpcChannel::IpcChannel( QLocalSocket* pSocket, QObject* pParent )
	: QObject( pParent )
	, m_pSocket( pSocket ) {
	if ( m_pSocket != nullptr ) {
		m_pSocket->setParent( this );
		enlargeSocketBuffers( m_pSocket );
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

bool IpcChannel::request( const IpcMessage& req, IpcMessage& reply,
						  int nTimeoutMs ) {
	if ( m_pSocket == nullptr ) {
		return false;
	}

	quint32 nId = m_nNextRequestId++;
	if ( nId == 0 ) {
		nId = m_nNextRequestId++; // 0 is reserved for "no correlation"
	}
	IpcMessage r = req;
	r.setRequestId( nId );
	if ( ! send( r ) ) {
		return false;
	}

	QElapsedTimer timer;
	timer.start();
	while ( true ) {
		// Pull our reply out of the pending queue, preserving the order of any
		// other (event/command) frames so receive()/messageReceived() still get
		// them.
		std::queue<IpcMessage> rest;
		bool bFound = false;
		while ( ! m_pending.empty() ) {
			IpcMessage m = m_pending.front();
			m_pending.pop();
			if ( ! bFound && m.getRequestId() == nId ) {
				reply = m;
				bFound = true;
			}
			else {
				rest.push( m );
			}
		}
		m_pending = std::move( rest );
		if ( bFound ) {
			return true;
		}

		const int nRemaining =
			nTimeoutMs - static_cast<int>( timer.elapsed() );
		if ( nRemaining <= 0 ) {
			return false;
		}
		if ( ! m_pSocket->waitForReadyRead( nRemaining ) ) {
			pump(); // last chance: bytes may already be buffered
			if ( m_pending.empty() ) {
				return false;
			}
		}
		else {
			pump();
		}
	}
}

};
