/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "Time.h"

#include <ctime>
#include <iomanip>
#include <sstream>


namespace H2Core {
	QString timePointToQString( const TimePoint& timePoint ) {
#ifdef Q_OS_MACX
		// We do not use `Clock` defined above since `high_resolution_clock` on
		// macOS is an alias for `steady_clock`, which - in contrast to
		// `system_clock` - does not have a `to_time_t` member.
		return QString::number(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				timePoint.time_since_epoch() ).count() );
#else
		auto t = Clock::to_time_t( timePoint );
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			timePoint.time_since_epoch() ) % 1000;
		std::ostringstream timePointStringStream;
		timePointStringStream
			<< std::put_time( std::gmtime( &t ), "%H:%M:%S")
			<< '.' << std::setfill( '0' ) << std::setw( 3 ) << ms.count();

		return std::move( QString::fromStdString( timePointStringStream.str() ) );
#endif
	}
};
