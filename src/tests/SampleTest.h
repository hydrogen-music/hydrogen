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

#ifndef H2TEST_SAMPLE_TEST_H
#define H2TEST_SAMPLE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class SampleTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( SampleTest );
	CPPUNIT_TEST( testLoadInvalidSample );
	CPPUNIT_TEST( testStoringSamplesInCurrentDrumkit );
	CPPUNIT_TEST_SUITE_END();

	void testLoadInvalidSample();
	/** For portability paths to samples within one of Hydrogen's installed
	 * drumkits will be stripped and only the filename is stored along with the
	 * name of the drumkit. But for samples imported into the current / the
	 * song's drumkit we have to be very careful. Only when saving the drumkit
	 * to the sound library, all contained samples will be copied into the
	 * corresponding drumkit folder. Priorly they can very well be scattered all
	 * over the place. */
	void testStoringSamplesInCurrentDrumkit();
};

#endif
