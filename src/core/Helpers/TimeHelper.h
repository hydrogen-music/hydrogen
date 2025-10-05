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

#ifndef TIME_HELPER_H
#define TIME_HELPER_H

#include <core/Helpers/Time.h>
#include <core/Object.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace H2Core
{

/** Sleeping is quite complicated. Giving up control and relying on the OS
	scheduler to retrieve it again is expensive. The C++ std method
	std::this_thread::sleep_for only guarantees to sleep for at least the
	provided amount. It could very well sleep longer. And it does. To circumvent
	this problem, we ask it for sleeping a little bit less and just wait the
	remaining time. */
class TimeHelper : public Object<TimeHelper>
{
		H2_OBJECT(TimeHelper)

	static constexpr int nSurplusVectorLength = 100;

public:
	TimeHelper();
	~TimeHelper();

	void highResolutionSleep(
		std::chrono::duration<float, std::micro> interval );

	bool isBurnedIn() const;


private:
	/** Perform a number of sleeps in a separate thread to fill
	 * #m_sleepSurplusNs in order to do some statistics in
	 * highResolutionSleep(). */
	static void burnIn( void* pInstance );
	bool m_bIsBurnedIn;
	bool m_bAbortBurnIn;

	/** Measurements of the time surplus slept by the system.
	 *
	 * When requesting a sleep for X ns and the system does sleep for X + Y ns,
     * the `Y` is stored in this vector. On the next highResolutionSleep(), it
     * is taken into account when compensating for the surplus time the system
     * may require during sleep to switch back the context to the calling
     * thread. */
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
