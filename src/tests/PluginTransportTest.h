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

#ifndef H2C_PLUGIN_TRANSPORT_TEST_H
#define H2C_PLUGIN_TRANSPORT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/** Host transport sanitization (ADR 0013): the HostTransportLatch policy
 *  in isolation, and the engine's reaction to the sanitized transport
 *  through HydrogenPlugin::process() - jitter must not become relocates,
 *  seeks must.
 */
class PluginTransportTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PluginTransportTest );
	CPPUNIT_TEST( testLatchAbsorbsRollingJitter );
	CPPUNIT_TEST( testLatchAbsorbsParkedJitter );
	CPPUNIT_TEST( testLatchHoldsOverAbsentTransport );
	CPPUNIT_TEST( testLatchClampsNegativeFrames );
	CPPUNIT_TEST( testLatchTempoGating );
	CPPUNIT_TEST( testLatchAdoptsSeek );
	CPPUNIT_TEST( testLatchAdoptsLoopWrap );
	CPPUNIT_TEST( testLatchStopParkAndResume );
	CPPUNIT_TEST( testLatchInvalidUntilTransportSeen );
	CPPUNIT_TEST( testHostJitterDoesNotRelocate );
	CPPUNIT_TEST( testHostSeekRelocatesOnce );
	CPPUNIT_TEST( testTransportlessHostLeavesEngineAlone );
	CPPUNIT_TEST_SUITE_END();

public:
	void testLatchAbsorbsRollingJitter();
	void testLatchAbsorbsParkedJitter();
	void testLatchHoldsOverAbsentTransport();
	void testLatchClampsNegativeFrames();
	void testLatchTempoGating();
	void testLatchAdoptsSeek();
	void testLatchAdoptsLoopWrap();
	void testLatchStopParkAndResume();
	void testLatchInvalidUntilTransportSeen();
	void testHostJitterDoesNotRelocate();
	void testHostSeekRelocatesOnce();
	void testTransportlessHostLeavesEngineAlone();
};

#endif
