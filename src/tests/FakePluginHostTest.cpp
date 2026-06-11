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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "FakePluginHostTest.h"

#include "utils/FakePluginHost.h"

void FakePluginHostTest::testConstructAndTeardown() {
	{
		FakePluginHost host;

		CPPUNIT_ASSERT_EQUAL( ( unsigned )44100, host.getSampleRate() );
		CPPUNIT_ASSERT_EQUAL( ( unsigned )1024, host.getBlockSize() );
		CPPUNIT_ASSERT_EQUAL( false, host.isPlaying() );
		CPPUNIT_ASSERT_EQUAL( 120.0f, host.getBpm() );
		CPPUNIT_ASSERT_EQUAL( ( long long )0, host.getFramePosition() );

		// Output buffers are valid and zeroed
		float* pL = host.getOutputL();
		float* pR = host.getOutputR();
		CPPUNIT_ASSERT( pL != nullptr );
		CPPUNIT_ASSERT( pR != nullptr );

		for ( unsigned i = 0; i < host.getBlockSize(); ++i ) {
			CPPUNIT_ASSERT_EQUAL( 0.0f, pL[i] );
			CPPUNIT_ASSERT_EQUAL( 0.0f, pR[i] );
		}

		// MIDI event list starts empty
		CPPUNIT_ASSERT( host.getMidiEvents().empty() );
	}
	// Clean teardown — no leaks
}

void FakePluginHostTest::testProcessZeroFrames() {
	FakePluginHost host;

	// Process zero frames — should return 0 (success)
	int nResult = host.process( 0 );
	CPPUNIT_ASSERT_EQUAL( 0, nResult );

	// Frame position unchanged
	CPPUNIT_ASSERT_EQUAL( ( long long )0, host.getFramePosition() );

	// Last output buffers populated (size matches block size)
	CPPUNIT_ASSERT_EQUAL( ( size_t )host.getBlockSize(), host.getLastOutputL().size() );
	CPPUNIT_ASSERT_EQUAL( ( size_t )host.getBlockSize(), host.getLastOutputR().size() );
}
