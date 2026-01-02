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
#ifndef H2C_GLOBALS_H
#define H2C_GLOBALS_H

/** \addtogroup docConfiguration
 * \addtogroup docCore
 * @{
 */

namespace H2Core {

	/** How many ticks make up a quarter note. This defines the smallest
	 * possible resolution to 1 / nTicksPerQuarter. */
	constexpr int nTicksPerQuarter = 48;
}

#define MIN_BPM                  10
#define MAX_BPM                 400

#define SAMPLE_CHANNELS         2

#define TWOPI                   6.28318530717958647692

#define UNUSED( v )             (v = v)

// m_nBeatCounter
//100,000 ms in 1 second.
#define                         US_DIVIDER .000001
// ~m_nBeatCounter

/** @} */

#endif // H2C_GLOBALS_H
