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

#include "StartupTestModes.h"

#include "EditorPathExerciser.h"
#include "Parser.h"
#include "PluginTransportGatingVerifier.h"
#include "SplashStartupVerifier.h"

#include <core/Preferences/Preferences.h>

#include <QApplication>
#include <QTimer>

void StartupTestModes::applyNonInteractivePreferences(
	const Parser& parser,
	const std::shared_ptr<H2Core::Preferences>& pPref )
{
	if ( parser.getQuitAfterStartup() ||
		 parser.getVerifyPluginTransportGating() ||
		 parser.getVerifySplashShown() ) {
		pPref->m_audioDriver = H2Core::Preferences::AudioDriver::Fake;
		pPref->setShowDevelWarning( false );
	}
}

void StartupTestModes::schedulePostStartupWork( const Parser& parser,
												QApplication* pApp,
												MainForm* pMainForm )
{
	// Startup smoke test (ADR 0015/0016): once the GUI has fully
	// constructed and shown, schedule an immediate quit so the event loop
	// runs one pass and then exits cleanly through the normal teardown. A
	// crash during construction or teardown surfaces as a non-zero exit
	// code.
	if ( parser.getQuitAfterStartup() ) {
		QTimer::singleShot( 0, pApp, &QApplication::quit );
	}

	// Editor-mode assert guard (ADR 0033): exercise all safe shortcut
	// actions through executeShortcut to surface any code path that
	// reaches an ASSERT_NO_EDITOR_MODE site. The exerciser dispatches one
	// action per timer tick, then quits the application. A crash (SIGABRT
	// from assert(false)) shows up as a non-zero/signal exit.
	if ( parser.getExerciseEditorPaths() ) {
		auto* pExerciser = new EditorPathExerciser( pMainForm, pApp );
		QTimer::singleShot( 500, pExerciser, &EditorPathExerciser::start );
	}

	// Plugin-host transport gating verification (ADR 0026): check the
	// transport controls' enablement against the plugin-host predicate
	// cached by the startup syncViaIpc() call, then quit with the verdict
	// (0 = gating correct, 1 = mismatch). Out-of-process like the
	// exerciser above so a CTest can fail on the exit code.
	if ( parser.getVerifyPluginTransportGating() ) {
		auto* pVerifier = new PluginTransportGatingVerifier( pApp );
		QTimer::singleShot( 500, pVerifier,
							&PluginTransportGatingVerifier::start );
	}

	// Splash startup verification: check that the splash screen got its
	// event pass during startup (the sentinel armed and captured in
	// main()) and quit with the verdict (0 = pass, 1 = the splash never
	// became visible). Out-of-process like the verifier above so a
	// CTest can fail on the exit code.
	if ( parser.getVerifySplashShown() ) {
		auto* pVerifier = new SplashStartupVerifier( pApp );
		QTimer::singleShot( 500, pVerifier, &SplashStartupVerifier::start );
	}
}
