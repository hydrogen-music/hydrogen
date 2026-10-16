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

#ifndef H2C_PLUGIN_UI_FRAME_GATE_TEST_H
#define H2C_PLUGIN_UI_FRAME_GATE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

// The embedded basic UI's frame gate (ADR 0035's amendment): pure logic,
// so every decision the window's idle() relies on is unit-testable
// headless with synthetic timestamps — no clocks, no sleeps.
class PluginUiFrameGateTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PluginUiFrameGateTest );
	CPPUNIT_TEST( testFirstFrameRendersImmediately );
	CPPUNIT_TEST( testUnrequestedTickDoesNotRender );
	CPPUNIT_TEST( testRequestRendersAfterInterval );
	CPPUNIT_TEST( testRequestPersistsUntilRendered );
	CPPUNIT_TEST( testMultipleRequestsCoalesce );
	CPPUNIT_TEST( testRenderedStampAdvances );
	CPPUNIT_TEST_SUITE_END();

public:
	/** A freshly opened window owes its first frame on the very first
	 * tick — unconditionally, or a mapped window could stay blank. */
	void testFirstFrameRendersImmediately();
	/** The gate is request-driven, not a fixed-rate loop: without a
	 * request no tick ever owes a frame, however much time passes. */
	void testUnrequestedTickDoesNotRender();
	/** A request renders on the first tick at or past the interval —
	 * the 27 ms threshold of the 30 Hz ceiling (33 ms minus slack: a
	 * 33 ms gate would beat against the hosts' 30 ms idle timers down
	 * to ~17 Hz). */
	void testRequestRendersAfterInterval();
	/** A request the gate could not serve yet (interval not elapsed,
	 * window not presentable) survives until a tick serves it. */
	void testRequestPersistsUntilRendered();
	/** Requests accumulate: however many arrive between frames, one
	 * rendered frame serves them all. */
	void testMultipleRequestsCoalesce();
	/** The interval measures from the last rendered frame, not from
	 * the request: a frame served late pushes the next opening. */
	void testRenderedStampAdvances();
};

#endif
