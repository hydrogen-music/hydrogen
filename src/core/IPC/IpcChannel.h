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

class QLocalSocket;

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
	/** Wrap an already-connected socket (server side). Takes ownership. */
	explicit IpcChannel( QLocalSocket* pSocket, QObject* pParent = nullptr );
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

signals:
	void messageReceived( const H2Core::IpcMessage& msg );
	void disconnected();

private slots:
	void onReadyRead();

private:
	void pump();

	QLocalSocket* m_pSocket;
	IpcFrameReader m_reader;
	std::queue<IpcMessage> m_pending;
};

};

#endif
