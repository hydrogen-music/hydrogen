/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#ifndef H2C_STARTUP_TEST_MODES_H
#define H2C_STARTUP_TEST_MODES_H

#include <memory>

#include <QtGlobal>

class Parser;
class MainForm;

// Only used as pointer/reference arguments of declarations (defined in
// StartupTestModes.cpp); a namespace-enclosed forward declaration keeps
// this header free of a QtWidgets include and valid under a static
// QT_NAMESPACE build.
QT_BEGIN_NAMESPACE
class QApplication;
QT_END_NAMESPACE

namespace H2Core {
	class Preferences;
}

/**
 * \ingroup docGUI
 *
 * StartupTestModes — the hidden CLI test modes in one place
 *
 * The hidden CLI options (--quit-after-startup, --exercise-editor-paths,
 * --verify-plugin-transport-gating) turn the GUI into a non-interactive
 * test subject for the out-of-process CTest harnesses (EditorAttach,
 * EditorModeAssertGuard, EditorTransportGating). This file owns their
 * wiring so main() only dispatches here and stays free of test-specific
 * logic.
 */
namespace StartupTestModes {

	/** ADR 0015/0016/0026: non-interactive runs use the no-op Fake audio
	 * driver and suppress the development-build warning, so the engine
	 * starts cleanly and no modal dialog blocks the headless run before
	 * the scheduled quit or verification. Call after the Preferences are
	 * loaded and before the audio engine is created. */
	void applyNonInteractivePreferences(
		const Parser& parser,
		const std::shared_ptr<H2Core::Preferences>& pPref );

	/** Schedules each active mode's post-startup work on the event loop:
	 * the smoke-test quit (one event-loop pass, then clean teardown), the
	 * editor-path exerciser (ADR 0033), and the transport-gating verifier
	 * (ADR 0026). Call once the GUI is fully constructed and shown. */
	void schedulePostStartupWork( const Parser& parser, QApplication* pApp,
								  MainForm* pMainForm );
}

#endif // H2C_STARTUP_TEST_MODES_H
