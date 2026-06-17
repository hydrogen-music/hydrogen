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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef IPC_PROTOCOL_TEST_H
#define IPC_PROTOCOL_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class IpcProtocolTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( IpcProtocolTest );
	CPPUNIT_TEST( testCommandRoundTrip );
	CPPUNIT_TEST( testEventRoundTrip );
	CPPUNIT_TEST( testStreamingFraming );
	CPPUNIT_TEST( testHelloHandshake );
	CPPUNIT_TEST( testEventClassification );
	CPPUNIT_TEST( testTelemetryRoundTrip );
	CPPUNIT_TEST( testTelemetryTearFree );
	CPPUNIT_TEST_SUITE_END();

public:
	void testCommandRoundTrip();
	void testEventRoundTrip();
	void testStreamingFraming();
	void testHelloHandshake();
	void testEventClassification();
	void testTelemetryRoundTrip();
	void testTelemetryTearFree();
};

#endif
