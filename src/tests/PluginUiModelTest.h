/*
 * Hydrogen
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

#ifndef H2C_PLUGIN_UI_MODEL_TEST_H
#define H2C_PLUGIN_UI_MODEL_TEST_H

#include <cppunit/extensions/HelperMacros.h>

// The embedded basic UI's view models (proposal 0006 UI-4): pure logic,
// no imgui types (TU1.3), so every behavior the renderers rely on is
// unit-testable headless.
class PluginUiModelTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PluginUiModelTest );
	CPPUNIT_TEST( testLandingLoadCommands );
	CPPUNIT_TEST( testLandingMidiConfigNoWriteBack );
	CPPUNIT_TEST( testLandingMidiConfigExposedReadApply );
	CPPUNIT_TEST( testMappingModelRoundTrip );
	CPPUNIT_TEST( testMappingModelInstrumentReads );
	CPPUNIT_TEST( testMappingModelInputMappingRead );
	CPPUNIT_TEST( testChannelDisplayName );
	CPPUNIT_TEST( testMixerModelStripsAndMeters );
	CPPUNIT_TEST( testMixerModelStripReads );
	CPPUNIT_TEST( testMixerModelBusCommands );
	CPPUNIT_TEST_SUITE_END();

public:
	/** TU4.a: the landing model's load/install commands — song, pattern,
	 * drumkit — install loaded objects into the plugin's engine and leave
	 * the current state untouched on unloadable paths; the editor button
	 * forwards to the plugin's editor seam. */
	void testLandingLoadCommands();
	/** TU4.a: MIDI config applies reach the plugin's engine preferences
	 * (ADR 0022 plugin-instance layer) and are never written back to the
	 * user config file. */
	void testLandingMidiConfigNoWriteBack();
	/** TU4.1: the read side populates the three exposed MIDI config
	 * widgets from the engine, and the narrow apply changes exactly
	 * those three while preserving the four settings the plugin UI does
	 * not expose (feedback, transport — engine-side, settable via the
	 * editor). */
	void testLandingMidiConfigExposedReadApply();
	/** TU4.b: the mapping model's setters land in the plugin's engine —
	 * map-wide modes/channels in the live MidiInstrumentMap, per-instrument
	 * custom input rows resolvable through it, per-instrument output
	 * note/channel on the instrument itself. */
	void testMappingModelRoundTrip();
	/** TU4.2: the mapping model's instrument reads — count and name
	 * populate the per-instrument rows from the engine's current kit,
	 * and the output note/channel reads follow the commands. */
	void testMappingModelInstrumentReads();
	/** TU4.2: the index-based custom-row insert and the effective input
	 * mapping read — the mapping view's per-instrument row cells. */
	void testMappingModelInputMappingRead();
	/** UI-4: the channel dropdowns' display convention — "all" for
	 * -1, "Off" for 0 and for the invalid -2 (unmapped rows), the
	 * number itself for 1..16. */
	void testChannelDisplayName();
	/** TU4.c: the mixer model's strip/master commands land on the
	 * instrument/song, and its meter reads serve the latest telemetry
	 * snapshot accepted through the engine-event-sink seam. */
	void testMixerModelStripsAndMeters();
	/** TU4.3: the mixer model's population reads — strip count/name and
	 * the strip/master values follow the commands and the engine state;
	 * the bus count mirrors the host context. */
	void testMixerModelStripReads();
	/** TU4.4: the mixer model's bus commands — explicit mapping lands on
	 * the engine's instrument and round-trips in the embedded song state
	 * (ADR 0019); -1 resets to the implicit 1-to-1 default. */
	void testMixerModelBusCommands();
};

#endif
