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
 * This 1. increases coverage for #MidiEventMap routines too and 2. is more easy to
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
	CPPUNIT_TEST( testPauseAction );
	CPPUNIT_TEST( testPitchLevelAbsoluteAction );
	CPPUNIT_TEST( testPlayAction );
	CPPUNIT_TEST( testPlaylistNextSongAction );
	CPPUNIT_TEST( testPlaylistPrevSongAction );
	CPPUNIT_TEST( testPlaylistSongAction );
	CPPUNIT_TEST( testPlayPauseToggleAction );
	CPPUNIT_TEST( testPlayStopToggleAction );
	CPPUNIT_TEST( testPreviousBarAction );
	CPPUNIT_TEST( testRecordExitAction );
	CPPUNIT_TEST( testRecordReadyAction );
	CPPUNIT_TEST( testRecordStrobeAction );
	CPPUNIT_TEST( testRecordStrobeToggleAction );
	CPPUNIT_TEST( testRedoAction );
	CPPUNIT_TEST( testSelectAndPlayPatternAction );
	CPPUNIT_TEST( testSelectInstrumentAction );
	CPPUNIT_TEST( testSelectNextPatternAction );
	CPPUNIT_TEST( testSelectNextPatternCcAbsoluteAction );
	CPPUNIT_TEST( testSelectNextPatternRelativeAction );
	CPPUNIT_TEST( testSelectOnlyNextPatternAction );
	CPPUNIT_TEST( testSelectOnlyNextPatternCcAbsoluteAction );
	CPPUNIT_TEST( testStopAction );
	CPPUNIT_TEST( testStripMuteToggleAction );
	CPPUNIT_TEST( testStripSoloToggleAction );
	CPPUNIT_TEST( testStripVolumeAbsoluteAction );
	CPPUNIT_TEST( testStripVolumeRelativeAction );
	CPPUNIT_TEST( testTapTempoAction );
	CPPUNIT_TEST( testToggleMetronomeAction );
	CPPUNIT_TEST( testUndoAction );
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
		void testPauseAction();
		void testPitchLevelAbsoluteAction();
		void testPlayAction();
		void testPlaylistNextSongAction();
		void testPlaylistPrevSongAction();
		void testPlaylistSongAction();
		void testPlayPauseToggleAction();
		void testPlayStopToggleAction();
		void testPreviousBarAction();
		void testRecordExitAction();
		void testRecordReadyAction();
		void testRecordStrobeAction();
		void testRecordStrobeToggleAction();
		void testRedoAction();
		void testSelectAndPlayPatternAction();
		void testSelectInstrumentAction();
		void testSelectNextPatternAction();
		void testSelectNextPatternCcAbsoluteAction();
		void testSelectNextPatternRelativeAction();
		void testSelectOnlyNextPatternAction();
		void testSelectOnlyNextPatternCcAbsoluteAction();
		void testStopAction();
		void testStripMuteToggleAction();
		void testStripSoloToggleAction();
		void testStripVolumeAbsoluteAction();
		void testStripVolumeRelativeAction();
		void testTapTempoAction();
		// For MidiAction::Type::TimingClockTick there is a specialized unit
		// test in MidiDriver test.
		void testToggleMetronomeAction();
		void testUndoAction();
		void testUnmuteAction();

	private:
		void sendMessage( const H2Core::MidiMessage& msg );

		H2Core::Preferences::MidiDriver m_previousDriver;
};

#endif
