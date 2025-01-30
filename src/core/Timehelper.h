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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#if defined(WIN32) || _DOXYGEN_

#include <time.h>
#include <sysinfoapi.h>

#ifndef TIMEHELPER_H
#define TIMEHELPER_H

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif


#ifndef timersub
# define timersub(a, b, result)						      \
  do {									      \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
	if ((result)->tv_usec < 0) {					      \
	  --(result)->tv_sec;						      \
	  (result)->tv_usec += 1000000;					      \
   }									      \
  } while (0)
#endif



#ifndef _TIMEZONE_DEFINED /* also in sys/time.h and time.h */
/*
 * Maybe I should #define _TIMEZONE_DEFINED but more care is needed.
 * It will be OK for now, if this file is included last.
 * Investigate what else is defined/declared in the other files.
 * Investigate the declaration of function gettimeofday below.
 */
struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
#endif /* _TIMEZONE_DEFINED */

int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#endif
