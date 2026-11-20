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

#ifndef H2C_IPC_TRANSPORT_TEST_H
#define H2C_IPC_TRANSPORT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class IpcTransportTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( IpcTransportTest );
	CPPUNIT_TEST( testHelloHandshakeOverSocket );
	CPPUNIT_TEST( testCommandReachesEngine );
	CPPUNIT_TEST( testProxyCommandReachesEngine );
	CPPUNIT_TEST( testProxyMarshalsParameterCommands );
	CPPUNIT_TEST( testRequestResponseRoundTrip );
	CPPUNIT_TEST( testRequestDropsStaleReply );
	CPPUNIT_TEST( testProxySetSongPayload );
	CPPUNIT_TEST( testProxyObjectPayloadCommands );
	CPPUNIT_TEST( testProxyPlaylistCommands );
	CPPUNIT_TEST( testRescanCommandReachesEngine );
	CPPUNIT_TEST( testEventForwardingOverSocket );
	CPPUNIT_TEST( testTelemetrySharedMemory );
#ifndef WIN32
	CPPUNIT_TEST( testListenClearsStaleSocketFile );
	CPPUNIT_TEST( testListenDoesNotStealBusyEndpoint );
#endif
	CPPUNIT_TEST( testListenDoesNotStealLiveEndpoint );
	CPPUNIT_TEST_SUITE_END();

public:
	void testHelloHandshakeOverSocket();
	void testCommandReachesEngine();
	void testProxyCommandReachesEngine();
	void testProxyMarshalsParameterCommands();
	void testRequestResponseRoundTrip();
	void testRequestDropsStaleReply();
	void testProxySetSongPayload();
	void testProxyObjectPayloadCommands();
	void testProxyPlaylistCommands();
	void testRescanCommandReachesEngine();
	void testEventForwardingOverSocket();
	void testTelemetrySharedMemory();
#ifndef WIN32
	void testListenClearsStaleSocketFile();
	void testListenDoesNotStealBusyEndpoint();
#endif
	void testListenDoesNotStealLiveEndpoint();
};

#endif
