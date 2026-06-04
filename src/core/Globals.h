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
#ifndef H2C_GLOBALS_H
#define H2C_GLOBALS_H

#include <QtGlobal>

/** \addtogroup docConfiguration
 * \addtogroup docCore
 * @{
 */

/**
 * Symbol visibility macro for the Hydrogen core library.
 *
 * When core is built as a shared library, classes used across the library
 * boundary must be marked for export/import. This matters in particular for
 * QObject subclasses that emit signals: the new-style (pointer-to-member)
 * QObject::connect() resolves a signal by comparing the function pointer taken
 * at the call site against the one baked into the moc-generated metacall code.
 * On Windows those two pointers only compare equal when the class is properly
 * dllexport'ed (in core) / dllimport'ed (in consumers); otherwise the call-site
 * pointer is an import thunk and the comparison fails at runtime with
 * "QObject::connect: signal not found". Q_DECL_EXPORT/Q_DECL_IMPORT are no-ops
 * for a static build and resolve to visibility attributes on ELF platforms, so
 * this is safe and correct on every supported toolchain (Qt >= 5.15).
 */
#if defined(H2CORE_STATIC)
#  define H2CORE_API
#elif defined(BUILDING_H2CORE)
#  define H2CORE_API Q_DECL_EXPORT
#else
#  define H2CORE_API Q_DECL_IMPORT
#endif

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
