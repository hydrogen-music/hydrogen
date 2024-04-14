/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Preferences/Preferences.h>

class TransportTestsTimebase : public CppUnit::TestCase {
		CPPUNIT_TEST_SUITE( TransportTestsTimebase );
		CPPUNIT_TEST( testFrameToTickConversion );
		CPPUNIT_TEST( testTransportProcessing );
		CPPUNIT_TEST( testTransportProcessingTimeline );
		CPPUNIT_TEST( testTransportRelocation );
		CPPUNIT_TEST( testLoopMode );
		CPPUNIT_TEST( testSongSizeChange );
		CPPUNIT_TEST( testSongSizeChangeInLoopMode );
		CPPUNIT_TEST( testPlaybackTrack );
		CPPUNIT_TEST( testSampleConsistency );
		CPPUNIT_TEST( testNoteEnqueuing );
		CPPUNIT_TEST( testNoteEnqueuingTimeline );
		CPPUNIT_TEST( testHumanization );
		CPPUNIT_TEST_SUITE_END();

public:
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

		virtual void tearDown() override {
			// The tests in here tend to produce a very large number of log
			// messages and a couple of them may tend to be printed _after_ the
			// results of the overall test runnner. This is quite unpleasant as
			// the overall result is only shown after scrolling. As the
			// TestRunner itself does not seem to support fixtures, we flush the
			// logger in here.
			H2Core::Logger::get_instance()->flush();

			// Reset to default audio driver config
			auto pPref = H2Core::Preferences::get_instance();
			pPref->m_nBufferSize = 1024;
			pPref->m_nSampleRate = 44100;
		}


private:
	static void perform( std::function<void()> func );
};
