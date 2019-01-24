#if defined(WIN32) || _DOXYGEN_

#include <time.h>
#include <windows.h>

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
