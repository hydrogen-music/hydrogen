/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef STDIN_READER_H
#define STDIN_READER_H

#include <QObject>

/**
 * Cross-platform stdin reader that runs in a worker thread.
 *
 * @c std::cin.get() blocks until input is available on all platforms (Unix,
 * Windows, macOS).  By moving this blocking call into a dedicated thread and
 * emitting a signal for every character received, the main thread is free to
 * pump the Qt event loop without resorting to POSIX-only constructs such as
 * @c fd_set / @c select().
 *
 * The worker thread has no event loop; it simply loops on @c std::cin.get()
 * until EOF is reached or the thread is terminated.  Because the thread is
 * blocked in @c std::cin.get() during shutdown, the caller is expected to
 * @c QThread::terminate() it.  @c QThread::wait() is called with a bounded
 * timeout so that, if @c terminate() fails to interrupt the blocking read on
 * a particular platform, the caller does not hang indefinitely — the leaked
 * thread is reclaimed when the process exits.
 */
class StdinReader : public QObject {
	Q_OBJECT

   public:
	explicit StdinReader( QObject* pParent = nullptr );

   public slots:
	/** Blocking loop that reads characters from stdin and emits signals. */
	void run();

   signals:
	/** Emitted for every character read from stdin. */
	void characterReceived( char c );
};

#endif	// STDIN_READER_H