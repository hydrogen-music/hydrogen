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
#include <core/Object.h>

#include <QtCore/QElapsedTimer>
#include <QtGlobal>
#include <QtNetwork/QLocalSocket>

#if defined( Q_OS_UNIX )
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
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
#if defined( Q_OS_UNIX )
// Raise one SO_*BUF option as high as the kernel will accept.
//
// macOS/BSD does NOT silently clamp an over-large request: sbreserve() rejects
// anything above kern.ipc.maxsockbuf scaled by MCLBYTES/(MSIZE+MCLBYTES) (~7 MB
// for an 8 MB max), so setsockopt() fails outright with ENOBUFS and the buffer
// keeps its ~8 KB default. The previous code requested a flat 8 MB and ignored
// the return value, so on macOS it was a silent no-op — leaving the buffer far
// too small for a Song/Drumkit XML frame, which then deadlocks a single-threaded
// synchronous send()→receive() (the failing IPC tests). Walk a descending list
// and keep the first size the kernel accepts. Returns the size actually in
// effect (read back via getsockopt), or -1 if even the smallest request failed.
int raiseSocketBuffer( int fd, int optname, const char* optlabel ) {
	static const int candidates[] = {
		8 * 1024 * 1024, 4 * 1024 * 1024, 2 * 1024 * 1024,
		1 * 1024 * 1024, 512 * 1024,     256 * 1024, 128 * 1024 };

	int nApplied = -1;
	int nLastErrno = 0;
	for ( const int nSize : candidates ) {
		if ( setsockopt( fd, SOL_SOCKET, optname, &nSize,
						 sizeof( nSize ) ) == 0 ) {
			nApplied = nSize;
			break;
		}
		nLastErrno = errno;
	}

	int nReadback = -1;
	socklen_t len = sizeof( nReadback );
	getsockopt( fd, SOL_SOCKET, optname, &nReadback, &len );

	// One-time, per-connection diagnostic (control channel, not a hot path) so a
	// future macOS run shows the buffer size actually obtained rather than the
	// requested one — the key signal for the synchronous-transfer deadlock.
	if ( nApplied < 0 ) {
		___WARNINGLOG( QString( "%1: all setsockopt attempts failed (last "
								"errno=%2 '%3'); buffer stays at default "
								"readback=%4 — large frames may deadlock a "
								"synchronous receive()" )
						   .arg( optlabel )
						   .arg( nLastErrno )
						   .arg( strerror( nLastErrno ) )
						   .arg( nReadback ) );
	}
	else {
		___INFOLOG( QString( "%1: applied=%2 readback=%3" )
						.arg( optlabel ).arg( nApplied ).arg( nReadback ) );
	}
	return nReadback;
}
#endif

void enlargeSocketBuffers( QLocalSocket* pSocket ) {
#if defined( Q_OS_UNIX )
	const qintptr fd = pSocket->socketDescriptor();
	if ( fd < 0 ) {
		___WARNINGLOG( "no socket descriptor available; cannot enlarge buffers" );
		return;
	}
	raiseSocketBuffer( static_cast<int>( fd ), SO_SNDBUF, "SO_SNDBUF" );
	raiseSocketBuffer( static_cast<int>( fd ), SO_RCVBUF, "SO_RCVBUF" );
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

	// After flush(), bytesToWrite() is what the OS send buffer could NOT absorb
	// and still sits in Qt's user-space write buffer. In normal cross-thread use
	// the peer drains it shortly; but a single-threaded synchronous
	// send()→receive() (the tests) has no concurrent drainer, so a non-zero
	// remainder here is exactly the deadlock condition. Surface it so a future
	// run pinpoints an undersized socket buffer rather than failing opaquely.
	const qint64 nRemaining = m_pSocket->bytesToWrite();
	if ( nWritten != static_cast<qint64>( frame.size() ) || nRemaining != 0 ) {
		___WARNINGLOG( QString( "incomplete send: opcode=%1 frame=%2 written=%3 "
								"bytesToWrite-after-flush=%4 (a non-zero "
								"remainder deadlocks a synchronous receive() on "
								"the same thread)" )
						   .arg( static_cast<int>( msg.getOpcode() ) )
						   .arg( frame.size() ).arg( nWritten )
						   .arg( nRemaining ) );
	}
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
				// Distinguish "nothing arrived" from "a partial frame arrived but
				// the rest is stuck in the sender" (the buffer-deadlock symptom):
				// bufferedBytes()>0 with no complete message means a frame was cut
				// off mid-flight — i.e. the sender could not flush it all.
				___WARNINGLOG( QString( "receive timed out after %1 ms: "
										"socketState=%2 bytesAvailable=%3 "
										"readerBuffered=%4 pending=%5" )
								   .arg( nTimeoutMs )
								   .arg( static_cast<int>( m_pSocket->state() ) )
								   .arg( m_pSocket->bytesAvailable() )
								   .arg( m_reader.bufferedBytes() )
								   .arg( static_cast<int>( m_pending.size() ) ) );
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
