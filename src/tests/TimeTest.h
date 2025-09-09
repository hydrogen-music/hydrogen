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

#include <core/config.h>

#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>

class TimeTest : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE( TimeTest );
		CPPUNIT_TEST( testElapsedTime );
		CPPUNIT_TEST( testHighResolutionSleep );
		CPPUNIT_TEST_SUITE_END();

	public:
		void setUp();
		void tearDown();

		/** Adds a couple of tempo markers and moves to various places within
		 * the song to check the calculation of the elapsed time.
		 */
		void testElapsedTime();

		void testHighResolutionSleep();

	private:

		/**
		 * \param nPatternPos Index of the pattern group to locate to
		 *
		 * \return float Time in seconds passed at @a nPatternPos since the
		 *   beginning of the song.
		 */
		float locateAndLookupTime( int nPatternPos );

		QString m_sValidPath;
};
