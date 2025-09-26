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

#include <thread>

namespace H2Core {

	TimeHelper::TimeHelper() {
	}
	TimeHelper::~TimeHelper() {
	}

	void TimeHelper::highResolutionSleep(
		std::chrono::duration<float, std::micro> interval )
	{
		const auto start = Clock::now();

		// Giving up control and relying on the OS scheduler to retrieve it
		// again is expensive. The C++ std method std::this_thread::sleep_for
		// only guarantees to sleep for at least the provided amount. It could
		// very sleep longer. And it does. To circumvent this problem, we ask it
		// for sleeping a little bit less and just wait the remaining time.
		const std::chrono::duration<float, std::milli> residualInterval{ 5 };
		if ( interval > residualInterval ) {
			std::this_thread::sleep_for( interval - residualInterval );
		}

		// This is very expensive. We waste a lot of CPU by doing nothing. To
		// make the time spent on this loop as small as possible is an
		// appropriate optimization target.
		while ( Clock::now() - start < interval ) {
		}
	}

};
