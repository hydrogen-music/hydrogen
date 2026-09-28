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

#include "StdinReader.h"

#include <QThread>

#include <iostream>

#ifdef WIN32
#include <conio.h>
#else
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#endif

StdinReader::StdinReader( QObject* pParent ) : QObject( pParent )
{
#ifndef WIN32
	if ( ::pipe( m_anWakePipe ) != 0 ) {
		m_anWakePipe[ 0 ] = m_anWakePipe[ 1 ] = -1;
		std::cerr << "Unable to create the wake pipe for the stdin reader"
				  << std::endl;
	}
#endif
}

StdinReader::~StdinReader()
{
#ifndef WIN32
	// The worker thread has finished (or was leaked) by the time the
	// reader is destroyed, so closing both ends here is race-free.
	if ( m_anWakePipe[ 0 ] >= 0 ) {
		::close( m_anWakePipe[ 0 ] );
	}
	if ( m_anWakePipe[ 1 ] >= 0 ) {
		::close( m_anWakePipe[ 1 ] );
	}
#endif
}

void StdinReader::stop()
{
#ifdef WIN32
	m_bStopRequested.store( true );
#else
	if ( m_anWakePipe[ 1 ] >= 0 ) {
		// The byte's content is irrelevant; waking the poll() is the
		// message. A failed write leaves the caller's bounded
		// QThread::wait() as the fallback.
		char ch = 0;
		::write( m_anWakePipe[ 1 ], &ch, 1 );
	}
#endif
}

void StdinReader::run()
{
#ifdef WIN32
	// Windows offers no poll()-able stdin handle covering both consoles
	// and pipes: poll for console input instead (piped stdin is not
	// supported in interactive mode on Windows).
	while ( ! m_bStopRequested.load() ) {
		if ( _kbhit() ) {
			emit characterReceived( static_cast<char>( _getch() ) );
		}
		else {
			QThread::msleep( 50 );
		}
	}
#else
	// Park in poll() on stdin plus the wake pipe and only read a single
	// byte once stdin is actually readable: the thread then never blocks
	// inside a read and never holds std::cin's stream lock (see the class
	// doc for why that matters).
	pollfd aFds[ 2 ] = {};
	aFds[ 0 ].fd = STDIN_FILENO;
	aFds[ 0 ].events = POLLIN;
	int nFdCount = 1;
	if ( m_anWakePipe[ 0 ] >= 0 ) {
		aFds[ 1 ].fd = m_anWakePipe[ 0 ];
		aFds[ 1 ].events = POLLIN;
		++nFdCount;
	}

	while ( true ) {
		const int nReady = ::poll( aFds, nFdCount, -1 );
		if ( nReady < 0 ) {
			if ( errno == EINTR ) {
				continue;
			}
			emit inputClosed();
			break;
		}
		if ( nFdCount == 2 && aFds[ 1 ].revents != 0 ) {
			// Woken by stop() (or the wake pipe broke): leave without
			// touching stdin again.
			break;
		}
		if ( aFds[ 0 ].revents != 0 ) {
			char ch;
			if ( ::read( STDIN_FILENO, &ch, 1 ) <= 0 ) {
				// EOF (0) or an error (-1): no further input will arrive.
				emit inputClosed();
				break;
			}
			emit characterReceived( ch );
		}
	}
#endif
}
