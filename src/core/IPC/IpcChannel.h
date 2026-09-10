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

#ifndef H2C_IPC_CHANNEL_H
#define H2C_IPC_CHANNEL_H

#include <core/IPC/IpcMessage.h>
#include <core/Object.h>

#include <QtCore/QObject>
#include <QtCore/QString>

#include <queue>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace H2Core {

/**
 * One end of the editor↔engine control channel (ADR 0018): a QLocalSocket
 * carrying length-prefixed IpcMessages. Received bytes are reassembled into
 * messages by an IpcFrameReader; each complete message is emitted via
 * messageReceived() (for an event-loop consumer) and — depending on the
 * #DeliveryMode — also queued for the blocking receive() (for synchronous
 * callers and tests).
 *
 * \ingroup docCore
 */
class IpcChannel : public QObject, public H2Core::Object<IpcChannel> {
	H2_OBJECT( IpcChannel )
	Q_OBJECT
public:
	/** How received frames reach their consumer.
	 *
	 * Poll (engine side, white-box tests): every frame is queued in addition
	 * to being emitted — a polling receive() consumer drains the queue.
	 *
	 * Signal (editor side): frames are delivered by the messageReceived emit
	 * only. The editor has no polling consumer, so queueing signal-delivered
	 * frames too would accumulate them for the whole session (nothing but
	 * request() ever pops the queue, and it re-queues non-matching frames).
	 * Correlated replies are queued in both modes — request() finds them in
	 * the queue. */
	enum class DeliveryMode { Poll, Signal };

	/** Wrap an already-connected socket. Takes ownership.
	 *
	 * @param bPushWrites when true, send() drives each write to completion via
	 * waitForBytesWritten(). This is needed only on the engine/server side,
	 * whose sending threads (the EngineSession serve loop, request responders)
	 * run no Qt event loop, so a bare flush() does not push the overlapped
	 * write to the peer on Windows. The editor/client side runs a Qt event loop
	 * (or, in white-box tests, its peer reads synchronously), so it leaves this
	 * false and keeps send() non-blocking — otherwise a fire-and-forget editor
	 * send (hello, commands) with no concurrent reader would stall for the
	 * whole timeout. */
	explicit IpcChannel( QLocalSocket* pSocket, QObject* pParent = nullptr,
						 bool bPushWrites = false,
						 DeliveryMode mode = DeliveryMode::Poll );
	~IpcChannel() override;

	/** Connect to a named server as a client. Returns nullptr on failure. */
	static IpcChannel* connectToServer( const QString& sName,
										int nTimeoutMs = 3000,
										QObject* pParent = nullptr,
										DeliveryMode mode = DeliveryMode::Poll );

	bool isConnected() const;
	/** Number of frames waiting in the pending queue. Diagnostics and tests
	 * only — call from the channel's owning thread. */
	int pendingCount() const { return static_cast<int>( m_pending.size() ); }
	/** Close the underlying socket. If the socket was connected, this emits
	 * #disconnected (relayed from QLocalSocket). Idempotent: a no-op when
	 * already disconnected. The channel object itself stays valid — only the
	 * transport is torn down. */
	void close();
	bool send( const IpcMessage& msg );
	/** Block until one message is available (or timeout). Returns false on
	 * timeout/disconnect. When @a bLogTimeout is false a timeout is silent —
	 * used by the EngineSession serve loop, which polls with a short timeout
	 * and a timeout is the normal idle case, not a warning. */
	bool receive( IpcMessage& out, int nTimeoutMs = 3000,
				  bool bLogTimeout = true );

	/** Blocking request/response (ADR 0030 tier 3). Stamps @a req with a fresh
	 * non-zero request id, sends it, and blocks until the matching reply (a
	 * frame echoing that id) arrives, returning it in @a reply. Non-reply
	 * frames received while waiting stay queued in order for a polling
	 * receive() consumer (Poll mode; on Signal channels they were already
	 * delivered via messageReceived and are not queued at all). A reply whose
	 * id belongs to an earlier, timed-out request is dropped: with one
	 * request in flight at a time it can never be correlated again. Returns
	 * false on timeout/disconnect. Used editor-side for the few commands that
	 * need an engine-computed result. */
	bool request( const IpcMessage& req, IpcMessage& reply, int nTimeoutMs = 5000 );

signals:
	void messageReceived( const H2Core::IpcMessage& msg );
	void disconnected();

private slots:
	void onReadyRead();

private:
	void pump();
	/** Drive the socket's pending write bytes to completion (bounded). Needed on
	 * Windows where an overlapped pipe write only progresses via the event loop or
	 * an explicit wait; used by the server-side send() and by request() (whose
	 * caller's thread may run no event loop while a separate peer reads). */
	void drainWrite();

	QLocalSocket* m_pSocket;
	/** See the constructor: drive writes to completion (server side only). */
	bool m_bPushWrites;
	/** See DeliveryMode: whether received frames are queued for receive(). */
	DeliveryMode m_mode;
	IpcFrameReader m_reader;
	std::queue<IpcMessage> m_pending;
	/** Monotonic source of request ids; 0 is reserved for "no correlation". */
	quint32 m_nNextRequestId = 1;
};

};

#endif
