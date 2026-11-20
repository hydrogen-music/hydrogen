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

#include "PluginUiFrameGateTest.h"

#include <plugin/ui/PluginUiFrameGate.h>

#include <chrono>

#include "TestHelper.h"

using namespace H2Core;

void PluginUiFrameGateTest::testFirstFrameRendersImmediately()
{
	___INFOLOG( "" );

	PluginUiFrameGate gate( std::chrono::milliseconds( 27 ) );
	CPPUNIT_ASSERT( gate.shouldRender( std::chrono::milliseconds( 0 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiFrameGateTest::testUnrequestedTickDoesNotRender()
{
	___INFOLOG( "" );

	PluginUiFrameGate gate( std::chrono::milliseconds( 27 ) );
	gate.markRendered( std::chrono::milliseconds( 0 ) );
	CPPUNIT_ASSERT( ! gate.shouldRender( std::chrono::milliseconds( 100 ) ) );
	CPPUNIT_ASSERT( ! gate.shouldRender( std::chrono::milliseconds( 1000 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiFrameGateTest::testRequestRendersAfterInterval()
{
	___INFOLOG( "" );

	PluginUiFrameGate gate( std::chrono::milliseconds( 27 ) );
	gate.markRendered( std::chrono::milliseconds( 0 ) );
	gate.request();
	CPPUNIT_ASSERT( ! gate.shouldRender( std::chrono::milliseconds( 26 ) ) );
	CPPUNIT_ASSERT( gate.shouldRender( std::chrono::milliseconds( 28 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiFrameGateTest::testRequestPersistsUntilRendered()
{
	___INFOLOG( "" );

	PluginUiFrameGate gate( std::chrono::milliseconds( 27 ) );
	gate.markRendered( std::chrono::milliseconds( 0 ) );
	gate.request();
	CPPUNIT_ASSERT( gate.shouldRender( std::chrono::milliseconds( 28 ) ) );
	// Not served at 28 — the request must survive later ticks.
	CPPUNIT_ASSERT( gate.shouldRender( std::chrono::milliseconds( 60 ) ) );
	gate.markRendered( std::chrono::milliseconds( 60 ) );
	CPPUNIT_ASSERT( ! gate.shouldRender( std::chrono::milliseconds( 61 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiFrameGateTest::testMultipleRequestsCoalesce()
{
	___INFOLOG( "" );

	PluginUiFrameGate gate( std::chrono::milliseconds( 27 ) );
	gate.markRendered( std::chrono::milliseconds( 0 ) );
	gate.request();
	gate.request();
	gate.request();
	CPPUNIT_ASSERT( gate.shouldRender( std::chrono::milliseconds( 30 ) ) );
	gate.markRendered( std::chrono::milliseconds( 30 ) );
	CPPUNIT_ASSERT( ! gate.shouldRender( std::chrono::milliseconds( 31 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiFrameGateTest::testRenderedStampAdvances()
{
	___INFOLOG( "" );

	PluginUiFrameGate gate( std::chrono::milliseconds( 27 ) );
	gate.markRendered( std::chrono::milliseconds( 0 ) );
	gate.request();
	gate.markRendered( std::chrono::milliseconds( 30 ) );
	gate.request();
	CPPUNIT_ASSERT( ! gate.shouldRender( std::chrono::milliseconds( 33 ) ) );
	CPPUNIT_ASSERT( gate.shouldRender( std::chrono::milliseconds( 58 ) ) );

	___INFOLOG( "passed" );
}
