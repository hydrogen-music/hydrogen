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

#ifdef WIN32
#include <atomic>
#endif

/**
 * Cross-platform stdin reader that runs in a worker thread.
 *
 * The worker parks in poll() on stdin plus a wake pipe (POSIX) or polls
 * _kbhit() (Windows) and emits a signal for every character received, so
 * the main thread stays free to pump the Qt event loop.
 *
 * The worker must never block inside std::cin.get(): a thread blocked in a
 * read cannot be cancelled — QThread::terminate() is a no-op for a slot
 * connected to QThread::started because Qt keeps pthread cancellation
 * disabled until QThread::run() — and even a successful cancellation would
 * risk dying while holding std::cin's stream lock, which deadlocks
 * exit-time stream destruction. stop() therefore wakes the poll() loop
 * through the wake pipe so the thread always leaves run() on its own.
 */
class StdinReader : public QObject {
	Q_OBJECT

   public:
	explicit StdinReader( QObject* pParent = nullptr );
	~StdinReader();

	/** Wake the reader loop in run() so the worker thread finishes and
	 * QThread::wait() succeeds. Safe to call more than once. */
	void stop();

   public slots:
	/** Loop reading characters from stdin and emitting signals. */
	void run();

   signals:
	/** Emitted for every character read from stdin. */
	void characterReceived( char c );
	/** Emitted when stdin reports EOF (e.g. Ctrl+D or a closed pipe) or
	 * an unrecoverable read error: no further input will arrive, so the
	 * caller should shut down. */
	void inputClosed();

   private:
#ifdef WIN32
	/** Checked between _kbhit() polls; set by stop(). */
	std::atomic<bool> m_bStopRequested = false;
#else
	/** Wake pipe: run() polls [ 0 ], stop() writes [ 1 ]. Both -1 if the
	 * pipe could not be created (run() then polls stdin only and the
	 * caller's bounded QThread::wait() leaks the thread — it parks in
	 * poll() holding no lock, so the process can still exit). */
	int m_anWakePipe[ 2 ] = { -1, -1 };
#endif
};

#endif	// STDIN_READER_H
