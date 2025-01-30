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

#ifndef H2_RTCLOCK_H
#define H2_RTCLOCK_H

#include <core/config.h>

/*
	RTCLOCK_SETUP
	RTCLOCK_START
	...
	code to be benchmarked
	...
	RTCLOCK_STOP
	DEBUGLOG(QString("elapsed time : %1 [ns]").arg(RTCLOCK_NS))
	DEBUGLOG(QString("elapsed time : %1 [us]").arg(RTCLOCK_US))
	DEBUGLOG(QString("elapsed time : %1 [ms]").arg(RTCLOCK_MS))
 */

#if defined(H2CORE_HAVE_DEBUG) and defined(HAVE_RTCLOCK)
	#include <time.h>
	//#include <stdint.h>
	#define RTCLOCK_SETUP struct timespec __t0,__t1; //uint64_t __dt;
	#define RTCLOCK_START clock_gettime(CLOCK_MONOTONIC, &__t0);
	#define RTCLOCK_STOP clock_gettime(CLOCK_MONOTONIC, &__t1);
	#define RTCLOCK_NS (((__t1.tv_sec * 1000000000) + __t1.tv_nsec) - ((__t0.tv_sec * 1000000000) + __t0.tv_nsec))
	#define RTCLOCK_US (((__t1.tv_sec * 1000000000) + __t1.tv_nsec) - ((__t0.tv_sec * 1000000000) + __t0.tv_nsec))/1000
	#define RTCLOCK_MS (((__t1.tv_sec * 1000000000) + __t1.tv_nsec) - ((__t0.tv_sec * 1000000000) + __t0.tv_nsec))/1000000
#else
	#define RTCLOCK_SETUP
	#define RTCLOCK_START
	#define RTCLOCK_STOP
	#define RTCLOCK_NS -1
	#define RTCLOCK_US -1
	#define RTCLOCK_MS -1
#endif

#endif // H2_RTCLOCK_H
