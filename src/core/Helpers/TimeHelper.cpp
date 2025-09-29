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

#include "TimeHelper.h"
#include <algorithm>
#include <chrono>
#include <locale>
#include <mutex>
#include <thread>

namespace H2Core {

TimeHelper::TimeHelper() : m_nSleepSurplusIndex( 0 )
						   , m_bIsBurnedIn( false )
						   , m_bAbortBurnIn( false )
{
	m_sleepSurplusNs.resize( TimeHelper::nSurplusVectorLength );
	for ( int ii = 0; ii < m_sleepSurplusNs.size(); ++ii ) {
		m_sleepSurplusNs[ ii ] = 0;
	}

	m_pBurnInThread = std::make_shared< std::thread >(
		TimeHelper::burnIn, (void*)this );
}

TimeHelper::~TimeHelper() {
	if ( m_pBurnInThread != nullptr ) {
		m_bAbortBurnIn = true;
		m_pBurnInThread->join();
	}
}

void TimeHelper::highResolutionSleep(
	std::chrono::duration<float, std::micro> interval )
{
	const auto start = Clock::now();

	// Giving up control and relying on the OS scheduler to retrieve it again is
	// expensive. The C++ std method std::this_thread::sleep_for only guarantees
	// to sleep for at least the provided amount. It could very sleep longer.
	// And it does. To circumvent this problem, we ask it for sleeping a little
	// bit less and just wait the remaining time.
	long long nSleepSurplusNs = 0;
	{
		std::scoped_lock lock{ m_sleepSurplusMutex };
		nSleepSurplusNs = *std::max_element( m_sleepSurplusNs.begin(),
											m_sleepSurplusNs.end() );
	}

	const std::chrono::duration<long long, std::nano> residualInterval{
		nSleepSurplusNs };
	DEBUGLOG( QString( "Max surplus: %1" ).arg( nSleepSurplusNs ) );
	if ( interval > residualInterval ) {

		// We measure the time require for sleeping to do some statistics and
		// provide the best guess possible when calculating the next sleep
		// interval.
		const auto preSleep = Clock::now();
		std::this_thread::sleep_for( interval - residualInterval );
		const auto postSleep = Clock::now();

		std::scoped_lock lock{ m_sleepSurplusMutex };
		if ( m_nSleepSurplusIndex >= m_sleepSurplusNs.size() ) {
			m_nSleepSurplusIndex = 0;
		}
		m_sleepSurplusNs[ m_nSleepSurplusIndex ] =
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				postSleep - preSleep - interval + residualInterval ).count();
		DEBUGLOG( QString( "Current surplus interval: %1" )
.arg( m_sleepSurplusNs[ m_nSleepSurplusIndex ] ) );
		++m_nSleepSurplusIndex;
	}

	// This is very expensive. We waste a lot of CPU by doing nothing. To make
	// the time spent on this loop as small as possible is an appropriate
	// optimization target.
	while ( Clock::now() - start < interval ) {
	}
}

void TimeHelper::burnIn( void* pInstance ) {
	auto pTimeHelper = static_cast<TimeHelper*>( pInstance );
	if ( pTimeHelper == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	for ( int ii = 0; ii < TimeHelper::nSurplusVectorLength; ++ii ) {
		if ( pTimeHelper->m_bAbortBurnIn ) {
			return;
		}
		std::scoped_lock lock{ pTimeHelper->m_sleepSurplusMutex };
		if ( pTimeHelper->m_bAbortBurnIn ) {
			return;
		}
		const auto start = Clock::now();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		const auto end = Clock::now();

		const auto sleepDuration = end - start - std::chrono::milliseconds( 1 );
		pTimeHelper->m_sleepSurplusNs[ ii ] =
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				sleepDuration ).count();
	}

	if ( pTimeHelper->m_bAbortBurnIn ) {
		return;
	}
	std::scoped_lock lock{ pTimeHelper->m_sleepSurplusMutex };
	if ( pTimeHelper->m_bAbortBurnIn ) {
		return;
	}
	pTimeHelper->m_nSleepSurplusIndex = 0;
	pTimeHelper->m_bIsBurnedIn = true;
}

};
