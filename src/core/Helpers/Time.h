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

#ifndef TIME_H
#define TIME_H


#include <chrono>
#include <ctime>
#include <iomanip>
#include <QString>
#include <sstream>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

namespace H2Core {

	static QString timePointToQString( const TimePoint& timePoint ) {
		auto t = Clock::to_time_t( timePoint );
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			timePoint.time_since_epoch() ) % 1000;
		std::ostringstream timePointStringStream;
		timePointStringStream
			<< std::put_time( std::gmtime( &t ), "%Y-%m-%dT%H:%M:%S")
			<< '.' << std::setfill( '0' ) << std::setw( 3 ) << ms.count();

		return std::move( QString::fromStdString( timePointStringStream.str() ) );
	}

};
#endif
