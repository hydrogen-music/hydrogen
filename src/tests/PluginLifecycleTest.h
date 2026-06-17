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

#ifndef PLUGIN_LIFECYCLE_TEST_H
#define PLUGIN_LIFECYCLE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class PluginLifecycleTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PluginLifecycleTest );
	CPPUNIT_TEST( testRepeatedLifecycle );
	CPPUNIT_TEST( testPluginProcess );
	CPPUNIT_TEST( testPluginStateRoundTrip );
	CPPUNIT_TEST( testPluginRepeatedLifecycle );
	CPPUNIT_TEST_SUITE_END();

public:
	void testRepeatedLifecycle();
	/** The format-agnostic HydrogenPlugin core (which the CLAP/LV2 entries wrap)
	 * activates, processes silence and notes into host master + bus buffers
	 * without NaNs, then deactivates (ADR 0014). */
	void testPluginProcess();
	/** HydrogenPlugin state save → load round-trips through the `.h2project`
	 * codec. */
	void testPluginStateRoundTrip();
	/** Repeated construct/destroy of HydrogenPlugin leaves no residual objects. */
	void testPluginRepeatedLifecycle();
};

#endif
