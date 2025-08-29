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

#ifndef MIDI_ACTION_TEST_H
#define MIDI_ACTION_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <core/Preferences/Preferences.h>

namespace H2Core {
	class MidiMessage;
}

/** Runs all available MIDI actions to check for potential crashes or dead
 * locks. These actions are not triggered directly but via incoming MIDI events
 * emulated using the #H2Core::LoopBackMidiDriver - for a more realistic test
 * scenario.
 *
 * Instead of loading a MIDI mapping from a config file, we create it locally.
 * This 1. increases coverage for #MidiMap routines too and 2. is more easy to
 * maintain and comprehend. */
class MidiActionTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( MidiActionTest );
	CPPUNIT_TEST( testBeatCounterAction );
	CPPUNIT_TEST( testBpmCcRelativeAction );
	CPPUNIT_TEST( testBpmDecreaseAction );
	CPPUNIT_TEST( testBpmFineCcRelativeAction );
	CPPUNIT_TEST( testBpmIncreaseAction );
	CPPUNIT_TEST( testClearPatternAction );
	CPPUNIT_TEST( testClearSelectedInstrumentAction );
	CPPUNIT_TEST( testEffectLevelAbsoluteAction );
	CPPUNIT_TEST( testEffectLevelRelativeAction );
	CPPUNIT_TEST( testFilterCutoffLevelAbsoluteAction );
	CPPUNIT_TEST( testGainLevelAbsoluteAction );
	CPPUNIT_TEST( testInstrumentPitchAction );
	CPPUNIT_TEST( testLoadNextDrumkitAction );
	CPPUNIT_TEST( testLoadPrevDrumkitAction );
	CPPUNIT_TEST( testMasterVolumeAbsoluteAction );
	CPPUNIT_TEST( testMasterVolumeRelativeAction );
	CPPUNIT_TEST( testMuteAction );
	CPPUNIT_TEST( testMuteToggleAction );
	CPPUNIT_TEST( testNextBarAction );
	CPPUNIT_TEST( testPanAbsoluteAction );
	CPPUNIT_TEST( testPanAbsoluteSymAction );
	CPPUNIT_TEST( testPanRelativeAction );
	CPPUNIT_TEST( testPitchLevelAbsoluteAction );
	CPPUNIT_TEST( testPreviousBarAction );
	CPPUNIT_TEST( testUnmuteAction );
	CPPUNIT_TEST_SUITE_END();

	public:
		virtual void setUp();
		virtual void tearDown();

		void testBeatCounterAction();
		void testBpmCcRelativeAction();
		void testBpmDecreaseAction();
		void testBpmFineCcRelativeAction();
		void testBpmIncreaseAction();
		void testClearPatternAction();
		void testClearSelectedInstrumentAction();
		void testEffectLevelAbsoluteAction();
		void testEffectLevelRelativeAction();
		void testFilterCutoffLevelAbsoluteAction();
		void testGainLevelAbsoluteAction();
		void testInstrumentPitchAction();
		void testLoadNextDrumkitAction();
		void testLoadPrevDrumkitAction();
		void testMasterVolumeAbsoluteAction();
		void testMasterVolumeRelativeAction();
		void testMuteAction();
		void testMuteToggleAction();
		void testNextBarAction();
		void testPanAbsoluteAction();
		void testPanAbsoluteSymAction();
		void testPanRelativeAction();
		void testPitchLevelAbsoluteAction();
		void testPreviousBarAction();
		void testUnmuteAction();

	private:
		void sendMessage( const H2Core::MidiMessage& msg );

		H2Core::Preferences::MidiDriver m_previousDriver;
};

#endif
