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

#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <core/Basics/Song.h>

class CoreActionControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( CoreActionControllerTest );
	CPPUNIT_TEST( testCountIn );
	CPPUNIT_TEST( testSessionManagement );
	CPPUNIT_TEST( testSetPatternSize );
	CPPUNIT_TEST( testEditNoteProperty );
	CPPUNIT_TEST( testAddOrRemoveNote );
	CPPUNIT_TEST( testSetPanLaw );
	CPPUNIT_TEST( testPlaybackTrack );
	CPPUNIT_TEST( testSaveSongDiscardEvent );
	CPPUNIT_TEST( testSetSongPatternSelectionPreservation );
	CPPUNIT_TEST( testSetMidiEventMap );
	CPPUNIT_TEST( testSetMidiInstrumentMap );
	CPPUNIT_TEST( testSetLastMidiEvent );
	CPPUNIT_TEST( testAddRemoveCustomSoundLibraryDir );
	CPPUNIT_TEST_SUITE_END();

public:

		void testCountIn();
		/** Bucket-B write-surface entry points (ADR 0027 / T4.5). */
		void testSetPatternSize();
		void testEditNoteProperty();
		void testAddOrRemoveNote();
		void testSetPanLaw();
		void testPlaybackTrack();

		/** saveSong() with discarded missing samples signals
		 * DrumkitLoaded — the change is confined to the instruments of
		 * the current drumkit — alongside the regular UpdateSong(1),
		 * and never UpdateSong(0), which the GUI treats as a new
		 * document and resets the undo stack for (ADR 0026 point 15). */
		void testSaveSongDiscardEvent();

		/** Hydrogen::setSong() only resets the selected pattern number
		 * when the document changes (path comparison) — a same-path
		 * re-install (the editor mirror applying a pulled copy in the
		 * IPC split) keeps the selection (ADR 0026 point 16). */
		void testSetSongPatternSelectionPreservation();

		/** setMidiEventMap() installs the provided map as the live map
		 * of the current Preferences (ADR 0030). */
		void testSetMidiEventMap();

		/** setMidiInstrumentMap() installs the provided map as the live
		 * map of the current Preferences (ADR 0030). */
		void testSetMidiInstrumentMap();

	/** setLastMidiEvent() sets the engine's MIDI-learn channel —
	 * type and parameter as one pair (ADR 0030). */
	void testSetLastMidiEvent();

	/** addCustomSoundLibraryDir()/removeCustomSoundLibraryDir() keep
	 * the Preferences list and the sound library database in sync
	 * (ADR 0030). */
	void testAddRemoveCustomSoundLibraryDir();

	void testSessionManagement();
};
