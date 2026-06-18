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

#ifndef H2C_IPC_SERVER_H
#define H2C_IPC_SERVER_H

#include <core/IPC/IpcChannel.h>

#include <QtCore/QObject>
#include <QtCore/QString>

class QLocalServer;

namespace H2Core {

/**
 * The engine-side listener: a QLocalServer that hands out an IpcChannel per
 * connecting editor process.
 *
 * \ingroup docCore
 */
class IpcServer : public QObject {
	Q_OBJECT
public:
	explicit IpcServer( QObject* pParent = nullptr );
	~IpcServer() override;

	bool listen( const QString& sName );
	QString serverName() const;

	/** Wrap the next pending connection, or nullptr if none. */
	IpcChannel* nextPendingChannel();
	/** Block until a connection arrives (or timeout); for synchronous/tests. */
	IpcChannel* waitForChannel( int nTimeoutMs = 3000 );

signals:
	/** Emitted when a connection is waiting; the consumer accepts it with
	 * nextPendingChannel(). We do not auto-accept, so this never races with the
	 * blocking waitForChannel(). */
	void connectionPending();

private:
	QLocalServer* m_pServer;
};

};

#endif
