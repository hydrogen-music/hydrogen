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

#ifndef H2C_TIME_H
#define H2C_TIME_H


#include <chrono>
#include <QString>

/** Clock for all time points used for timing (MIDI, tapping, sleeping).
 *
 * It has to be monotonic: deadlines and intervals must not follow changes of
 * the wall clock (NTP steps, manual adjustments). `high_resolution_clock` is
 * not suited, since it is an alias of the wall clock `system_clock` in
 * libstdc++ (Linux, MinGW). `steady_clock` has the same resolution
 * (CLOCK_MONOTONIC on Linux) and is consistent across processes on the same
 * machine, as required for time points sent via IPC. */
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

namespace H2Core {

	/** Formats @a timePoint as wall-clock time (UTC) "HH:MM:SS.mmm". */
	QString timePointToQString( const TimePoint& timePoint );

};
#endif
