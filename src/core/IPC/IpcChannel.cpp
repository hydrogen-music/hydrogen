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
// Upper bound for draining a single frame to the OS in send() (see the
// waitForBytesWritten() loop). Generous: a healthy peer drains far faster; this
// only caps a pathological stall so send() never hangs a thread forever.
constexpr int kSendTimeoutMs = 3000;

// receive()/request() poll the socket in slices of this size rather than issuing
// one long waitForReadyRead(). On Windows a socket moved to an event-loop-less
// thread (the test responder) — and any multi-read frame — may not get a
// readyRead signal for every chunk, so a single long wait can block the entire
// timeout while bytes sit unread; a short wait still pumps the pipe read, and
// pump() reassembles what has arrived. Small enough to keep latency low, large
// enough to avoid busy-spinning.
constexpr int kPollSliceMs = 25;

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

IpcChannel::IpcChannel( QLocalSocket* pSocket, QObject* pParent,
						bool bPushWrites )
	: QObject( pParent )
	, m_pSocket( pSocket )
	, m_bPushWrites( bPushWrites ) {
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

	if ( ! m_bPushWrites ) {
		// Client/editor side: fire-and-forget. The editor runs a Qt event loop
		// (or, in white-box tests, its peer reads synchronously) which pushes the
		// write out, so we must NOT block here — a send with no concurrent reader
		// (e.g. the connect-time hello, or a command issued before the peer
		// reads) would otherwise stall for the whole timeout.
		return nWritten == static_cast<qint64>( frame.size() );
	}

	// Server/engine side: drive the write to completion. On Windows a QLocalSocket
	// is an *overlapped* named pipe — write()/flush() only INITIATE the I/O, and
	// the engine's sending threads (EngineSession::serve, request responders) run
	// no Qt event loop, so without this push a forwarded event/reply can sit
	// half-sent and never reach the editor. The peer is always reading, so this
	// returns promptly. On Unix flush() already wrote to the fd, so bytesToWrite()
	// is 0 and the loop is a no-op. Bounded so a peer that vanished mid-send (a
	// closing pipe at teardown) is surfaced below instead of hanging.
	drainWrite();

	const qint64 nPending = m_pSocket->bytesToWrite();
	if ( nWritten != static_cast<qint64>( frame.size() ) || nPending != 0 ) {
		___WARNINGLOG( QString( "incomplete server send: opcode=%1 frame=%2 "
								"written=%3 bytesToWrite-after-push=%4 (peer gone "
								"or unresponsive within %5 ms)" )
						   .arg( static_cast<int>( msg.getOpcode() ) )
						   .arg( frame.size() ).arg( nWritten )
						   .arg( nPending ).arg( kSendTimeoutMs ) );
	}
	return nWritten == static_cast<qint64>( frame.size() );
}

void IpcChannel::drainWrite() {
	if ( m_pSocket == nullptr ) {
		return;
	}
	QElapsedTimer timer;
	timer.start();
	while ( m_pSocket->bytesToWrite() > 0 ) {
		const int nRemaining =
			kSendTimeoutMs - static_cast<int>( timer.elapsed() );
		if ( nRemaining <= 0 || ! m_pSocket->waitForBytesWritten( nRemaining ) ) {
			break;
		}
	}
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
	QElapsedTimer timer;
	timer.start();
	while ( m_pending.empty() ) {
		if ( m_pSocket == nullptr ) {
			return false;
		}
		// Drain whatever bytes have already arrived first, then poll in short
		// slices. A single long waitForReadyRead() can block the whole timeout
		// while data sits unread on an event-loop-less / moved socket (or between
		// the chunks of a large frame); a short wait still pumps the pipe read and
		// pump() reassembles. See kPollSliceMs.
		pump();
		if ( ! m_pending.empty() ) {
			break;
		}
		const int nRemaining = nTimeoutMs - static_cast<int>( timer.elapsed() );
		if ( nRemaining <= 0 ) {
			// bufferedBytes()>0 with no complete message means a frame arrived
			// only partially (sender could not flush it all); 0 means nothing came.
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
		m_pSocket->waitForReadyRead(
			nRemaining < kPollSliceMs ? nRemaining : kPollSliceMs );
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
	const bool bSent = send( r );
	if ( ! bSent ) {
		return false;
	}
	// Push the request out now. send() is non-blocking on the client side, but a
	// request is a synchronous round-trip whose peer is already reading; if the
	// caller's thread runs no Qt event loop (white-box tests, headless callers)
	// the overlapped write would otherwise not progress until the peer happens to
	// pull it — observed as a multi-second delay that outran the reply timeout on
	// Windows. The reader drains it immediately, so this returns at once.
	drainWrite();
	___INFOLOG( QString( "request sent: opcode=%1 reqId=%2 payloadBytes=%3 "
						 "bytesToWrite=%4" )
					.arg( static_cast<int>( r.getOpcode() ) ).arg( nId )
					.arg( r.getPayload().size() )
					.arg( m_pSocket->bytesToWrite() ) );

	QElapsedTimer timer;
	timer.start();
	int nFramesSeen = 0;
	while ( true ) {
		// Pull our reply out of the pending queue, preserving the order of any
		// other (event/command) frames so receive()/messageReceived() still get
		// them.
		std::queue<IpcMessage> rest;
		bool bFound = false;
		while ( ! m_pending.empty() ) {
			IpcMessage m = m_pending.front();
			m_pending.pop();
			++nFramesSeen;
			if ( ! bFound && m.getRequestId() == nId ) {
				reply = m;
				bFound = true;
			}
			else {
				// A frame arrived but it is not our reply — log its correlation
				// so a lost/mis-correlated reply is distinguishable from "nothing
				// ever arrived".
				___INFOLOG( QString( "request awaiting reqId=%1: requeuing "
									 "non-matching frame opcode=%2 reqId=%3" )
								.arg( nId )
								.arg( static_cast<int>( m.getOpcode() ) )
								.arg( m.getRequestId() ) );
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
			// Genuine deadline. Report what the socket looked like so a future run
			// shows whether the reply bytes never arrived (bytesAvailable /
			// readerBuffered == 0) or arrived but failed to correlate (logged
			// above).
			___WARNINGLOG( QString( "request timed out after %1 ms awaiting "
									"reqId=%2: framesSeen=%3 socketState=%4 "
									"bytesAvailable=%5 readerBuffered=%6 pending=%7" )
							   .arg( nTimeoutMs ).arg( nId ).arg( nFramesSeen )
							   .arg( static_cast<int>( m_pSocket->state() ) )
							   .arg( m_pSocket->bytesAvailable() )
							   .arg( m_reader.bufferedBytes() )
							   .arg( static_cast<int>( m_pending.size() ) ) );
			return false;
		}
		// Poll for the reply in short slices, then pump. A single long
		// waitForReadyRead() is unreliable here: on Windows it can return false
		// before the timeout elapses, and it may not signal data on an
		// event-loop-less / moved peer socket — so we never treat one false wait
		// as terminal. Only the deadline check above ends the loop. See
		// kPollSliceMs.
		m_pSocket->waitForReadyRead(
			nRemaining < kPollSliceMs ? nRemaining : kPollSliceMs );
		pump();
	}
}

};
