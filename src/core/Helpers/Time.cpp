/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "Time.h"

#include <ctime>
#include <iomanip>
#include <sstream>


namespace H2Core {
	QString timePointToQString( const TimePoint& timePoint ) {
		// #Clock is monotonic and has no relation to the calendar. Its time
		// points are translated into wall-clock time via the current offset
		// between both clocks.
		const auto wallTimePoint = std::chrono::system_clock::now() +
			std::chrono::duration_cast<std::chrono::system_clock::duration>(
				timePoint - Clock::now() );

		const auto t = std::chrono::system_clock::to_time_t( wallTimePoint );
		const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			wallTimePoint.time_since_epoch() ) % 1000;
		std::ostringstream timePointStringStream;
		timePointStringStream
			<< std::put_time( std::gmtime( &t ), "%H:%M:%S")
			<< '.' << std::setfill( '0' ) << std::setw( 3 ) << ms.count();

		return QString::fromStdString( timePointStringStream.str() );
	}
};
