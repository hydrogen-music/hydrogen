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

#include "TimeHelper.h"
#include <algorithm>
#include <chrono>
#include <locale>
#include <mutex>
#include <thread>

#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <sys/prctl.h>
#endif

namespace H2Core {

TimeHelper::TimeHelper() : m_nSleepSurplusIndex( 0 )
						   , m_bIsBurnedIn( false )
						   , m_bAbortBurnIn( false )
{
	m_sleepSurplusNs.resize( TimeHelper::nSurplusVectorLength );
	for ( int ii = 0; ii < m_sleepSurplusNs.size(); ++ii ) {
		m_sleepSurplusNs[ ii ] =
			std::chrono::nanoseconds( TimeHelper::initialBaseLatency ).count();
	}

	// The burn-in thread serves one engine instance for its whole life, so a
	// single scope at entry routes its logging to that instance's logger
	// (ADR 0015, T1.6). The instance logger is captured at spawn: the
	// constructor runs inside the owning instance's scope.
	auto pLogger = Logger::currentLogger();
	m_pBurnInThread = std::make_shared< std::thread >(
		[pLogger]( void* pInstance ) {
			Logger::Scope loggerScope( pLogger );
			TimeHelper::burnIn( pInstance );
		}, (void*)this );
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
	sleepUntil(
		Clock::now() + std::chrono::duration_cast<Clock::duration>( interval ) );
}

void TimeHelper::sleepUntil( const TimePoint& deadline )
{
	const auto latency = baseLatency();

	while ( true ) {
		// Half of the remaining time: even if the system adds a slack as
		// large as the chunk itself on top of the base latency, we still
		// wake up before the deadline.
		const auto chunk = ( deadline - Clock::now() - latency ) / 2;
		if ( chunk < TimeHelper::minSleepChunk ) {
			break;
		}

		const auto preSleep = Clock::now();
		std::this_thread::sleep_for( chunk );
		if ( chunk <= TimeHelper::baseLatencyProbe ) {
			addSurplus( Clock::now() - preSleep - chunk );
		}
	}

	// This is very expensive. We waste CPU by doing nothing. But it is only
	// done for about the base latency.
	while ( Clock::now() < deadline ) {
	}
}

std::chrono::nanoseconds TimeHelper::baseLatency()
{
	std::scoped_lock lock{ m_sleepSurplusMutex };
	return std::chrono::nanoseconds( *std::max_element(
		m_sleepSurplusNs.begin(), m_sleepSurplusNs.end() ) );
}

void TimeHelper::addSurplus( std::chrono::nanoseconds surplus )
{
	std::scoped_lock lock{ m_sleepSurplusMutex };
	if ( m_nSleepSurplusIndex >= m_sleepSurplusNs.size() ) {
		m_nSleepSurplusIndex = 0;
	}
	m_sleepSurplusNs[ m_nSleepSurplusIndex ] =
		std::max( surplus, std::chrono::nanoseconds::zero() ).count();
	++m_nSleepSurplusIndex;
}

std::optional<std::chrono::nanoseconds> TimeHelper::minimizeTimerSlack()
{
#ifdef __linux__
	// 1ns is the minimum. 0 would reset the slack to the thread's default
	// (50us unless changed).
	if ( prctl( PR_SET_TIMERSLACK, 1UL, 0UL, 0UL, 0UL ) != 0 ) {
		WARNINGLOG( QString( "Unable to minimize timer slack: [%1]" )
						.arg( std::strerror( errno ) ) );
	}
	const int nSlackNs = prctl( PR_GET_TIMERSLACK, 0UL, 0UL, 0UL, 0UL );
	if ( nSlackNs < 0 ) {
		WARNINGLOG( QString( "Unable to retrieve timer slack: [%1]" )
						.arg( std::strerror( errno ) ) );
		return std::nullopt;
	}
	return std::chrono::nanoseconds( nSlackNs );
#else
	return std::nullopt;
#endif
}

void TimeHelper::burnIn( void* pInstance ) {
	auto pTimeHelper = static_cast<TimeHelper*>( pInstance );
	if ( pTimeHelper == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	// The lock is not held while sleeping, so sleeps on other threads are
	// not delayed by the burn-in.
	for ( int ii = 0; ii < TimeHelper::nSurplusVectorLength; ++ii ) {
		if ( pTimeHelper->m_bAbortBurnIn ) {
			return;
		}
		const auto start = Clock::now();
		std::this_thread::sleep_for( TimeHelper::baseLatencyProbe );
		pTimeHelper->addSurplus(
			Clock::now() - start - TimeHelper::baseLatencyProbe );
	}

	pTimeHelper->m_bIsBurnedIn = true;
}

};
