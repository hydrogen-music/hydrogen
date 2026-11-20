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

#ifndef H2C_ENGINE_EVENT_SINK_TEST_H
#define H2C_ENGINE_EVENT_SINK_TEST_H

#include <cppunit/extensions/HelperMacros.h>

// The engine-process event/meter fan-out seam (ADR 0035 UI-3): a sink
// registered on the engine session receives the same engine-origin events
// and telemetry snapshots the IPC editor receives — with and without an
// editor attached.
class EngineEventSinkTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( EngineEventSinkTest );
	CPPUNIT_TEST( testFanoutToEditorAndSink );
	CPPUNIT_TEST( testFanoutWithoutEditor );
	CPPUNIT_TEST( testSinkQueueDrainedByIdle );
	CPPUNIT_TEST( testPluginFanoutWithoutEditor );
	CPPUNIT_TEST( testInfoSnapshotFanout );
	CPPUNIT_TEST( testBuildInfoSnapshotDegradedSkips );
	CPPUNIT_TEST( testUnregisterQuiescesInFlight );
	CPPUNIT_TEST( testClearEventSinkKeepsSuccessor );
	CPPUNIT_TEST( testTeardownOnFailedAssert );
	CPPUNIT_TEST_SUITE_END();

public:
	/** TU3.1 (editor case): one drained event fans out to BOTH the attached
	 * editor and a registered in-process sink; one telemetry build feeds BOTH
	 * the SHM block and the sink (meter values equal, not halved). */
	void testFanoutToEditorAndSink();
	/** TU3.1 (no-editor case) + TU3.5: with no editor ever attached, a
	 * registered sink still receives the engine's events; errors are
	 * additionally retained for the ADR 0026 editor-attach replay. */
	void testFanoutWithoutEditor();
	/** TU3.2: an observing sink sees a burst of events complete and in
	 * order while the session idles (no editor); unregistering stops
	 * delivery without loss or duplication. */
	void testSinkQueueDrainedByIdle();
	/** TU3.5: a plugin that never opens its editor still runs its engine
	 * session for its whole lifetime — a sink registered on the plugin
	 * receives the engine's events (basic-UI-only mode). */
	void testPluginFanoutWithoutEditor();
	/** The publish cycle delivers an info snapshot to a registered sink
	 * (ADR 0035 UI-3): the same song-level data the landing info
	 * section shows — identity, mode, counts, playback position —
	 * built on the bridge thread, so the UI never reads the song
	 * itself. */
	void testInfoSnapshotFanout();
	/** A degraded cycle (engine-lock contention for the builder's
	 * small try-lock budget) skips info delivery entirely — nullopt,
	 * never a blank snapshot that would read as "no song". */
	void testBuildInfoSnapshotDegradedSkips();
	/** The unregister-before-destroy contract: setEventSink( nullptr )
	 * drains in-flight callbacks before returning — it must not return
	 * while the bridge thread is still inside a sink callback, or the
	 * caller could destroy the sink out from under the bridge. */
	void testUnregisterQuiescesInFlight();
	/** HydrogenPlugin::clearEventSink is conditional: it unregisters
	 * only the sink still registered — a displaced sink's teardown
	 * must not kill its successor. */
	void testClearEventSinkKeepsSuccessor();
	/** A failed assertion in a session-based sink test must not be
	 * shadowed by a crash: the unwind tears the sink seam down
	 * (unregister, stop, engine delete) instead of destroying the raw
	 * sink while the serve loop still calls it. */
	void testTeardownOnFailedAssert();
};

#endif
