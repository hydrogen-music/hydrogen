#include <bits/time.h> // clock_adjtime
#include <bits/timex.h> // timex
#include <bits/types/struct_timeval.h> // timeval
#include <iostream> // --trace
#include <pthread.h> // __cancel_jmp_buf_tag
#include <pthread.h> // __gthread_active_p
#include <pthread.h> // __gthread_detach
#include <pthread.h> // __gthread_equal
#include <pthread.h> // __gthread_getspecific
#include <pthread.h> // __gthread_key_delete
#include <pthread.h> // __gthread_self
#include <pthread.h> // __gthread_setspecific
#include <pthread.h> // __gthread_yield
#include <pthread.h> // __pthread_cleanup_frame
#include <pthread.h> // __pthread_cond_s
#include <pthread.h> // __pthread_cond_s::(anonymous)
#include <pthread.h> // __pthread_internal_list
#include <pthread.h> // __pthread_internal_slist
#include <pthread.h> // __pthread_mutex_s
#include <pthread.h> // __pthread_rwlock_arch_t
#include <pthread.h> // _pthread_cleanup_buffer
#include <pthread.h> // pthread_attr_destroy
#include <pthread.h> // pthread_attr_getdetachstate
#include <pthread.h> // pthread_attr_getguardsize
#include <pthread.h> // pthread_attr_getinheritsched
#include <pthread.h> // pthread_attr_getschedparam
#include <pthread.h> // pthread_attr_getschedpolicy
#include <pthread.h> // pthread_attr_getscope
#include <pthread.h> // pthread_attr_getstacksize
#include <pthread.h> // pthread_attr_init
#include <pthread.h> // pthread_attr_setdetachstate
#include <pthread.h> // pthread_attr_setguardsize
#include <pthread.h> // pthread_attr_setinheritsched
#include <pthread.h> // pthread_attr_setschedparam
#include <pthread.h> // pthread_attr_setschedpolicy
#include <pthread.h> // pthread_attr_setscope
#include <pthread.h> // pthread_attr_setstack
#include <pthread.h> // pthread_attr_setstackaddr
#include <pthread.h> // pthread_attr_setstacksize
#include <pthread.h> // pthread_attr_t
#include <pthread.h> // pthread_cancel
#include <pthread.h> // pthread_detach
#include <pthread.h> // pthread_equal
#include <pthread.h> // pthread_exit
#include <pthread.h> // pthread_getattr_default_np
#include <pthread.h> // pthread_getattr_np
#include <pthread.h> // pthread_getconcurrency
#include <pthread.h> // pthread_getcpuclockid
#include <pthread.h> // pthread_getname_np
#include <pthread.h> // pthread_getschedparam
#include <pthread.h> // pthread_getspecific
#include <pthread.h> // pthread_key_delete
#include <pthread.h> // pthread_self
#include <pthread.h> // pthread_setattr_default_np
#include <pthread.h> // pthread_setcancelstate
#include <pthread.h> // pthread_setcanceltype
#include <pthread.h> // pthread_setconcurrency
#include <pthread.h> // pthread_setname_np
#include <pthread.h> // pthread_setschedparam
#include <pthread.h> // pthread_setschedprio
#include <pthread.h> // pthread_setspecific
#include <pthread.h> // pthread_spin_destroy
#include <pthread.h> // pthread_spin_init
#include <pthread.h> // pthread_spin_lock
#include <pthread.h> // pthread_spin_trylock
#include <pthread.h> // pthread_spin_unlock
#include <pthread.h> // pthread_testcancel
#include <pthread.h> // pthread_yield
#include <pthread.h> // sched_param
#include <sstream> // __str__
#include <time.h> // asctime
#include <time.h> // asctime_r
#include <time.h> // clock
#include <time.h> // clock_getcpuclockid
#include <time.h> // clock_getres
#include <time.h> // clock_gettime
#include <time.h> // clock_nanosleep
#include <time.h> // clock_settime
#include <time.h> // ctime
#include <time.h> // ctime_r
#include <time.h> // difftime
#include <time.h> // dysize
#include <time.h> // getdate
#include <time.h> // getdate_r
#include <time.h> // gmtime
#include <time.h> // gmtime_r
#include <time.h> // itimerspec
#include <time.h> // localtime
#include <time.h> // localtime_r
#include <time.h> // mktime
#include <time.h> // nanosleep
#include <time.h> // strftime
#include <time.h> // strftime_l
#include <time.h> // strptime
#include <time.h> // strptime_l
#include <time.h> // time
#include <time.h> // timegm
#include <time.h> // timelocal
#include <time.h> // timer_delete
#include <time.h> // timer_getoverrun
#include <time.h> // timer_gettime
#include <time.h> // timer_settime
#include <time.h> // timespec
#include <time.h> // timespec_get
#include <time.h> // tm
#include <time.h> // tzset
#include <qtreset.h>


