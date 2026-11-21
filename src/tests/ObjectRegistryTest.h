/*
 * Hydrogen
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_OBJECT_REGISTRY_TEST_H
#define H2C_OBJECT_REGISTRY_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/** Pins the thread safety of the debug-build class registry behind
 * Base::getObjectMap(). */
class ObjectRegistryTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( ObjectRegistryTest );
	CPPUNIT_TEST( testConcurrentFirstConstruction );
	CPPUNIT_TEST_SUITE_END();

   public:
	/** Many threads construct the first instances of many classes at
	 * the same time while another thread reads the registry. Every class
	 * must end up registered once with all its constructions counted. */
	void testConcurrentFirstConstruction();
};

#endif  // H2C_OBJECT_REGISTRY_TEST_H
