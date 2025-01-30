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

#include <functional>
#include <cppunit/extensions/HelperMacros.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

class TransportTestsTimebase : public CppUnit::TestCase {
		CPPUNIT_TEST_SUITE( TransportTestsTimebase );
		CPPUNIT_TEST( testTransportProcessingJack );
		CPPUNIT_TEST( testTransportProcessingOffsetsJack );
		CPPUNIT_TEST( testTransportRelocationJack );
		CPPUNIT_TEST( testTransportRelocationOffsetsJack );
		CPPUNIT_TEST_SUITE_END();

public:
	void testTransportProcessingJack();
	void testTransportProcessingOffsetsJack();
	void testTransportRelocationJack();
	void testTransportRelocationOffsetsJack();

		virtual void tearDown() override {
			// The tests in here tend to produce a very large number of log
			// messages and a couple of them may tend to be printed _after_ the
			// results of the overall test runnner. This is quite unpleasant as
			// the overall result is only shown after scrolling. As the
			// TestRunner itself does not seem to support fixtures, we flush the
			// logger in here.
			H2Core::Logger::get_instance()->flush();
		}


private:
	static void perform( std::function<void()> func );
};
