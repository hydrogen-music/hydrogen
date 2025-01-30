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

#ifdef H2CORE_HAVE_OSC

#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <lo/lo.h>
#include <lo/lo_cpp.h>

class OscServerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( OscServerTest );
	CPPUNIT_TEST( testSessionManagement );
	CPPUNIT_TEST_SUITE_END();
	
private:
	H2Core::Hydrogen* m_pHydrogen;
	lo::ServerThread* m_pServerThread;
	
	QString m_sValidPath;
	QString m_sValidPath2;
	
public:
	/**
	 * Constructs a liblo server thread in #m_pServerThread, registers
	 * all methods of OscServer relevant for the session management,
	 * and starts the server.
	 *
	 * I'm not quite sure why using the (even a fresh) instance of the
	 * OscServer itself does result in a segmentation fault as soon as
	 * the send() method of a lo::Address is called.
	 *
	 * It will also determine the paths to the files used during
	 * testing.
	 */ 
	void setUp();
	// Calls the destructor of #m_pServerThread and removes all
	// temporarily created files.
	void tearDown();
	
	/**
	 * Uses liblo to send OSC messages to the dummy OSC server
	 * assigned to #m_pServerThread in setUp(). 
	 *
	 * It will test OscServer::generic_handler(),
	 * OscServer::NEW_SONG_Handler(), OscServer::OPEN_SONG_Handler(),
	 * OscServer::SAVE_SONG_Handler(), and
	 * OscServer::SAVE_SONG_AS_Handler().
	 *
	 * Each test will be considered successful if the file name of the
	 * current song does match the expected result.
	 */
	void testSessionManagement();
};

#endif
