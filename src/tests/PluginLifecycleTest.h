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

#ifndef H2C_PLUGIN_LIFECYCLE_TEST_H
#define H2C_PLUGIN_LIFECYCLE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <core/config.h>

class PluginLifecycleTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( PluginLifecycleTest );
	CPPUNIT_TEST( testRepeatedLifecycle );
	CPPUNIT_TEST( testPluginProcess );
	CPPUNIT_TEST( testPluginReset );
	CPPUNIT_TEST( testPluginStateRoundTrip );
	CPPUNIT_TEST( testPluginStatePreferencesRoundTrip );
	CPPUNIT_TEST( testPluginRepeatedLifecycle );
	CPPUNIT_TEST( testEditorOpenServesEngine );
	CPPUNIT_TEST( testEditorCommandReachesEngine );
	CPPUNIT_TEST( testLocalCommandReachesEngine );
	CPPUNIT_TEST( testEditorReopen );
	CPPUNIT_TEST( testEditorBinaryDiscovery );
	CPPUNIT_TEST( testResolveSystemDataPath );
#ifdef __APPLE__
	CPPUNIT_TEST( testEditorBinaryDiscoveryMacPrefixLayout );
	CPPUNIT_TEST( testEditorBinaryDiscoveryMacInstalledBundles );
	CPPUNIT_TEST( testResolveSystemDataPathMacBundle );
#endif
#ifdef H2CORE_HAVE_DEBUG
	CPPUNIT_TEST( testProcessDefaultAuditSurface );
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	void testRepeatedLifecycle();
	/** The format-agnostic HydrogenPlugin core (which the CLAP/LV2 entries wrap)
	 * activates, processes silence and notes into host master + bus buffers
	 * without NaNs, then deactivates (ADR 0014). */
	void testPluginProcess();
	/** The host reset hook (CLAP `reset`, [audio-thread & active]) maps to
	 * CoreActionController::panic(): a live note still sounding after its
	 * block is killed — the sampler holds no playing notes afterwards and
	 * picks up none over subsequent blocks. */
	void testPluginReset();
	/** HydrogenPlugin state save → load round-trips through the `.h2project`
	 * codec. */
	void testPluginStateRoundTrip();
	/** The plugin's base-layer preferences (last-used directories, MIDI
	 * instrument map) ride in the state blob: a restoring instance adopts
	 * them while its host-owned override layer (sample rate, buffer size)
	 * stays its own, and the shared user config file is never written.
	 * Legacy bare-XML song blobs still load and leave the current
	 * preferences untouched. */
	void testPluginStatePreferencesRoundTrip();
	/** Repeated construct/destroy of HydrogenPlugin leaves no residual objects. */
	void testPluginRepeatedLifecycle();
	/** openEditor() serves the instance's engine over IPC; an editor (here an
	 * EditorSession standing in for the real GUI process) attaches and receives
	 * the engine's song; closeEditor() kills the editor process but keeps the
	 * always-on session serving (ADR 0016/0035 UI-3). */
	void testEditorOpenServesEngine();
	/** A command issued on the attached editor reaches and mutates the plugin's
	 * engine. */
	void testEditorCommandReachesEngine();
	/** TU3.4 (ADR 0035 UI-3): the local command path the embedded basic UI
	 * uses — a command issued on the plugin's own engine controller (no
	 * editor round-trip) mutates the authoritative engine state directly. */
	void testLocalCommandReachesEngine();
	/** openEditor()/closeEditor() are idempotent and re-openable. */
	void testEditorReopen();
	/** The editor binary is resolved by precedence: explicit override →
	 * $HYDROGEN_EDITOR_PATH → bundled-next-to-plugin → `hydrogen` on PATH. */
	void testEditorBinaryDiscovery();
	/** The shared system data folder is resolved for plugin use by
	 * precedence: $HYDROGEN_SYS_DATA_PATH → (macOS only, see
	 * #testResolveSystemDataPathMacBundle) the editor's app bundle → empty,
	 * the Filesystem platform default. Never relative to the plugin. */
	void testResolveSystemDataPath();
#ifdef __APPLE__
	/** ADR 0038: in the `<prefix>/bin/hydrogen.app` install layout the
	 * editor is found from `<prefix>/lib/{clap,vst3,lv2}/...`. */
	void testEditorBinaryDiscoveryMacPrefixLayout();
	/** ADR 0038: installed app bundles are used after the relative search;
	 * the executable comes from `CFBundleExecutable`, a bundle matching the
	 * plugin's version is preferred and bundles without an executable are
	 * skipped. */
	void testEditorBinaryDiscoveryMacInstalledBundles();
	/** ADR 0038: the system data folder is `Contents/Resources/data` of the
	 * editor's app bundle; the environment override still wins. */
	void testResolveSystemDataPathMacBundle();
#endif
	/** The process-default log doubles as the audit surface: while
	 * per-instance loggers are alive, every line reaching it is marked
	 * `[unscoped]`. After a full HydrogenPlugin lifecycle (construction,
	 * activation, state round-trip, processing, teardown) the only marked
	 * line must be the deliberate "Spawning instance logger" index
	 * announcement. Debug builds only (same reason as LoggerInstanceTest). */
	void testProcessDefaultAuditSurface();
};

#endif