#include <pybind11/pybind11.h>
#include <functional>
#include <string>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qline.h>
#include <QtCore/qmargins.h>
#include <QtCore/qurl.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qrect.h>
#include <QtCore/qtextcodec.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qeasingcurve.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qxmlstream.h>
#include <QtGui/qcolor.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qcalendar.h>
#include <QtXml/qdom.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qtimezone.h>
#include <setjmp.h>
#include <core/Logger.h>
#include <custom_qt_casters.h>


#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>)
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*)
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>)
#endif

void bind_bits_types_struct_timeval(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B0_[timeval] ";
	{ // timeval file:bits/types/struct_timeval.h line:8
		pybind11::class_<timeval, std::shared_ptr<timeval>> cl(M(""), "timeval", "");
		cl.def( pybind11::init( [](){ return new timeval(); } ) );
		cl.def( pybind11::init( [](timeval const &o){ return new timeval(o); } ) );
		cl.def_readwrite("tv_sec", &timeval::tv_sec);
		cl.def_readwrite("tv_usec", &timeval::tv_usec);
		cl.def("assign", (struct timeval & (timeval::*)(const struct timeval &)) &timeval::operator=, "C++: timeval::operator=(const struct timeval &) --> struct timeval &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B1_[timex] ";
	std::cout << "B2_[int clock_adjtime(int, struct timex *)] ";
	std::cout << "B3_[tm] ";
	std::cout << "B4_[itimerspec] ";
	std::cout << "B5_[long clock()] ";
	std::cout << "B6_[long time(long *)] ";
	std::cout << "B7_[double difftime(long, long)] ";
	std::cout << "B8_[long mktime(struct tm *)] ";
	std::cout << "B9_[unsigned long strftime(char *__restrict, unsigned long, const char *__restrict, const struct tm *__restrict)] ";
	std::cout << "B10_[char * strptime(const char *__restrict, const char *__restrict, struct tm *)] ";
	std::cout << "B11_[unsigned long strftime_l(char *__restrict, unsigned long, const char *__restrict, const struct tm *__restrict, struct __locale_struct *)] ";
	std::cout << "B12_[char * strptime_l(const char *__restrict, const char *__restrict, struct tm *, struct __locale_struct *)] ";
	std::cout << "B13_[struct tm * gmtime(const long *)] ";
	std::cout << "B14_[struct tm * localtime(const long *)] ";
	std::cout << "B15_[struct tm * gmtime_r(const long *__restrict, struct tm *__restrict)] ";
	std::cout << "B16_[struct tm * localtime_r(const long *__restrict, struct tm *__restrict)] ";
	std::cout << "B17_[char * asctime(const struct tm *)] ";
	std::cout << "B18_[char * ctime(const long *)] ";
	std::cout << "B19_[char * asctime_r(const struct tm *__restrict, char *__restrict)] ";
	std::cout << "B20_[char * ctime_r(const long *__restrict, char *__restrict)] ";
	std::cout << "B21_[void tzset()] ";
	std::cout << "B22_[long timegm(struct tm *)] ";
	std::cout << "B23_[long timelocal(struct tm *)] ";
	std::cout << "B24_[int dysize(int)] ";
	std::cout << "B25_[int nanosleep(const struct timespec *, struct timespec *)] ";
	std::cout << "B26_[int clock_getres(int, struct timespec *)] ";
	std::cout << "B27_[int clock_gettime(int, struct timespec *)] ";
	std::cout << "B28_[int clock_settime(int, const struct timespec *)] ";
	std::cout << "B29_[int clock_nanosleep(int, int, const struct timespec *, struct timespec *)] ";
	std::cout << "B30_[int clock_getcpuclockid(int, int *)] ";
	std::cout << "B31_[int timer_delete(void *)] ";
	std::cout << "B32_[int timer_settime(void *, int, const struct itimerspec *__restrict, struct itimerspec *__restrict)] ";
	std::cout << "B33_[int timer_gettime(void *, struct itimerspec *)] ";
	std::cout << "B34_[int timer_getoverrun(void *)] ";
	std::cout << "B35_[int timespec_get(struct timespec *, int)] ";
	std::cout << "B36_[struct tm * getdate(const char *)] ";
	std::cout << "B37_[int getdate_r(const char *__restrict, struct tm *__restrict)] ";
	std::cout << "B38_[__pthread_internal_list] ";
	std::cout << "B39_[__pthread_internal_slist] ";
	std::cout << "B40_[__pthread_mutex_s] ";
	std::cout << "B41_[__pthread_rwlock_arch_t] ";
	std::cout << "B42_[__pthread_cond_s] ";
	std::cout << "B43_[pthread_attr_t] ";
	std::cout << "B44_[__jmp_buf_tag] ";
	std::cout << "B45_[_pthread_cleanup_buffer] ";
	std::cout << "B46_[void pthread_exit(void *)] ";
	std::cout << "B47_[int pthread_detach(unsigned long)] ";
	std::cout << "B48_[unsigned long pthread_self()] ";
	std::cout << "B49_[int pthread_equal(unsigned long, unsigned long)] ";
	std::cout << "B50_[int pthread_attr_init(union pthread_attr_t *)] ";
	std::cout << "B51_[int pthread_attr_destroy(union pthread_attr_t *)] ";
	std::cout << "B52_[int pthread_attr_getdetachstate(const union pthread_attr_t *, int *)] ";
	std::cout << "B53_[int pthread_attr_setdetachstate(union pthread_attr_t *, int)] ";
	std::cout << "B54_[int pthread_attr_getguardsize(const union pthread_attr_t *, unsigned long *)] ";
	std::cout << "B55_[int pthread_attr_setguardsize(union pthread_attr_t *, unsigned long)] ";
	std::cout << "B56_[int pthread_attr_getschedparam(const union pthread_attr_t *__restrict, struct sched_param *__restrict)] ";
	std::cout << "B57_[int pthread_attr_setschedparam(union pthread_attr_t *__restrict, const struct sched_param *__restrict)] ";
	std::cout << "B58_[int pthread_attr_getschedpolicy(const union pthread_attr_t *__restrict, int *__restrict)] ";
	std::cout << "B59_[int pthread_attr_setschedpolicy(union pthread_attr_t *, int)] ";
	std::cout << "B60_[int pthread_attr_getinheritsched(const union pthread_attr_t *__restrict, int *__restrict)] ";
	std::cout << "B61_[int pthread_attr_setinheritsched(union pthread_attr_t *, int)] ";
	std::cout << "B62_[int pthread_attr_getscope(const union pthread_attr_t *__restrict, int *__restrict)] ";
	std::cout << "B63_[int pthread_attr_setscope(union pthread_attr_t *, int)] ";
	std::cout << "B64_[int pthread_attr_setstackaddr(union pthread_attr_t *, void *)] ";
	std::cout << "B65_[int pthread_attr_getstacksize(const union pthread_attr_t *__restrict, unsigned long *__restrict)] ";
	std::cout << "B66_[int pthread_attr_setstacksize(union pthread_attr_t *, unsigned long)] ";
	std::cout << "B67_[int pthread_attr_setstack(union pthread_attr_t *, void *, unsigned long)] ";
	std::cout << "B68_[int pthread_getattr_default_np(union pthread_attr_t *)] ";
	std::cout << "B69_[int pthread_setattr_default_np(const union pthread_attr_t *)] ";
	std::cout << "B70_[int pthread_getattr_np(unsigned long, union pthread_attr_t *)] ";
	std::cout << "B71_[int pthread_setschedparam(unsigned long, int, const struct sched_param *)] ";
	std::cout << "B72_[int pthread_getschedparam(unsigned long, int *__restrict, struct sched_param *__restrict)] ";
	std::cout << "B73_[int pthread_setschedprio(unsigned long, int)] ";
	std::cout << "B74_[int pthread_getname_np(unsigned long, char *, unsigned long)] ";
	std::cout << "B75_[int pthread_setname_np(unsigned long, const char *)] ";
	std::cout << "B76_[int pthread_getconcurrency()] ";
	std::cout << "B77_[int pthread_setconcurrency(int)] ";
	std::cout << "B78_[int pthread_yield()] ";
	std::cout << "B79_[int pthread_setcancelstate(int, int *)] ";
	std::cout << "B80_[int pthread_setcanceltype(int, int *)] ";
	std::cout << "B81_[int pthread_cancel(unsigned long)] ";
	std::cout << "B82_[void pthread_testcancel()] ";
	std::cout << "B83_[__cancel_jmp_buf_tag] ";
	std::cout << "B84_[__pthread_cleanup_frame] ";
	std::cout << "B85_[int pthread_spin_init(volatile int *, int)] ";
	std::cout << "B86_[int pthread_spin_destroy(volatile int *)] ";
	std::cout << "B87_[int pthread_spin_lock(volatile int *)] ";
	std::cout << "B88_[int pthread_spin_trylock(volatile int *)] ";
	std::cout << "B89_[int pthread_spin_unlock(volatile int *)] ";
	std::cout << "B90_[int pthread_key_delete(unsigned int)] ";
	std::cout << "B91_[void * pthread_getspecific(unsigned int)] ";
	std::cout << "B92_[int pthread_setspecific(unsigned int, const void *)] ";
	std::cout << "B93_[int pthread_getcpuclockid(unsigned long, int *)] ";
	std::cout << "B94_[void * __gthrw_pthread_getspecific(unsigned int)] ";
	std::cout << "B95_[int __gthrw_pthread_setspecific(unsigned int, const void *)] ";
	std::cout << "B96_[int __gthrw_pthread_equal(unsigned long, unsigned long)] ";
	std::cout << "B97_[unsigned long __gthrw_pthread_self()] ";
	std::cout << "B98_[int __gthrw_pthread_detach(unsigned long)] ";
	std::cout << "B99_[int __gthrw_pthread_cancel(unsigned long)] ";
	std::cout << "B100_[int __gthrw_sched_yield()] ";
	std::cout << "B101_[int __gthrw_pthread_key_delete(unsigned int)] ";
	std::cout << "B102_[int __gthread_active_p()] ";
	std::cout << "B103_[int __gthread_detach(unsigned long)] ";
	std::cout << "B104_[int __gthread_equal(unsigned long, unsigned long)] ";
	std::cout << "B105_[unsigned long __gthread_self()] ";
	std::cout << "B106_[int __gthread_yield()] ";
	std::cout << "B107_[int __gthread_key_delete(unsigned int)] ";
	std::cout << "B108_[void * __gthread_getspecific(unsigned int)] ";
	std::cout << "B109_[int __gthread_setspecific(unsigned int, const void *)] ";
}
