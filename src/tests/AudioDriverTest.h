/*
 * Hydrogen
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

#ifndef AUDIO_DRIVER_TEST_H
#define AUDIO_DRIVER_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <core/Preferences/Preferences.h>

class AudioDriverTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( AudioDriverTest );
	CPPUNIT_TEST( testDriverSwitching );
	CPPUNIT_TEST_SUITE_END();

	public:
		virtual void setUp();
		virtual void tearDown();

		// Check that drivers can be switched without any crashes.
		void testDriverSwitching();

	private:
		int m_nPrevBufferSize;
		H2Core::Preferences::AudioDriver m_prevAudioDriver;
};

#endif
