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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <functional>

#include <core/config.h>

#include <cppunit/extensions/HelperMacros.h>
#include <core/Basics/Song.h>

class TransportTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( TransportTest );
	CPPUNIT_TEST( testFrameToTickConversion );
	CPPUNIT_TEST( testTransportProcessing );
	CPPUNIT_TEST( testTransportProcessingTimeline );
	CPPUNIT_TEST( testTransportRelocation );
	CPPUNIT_TEST( testLoopMode );
	CPPUNIT_TEST( testSongSizeChange );
	CPPUNIT_TEST( testSongSizeChangeInLoopMode );
#ifndef WIN32
	CPPUNIT_TEST( testPlaybackTrack );
	CPPUNIT_TEST( testSampleConsistency );
#endif
	CPPUNIT_TEST( testNoteEnqueuing );
	CPPUNIT_TEST( testNoteEnqueuingTimeline );
	CPPUNIT_TEST( testMuteGroups );
	CPPUNIT_TEST( testNoteOff );
	CPPUNIT_TEST( testHumanization );
	CPPUNIT_TEST( testUpdateTransport );
	// T3.3 — host-transport follower (ADR 0013)
	CPPUNIT_TEST( testPluginHostTempo );
	CPPUNIT_TEST( testPluginHostTransportState );
	CPPUNIT_TEST( testPluginHostPositionTracking );
	CPPUNIT_TEST( testPluginHostRelocate );
	CPPUNIT_TEST( testPluginHostLoop );
	CPPUNIT_TEST( testPluginHostTempoWinsOverTimeline );
	CPPUNIT_TEST_SUITE_END();
private:
	void perform( std::function<void()> func );

public:
	void setUp();
	void tearDown();
	
	void testFrameToTickConversion();

	void testTransportProcessing();
	void testTransportProcessingTimeline();
	void testTransportRelocation();
	void testLoopMode();
	void testSongSizeChange();
	void testSongSizeChangeInLoopMode();
	/**
	 * Checks whether the playback track is rendered properly and
	 * whether it doesn't get affected by tempo markers.
	 */
	void testPlaybackTrack();
	void testSampleConsistency();
	void testNoteEnqueuing();
	/**
	 * Checks whether the order of notes enqueued and processed by the
	 * Sampler is consistent on tempo change.
	 */
	void testNoteEnqueuingTimeline();
	void testHumanization();
	void testMuteGroups();
	void testNoteOff();
		void testUpdateTransport();

	/** The engine's playhead tempo follows the plugin host's broadcast tempo. */
	void testPluginHostTempo();
	/** Host start/stop drives the engine: rolling advances, stopped freezes. */
	void testPluginHostTransportState();
	/** While the host rolls contiguously the playhead tracks it frame-accurately. */
	void testPluginHostPositionTracking();
	/** A host relocate (frame jump) moves the playhead to the host frame. */
	void testPluginHostRelocate();
	/** A host loop (frame wraps backwards) is followed without hang or stop. */
	void testPluginHostLoop();
	/** With a Timeline present, the host tempo still wins (ADR 0013). */
	void testPluginHostTempoWinsOverTimeline();
};
