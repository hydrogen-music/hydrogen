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

#include <cppunit/extensions/HelperMacros.h>

/** While #TransportTest focus on individual parts of the #H2Core::AudioEngine
 * and #H2Core::Sampler, this test suite is meant to tackle both classes as
 * integration tests with both functional audio and MIDI driver. */
class AudioEngineTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( AudioEngineTest );
	CPPUNIT_TEST( testNotePickup );
	CPPUNIT_TEST_SUITE_END();

   public:
	/** Ensure when playing a song in song mode without looping enabled, the
	 * note at position zero is not picked up twice. */
	void testNotePickup();
};
