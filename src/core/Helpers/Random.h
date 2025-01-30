/*
 * Hydrogen
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

#ifndef H2C_RANDOM_H
#define H2C_RANDOM_H

#include <core/Object.h>

namespace H2Core
{

/**
 * Container for functions generating random number.
 *
 * \ingroup docCore
 */
class Random : public H2Core::Object<Random>
{
	H2_OBJECT(Random)
public:
	/**
	 * Draws an uncorrelated random value from a Gaussian distribution
	 * of mean 0 and @a fStandardDeviation.
	 *
	 * @param fStandardDeviation Defines the width of the distribution used.
	 */
	static float getGaussian( float fStandardDeviation );
};

};

#endif  // H2C_RANDOM_H
