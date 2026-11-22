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

#ifndef H2C_TIME_HELPER_H
#define H2C_TIME_HELPER_H

#include <core/Helpers/Time.h>
#include <core/Object.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace H2Core
{

/** Sleeping is quite complicated. std::this_thread::sleep_for() only
	guarantees to sleep for _at least_ the provided amount. How much longer it
	takes consists of two parts:

	- a base latency of the scheduler to hand control back to the thread
	  (roughly constant; e.g. ~0.1ms on Linux, ~15ms on Windows without
	  timeBeginPeriod), and
	- a slack growing with the requested duration. macOS coalesces timers and
	  lets a sleep of ~160ms end tens of milliseconds late.

	sleepUntil() therefore sleeps in chunks of half the time remaining (minus
	the base latency), so neither part can carry a chunk beyond the deadline,
	and only waits actively for the last, short bit. */
class TimeHelper : public Object<TimeHelper>
{
		H2_OBJECT(TimeHelper)

	static constexpr int nSurplusVectorLength = 100;
	/** Chunks shorter than this are not worth giving up the CPU for. */
	static constexpr std::chrono::microseconds minSleepChunk{ 200 };
	/** Duration of the sleeps measuring the base latency. Chunks this short
	 * hardly carry any duration-dependent slack and do refresh the
	 * measurement. */
	static constexpr std::chrono::milliseconds baseLatencyProbe{ 1 };
	/** Assumed base latency until the burn-in measured the actual one. */
	static constexpr std::chrono::milliseconds initialBaseLatency{ 2 };

public:
	TimeHelper();
	~TimeHelper();

	/** Blocks for @a interval. Equivalent to sleepUntil( now + @a interval ).
	 *
	 * For periodic work - a MIDI clock, a metronome, tapping - call
	 * sleepUntil() with deadlines on a fixed grid instead. Repeated relative
	 * sleeps accumulate the time spent in between. */
	void highResolutionSleep(
		std::chrono::duration<float, std::micro> interval );

	/** Blocks until @a deadline is reached. Returns right away if it
	 * already passed. */
	void sleepUntil( const TimePoint& deadline );

	bool isBurnedIn() const;


private:
	/** Perform a number of short sleeps in a separate thread to measure the
	 * base latency (#m_sleepSurplusNs) right from the start. */
	static void burnIn( void* pInstance );
	std::atomic<bool> m_bIsBurnedIn;
	std::atomic<bool> m_bAbortBurnIn;

	/** Worst recent base latency, see #m_sleepSurplusNs. */
	std::chrono::nanoseconds baseLatency();
	void addSurplus( std::chrono::nanoseconds surplus );

	/** Measurements of the base latency.
	 *
	 * When requesting a short sleep of X ns and the system does sleep for
	 * X + Y ns, the `Y` is stored in this ring buffer. */
	std::vector<long long> m_sleepSurplusNs;
	int m_nSleepSurplusIndex;

	/** Since this class is designed to work in parallel threads, we have to
        guard the access ot #m_sleepSurplusNs in order to avoid race
        conditions. */
	std::mutex m_sleepSurplusMutex;
	std::shared_ptr< std::thread > m_pBurnInThread;

};

inline bool TimeHelper::isBurnedIn() const {
	return m_bIsBurnedIn;
}

};
#endif
