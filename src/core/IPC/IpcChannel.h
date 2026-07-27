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
 * messages by an IpcFrameReader; each complete message is both emitted via
 * messageReceived() (for an event-loop consumer) and queued for the blocking
 * receive() (for synchronous callers and tests).
 *
 * \ingroup docCore
 */
class IpcChannel : public QObject {
	Q_OBJECT
public:
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
						 bool bPushWrites = false );
	~IpcChannel() override;

	/** Connect to a named server as a client. Returns nullptr on failure. */
	static IpcChannel* connectToServer( const QString& sName,
										int nTimeoutMs = 3000,
										QObject* pParent = nullptr );

	bool isConnected() const;
	bool send( const IpcMessage& msg );
	/** Block until one message is available (or timeout). Returns false on
	 * timeout/disconnect. */
	bool receive( IpcMessage& out, int nTimeoutMs = 3000 );

	/** Blocking request/response (ADR 0030 tier 3). Stamps @a req with a fresh
	 * non-zero request id, sends it, and blocks until the matching reply (a
	 * frame echoing that id) arrives, returning it in @a reply. Other frames
	 * received while waiting stay queued for receive()/messageReceived(), in
	 * order. Returns false on timeout/disconnect. Used editor-side for the few
	 * commands that need an engine-computed result. */
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
	IpcFrameReader m_reader;
	std::queue<IpcMessage> m_pending;
	/** Monotonic source of request ids; 0 is reserved for "no correlation". */
	quint32 m_nNextRequestId = 1;
};

};

#endif
